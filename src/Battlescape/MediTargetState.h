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
 * along with OpenXcom. If not, see <http:///www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_MEDITARGETSTATE_H
#define OPENXCOM_MEDITARGETSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class BattleUnit;

class Text;
class TextButton;
class TextList;
class Window;

struct BattleAction;


/**
 * Window that allows the player to select a targetUnit with the MediKit.
 */
class MediTargetState
	:
		public State
{

private:
	static const Uint8
		ORANGE	=  96,
		PINK	= 176;

	BattleAction* _action;

	Text
		* _txtTitle,
		* _txtWounds,
		* _txtHealth,
		* _txtEnergy,
		* _txtMorale;
	TextButton* _btnCancel;
	TextList* _lstTarget;
	Window* _window;

	std::vector<BattleUnit*> _targetUnits;


	public:
		/// Creates the Medi Target state.
		explicit MediTargetState(BattleAction* const action);
		/// Cleans up the Medi Target state.
		~MediTargetState();

		/// Resets the palette and adds targets to the TextList.
		void init();

		/// Chooses a unit to apply Medikit to.
		void lstTargetPress(Action* action);
		/// Returns to the previous screen.
		void btnCancelClick(Action* action);
};

}

#endif
