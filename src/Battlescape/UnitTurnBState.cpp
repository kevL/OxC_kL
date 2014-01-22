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

#include "BattlescapeState.h"
#include "Map.h"
#include "TileEngine.h"

#include "../Engine/Game.h"
#include "../Engine/Options.h"
#include "../Engine/RNG.h"
#include "../Engine/Sound.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"


namespace OpenXcom
{

/**
 * Sets up an UnitTurnBState.
 * @param parent Pointer to the Battlescape.
 * @param action Pointer to an action.
 */
UnitTurnBState::UnitTurnBState(
		BattlescapeGame* parent,
		BattleAction action)
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
	//Log(LOG_INFO) << "UnitTurnBState::init()";

	_unit = _action.actor;
	_action.TU = 0;

	_unit->setStopShot(false); // kL

	//Log(LOG_INFO) << "UnitTurnBState::init() unitID = " << _unit->getId() << "_action.strafe = " << _action.strafe;

	if (_unit->getFaction() == FACTION_PLAYER)
		_parent->setStateInterval(Options::getInt("battleXcomSpeed"));
	else
		_parent->setStateInterval(Options::getInt("battleAlienSpeed"));

	// if the unit has a turret and we are turning during targeting, then only the turret turns
	_turret = _unit->getTurretType() != -1
				&& (_action.strafe
					|| _action.targeting);

	_unit->lookAt(_action.target, _turret);	// -> STATUS_TURNING

//	_unit->setCache(0);						// kL, done in UnitTurnBState::think()
//	_parent->getMap()->cacheUnit(_unit);	// kL, done in UnitTurnBState::think()


	if (_unit->getStatus() != STATUS_TURNING) // try to open a door
	{
		if (_action.type == BA_NONE)
		{
			int door = _parent->getTileEngine()->unitOpensDoor(_unit, true);
			if (door == 0)
			{
				//Log(LOG_INFO) << ". open door PlaySound";
				_parent->getResourcePack()->getSound("BATTLE.CAT", 3)->play(); // normal door
			}
			else if (door == 1)
			{
				//Log(LOG_INFO) << ". open uFo door PlaySound";
				_parent->getResourcePack()->getSound("BATTLE.CAT", RNG::generate(20, 21))->play(); // ufo door
			}
			else if (door == 4)
			{
				_action.result = "STR_NOT_ENOUGH_TIME_UNITS";
			}
			else if (door == 5)
			{
				_action.result = "STR_TUS_RESERVED";
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
	bool thisFaction = _unit->getFaction() == _parent->getSave()->getSide(); // kL

	int turretType = _unit->getTurretType();
	int tu = 1;									// one tu per facing change
	if (!thisFaction) tu = 0;					// reaction fire
//	else if (_unit->getArmor()->getSize() > 1)
	else if (turretType != -1
		&& !_action.strafe)						// only for xCom vehicles. (i think)
	{
		if (turretType < 3)
			tu = 3;								// tracked vehicles cost 3 per facing change
		else
			tu = 2;								// hover vehicles cost 2 per facing change
	}


//kL	if (_unit->getFaction() == _parent->getSave()->getSide()
	if (thisFaction // kL
		&& _parent->getPanicHandled()
		&& !_action.targeting
		&& !_parent->checkReservedTU(_unit, tu))
	{
		//Log(LOG_INFO) << "UnitTurnBState::think(), abortTurn() popState()";
		_unit->abortTurn();		// -> STATUS_STANDING.
		_parent->popState();

//kL		return;
	}
	else // kL
	if (_unit->spendTimeUnits(tu))
	{
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
				&& _unit->getUnitsSpottedThisTurn().size() > unitsSpotted	// kL
				&& _unit->getFaction() != FACTION_PLAYER))					// kL
		{
			//if (_unit->getFaction() == FACTION_PLAYER)
			//{
				//Log(LOG_INFO) << ". . newVis = TRUE, Abort turn";
			//}
			//else if (_unit->getFaction() != FACTION_PLAYER)
			//{
				//Log(LOG_INFO) << ". . newUnitSpotted = TRUE, Abort turn";
			//}

			//Log(LOG_INFO) << "UnitTurnBState::think(), abortTurn()";
			_unit->abortTurn();			// -> STATUS_STANDING.

			// keep this for Faction_Player only, till I figure out the AI better:
			if (_unit->getFaction() == FACTION_PLAYER)	// kL
				_unit->setStopShot(true);				// kL

			// kL_note: Can i pop the state (ProjectileFlyBState) here if we came from
			// BattlescapeGame::primaryAction() and as such STOP a unit from shooting
			// elsewhere, if it was turning to do a/the shot when a newVis unit gets Spotted
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
		_unit->abortTurn();		// -> STATUS_STANDING.
		_parent->popState();
	}

	//Log(LOG_INFO) << "UnitTurnBState::think() EXIT";
}

/**
 * Unit turning cannot be cancelled.
 */
//void UnitTurnBState::cancel()
//{
//}

}
