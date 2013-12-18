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

#ifndef OPENXCOM_CRAFTINFOSTATE_H
#define OPENXCOM_CRAFTINFOSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class Text;
class TextButton;
class TextEdit;
class Surface;
class Window;


/**
 * Craft Info screen that shows all the
 * info of a specific craft.
 */
class CraftInfoState
	:
		public State
{

private:
	size_t _craft;

	Base* _base;
	TextButton
		* _btnArmor,
		* _btnCrew,
		* _btnEquip,
		* _btnOk,
		* _btnW1,
		* _btnW2;
	TextEdit* _edtCraft;
	Text
		* _txtDamage,
		* _txtFuel,
		* _txtW1Ammo,
		* _txtW1Max,
		* _txtW1Name,
		* _txtW2Ammo,
		* _txtW2Max,
		* _txtW2Name;
	Surface
		* _sprite,
		* _weapon1,
		* _weapon2,
		* _crew,
		* _equip;
	Window* _window;


	public:
		/// Creates the Craft Info state.
		CraftInfoState(
				Game* game,
				Base* base,
				size_t craft);
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
		/// Handler for pressing a key on the Name edit.
		void edtCraftKeyPress(Action* action);
};

}

#endif
