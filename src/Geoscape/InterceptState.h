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

#ifndef OPENXCOM_INTERCEPTSTATE_H
#define OPENXCOM_INTERCEPTSTATE_H

#include <vector>

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class Craft;
class GeoscapeState;
class Globe;
class Target;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * Intercept window that lets the player launch crafts into missions from the Geoscape.
 */
class InterceptState
	:
		public State
{

private:
	Uint8 _cellColor;

	Base* _base;
	GeoscapeState* _geo;
	Globe* _globe;
	Target* _target;
	Text
		* _txtBase,
		* _txtCraft,
		* _txtStatus,
		* _txtWeapons;
	TextButton
		* _btnCancel,
		* _btnGotoBase;
	TextList* _lstCrafts;
	Window* _window;

	std::vector<std::wstring> _bases;

	std::vector<Craft*> _crafts;

	/// A more descriptive status of a Craft.
	std::wstring getAltStatus(Craft* const craft);

	/// Formats a duration in hours into a day & hour string.
	std::wstring formatTime(
			const int total,
			const bool delayed) const;


	public:
		/// Creates the Intercept state.
		InterceptState(
				Globe* globe,
				Base* base = NULL,
				Target* target = NULL,
				GeoscapeState* geo = NULL);
		/// Cleans up the Intercept state.
		~InterceptState();

		/// Handler for clicking the Cancel button.
		void btnCancelClick(Action* action);

		/// Handler for clicking the Go To Base button.
		void btnGotoBaseClick(Action* action);
		/// Handler for clicking the Crafts list.
		void lstCraftsLeftClick(Action* action);
		/// Handler for right clicking the Crafts list.
		void lstCraftsRightClick(Action* action);

		/// Handler for moving the mouse over a list item.
		void lstCraftsMouseOver(Action* action);
		/// Handler for moving the mouse outside the list borders.
		void lstCraftsMouseOut(Action* action);
};

}

#endif
