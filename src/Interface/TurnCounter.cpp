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
//#include "../Battlescape/BattlescapeGame.h"	// kL

//#include <typeinfo>
//using namespace std;

namespace OpenXcom
{

/**
 * Creates a Turn counter of the specified size.
 * @param width Width in pixels.
 * @param height Height in pixels.
 * @param x X position in pixels.
 * @param y Y position in pixels.
 */
TurnCounter::TurnCounter(int width, int height, int x, int y)//, SavedBattleGame battleGame)
//	: Surface(width, height, x, y)//, BattlescapeGame()//, _battleGame(&battleGame)
//	:
//	_tCount(0)
{
	Log(LOG_INFO) << "Create TurnCounter";		// kL

//	_tCount = 0;

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
	Log(LOG_INFO) << "Delete TurnCounter";		// kL
//	delete _text;
//	delete _timer;
}

/**
 * Updates the Turn.
 */
void TurnCounter::update(int t)
{
//	Log(LOG_INFO) << ". pre Battle Turn";

//	tCount++;

	Log(LOG_INFO) << ". TurnCounter::update() : " << t;

//	_tCount = t;
	int _tCount = t;
//	_tCount = const_cast<int&> (t);
//	_tCount = &t;

//	setCount(t);

	Log(LOG_INFO) << ". TurnCounter::update() END : " << _tCount;

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
 * 
 */
void TurnCounter::setCount(int t)
{
	Log(LOG_INFO) << ". TurnCounter::setCount() : " << t;
//	Log(LOG_INFO) << ". . typeof (t) : " << typeid(t).name();
//	Log(LOG_INFO) << ". . typeof (_tCount) : " << typeid(_tCount).name();

//	Uint8 TEMP = (unsigned char)t;
//	Log(LOG_INFO) << ". . typeof (TEMP) : " << typeid(TEMP).name();

//	_tCount = (unsigned char)t;

	Log(LOG_INFO) << ". TurnCounter::setCount() END";
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
