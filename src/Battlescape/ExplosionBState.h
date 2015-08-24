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

#ifndef OPENXCOM_EXPLOSIONBSTATE_H
#define OPENXCOM_EXPLOSIONBSTATE_H

//#include <string>

#include "BattleState.h"
#include "Position.h"


namespace OpenXcom
{

class BattlescapeGame;
class BattleUnit;
class BattleItem;
class SavedBattleGame;
class Tile;


/**
 * Explosion state not only handles explosions, but also bullet impacts!
 * Refactoring tip : ImpactBState.
 */
class ExplosionBState
	:
		public BattleState
{

private:
	bool
		_areaOfEffect,
//		_cosmetic,
		_forceCamera,
		_hit,
		_lowerWeapon,
		_pistolWhip,
		_hitSuccess;
	int
//		_extend,
		_power;

	BattleItem* _item;
	BattleUnit* _unit;
	SavedBattleGame* _battleSave;
	Tile* _tile;

	Position _center;


	/// Calculates the effects of the explosion.
	void explode();


	public:
		/// Creates a new ExplosionBState class.
		ExplosionBState(
				BattlescapeGame* const parent,
				Position center,
				BattleItem* const item,
				BattleUnit* const unit,
				Tile* const tile = NULL,
				bool lowerWeapon = false,
				bool meleeSuccess = false,
				bool forceCamera = false);
//				bool cosmetic = false);
		/// Cleans up the ExplosionBState.
		~ExplosionBState();

		/// Initializes the state.
		void init();
		/// Handles a cancel request.
		void cancel();
		/// Runs state functionality every cycle.
		void think();
		/// Gets the result of the state.
//		std::string getResult() const;
};

}

#endif
