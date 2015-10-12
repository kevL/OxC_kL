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

#include "ArrowButton.h"

#include "TextList.h"

#include "../Engine/Action.h"
#include "../Engine/Timer.h"


namespace OpenXcom
{

/**
 * Sets up an arrow button with the specified size and position.
 * @param shape		- shape of the arrow
 * @param width		- width in pixels
 * @param height	- height in pixels
 * @param x			- X position in pixels (default 0)
 * @param y			- Y position in pixels (default 0)
 */
ArrowButton::ArrowButton(
		ArrowShape shape,
		int width,
		int height,
		int x,
		int y)
	:
		ImageButton(
			width,
			height,
			x,y),
		_shape(shape),
		_list(NULL)
{
	_timer = new Timer(77);
	_timer->onTimer((SurfaceHandler)& ArrowButton::scroll);
}

/**
 * Deletes timers.
 */
ArrowButton::~ArrowButton()
{
	delete _timer;
}

/**
 *
 */
bool ArrowButton::isButtonHandled(Uint8 btn)
{
	if (_list != NULL)
		return btn == SDL_BUTTON_LEFT
			|| btn == SDL_BUTTON_RIGHT;
	else
		return ImageButton::isButtonHandled(btn);
}

/**
 * Changes the color for the image button.
 * @param color Color value.
 */
void ArrowButton::setColor(Uint8 color)
{
	ImageButton::setColor(color);
	_redraw = true;
}

/**
 * Changes the shape for the arrow button.
 * @param shape Shape of the arrow.
 */
void ArrowButton::setShape(ArrowShape shape)
{
	_shape = shape;
	_redraw = true;
}

/**
 * Changes the list associated with the arrow button.
 * This makes the button scroll that list.
 * @param textList - pointer to TextList
 */
void ArrowButton::setTextList(TextList* textList)
{
	_list = textList;
}

/**
 * Draws the button with the specified arrow shape.
 */
void ArrowButton::draw()
{
	ImageButton::draw();
	lock();

	SDL_Rect square; // Draw button
	Uint8 color = _color + 2;

	square.x =
	square.y = 0;
	square.w = static_cast<Uint16>(getWidth() - 1);
	square.h = static_cast<Uint16>(getHeight() - 1);

	drawRect(&square, color);

	++square.x;
	++square.y;
	color = _color + 5;

	drawRect(&square, color);

	--square.w;
	--square.h;
	color = _color + 4;

	drawRect(&square, color);

	setPixelColor(
			0,0,
			_color + 1);
	setPixelColor(
			0,
			getHeight() - 1,
			_color + 4);
	setPixelColor(
			getWidth() - 1,
			0,
			_color + 4);

	color = _color + 1;

	switch (_shape)
	{
		case OpenXcom::ARROW_BIG_UP:
		{
			square.x = 5; // Draw arrow square
			square.y = 8;
			square.w = 3;
			square.h = 3;

			drawRect(&square, color);

			square.x = 2; // Draw arrow triangle
			square.y = 7;
			square.w = 9;
			square.h = 1;

			for (; square.w > 1; square.w -= 2)
			{
				drawRect(&square, color);
				++square.x;
				--square.y;
			}

			drawRect(&square, color);
		}
		break;

		case OpenXcom::ARROW_BIG_DOWN:
		{
			square.x = 5; // Draw arrow square
			square.y = 3;
			square.w = 3;
			square.h = 3;

			drawRect(&square, color);

			square.x = 2; // Draw arrow triangle
			square.y = 6;
			square.w = 9;
			square.h = 1;

			for (; square.w > 1; square.w -= 2)
			{
				drawRect(&square, color);
				++square.x;
				++square.y;
			}

			drawRect(&square, color);
		}
		break;

		case OpenXcom::ARROW_SMALL_UP:
		{
			square.x = 1; // Draw arrow triangle 1
			square.y = 5;
			square.w = 9;
			square.h = 1;

			for (; square.w > 1; square.w -= 2)
			{
				drawRect(&square, color + 2);
				++square.x;
				--square.y;
			}

			drawRect(&square, color + 2);

			square.x = 2; // Draw arrow triangle 2
			square.y = 5;
			square.w = 7;
			square.h = 1;

			for (; square.w > 1; square.w -= 2)
			{
				drawRect(&square, color);
				++square.x;
				--square.y;
			}

			drawRect(&square, color);
		}
		break;

		case OpenXcom::ARROW_SMALL_DOWN:
		{
			square.x = 1; // Draw arrow triangle 1
			square.y = 2;
			square.w = 9;
			square.h = 1;

			for (; square.w > 1; square.w -= 2)
			{
				drawRect(&square, color + 2);
				++square.x;
				++square.y;
			}

			drawRect(&square, color + 2);

			square.x = 2; // Draw arrow triangle 2
			square.y = 2;
			square.w = 7;
			square.h = 1;

			for (; square.w > 1; square.w -= 2)
			{
				drawRect(&square, color);
				++square.x;
				++square.y;
			}

			drawRect(&square, color);
		}
		break;

		case OpenXcom::ARROW_SMALL_LEFT:
		{
			square.x = 2; // Draw arrow triangle 1
			square.y = 4;
			square.w = 2;
			square.h = 1;

			for (; square.h < 5; square.h += 2)
			{
				drawRect(&square, color + 2);
				square.x += 2;
				--square.y;
			}

			square.w = 1;
			drawRect(&square, color + 2);

			square.x = 3; // Draw arrow triangle 2
			square.y = 4;
			square.w = 2;
			square.h = 1;

			for (; square.h < 5; square.h += 2)
			{
				drawRect(&square, color);
				square.x += 2;
				--square.y;
			}

			square.w = 1;
			drawRect(&square, color);
		}
		break;

		case OpenXcom::ARROW_SMALL_RIGHT:
		{
			square.x = 7; // Draw arrow triangle 1
			square.y = 4;
			square.w = 2;
			square.h = 1;

			for (; square.h < 5; square.h += 2)
			{
				drawRect(&square, color + 2);
				square.x -= 2;
				--square.y;
			}

			++square.x;
			square.w = 1;
			drawRect(&square, color + 2);

			square.x = 6; // Draw arrow triangle 2
			square.y = 4;
			square.w = 2;
			square.h = 1;

			for (; square.h < 5; square.h += 2)
			{
				drawRect(&square, color);
				square.x -= 2;
				--square.y;
			}

			++square.x;
			square.w = 1;
			drawRect(&square, color);
		}
	}
	unlock();
}

/**
 * Keeps the scrolling timers running.
 */
void ArrowButton::think()
{
	_timer->think(NULL, this);
}

/**
 * Scrolls the list.
 */
void ArrowButton::scroll()
{
	if (_shape == ARROW_BIG_UP)
		_list->scrollUp();
	else if (_shape == ARROW_BIG_DOWN)
		_list->scrollDown();
}

/**
 * Starts scrolling the associated list.
 * @param action - pointer to an Action
 * @param state - state that the action handlers belong to
 */
void ArrowButton::mousePress(Action* action, State* state)
{
	ImageButton::mousePress(action, state);

	if (_list != NULL)
	{
		if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
			_timer->start();
		else if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
			_list->scrollUp(false, true);
		else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
			_list->scrollDown(false, true);
	}
}

/**
 * Stops scrolling the associated list.
 * @param action - pointer to an Action
 * @param state - state that the action handlers belong to
 */
void ArrowButton::mouseRelease(Action* action, State* state)
{
	ImageButton::mouseRelease(action, state);

	if (_list != NULL
		&& action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_timer->stop();
	}
}

/**
 * Scrolls the associated list to top or bottom.
 * @param action - pointer to an Action
 * @param state - state that the action handlers belong to
 */
void ArrowButton::mouseClick(Action* action, State* state)
{
	ImageButton::mouseClick(action, state);

	if (_list != NULL
		&& action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		if (_shape == ARROW_BIG_UP)
			_list->scrollUp(true);
		else if (_shape == ARROW_BIG_DOWN)
			_list->scrollDown(true);
	}
}

}
