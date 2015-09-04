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

#include "TextButton.h"

//#include <SDL.h>
//#include <SDL_mixer.h>

#include "ComboBox.h"
#include "Text.h"

#include "../Engine/Action.h"
#include "../Engine/Sound.h"


namespace OpenXcom
{

Sound* TextButton::soundPress = 0;


/**
 * Sets up a text button with the specified size and position.
 * The text is centered on the button.
 * @param width		- width in pixels
 * @param height	- height in pixels
 * @param x			- X position in pixels (default 0)
 * @param y			- Y position in pixels (default 0)
 */
TextButton::TextButton(
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
		_group(NULL),
		_contrast(1),
		_geoscapeButton(false),
		_comboBox(NULL)
{
	_text = new Text(
					width,
					height,
					0,0);
	_text->setSmall();
	_text->setAlign(ALIGN_CENTER);
	_text->setVerticalAlign(ALIGN_MIDDLE);
//	_text->setWordWrap();
}

/**
 * Deletes the contained Text.
 */
TextButton::~TextButton()
{
	delete _text;
}

/**
 *
 * @param btn - (default 0)
 */
bool TextButton::isButtonHandled(Uint8 btn)
{
	if (_comboBox != NULL)
		return (btn == SDL_BUTTON_LEFT);
	else
		return InteractiveSurface::isButtonHandled(btn);
}

/**
 * Changes the color for the button and text.
 * @param color - color value
 */
void TextButton::setColor(Uint8 color)
{
	_color = color;
	_text->setColor(color);
	_redraw = true;
}

/**
 * Returns the color for the button and text.
 * @return, color value
 */
Uint8 TextButton::getColor() const
{
	return _color;
}

/**
 * Sets the secondary color of this TextButton.
 * @param color - the color
 */
void TextButton::setSecondaryColor(Uint8 color)
{
	_text->setColor(color);
	_redraw = true;
}

/**
* Changes the color for the text only.
* @param color - color value
*/
void TextButton::setTextColor(Uint8 color)
{
	_text->setColor(color);
	_redraw = true;
}

/**
* Changes the text to use the big-size font.
*/
void TextButton::setBig()
{
	_text->setBig();
	_redraw = true;
}

/**
* Changes the text to use the small-size font.
*/
void TextButton::setSmall()
{
	_text->setSmall();
	_redraw = true;
}

/**
* Returns the font currently used by the text.
* @return, pointer to Font
*/
Font* TextButton::getFont() const
{
	return _text->getFont();
}

/**
 * Changes the various resources needed for text rendering.
 * The different fonts need to be passed in advance since the
 * text size can change mid-text, and the language affects
 * how the text is rendered.
 * @param big	- pointer to large-size Font
 * @param small	- pointer to small-size Font
 * @param lang	- pointer to current Language
 */
void TextButton::initText(
		Font* big,
		Font* small,
		Language* lang)
{
	_text->initText(big, small, lang);
	_redraw = true;
}

/**
 * Enables/disables high contrast color. Mostly used for Battlescape UI.
 * @param contrast - high contrast setting (default true)
 */
void TextButton::setHighContrast(bool contrast)
{
	_contrast = contrast ? 2 : 1;
	_text->setHighContrast(contrast);
	_redraw = true;
}

/**
 * Changes the text of the button label.
 * @param text - reference to a text string
 */
void TextButton::setText(const std::wstring& text)
{
	_text->setText(text);
	_redraw = true;
}

/**
 * Returns the text of the button label.
 * @return, text string
 */
std::wstring TextButton::getText() const
{
	return _text->getText();
}

/**
 * Gets a pointer to the Text.
 * @return, pointer to Text
 */
Text* TextButton::getTextPtr() const
{
	return _text;
}

/**
 * Changes the button group this button belongs to.
 * @param group - pointer to a pointer to the pressed button in the group
 * Null makes it a regular button.
 */
void TextButton::setGroup(TextButton** group)
{
	_group = group;
	_redraw = true;
}

/**
 * Replaces a certain amount of colors in the surface's palette.
 * @param colors		- pointer to the set of colors
 * @param firstcolor	- offset of the first color to replace (default 0)
 * @param ncolors		- amount of colors to replace (default 256)
 */
void TextButton::setPalette(
		SDL_Color* colors,
		int firstcolor,
		int ncolors)
{
	Surface::setPalette(colors, firstcolor, ncolors);
	_text->setPalette(colors, firstcolor, ncolors);
}

/**
 * Draws the labeled button.
 * The colors are inverted if the button is pressed.
 */
void TextButton::draw()
{
	Surface::draw();

	SDL_Rect rect;
	rect.x =
	rect.y = 0;
	rect.w = static_cast<Uint16>(getWidth());
	rect.h = static_cast<Uint16>(getHeight());

	Uint8 color = _color + _contrast;

	// limit highest color to darkest hue/shade in its palette-block.
//	const Uint8 topColor = ((_color / 16) * 16) + 15; // exploit INT

	for (int
			i = 0;
			i != 5;
			++i)
	{
//		if (color > topColor) color = topColor;
		if (i == 0)
			color = _color + (_contrast * 5);

		drawRect(&rect, color);

		if (i %2 == 0)
		{
			++rect.x;
			++rect.y;
		}

		--rect.w;
		--rect.h;

		switch (i)
		{
			case 0:
//				color = _color + (_contrast * 5);
//				if (color > topColor) color = topColor;

				setPixelColor(
						static_cast<int>(rect.w),
						0,
						color);
			break;

			case 1:
				color = _color + (_contrast * 2);
			break;

			case 2:
				color = _color + (_contrast * 4);
//				if (color > topColor) color = topColor;

				setPixelColor(
						static_cast<int>(rect.w) + 1,
						1,
						color);
			break;

			case 3:
				color = _color + (_contrast * 3);
			break;

			case 4:
				if (_geoscapeButton == true)
				{
					setPixelColor(0,0, _color);
					setPixelColor(1,1, _color);
				}
		}
	}

	bool press;
	if (_group == NULL)
		press = isButtonPressed();
	else
		press = (*_group == this);

	if (press == true)
	{
		if (_geoscapeButton == true)
			this->invert(_color + (_contrast * 2));
		else
			this->invert(_color + (_contrast * 3));
	}

	_text->setInvert(press);
	_text->blit(this);
}

/**
 * Sets the button as the pressed button if it's part of a group.
 * @param action	- pointer to an Action
 * @param state		- state that the action handlers belong to
 */
void TextButton::mousePress(Action* action, State* state)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT
		&& _group != NULL)
	{
		TextButton* pre = *_group;
		*_group = this;

		if (pre != NULL)
			pre->draw();

		draw();
	}

	if (isButtonHandled(action->getDetails()->button.button) == true)
	{
		if (soundPress != NULL
//			&& _group == NULL
			&& action->getDetails()->button.button != SDL_BUTTON_WHEELUP
			&& action->getDetails()->button.button != SDL_BUTTON_WHEELDOWN
			&& (_comboBox == NULL
				|| _comboBox->getVisible() == true))
		{
			soundPress->play(Mix_GroupAvailable(0));
		}

		if (_comboBox != NULL
			&& _comboBox->getVisible() == true)
		{
			_comboBox->toggle();
		}

		draw();
	}

	InteractiveSurface::mousePress(action, state);
}

/**
 * Sets the button as the released button.
 * @param action	- pointer to an Action
 * @param state		- state that the action handlers belong to
 */
void TextButton::mouseRelease(Action* action, State* state)
{
	if (isButtonHandled(action->getDetails()->button.button) == true)
		draw();

	InteractiveSurface::mouseRelease(action, state);
}

/**
 * Hooks up the button to work as part of an existing combobox toggling its
 * state when pressed.
 * @param comboBox - pointer to ComboBox
 */
void TextButton::setComboBox(ComboBox* comboBox)
{
	_comboBox = comboBox;

	if (_comboBox != NULL)
		_text->setX(-6);
//	else _text->setX(0);
}

/**
 * Sets the width of this TextButton.
 * @param width - the width to set
 */
void TextButton::setWidth(int width)
{
	Surface::setWidth(width);
	_text->setWidth(width);
}

/**
 * Sets the height of this TextButton.
 * @param height - the height to set
 */
void TextButton::setHeight(int height)
{
	Surface::setHeight(height);
	_text->setHeight(height);
}

void TextButton::setGeoscapeButton(bool geo)
{
	_geoscapeButton = geo;
}

}
