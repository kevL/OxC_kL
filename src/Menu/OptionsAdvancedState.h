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

#ifndef OPENXCOM_OPTIONSADVANCEDSTATE_H
#define OPENXCOM_OPTIONSADVANCEDSTATE_H

#include "OptionsBaseState.h"


namespace OpenXcom
{

class InteractiveSurface;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * Options window that displays the
 * advanced game settings.
 */
class OptionsAdvancedState
	:
		public OptionsBaseState
{

private:
	size_t _boolQuantity;

	Text
		* _txtTitle,
		* _txtDescription;
	TextButton
		* _btnOk,
		* _btnCancel;
	TextList* _lstOptions;
	Window* _window;

	// intentionally avoiding using a map here, to avoid auto-sorting.
	std::vector<std::pair<std::string, bool> > _settingBoolSet;
	std::vector<std::pair<std::string, int> > _settingIntSet;


	public:
		/// Creates the Advanced state.
		OptionsAdvancedState(
				Game* game,
				OptionsOrigin origin);
		/// Cleans up the Advanced state.
		~OptionsAdvancedState();

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
		/// Handler for clicking the Cancel button.
		void btnCancelClick(Action* action);
		/// Handler for clicking an item on the menu.
		void lstOptionsPress(Action* action);
		/// Handler for moving the mouse over a menu item.
		void lstOptionsMouseOver(Action* action);
		/// Handler for moving the mouse outside the menu borders.
		void lstOptionsMouseOut(Action* action);

		/// special function to sub out strings for pathfinding settings
		std::wstring updatePathString(int sel);
};

}

#endif
