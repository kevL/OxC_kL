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

#ifndef OPENXCOM_PROJECTILEFLYBSTATE_H
#define OPENXCOM_PROJECTILEFLYBSTATE_H

#include "BattleState.h"
#include "Position.h"


namespace OpenXcom
{

class BattlescapeGame;
class BattleUnit;
class BattleItem;
class Tile;


/**
 * A projectile state.
 */
class ProjectileFlyBState
	:
		public BattleState
{

private:
	bool
		_initialized,
		_targetFloor;
	int _projectileImpact;

	BattleItem
		* _ammo,
		* _projectileItem;
	BattleUnit* _unit;
	Position
		_origin,
		_originVoxel,
		_targetVoxel;

	/// Tries to create a projectile sprite.
	bool createNewProjectile();


	public:
		/// Creates a new ProjectileFlyB class
		ProjectileFlyBState(
				BattlescapeGame* parent,
				BattleAction action);
		/// Creates a new ProjectileFlyB class
		ProjectileFlyBState(
				BattlescapeGame* parent,
				BattleAction action,
				Position origin);
		/// Cleans up the ProjectileFlyB.
		~ProjectileFlyBState();

		/// Initializes the state.
		void init();
		/// Handles a cancel request.
		void cancel();
		/// Runs state functionality every cycle.
		void think();

		/// Validates the throwing range.
		static bool validThrowRange(
				const BattleAction* const action,
				const Position origin,
				const Tile* const target);
		/// Calculates the maximum throwing range.
		static int getMaxThrowDistance(
				int weight,
				int strength,
				int level);

		/// Set the origin voxel, used for the blaster launcher.
		void setOriginVoxel(Position pos);

		/// Set the boolean flag to angle a blaster bomb towards the floor.
		void targetFloor();

		/// Peforms a melee attack.
		void performMeleeAttack();
};

}

#endif
