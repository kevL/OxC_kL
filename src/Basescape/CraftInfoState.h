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

#ifndef OPENXCOM_CRAFTINFOSTATE_H
#define OPENXCOM_CRAFTINFOSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class Craft;
class Text;
class TextButton;
class TextEdit;
class Surface;
class Window;


/**
 * Craft Info screen that shows all the info of a specific craft.
 */
class CraftInfoState
	:
		public State
{

private:
	size_t _craftId;
	std::wstring _defaultName;

	Base* _base;
	Craft* _craft;
	Surface
		* _sprite,
		* _weapon1,
		* _weapon2,
		* _crew,
		* _equip;
	Text
		* _txtBaseLabel,
		* _txtCost,
		* _txtDamage,
		* _txtFuel,
		* _txtRadar,
		* _txtStatus,
		* _txtW1Ammo,
		* _txtW1Name,
		* _txtW2Ammo,
		* _txtW2Name,
		* _txtKills;
	TextButton
		* _btnArmor,
		* _btnCrew,
		* _btnEquip,
		* _btnInventory,
		* _btnOk,
		* _btnW1,
		* _btnW2;
	TextEdit* _edtCraft;
	Window* _window;

	/// Formats an amount of time.
	std::wstring formatTime(
			const int time,
			const bool delayed) const;
	/// Sets current cost to send the Craft on a mission.
	void calcCost();


	public:
		/// Creates the Craft Info state.
		CraftInfoState(
				Base* base,
				size_t craftId);
		/// Cleans up the Craft Info state.
		~CraftInfoState();

		/// Updates the craft info.
		void init();

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
		/// Handler for clicking the 1 button.
		void btnW1Click(Action* action);
		/// Handler for clicking the 2 button.
		void btnW2Click(Action* action);
		/// Handler for clicking the Crew button.
		void btnCrewClick(Action* action);
		/// Handler for clicking the Equipment button.
		void btnEquipClick(Action* action);
		/// Handler for clicking the Armor button.
		void btnArmorClick(Action* action);
		/// Handler for clicking the Inventory button.
		void btnInventoryClick(Action* action);
		/// Handler for changing the text on the Name edit.
		void edtCraftChange(Action* action);
};

}

#endif
