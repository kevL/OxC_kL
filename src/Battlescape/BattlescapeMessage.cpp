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

#include "BattlescapeMessage.h"

#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/Window.h"


namespace OpenXcom
{

/**
 * Sets up a blank Battlescape message with the specified size and position.
 * @param width		- width in pixels
 * @param height	- height in pixels
 * @param x			- X position in pixels (default 0)
 * @param y			- Y position in pixels (default 0)
 */
BattlescapeMessage::BattlescapeMessage(
		int width,
		int height,
		int x,
		int y)
	:
		Surface(
			width,
			height,
			x,
			y)
{
	_window = new Window(
						NULL,
						width,
						height,
						x,
						y,
						POPUP_NONE);
	_window->setColor(Palette::blockOffset(0)-1);
	_window->setHighContrast();

	// HIDDEN MOVEMENT... text
	_text = new Text(
					width - 12,
					height - 14,
					x,
					y + 14);
	_text->setColor(Palette::blockOffset(0)-1);
	_text->setAlign(ALIGN_CENTER);
	_text->setVerticalAlign(ALIGN_MIDDLE);
	_text->setHighContrast();
}

/**
 * Deletes surfaces.
 */
BattlescapeMessage::~BattlescapeMessage()
{
	delete _window;
	delete _text;
}

/**
* Changes the position of the surface in the X axis.
* @param x - X position in pixels
*/
void BattlescapeMessage::setX(int x)
{
	Surface::setX(x);
	_window->setX(x);
	_text->setX(x);
}

/**
* Changes the position of the surface in the Y axis.
* @param y - Y position in pixels
*/
void BattlescapeMessage::setY(int y)
{
	Surface::setY(y);
	_window->setY(y);
	_text->setY(y);
}

/**
 * Changes the message background.
 * @param background - pointer to background surface
 */
void BattlescapeMessage::setBackground(Surface* background)
{
	_window->setBackground(background);
}

/**
 * Changes the message text.
 * @param message - reference the Message string
 */
void BattlescapeMessage::setText(const std::wstring& message)
{
	_text->setText(message);
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
void BattlescapeMessage::initText(
		Font* big,
		Font* small,
		Language* lang)
{
	_text->initText(big, small, lang);
	_text->setBig();
}

/**
 * Replaces a certain amount of colors in the surface's palette.
 * @param colors		- pointer to the set of colors
 * @param firstcolor	- offset of the first color to replace (default 0)
 * @param ncolors		- amount of colors to replace (default 256)
 */
void BattlescapeMessage::setPalette(
		SDL_Color* colors,
		int firstcolor,
		int ncolors)
{
	Surface::setPalette(colors, firstcolor, ncolors);
	_window->setPalette(colors, firstcolor, ncolors);
	_text->setPalette(colors, firstcolor, ncolors);
}

/**
 * Blits the battlescape message.
 * @param surface - pointer to a Surface
 */
void BattlescapeMessage::blit(Surface* surface)
{
	Surface::blit(surface);
	_window->blit(surface);
	_text->blit(surface);
}

/**
 * Special handling for setting the height of the battlescape message.
 * @param height - the new height
 */
void BattlescapeMessage::setHeight(int height)
{
	Surface::setHeight(height);
	_window->setHeight(height);
	_text->setHeight(height);
}

/**
 * Sets the text color of the battlescape message.
 * @param color - the new color
 */
void BattlescapeMessage::setTextColor(Uint8 color)
{
	_text->setColor(color);
}

}
