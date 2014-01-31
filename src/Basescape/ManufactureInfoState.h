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

#ifndef OPENXCOM_MANUFACTUREINFOSTATE_H
#define OPENXCOM_MANUFACTUREINFOSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class ArrowButton;
class Base;
class InteractiveSurface;
class Production;
class RuleManufacture;
class Text;
class TextButton;
class Timer;
class Window;


/**
 * Screen that allows changing of Production settings (assigned engineer, units to build).
*/
class ManufactureInfoState
	:
		public State
{

private:
	int _changeValueByMouseWheel;

	ArrowButton
		* _btnEngineerDown,
		* _btnEngineerUp,
		* _btnUnitDown,
		* _btnUnitUp;
	Base* _base;
	InteractiveSurface
		* _surface1,
		* _surface2;
	Production* _production;
	RuleManufacture* _item;
	Text
		* _txtAllocated,
		* _txtAllocatedEngineer,
		* _txtAvailableEngineer,
		* _txtAvailableSpace,
		* _txtEngineerDown,
		* _txtEngineerUp,
		* _txtTitle,
		* _txtTodo,
		* _txtUnitDown,
		* _txtUnitToProduce,
		* _txtUnitUp;
	TextButton
		* _btnOk,
		* _btnStop;
	Timer
		* _timerLessEngineer,
		* _timerLessUnit,
		* _timerMoreEngineer,
		* _timerMoreUnit;
	Window* _window;

	/// Handler for the Stop button.
	void btnStopClick(Action* action);
	/// Handler for the OK button.
	void btnOkClick(Action* action);

	/// Adds given number of engineers to the project if possible.
	void moreEngineer(int change);
	/// Handler for pressing the more engineer button.
	void moreEngineerPress(Action* action);
	/// Handler for releasing the more engineer button.
	void moreEngineerRelease(Action* action);
	/// Handler for clicking the more engineer button.
	void moreEngineerClick(Action* action);

	/// Adds given number of units to produce to the project if possible.
	void moreUnit(int change);
	/// Handler for pressing the more unit button.
	void moreUnitPress(Action* action);
	/// Handler for releasing the more unit button.
	void moreUnitRelease(Action* action);
	/// Handler for clicking the more unit button.
	void moreUnitClick(Action* action);

	/// Removes the given number of engineers from the project if possible.
	void lessEngineer(int change);
	/// Handler for pressing the less engineer button.
	void lessEngineerPress(Action* action);
	/// Handler for releasing the less engineer button.
	void lessEngineerRelease(Action* action);
	/// Handler for clicking the less engineer button.
	void lessEngineerClick(Action* action);

	/// Removes the given number of units to produce from the project if possible.
	void lessUnit(int change);
	/// Handler for pressing the less unit button.
	void lessUnitPress(Action* action);
	/// Handler for releasing the less unit button.
	void lessUnitRelease(Action* action);
	/// Handler for clicking the less unit button.
	void lessUnitClick(Action* action);

	/// Adds one engineer to the production (if possible).
	void onMoreEngineer();
	/// Removes one engineer from the production (if possible).
	void onLessEngineer();

	/// Handler for using the mouse wheel on the Engineer-part of the screen.
	void handleWheelEngineer(Action* action);

	/// Increases count of number of units to make.
	void onMoreUnit();
	/// Decreases count of number of units to make (if possible).
	void onLessUnit();

	/// Handler for using the mouse wheel on the Unit-part of the screen.
	void handleWheelUnit(Action* action);

	/// Updates display of assigned/available engineers and workshop space.
	void setAssignedEngineer();

	/// Runs state functionality every cycle.
	void think();
	/// Builds the User Interface.
	void buildUi();
	/// Helper to exit the State.
	void exitState();


	public:
		/// Creates the State (new production).
		ManufactureInfoState(
				Game* game,
				Base* base,
				RuleManufacture* _item);
		/// Creates the State (modify production).
		ManufactureInfoState(
				Game* game,
				Base* base,
				Production* production);
		/// kL. Cleans up the ManufactureInfo state.
		~ManufactureInfoState();
};

}

#endif
