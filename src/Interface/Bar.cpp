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

#include "Bar.h"

//#include <SDL.h>


namespace OpenXcom
{

/**
 * Sets up a blank bar with the specified size and position.
 * @param width		- width in pixels
 * @param height	- height in pixels
 * @param x			- X position in pixels (default 0)
 * @param y			- Y position in pixels (default 0)
 */
Bar::Bar(
		int width,
		int height,
		int x,
		int y)
	:
		Surface(
			width,
			height,
			x,y),
		_color(0),
		_color2(0),
		_borderColor(0),
		_scale(0.),
		_max(0.),
		_value(0.),
		_value2(0.),
		_invert(false),
		_secondOnTop(true),
		_offSecond_y(0)
{}

/**
 * dTor
 */
Bar::~Bar()
{}

/**
 * Changes the color used to draw the border and contents.
 * @param color - color value
 */
void Bar::setColor(Uint8 color)
{
	_color = color;
	_redraw = true;
}

/**
 * Returns the color used to draw the bar.
 * @return, color value
 */
Uint8 Bar::getColor() const
{
	return _color;
}

/**
 * Changes the color used to draw the second contents.
 * @param color - color value
 */
void Bar::setSecondaryColor(Uint8 color)
{
	_color2 = color;
	_redraw = true;
}

/**
 * Returns the second color used to draw the bar.
 * @return, color value
 */
Uint8 Bar::getSecondaryColor() const
{
	return _color2;
}

/**
 * Changes the scale factor used to draw the bar values.
 * @param scale - scale in pixels/unit (default 1.0)
 */
void Bar::setScale(double scale)
{
	_scale = scale;
	_redraw = true;
}

/**
 * Returns the scale factor used to draw the bar values.
 * @return, scale in pixels/unit
 */
double Bar::getScale() const
{
	return _scale;
}

/**
 * Changes the maximum value used to draw the outer border.
 * @param maxVal - maximum value (default 100.)
 */
void Bar::setMax(double maxVal)
{
	_max = maxVal;
	_redraw = true;
}

/**
 * Returns the maximum value used to draw the outer border.
 * @return, maximum value
 */
double Bar::getMax() const
{
	return _max;
}

/**
 * Changes the value used to draw the inner contents.
 * @param value - current value
 */
void Bar::setValue(double value)
{
	_value = (value < 0.) ? 0. : value;
	_redraw = true;
}

/**
 * Returns the value used to draw the inner contents.
 * @return, current value
 */
double Bar::getValue() const
{
	return _value;
}

/**
 * Changes the value used to draw the second inner contents.
 * @param value - current value
 */
void Bar::setValue2(double value)
{
	_value2 = (value < 0.) ? 0. : value;
	_redraw = true;
}

/**
 * Returns the value used to draw the second inner contents.
 * @return, current value
 */
double Bar::getValue2() const
{
	return _value2;
}

/**
 * Defines whether the second value should be drawn on top.
 * @param onTop - true if second value on top (default true)
 */
void Bar::setSecondValueOnTop(bool onTop)
{
	_secondOnTop = onTop;
}

/**
 * Offsets y-value of second bar.
 * @note Only works if second is on top.
 * @param y - amount of y to offset by
 */
void Bar::offsetSecond(int y)
{
	_offSecond_y = y;
}

/**
 * Enables/disables color inverting.
 * Some bars have darker borders and others have lighter borders.
 * @param invert - invert setting (default true)
 */
void Bar::setInvert(bool invert)
{
	_invert = invert;
	_redraw = true;
}

/**
 * Draws the bordered bar filled according to its values.
 */
void Bar::draw()
{
	Surface::draw();

	SDL_Rect rect;
	rect.x =
	rect.y = 0;
	rect.w = static_cast<Uint16>(Round(_scale * _max)) + 1;
	rect.h = static_cast<Uint16>(getHeight());

	if (_invert == true)
		drawRect(&rect, _color);
	else
	{
		if (_borderColor != 0)
			drawRect(&rect, _borderColor);
		else
			drawRect(&rect, _color + 6); // was +4 but red is wonky.
	}

	++rect.y;
	--rect.w;
	rect.h -= 2;

	drawRect(&rect, 0);

	double
		width = _scale * _value,
		width2 = _scale * _value2;

	if (width > 0. && width < 1.) // these ensure that miniscule amounts still show up.
		width = 1.;
	if (width2 > 0. && width2 < 1.)
		width2 = 1.;

	if (_invert == true)
	{
		if (_secondOnTop == true)
		{
			rect.w = static_cast<Uint16>(Round(width));
			drawRect(&rect, _color + 4);

			rect.w = static_cast<Uint16>(Round(width2));
			rect.y += static_cast<Sint16>(_offSecond_y);
			drawRect(&rect, _color2 + 4);
		}
		else
		{
			rect.w = static_cast<Uint16>(Round(width2));
			drawRect(&rect, _color2 + 4);

			rect.w = static_cast<Uint16>(Round(width));
			drawRect(&rect, _color + 4);
		}
	}
	else
	{
		if (_secondOnTop == true)
		{
			rect.w = static_cast<Uint16>(Round(width));
			drawRect(&rect, _color);

			rect.w = static_cast<Uint16>(Round(width2));
			rect.y += static_cast<Sint16>(_offSecond_y);
			drawRect(&rect, _color2);
		}
		else
		{
			rect.w = static_cast<Uint16>(Round(width2));
			drawRect(&rect, _color2);

			rect.w = static_cast<Uint16>(Round(width));
			drawRect(&rect, _color);
		}
	}
}

/**
 * Sets the border color for the bar.
 * @note Will use base color+4 if none is defined here.
 * @param color - the color for the outline of the bar
 */
void Bar::setBorderColor(Uint8 color)
{
	_borderColor = color;
}

}
