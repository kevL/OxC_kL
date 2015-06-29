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

#ifndef OPENXCOM_LOWFUELSTATE_H
#define OPENXCOM_LOWFUELSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Craft;
class GeoscapeState;
class Text;
class TextButton;
class Timer;
class Window;


/**
 * Window displayed when a craft starts running out of fuel
 * (only has exactly enough to make it back to base).
 */
class LowFuelState
	:
		public State
{

private:
	Craft* _craft;
	GeoscapeState* _state;

	Text
		* _txtMessage,
		* _txtTitle;
	TextButton
		* _btnOk,
		* _btnOk5Secs;
	Timer* _blinkTimer;
	Window* _window;


	public:
		/// Creates the Low Fuel state.
		LowFuelState(
				Craft* craft,
				GeoscapeState* state);
		/// Cleans up the Low Fuel state.
		~LowFuelState();

		/// Initializes the state.
//		void init();
		/// Runs the blink timer.
		void think();
		/// Blinks the message text.
		void blink();

		/// Handler for clicking the Ok button.
		void btnOkClick(Action* action);
		/// Handler for clicking the Ok - 5 Secs button.
		void btnOk5SecsClick(Action* action);
};

}

#endif
