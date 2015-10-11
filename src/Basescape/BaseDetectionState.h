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

#ifndef OPENXCOM_BASEDETECTIONSTATE_H
#define OPENXCOM_BASEDETECTIONSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class Text;
class TextButton;
class Timer;
class Window;


/**
 * BaseDetection window that displays current basic chance of being detected.
 */
class BaseDetectionState
	:
		public State
{

private:
	static const Uint8
		RED		=  32,
		YELLOW	= 144,
		PURPLE	= 246;

	const Base* _base;
	Text // TODO: add base defenses
//		* _txtDifficulty,
//		* _txtDifficultyVal,
		* _txtExposure,
		* _txtExposureVal,
		* _txtFacilities,
		* _txtFacilitiesVal,
		* _txtShields,
		* _txtShieldsVal,
		* _txtSpotted,
//		* _txtTimePeriod,
		* _txtTitle;
	TextButton* _btnOk;
	Timer* _blinkTimer;
	Window* _window;


	public:
		/// Creates the BaseDetection state.
		explicit BaseDetectionState(const Base* const base);
		/// Cleans up the BaseDetection state.
		~BaseDetectionState();

		/// Runs the blink timer.
		void think();
		/// Blinks the message text.
		void blink();

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
};

}

#endif
