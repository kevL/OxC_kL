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

#ifndef OPENXCOM_SOLDIERARMORSTATE_H
#define OPENXCOM_SOLDIERARMORSTATE_H

//#include <vector>

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class RuleArmor;
class Soldier;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * Select Armor window that allows changing of the armor equipped on a soldier.
 */
class SoldierArmorState
	:
		public State
{

private:
	Base* _base;
	Soldier* _soldier;
	Text
		* _txtQuantity,
		* _txtSoldier,
		* _txtType;
	TextButton* _btnCancel;
	TextList* _lstArmor;
	Window* _window;

	std::vector<RuleArmor*> _armors;


	public:
		/// Creates the Soldier Armor state.
		SoldierArmorState(
				Base* const base,
				size_t soldierId);
		/// Cleans up the Soldier Armor state.
		~SoldierArmorState();

		/// Handler for clicking the Cancel button.
		void btnCancelClick(Action* action);
		/// Handler for clicking the Weapons list.
		void lstArmorClick(Action* action);
};

}

#endif
