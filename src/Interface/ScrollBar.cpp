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

#include "ScrollBar.h"

#include <algorithm>

#include "../fmath.h"

#include "../Engine/Action.h"
#include "../Engine/Palette.h"

#include "../Interface/TextList.h"


namespace OpenXcom
{

/**
 * Sets up a scrollbar with the specified size and position.
 * @param width		- width in pixels
 * @param height	- height in pixels
 * @param x			- X position in pixels
 * @param y			- Y position in pixels
 */
ScrollBar::ScrollBar(
		int width,
		int height,
		int x,
		int y)
	:
		InteractiveSurface(
			width,
			height,
			x,
			y),
		_list(0),
		_color(0),
		_pressed(false),
		_contrast(false),
		_offset(0),
		_bg(0)
{
//kL	_track = new Surface(width - 2, height + 1, x, y);
	_track = new Surface(width - 2, height, x, y); // kL
	_thumb = new Surface(width, height, x, y);

	_thumbRect.x = 0;
	_thumbRect.y = 0;
	_thumbRect.w = 0;
	_thumbRect.h = 0;
}

/**
 * Deletes contents.
 */
ScrollBar::~ScrollBar()
{
	delete _track;
	delete _thumb;
}

/**
 * Changes the position of the surface in the X axis.
 * @param x - X position in pixels
 */
void ScrollBar::setX(int x)
{
	Surface::setX(x);

	_track->setX(x + 1);
	_thumb->setX(x);
}

/**
 * Changes the position of the surface in the Y axis.
 * @param y - Y position in pixels
 */
void ScrollBar::setY(int y)
{
	Surface::setY(y);

	_track->setY(y);
	_thumb->setY(y);
}

/**
 * Changes the height of the scrollbar.
 * @param height - new height in pixels
 */
void ScrollBar::setHeight(int height)
{
	Surface::setHeight(height);

	_track->setHeight(height);
	_thumb->setHeight(height);

	_redraw = true;
}

/**
 * Changes the color used to render the scrollbar.
 * @param color - color value
 */
void ScrollBar::setColor(Uint8 color)
{
	_color = color;
}

/**
 * Returns the color used to render the scrollbar.
 * @return, color value
 */
Uint8 ScrollBar::getColor() const
{
	return _color;
}

/**
 * Enables/disables high contrast color. Mostly used for Battlescape text.
 * @param contrast - high contrast setting
 */
void ScrollBar::setHighContrast(bool contrast)
{
	_contrast = contrast;
}

/**
 * Changes the list associated with the scrollbar. This makes the button scroll that list.
 * @param list - pointer to TextList
 */
void ScrollBar::setTextList(TextList* list)
{
	_list = list;
}

/**
 * Changes the surface used to draw the background of the track.
 * @param bg - pointer to a Surface
 */
void ScrollBar::setBackground(Surface* bg)
{
	_bg = bg;
}

/**
 * Replaces a certain amount of colors in the scrollbar's palette.
 * @param colors		- pointer to the set of colors
 * @param firstcolor	- offset of the first color to replace
 * @param ncolors		- amount of colors to replace
 */
void ScrollBar::setPalette(
		SDL_Color* colors,
		int firstcolor,
		int ncolors)
{
	Surface::setPalette(colors, firstcolor, ncolors);

	_track->setPalette(colors, firstcolor, ncolors);
	_thumb->setPalette(colors, firstcolor, ncolors);
}

/**
 * Automatically updates the scrollbar when the mouse moves.
 * @param action - pointer to an action
 * @param state - state that the action handlers belong to
 */
void ScrollBar::handle(Action* action, State* state)
{
	InteractiveSurface::handle(action, state); // kL_note: screw it. Okay, try it again ... nah Screw it.

	if (_pressed
		&& (action->getDetails()->type == SDL_MOUSEMOTION
			|| action->getDetails()->type == SDL_MOUSEBUTTONDOWN))
	{
		int cursorY = static_cast<int>(action->getAbsoluteYMouse()) - getY();
		int y = std::min(
					std::max(
							cursorY + _offset,
							0),
					getHeight() - static_cast<int>(_thumbRect.h) + 1);

		double scale = static_cast<double>(_list->getRows()) / static_cast<double>(getHeight());
		size_t scroll = static_cast<size_t>(Round(static_cast<double>(y) * scale));

		_list->scrollTo(scroll);
	}
}

/**
 * Blits the scrollbar contents.
 * @param surface - pointer to a Surface to blit onto
 */
void ScrollBar::blit(Surface* surface)
{
	Surface::blit(surface);

	if (_visible
		&& !_hidden)
	{
		_track->blit(surface);
		_thumb->blit(surface);

		invalidate();
	}
}

/**
 * The scrollbar only moves while the button is pressed.
 * @param action - pointer to an action
 * @param state - state that the action handlers belong to
 */
void ScrollBar::mousePress(Action* action, State* state)
{
	InteractiveSurface::mousePress(action, state);

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		int cursorY = static_cast<int>(action->getAbsoluteYMouse()) - getY();
		if (cursorY >= static_cast<int>(_thumbRect.y)
			&& cursorY < static_cast<int>(_thumbRect.y) + static_cast<int>(_thumbRect.h))
		{
			_offset = static_cast<int>(_thumbRect.y) - cursorY;
		}
		else
			_offset = -(static_cast<int>(_thumbRect.h) / 2);

		_pressed = true;
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
		_list->scrollUp(false, true);
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
		_list->scrollDown(false, true);
}

/**
 * The scrollbar stops moving when the button is released.
 * @param action - pointer to an action
 * @param state - state that the action handlers belong to
 */
void ScrollBar::mouseRelease(Action* action, State* state)
{
	InteractiveSurface::mouseRelease(action, state);

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_pressed = false;
		_offset = 0;
	}
}

/**
 * Updates the thumb according to the current list position.
 */
void ScrollBar::draw()
{
	Surface::draw();

	drawTrack();
	drawThumb();
}

/**
 * Draws the track (background bar) semi-transparent.
 */
void ScrollBar::drawTrack()
{
	if (_bg)
	{
		_track->copy(_bg);

		if (_contrast)
			_track->offset(-5, 1);
		else if (_list->getComboBox())
			_track->offset(1, Palette::backPos);
		else
			_track->offset(-5, Palette::backPos);
	}
}

/**
 * Draws the thumb (button) as a hollow square.
 */
void ScrollBar::drawThumb()
{
	double scale = static_cast<double>(getHeight()) / static_cast<double>(_list->getRows());

	_thumbRect.x = 0;
	_thumbRect.y = static_cast<Sint16>(floor(static_cast<double>(_list->getScroll()) * scale));
	_thumbRect.w = static_cast<Uint16>(_thumb->getWidth());
	_thumbRect.h = static_cast<Uint16>(ceil(static_cast<double>(_list->getVisibleRows()) * scale));

	_thumb->clear();
	_thumb->lock();

	SDL_Rect square = _thumbRect; // Draw filled button
	Uint8 color = _color + 2;

	square.w--;
	square.h--;

	_thumb->drawRect(&square, color);

	square.x++;
	square.y++;
	color = _color + 5;

	_thumb->drawRect(&square, color);

	square.w--;
	square.h--;
	color = _color + 4;

	_thumb->drawRect(&square, color);

	_thumb->setPixelColor(
					_thumbRect.x,
					_thumbRect.y,
					_color + 1);
	_thumb->setPixelColor(
					_thumbRect.x,
					_thumbRect.y + _thumbRect.h - 1,
					_color + 4);
	_thumb->setPixelColor(
					_thumbRect.x + _thumbRect.w - 1,
					_thumbRect.y,
					_color + 4);

	if (static_cast<int>(square.h) - 4 > 0)
	{
		color = _color + 5; // Hollow it out

		square.x++;
		square.y++;
		square.w -= 3;
		square.h -= 3;

		_thumb->drawRect(&square, color);

		square.x++;
		square.y++;
		color = _color + 2;

		_thumb->drawRect(&square, color);

		square.w--;
		square.h--;
		color = 0;

		_thumb->drawRect(&square, color);

		_thumb->setPixelColor(
						_thumbRect.x + 2 + _thumbRect.w - 1 - 4,
						_thumbRect.y + 2 + _thumbRect.h - 1 - 4,
						_color + 1);
		_thumb->setPixelColor(
						_thumbRect.x + 2,
						_thumbRect.y + 2 + _thumbRect.h - 1 - 4,
						_color + 4);
		_thumb->setPixelColor(
						_thumbRect.x + 2 + _thumbRect.w - 1 - 4,
						_thumbRect.y + 2,
						_color + 4);
	}

	_thumb->unlock();
}

}
