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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_NEWGAMESTATE_H
#define OPENXCOM_NEWGAMESTATE_H

#include "../Savegame/SavedGame.h"

#include "../Engine/State.h"


namespace OpenXcom
{

class Text;
class TextButton;
class ToggleTextButton;
class Window;


/**
 * New Game window that displays a list
 * of possible difficulties for creating
 * a saved game.
 */
class NewGameState
	:
		public State
{

private:
	Text
		* _txtIronman,
		* _txtTitle;
	TextButton
		* _btnBeginner,
		* _btnExperienced,
		* _btnVeteran,
		* _btnGenius,
		* _btnSuperhuman,
		* _btnCancel,
		* _btnOk,
		* _difficulty;
	ToggleTextButton* _btnIronman;
	Window* _window;


	public:
		/// Creates the New Game state.
		NewGameState(Game* game);
		/// Cleans up the New Game state.
		~NewGameState();

		/// Handler for clicking the Ok button.
		void btnOkClick(Action*);
		/// Handler for clicking the Cancel button.
		void btnCancelClick(Action*);
};

}

#endif
