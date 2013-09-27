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

#include "UnitTurnBState.h"
#include "TileEngine.h"
#include "BattlescapeState.h"
#include "Map.h"
#include "../Engine/Game.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Resource/ResourcePack.h"
#include "../Engine/Sound.h"
#include "../Engine/RNG.h"
#include "../Engine/Options.h"


namespace OpenXcom
{

/**
 * Sets up an UnitTurnBState.
 * @param parent Pointer to the Battlescape.
 * @param action Pointer to an action.
 */
UnitTurnBState::UnitTurnBState(BattlescapeGame* parent, BattleAction action)
	:
	BattleState(parent, action), _unit(0), _turret(false)
{
}

/**
 * Deletes the UnitTurnBState.
 */
UnitTurnBState::~UnitTurnBState()
{
}

/**
 * Initializes the state.
 */
void UnitTurnBState::init()
{
	_unit = _action.actor;
	_action.TU = 0;

	if (_unit->getFaction() == FACTION_PLAYER)
		_parent->setStateInterval(Options::getInt("battleXcomSpeed"));
	else
		_parent->setStateInterval(Options::getInt("battleAlienSpeed"));

	// if the unit has a turret and we are turning during targeting, then only the turret turns
	_turret = (_unit->getTurretType() != -1 && _action.targeting) || _action.strafe;

	_unit->lookAt(_action.target, _turret);

	if (_unit->getStatus() != STATUS_TURNING)
	{
		if (_action.type == BA_NONE)
		{
			int door = _parent->getTileEngine()->unitOpensDoor(_unit, true); // try to open a door
			if (door == 0)
			{
				_parent->getResourcePack()->getSound("BATTLE.CAT", 3)->play(); // normal door
			}
			else if (door == 1)
			{
				_parent->getResourcePack()->getSound("BATTLE.CAT", RNG::generate(20, 21))->play(); // ufo door
			}
			else if (door == 4)
			{
				_action.result = "STR_NOT_ENOUGH_TIME_UNITS";
			}
		}

		_parent->popState();
	}
}

/**
 * Runs state functionality every cycle.
 */
void UnitTurnBState::think()
{
	const int tu = _unit->getFaction() == _parent->getSave()->getSide() ? 1 : 0; // one turn is 1 tu unless during reaction fire.

	if (_unit->getFaction() == _parent->getSave()->getSide()
		&& _parent->getPanicHandled()
		&& _parent->checkReservedTU(_unit, tu) == false)
	{
		_unit->abortTurn();
		_parent->popState();

		return;
	}

	if (_unit->spendTimeUnits(tu))
	{
//kL		size_t unitSpotted = _unit->getUnitsSpottedThisTurn().size();
		size_t unitsSpotted = _unit->getUnitsSpottedThisTurn().size();
		_unit->turn(_turret);
		_parent->getTileEngine()->calculateFOV(_unit);
		_unit->setCache(0);
		_parent->getMap()->cacheUnit(_unit);

		// kL_begin:
		bool notMC = _unit->getFaction() == _parent->getSave()->getSide();
		bool anotherone = false;

		// CRASH_start:
		/* SavedBattleGame *bunit;
		for (std::vector<BattleUnit*>::iterator b = bunit->getUnits()->begin(); b != bunit->getUnits()->end(); ++b)
		{
//			if ((*b)->getFaction() == FACTION_PLAYER
//				&& (*b)->getOriginalFaction() == FACTION_PLAYER
			if (notMC
				&& (*b)->getUnitsSpottedThisTurn().size() > unitsSpotted)
			{
				anotherone = true;
			}
		} */ // CRASH_end.
		// kL_end.

//kL		if (_unit->getFaction() == _parent->getSave()->getSide()
		if (notMC														// kL
			&& _parent->getPanicHandled()
			&& _action.type == BA_NONE
//			&& (_action.type == BA_NONE || BA_TURN || BA_WALK)			// kL
//kL			&& _unit->getUnitsSpottedThisTurn().size() > unitSpotted)
			&& _unit->getUnitsSpottedThisTurn().size() > unitsSpotted	// kL, is this screwing up? (see also, UnitWalkBState.cpp)
																		// ie. abortTurn() gets called even if enemy unit is *already spotted*
																		// bool BattleUnit::addToVisibleUnits(BattleUnit *unit)
																		// and
																		// calculateFOV() both involve adding new visible units...
																		// oh, and by the way, a similar routine should go into standing up from kneeling position.
																		// because that *won't* abortTurn if standing spots a new unit (soldier continues to move
																		// if previously told to) .. something in Denmark ..
																		//
																		// ah. This does not abort if the *same* xCom soldier has already seen the new alien, and
																		// 'rediscovers' it. But if a new xCom soldier uncovers an already seen alien, it aborts.
																		// Does this mean that a vector of _unitsSpottedThisTurn is being created per soldier???
																		// then, how would I cycle through *all* xCom to prevent this abortion?
			&& !anotherone)												// kL
		{
			_unit->abortTurn();
		}

		if (_unit->getStatus() == STATUS_STANDING)
		{
			_parent->popState();
		}
	}
	else if (_parent->getPanicHandled())
	{
		_action.result = "STR_NOT_ENOUGH_TIME_UNITS";

		_unit->abortTurn();
		_parent->popState();
	}
}

/**
 * Unit turning cannot be cancelled.
 */
void UnitTurnBState::cancel()
{
}

}
