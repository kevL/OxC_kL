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

#ifndef OPENXCOM_SOLDIERINFOSTATE_H
#define OPENXCOM_SOLDIERINFOSTATE_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Bar;
class Base;
class Surface;
class Text;
class TextButton;
class TextEdit;


/**
 * Soldier Info screen that shows all the
 * info of a specific soldier.
 */
class SoldierInfoState
	:
		public State
{
private:
	size_t _soldier;

	Bar
		* _barTimeUnits,
		* _barStamina,
		* _barHealth,
		* _barBravery,
		* _barReactions,
		* _barFiring,
		* _barThrowing,
		* _barStrength,
		* _barPsiStrength,
		* _barPsiSkill;
	Base* _base;
	Surface
		* _bg,
		* _rank;
	Text
		* _txtArmor, // kL
		* _txtCraft,
		* _txtKills,
		* _txtMissions,
		* _txtPsionic,
		* _txtRank,
		* _txtRecovery,

		* _txtTimeUnits,
		* _txtStamina,
		* _txtHealth,
		* _txtBravery,
		* _txtReactions,
		* _txtFiring,
		* _txtThrowing,
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
		* _numStrength,
		* _numPsiStrength,
		* _numPsiSkill;
	TextButton
		* _btnArmor,
		* _btnNext,
		* _btnOk,
		* _btnPrev,
		* _btnAutoStat, // kL
		* _btnSack;
	TextEdit* _edtSoldier;

	/// kL. Automatically renames a soldier according to its statistics.
	void btnAutoStat(Action* action);


	public:
		/// Creates the Soldier Info state.
		SoldierInfoState(
				Game* game,
				Base* base,
				size_t soldier);
		/// Cleans up the Soldier Info state.
		~SoldierInfoState();

		/// Updates the soldier info.
		void init();

		/// Handler for pressing a key on the Name edit.
		void edtSoldierKeyPress(Action* action);
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
};

}

#endif
