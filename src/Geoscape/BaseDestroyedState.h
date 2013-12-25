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
 * along with OpenXcom.  If not, see <http:///www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_BASEDESTROYEDSTATE_H
#define OPENXCOM_BASEDESTROYEDSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class Text;
class TextButton;
class Window;


/**
 * Base Destroyed screen.
 */
class BaseDestroyedState
	:
		public State
{

private:
	Base* _base;
	Text* _txtMessage;
	TextButton* _btnOk;
	Window* _window;


	public:
		/// Creates the Select Destination state.
		BaseDestroyedState(
				Game* game,
				Base* base);
		/// Cleans up the Select Destination state.
		~BaseDestroyedState();

		/// Updates the palette.
		void init();

		/// Handler for clicking the Cydonia mission button.
		void btnOkClick(Action* action);
};

}

#endif
