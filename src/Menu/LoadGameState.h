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

#ifndef OPENXCOM__LOADGAMESTATE
#define OPENXCOM__LOADGAMESTATE

//#include <string>
//#include <SDL.h>

#include "OptionsBaseState.h"

#include "../Engine/State.h"

#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

class Text;


/**
 * Loads a saved game with an optional message.
 */
class LoadGameState
	:
		public State
{

private:
	int _firstRun;
	std::string _file;

	Text* _txtStatus;

	OptionsOrigin _origin;


	public:
		/// Creates the Load Game state.
		LoadGameState(
				OptionsOrigin origin,
				const std::string& file,
				SDL_Color* const palette);
		/// Creates the Load Game state.
		LoadGameState(
				OptionsOrigin origin,
				SaveType type,
				SDL_Color* const palette);
		/// Cleans up the Load Game state.
		~LoadGameState();

		/// Creates the interface.
		void buildUi(
				SDL_Color* const palette,
				bool dropText = false);

		/// Validates the game.
		void init();
		/// Loads the game.
		void think();
};

}

#endif
