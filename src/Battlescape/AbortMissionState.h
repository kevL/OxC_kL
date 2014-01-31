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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_ABORTMISSION_H
#define OPENXCOM_ABORTMISSION_H

#include "../Engine/State.h"


namespace OpenXcom
{

class BattlescapeState;
class SavedBattleGame;
class Text;
class TextButton;
class Window;


/**
 * Screen which asks for confirmation to abort mission.
 */
class AbortMissionState
	:
		public State
{

private:
    int
        _inExitArea,
        _outExitArea;

    BattlescapeState* _state;
    Text
        * _txtAbort,
        * _txtInExit,
        * _txtOutsideExit;
    TextButton
        * _btnCancel,
        * _btnOk;
    SavedBattleGame* _battleGame;
    Window* _window;


	public:
		/// Creates the Abort Mission state.
		AbortMissionState(
                Game* game,
                SavedBattleGame* battleGame,
                BattlescapeState* state);
		/// Cleans up the Abort Mission state.
		~AbortMissionState();

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
		/// Handler for clicking the Cancel button.
		void btnCancelClick(Action* action);
};

}

#endif
