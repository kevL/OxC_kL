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

#ifndef OPENXCOM_STORESMATRIXSTATE_H
#define OPENXCOM_STORESMATRIXSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * Stores window that displays all the items currently stored in a base.
 */
class StoresMatrixState
	:
		public State
{

private:
	static const size_t MTX_BASES = 8;

	Base* _base;
	Text
		* _txtBaseLabel,
		* _txtBase_0,
		* _txtBase_1,
		* _txtBase_2,
		* _txtBase_3,
		* _txtBase_4,
		* _txtBase_5,
		* _txtBase_6,
		* _txtBase_7,
		* _txtItem,
		* _txtTitle;
	TextButton* _btnOk;
	TextList* _lstMatrix;
	Window* _window;


	public:
		/// Creates the Stores Matrix state.
		StoresMatrixState(Base* base);
		/// Cleans up the Stores Matrix state.
		~StoresMatrixState();

		/// Handler for clicking the OK button.
		void btnOkClick(Action*);
};

}

#endif
