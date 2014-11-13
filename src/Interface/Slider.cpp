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

#include "Slider.h"

#include <algorithm>

#include "../fmath.h"

#include "../Engine/Action.h"

#include "../Interface/Frame.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"


namespace OpenXcom
{

/**
 * Sets up a slider with the specified size and position.
 * @param width		- width in pixels
 * @param height	- height in pixels
 * @param x			- X position in pixels
 * @param y			- Y position in pixels
 */
Slider::Slider(
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
		_pos(0.0),
		_min(0),
		_max(100),
		_pressed(false),
		_change(0),
		_offsetX(0)
{
	_thickness = 5;
	_textness = 8;

	_txtMinus = new Text(
					_textness,
					height - 2,
					x - 1,
					y);
	_txtPlus = new Text(
					_textness,
					height - 2,
					x + width - _textness,
					y);
	_frame = new Frame(
					width - (_textness * 2),
					_thickness,
					x + _textness,
					y + (height - _thickness) / 2);
	_button = new TextButton(10, height, x, y);


	_frame->setThickness(_thickness);

	_txtMinus->setAlign(ALIGN_CENTER);
	_txtMinus->setVerticalAlign(ALIGN_MIDDLE);
	_txtMinus->setText(L"-");

	_txtPlus->setAlign(ALIGN_CENTER);
	_txtPlus->setVerticalAlign(ALIGN_MIDDLE);
	_txtPlus->setText(L"+");

	_minX = _frame->getX();
	_maxX = _frame->getX() + _frame->getWidth() - _button->getWidth();

//kL	setValue(_pos);
	setValue(static_cast<int>(_pos)); // kL
}

/**
 * Deletes contents.
 */
Slider::~Slider()
{
	delete _txtMinus;
	delete _txtPlus;
	delete _frame;
	delete _button;
}

/**
 * Changes the position of the surface in the X axis.
 * @param x - X position in pixels
 */
void Slider::setX(int x)
{
	Surface::setX(x);

	_txtMinus->setX(x - 1);
	_txtPlus->setX(x + getWidth() - _textness);

	_frame->setX(getX() + _textness);

	_minX = _frame->getX();
	_maxX = _frame->getX() + _frame->getWidth() - _button->getWidth();

//kL	setValue(_pos);
	setValue(static_cast<int>(_pos)); // kL
}

/**
 * Changes the position of the surface in the Y axis.
 * @param y - Y position in pixels
 */
void Slider::setY(int y)
{
	Surface::setY(y);

	_txtMinus->setY(y);
	_txtPlus->setY(y);

	_frame->setY(getY() + (getHeight() - _thickness) / 2);
	_button->setY(getY());
}

/**
 * Changes the various resources needed for text rendering.
 * The different fonts need to be passed in advance since the
 * text size can change mid-text, and the language affects
 * how the text is rendered.
 * @param big	- pointer to large-size font
 * @param small	- pointer to small-size font
 * @param lang	- pointer to current language
 */
void Slider::initText(
		Font* big,
		Font* small,
		Language* lang)
{
	_txtMinus->initText(big, small, lang);
	_txtPlus->initText(big, small, lang);

	_button->initText(big, small, lang);
}

/**
 * Enables/disables high contrast color. Mostly used for Battlescape.
 * @param contrast - high contrast setting
 */
void Slider::setHighContrast(bool contrast)
{
	_txtMinus->setHighContrast(contrast);
	_txtPlus->setHighContrast(contrast);

	_frame->setHighContrast(contrast);
	_button->setHighContrast(contrast);
}

/**
 * Changes the color used to render the slider.
 * @param color - color value
 */
void Slider::setColor(Uint8 color)
{
	_txtMinus->setColor(color);
	_txtPlus->setColor(color);

	_frame->setColor(color);
	_button->setColor(color);
}

/**
 * Returns the color used to render the slider.
 * @return, color value
 */
Uint8 Slider::getColor() const
{
	return _button->getColor();
}

/**
 * Replaces a certain amount of colors in the slider's palette.
 * @param colors		- pointer to the set of colors
 * @param firstcolor	- offset of the first color to replace
 * @param ncolors		- amount of colors to replace
 */
void Slider::setPalette(
		SDL_Color* colors,
		int firstcolor,
		int ncolors)
{
	Surface::setPalette(colors, firstcolor, ncolors);

	_txtMinus->setPalette(colors, firstcolor, ncolors);
	_txtPlus->setPalette(colors, firstcolor, ncolors);

	_frame->setPalette(colors, firstcolor, ncolors);
	_button->setPalette(colors, firstcolor, ncolors);
}

/**
 * Automatically updates the slider when the mouse moves.
 * @param action - pointer to an Action
 * @param state - state that the action handlers belong to
 */
void Slider::handle(Action* action, State* state)
{
	InteractiveSurface::handle(action, state);
//	_button->handle(action, state);
	if (_pressed
		&& (action->getDetails()->type == SDL_MOUSEMOTION
			|| action->getDetails()->type == SDL_MOUSEBUTTONDOWN))
	{
		int cursorX = static_cast<int>(action->getAbsoluteXMouse());
		double buttonX = static_cast<double>(std::min(
													std::max(
															_minX,
															cursorX + _offsetX),
													_maxX));
		double pos = (buttonX - static_cast<double>(_minX)) / static_cast<double>(_maxX - _minX);

		int value = _min + static_cast<int>(Round(static_cast<double>(_max - _min) * pos));
		setValue(value);

		if (_change)
			(state->*_change)(action);
	}
}

/**
 * Moves the slider to the new value position.
 * @param value - new value
 */
void Slider::setPosition(double pos)
{
	_pos = pos;
	_button->setX(static_cast<int>(floor(static_cast<double>(_minX) + static_cast<double>(_maxX - _minX) * _pos)));
}

/**
 * Changes the range of values the slider can contain.
 * @param min - minimum value
 * @param max - maximum value
 */
void Slider::setRange(
		int min,
		int max)
{
	_min = min;
	_max = max;

	setValue(_value);
}

/**
 * Changes the current value of the slider and positions it appropriately.
 * @param value - new value
 */
void Slider::setValue(int value)
{
	if (_min < _max)
		_value = std::min(std::max(_min, value), _max);
	else
		_value = std::min(std::max(_max, value), _min);

	double pos = (double)(_value - _min) / (double)(_max - _min);
	setPosition(pos);
}

/**
* Returns the current value of the slider.
* @return Value.
*/
int Slider::getValue() const
{
	return _value;
}

/**
 * Blits the slider contents.
 * @param surface - pointer to a Surface to blit onto
 */
void Slider::blit(Surface* surface)
{
	Surface::blit(surface);

	if (_visible
		&& !_hidden)
	{
		_txtMinus->blit(surface);
		_txtPlus->blit(surface);

		_frame->blit(surface);
		_button->blit(surface);
	}
}

/**
 * The slider only moves while the button is pressed.
 * @param action - pointer to an Action
 * @param state - state that the action handlers belong to
 */
void Slider::mousePress(Action* action, State* state)
{
	InteractiveSurface::mousePress(action, state);

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_pressed = true;

		int cursorX = static_cast<int>(action->getAbsoluteXMouse());
		if (cursorX >= _button->getX()
			&& cursorX < _button->getX() + _button->getWidth())
		{
			_offsetX = _button->getX() - cursorX;
		}
		else
			_offsetX = -_button->getWidth() / 2;
	}
}

/**
 * The slider stops moving when the button is released.
 * @param action - pointer to an Action
 * @param state - state that the action handlers belong to
 */
void Slider::mouseRelease(Action* action, State* state)
{
	InteractiveSurface::mouseRelease(action, state);

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		_pressed = false;
		_offsetX = 0;
	}
}

/**
 * Sets a function to be called every time the slider's value changes.
 * @param handler Action handler.
 */
void Slider::onChange(ActionHandler handler)
{
	_change = handler;
}

}
