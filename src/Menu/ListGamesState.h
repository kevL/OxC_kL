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

#ifndef OPENXCOM_LISTGAMESSTATE_H
#define OPENXCOM_LISTGAMESSTATE_H

//#include <string>
//#include <vector>

#include "OptionsBaseState.h"

//#include "../Engine/Options.h"
#include "../Engine/State.h"

#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

class ArrowButton;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * Base class for saved game screens which provides the common layout and listing.
 */
class ListGamesState
	:
		public State
{

protected:
	bool
		_autoquick,
		_editMode,
		_refresh,
		_sortable;
	size_t _firstValid;

	OptionsOrigin _origin;

	ArrowButton
		* _sortName,
		* _sortDate;
	Text
		* _txtTitle,
		* _txtName,
		* _txtDate,
		* _txtDelete,
		* _txtDetails;
	TextButton* _btnCancel;
	TextList* _lstSaves;
	Window* _window;

	std::vector<SaveInfo> _saves;


	/// Updates the list-order arrows.
	void updateArrows();


	public:
		/// Creates the Saved Game state.
		ListGamesState(
				OptionsOrigin origin,
				size_t firstValid,
				bool autoquick);
		/// Cleans up the Saved Game state.
		virtual ~ListGamesState();

		/// Sets up the saves list.
		void init();
		/// Checks when popup is done.
		void think();

		/// Sorts the savegame list.
		void sortList(SaveSort order);

		/// Updates the savegame list.
		virtual void updateList();

		/// Handler for clicking the Cancel button.
		void btnCancelClick(Action* action);
		/// Handler for pressing the Escape key.
		void btnCancelKeypress(Action* action);

		/// Handler for moving the mouse over a list item.
		void lstSavesMouseOver(Action* action);
		/// Handler for moving the mouse outside the list borders.
		void lstSavesMouseOut(Action* action);
		/// Handler for clicking the Saves list.
		virtual void lstSavesPress(Action* action);

		/// Handler for clicking the Name arrow.
		void sortNameClick(Action* action);
		/// Handler for clicking the Date arrow.
		void sortDateClick(Action* action);

		/// disables the sort buttons.
		void disableSort();
};

}

#endif
