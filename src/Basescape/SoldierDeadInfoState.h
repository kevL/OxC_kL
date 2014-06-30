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

#ifndef OPENXCOM_SOLDIERDEADINFOSTATE_H
#define OPENXCOM_SOLDIERDEADINFOSTATE_H

#include <vector>

#include "../Engine/State.h"


namespace OpenXcom
{

class Bar;
class SoldierDead;
class Surface;
class Text;
class TextButton;


/**
 * Soldier Dead Info screen that shows all the info of a specific dead soldier.
 */
class SoldierDeadInfoState
	:
		public State
{

private:
	size_t _soldierId;

	std::vector<SoldierDead*>* _list;

	SoldierDead* _soldier;

	Bar
		* _barTimeUnits,
		* _barStamina,
		* _barHealth,
		* _barBravery,
		* _barReactions,
		* _barFiring,
		* _barThrowing,
		* _barMelee,
		* _barStrength,
		* _barPsiStrength,
		* _barPsiSkill;
	Surface
		* _bg,
		* _rank;
	Text
		* _txtDate,
		* _txtDeath,
		* _txtKills,
		* _txtMissions,
		* _txtRank,
		* _txtSoldier,

		* _txtTimeUnits,
		* _txtStamina,
		* _txtHealth,
		* _txtBravery,
		* _txtReactions,
		* _txtFiring,
		* _txtThrowing,
		* _txtMelee,
		* _txtStrength,
		* _txtPsiStrength,
		* _txtPsiSkill,

		* _numTimeUnits,
		* _numStamina,
		* _numHealth,
		* _numBravery,
		* _numReactions,
		* _numFiring,
		* _numThrowing,
		* _numMelee,
		* _numStrength,
		* _numPsiStrength,
		* _numPsiSkill;
	TextButton
		* _btnNext,
		* _btnOk,
		* _btnPrev,
		* _btnDiary;


	public:
		/// Creates the Soldier Dead Info state.
		SoldierDeadInfoState(size_t soldierId);
		/// Cleans up the Soldier Dead Info state.
		~SoldierDeadInfoState();

		/// Updates the dead soldier info.
		void init();

		/// Set the soldier Id.
		void setSoldierId(size_t soldier);

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
		/// Handler for clicking the Previous button.
		void btnPrevClick(Action* action);
		/// Handler for clicking the Next button.
		void btnNextClick(Action* action);

		/// Handler for clicking the Diary button.
		void btnDiaryClick(Action* action);
};

}

#endif
