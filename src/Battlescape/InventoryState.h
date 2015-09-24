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
 * Screen that displays a Battleunit's inventory.
 */
class InventoryState
	:
		public State
{

private:
/*	std::string _currentTooltip; */
	const bool _tuMode;

	int _flarePower;

	BattlescapeButton
		* _btnGround,
		* _btnNext,
		* _btnOk,
		* _btnPrev,
		* _btnRank,
		* _btnUnload;
/*		* _btnCreateTemplate,
		* _btnApplyTemplate,
		* _btnClearInventory; */
	BattlescapeState* _parent;
	Inventory* _inv;
	NumberText
		* _numOrder,
		* _tuCost,
		* _wndHead,
		* _wndTorso,
		* _wndRightArm,
		* _wndLeftArm,
		* _wndRightLeg,
		* _wndLeftLeg;
	SavedBattleGame* _battleSave;
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
		* _txtTUs,
		* _txtFAcc,
		* _txtReact,
		* _txtThrow,
		* _txtMelee,
		* _txtPSkill,
		* _txtPStr,
		* _txtUseTU,
		* _txtThrowTU,
		* _txtPsiTU;
//	Timer* _timer;

/*	std::vector<EquipmentLayoutItem*> _curInventoryTemplate; */

	/// Advances to the next/previous Unit when right/left key is depressed.
//	void keyRepeat(); // <- too twitchy.

	/// Updates the selected unit's info - weight, TU, etc.
	void updateStats();
	/// Shows woundage values.
	void updateWounds();

	/// Refresh the hover status of the mouse.
	void refreshMouse();

	/// Saves all soldiers' equipment-layouts.
	bool saveAllLayouts() const;
	/// Saves a soldier's equipment-layout.
	bool saveLayout(BattleUnit* const unit) const;

/*	/// Clears current unit's inventory. (was static)
	void clearInventory(Game* game, std::vector<BattleItem*>* unitInv, Tile* groundTile); */

	/// Sets the extra-info fields on mouseover and mouseclicks.
	void setExtraInfo(
			const BattleItem* const item,
			const RuleItem* const itemRule,
			const BattleItem* const ammo);
	/// Update the visibility and icons for the template buttons.
/*	void _updateTemplateButtons(bool isVisible); */


	public:
		/// Creates the Inventory state.
		InventoryState(
				bool tuMode = false,
				BattlescapeState* const parent = NULL);
		/// Cleans up the Inventory state.
		~InventoryState();

		/// Updates all soldier info.
		void init();
		/// Runs the timer.
//		void think();

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);

		/// Handler for clicking the Previous button.
		void btnPrevClick(Action* action);
		/// Handler for clicking the Next button.
		void btnNextClick(Action* action);

		/// Handler for clicking the Unload button.
		void btnUnloadClick(Action* action);
		/// Handler for right-clicking the Unload button.
		void btnSaveLayouts(Action* action);

		/// Handler for clicking the Ground button.
		void btnGroundClick(Action* action);
		/// Handler for right-clicking the Ground button.
		void btnUnequipUnitClick(Action* action);

		/// Handler for clicking the Rank button.
		void btnRankClick(Action* action);

		/// Handler for clicking on inventory items.
		void invClick(Action* action);

		/// Handler for showing item info.
		void invMouseOver(Action* action);
		/// Handler for hiding item info.
		void invMouseOut(Action* action);

		/// Handles keypresses.
		void handle(Action* action);

/*		/// Handler for showing tooltip.
		void txtTooltipIn(Action* action);
		/// Handler for hiding tooltip.
		void txtTooltipOut(Action* action); */
/*		/// Handler for clicking on the Create Template button.
		void btnCreateTemplateClick(Action* action);
		/// Handler for clicking the Apply Template button.
		void btnApplyTemplateClick(Action* action); */
};

}

#endif
