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
	ArrowButton
		* _btnMore,
		* _btnLess;
	Base* _base;
//	InteractiveSurface* _srfScientists;
	ResearchProject* _project;
	const RuleResearch* _resRule;
	Text
		* _txtAllocatedScientist,
		* _txtAvailableScientist,
		* _txtAvailableSpace,
//		* _txtLess,
//		* _txtMore,
		* _txtTitle;
	TextButton
		* _btnCancel,
		* _btnOk;
	Timer
		* _timerLess,
		* _timerMore;
	Window* _window;

	///
	void buildUi();

	/// Handler for clicking the OK button.
	void btnOkClick(Action* action);
	/// Handler for clicking the Cancel button.
	void btnCancelClick(Action* action);

	///
	void updateInfo();

	/// Handler for using the mouse wheel.
//	void handleWheel(Action* action);

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

	/// Function called every time the _timerMore timer is triggered.
	void moreSci();
	/// Add given number of scientists to the project if possible
	void moreByValue(int change);
	/// Function called every time the _timerLess timer is triggered.
	void lessSci();
	/// Remove the given number of scientists from the project if possible
	void lessByValue(int change);

	/// Gets quantity to change by.
	int getQty() const;

	/// Runs state functionality every cycle(used to update the timer).
	void think();


	public:
		/// Creates a ResearchProject state.
		ResearchInfoState(
				Base* const base,
				const RuleResearch* const resRule);
		/// Creates a ResearchProject state.
		ResearchInfoState(
				Base* const base,
				ResearchProject* const project);
		/// Cleans up the ResearchInfo state.
		~ResearchInfoState();
};

}

#endif
