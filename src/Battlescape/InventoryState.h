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

#ifndef OPENXCOM_INVENTORYSTATE_H
#define OPENXCOM_INVENTORYSTATE_H

#include "../Engine/State.h"

#include "../Savegame/EquipmentLayoutItem.h"


namespace OpenXcom
{

class BattlescapeButton;
class BattlescapeState;
class BattleItem;
class BattleUnit;
class Inventory;
class NumberText;
class RuleItem;
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
	const bool _tuMode;
//	std::string _currentTooltip;

	BattlescapeButton
		* _btnGround,
		* _btnNext,
		* _btnOk,
		* _btnPrev,
		* _btnRank,
		* _btnUnload;
//		* _btnCreateTemplate,
//		* _btnApplyTemplate,
//		* _btnClearInventory;
	BattlescapeState* _parent;
	Inventory* _inv;
	NumberText
		* _numOrder,
		* _tuCost;
	SavedBattleGame* _battleGame;
	Surface
		* _bg,
		* _gender,
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
		* _txtThrow,
		* _txtMelee,
		* _txtPSkill,
		* _txtPStr,
		* _txtUseTU,
		* _txtThrowTU,
		* _txtPsiTU;
//	std::vector<EquipmentLayoutItem*> _curInventoryTemplate;

	/// Refresh the hover status of the mouse.
	void _refreshMouse();
	/// Sets the extra-info fields on mouseover and mouseclicks.
	void setExtraInfo(
			const BattleItem* const item,
			const RuleItem* const itemRule,
			const BattleItem* const ammo);
	/// Update the visibility and icons for the template buttons.
//	void _updateTemplateButtons(bool isVisible);


	public:
		/// Creates the Inventory state.
		InventoryState(
				bool tuMode,
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

		/// Handler for clicking the Clear Inventory button.
		void btnClearInventoryClick(Action* action);

		/// Handler for clicking on inventory items.
		void invClick(Action* action);

		/// Handler for showing item info.
		void invMouseOver(Action* action);
		/// Handler for hiding item info.
		void invMouseOut(Action* action);

		/// Handles keypresses.
		void handle(Action* action);

		/// Handler for showing tooltip.
//		void txtTooltipIn(Action* action);
		/// Handler for hiding tooltip.
//		void txtTooltipOut(Action* action);
		/// Handler for clicking on the Create Template button.
//		void btnCreateTemplateClick(Action* action);
		/// Handler for clicking the Apply Template button.
//		void btnApplyTemplateClick(Action* action);
};

}

#endif
