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

#ifndef OPENXCOM_CRAFTSOLDIERSSTATE_H
#define OPENXCOM_CRAFTSOLDIERSSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * Select Squad screen that lets the player
 * pick the soldiers to assign to a craft.
 */
class CraftSoldiersState
	:
		public State
{

private:
	size_t _craft;

	Base* _base;
	Text
		* _txtAvailable,
		* _txtBaseLabel,
		* _txtCraft,
		* _txtName,
		* _txtRank,
		* _txtTitle,
		* _txtUsed;
	TextButton
		* _btnOk,
		* _btnUnload;
	TextList* _lstSoldiers;
	Window* _window;


	public:
		/// Creates the Craft Soldiers state.
		CraftSoldiersState(
				Game* game,
				Base* base,
				size_t craft);
		/// Cleans up the Craft Soldiers state.
		~CraftSoldiersState();

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
		/// kL. Handler for clicking the Unload button.
		/// * NB: This relies on no two transport craft having the same name!!!!!
		/// * See, void CraftInfoState::edtCraftKeyPress(Action* action) etc.
		void btnUnloadClick(Action* action);

		/// Updates the soldiers list.
		void init();
//		/// Shows Soldiers in a list.
//		void populateList();
		/// Handler for clicking the Soldiers reordering button.
		void lstLeftArrowClick(Action* action);
		/// Moves a soldier up.
/*		void moveSoldierUp(
				Action* action,
				int row,
				bool max = false); */
		/// Handler for clicking the Soldiers reordering button.
		void lstRightArrowClick(Action* action);
		/// Moves a soldier down.
/*		void moveSoldierDown(
				Action* action,
				int row,
				bool max = false); */
		/// Handler for clicking the Soldiers list.
		void lstSoldiersClick(Action* action);
		/// Handler for pressing-down a mouse-button in the list.
//		void lstSoldiersMousePress(Action* action);
};

}

#endif
