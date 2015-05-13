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

#ifndef OPENXCOM_SCANNERSTATE_H
#define OPENXCOM_SCANNERSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class ScannerView;
class Surface;
class Timer;

struct BattleAction;


/**
 * The Scanner User Interface.
 */
class ScannerState
	:
		public State
{

private:
	BattleAction* _action;
	InteractiveSurface* _bg;
	ScannerView* _scannerView;
	Surface* _scan;
	Timer* _timerAnimate;

	/// Updates scanner interface.
	void update();
	/// Handles Minimap animation.
	void animate();


	public:
		/// Creates the ScannerState.
		explicit ScannerState(BattleAction* action);
		///
		~ScannerState();

		/// Handler for right-clicking anything.
		void handle(Action* action);

		/// Handles timers.
		void think();

		/// Handler for exiting the state.
		void exitClick(Action* action);
};

}

#endif
