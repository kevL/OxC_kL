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

//#include "../Engine/Options.h"
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
 * @param chargeTu	- true if there is TU cost, false for reaction fire and panic (default true)
 */
UnitTurnBState::UnitTurnBState(
		BattlescapeGame* parent,
		BattleAction action,
		bool chargeTu)
	:
		BattleState(
			parent,
			action),
		_chargeTu(chargeTu),
		_unit(action.actor),
		_turret(false)
{}

/**
 * Deletes the UnitTurnBState.
 */
UnitTurnBState::~UnitTurnBState()
{
	_parent->setStateInterval(BattlescapeState::STATE_INTERVAL_STANDARD); // kL
}

/**
 * Initializes the state.
 */
void UnitTurnBState::init()
{
//	if (_unit->isOut(true, true))
	if (_unit->isOut_t(OUT_STAT) == true)
	{
		_unit->setTurnDirection(0);
		_parent->popState();

		return;
	}

	_action.TU = 0;
	_unit->setStopShot(false);

	Uint32 interval;
	if (_unit->getFaction() == FACTION_PLAYER)
		interval = static_cast<Uint32>(Options::battleXcomSpeed);
	else
		interval = static_cast<Uint32>(Options::battleAlienSpeed);
	_parent->setStateInterval(interval);

	// if the unit has a turret and it's turning during targeting then only the turret turns
	_turret = _unit->getTurretType() != -1
		   && (_action.strafe == true || _action.targeting == true);

	if (_unit->getPosition().x != _action.target.x
		|| _unit->getPosition().y != _action.target.y)
	{
		_unit->lookAt( // -> STATUS_TURNING
					_action.target,
					_turret);
	}


	if (_chargeTu == true
		&& _unit->getUnitStatus() != STATUS_TURNING) // try to open a door
	{
		if (_action.type == BA_NONE)
		{
			int soundId;
			switch (_parent->getTileEngine()->unitOpensDoor(_unit, true))
			{
				case 0: soundId = ResourcePack::DOOR_OPEN;
				break;
				case 1: soundId = ResourcePack::SLIDING_DOOR_OPEN;
				break;
				case 4:
					soundId = -1;
					_action.result = "STR_NOT_ENOUGH_TIME_UNITS";
				break;
				case 5:
					soundId = -1;
					_action.result = "STR_TUS_RESERVED";
				break;
				default: soundId = -1;
			}

			if (soundId != -1)
				_parent->getResourcePack()->getSound("BATTLE.CAT", soundId)
											->play(-1, _parent->getMap()->getSoundAngle(_unit->getPosition()));
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
	int tu;
	if (_chargeTu == false)					// reaction fire & panic permit free turning
		tu = 0;
	else if (_unit->getTurretType() > -1	// if turreted vehicle
		&& _action.strafe == false			// but not swivelling turret
		&& _action.targeting == false)		// and not taking a shot at something...
	{
		if (_unit->getMoveTypeUnit() == MT_FLY)
			tu = 2;							// hover vehicles cost 2 per facing change
		else
			tu = 3;							// large tracked vehicles cost 3 per facing change
	}
	else
		tu = 1;								// one tu per facing change

	if (_chargeTu == true
		&& _action.targeting == false
		&& _unit->getFaction() != FACTION_PLAYER // <- no Reserve tolerance.
		&& _parent->checkReservedTu(_unit, tu) == false)
	{
		_unit->setUnitStatus(STATUS_STANDING);
		_unit->setTurnDirection(0);
		_parent->popState();
	}
	else if (_unit->spendTimeUnits(tu) == true)
	{
		_unit->turn(_turret); // -> STATUS_STANDING if done
		_unit->clearCache();
		_parent->getMap()->cacheUnit(_unit);

		const size_t preSpots = _unit->getHostileUnitsThisTurn().size();
		const bool vis = _parent->getTileEngine()->calculateFOV(_unit);

		if (_unit->getFaction() == FACTION_PLAYER)
		{
			if (_chargeTu == true && vis == true)
			{
				_unit->setUnitStatus(STATUS_STANDING);

				// keep this for Faction_Player only till I figure out the AI better:
				if (_action.targeting == true)
					_unit->setStopShot();
			}
		}
		else if (_chargeTu == true
			&& _action.type == BA_NONE
			&& _unit->getHostileUnitsThisTurn().size() > preSpots)
		{
			_unit->setUnitStatus(STATUS_STANDING);
		}

		if (_unit->getUnitStatus() == STATUS_STANDING)
		{
			_unit->setTurnDirection(0);
			_parent->popState();
		}
		else if (_chargeTu == true)
			_parent->getBattlescapeState()->updateHostileHotcons();
	}
	else
	{
		_action.result = "STR_NOT_ENOUGH_TIME_UNITS";

		_unit->setUnitStatus(STATUS_STANDING);
		_unit->setTurnDirection(0);
		_parent->popState();
	}
}

/**
 * Unit turning cannot be cancelled.
 */
//void UnitTurnBState::cancel(){}

}
