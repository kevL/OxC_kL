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

#include "../Engine/Surface.h"
//#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

class NumberText;
class SavedBattleGame;
//class NextTurnState;
//class Node;

/**
 * Gets Battlescape turn and displays it in a NumberText surface.
 */
class TurnCounter
	:
	public Surface
{
	private:
		NumberText* _text;
		SavedBattleGame* _sbgame;
//		NextTurnState* _next;
		int _tCount;
//		YAML::Node node;

	public:
		/// Creates a new Turn counter linked to a game.
		TurnCounter(int width, int height, int x, int y, SavedBattleGame* battleGame);
		/// Cleans up all the Turn counter resources.
		~TurnCounter();

		/// Sets the TurnCounter's color.
		void setColor(Uint8 color);
		/// Sets the turn that the TurnCounter will display.
//		void setTurnCount(int t);
		/// Updates Turn counter.
//		void update(int t);
		void update();

		/// Sets the Turn counter's palette.
		void setPalette(SDL_Color* colors, int firstcolor = 0, int ncolors = 256);
		/// Draws the Turn counter.
		void draw();

		/// Gets the turn from a saved game file.
//		int getSavedTurn(const YAML::Node& node);
//		int getSavedTurn();
		/// Loads the AI state from YAML.
//		void load(const YAML::Node& node);
};

}

#endif
