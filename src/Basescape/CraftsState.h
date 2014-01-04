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

#ifndef OPENXCOM_CRAFTSSTATE_H
#define OPENXCOM_CRAFTSSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * Equip Craft screen that lets the player manage all the crafts in a base.
 */
class CraftsState
	:
		public State
{

private:
	Base* _base;
	Text
		* _txtBase,
		* _txtCrew,
		* _txtHwp,
		* _txtName,
		* _txtStatus,
		* _txtTitle,
		* _txtWeapon;
	TextButton* _btnOk;
	TextList* _lstCrafts;
	Window* _window;


	public:
		/// Creates the Crafts state.
		CraftsState(
				Game* game,
				Base* base);
		/// Cleans up the Crafts state.
		~CraftsState();

		/// Updates the craft info.
		void init();

		/// Handler for clicking the Crafts list.
		void lstCraftsClick(Action* action);
		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
};

}

#endif
