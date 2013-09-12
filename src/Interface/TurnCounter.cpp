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
//#include <cmath>
//#include "../Engine/Palette.h"
//#include "../Engine/Action.h"
//#include "../Engine/Timer.h"
//#include "../Engine/Options.h"
//#include "NumberText.h"
//#include "../Savegame/SavedBattleGame.h"		// kL
//#include "../Battlescape/BattlescapeState.h"	// kL
//#include "../Battlescape/BattlescapeGame.h"		// kL

namespace OpenXcom
{

/**
 * Creates a Turn counter of the specified size.
 * @param width Width in pixels.
 * @param height Height in pixels.
 * @param x X position in pixels.
 * @param y Y position in pixels.
 */
TurnCounter::TurnCounter(int width, int height, int x, int y, int tCount)//, SavedBattleGame battleGame)
//	: Surface(width, height, x, y)//, BattlescapeGame()//, _battleGame(&battleGame)
	: tCount(0)
{
//	tCount = 0;
//	_visible = Options::getBool("fpsCounter");
//	_visible = true;

//	_timer = new Timer(1000);
//	_timer->onTimer((SurfaceHandler) &TurnCounter::update);
//	_timer->start();

//	_text = new NumberText(width, height, x, y);
//	setColor(Palette::blockOffset(15)+12);
}

/**
 * Deletes Turn counter content.
 */
TurnCounter::~TurnCounter()
{
//	delete _text;
//	delete _timer;
}

/**
 * Updates the Turn.
 */
void TurnCounter::update()
{
	Log(LOG_INFO) << ". pre Battle Turn";

//	tCount++;

	Log(LOG_INFO) << ". Battle Turn : " << tCount;


//	int fps = (int)floor((double)_frames / _timer->getTime() * 1000);
//	_text->setValue(fps);

//	SavedBattleGame turn;
//	unsigned int t = turn.getTurn();

//	SavedBattleGame *_bgame;
//	int t = SavedBattleGame::getTurn();
//	BattlescapeGame _bsgame;
//	Log(LOG_INFO) << ". pre Battle Turn : " << _bsgame->getSave()->getTurn();
//	Log(LOG_INFO) << ". Battle Turn : " << _sbgame->getTurn();
//	int t = _battleGame->getTurn();
//	int t = (int)(_bsgame->getSave()->getTurn());
//	Log(LOG_INFO) << ". Battle Turn : " << t;

//	_text->setValue(t);

//	_turncount = 0;
//	_redraw = true;
}

/**
 * Replaces a certain amount of colors in the Turn counter palette.
 * @param colors Pointer to the set of colors.
 * @param firstcolor Offset of the first color to replace.
 * @param ncolors Amount of colors to replace.
 */
/* void TurnCounter::setPalette(SDL_Color *colors, int firstcolor, int ncolors)
{
	Surface::setPalette(colors, firstcolor, ncolors);
	_text->setPalette(colors, firstcolor, ncolors);
} */

/**
 * Sets the text color of the counter.
 * @param color The color to set.
 */
/* void TurnCounter::setColor(Uint8 color)
{
	_text->setColor(color);
} */

/**
 * Shows / hides the Turn counter.
 * @param action Pointer to an action.
 */
/* void TurnCounter::handle(Action *action)
{
	if (action->getDetails()->type == SDL_KEYDOWN
		&& action->getDetails()->key.keysym.sym == Options::getInt("keyFps"))
	{
		_visible = !_visible;
		Options::setBool("fpsCounter", _visible);
	}
} */

/**
 * Advances turn counter.
 */
/* void TurnCounter::think()
{
//	_turncount++;
//	_timer->think(0, this);
} */

/**
 * Draws the Turn counter.
 */
/* void TurnCounter::draw()
{
	Surface::draw();
	_text->blit(this);
} */

}
