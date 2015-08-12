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

#ifndef OPENXCOM_EXECUTESTATE_H
#define OPENXCOM_EXECUTESTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class BattleUnit;
class Tile;

class Text;
class TextButton;
class TextList;
class Window;

struct BattleAction;


/**
 * Window that allows the player to select a targetUnit to execute.
 */
class ExecuteState
	:
		public State
{

private:
	BattleAction* _action;

	Text* _txtTitle;
	TextButton* _btnCancel;
	TextList* _lstTarget;
	Window* _window;

	std::vector<BattleUnit*> _targetUnits;

	/// Gets an unconscious unit in valid range.
	Tile* getTargetTile();


	public:
		/// Creates the Execute state.
		explicit ExecuteState(BattleAction* const action);
		/// Cleans up the Execute state.
		~ExecuteState();

		/// Resets the palette and adds targets to the TextList.
		void init();

		/// Chooses a unit to apply execution to.
		void lstTargetPress(Action* action);
		/// Returns to the previous screen.
		void btnCancelClick(Action* action);
};

}

#endif
