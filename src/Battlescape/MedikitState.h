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

#ifndef OPENXCOM_MEDIKITSTATE_H
#define OPENXCOM_MEDIKITSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Bar;
class InteractiveSurface;
class MedikitView;
class NumberText;
class Text;

struct BattleAction;


/**
 * The Medikit User Interface. Medikit is an item for healing a soldier.
 */
class MedikitState
	:
		public State
{

private:
	Bar
		* _barEnergy,
		* _barHealth,
		* _barMorale,
		* _barTimeUnits;
	BattleAction* _action;
	InteractiveSurface
		* _btnClose,
		* _btnHeal,
		* _btnPain,
		* _btnStim;
	MedikitView* _mediView;
	NumberText
		* _numEnergy,
		* _numHealth,
		* _numMorale,
		* _numStun,
		* _numTimeUnits,
		* _numTotalHP;
	Surface* _bg;
	Text
		* _txtHeal,
		* _txtPain,
		* _txtPart,
		* _txtStim,
		* _txtUnit,
		* _txtWound;

	/// Handler for the end button.
	void onCloseClick(Action* action);
	/// Handler for the heal button.
	void onHealClick(Action* action);
	/// Handler for the stimulant button.
	void onStimClick(Action* action);
	/// Handler for the pain killer button.
	void onPainClick(Action* action);

	/// Updates the medikit interface.
	void update();


	public:
		/// Creates the MedikitState.
		MedikitState(BattleAction* action);

		/// Handler for right-clicking anything.
		void handle(Action* action);
};

}

#endif
