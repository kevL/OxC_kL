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

#include "TurnCounter.h"
#include "NumberText.h"
#include "../Engine/Palette.h"
//#include "../Savegame/SavedBattleGame.h"


namespace OpenXcom
{

unsigned int kL_TurnCount = 1;

/**
 * Creates a Turn counter of the specified size.
 * @param width, Width in pixels.
 * @param height, Height in pixels.
 * @param x, X position in pixels.
 * @param y, Y position in pixels.
 */
TurnCounter::TurnCounter(int width, int height, int x, int y)
	:
		Surface(width, height, x + 1, y + 1),
		_tCount(1)
{
	//Log(LOG_INFO) << "Create TurnCounter";

	_visible = true;

	_text = new NumberText(width, height, x, y);
	setColor(Palette::blockOffset(15)+12);

//	_text->setValue(_sbgame->getTurn());
	_text->setValue(kL_TurnCount);
//	_text->setValue(_tCount);
}

/**
 * Deletes Turn counter content.
 */
TurnCounter::~TurnCounter()
{
	//Log(LOG_INFO) << "Delete TurnCounter";

	delete _text;
}

/**
 * Updates the Turn display.
 */
// void TurnCounter::update(int t)
void TurnCounter::update()
{
	_text->setValue(kL_TurnCount);
//	_text->setValue(_tCount);

	_redraw = true;
}

/**
 * Replaces a certain amount of colors in the Turn counter palette.
 * @param colors Pointer to the set of colors.
 * @param firstcolor Offset of the first color to replace.
 * @param ncolors Amount of colors to replace.
 */
void TurnCounter::setPalette(SDL_Color* colors, int firstcolor, int ncolors)
{
	Surface::setPalette(colors, firstcolor, ncolors);
	_text->setPalette(colors, firstcolor, ncolors);
}

/**
 * Sets the text color of the counter.
 * @param color The color to set.
 */
void TurnCounter::setColor(Uint8 color)
{
	_text->setColor(color);
}

/**
 * Draws the Turn counter.
 */
void TurnCounter::draw()
{
	Surface::draw();
	_text->blit(this);
}

}
