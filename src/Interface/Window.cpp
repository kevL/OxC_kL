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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Window.h"

#include <SDL.h>

#include "../aresame.h"

#include "../Engine/RNG.h"
#include "../Engine/Sound.h"
#include "../Engine/Timer.h"


namespace OpenXcom
{

const double Window::POPUP_SPEED = 0.07;

Sound* Window::soundPopup[3] = {0, 0, 0};


/**
 * Sets up a blank window with the specified size and position.
 * @param state Pointer to state the window belongs to.
 * @param width Width in pixels.
 * @param height Height in pixels.
 * @param x X position in pixels.
 * @param y Y position in pixels.
 * @param popup Popup animation.
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
		_bg(0),
		_color(0),
		_popup(popup),
		_popupStep(0.0),
		_state(state),
		_contrast(false),
		_screen(false)
{
	_timer = new Timer(10);
	_timer->onTimer((SurfaceHandler)& Window::popup);

	if (_popup == POPUP_NONE)
		_popupStep = 1.0;
	else
	{
		setHidden(true);
		_timer->start();

		if (_state != 0)
		{
			_screen = state->isScreen();
			if (_screen)
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
 * Changes the surface used to draw the background of the window.
 * @param bg New background.
 */
void Window::setBackground(Surface* bg)
{
	_bg = bg;
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
 * Enables/disables high contrast color. Mostly used for
 * Battlescape UI.
 * @param contrast High contrast setting.
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
	if (_hidden && _popupStep < 1.0)
	{
		_state->hideAll();
		setHidden(false);
	}

	_timer->think(0, this);
}

/**
 * Plays the window popup animation.
 */
void Window::popup()
{
	if (AreSame(_popupStep, 0.0))
	{
//kL		int sound = RNG::generate(0, 2);
		int sound = RNG::generate(1, 2); // kL
//kL		if (soundPopup[sound] != 0)
//		{
		soundPopup[sound]->play();
//		}
	}

	if (_popupStep < 1.0)
	{
		_popupStep += POPUP_SPEED;
	}
	else
	{
		if (_screen)
		{
			_state->toggleScreen();
		}

		_state->showAll();
		_popupStep = 1.0;

		_timer->stop();
	}

	_redraw = true;
}

/**
 * Draws the bordered window with a graphic background.
 * The background never moves with the window, it's
 * always aligned to the top-left corner of the screen
 * and cropped to fit the inside area.
 */
void Window::draw()
{
	Surface::draw();


	SDL_Rect square;

	if (_popup == POPUP_HORIZONTAL || _popup == POPUP_BOTH)
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

	if (_popup == POPUP_VERTICAL || _popup == POPUP_BOTH)
	{
		square.y = static_cast<Sint16>(
				(static_cast<double>(getHeight()) - (static_cast<double>(getHeight()) * _popupStep)) / 2.0);
		square.h = static_cast<Uint16>(
				static_cast<double>(getHeight()) * _popupStep);
	}
	else
	{
		square.y = 0;
		square.h = static_cast<Uint16>(getHeight());
	}

	Uint8 mult = 1;
	if (_contrast)
	{
		mult = 2;
	}
	Uint8 color = _color + 3 * mult;

	for (int
			i = 0;
			i < 5;
			++i)
	{
		drawRect(&square, color);

		if (i < 2)
			color -= 1 * mult;
		else
			color += 1 * mult;

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

	if (_bg != 0)
	{
		_bg->getCrop()->x = static_cast<Sint16>(getX() + static_cast<int>(square.x) - _dx);
		_bg->getCrop()->y = static_cast<Sint16>(getY() + static_cast<int>(square.y) - _dy);
		_bg->getCrop()->w = square.w;
		_bg->getCrop()->h = square.h;

		_bg->setX(static_cast<int>(square.x));
		_bg->setY(static_cast<int>(square.y));

		_bg->blit(this);
	}
}

}
