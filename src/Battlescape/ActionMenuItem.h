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

#ifndef OPENXCOM_ACTIONMENUITEM_H
#define OPENXCOM_ACTIONMENUITEM_H

#include "BattlescapeGame.h"

#include "../Engine/InteractiveSurface.h"


namespace OpenXcom
{

class Font;
class Frame;
class Game;
class Language;
class Text;


/**
 * A class that represents a single box in the action popup menu on the battlescape.
 * It shows the possible actions of an item, their TU cost and accuracy.
 * Mouse over highlights the action, when clicked the action is sent to the parent state.
 */
class ActionMenuItem
	:
		public InteractiveSurface
{

private:
	bool _highlighted;
	int
		_id,
		_tu;

	BattleActionType _action;
	Frame* _frame;
	Text
		* _txtDesc,
		* _txtTU,
		* _txtAcc;


	public:
		/// Creates a new ActionMenuItem.
		ActionMenuItem(
				int id,
				Game* game,
				int x,
				int y);
		/// Cleans up the ActionMenuItem.
		~ActionMenuItem();

		/// Assigns an action to it.
		void setAction(
				BattleActionType action,
				std::wstring description,
				std::wstring timeunits,
				std::wstring accuracy,
				int tu);
		/// Gets the assigned action.
		BattleActionType getAction() const;

		/// Gets the assigned action TUs.
		int getTUs() const;

		/// Sets the palettes.
		void setPalette(
				SDL_Color* colors,
				int firstcolor,
				int ncolors);
		/// Redraws it.
		void draw();

		/// Processes a mouse hover in event.
		void mouseIn(Action* action, State* state);
		/// Processes a mouse hover out event.
		void mouseOut(Action* action, State* state);
};

}

#endif
