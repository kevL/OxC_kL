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

#ifndef OPENXCOM_PRODUCTIONCOMPLETESTATE_H
#define OPENXCOM_PRODUCTIONCOMPLETESTATE_H

#include <string>

#include "../Engine/State.h"

#include "../Savegame/Production.h"


namespace OpenXcom
{

class Base;
class GeoscapeState;
class Text;
class TextButton;
class Window;


/**
 * Window used to notify the player when
 * a production is completed.
 */
class ProductionCompleteState
	:
		public State
{

private:
	Base* _base;
	GeoscapeState* _state;

	ProdProgress _endType;
	Text* _txtMessage;
	TextButton
		* _btnGotoBase,
		* _btnOk,
		* _btnOk5Secs;
	Window* _window;


	public:
		/// Creates the Production Complete state.
		ProductionCompleteState(
				Game* game,
				Base* base,
				const std::wstring& item,
				GeoscapeState* state,
				ProdProgress endType = PROGRESS_COMPLETE);
		/// Cleans up the Production Complete state.
		~ProductionCompleteState();

		/// Updates the palette.
		void init();

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
		/// Handler for clicking the Ok 5sec button.
		void btnOk5SecsClick(Action* action);
		/// Handler for clicking the Go To Base button.
		void btnGotoBaseClick(Action* action);
};

}

#endif
