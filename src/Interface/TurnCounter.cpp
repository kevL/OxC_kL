/*
 * Copyright 2010-2013 OpenXcom Developers.
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
#include "../Savegame/SavedBattleGame.h"
//#include "../Battlescape/NextTurnState.h"
#include "../Engine/Palette.h"


namespace OpenXcom
{

/**
 * Creates a Turn counter of the specified size.
 * @param width, Width in pixels.
 * @param height, Height in pixels.
 * @param x, X position in pixels.
 * @param y, Y position in pixels.
 */
TurnCounter::TurnCounter(int width, int height, int x, int y, SavedBattleGame* battleGame)
	:
	Surface(width, height, x + 1, y + 1),
	_tCount(1),
	_sbgame(battleGame)
//	node(node["turn"].as<int>())
{
//	Log(LOG_INFO) << "Create TurnCounter";

	_visible = true;

	_text = new NumberText(width, height, x, y);
	setColor(Palette::blockOffset(15)+12);

	_text->setValue(_tCount);
//	_text->setValue((unsigned int)_sbgame->getTurn());
//	_text->setValue((unsigned int)getSavedTurn(node["turn"]));
/*	int t = getSavedTurn();
	Log(LOG_INFO) << "t = " << t;
	if (t < 1)
	{
		Log(LOG_INFO) << "Create TurnCounter, tCount = " << _tCount;

//		_text->setValue((unsigned int)_tCount);
	}
	else
	{
		_tCount = t;
		Log(LOG_INFO) << "Create TurnCounter, t = " << t;

//		_text->setValue((unsigned int)getSavedTurn());
//		_text->setValue((unsigned int)t);
	}

	_text->setValue((unsigned int)_tCount); */
}

/**
 * Deletes Turn counter content.
 */
TurnCounter::~TurnCounter()
{
//	Log(LOG_INFO) << "Delete TurnCounter";

	delete _text;
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
 * Sets the turn that the TurnCounter will display.
 */
/* void TurnCounter::setTurnCount(int t)
{
	_tCount = t;
} */
		
/**
 * Gets the turn from a saved game file.
 */
// int TurnCounter::getSavedTurn(const YAML::Node& node)
/* int TurnCounter::getSavedTurn()
{
//	YAML::Node& node = node["turn"].as<int>();

//	_tCount = node["turn"].as<int>(_tCount);
	return node["turn"].as<int>();
} */

/**
 * Updates the Turn display.
 */
// void TurnCounter::update(int t)
void TurnCounter::update()
{
//	_tCount = (unsigned int)_sbgame->getTurn();

	_tCount++;
//	Log(LOG_INFO) << ". TurnCounter::update() attempting update! turn = " << _tCount;

//	_text->setValue((unsigned int)_tCount);
	_text->setValue(_tCount);

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
 * Draws the Turn counter.
 */
void TurnCounter::draw()
{
	Surface::draw();
	_text->blit(this);
}

}
