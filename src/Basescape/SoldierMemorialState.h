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

#ifndef OPENXCOM_SOLDIERMEMORIALSTATE_H
#define OPENXCOM_SOLDIERMEMORIALSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Text;
class TextButton;
class TextList;
class Window;


/**
 * Screen that shows all the soldiers that have died throughout the game.
 */
class SoldierMemorialState
	:
		public State
{

private:
	Text
		* _txtDate,
		* _txtLost,
		* _txtName,
		* _txtRank,
		* _txtRecruited,
		* _txtTitle;
	TextButton* _btnOk;
	TextList* _lstSoldiers;
	Window* _window;


	public:
		/// Creates the Soldiers state.
		SoldierMemorialState();
		/// Cleans up the Soldiers state.
		~SoldierMemorialState();

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
		/// Handler for clicking the Soldiers list.
		void lstSoldiersPress(Action* action);
};

}

#endif
