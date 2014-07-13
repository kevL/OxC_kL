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

#ifndef OPENXCOM_CRAFTPATROLSTATE_H
#define OPENXCOM_CRAFTPATROLSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Craft;
class GeoscapeState;
class Globe;
class Text;
class TextButton;
class Window;


/**
 * Window displayed when a craft
 * starts patrolling a waypoint.
 */
class CraftPatrolState
	:
		public State
{

private:
	Craft* _craft;
	GeoscapeState* _geo;
	Globe* _globe;
	Text
		* _txtDestination,
		* _txtPatrolling;
	TextButton
		* _btnOk,
		* _btnCenter, // kL
		* _btnRedirect;
	Window* _window;


	public:
		/// Creates the Geoscape CraftPatrol state.
		CraftPatrolState(
				Craft* craft,
				Globe* globe,
				GeoscapeState* geo);
		/// Cleans up the Geoscape CraftPatrol state.
		~CraftPatrolState();

		/// Handler for clicking the OK button.
		void btnOkClick(Action*);
		/// kL. Handler for clicking the Center button.
		void btnCenterClick(Action*); // kL
		/// Handler for clicking the Redirect Craft button.
		void btnRedirectClick(Action*);
};

}

#endif
