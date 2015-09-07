/*
 * Copyright 2010-2015 OpenXcom Developers.
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Surface.h"

//#include <fstream>
//#include <vector>
//#include <SDL_endian.h>
//#include <SDL_gfxPrimitives.h>
//#include <SDL_image.h>
//#include <stdlib.h>

//#include "Exception.h"
#include "Language.h"
//#include "Palette.h"
//#include "ShaderDraw.h"
//#include "ShaderMove.h"

#ifdef _WIN32
	#include <malloc.h>
#endif

#if defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR)
	#define _aligned_malloc __mingw_aligned_malloc
	#define _aligned_free   __mingw_aligned_free
#endif

#ifdef __MORPHOS__
	#include <ppcinline/exec.h>
#endif


namespace OpenXcom
{

namespace
{

/**
 * Helper function counting pitch in bytes with 16byte padding.
 * @param bpp	- bytes per pixel
 * @param width	- number of pixel in row
 * @return, pitch in bytes
 */
inline int GetPitch(
		int bpp,
		int width)
{ return ((bpp / 8) * width + 15) & ~0xF; }

/**
 * Helper function creating aligned buffer
 * @param bpp		- bytes per pixel
 * @param width		- number of pixel in row
 * @param height	- number of rows
 * @return, pointer to memory
 */
inline void* NewAligned(
		int bpp,
		int width,
		int height)
{	const int
		pitche = GetPitch(bpp, width),
		total = pitche * height;
	void* buffer = NULL;

#ifndef _WIN32
	#ifdef __MORPHOS__

	buffer = calloc( total, 1 );
	if (buffer == NULL)
	{
		throw Exception("Failed to allocate surface");
	}

	#else
	int rc;
	if ((rc = posix_memalign(&buffer, 16, total)))
	{
		throw Exception(strerror(rc));
	}
	#endif
#else
	// of course Windows has to be difficult about this!
	buffer = _aligned_malloc(total, 16);
	if (buffer == NULL)
	{
		throw Exception("Failed to allocate surface");
	}
#endif

	std::memset(
			buffer,
			0,
			total);

	return buffer; }

/**
 * Helper function release aligned memory
 * @param buffer - buffer to delete
 */
inline void DeleteAligned(void* buffer)
{	if (buffer != NULL)
	{
#ifdef _WIN32
		_aligned_free(buffer);
#else
		free(buffer);
#endif
	} }

}


/**
 * Sets up a blank 8bpp surface with the specified size and position with pure
 * black as the transparent color.
 * @note Surfaces don't have to fill the whole size since their
 * background is transparent, specially subclasses with their own
 * drawing logic, so it just covers the maximum drawing area.
 * @param width		- width in pixels
 * @param height	- height in pixels
 * @param x			- X position in pixels (default 0)
 * @param y			- Y position in pixels (default 0)
 * @param bpp		- bits-per-pixel depth (default 8)
 */
Surface::Surface(
		int width,
		int height,
		int x,
		int y,
		int bpp)
	:
		_x(x),
		_y(y),
		_visible(true),
		_hidden(false),
		_redraw(false),
		_alignedBuffer(NULL)
{
	_alignedBuffer = NewAligned(
							bpp,
							width,
							height);
	_surface = SDL_CreateRGBSurfaceFrom(
									_alignedBuffer,
									width,
									height,
									bpp,
									GetPitch(
										bpp,
										width),
									0,0,0,0);

	if (_surface == NULL)
	{
		throw Exception(SDL_GetError());
	}

	SDL_SetColorKey(
				_surface,
				SDL_SRCCOLORKEY,
				0);

	_crop.x =
	_crop.y = 0;
	_crop.w =
	_crop.h = 0;

	_clear.x =
	_clear.y = 0;
	_clear.w = static_cast<Uint16>(getWidth());
	_clear.h = static_cast<Uint16>(getHeight());
}

/**
 * Performs a deep copy of an existing surface.
 * @param other - reference to a Surface to copy from
 */
Surface::Surface(const Surface& other)
{
	if (other._alignedBuffer) // if native OpenXcom aligned surface
	{
		const Uint8 bpp = other._surface->format->BitsPerPixel;
		const int
			width = other.getWidth(),
			height = other.getHeight(),
			pitche = GetPitch(bpp, width);

		_alignedBuffer = NewAligned(bpp, width, height);
		_surface = SDL_CreateRGBSurfaceFrom(
										_alignedBuffer,
										width,
										height,
										bpp,
										pitche,
										0,0,0,0);

		SDL_SetColorKey(
					_surface,
					SDL_SRCCOLORKEY,
					0);
		// can't call 'setPalette' because it's a virtual function and don't work correctly in constructor
		SDL_SetColors(
					_surface,
					other.getPalette(),
					0,255);

		std::memcpy(
				_alignedBuffer,
				other._alignedBuffer,
				height * pitche);
	}
	else
	{
		_surface = SDL_ConvertSurface(
									other._surface,
									other._surface->format,
									other._surface->flags);
		_alignedBuffer = NULL;
	}

	if (_surface == NULL)
	{
		throw Exception(SDL_GetError());
	}

	_x = other._x;
	_y = other._y;

	_crop.x = other._crop.x;
	_crop.y = other._crop.y;
	_crop.w = other._crop.w;
	_crop.h = other._crop.h;

	_clear.x = other._clear.x;
	_clear.y = other._clear.y;
	_clear.w = other._clear.w;
	_clear.h = other._clear.h;

	_visible = other._visible;
	_hidden = other._hidden;
	_redraw = other._redraw;
}

/**
 * Deletes the surface from memory.
 */
Surface::~Surface() // virtual.
{
	DeleteAligned(_alignedBuffer);
	SDL_FreeSurface(_surface);
}

/**
 * Loads the contents of an X-Com SCR image file into the surface.
 * SCR files are simply uncompressed images containing the palette offset of each pixel.
 * @param file - reference the filename of the SCR image
 * @sa http://www.ufopaedia.org/index.php?title=Image_Formats#SCR_.26_DAT
 */
void Surface::loadScr(const std::string& file)
{
	std::ifstream imgFile( // Load file and put pixels in surface
					file.c_str(),
					std::ios::binary);
	if (imgFile.fail() == true)
	{
		throw Exception(file + " not found");
	}

	std::vector<char> buffer(
						(std::istreambuf_iterator<char>(imgFile)),
						(std::istreambuf_iterator<char>()));

	lock();
	int
		x = 0,
		y = 0;

	for (std::vector<char>::const_iterator
			i = buffer.begin();
			i != buffer.end();
			++i)
	{
		setPixelIterative(&x,&y, *i);
	}
	unlock();
}

/**
 * Loads the contents of an image file of a known format into the surface.
 * @param file - reference the filename of the image
 */
void Surface::loadImage(const std::string& file)
{
	DeleteAligned(_alignedBuffer); // Destroy current surface (will be replaced)

	SDL_FreeSurface(_surface);
	_alignedBuffer = NULL;
	_surface = NULL;

	// SDL only takes UTF-8 filenames
	// so here's an ugly hack to match this ugly reasoning
	const std::string utf8 = Language::wstrToUtf8(Language::fsToWstr(file)); // Load file
	_surface = IMG_Load(utf8.c_str());
	if (_surface == NULL)
	{
		const std::string err = file + ":" + IMG_GetError();
		throw Exception(err);
	}
}

/**
 * Loads the contents of an X-Com SPK image file into
 * the surface. SPK files are compressed with a custom
 * algorithm since they're usually full-screen images.
 * @param file - reference the filename of the SPK image
 * @sa http://www.ufopaedia.org/index.php?title=Image_Formats#SPK
 */
void Surface::loadSpk(const std::string& file)
{
	std::ifstream imgFile( // Load file and put pixels in surface
						file.c_str(),
						std::ios::in | std::ios::binary);
	if (imgFile.fail() == true)
	{
		throw Exception(file + " not found");
	}

	lock();
	Uint16 flag;
	Uint8 value;
	int
		x = 0,
		y = 0;

	while (imgFile.read(
					(char*)&flag,
					sizeof(flag)))
	{
		flag = SDL_SwapLE16(flag);

		if (flag == 65535)
		{
			imgFile.read(
					(char*)&flag,
					sizeof(flag));
			flag = SDL_SwapLE16(flag);

			for (int
					i = 0;
					i < flag * 2;
					++i)
			{
				setPixelIterative(&x,&y, 0);
			}
		}
		else if (flag == 65534)
		{
			imgFile.read(
					(char*)&flag,
					sizeof(flag));
			flag = SDL_SwapLE16(flag);

			for (int
					i = 0;
					i < flag * 2;
					++i)
			{
				imgFile.read(
						(char*)&value,
						1);
				setPixelIterative(&x,&y, value);
			}
		}
	}
	unlock();

	imgFile.close();
}

/**
 * Loads the contents of a TFTD BDY image file into the surface;
 * BDY files are compressed with a custom algorithm.
 * @param file - reference the filename of the BDY image
 * @sa http://www.ufopaedia.org/index.php?title=Image_Formats#BDY
 */
void Surface::loadBdy(const std::string& file)
{
	std::ifstream imgFile( // Load file and put pixels in surface
						file.c_str(),
						std::ios::in | std::ios::binary);
	if (imgFile.fail() == true)
	{
		throw Exception(file + " not found");
	}

	lock();
	Uint8 dataByte;
	int
		pixelCnt,
		x = 0,
		y = 0,
		currentRow = 0;

	while (imgFile.read(
					(char*)&dataByte,
					sizeof(dataByte)))
	{
		if (dataByte >= 129)
		{
			pixelCnt = 257 - static_cast<int>(dataByte);
			imgFile.read(
					(char*)&dataByte,
					sizeof(dataByte));

			currentRow = y;
			for (int
					i = 0;
					i < pixelCnt;
					++i)
			{
				if (currentRow == y) // avoid overscan into next row
					setPixelIterative(&x,&y, dataByte);
			}
		}
		else
		{
			pixelCnt = 1 + static_cast<int>(dataByte);

			currentRow = y;
			for (int
					i = 0;
					i < pixelCnt;
					++i)
			{
				imgFile.read(
						(char*)&dataByte,
						sizeof(dataByte));
				if (currentRow == y) // avoid overscan into next row
					setPixelIterative(&x,&y, dataByte);
			}
		}
	}
	unlock();

	imgFile.close();
}

/**
 * Clears the entire contents of the surface, resulting in a blank image
 * of the specified color (0 for transparent).
 * @param color - the color for the background of the surface (default 0)
 */
void Surface::clear(Uint32 color)
{
	if (_surface->flags & SDL_SWSURFACE)
		std::memset(
				_surface->pixels,
				static_cast<int>(color),
				static_cast<size_t>(_surface->h) * static_cast<size_t>(_surface->pitch));
	else
		SDL_FillRect(
				_surface,
				&_clear,
				color);
}

/**
 * Shifts all the colors in the surface by a set amount.
 * @note This is a common method in 8bpp games to simulate color effects for cheap.
 * @param delta		- amount to shift
 * @param minColor	- minimum color to shift to (default -1)
 * @param maxColor	- maximum color to shift to (default -1)
 * @param mult		- shift multiplier (default 1)
 */
void Surface::offset(
		int delta,
		int minColor,
		int maxColor,
		int mult)
{
	if (delta != 0)
	{
		lock();
		for (int
				x = 0,
					y = 0;
				x < getWidth()
					&& y < getHeight();
				)
		{
			const int pixel = static_cast<int>(getPixelColor(x,y));	// the old color
			int p;													// the new color

			if (delta > 0)
				p = (pixel * mult) + delta;
			else
				p = (pixel + delta) / mult;

			if (minColor != -1
				&& p < minColor)
			{
				p = minColor;
			}
			else if (maxColor != -1
				&& p > maxColor)
			{
				p = maxColor;
			}

			if (pixel > 0)
				setPixelIterative(&x,&y, static_cast<Uint8>(p));
			else
				setPixelIterative(&x,&y, 0);
		}
		unlock();
	}
}

/**
 * Inverts all the colors in the surface according to a middle point.
 * @note Used for effects like shifting a button between pressed and unpressed.
 * @param mid - middle point color
 */
void Surface::invert(Uint8 mid)
{
	lock();
	for (int
			x = 0,
				y = 0;
			x < getWidth()
				&& y < getHeight();
			)
	{
		Uint8 color = getPixelColor(x,y);
		if (color != 0)
			color += static_cast<Uint8>((static_cast<int>(mid) - static_cast<int>(color))) * 2;

		setPixelIterative(&x,&y, color);
	}
	unlock();
}

/**
 * Runs any code the surface needs to keep updating every
 * game cycle like animations and other real-time elements.
 */
void Surface::think() // virtual.
{}

/**
 * Draws the graphic that the surface contains before it gets blitted onto
 * other surfaces.
 * @note The surface is only redrawn if the flag is set by a property change to
 * avoid unnecessary drawing.
 */
void Surface::draw() // virtual.
{
	_redraw = false;
	clear();
}

/**
 * Blits this surface onto another one with its position relative to the
 * top-left corner of the target surface.
 * @note The cropping rectangle controls the portion of the surface that is blitted.
 * @param surface - pointer to surface to blit onto
 */
void Surface::blit(Surface* surface) // virtual.
{
	if (_visible == true
		&& _hidden == false)
	{
		if (_redraw == true)
			draw();

		SDL_Rect* crop;
		SDL_Rect target;

		if (_crop.w == 0
			&& _crop.h == 0)
		{
			crop = NULL;
		}
		else
			crop = &_crop;

		target.x = static_cast<Sint16>(getX());
		target.y = static_cast<Sint16>(getY());

		SDL_BlitSurface(
					_surface,
					crop,
					surface->getSurface(),
					&target);
	}
}

/**
 * Copies the exact contents of another surface onto this one.
 * @note Only the content that would overlap both surfaces is copied in
 * accordance with their positions. This is handy for applying effects over
 * another surface without modifying the original.
 * @param surface - pointer to a Surface to copy from
 */
void Surface::copy(Surface* surface)
{
/*	SDL_BlitSurface uses color matching, and is therefore unreliable
	as a means to copy the contents of one surface to another; instead
	we have to do this manually.

	SDL_Rect from;
	from.w = getWidth();
	from.h = getHeight();
	from.x = getX() - surface->getX();
	from.y = getY() - surface->getY();

	SDL_BlitSurface(
				surface->getSurface(),
				&from,
				_surface,
				NULL); */
	const int
		from_x = getX() - surface->getX(),
		from_y = getY() - surface->getY();

	lock();
	for (int
			x = 0,
				y = 0;
			x < getWidth()
				&& y < getHeight();
			)
	{
		const Uint8 pixel = surface->getPixelColor(
												from_x + x,
												from_y + y);
		setPixelIterative(
						&x,&y,
						pixel);
	}
	unlock();
}

/**
 * Draws a filled rectangle on the surface.
 * @param rect	- pointer to SDL_Rect
 * @param color	- color of the rectangle
 */
void Surface::drawRect(
		SDL_Rect* rect,
		Uint8 color)
{
	SDL_FillRect(
			_surface,
			rect,
			static_cast<Uint32>(color));
}

/**
 * Draws a filled rectangle on the surface.
 * @param x		- X position in pixels
 * @param y		- Y position in pixels
 * @param w		- width in pixels
 * @param h		- height in pixels
 * @param color	- color of the rectangle
 */
void Surface::drawRect(
		Sint16 x,
		Sint16 y,
		Sint16 w,
		Sint16 h,
		Uint8 color)
{
	SDL_Rect rect;

	rect.x = x;
	rect.y = y;
	rect.w = static_cast<Uint16>(w);
	rect.h = static_cast<Uint16>(h);

	SDL_FillRect(
			_surface,
			&rect,
			static_cast<Uint32>(color));
}

/**
 * Draws a line on the surface.
 * @param x1	- start x coordinate in pixels
 * @param y1	- start y coordinate in pixels
 * @param x2	- end x coordinate in pixels
 * @param y2	- end y coordinate in pixels
 * @param color	- color of the line
 */
void Surface::drawLine(
		Sint16 x1,
		Sint16 y1,
		Sint16 x2,
		Sint16 y2,
		Uint8 color)
{
	lineColor(
			_surface,
			x1,y1,
			x2,y2,
			Palette::getRGBA(
						getPalette(),
						color));
}

/**
 * Draws a filled circle on the surface.
 * @param x		- X coordinate in pixels
 * @param y		- Y coordinate in pixels
 * @param r		- radius in pixels
 * @param color	- color of the circle
 */
void Surface::drawCircle(
		Sint16 x,
		Sint16 y,
		Sint16 r,
		Uint8 color)
{
	filledCircleColor(
					_surface,
					x,y,
					r,
					Palette::getRGBA(
								getPalette(),
								color));
}

/**
 * Draws a filled polygon on the surface.
 * @param x		- pointer to (an array of) x coordinate(s)
 * @param y		- pointer to (an array of) y coordinate(s)
 * @param n		- number of points
 * @param color	- color of the polygon
 */
void Surface::drawPolygon(
		Sint16* x,
		Sint16* y,
		size_t n,
		Uint8 color)
{
	filledPolygonColor(
					_surface,
					x,y,
					static_cast<int>(n),
					Palette::getRGBA(
								getPalette(),
								color));
}

/**
 * Draws a textured polygon on the surface.
 * @param x			- pointer to (an array of) x coordinate(s)
 * @param y			- pointer to (an array of) y coordinate(s)
 * @param n			- number of points
 * @param texture	- pointer to texture for polygon
 * @param dx		- X offset of texture relative to the screen
 * @param dy		- Y offset of texture relative to the screen
 */
void Surface::drawTexturedPolygon(
		Sint16* x,
		Sint16* y,
		size_t n,
		Surface* texture,
		int dx,
		int dy)
{
	texturedPolygon(
				_surface,
				x,y,
				static_cast<int>(n),
				texture->getSurface(),
				dx,dy);
}

/**
 * Draws a text string on the surface.
 * @param x		- X coordinate in pixels
 * @param y		- Y coordinate in pixels
 * @param s		- pointer to string of char's to draw
 * @param color	- color of string
 */
void Surface::drawString(
		Sint16 x,
		Sint16 y,
		const char* s,
		Uint8 color)
{
	stringColor(
			_surface,
			x,y,
			s,
			Palette::getRGBA(
						getPalette(),
						color));
}

/**
 * Changes the position of the surface in the X axis.
 * @param x - X position in pixels
 */
void Surface::setX(int x) // virtual.
{
	_x = x;
}

/**
 * Changes the position of the surface in the Y axis.
 * @param y - Y position in pixels
 */
void Surface::setY(int y) // virtual.
{
	_y = y;
}

/**
 * Changes the visibility of the surface.
 * @note An nonvisible surface isn't blitted nor does it receive events.
 * @param visible - visibility (default true)
 */
void Surface::setVisible(bool visible) // virtual.
{
	_visible = visible;
}

/**
 * Returns the visible state of the surface.
 * @return, current visibility
 */
bool Surface::getVisible() const
{
	return _visible;
}

/**
 * Resets the cropping rectangle set for this surface
 * so the whole surface is blitted.
 */
void Surface::resetCrop()
{
	_crop.x =
	_crop.y = 0;
	_crop.w =
	_crop.h = 0;
}

/**
 * Returns the cropping rectangle for this surface.
 * @return, pointer to the cropping rectangle
 */
SDL_Rect* Surface::getCrop()
{
	return &_crop;
}

/**
 * Replaces a certain amount of colors in the surface's palette.
 * @param colors		- pointer to the set of colors
 * @param firstcolor	- offset of the first color to replace (default 0)
 * @param ncolors		- amount of colors to replace (default 256)
 */
void Surface::setPalette( // virtual.
		SDL_Color* colors,
		int firstcolor,
		int ncolors)
{
	if (_surface->format->BitsPerPixel == 8)
		SDL_SetColors(
					_surface,
					colors,
					firstcolor,
					ncolors);
}

/**
 * This is a separate visibility setting intended for temporary effects like
 * window popups so as to not override the default visibility setting.
 * @note Do not confuse with setVisible!
 * @param hidden - shown or hidden (default true)
 */
void Surface::setHidden(bool hidden)
{
	_hidden = hidden;
}

/**
 * Locks the surface from outside access for pixel-level access.
 * @note Must be unlocked afterwards.
 * @sa unlock()
 */
void Surface::lock()
{
	SDL_LockSurface(_surface);
}

/**
 * Unlocks the surface after it's been locked to resume blitting operations.
 * @sa lock()
 */
void Surface::unlock()
{
	SDL_UnlockSurface(_surface);
}


/**
 * helper class used for Surface::blitNShade
 */
struct ColorReplace
{

/**
* Function used by ShaderDraw in Surface::blitNShade
* set shade and replace color in that surface
* @param dest		- destination pixel
* @param src		- source pixel
* @param shade		- value of shade of this surface
* @param newColor	- new color to set (it should be offseted by 4)
* @param - notused
*/
static inline void func(
		Uint8& dest,
		const Uint8& src,
		const int& shade,
		const int& newColor,
		const int&)
{
	if (src != 0)
	{
		const int newShade = static_cast<int>(src & 15) + shade;
		if (newShade > 15) // so dark it would flip over to another color - make it black instead
			dest = 15;
		else
			dest = static_cast<Uint8>(newColor | newShade);
	}
}

};


/**
 * helper class used for Surface::blitNShade
 */
struct StandardShade
{

/**
* Function used by ShaderDraw in Surface::blitNShade
* set shade
* @param dest	- destination pixel
* @param src	- source pixel
* @param shade	- value of shade of this surface
* @param - notused
* @param - notused
*/
static inline void func(
		Uint8& dest,
		const Uint8& src,
		const int& shade,
		const int&,
		const int&)
{
	if (src != 0)
	{
		const int newShade = static_cast<int>(src & 15) + shade;
		if (newShade > 15) // so dark it would flip over to another color - make it black instead
			dest = 15;
		else
			dest = static_cast<Uint8>((static_cast<int>(src) & (15 << 4)) | newShade);
	}
}

};


/**
 * Specific blit function to blit battlescape terrain data in different shades
 * in a fast way.
 * @note There is no surface locking here - you have to make sure to lock the
 * surface at the start of blitting and unlock it when done.
 * @param surface		- Surface to blit to
 * @param x				- X position of Surface blitted to
 * @param y				- Y position of Surface blitted to
 * @param colorOffset	- color offset (generally 0-15)
 * @param halfRight		- some tiles blit only the right half (default false)
 * @param colorGroup	- Attention: the actual colorblock + 1 because 0 is no new base color (default 0)
 * @param halfLeft		- kL_add: blits only the left half - NOTE This conflicts w/ 'halfRight' (default false)
 *						  but i am far too lazy to refactor a gajillion blitNShade calls!
 */
void Surface::blitNShade(
		Surface* surface,
		int x,
		int y,
		int colorOffset,
		bool halfRight,
		int colorGroup,
		bool halfLeft)
{
	ShaderMove<Uint8> src (this, x,y); // init.

	if (halfRight == true)
	{
		GraphSubset graph (src.getDomain()); // init.
		graph.beg_x = graph.end_x / 2;
		src.setDomain(graph);
	}
	else if (halfLeft == true) // kL_add->
	{
		GraphSubset graph (src.getDomain()); // init.
		graph.end_x = graph.end_x / 2;
		src.setDomain(graph);
	}

	if (colorGroup != 0)
	{
		--colorGroup;
		colorGroup <<= 4;
		ShaderDraw<ColorReplace>(
							ShaderSurface(surface),
							src,
							ShaderScalar(colorOffset),
							ShaderScalar(colorGroup));
	}
	else
		ShaderDraw<StandardShade>(
							ShaderSurface(surface),
							src,
							ShaderScalar(colorOffset));
}

/**
 * Set the surface to be redrawn.
 * @param redraw - true means redraw (default true)
 */
void Surface::invalidate(bool redraw)
{
	_redraw = redraw;
}

/*
 * Returns the help description of this surface for showing in tooltips eg.
 * @return, string ID
 *
std::string Surface::getTooltip() const
{
	return _tooltip;
} */

/*
* Changes the help description of this surface for showing in tooltips eg.
* @param tooltip - reference a string ID
*
void Surface::setTooltip(const std::string& tooltip)
{
	_tooltip = tooltip;
} */

/**
 * Recreates the surface with a new size.
 * @note Old contents will not be altered and may be cropped to fit the new size.
 * @param width Width in pixels.
 * @param height Height in pixels.
 */
void Surface::resize(
		int width,
		int height)
{
	Uint8 bpp = _surface->format->BitsPerPixel; // Set up new surface
	int pitche = GetPitch(bpp, width);
	void* alignedBuffer = NewAligned(
								bpp,
								width,
								height);
	SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(
												alignedBuffer,
												width,
												height,
												bpp,
												pitche,
												0,0,0,0);
	if (surface == NULL)
	{
		throw Exception(SDL_GetError());
	}


	SDL_SetColorKey( // Copy old contents
				surface,
				SDL_SRCCOLORKEY,
				0);
	SDL_SetColors(
				surface,
				getPalette(),
				0,
				256);
	SDL_BlitSurface(
				_surface,
				NULL,
				surface,
				NULL);


	DeleteAligned(_alignedBuffer); // Delete old surface
	SDL_FreeSurface(_surface);

	_alignedBuffer = alignedBuffer;
	_surface = surface;

	_clear.w = static_cast<Uint16>(getWidth());
	_clear.h = static_cast<Uint16>(getHeight());
}

/**
 * Changes the width of the surface.
 * @warning This is not a trivial setter! It will force the surface to be
 * recreated for the new size.
 * @param width - new width in pixels
 */
void Surface::setWidth(int width) // virtual.
{
	resize(
		width,
		getHeight());

	_redraw = true;
}

/**
 * Changes the height of the surface.
 * @warning This is not a trivial setter! It will force the surface to be
 * recreated for the new size.
 * @param height - new height in pixels
 */
void Surface::setHeight(int height) // virtual.
{
	resize(
		getWidth(),
		height);

	_redraw = true;
}

}
