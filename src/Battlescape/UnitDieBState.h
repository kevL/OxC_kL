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

#ifndef OPENXCOM_UNITDIEBSTATE_H
#define OPENXCOM_UNITDIEBSTATE_H

#include "BattleState.h"

#include "../Ruleset/RuleItem.h" // ItemDamageType


namespace OpenXcom
{

class BattlescapeGame;
class BattleUnit;


/**
 * State for dying units.
 */
class UnitDieBState
	:
		public BattleState
{

private:
	bool
		_noSound,
		_doneScream;
	int _extraTicks;

	BattleUnit* _unit;
	ItemDamageType _damageType;

	/// Converts a unit to a corpse.
	void convertToCorpse();
	/// Plays the death sound.
	void playDeathSound();


	public:
		/// Creates a new UnitDieBState class
		UnitDieBState(
				BattlescapeGame* parent,
				BattleUnit* unit,
				ItemDamageType damageType,
				bool noSound = false);
		/// Cleans up the UnitDieBState.
		~UnitDieBState();

		/// Initializes the state.
		void init();
		/// Handles a cancels request.
		void cancel();
		/// Runs state functionality every cycle.
		void think();
};

}

#endif
