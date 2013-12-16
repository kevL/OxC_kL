/*
 * Copyright 2010-2013 OpenXcom Developers.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_UNITWALKBSTATE_H
#define OPENXCOM_UNITWALKBSTATE_H

#include <climits>

#include "BattlescapeGame.h"
#include "BattleState.h"
#include "Position.h"


namespace OpenXcom
{

class BattleUnit;
class Pathfinding;
class TileEngine;


/**
 * State for walking units.
 */
class UnitWalkBState
	:
		public BattleState
{

private:
	bool
		_falling,
		_newUnitSpotted,
		_newVis, // kL
		_onScreen,
		_turnBeforeFirstStep,
		_tileSwitchDone; // kL
	int
		_preMovementCost;

	BattleUnit
		* _unit;
	Pathfinding
		* _pf;
	Position
		_target;
	TileEngine
		* _terrain;

	std::size_t
		_unitsSpotted;

	/// Handles some calculations when the walking is finished.
	void setNormalWalkSpeed();

	/// Handles some calculations when the path is finished.
	void postPathProcedures();

	/// This function continues unit movement.
	bool doStatusWalk();
	/// This function ends unit movement.
	bool doStatusStand_end();
	/// This function begins unit movement.
	bool doStatusStand();
	/// This function turns unit during movement.
	void doStatusTurn();

	/// Handles the stepping sounds.
	void playMovementSound();


	public:
		/// Creates a new UnitWalkBState class.
		UnitWalkBState(
				BattlescapeGame* parent,
				BattleAction _action);
		/// Cleans up the UnitWalkBState.
		~UnitWalkBState();

		/// Sets the target to walk to.
//		void setTarget(Position target);
		/// Initializes the state.
		void init();
		/// Handles a cancels request.
//		void cancel();
		/// Runs state functionality every cycle.
		void think();
};

}

#endif
