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
class TextList;
class Window;


/**
 * BaseDetection window that displays current basic chance of being detected.
 */
class BaseDetectionState
	:
		public State
{

private:
	Base* _base;
	Text // TODO: add base defenses
		* _txtDifficulty,
		* _txtDifficultyVal,
		* _txtExposure,
		* _txtExposureVal,
		* _txtFacilities,
		* _txtFacilitiesVal,
		* _txtShields,
		* _txtShieldsVal,
		* _txtTimePeriod,
		* _txtTitle;
	TextButton* _btnOk;
	TextList* _lstDetection;
	Window* _window;


	public:
		/// Creates the BaseDetection state.
		BaseDetectionState(Base* base);
		/// Cleans up the BaseDetection state.
		~BaseDetectionState();

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
};

}

#endif
