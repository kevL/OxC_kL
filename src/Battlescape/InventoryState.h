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

#ifndef OPENXCOM_INVENTORYSTATE_H
#define OPENXCOM_INVENTORYSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class BattlescapeState;
class BattleUnit;
class InteractiveSurface;
class Inventory;
class SavedBattleGame;
class Surface;
class Text;


/**
 * Screen which displays soldier's inventory.
 */
class InventoryState
	:
		public State
{

private:
	bool
		_showStats,
		_tu;

	BattlescapeState* _parent;
	InteractiveSurface
		* _btnGround,
		* _btnNext,
		* _btnOk,
		* _btnPrev,
		* _btnRank,
		* _btnUnload;
	Inventory* _inv;
	SavedBattleGame* _battleGame;
	Surface
		* _bg,
		* _selAmmo,
		* _soldier;
	Text
		* _txtName,
		* _txtItem,
		* _txtAmmo,
		* _txtWeight,
		* _txtTus,
		* _txtFAcc,
		* _txtReact,
		* _txtPSkill,
		* _txtPStr;


	public:
		/// Creates the Inventory state.
		InventoryState(
				Game* game,
				bool tu,
				BattlescapeState* parent);
		/// Cleans up the Inventory state.
		~InventoryState();

		/// Updates all soldier info.
		void init();
		/// Updates the soldier info (Weight, TU).
		void updateStats();
		/// Saves the soldiers' equipment-layout.
		void saveEquipmentLayout();
		/// Handler for clicking the OK button.

		void btnOkClick(Action* action);
		/// Handler for clicking the Previous soldier button.
		void btnPrevClick(Action* action);
		/// Handler for clicking the Next soldier button.
		void btnNextClick(Action* action);
		/// Handler for clicking the Unload weapon button.
		void btnUnloadClick(Action* action);
		/// Handler for clicking on the Ground -> button.
		void btnGroundClick(Action* action);
		/// Handler for clicking the Rank button.
		void btnRankClick(Action* action);
		/// Handler for clicking on inventory items.
		void invClick(Action* action);
		/// Handles keypresses.
		void handle(Action* action);
};

}

#endif
