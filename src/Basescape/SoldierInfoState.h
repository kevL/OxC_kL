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

#ifndef OPENXCOM_SOLDIERINFOSTATE_H
#define OPENXCOM_SOLDIERINFOSTATE_H

//#include <vector>

#include "../Engine/State.h"


namespace OpenXcom
{

class Bar;
class Base;
class Soldier;
class Surface;
class Text;
class TextButton;
class TextEdit;


/**
 * Soldier Info screen that shows all the info of a specific soldier.
 */
class SoldierInfoState
	:
		public State
{

private:
	size_t _soldierID;

	std::vector<Soldier*>* _list;

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
	Base* _base;
	Soldier* _soldier;
	Surface
		* _bg,
		* _gender,
		* _rank;
	Text
		* _txtArmor,
		* _txtCraft,
		* _txtKills,
		* _txtMissions,
		* _txtPsionic,
		* _txtRank,
		* _txtRecovery,
		* _txtDay,

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
		* _btnArmor,
		* _btnNext,
		* _btnOk,
		* _btnPrev,
		* _btnAutoStat,
		* _btnSack,
		* _btnDiary;
	TextEdit* _edtSoldier;

	/// kL. Handles autoStat click.
	void btnAutoStat(Action*);
	/// kL. Handles autoStatAll click.
	void btnAutoStatAll(Action*);


	public:
		/// Creates the Soldier Info state.
		SoldierInfoState(
				Base* base,
				size_t soldierID);
		/// Cleans up the Soldier Info state.
		~SoldierInfoState();

		/// Updates the soldier info.
		void init();

		/// Handler for pressing on the Name edit (disables editing).
//kL	void edtSoldierPress(Action* action);
		/// Set the soldier Id.
		void setSoldierID(size_t soldierID);
		/// Handler for changing text on the Name edit.
		void edtSoldierChange(Action* action);

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
		/// Handler for clicking the Previous button.
		void btnPrevClick(Action* action);
		/// Handler for clicking the Next button.
		void btnNextClick(Action* action);

		/// Handler for clicking the Armor button.
		void btnArmorClick(Action* action);
		/// Handler for clicking the Sack button.
		void btnSackClick(Action* action);
		/// Handler for clicking the Diary button.
		void btnDiaryClick(Action* action);
};

}

#endif
