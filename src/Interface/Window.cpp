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

#include "Window.h"

//#include <SDL.h>
//#include <SDL_mixer.h>

//#include "../fmath.h"

#include "../Engine/Sound.h"
#include "../Engine/Timer.h"


namespace OpenXcom
{

//const double Window::POPUP_SPEED = 0.076; // higher is faster
const double Window::POPUP_SPEED = 0.135; // for high-quality filters & shaders, like 4xHQX

Sound* Window::soundPopup[3] = {0, 0, 0};


/**
 * Sets up a blank window with the specified size and position.
 * @param state		- pointer to State the window belongs to
 * @param width		- width in pixels
 * @param height	- height in pixels
 * @param x			- X position in pixels (default 0)
 * @param y			- Y position in pixels (default 0)
 * @param popup		- popup animation (default POPUP_NONE)
 */
Window::Window(
		State* state,
		int width,
		int height,
		int x,
		int y,
		WindowPopup popup)
	:
		Surface(
			width,
			height,
			x,
			y),
		_state(state),
		_popup(popup),
		_bg(NULL),
		_dx(-x),
		_dy(-y),
		_color(0),
		_popupStep(0.),
		_contrast(false),
		_screen(false),
		_thinBorder(false),
		_bgX(0),
		_bgY(0)
{
	_timer = new Timer(12);
	_timer->onTimer((SurfaceHandler)& Window::popup);

	if (_popup == POPUP_NONE)
		_popupStep = 1.;
	else
	{
		_hidden = true;
		_timer->start();

		if (_state != NULL)
		{
			_screen = _state->isScreen();
			if (_screen == true)
				_state->toggleScreen();
		}
	}
}

/**
 * Deletes timers.
 */
Window::~Window()
{
	delete _timer;
}

/**
 * Sets the surface used to draw the background of the window.
 * @param bg - background
 * @param dx - x offset (default 0)
 * @param dy - y offset (default 0)
 */
void Window::setBackground(
		Surface* bg,
		int dx,
		int dy)
{
	_bg = bg;

	_bgX = dx;
	_bgY = dy;

	_redraw = true;
}

/**
 * Changes the color used to draw the shaded border.
 * @param color Color value.
 */
void Window::setColor(Uint8 color)
{
	_color = color;
	_redraw = true;
}

/**
 * Returns the color used to draw the shaded border.
 * @return Color value.
 */
Uint8 Window::getColor() const
{
	return _color;
}

/**
 * Enables/disables high contrast color. Mostly used for Battlescape UI.
 * @param contrast - high contrast setting (default true)
 */
void Window::setHighContrast(bool contrast)
{
	_contrast = contrast;
	_redraw = true;
}

/**
 * Keeps the animation timers running.
 */
void Window::think()
{
	if (_hidden == true
		&& _popupStep < 1.)
	{
		_state->hideAll();
		_hidden = false;
	}

	_timer->think(NULL, this);
}

/**
 * Plays the window popup animation.
 */
void Window::popup()
{

	if (AreSame(_popupStep, 0.) == true)
		soundPopup[(SDL_GetTicks() %2) + 1]->play(Mix_GroupAvailable(0));

	if (_popupStep < 1.)
		_popupStep += POPUP_SPEED;
	else
	{
		if (_screen == true)
			_state->toggleScreen();

		_popupStep = 1.;

		_state->showAll();
		_timer->stop();
	}

	_redraw = true;
}

/**
 * Gets if this window has finished popping up.
 * @return, true if popup is finished
 */
bool Window::isPopupDone() const
{
	return (AreSame(_popupStep, 1.) == true);
}

/**
 * Draws the bordered window with a graphic background.
 * The background never moves with the window, it's always aligned to the
 * top-left corner of the screen and cropped to fit the inside area.
 */
void Window::draw()
{
	Surface::draw();


	SDL_Rect square;

	if (_popup == POPUP_HORIZONTAL
		|| _popup == POPUP_BOTH)
	{
		square.x = static_cast<Sint16>(
					(static_cast<double>(getWidth()) - (static_cast<double>(getWidth()) * _popupStep)) / 2);
		square.w = static_cast<Uint16>(
					static_cast<double>(getWidth()) * _popupStep);
	}
	else
	{
		square.x = 0;
		square.w = static_cast<Uint16>(getWidth());
	}

	if (_popup == POPUP_VERTICAL
		|| _popup == POPUP_BOTH)
	{
		square.y = static_cast<Sint16>(
					(static_cast<double>(getHeight()) - (static_cast<double>(getHeight()) * _popupStep)) / 2);
		square.h = static_cast<Uint16>(
					static_cast<double>(getHeight()) * _popupStep);
	}
	else
	{
		square.y = 0;
		square.h = static_cast<Uint16>(getHeight());
	}

	Uint8 gradient = 1;
	if (_contrast == true)
		gradient = 2;

	Uint8 color = _color + (gradient * 3);

	if (_thinBorder == true)
	{
		color = _color + gradient;
		for (int
				i = 0;
				i < 5;
				++i)
		{
			drawRect(
					&square,
					color);

			if (i %2 == 0)
			{
				square.x++;
				square.y++;
			}
			square.w--;
			square.h--;

			switch (i)
			{
				case 0:
					color = _color + (gradient * 5);
					setPixelColor(square.w, 0, color);
				break;
				case 1:
					color = _color + (gradient * 2);
				break;
				case 2:
					color = _color + (gradient * 4);
					setPixelColor(square.w + 1, 1, color);
				break;
				case 3:
					color = _color + (gradient * 3);
				break;
			}
		}
	}
	else
	{
		for (int
				i = 0;
				i < 5;
				++i)
		{
			drawRect(
					&square,
					color);

			if (i < 2)
				color -= gradient;
			else
				color += gradient;

			square.x++;
			square.y++;

			if (square.w >= 2)
				square.w -= 2;
			else
				square.w = 1;

			if (square.h >= 2)
				square.h -= 2;
			else
				square.h = 1;
		}
	}

	if (_bg != NULL)
	{
		_bg->getCrop()->x = static_cast<Sint16>(static_cast<int>(square.x) - _dx - _bgX);
		_bg->getCrop()->y = static_cast<Sint16>(static_cast<int>(square.y) - _dy - _bgY);
		_bg->getCrop()->w = square.w;
		_bg->getCrop()->h = square.h;

		_bg->setX(static_cast<int>(square.x));
		_bg->setY(static_cast<int>(square.y));

		_bg->blit(this);
	}
}

/**
 * Changes the horizontal offset of the surface in the X axis.
 * @param dx - X position in pixels
 */
void Window::setDX(int dx)
{
	_dx = dx;
}

/**
 * Changes the vertical offset of the surface in the Y axis.
 * @param dy - Y position in pixels
 */
void Window::setDY(int dy)
{
	_dy = dy;
}

/**
 * Changes the window to have a thin border.
 */
void Window::setThinBorder()
{
	_thinBorder = true;
}

}
