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

#ifndef OPENXCOM_ACTIONMENUITEM_H
#define OPENXCOM_ACTIONMENUITEM_H

#include "BattlescapeGame.h"

#include "../Engine/InteractiveSurface.h"


namespace OpenXcom
{

class Frame;
class Text;


/**
 * A class that represents a single box in the action popup menu on the
 * battlescape.
 * @note It shows the possible actions of an item along with the TU cost and
 * accuracy. Mouse over highlights the action and when clicked the action is
 * sent to the parent state.
 */
class ActionMenuItem
	:
		public InteractiveSurface
{

private:
	bool _highlighted;
	int _tu;
	Uint8 _highlightModifier;

	BattleActionType _action;
	Frame* _frame;
	Text
		* _txtAcc,
		* _txtDesc,
		* _txtTU;


	public:
		/// Creates a new ActionMenuItem.
		ActionMenuItem(
				size_t id,
				const Game* const game,
				int x,
				int y);
		/// Cleans up the ActionMenuItem.
		~ActionMenuItem();

		/// Assigns an action to it.
		void setAction(
				BattleActionType batType,
				const std::wstring& desc,
				const std::wstring& acu,
				const std::wstring& tu,
				int tuCost);
		/// Gets the assigned action.
		BattleActionType getAction() const;

		/// Gets the assigned action TUs.
		int getActionMenuTu() const;

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
