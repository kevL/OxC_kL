/*
 * Copyright 2010-2013 OpenXcom Developers.
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

#ifndef OPENXCOM__SAVEDGAMESTATE_H
#define OPENXCOM__SAVEDGAMESTATE_H

#include <string>
#include <vector>

#include "OptionsBaseState.h"

#include "../Engine/State.h"


namespace OpenXcom
{

class Text;
class TextButton;
class TextList;
class Window;


/**
 * Base class for saved game screens which
 * provides the common layout and listing.
 */
class SavedGameState
	:
		public State
{

protected:
	bool
		_showMsg,
		_noUI;
	int _firstValidRow;

	OptionsOrigin _origin;

	Text
		* _txtTitle,
		* _txtName,
		* _txtTime,
		* _txtDate,
		* _txtStatus,
		* _txtDelete,
		* _txtDetails;
	TextButton* _btnCancel;
	TextList* _lstSaves;
	Window* _window;

	std::vector<std::string> _saves;
	std::vector<std::wstring> _details;


	public:
		/// Creates the Saved Game state.
		SavedGameState(
				Game* game,
				OptionsOrigin origin,
				int firstValidRow);
		/// Creates the Saved Game state (autosave option).
		SavedGameState(
				Game* game,
				OptionsOrigin origin,
				int firstValidRow,
				bool showMsg);
		/// Cleans up the Saved Game state.
		virtual ~SavedGameState();

		/// Updates the palette.
		void init();
		/// Updates the savegame list.
		virtual void updateList();
		/// Updates the status message.
		void updateStatus(const std::string& msg);

		/// Handler for clicking the Cancel button.
		void btnCancelClick(Action* action);
		/// Handler for moving the mouse over a list item.
		void lstSavesMouseOver(Action* action);
		/// Handler for moving the mouse outside the list borders.
		void lstSavesMouseOut(Action* action);
};

}

#endif
