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

#ifndef OPENXCOM_TURNCOUNTER_H
#define OPENXCOM_TURNCOUNTER_H

//#include "../Engine/Surface.h"


namespace OpenXcom
{

//class NumberText;
//class Timer;
//class Action;
//class SavedBattleGame;
//class BattlescapeState;
//class BattlescapeGame;

/**
 * Gets Battlescape turn and displays it in a NumberText surface.
 */
class TurnCounter// : public Surface//, public SavedBattleGame//, public BattlescapeGame
{
	private:
//		Uint8 _tCount;
		void setCount(int t);

//		NumberText *_text;
	//	int t = 0;
	//	Timer *_timer;
//		SavedBattleGame* _sbgame;
	//	BattlescapeGame *_bsgame;

	public:
		/// Creates a new Turn counter linked to a game.
		TurnCounter(int width, int height, int x, int y);//, SavedBattleGame battleGame);
		/// Cleans up all the Turn counter resources.
		~TurnCounter();
		// Updates Turn counter.
		void update(int t);
//		Uint8 _tCount;

		/// Sets the Turn counter's palette.
/*		void setPalette(SDL_Color *colors, int firstcolor = 0, int ncolors = 256);
		/// Sets the TurnCounter's color.
		void setColor(Uint8 color);
		/// Handles keyboard events.
	//	void handle(Action *action);
		/// Advances frame counter.
		void think();
		/// Draws the Turn counter.
		void draw(); */
};

}

#endif
