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

#ifndef OPENXCOM_MISSIONDETECTEDSTATE_H
#define OPENXCOM_MISSIONDETECTEDSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class GeoscapeState;
class MissionSite;
class Text;
class TextButton;
class Window;


/**
 * Displays info on a detected mission site.
 */
class MissionDetectedState
	:
		public State
{

private:

	GeoscapeState* _state;
	MissionSite* _mission;
	TextButton
		* _btnCancel,
		* _btnCenter,
		* _btnIntercept;
	Text
		* _txtCity,
		* _txtTitle;
	Window* _window;


	public:

		/// Creates the Mission Detected state.
		MissionDetectedState(
				MissionSite* mission,
				const std::string& city,
				GeoscapeState* state);
		/// Cleans up the Mission Detected state.
		~MissionDetectedState();

		/// Handler for clicking the Intercept button.
		void btnInterceptClick(Action* action);
		/// Handler for clicking the Center on Site button.
		void btnCenterClick(Action* action);
		/// Handler for clicking the Cancel button.
		void btnCancelClick(Action* action);
};

}

#endif
