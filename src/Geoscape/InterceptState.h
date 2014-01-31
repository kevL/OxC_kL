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

#ifndef OPENXCOM_INTERCEPTSTATE_H
#define OPENXCOM_INTERCEPTSTATE_H

#include <vector>

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class Craft;
class Globe;
class Target;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * Intercept window that lets the player launch
 * crafts into missions from the Geoscape.
 */
class InterceptState
	:
		public State
{

private:
	Uint8 _cellColor;

	Base* _base;
	Globe* _globe;
	Target* _target; // kL_note: Doesn't seem to be used... really.
	Text
		* _txtBase,
		* _txtCraft,
		* _txtStatus,
//		* _txtTitle,
		* _txtWeapons;
	TextButton* _btnCancel;
	TextList* _lstCrafts;
	Window* _window;

	std::vector<std::wstring> _bases;

	std::vector<Craft*> _crafts;

	/// kL. A more descriptive state of the Craft.
	std::wstring getAltStatus(Craft* craft);


	public:
		/// Creates the Intercept state.
		InterceptState(
				Game* game,
				Globe* globe,
				Base* base = 0,
				Target* target = 0);
		/// Cleans up the Intercept state.
		~InterceptState();

		/// Handler for clicking the Cancel button.
		void btnCancelClick(Action* action);

		/// Handler for clicking the Crafts list.
		void lstCraftsLeftClick(Action* action);
		/// Handler for right clicking the Crafts list.
		void lstCraftsRightClick(Action* action);

		// kL. These two are from SavedGameState:
		/// Handler for moving the mouse over a list item.
		void lstCraftsMouseOver(Action* action); // kL
		/// Handler for moving the mouse outside the list borders.
		void lstCraftsMouseOut(Action* action); // kL
};

}

#endif
