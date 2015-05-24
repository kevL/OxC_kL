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

#ifndef OPENXCOM_STORESSTATE_H
#define OPENXCOM_STORESSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * Stores window that displays all the items currently stored at a base.
 */
class StoresState
	:
		public State
{

private:
	Base* _base;
	Text
		* _txtBaseLabel,
		* _txtItem,
		* _txtQuantity,
		* _txtSpaceUsed,
		* _txtTitle,
		* _txtTotal;
	TextButton
		* _btnOk,
		* _btnTransfers;
	TextList* _lstStores;
	Window* _window;


	public:
		/// Creates the Stores state.
		explicit StoresState(Base* base);
		/// Cleans up the Stores state.
		~StoresState();

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
		/// Handler for clicking the in-Transit button.
		void btnIncTransClick(Action* action);
};

}

#endif
