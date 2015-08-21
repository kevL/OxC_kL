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

#ifndef OPENXCOM_ACTIONMENUSTATE_H
#define OPENXCOM_ACTIONMENUSTATE_H

#include "BattlescapeGame.h"

#include "../Engine/State.h"


namespace OpenXcom
{

class ActionMenuItem;


/**
 * A menu popup that allows player to select a battlescape action.
 */
class ActionMenuState
	:
		public State
{

private:
	static const size_t MENU_ITEMS = 8;

	ActionMenuItem* _menuSelect[MENU_ITEMS];
	BattleAction* _action;


	/// Adds a new menu item for an action.
	void addItem(
			const BattleActionType bat,
			const std::string& desc,
			size_t* id);
	/// Checks if there is a viable execution target nearby.
	bool canExecuteTarget();


	public:
		/// Creates the Action Menu state.
		ActionMenuState(
				BattleAction* const action,
				int x,
				int y,
				bool injured);
		/// Cleans up the Action Menu state.
		~ActionMenuState();

		/// Handler for right-clicking anything.
		void handle(Action* action);

		/// Handler for clicking an action menu item.
		void btnActionMenuClick(Action* action);

		/// Updates the resolution settings - just resized the window.
		void resize(
				int& dX,
				int& dY);
};

}

#endif
