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

#ifndef OPENXCOM_UNITINFOSTATE_H
#define OPENXCOM_UNITINFOSTATE_H


#include "../Engine/State.h"

namespace OpenXcom
{

class Bar;
class BattlescapeState;
class BattleUnit;
class SavedBattleGame;
class Surface;
class Text;
class TextButton;


/**
 * Unit Info screen that shows all the info of a specific unit.
 */
class UnitInfoState
	:
		public State
{

private:
	bool
		_fromInventory,
		_mindProbe;

	Bar
		* _barTimeUnits,
		* _barEnergy,
		* _barHealth,
		* _barFatalWounds,
		* _barBravery,
		* _barMorale,
		* _barReactions,
		* _barFiring,
		* _barThrowing,
		* _barStrength,
		* _barPsiStrength,
		* _barPsiSkill,

		* _barFrontArmor,
		* _barLeftArmor,
		* _barRightArmor,
		* _barRearArmor,
		* _barUnderArmor;
	BattlescapeState* _parent;
	BattleUnit* _unit;
	SavedBattleGame *_battleGame;
	Surface* _bg;
	Text
		* _txtName,

		* _txtTimeUnits,
		* _txtEnergy,
		* _txtHealth,
		* _txtFatalWounds,
		* _txtBravery,
		* _txtMorale,
		* _txtReactions,
		* _txtFiring,
		* _txtThrowing,
		* _txtStrength,
		* _txtPsiStrength,
		* _txtPsiSkill,

		* _numTimeUnits,
		* _numEnergy,
		* _numHealth,
		* _numFatalWounds,
		* _numBravery,
		* _numMorale,
		* _numReactions,
		* _numFiring,
		* _numThrowing,
		* _numStrength,
		* _numPsiStrength,
		* _numPsiSkill,

		* _txtFrontArmor,
		* _txtLeftArmor,
		* _txtRightArmor,
		* _txtRearArmor,
		* _txtUnderArmor,

		* _numFrontArmor,
		* _numLeftArmor,
		* _numRightArmor,
		* _numRearArmor,
		* _numUnderArmor;
	TextButton
		* _btnNext,
		* _btnPrev;


	public:
		/// Creates the Unit Info state.
		UnitInfoState(
				Game* game,
				BattleUnit* unit,
				BattlescapeState* parent,
				bool fromInventory,
				bool mindProbe);
		/// Cleans up the Unit Info state.
		~UnitInfoState();

		/// Updates the unit info.
		void init();

		/// Handler for clicking the button.
		void handle(Action* action);
		/// Handler for clicking the Previous button.
		void btnPrevClick(Action* action);
		/// Handler for clicking the Next button.
		void btnNextClick(Action* action);
};

}

#endif
