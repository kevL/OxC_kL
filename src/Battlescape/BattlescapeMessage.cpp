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

#include "BattlescapeMessage.h"

#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/Window.h"


namespace OpenXcom
{

/**
 * Sets up a blank Battlescape message with the specified size and position.
 * @param width Width in pixels.
 * @param height Height in pixels.
 * @param x X position in pixels.
 * @param y Y position in pixels.
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
						0,
						width,
						height,
						x,
						y,
						POPUP_NONE);
	_window->setColor(Palette::blockOffset(0)-1);
	_window->setHighContrast(true);

	// 'Hidden Movement' text
	_text = new Text(
					width - 10,
					height - 12,
					x,
					y + 12);
	_text->setColor(Palette::blockOffset(0)-1);
	_text->setAlign(ALIGN_CENTER);
	_text->setVerticalAlign(ALIGN_MIDDLE);
	_text->setHighContrast(true);
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
 * Changes the message background.
 * @param background Pointer to background surface.
 */
void BattlescapeMessage::setBackground(Surface* background)
{
	_window->setBackground(background);
}

/**
 * Changes the message text.
 * @param message Message string.
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
 * @param big, Pointer to large-size font.
 * @param small, Pointer to small-size font.
 * @param lang, Pointer to current language.
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
 * @param colors Pointer to the set of colors.
 * @param firstcolor Offset of the first color to replace.
 * @param ncolors Amount of colors to replace.
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
 */
void BattlescapeMessage::blit(Surface* surface)
{
	Surface::blit(surface);
	_window->blit(surface);
	_text->blit(surface);
}

}
