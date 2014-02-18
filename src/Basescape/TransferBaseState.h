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

#ifndef OPENXCOM_TRANSFERBASESTATE_H
#define OPENXCOM_TRANSFERBASESTATE_H

#include <vector>

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * Window that lets the player pick the base to transfer items to.
 */
class TransferBaseState
	:
		public State
{

private:
	Base* _base;
	Text
		* _txtArea,
		* _txtBaseLabel,
		* _txtFunds,
		* _txtName,
		* _txtTitle;
	TextButton* _btnCancel;
	TextList* _lstBases;
	Window* _window;

	std::vector<Base*> _bases;


	public:
		/// Creates the Transfer Base state.
		TransferBaseState(
				Game* game,
				Base* base);
		/// Cleans up the Transfer Base state.
		~TransferBaseState();

		/// Handler for clicking the Cancel button.
		void btnCancelClick(Action* action);
		/// Handler for clicking the Bases list.
		void lstBasesClick(Action* action);
};

}

#endif
