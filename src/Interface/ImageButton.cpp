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

#include "ImageButton.h"

#include "../Engine/Action.h"


namespace OpenXcom
{

/**
 * Sets up an image button with the specified size and position.
 * @param width		- width in pixels
 * @param height	- height in pixels
 * @param x			- X position in pixels (default 0)
 * @param y			- Y position in pixels (default 0)
 */
ImageButton::ImageButton(
		int width,
		int height,
		int x,
		int y)
	:
		InteractiveSurface(
			width,
			height,
			x,y),
		_color(0),
		_group(0),
		_inverted(false)
{}

/**
 * dTor.
 */
ImageButton::~ImageButton()
{}

/**
 * Changes the color for the image button.
 * @param color - color value
 */
void ImageButton::setColor(Uint8 color)
{
	_color = color;
}

/**
 * Returns the color for the image button.
 * @return, color value
 */
Uint8 ImageButton::getColor() const
{
	return _color;
}

/**
 * Changes the button group this image button belongs to.
 * @param group - pointer to the pressed button pointer in the group;
 *				  NULL makes it a regular button
 */
void ImageButton::setGroup(ImageButton** group)
{
	_group = group;

	if (_group != NULL && *_group == this)
		invert(_color + 3);
}

/**
 * Sets the button as the pressed button if it's part of a group and inverts the
 * colors when pressed.
 * @param action	- pointer to an Action
 * @param state		- State that the action handlers belong to
 */
void ImageButton::mousePress(Action* action, State* state)
{
	if (_group != NULL)
	{
		if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		{
			(*_group)->invert((*_group)->getColor() + 3);
			*_group = this;
			invert(_color + 3);
		}
	}
	else if (_inverted == false
		&& isButtonPressed() == true
		&& isButtonHandled(action->getDetails()->button.button) == true)
	{
		_inverted = true;
		invert(_color + 3);
	}

	InteractiveSurface::mousePress(action, state);
}

/**
 * Sets the button as the released button if it's part of a group.
 * @param action	- pointer to an Action
 * @param state		- State that the action handlers belong to
 */
void ImageButton::mouseRelease(Action* action, State* state)
{
	if (_inverted == true
		&& isButtonHandled(action->getDetails()->button.button) == true)
	{
		_inverted = false;
		invert(_color + 3);
	}

	InteractiveSurface::mouseRelease(action, state);
}

/**
 * Inverts a button explicitly either ON or OFF and keeps track of the state
 * using internal variables.
 * @param press - true to set this button as pressed
 */
void ImageButton::toggle(bool press)
{
	if (_inverted != press)
	{
		_inverted = !_inverted;
		invert(_color + 3);
	}
}

/**
 * Forces a group of buttons to automatically switch to this ImageButton.
 * @note Does not require user input.
 */
void ImageButton::releaseButtonGroup()
{
	(*_group)->invert((*_group)->getColor() + 3);
	*_group = this;
	invert(_color + 3);
}

}
