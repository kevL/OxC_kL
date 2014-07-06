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

#ifndef OPENXCOM_CRAFTARMORSTATE_H
#define OPENXCOM_CRAFTARMORSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * Select Armor screen that lets the player
 * pick armor for the soldiers on the craft.
 */
class CraftArmorState
	:
		public State
{

private:
	size_t
		_craftID,
		_curRow;

	Base* _base;
	Text
		* _txtArmor,
		* _txtBaseLabel,
		* _txtCraft,
		* _txtName,
		* _txtTitle;
	TextButton* _btnOk;
	TextList* _lstSoldiers;
	Window* _window;


	public:
		/// Creates the Craft Armor state.
		CraftArmorState(
				Base* base,
				size_t craftID);
		/// Cleans up the Craft Armor state.
		~CraftArmorState();

		/// Updates the soldier armors.
		void init();
		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);

		/// Handler for clicking the Soldiers list.
		void lstSoldiersClick(Action* action);

		/// kL. Handler for clicking the Soldiers reordering button.
		void lstLeftArrowClick(Action* action); // kL
		/// kL. Handler for clicking the Soldiers reordering button.
		void lstRightArrowClick(Action* action); // kL
};

}

#endif
