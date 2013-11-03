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
		BattleState(parent, action),
		_unit(0),
		_turret(false)
{
	//Log(LOG_INFO) << "Create UnitTurnBState";
}

/**
 * Deletes the UnitTurnBState.
 */
UnitTurnBState::~UnitTurnBState()
{
	//Log(LOG_INFO) << "Delete UnitTurnBState";
}

/**
 * Initializes the state.
 */
void UnitTurnBState::init()
{
	_unit = _action.actor;
	_action.TU = 0;

	//Log(LOG_INFO) << "UnitTurnBState::init() unitID = " << _unit->getId() << "_action.strafe = " << _action.strafe;

	if (_unit->getFaction() == FACTION_PLAYER)
		_parent->setStateInterval(Options::getInt("battleXcomSpeed"));
	else
		_parent->setStateInterval(Options::getInt("battleAlienSpeed"));

	// if the unit has a turret and we are turning during targeting, then only the turret turns
//kL	_turret = (_unit->getTurretType() != -1 && _action.targeting) || _action.strafe;
	_turret = _action.strafe					// kL
			|| (_unit->getTurretType() != -1	// kL
				&& _action.targeting);			// kL

	_unit->lookAt(_action.target, _turret);	// kL_note: -> STATUS_TURNING

//	_unit->setCache(0);						// kL, done in UnitTurnBState::think()
//	_parent->getMap()->cacheUnit(_unit);	// kL, done in UnitTurnBState::think()


	if (_unit->getStatus() != STATUS_TURNING) // try to open a door
	{
		if (_action.type == BA_NONE)
		{
			int door = _parent->getTileEngine()->unitOpensDoor(_unit, true);
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
	//Log(LOG_INFO) << "UnitTurnBState::think() unitID = " << _unit->getId();
	bool thisFaction = _unit->getFaction() == _parent->getSave()->getSide();	// kL

//kL	const int tu = _unit->getFaction() == _parent->getSave()->getSide() ? 1 : 0; // one turn is 1 tu unless during reaction fire.
	const int tu = thisFaction? 1:0;	// kL

//kL	if (_unit->getFaction() == _parent->getSave()->getSide()
	if (thisFaction		// kL
		&& _parent->getPanicHandled()
		&& !_action.targeting
		&& !_parent->checkReservedTU(_unit, tu))
	{
		//Log(LOG_INFO) << "UnitTurnBState::think(), abortTurn() popState()";
		_unit->abortTurn();		// -> STATUS_STANDING.
		_parent->popState();

//kL		return;
	}
	else	// kL
	if (_unit->spendTimeUnits(tu))
	{
//kL		size_t unitSpotted = _unit->getUnitsSpottedThisTurn().size();
		size_t unitsSpotted = _unit->getUnitsSpottedThisTurn().size();

		_unit->turn(_turret);	// kL_note: -> STATUS_STANDING
//kL		_parent->getTileEngine()->calculateFOV(_unit);
		bool newVis = _parent->getTileEngine()->calculateFOV(_unit);	// kL

		_unit->setCache(0);
		_parent->getMap()->cacheUnit(_unit);

//kL		if (_unit->getFaction() == _parent->getSave()->getSide()
		if ((newVis
				&& _unit->getFaction() == FACTION_PLAYER)
			|| (thisFaction												// kL
				&& _parent->getPanicHandled()
				&& _action.type == BA_NONE
//kL				&& _unit->getUnitsSpottedThisTurn().size() > unitSpotted)
				&& _unit->getUnitsSpottedThisTurn().size() > unitsSpotted	// kL
				&& _unit->getFaction() != FACTION_PLAYER))					// kL
		{
			if (_unit->getFaction() == FACTION_PLAYER)
			{
				//Log(LOG_INFO) << ". . newVis = TRUE, Abort turn";
			}
			else if (_unit->getFaction() != FACTION_PLAYER)
			{
				//Log(LOG_INFO) << ". . newUnitSpotted = TRUE, Abort turn";
			}

			//Log(LOG_INFO) << "UnitTurnBState::think(), abortTurn()";
			_unit->abortTurn();
		}

		if (_unit->getStatus() == STATUS_STANDING)
		{
			//Log(LOG_INFO) << "UnitTurnBState::think(), popState()";
			_parent->popState();
		}
	}
	else if (_parent->getPanicHandled())
	{
		_action.result = "STR_NOT_ENOUGH_TIME_UNITS";

		//Log(LOG_INFO) << "UnitTurnBState::think(), abortTurn() popState() 2";
		_unit->abortTurn();
		_parent->popState();
	}

	//Log(LOG_INFO) << "UnitTurnBState::think() EXIT";
}

/**
 * Unit turning cannot be cancelled.
 */
void UnitTurnBState::cancel()
{
}

}
