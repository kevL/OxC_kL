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
 * Select Squad screen that lets the player pick the soldiers to assign to a craft.
 */
class CraftSoldiersState
	:
		public State
{

private:
	size_t _craftID;
//	Uint8 _colorOtherCraft;

	Base* _base;
	Text
//		* _txtAvailable,
		* _txtBaseLabel,
		* _txtCraft,
		* _txtName,
		* _txtRank,
		* _txtTitle,
//		* _txtUsed,
		* _txtSpace,
		* _txtLoad;
	TextButton
		* _btnInventory,
		* _btnOk,
		* _btnUnload;
	TextList* _lstSoldiers;
	Window* _window;

	/// initializes the display list based on the craft soldier's list and the position to display
//	void initList(size_t scrl);


	public:
		/// Creates the Craft Soldiers state.
		CraftSoldiersState(
				Base* base,
				size_t craftID);
		/// Cleans up the Craft Soldiers state.
		~CraftSoldiersState();

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
		/// Handler for clicking the Unload button.
		void btnUnloadClick(Action* action);

		/// Updates the soldiers list.
		void init();

		/// Handler for clicking the Soldiers list.
		void lstSoldiersPress(Action* action);

		/// Handler for clicking the Soldiers reordering button.
		void lstLeftArrowClick(Action* action);
		/// Handler for clicking the Soldiers reordering button.
		void lstRightArrowClick(Action* action);

		/// Handler for clicking the Inventory button.
		void btnInventoryClick(Action* action);

//		/// Shows Soldiers in a list.
//		void populateList();
		/// Moves a soldier up.
/*		void moveSoldierUp(
				Action* action,
				size_t row,
				bool max = false); */
		/// Moves a soldier down.
/*		void moveSoldierDown(
				Action* action,
				size_t row,
				bool max = false); */
		/// Handler for pressing-down a mouse-button in the list.
//		void lstSoldiersMousePress(Action* action);
};

}

#endif
