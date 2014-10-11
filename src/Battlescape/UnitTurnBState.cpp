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

#include "UnitTurnBState.h"

#include "BattlescapeState.h"
#include "Map.h"
#include "TileEngine.h"

#include "../Engine/Game.h"
#include "../Engine/Logger.h"
#include "../Engine/Options.h"
#include "../Engine/RNG.h"
#include "../Engine/Sound.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/Armor.h"

#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"


namespace OpenXcom
{

/**
 * Sets up an UnitTurnBState.
 * @param parent	- pointer to BattlescapeGame
 * @param action	- the current BattleAction
 * @param chargeTUs	- true if there is TU cost, false for reaction fire
 */
UnitTurnBState::UnitTurnBState(
		BattlescapeGame* parent,
		BattleAction action,
		bool chargeTUs)
	:
		BattleState(
			parent,
			action),
		_chargeTUs(chargeTUs),
		_unit(NULL),
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
	//Log(LOG_INFO) << "UnitTurnBState::init() unitID = " << _action.actor->getId() << " strafe = " << _action.strafe;
	_unit = _action.actor;
	if (_unit->isOut(true, true))
	{
		_parent->popState();
		return;
	}

	_action.TU = 0;
	_unit->setStopShot(false);

	if (_unit->getFaction() == FACTION_PLAYER)
		_parent->setStateInterval(Options::battleXcomSpeed);
	else
		_parent->setStateInterval(Options::battleAlienSpeed);

	// if the unit has a turret and we are turning during targeting, then only the turret turns
	_turret = _unit->getTurretType() != -1
			&& (_action.strafe
				|| _action.targeting);

	if (_unit->getPosition().x != _action.target.x
		|| _unit->getPosition().y != _action.target.y)
	{
		_unit->lookAt( // -> STATUS_TURNING
					_action.target,
					_turret);
	}


	if (_chargeTUs
		&& _unit->getStatus() != STATUS_TURNING) // try to open a door
	{
		if (_action.type == BA_NONE)
		{
			int
				sound = -1,
				door = _parent->getTileEngine()->unitOpensDoor(
															_unit,
															true);
			if (door == 0)
			{
				//Log(LOG_INFO) << ". open door PlaySound";
				sound = ResourcePack::DOOR_OPEN;
			}
			else if (door == 1)
			{
				//Log(LOG_INFO) << ". open uFo door PlaySound";
				sound = ResourcePack::SLIDING_DOOR_OPEN;
			}
			else if (door == 4)
				_action.result = "STR_NOT_ENOUGH_TIME_UNITS";
			else if (door == 5)
				_action.result = "STR_TUS_RESERVED";

			if (sound != -1)
				_parent->getResourcePack()->getSoundByDepth(
														_parent->getDepth(),
														sound)
													->play(
														-1,
														_parent->getMap()->getSoundAngle(_unit->getPosition()));
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
	const bool
		playerTurn = _unit->getFaction() == FACTION_PLAYER,
		onSide = _unit->getFaction() == _parent->getSave()->getSide();

	int tu = 1;								// one tu per facing change

	if (onSide == false)					// reaction fire permits free turning
		tu = 0;
	else if (_unit->getTurretType() > -1	// if turreted vehicle
		&& _action.strafe == false			// but not swivelling turret
		&& _action.targeting == false)		// or not taking a shot at something...
	{
		if (_unit->getMovementType() == MT_FLY)
			tu = 2; // hover vehicles cost 2 per facing change
		else
			tu = 3; // large tracked vehicles cost 3 per facing change
	}

	if (tu == 0
		&& _chargeTUs)
	{
		tu = 1;
	}

	if (_chargeTUs
		&& onSide
		&& _parent->getPanicHandled()
		&& _action.targeting == false
		&& _parent->checkReservedTU(
									_unit,
									tu)
								== false)
	{
		//Log(LOG_INFO) << "UnitTurnBState::think(), abortTurn() popState()";
		_unit->setStatus(STATUS_STANDING);
		_parent->popState();
	}
	else if (_unit->spendTimeUnits(tu))
	{
		size_t unitsSpotted = _unit->getUnitsSpottedThisTurn().size();

		_unit->turn(_turret); // -> STATUS_STANDING
		bool newVis = _parent->getTileEngine()->calculateFOV(_unit);

		_unit->setCache(NULL);
		_parent->getMap()->cacheUnit(_unit);

		if ((newVis
				&& playerTurn)
			|| (_chargeTUs
				&& onSide
				&& playerTurn == false
				&& _action.type == BA_NONE
				&& _parent->getPanicHandled()
				&& _unit->getUnitsSpottedThisTurn().size() > unitsSpotted))
		{
			//if (_unit->getFaction() == FACTION_PLAYER) Log(LOG_INFO) << ". . newVis = TRUE, Abort turn";
			//else if (_unit->getFaction() != FACTION_PLAYER) Log(LOG_INFO) << ". . newUnitSpotted = TRUE, Abort turn";

			//Log(LOG_INFO) << "UnitTurnBState::think(), abortTurn()";
			_unit->setStatus(STATUS_STANDING);

			// keep this for Faction_Player only, till I figure out the AI better:
			if (playerTurn
				&& onSide
				&& _action.targeting)
			{
				//Log(LOG_INFO) << "UnitTurnBState::think(), setStopShot ID = " << _unit->getId();
				_unit->setStopShot(true);
			}
			// kL_note: Can i pop the state (ProjectileFlyBState) here if we came from
			// BattlescapeGame::primaryAction() and as such STOP a unit from shooting
			// elsewhere, if it was turning to do the shot when a newVis unit gets Spotted
		}

		if (_unit->getStatus() == STATUS_STANDING)
		{
			//Log(LOG_INFO) << "UnitTurnBState::think(), popState()";
			_parent->popState();
		}
		else
			_parent->getBattlescapeState()->refreshVisUnits();
	}
	else if (_parent->getPanicHandled())
	{
		_action.result = "STR_NOT_ENOUGH_TIME_UNITS";

		//Log(LOG_INFO) << "UnitTurnBState::think(), abortTurn() popState() 2";
		_unit->setStatus(STATUS_STANDING);
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
