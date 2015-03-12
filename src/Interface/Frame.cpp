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

#include "Frame.h"

#include "../Engine/Palette.h"


namespace OpenXcom
{

/**
 * Sets up a blank Frame with the specified size and position.
 * @param width		- width in pixels
 * @param height	- height in pixels
 * @param x			- X position in pixels
 * @param y			- Y position in pixels
 */
Frame::Frame(
		int width,
		int height,
		int x,
		int y)
	:
		Surface(
			width,
			height,
			x,
			y),
		_color(0),
		_bg(0),
		_thickness(5),
		_contrast(false)
{
}

/**
 * dTor.
 */
Frame::~Frame()
{
}

/**
 * Changes the color used to draw the shaded border.
 * @param color - color value
 */
void Frame::setColor(Uint8 color)
{
	_color = color;
	_redraw = true;
}

/**
 * Changes the color used to draw the shaded border.
 * Only really to be used in conjunction with the State add()
 * function as a convenient wrapper to avoid ugly quacks at that end;
 * better to have them here!
 * @param color - color value
 */
void Frame::setBorderColor(Uint8 color)
{
	setColor(color);
}

/**
 * Returns the color used to draw the shaded border.
 * @return, color value
 */
Uint8 Frame::getColor() const
{
	return _color;
}

/**
* Changes the color used to draw the background.
* @param bg - color value
*/
void Frame::setSecondaryColor(Uint8 bg)
{
	_bg = bg;
	_redraw = true;
}

/**
* Returns the color used to draw the background.
* @return, color value
*/
Uint8 Frame::getSecondaryColor() const
{
	return _bg;
}

/**
 * Enables/disables high contrast color. Mostly used for Battlescape UI.
 * @param contrast - high contrast setting
 */
void Frame::setHighContrast(bool contrast)
{
	_contrast = contrast;
	_redraw = true;
}

/**
* Changes the thickness of the border to draw.
* @param thickness - thickness in pixels
*/
void Frame::setThickness(int thickness)
{
	_thickness = thickness;
	_redraw = true;
}

/**
 * Draws the bordered frame with a graphic background.
 * The background never moves with the frame, it's  always aligned to the
 * top-left corner of the screen and cropped to fit the inside area.
 */
void Frame::draw()
{
	Surface::draw();

	SDL_Rect square;
	square.x = 0;
	square.y = 0;
	square.w = static_cast<Uint16>(getWidth());
	square.h = static_cast<Uint16>(getHeight());

	int mult = 1;
	if (_contrast)
		mult = 2;

	Uint8
		darkest = Palette::blockOffset(_color / 16) + 15,
		color = _color;

	for (int
			i = 0;
			i < _thickness;
			++i)
	{
		if ((_thickness > 1
				&& i == _thickness - 1)
			|| color / 16 != _color / 16)
		{
			color = darkest;
		}
		else
			color = _color + static_cast<Uint8>(std::abs(i - _thickness / 2) * mult);

		drawRect(
				&square,
				color);

		++square.x;
		++square.y;

		if (square.w > 1)
			square.w -= 2;
		else
			square.w = 1;

		if (square.h > 1)
			square.h -= 2;
		else
			square.h = 1;
	}

	drawRect(
			&square,
			_bg);
}

}
