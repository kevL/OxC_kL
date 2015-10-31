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

#ifndef OPENXCOM_TIMER_H
#define OPENXCOM_TIMER_H

//#include <SDL.h>

#include "State.h"
#include "Surface.h"


namespace OpenXcom
{

typedef void (State::*StateHandler)();
typedef void (Surface::*SurfaceHandler)();


/**
 * Timer used to run code in fixed intervals.
 * @note Used for code that should run at the same fixed interval in various
 * machines based on milliseconds instead of CPU cycles.
 */
class Timer
{

private:
	bool _running;
	Uint32
		_interval,
		_startTick;

	bool _debug;
	std::string _stDebugObject;

	StateHandler _state;
	SurfaceHandler _surface;


	public:
		static Uint32 coreInterval;
		static const Uint32
			SCROLL_SLOW = 250,
			SCROLL_FAST =  80;

		/// Creates a stopped timer.
		Timer(Uint32 interval);
		/// Cleans up the timer.
		~Timer();

		/// Starts the timer.
		void start();
		/// Stops the timer.
		void stop();

		/// Gets the current time interval.
		Uint32 getTimerElapsed() const;

		/// Gets if the timer is running.
		bool isRunning() const;

		/// Advances the timer.
		void think(
				State* const state,
				Surface* const surface);

		/// Sets the timer's interval.
		void setInterval(Uint32 interval);

		/// Hooks a state action handler to the timer interval.
		void onTimer(StateHandler handler);
		/// Hooks a surface action handler to the timer interval.
		void onTimer(SurfaceHandler handler);

		/// Debugs.
		void debug(const std::string& stDebug);
};

}

#endif
