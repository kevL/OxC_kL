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

#include "ToggleTextButton.h"


namespace OpenXcom
{

/**
 * Constructs a ToggleTextButton.
 * @param width		- the width
 * @param height	- the height
 * @param x			- the X position
 * @param y			- the Y position
 */
ToggleTextButton::ToggleTextButton(
		int width,
		int height,
		int x,
		int y)
	:
		TextButton(
			width,
			height,
			x,y),
		_originalColor(std::numeric_limits<uint8_t>::max()),
		_invertedColor(std::numeric_limits<uint8_t>::max()),
		_fakeGroup(NULL)
{
	_isPressed = false;
	TextButton::setGroup(&_fakeGroup);
}

/**
 * dTor.
 */
ToggleTextButton::~ToggleTextButton(void)
{}

/**
 * Handles mouse clicks by toggling the button state.
 * @note Use '_fakeGroup' to trick TextButton into drawing the right thing.
 * @param action	- pointer to an Action
 * @param state		- pointer to a State
 */
void ToggleTextButton::mousePress(Action* action, State* state)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT
		|| action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		_isPressed = !_isPressed;
		_fakeGroup = _isPressed ? this : NULL; // this is the trick that makes TextButton stick

		if (_isPressed == true
			&& _invertedColor != std::numeric_limits<uint8_t>::max())
		{
			TextButton::setColor(_invertedColor);
		}
		else
			TextButton::setColor(_originalColor);
	}

	InteractiveSurface::mousePress(action, state); // skip TextButton's code as it will try to set *_group
	draw();
}

/**
 * Sets the '_isPressed' state of the button and forces it to redraw.
 * @param pressed - true if pressed
 */
void ToggleTextButton::setPressed(bool pressed)
{
	if (_isPressed == pressed)
		return;

	_isPressed = pressed;
	_fakeGroup = _isPressed ? this : NULL;

	if (_isPressed == true
		&& _invertedColor != std::numeric_limits<uint8_t>::max())
	{
		TextButton::setColor(_invertedColor);
	}
	else
		TextButton::setColor(_originalColor);

	_redraw = true;
}

/**
 * Sets the color of this button.
 * @param color - color
 */
void ToggleTextButton::setColor(Uint8 color)
{
	_originalColor = color;
	TextButton::setColor(color);
}

/**
 * Surface::invert() is called when this value is set and the button is depressed.
 * @param color - the invert color
 */
void ToggleTextButton::setInvertColor(Uint8 color)
{
	_invertedColor = color;
	_fakeGroup = NULL;

	_redraw = true;
}

/**
 * Handles draw() in case the button needs to be painted with a garish color.
 */
void ToggleTextButton::draw()
{
	if (_invertedColor != std::numeric_limits<uint8_t>::max())
		_fakeGroup = NULL; // nevermind, TextButton. We'll invert the surface ourselves.

	TextButton::draw();

	if (_isPressed == true
		&& _invertedColor != std::numeric_limits<uint8_t>::max())
	{
		this->invert(static_cast<Uint8>(_invertedColor + 4));
	}
}

}
