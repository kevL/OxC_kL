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

#ifndef OPENXCOM_SACKSOLDIERSTATE_H
#define OPENXCOM_SACKSOLDIERSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
//class Soldier;
class Text;
class TextButton;
class Window;

/**
 * Window shown when the player tries to sack a soldier.
 */
class SackSoldierState
	:
		public State
{

private:
	size_t _soldierId;

	Base* _base;

	Text
		* _txtSoldier,
		* _txtTitle;
	TextButton
		* _btnCancel,
		* _btnOk;
	Window* _window;


	public:
		/// Creates the Sack Soldier state.
		SackSoldierState(
				Base* base,
				size_t _soldierId);
		/// Cleans up the Sack Soldier state.
		~SackSoldierState();

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
		/// Handler for clicking the Cancel button.
		void btnCancelClick(Action* action);
};

}

#endif
