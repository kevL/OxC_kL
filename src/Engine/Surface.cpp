/*
 * Copyright 2010-2014 OpenXcom Developers.
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

#include <fstream>
#include <vector>

#include <SDL_endian.h>
#include <SDL_gfxPrimitives.h>
#include <SDL_image.h>
#include <stdlib.h>

#include "Exception.h"
#include "Language.h"
#include "Palette.h"
#include "Screen.h"
#include "ShaderDraw.h"
#include "ShaderMove.h"
#include "Surface.h"

#ifdef _WIN32
#include <malloc.h>
#endif

#if defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR)
#define _aligned_malloc __mingw_aligned_malloc
#define _aligned_free   __mingw_aligned_free
#endif //MINGW

#ifdef __MORPHOS__
#include <ppcinline/exec.h>
#endif


namespace OpenXcom
{

namespace
{

/**
 * Helper function counting pitch in bytes with 16byte padding
 * @param bpp bytes per pixel
 * @param width number of pixel in row
 * @return pitch in bytes
 */
inline int GetPitch(
		int bpp,
		int width)
{
	return ((bpp / 8) * width + 15) & ~0xF;
}

/**
 * Helper function creating aligned buffer
 * @param bpp bytes per pixel
 * @param width number of pixel in row
 * @param height number of rows
 * @return pointer to memory
 */
inline void* NewAligned(
		int bpp,
		int width,
		int height)
{
	const int pitch = GetPitch(bpp, width);
	const int total = pitch * height;
	void* buffer = 0;

#ifndef _WIN32
	#ifdef __MORPHOS__

	buffer = calloc( total, 1 );
	if (!buffer)
	{
		throw Exception("Where's the memory, Lebowski?");
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
	if (!buffer)
	{
		throw Exception("Where's the memory, Lebowski?");
	}
#endif

	memset(buffer, 0, total);

	return buffer;
}

/**
 * Helper function release aligned memory
 * @param buffer buffer to delete
 */
inline void DeleteAligned(void* buffer)
{
	if (buffer)
	{
#ifdef _WIN32
		_aligned_free(buffer);
#else
		free(buffer);
#endif
	}
}

} //namespace


/**
 * Sets up a blank 8bpp surface with the specified size and position,
 * with pure black as the transparent color.
 * @note Surfaces don't have to fill the whole size since their
 * background is transparent, specially subclasses with their own
 * drawing logic, so it just covers the maximum drawing area.
 * @param width Width in pixels.
 * @param height Height in pixels.
 * @param x X position in pixels.
 * @param y Y position in pixels.
 * @param bpp Bits-per-pixel depth.
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
		_alignedBuffer(0)
{
	_alignedBuffer = NewAligned(bpp, width, height);
	_surface = SDL_CreateRGBSurfaceFrom(
								_alignedBuffer,
								width,
								height,
								bpp,
								GetPitch(
									bpp,
									width),
								0,
								0,
								0,
								0);

	if (_surface == 0)
	{
		throw Exception(SDL_GetError());
	}

	SDL_SetColorKey(_surface, SDL_SRCCOLORKEY, 0);

	_crop.w = 0;
	_crop.h = 0;
	_crop.x = 0;
	_crop.y = 0;

	_dx = Screen::getDX();
	_dy = Screen::getDY();
}

/**
 * Performs a deep copy of an existing surface.
 * @param other Surface to copy from.
 */
Surface::Surface(const Surface& other)
{
//	Log(LOG_INFO) << "Create Surface 2";

	if (other._alignedBuffer) // if native OpenXcom aligned surface
	{
		Uint8 bpp = other._surface->format->BitsPerPixel;
		int width = other.getWidth();
		int height = other.getHeight();
		int pitch = GetPitch(bpp, width);

		_alignedBuffer = NewAligned(bpp, width, height);
		_surface = SDL_CreateRGBSurfaceFrom(
										_alignedBuffer,
										width,
										height,
										bpp,
										pitch,
										0,
										0,
										0,
										0);

		SDL_SetColorKey(_surface, SDL_SRCCOLORKEY, 0);
		//can't call 'setPalette' because it's a vitual function and it don't work correctly in constructor
		SDL_SetColors(
				_surface,
				other.getPalette(),
				0,
				255);

		memcpy(
			_alignedBuffer,
			other._alignedBuffer,
			height * pitch);
	}
	else
	{
		_surface = SDL_ConvertSurface(
									other._surface,
									other._surface->format,
									other._surface->flags);
		_alignedBuffer = 0;
	}

	if (_surface == 0)
	{
		throw Exception(SDL_GetError());
	}

	_x = other._x;
	_y = other._y;

	_crop.w = other._crop.w;
	_crop.h = other._crop.h;
	_crop.x = other._crop.x;
	_crop.y = other._crop.y;

	_visible = other._visible;
	_hidden = other._hidden;
	_redraw = other._redraw;

	_dx = other._dx;
	_dy = other._dy;
}

/**
 * Deletes the surface from memory.
 */
Surface::~Surface()
{
//	Log(LOG_INFO) << "Delete Surface";

	DeleteAligned(_alignedBuffer);

	SDL_FreeSurface(_surface);
}

/**
 * Loads the contents of an X-Com SCR image file into
 * the surface. SCR files are simply uncompressed images
 * containing the palette offset of each pixel.
 * @param filename Filename of the SCR image.
 * @sa http://www.ufopaedia.org/index.php?title=Image_Formats#SCR_.26_DAT
 */
void Surface::loadScr(const std::string& filename)
{
	// Load file and put pixels in surface
	std::ifstream imgFile(filename.c_str(), std::ios::binary);
	if (!imgFile)
	{
		throw Exception(filename + " not found");
	}

	std::vector<char> buffer(
						(std::istreambuf_iterator<char>(imgFile)),
						(std::istreambuf_iterator<char>()));

	// Lock the surface
	lock();

	int
		x = 0,
		y = 0;

	for (std::vector<char>::iterator
			i = buffer.begin();
			i != buffer.end();
			++i)
	{
		setPixelIterative(&x, &y, *i);
	}

	// Unlock the surface
	unlock();
}

/**
 * Loads the contents of an image file of a known format into the surface.
 * @param filename Filename of the image.
 */
void Surface::loadImage(const std::string& filename)
{
	// Destroy current surface (will be replaced)
	DeleteAligned(_alignedBuffer);

	SDL_FreeSurface(_surface);
	_alignedBuffer = 0;
	_surface = 0;

	// SDL only takes UTF-8 filenames
	// so here's an ugly hack to match this ugly reasoning
	std::string utf8 = Language::wstrToUtf8(Language::fsToWstr(filename));

	// Load file
	_surface = IMG_Load(utf8.c_str());
	if (!_surface)
	{
		std::string err = filename + ":" + IMG_GetError();
		throw Exception(err);
	}
}

/**
 * Loads the contents of an X-Com SPK image file into
 * the surface. SPK files are compressed with a custom
 * algorithm since they're usually full-screen images.
 * @param filename Filename of the SPK image.
 * @sa http://www.ufopaedia.org/index.php?title=Image_Formats#SPK
 */
void Surface::loadSpk(const std::string& filename)
{
	// Load file and put pixels in surface
	std::ifstream imgFile(
						filename.c_str(),
						std::ios::in | std::ios::binary);
	if (!imgFile)
	{
		throw Exception(filename + " not found");
	}

	// Lock the surface
	lock();

	Uint16 flag;
	Uint8 value;
	int
		x = 0,
		y = 0;

	while (imgFile.read(
					(char*)& flag,
					sizeof(flag)))
	{
		flag = SDL_SwapLE16(flag);

		if (flag == 65535)
		{
			imgFile.read(
					(char*)& flag,
					sizeof(flag));
			flag = SDL_SwapLE16(flag);

			for (int
					i = 0;
					i < flag * 2;
					++i)
			{
				setPixelIterative(&x, &y, 0);
			}
		}
		else if (flag == 65534)
		{
			imgFile.read(
					(char*)& flag,
					sizeof(flag));
			flag = SDL_SwapLE16(flag);

			for (int
					i = 0;
					i < flag * 2;
					++i)
			{
				imgFile.read(
							(char*)& value,
							1);
				setPixelIterative(&x, &y, value);
			}
		}
	}

	// Unlock the surface
	unlock();

	imgFile.close();
}

/**
 * Loads the contents of a TFTD BDY image file into the surface;
 * BDY files are compressed with a custom algorithm.
 * @param filename Filename of the BDY image.
 * @sa http://www.ufopaedia.org/index.php?title=Image_Formats#BDY
 */
void Surface::loadBdy(const std::string& filename)
{
	// Load file and put pixels in surface
	std::ifstream imgFile(
						filename.c_str(),
						std::ios::in | std::ios::binary);
	if (!imgFile)
	{
		throw Exception(filename + " not found");
	}

	// Lock the surface
	lock();

	Uint8 dataByte;
	int pixelCnt;
	int
		x = 0,
		y = 0,
		currentRow = 0;

	while (imgFile.read(
					(char*)& dataByte,
					sizeof(dataByte)))
	{
		if (dataByte >= 129)
		{
			pixelCnt = 257 - static_cast<int>(dataByte);
			imgFile.read(
					(char*)& dataByte,
					sizeof(dataByte));

			currentRow = y;
			for (int
					i = 0;
					i < pixelCnt;
					++i)
			{
				if (currentRow == y) // avoid overscan into next row
					setPixelIterative(&x, &y, dataByte);
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
						(char*)& dataByte,
						sizeof(dataByte));
				if (currentRow == y) // avoid overscan into next row
					setPixelIterative(&x, &y, dataByte);
			}
		}
	}

	// Unlock the surface
	unlock();

	imgFile.close();
}

/**
 * Clears the entire contents of the surface, resulting in a blank image.
 */
void Surface::clear()
{
	SDL_Rect square;
	square.x = 0;
	square.y = 0;
	square.w = getWidth();
	square.h = getHeight();

	if (_surface->flags & SDL_SWSURFACE)
	{
		memset(
			_surface->pixels,
			0,
			_surface->h * _surface->pitch);
	}
	else
		SDL_FillRect(
				_surface,
				&square,
				0);
}

/**
 * Shifts all the colors in the surface by a set amount.
 * This is a common method in 8bpp games to simulate color effects for cheap.
 * @param off, Amount to shift.
 * @param min, Minimum color to shift to.
 * @param max, Maximum color to shift to.
 * @param mult, Shift multiplier.
 */
void Surface::offset(
		int off,
		int min,
		int max,
		int mult)
{
	if (off == 0) return;

	lock(); // Lock the surface
	for (int
			x = 0,
				y = 0;
			x < getWidth()
				&& y < getHeight();
			)
	{
		Uint8 pixel = getPixel(x, y);	// getPixelColor
		int p;							// the new color

		if (off > 0)
			p = (pixel * mult) + off;
		else
			p = (pixel + off) / mult;

		if (min != -1
			&& p < min)
		{
			p = min;
		}
		else if (max != -1
			&& p > max)
		{
			p = max;
		}

		if (pixel > 0)
			setPixelIterative(&x, &y, p);
		else
			setPixelIterative(&x, &y, 0);
	}
	unlock(); // Unlock the surface
}

/**
 * Inverts all the colors in the surface according to a middle point.
 * Used for effects like shifting a button between pressed and unpressed.
 * @param mid, Middle point.
 */
void Surface::invert(Uint8 mid)
{
	// Lock the surface
	lock();

	for (int
			x = 0,
				y = 0;
			x < getWidth()
				&& y < getHeight();
			)
	{
		Uint8 pixel = getPixel(x, y);
		if (pixel > 0)
			setPixelIterative(
							&x,
							&y,
							pixel +
								2 * (static_cast<int>(mid) - static_cast<int>(pixel)));
		else
			setPixelIterative(&x, &y, 0);
	}

	// Unlock the surface
	unlock();
}

/**
 * Runs any code the surface needs to keep updating every
 * game cycle, like animations and other real-time elements.
 */
void Surface::think()
{
}

/**
 * Draws the graphic that the surface contains before it
 * gets blitted onto other surfaces. The surface is only
 * redrawn if the flag is set by a property change, to
 * avoid unecessary drawing.
 */
void Surface::draw()
{
	_redraw = false;

	clear();
}

/**
 * Blits this surface onto another one, with its position
 * relative to the top-left corner of the target surface.
 * The cropping rectangle controls the portion of the surface
 * that is blitted.
 * @param surface Pointer to surface to blit onto.
 */
void Surface::blit(Surface* surface)
{
//	Log(LOG_INFO) << "blit()";	// kL

	if (_visible
		&& !_hidden)
	{
		if (_redraw)
			draw();

		SDL_Rect* cropper;
		SDL_Rect target;

		if (_crop.w == 0
			&& _crop.h == 0)
		{
			cropper = 0;
		}
		else
			cropper = &_crop;

		target.x = getX();
		target.y = getY();

		SDL_BlitSurface(
					_surface,
					cropper,
					surface->getSurface(),
					&target);
	}
}

/**
 * Copies the exact contents of another surface onto this one.
 * Only the content that would overlap both surfaces is copied, in
 * accordance with their positions. This is handy for applying
 * effects over another surface without modifying the original.
 * @param surface Pointer to surface to copy from.
 */
void Surface::copy(Surface* surface)
{
	SDL_Rect from;
	from.x = getX() - surface->getX();
	from.y = getY() - surface->getY();
	from.w = getWidth();
	from.h = getHeight();

	SDL_BlitSurface(
				surface->getSurface(),
				&from,
				_surface,
				0);
}

/**
 * Draws a filled rectangle on the surface.
 * @param rect Pointer to Rect.
 * @param color Color of the rectangle.
 */
void Surface::drawRect(
		SDL_Rect* rect,
		Uint8 color)
{
	SDL_FillRect(
			_surface,
			rect,
			color);
}

/**
 * Draws a line on the surface.
 * @param x1 Start x coordinate in pixels.
 * @param y1 Start y coordinate in pixels.
 * @param x2 End x coordinate in pixels.
 * @param y2 End y coordinate in pixels.
 * @param color Color of the line.
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
			x1,
			y1,
			x2,
			y2,
			Palette::getRGBA(
				getPalette(),
				color));
}

/**
 * Draws a filled circle on the surface.
 * @param x X coordinate in pixels.
 * @param y Y coordinate in pixels.
 * @param r Radius in pixels.
 * @param color Color of the circle.
 */
void Surface::drawCircle(
		Sint16 x,
		Sint16 y,
		Sint16 r,
		Uint8 color)
{
	filledCircleColor(
					_surface,
					x,
					y,
					r,
					Palette::getRGBA(
						getPalette(),
						color));
}

/**
 * Draws a filled polygon on the surface.
 * @param x Array of x coordinates.
 * @param y Array of y coordinates.
 * @param n Number of points.
 * @param color Color of the polygon.
 */
void Surface::drawPolygon(
		Sint16* x,
		Sint16* y,
		int n,
		Uint8 color)
{
	filledPolygonColor(
					_surface,
					x,
					y,
					n,
					Palette::getRGBA(
						getPalette(),
						color));
}

/**
 * Draws a textured polygon on the surface.
 * @param x Array of x coordinates.
 * @param y Array of y coordinates.
 * @param n Number of points.
 * @param texture Texture for polygon.
 * @param dx X offset of texture relative to the screen.
 * @param dy Y offset of texture relative to the screen.
 */
void Surface::drawTexturedPolygon(
		Sint16* x,
		Sint16* y,
		int n,
		Surface* texture,
		int dx,
		int dy)
{
	texturedPolygon(
				_surface,
				x,
				y,
				n,
				texture->getSurface(),
				dx,
				dy);
}

/**
 * Draws a text string on the surface.
 * @param x X coordinate in pixels.
 * @param y Y coordinate in pixels.
 * @param s Character string to draw.
 * @param color Color of string.
 */
void Surface::drawString(
		Sint16 x,
		Sint16 y,
		const char* s,
		Uint8 color)
{
	stringColor(
			_surface,
			x,
			y,
			s,
			Palette::getRGBA(
				getPalette(),
				color));
}

/**
 * Changes the position of the surface in the X axis.
 * @param x X position in pixels.
 */
void Surface::setX(int x)
{
	_x = x;
}

/**
 * Returns the position of the surface in the X axis.
 * @return X position in pixels.
 */
int Surface::getX() const
{
	return _x;
}

/**
 * Changes the position of the surface in the Y axis.
 * @param y Y position in pixels.
 */
void Surface::setY(int y)
{
	_y = y;
}

/**
 * Returns the position of the surface in the Y axis.
 * @return Y position in pixels.
 */
int Surface::getY() const
{
	return _y;
}

/**
 * Changes the visibility of the surface. A hidden surface
 * isn't blitted nor receives events.
 * @param visible New visibility.
 */
void Surface::setVisible(bool visible)
{
	_visible = visible;
}

/**
 * Returns the visible state of the surface.
 * @return Current visibility.
 */
bool Surface::getVisible() const
{
	return _visible;
}

/**
 * Resets the cropping rectangle set for this surface,
 * so the whole surface is blitted.
 */
void Surface::resetCrop()
{
	_crop.w = 0;
	_crop.h = 0;
	_crop.x = 0;
	_crop.y = 0;
}

/**
 * Returns the cropping rectangle for this surface.
 * @return Pointer to the cropping rectangle.
 */
SDL_Rect* Surface::getCrop()
{
	return &_crop;
}

/**
 * Replaces a certain amount of colors in the surface's palette.
 * @param colors Pointer to the set of colors.
 * @param firstcolor Offset of the first color to replace.
 * @param ncolors Amount of colors to replace.
 */
void Surface::setPalette(
		SDL_Color* colors,
		int firstcolor,
		int ncolors)
{
	SDL_SetColors(
				_surface,
				colors,
				firstcolor,
				ncolors);
}

/**
 * This is a separate visibility setting intended
 * for temporary effects like window popups,
 * so as to not override the default visibility setting.
 * @note Do not confuse with setVisible!
 * @param hidden Shown or hidden.
 */
void Surface::setHidden(bool hidden)
{
	_hidden = hidden;
}

/**
 * Locks the surface from outside access
 * for pixel-level access. Must be unlocked
 * afterwards.
 * @sa unlock()
 */
void Surface::lock()
{
	SDL_LockSurface(_surface);
}

/**
 * Unlocks the surface after it's been locked
 * to resume blitting operations.
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
* @param dest destination pixel
* @param src source pixel
* @param shade value of shade of this surface
* @param newColor new color to set (it should be offseted by 4)
*/
static inline void func(
		Uint8& dest,
		const Uint8& src,
		const int& shade,
		const int& newColor,
		const int&)
{
	if (src)
	{
		const int newShade = (src & 15) + shade;
		if (newShade > 15)
			// so dark it would flip over to another color - make it black instead
			dest = 15;
		else
			dest = newColor | newShade;
	}
}
};


/**
 * helper class used for Surface::blitNShade
 */
struct StandartShade
{
/**
* Function used by ShaderDraw in Surface::blitNShade
* set shade
* @param dest destination pixel
* @param src source pixel
* @param shade value of shade of this surface
* @param notused
* @param notused
*/
static inline void func(
		Uint8& dest,
		const Uint8& src,
		const int& shade,
		const int&,
		const int&)
{
	if (src)
	{
		const int newShade = (src & 15) + shade;
		if (newShade > 15)
			// so dark it would flip over to another color - make it black instead
			dest = 15;
		else
			dest = (src & (15 << 4)) | newShade;
	}
}
};


/**
 * Specific blit function to blit battlescape terrain data in different shades in a fast way.
 * Notice there is no surface locking here - you have to make sure you lock the surface yourself
 * at the start of blitting and unlock it when done.
 * @param surface to blit to
 * @param x
 * @param y
 * @param off
 * @param half some tiles are blitted only the right half
 * @param newBaseColor Attention: the actual color + 1, because 0 is no new base color.
 */
void Surface::blitNShade(
		Surface* surface,
		int x,
		int y,
		int off,
		bool half,
		int newBaseColor)
{
	ShaderMove<Uint8> src(this, x, y);

	if (half)
	{
		GraphSubset g = src.getDomain();
		g.beg_x = g.end_x/2;
		src.setDomain(g);
	}

	if (newBaseColor)
	{
		--newBaseColor;
		newBaseColor <<= 4;
		ShaderDraw<ColorReplace>(
							ShaderSurface(surface),
							src,
							ShaderScalar(off),
							ShaderScalar(newBaseColor));
	}
	else
		ShaderDraw<StandartShade>(
							ShaderSurface(surface),
							src,
							ShaderScalar(off));
}

/**
 * Set the surface to be redrawn
 */
void Surface::invalidate()
{
	_redraw = true;
}

/**
 *
 */
void Surface::setDX(int dx)
{
	_dx = dx;
}

/**
 *
 */
void Surface::setDY(int dy)
{
	_dy = dy;
}

/**
 * Returns the help description of this surface,
 * for example for showing in tooltips.
 * @return String ID.
 */
/* kL: not used
std::string Surface::getTooltip() const
{
	return _tooltip;
} */

/**
* Changes the help description of this surface,
* for example for showing in tooltips.
* @param str String ID.
*/
/* kL: not used
void Surface::setTooltip(const std::string& tooltip)
{
	_tooltip = tooltip;
} */

}
