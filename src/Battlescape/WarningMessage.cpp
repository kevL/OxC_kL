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

#include "WarningMessage.h"

//#include <string>
//#include <SDL.h>

#include "../Engine/Timer.h"

#include "../Interface/Text.h"


namespace OpenXcom
{

/**
 * Sets up a blank warning message with the specified size and position.
 * @param width		- width in pixels
 * @param height	- height in pixels
 * @param x			- X position in pixels (default 0)
 * @param y			- Y position in pixels (default 0)
 */
WarningMessage::WarningMessage(
		int width,
		int height,
		int x,
		int y)
	:
		Surface(
			width,
			height,
			x,y),
		_color(0),
		_fade(0)
{
	_text = new Text(
					width,
					height,
					0,0);
	_text->setHighContrast();
	_text->setAlign(ALIGN_CENTER);
	_text->setVerticalAlign(ALIGN_MIDDLE);
	_text->setWordWrap();

	_timer = new Timer(80);
	_timer->onTimer((SurfaceHandler)& WarningMessage::fade);

	setVisible(false);
}

/**
 * Deletes timers.
 */
WarningMessage::~WarningMessage()
{
	delete _timer;
	delete _text;
}

/**
 * Changes the color for the message background.
 * @param color Color value.
 */
void WarningMessage::setColor(Uint8 color)
{
	_color = color;
}

/**
 * Changes the color for the message text.
 * @param color Color value.
 */
void WarningMessage::setTextColor(Uint8 color)
{
	_text->setColor(color);
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
void WarningMessage::initText(
		Font* big,
		Font* small,
		Language* lang)
{
	_text->initText(big, small, lang);
}

/**
 * Replaces a certain amount of colors in the surface's palette.
 * @param colors		- pointer to the set of colors
 * @param firstcolor 	- offset of the first color to replace (default 0)
 * @param ncolors		- amount of colors to replace (default 256)
 */
void WarningMessage::setPalette(
		SDL_Color* colors,
		int firstcolor,
		int ncolors)
{
	Surface::setPalette(colors, firstcolor, ncolors);
	_text->setPalette(colors, firstcolor, ncolors);
}

/**
 * Displays the warning message.
 * @param msg - reference to a message string
 */
void WarningMessage::showMessage(const std::wstring& msg)
{
	_text->setText(msg);
	_fade = 0;
	_redraw = true;

	setVisible();
	_timer->start();
}

/**
 * Keeps the animation timers running.
 */
void WarningMessage::think()
{
	_timer->think(NULL, this);
}

/**
 * Plays the message fade animation.
 */
void WarningMessage::fade()
{
	_redraw = true;

	++_fade;
	if (_fade == 15)
	{
		setVisible(false);
		_timer->stop();
	}
}

/**
 * Draws the warning message.
 */
void WarningMessage::draw()
{
	Surface::draw();

	Uint8 color = _color + 1 + _fade;
	if (_fade == 15)
		color -= 1;

	drawRect(
			0,0,
			static_cast<Sint16>(getWidth()),
			static_cast<Sint16>(getHeight()),
			color);

	_text->blit(this);
}

}
