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
	const BattleAction* _action;
	InteractiveSurface* _bg;
	ScannerView* _scanView;
	Surface* _scan;
	Timer* _timer;

	/// Updates scanner interface.
//	void update();
	/// Handles Minimap animation.
	void animate();

	/// Handler for exiting the state.
	void exitClick(Action* action);


	public:
		/// Creates the ScannerState.
		explicit ScannerState(const BattleAction* const action);
		/// dTor.
		~ScannerState();

		/// Handler for right-clicking anything.
		void handle(Action* action);

		/// Handles timers.
		void think();
};

}

#endif
