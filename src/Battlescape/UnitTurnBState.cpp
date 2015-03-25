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

#include "UnitTurnBState.h"

#include "BattlescapeState.h"
#include "Map.h"
#include "TileEngine.h"

#include "../Engine/Game.h"
//#include "../Engine/Logger.h"
//#include "../Engine/Options.h"
//#include "../Engine/RNG.h"
#include "../Engine/Sound.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleArmor.h"

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
{}

/**
 * Deletes the UnitTurnBState.
 */
UnitTurnBState::~UnitTurnBState()
{}

/**
 * Initializes the state.
 */
void UnitTurnBState::init()
{
	_unit = _action.actor;
	if (_unit->isOut(true, true))
	{
		_unit->setTurnDirection(0);
		_parent->popState();

		return;
	}

	_action.TU = 0;
	_unit->setStopShot(false);

	if (_unit->getFaction() == FACTION_PLAYER)
		_parent->setStateInterval(Options::battleXcomSpeed);
	else
		_parent->setStateInterval(Options::battleAlienSpeed);

	// if the unit has a turret and we are turning during targeting then only the turret turns
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
			int sound = -1;
			const int door = _parent->getTileEngine()->unitOpensDoor(
																_unit,
																true);
			if (door == 0)
				sound = ResourcePack::DOOR_OPEN;
			else if (door == 1)
				sound = ResourcePack::SLIDING_DOOR_OPEN;
			else if (door == 4)
				_action.result = "STR_NOT_ENOUGH_TIME_UNITS";
			else if (door == 5)
				_action.result = "STR_TUS_RESERVED";

			if (sound != -1)
				_parent->getResourcePack()->getSoundByDepth(
														_parent->getDepth(),
														sound)->play(
																-1,
																_parent->getMap()->getSoundAngle(_unit->getPosition()));
		}

		_unit->setTurnDirection(0);
		_parent->popState();
	}
}

/**
 * Runs state functionality every cycle.
 */
void UnitTurnBState::think()
{
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
		&& _chargeTUs == true)
	{
		tu = 1;
	}

	if (_chargeTUs == true
		&& onSide == true
		&& _parent->getPanicHandled() == true
		&& _action.targeting == false
		&& _parent->checkReservedTU(
								_unit,
								tu) == false)
	{
		_unit->setStatus(STATUS_STANDING);
		_unit->setTurnDirection(0);
		_parent->popState();
	}
	else if (_unit->spendTimeUnits(tu) == true)
	{
		const size_t unitsSpotted = _unit->getUnitsSpottedThisTurn().size();

		_unit->turn(_turret); // -> STATUS_STANDING
		const bool newVis = _parent->getTileEngine()->calculateFOV(_unit);

		_unit->setCache(NULL);
		_parent->getMap()->cacheUnit(_unit);

		if ((newVis == true
				&& playerTurn == true)
			|| (_chargeTUs == true
				&& onSide == true
				&& playerTurn == false
				&& _action.type == BA_NONE
				&& _parent->getPanicHandled() == true
				&& _unit->getUnitsSpottedThisTurn().size() > unitsSpotted))
		{
			_unit->setStatus(STATUS_STANDING);

			// keep this for Faction_Player only, till I figure out the AI better:
			if (playerTurn == true
				&& onSide == true
				&& _action.targeting == true)
			{
				_unit->setStopShot();
			}
		}

		if (_unit->getStatus() == STATUS_STANDING)
		{
			_unit->setTurnDirection(0);
			_parent->popState();
		}
		else
			_parent->getBattlescapeState()->refreshVisUnits();
	}
	else if (_parent->getPanicHandled() == true)
	{
		_action.result = "STR_NOT_ENOUGH_TIME_UNITS";

		_unit->setStatus(STATUS_STANDING);
		_unit->setTurnDirection(0);
		_parent->popState();
	}
}

/**
 * Unit turning cannot be cancelled.
 */
void UnitTurnBState::cancel()
{}

}
