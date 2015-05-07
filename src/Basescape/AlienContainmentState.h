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

#ifndef OPENXCOM_ALIENCONTAINMENTSTATE_H
#define OPENXCOM_ALIENCONTAINMENTSTATE_H

//#include <string>
//#include <vector>

#include "../Engine/State.h"

#include "../Menu/OptionsBaseState.h"


namespace OpenXcom
{

class Base;
class Text;
class TextButton;
class TextList;
class Timer;
class Window;


/**
 * ManageAlienContainment screen that lets the player manage
 * alien numbers in a particular base.
 */
class AlienContainmentState
	:
		public State
{

private:
	bool _overCrowded;
	int _fishFood;
	size_t _sel;

	OptionsOrigin _origin;

	Base* _base;
	Text
		* _txtAvailable,
		* _txtBaseLabel,
		* _txtDeadAliens,
		* _txtItem,
		* _txtLiveAliens,
		* _txtTitle,
		* _txtUsed;
	TextButton
		* _btnCancel,
		* _btnOk;
	TextList* _lstAliens;
	Timer
		* _timerDec,
		* _timerInc;
	Window* _window;

	std::vector<int> _qtys;
	std::vector<std::string> _aliens;

	/// Gets selected quantity.
	int getQuantity();
	/// Updates the quantity-strings of the selected alien.
	void updateStrings();


	public:
		/// Creates the ManageAlienContainment state.
		AlienContainmentState(
				Base* base,
				OptionsOrigin origin);
		/// Cleans up the ManageAlienContainment state.
		~AlienContainmentState();

		/// Runs the timers.
		void think();

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
		/// Handler for clicking the Cancel button.
		void btnCancelClick(Action* action);
		/// Handler for pressing an Increase arrow in the list.
		void lstItemsLeftArrowPress(Action* action);
		/// Handler for releasing an Increase arrow in the list.
		void lstItemsLeftArrowRelease(Action* action);
		/// Handler for clicking an Increase arrow in the list.
		void lstItemsLeftArrowClick(Action* action);
		/// Handler for pressing a Decrease arrow in the list.
		void lstItemsRightArrowPress(Action* action);
		/// Handler for releasing a Decrease arrow in the list.
		void lstItemsRightArrowRelease(Action* action);
		/// Handler for clicking a Decrease arrow in the list.
		void lstItemsRightArrowClick(Action* action);
		/// Handler for pressing-down a mouse-button in the list.
//		void lstItemsMousePress(Action* action);

		/// Increases the quantity of an alien by one.
		void increase();
		/// Increases the quantity of an alien by the given value.
		void increaseByValue(int change);
		/// Decreases the quantity of an alien by one.
		void decrease();
		/// Decreases the quantity of an alien by the given value.
		void decreaseByValue(int change);
};

}

#endif
