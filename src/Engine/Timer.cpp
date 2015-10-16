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

#include "Timer.h"

#include "Game.h"


namespace OpenXcom
{

namespace
{

const Uint32 ACCURATE = 4;

Uint32 slowTick()
{
	static Uint32
		tickOld (SDL_GetTicks()),
		tickFalse (tickOld << ACCURATE);

	Uint32 tickNew (SDL_GetTicks() << ACCURATE);

	tickFalse += (tickNew - tickOld) / Timer::coreInterval;
	tickOld = tickNew;

	return (tickFalse >> ACCURATE);
}

}

Uint32 Timer::coreInterval = 1;


/**
 * Initializes a new timer with a set interval.
 * @param interval - time interval in milliseconds
 */
Timer::Timer(Uint32 interval)
	:
		_startTick(0),
		_interval(interval),
		_running(false),
		_state(NULL),
		_surface(NULL),
		_debug(false)
{}

/**
 * dTor.
 */
Timer::~Timer()
{}

/**
 * Starts the timer running and counting time.
 */
void Timer::start()
{
	//if (_debug) Log(LOG_INFO) << "Timer: start() [" << _stDebugObject << "]";
	_startTick = slowTick();
	_running = true;
}

/**
 * Stops the timer from running.
 */
void Timer::stop()
{
	//if (_debug) Log(LOG_INFO) << "Timer: stop() [" << _stDebugObject << "]";
	_startTick = 0;
	_running = false;
}

/**
 * Returns the time passed since the last interval.
 * @note Used only for the FPS counter.
 * @return, time in milliseconds
 */
Uint32 Timer::getTimerElapsed() const
{
	if (_running == true)
		return slowTick() - _startTick; // WARNING: this will give a single erroneous reading @ ~49 days RT.

	return 0;
}

/**
 * Returns if the timer has been started.
 * @return, true if running state
 */
bool Timer::isRunning() const
{
	return _running;
}

/**
 * The timer keeps calculating passed-time while it's running - calling the
 * respective Handler (State and/or Surface) at each set-interval-pass.
 * @param state		- State that the action handler belongs to
 * @param surface	- Surface that the action handler belongs to
 */
void Timer::think(
		State* const state,
		Surface* const surface)
{
	//if (_debug) Log(LOG_INFO) << "Timer: think() [" << _stDebugObject << "]" << " interval = " << _interval;
	if (_running == true)
	{
		Uint32 ticks (slowTick());
		//if (_debug) Log(LOG_INFO) << ". ticks = " << ticks;
		if (ticks >= _startTick + _interval)
		{
			//if (_debug) Log(LOG_INFO) << ". . ticks > " << _startTick + _interval;
			if (state != NULL && _state != NULL)
			{
				//if (_debug) Log(LOG_INFO) << ". . . call StateHandler";
				(state->*_state)();		// call to *StateHandler.
			}

			if (_running == true &&
				surface != NULL && _surface != NULL)
			{
				//if (_debug) Log(LOG_INFO) << ". . . call SurfaceHandler";
				(surface->*_surface)();	// call to *SurfaceHandler.
			}

			//if (_debug) Log(LOG_INFO) << ". . reset _startTick";
			_startTick = ticks;
		}
	}
}

/**
 * Changes the timer's interval to a new value.
 * @param interval - interval in milliseconds
 */
void Timer::setInterval(Uint32 interval)
{
	_interval = interval;
}

/**
 * Sets a state function for the timer to call every interval.
 * @param handler - event handler
 */
void Timer::onTimer(StateHandler handler)
{
	_state = handler;
}

/**
 * Sets a surface function for the timer to call every interval.
 * @param handler - event handler
 */
void Timer::onTimer(SurfaceHandler handler)
{
	_surface = handler;
}

/**
 * Debugs.
 */
void Timer::debug(const std::string& stDebug)
{
	_debug = true;
	_stDebugObject = stDebug;
}

}
