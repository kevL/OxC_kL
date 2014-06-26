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

#ifndef OPENXCOM_TARGETINFOSTATE_H
#define OPENXCOM_TARGETINFOSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class AlienBase; // kL
class GeoscapeState;
class Globe;
class Target;
class Text;
class TextButton;
class TextEdit; // kL
class Window;


/**
 * Generic window used to display all the crafts targeting a certain point on the globe.
 */
class TargetInfoState
	:
		public State
{

private:

	AlienBase* _ab; // kL
	GeoscapeState* _state;
	Globe* _globe;
	Target* _target;
	Text
		* _txtTitle,
		* _txtTargetted,
		* _txtFollowers;
	TextButton
		* _btnIntercept,
		* _btnOk;
	TextEdit* _edtTarget; // kL
	Window* _window;


	public:

		/// Creates the Target Info state.
		TargetInfoState(
				Target* target,
				Globe* globe,
				GeoscapeState* state);
		/// Cleans up the Target Info state.
		~TargetInfoState();

		/// kL. Handler for changing the text in the Name edit.
		void edtTargetChange(Action* action); // kL

		/// Handler for clicking the Intercept button.
		void btnInterceptClick(Action* action);
		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
};

}

#endif
