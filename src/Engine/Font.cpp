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

//#include "Font.h"

//#include "CrossPlatform.h"
#include "DosFont.h"
#include "Language.h"
//#include "Logger.h"
#include "Surface.h"


namespace OpenXcom
{

std::wstring Font::_index;

SDL_Color Font::_palette[] =
{
	{  0,   0,   0,   0},
	{255, 255, 255, 255},
	{207, 207, 207, 255},
	{159, 159, 159, 255},
	{111, 111, 111, 255},
	{ 63,  63,  63, 255}
};


/**
 * Initializes the font with a blank surface.
 */
Font::Font()
	:
		_surface(0),
		_width(0),
		_height(0),
		_spacing(0),
		_monospace(false)
{}

/**
 * Deletes the font's surface.
 */
Font::~Font()
{
	delete _surface;
}

/**
 * Loads the characters contained in each font from a UTF-8 string to use as the index.
 * @param index - reference a string of characters
 */
void Font::setIndex(const std::wstring& index)
{
	_index = index;
}

/**
 * Loads the font from a YAML file.
 * @param node - refrence a YAML node
 */
void Font::load(const YAML::Node& node)
{
	_width		= node["width"]		.as<int>(_width);
	_height		= node["height"]	.as<int>(_height);
	_spacing	= node["spacing"]	.as<int>(_spacing);
	_monospace	= node["monospace"]	.as<bool>(_monospace);

	const std::string image = "Language/" + node["image"].as<std::string>();

	Surface* const fontTemp = new Surface(
										_width,
										_height);
	fontTemp->loadImage(CrossPlatform::getDataFile(image));

	_surface = new Surface(
						fontTemp->getWidth(),
						fontTemp->getHeight());
	_surface->setPalette(
					_palette,
					0,6);
	fontTemp->blit(_surface);

	delete fontTemp;

	init();
}

/**
 * Generates a pre-defined Codepage 437 (MS-DOS terminal) font.
 */
void Font::loadTerminal()
{
	_width = 9;
	_height = 16;
	_spacing = 0;
	_monospace = true;

	SDL_RWops* const rw = SDL_RWFromConstMem(
										dosFont,
										DOSFONT_SIZE);
	SDL_Surface* const s = SDL_LoadBMP_RW(rw, 0);
	SDL_FreeRW(rw);

	_surface = new Surface(s->w, s->h);
	SDL_Color terminal[2] =
	{
		{  0,   0,   0,   0},
		{192, 192, 192, 255}
	};

	_surface->setPalette(terminal, 0, 2);

	SDL_BlitSurface(
				s,
				0,
				_surface->getSurface(),
				0);
	SDL_FreeSurface(s);

	const std::wstring temp = _index;
	_index = L" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";

	init();

	_index = temp;
}

/**
 * Calculates the real size and position of each character in the surface
 * and stores them in SDL_Rect's for future use by other classes.
 */
void Font::init()
{
	_surface->lock();

	const int len = (_surface->getWidth() / _width);

	if (_monospace == true)
	{
		for (size_t
				i = 0;
				i < _index.length();
				++i)
		{
			SDL_Rect rect;

			const int
				startX = i %len * _width,
				startY = i / len * _height;

			rect.x = startX;
			rect.y = startY;
			rect.w = _width;
			rect.h = _height;

			_chars[_index[i]] = rect;
		}
	}
	else
	{
		for (size_t
				i = 0;
				i < _index.length();
				++i)
		{
			SDL_Rect rect;

			const int
				startX = i %len * _width,
				startY = i / len * _height;
			int
				left = -1,
				right = -1;

			for (int
					x = startX;
					x < startX + _width;
					++x)
			{
				for (int
						y = startY;
						y < startY + _height
							&& left == -1;
						++y)
				{
					if (_surface->getPixelColor(x, y) != 0)
						left = x;
				}
			}

			for (int
					x = startX + _width - 1;
					x >= startX;
					--x)
			{
				for (int
						y = startY + _height;
						y-- != startY
							&& right == -1;
						)
				{
					if (_surface->getPixelColor(x, y) != 0)
						right = x;
				}
			}

			rect.x = left;
			rect.y = startY;
			rect.w = right - left + 1;
			rect.h = _height;

			_chars[_index[i]] = rect;
		}
	}

	_surface->unlock();
}

/**
 * Returns a particular character from the set stored in the font.
 * @param fontChar - character to use for size/position
 * @return, pointer to the font's surface with the respective cropping rectangle set up
 */
Surface* Font::getChar(wchar_t fontChar)
{
	if (_chars.find(fontChar) == _chars.end())
		return NULL;

	_surface->getCrop()->x = _chars[fontChar].x;
	_surface->getCrop()->y = _chars[fontChar].y;
	_surface->getCrop()->w = _chars[fontChar].w;
	_surface->getCrop()->h = _chars[fontChar].h;

	return _surface;
}
/**
 * Returns the maximum width for any character in the font.
 * @return, width in pixels
 */
int Font::getWidth() const
{
	return _width;
}

/**
 * Returns the maximum height for any character in the font.
 * @return, height in pixels
 */
int Font::getHeight() const
{
	return _height;
}

/**
 * Returns the spacing for any character in the font.
 * @return, spacing in pixels
 * @note This does not refer to character spacing within the surface
 * but to the spacing used between multiple characters in a line.
 */
int Font::getSpacing() const
{
	return _spacing;
}

/**
 * Returns the dimensions of a particular character in the font.
 * @param fontChar - font character
 * @return, width and height dimensions (X and Y are ignored)
 */
SDL_Rect Font::getCharSize(wchar_t fontChar)
{
	SDL_Rect charSize = {0,0,0,0};

	if (fontChar != 1
		&& isLinebreak(fontChar) == false
		&& isSpace(fontChar) == false)
	{
		charSize.w = _chars[fontChar].w + _spacing;
		charSize.h = _chars[fontChar].h + _spacing;
	}
	else
	{
		if (_monospace == true)
			charSize.w = _width + _spacing;
		else if (isNonBreakableSpace(fontChar) == true)
			charSize.w = _width / 4;
		else
			charSize.w = _width / 2;

		charSize.h = _height + _spacing;
	}

	charSize.x = charSize.w; // in case anyone mixes them up
	charSize.y = charSize.h;

	return charSize;
}

/**
 * Returns the surface stored within the font.
 * Used for loading the actual graphic into the font.
 * @return, pointer to the internal surface
 */
Surface* Font::getSurface() const
{
	return _surface;
}

/**
 *
 * @param file	-
 * @param width	-
 */
void Font::fix(
		const std::string& file,
		int width)
{
	Surface* const srf = new Surface(width, 512);

	srf->setPalette(
				_palette,
				0,6);
	_surface->setPalette(
				_palette,
				0,6);

	int
		x = 0,
		y = 0;

	for (size_t
			i = 0;
			i < _index.length();
			++i)
	{
		SDL_Rect rect = _chars[_index[i]];
		_surface->getCrop()->x = rect.x;
		_surface->getCrop()->y = rect.y;
		_surface->getCrop()->w = rect.w;
		_surface->getCrop()->h = rect.h;
		_surface->setX(x);
		_surface->setY(y);
		_surface->blit(srf);

		x += _width;
		if (x == width)
		{
			x = 0;
			y += _height;
		}
	}

	SDL_SaveBMP(
			srf->getSurface(),
			file.c_str());
}

}
