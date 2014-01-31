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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_RESEARCHINFOSTATE
#define OPENXCOM_RESEARCHINFOSTATE

#include "../Engine/State.h"


namespace OpenXcom
{

class ArrowButton;
class Base;
class InteractiveSurface;
class ResearchProject;
class RuleResearch;
class Text;
class TextButton;
class Timer;
class Window;


/**
 * Window which allows changing of the number of assigned scientists to a project.
 */
class ResearchInfoState
	:
		public State
{

private:
	int _changeValueByMouseWheel;

	ArrowButton
		* _btnMore,
		* _btnLess;
	Base* _base;
	InteractiveSurface* _surface;
	ResearchProject* _project;
	RuleResearch* _rule;
	Text
		* _txtAllocatedScientist,
		* _txtAvailableScientist,
		* _txtAvailableSpace,
		* _txtLess,
		* _txtMore,
		* _txtTitle;
	TextButton
		* _btnOk;
	TextButton
		* _btnCancel;
	Timer
		* _timerLess,
		* _timerMore;
	Window* _window;

	///
	void setAssignedScientist();
	///
	void buildUi();


	public:
		/// Creates a ResearchProject state.
		ResearchInfoState(
				Game* game,
				Base* base,
				RuleResearch* rule);
		/// Creates a ResearchProject state.
		ResearchInfoState(
				Game* game,
				Base* base,
				ResearchProject* project);
		/// kL_begin: Cleans up the ResearchInfo state.
		~ResearchInfoState(); // not implemented yet.
		// kL_end.

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
		/// Handler for clicking the Cancel button.
		void btnCancelClick(Action* action);

		/// Function called every time the _timerMore timer is triggered.
		void more();
		/// Add given number of scientists to the project if possible
		void moreByValue(int change);
		/// Function called every time the _timerLess timer is triggered.
		void less();
		/// Remove the given number of scientists from the project if possible
		void lessByValue(int change);

		/// Handler for using the mouse wheel.
		void handleWheel(Action* action);
		/// Handler for pressing the More button.
		void morePress(Action* action);
		/// Handler for releasing the More button.
		void moreRelease(Action* action);
		/// Handler for clicking the More button.
		void moreClick(Action* action);
		/// Handler for pressing the Less button.
		void lessPress(Action* action);
		/// Handler for releasing the Less button.
		void lessRelease(Action* action);
		/// Handler for clicking the Less button.
		void lessClick(Action* action);

		/// Runs state functionality every cycle(used to update the timer).
		void think();
};

}

#endif
