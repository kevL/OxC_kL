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

#ifndef OPENXCOM_BASEDEFENSESTATE_H
#define OPENXCOM_BASEDEFENSESTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class GeoscapeState;
class Text;
class TextButton;
class TextList;
class Timer;
class Ufo;
class Window;


enum BaseDefenseActionType
{
	BD_NONE,
	BD_FIRE,
	BD_RESOLVE,
	BD_DESTROY,
	BD_END
};


/**
 * Base Defense Screen for when ufos try to attack.
 */
class BaseDefenseState
	:
		public State
{

private:
	static const Uint32
		TI_SLOW		= 973, // Time Intervals
		TI_MEDIUM	= 269,
		TI_FAST		= 76;

	int
		_thinkCycles;
	size_t
		_attacks,
		_defenses,
		_explosionCount,
		_gravShields,
		_passes,
		_row,
		_stLen_destroyed,
		_stLen_initiate,
		_stLen_repulsed;

	std::wstring
		_destroyed,
		_initiate,
		_repulsed;

	BaseDefenseActionType _action;

	Base* _base;
	GeoscapeState* _state;
	Text
		* _txtDestroyed,
		* _txtInit,
		* _txtTitle;
	TextButton* _btnOk;
	TextList* _lstDefenses;
	Timer* _timer;
	Ufo* _ufo;
	Window* _window;


	public:
		/// Creates the Base Defense state.
		BaseDefenseState(
				Base* base,
				Ufo* ufo,
				GeoscapeState* state);
		/// Cleans up the Base Defense state.
		~BaseDefenseState();

		/// Handle the Timer.
		void think();
		/// Does the next step.
		void nextStep();

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
};

}

#endif
