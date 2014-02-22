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
	Log(LOG_INFO) << "Create UnitTurnBState";
}

/**
 * Deletes the UnitTurnBState.
 */
UnitTurnBState::~UnitTurnBState()
{
	//Log(LOG_INFO) << "Delete UnitTurnBState";
//	_unit->setShowVisUnits(true); // kL
//	_parent->getBattlescapeState()->toggleVisUnits(true); // kL
}

/**
 * Initializes the state.
 */
void UnitTurnBState::init()
{
	Log(LOG_INFO) << "UnitTurnBState::init() unitID = "
			<< _action.actor->getId() << " strafe = " << _action.strafe;
	_unit = _action.actor;
	_action.TU = 0;

	_unit->setStopShot(false); // kL

	if (_unit->getFaction() == FACTION_PLAYER)
		_parent->setStateInterval(Options::getInt("battleXcomSpeed"));
	else
		_parent->setStateInterval(Options::getInt("battleAlienSpeed"));

	// if the unit has a turret and we are turning during targeting, then only the turret turns
	_turret = _unit->getTurretType() != -1
			&& (_action.strafe
				|| _action.targeting);

	_unit->lookAt( // -> STATUS_TURNING
				_action.target,
				_turret);


	if (_unit->getStatus() != STATUS_TURNING) // try to open a door
	{
		if (_action.type == BA_NONE)
		{
			int door = _parent->getTileEngine()->unitOpensDoor(
															_unit,
															true);
			if (door == 0)
				//Log(LOG_INFO) << ". open door PlaySound";
				_parent->getResourcePack()->getSound( // normal door
													"BATTLE.CAT",
													3)
												->play();
			else if (door == 1)
				//Log(LOG_INFO) << ". open uFo door PlaySound";
				_parent->getResourcePack()->getSound( // ufo door
													"BATTLE.CAT",
													RNG::generate(20, 21))
												->play();
			else if (door == 4)
				_action.result = "STR_NOT_ENOUGH_TIME_UNITS";
			else if (door == 5)
				_action.result = "STR_TUS_RESERVED";
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
	bool
		factPlayer	= _unit->getFaction() == FACTION_PLAYER,
		factSide	= _unit->getFaction() == _parent->getSave()->getSide();
	int
		tu = 1,					// one tu per facing change
		turretType = _unit->getTurretType();

	if (!factSide)				// reaction fire permits free turning
		tu = 0;
	else if (turretType != -1	// if xCom tank
		&& !_action.strafe		// but not swivelling turret
		&& !_action.targeting)	// or not taking a shot at something...
	{
		if (turretType < 3)		// tracked vehicles cost 3 per facing change
			tu = 3;
		else					// hover vehicles cost 2 per facing change
			tu = 2;
	}


	if (factSide
		&& _parent->getPanicHandled()
		&& !_action.targeting
		&& !_parent->checkReservedTU(
									_unit,
									tu))
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

		_unit->setCache(0);
		_parent->getMap()->cacheUnit(_unit);

		if ((newVis
				&& factPlayer)
			|| (factSide
				&& !factPlayer
				&& _action.type == BA_NONE
				&& _parent->getPanicHandled()
				&& _unit->getUnitsSpottedThisTurn().size() > unitsSpotted))
		{
			//if (_unit->getFaction() == FACTION_PLAYER) Log(LOG_INFO) << ". . newVis = TRUE, Abort turn";
			//else if (_unit->getFaction() != FACTION_PLAYER) Log(LOG_INFO) << ". . newUnitSpotted = TRUE, Abort turn";

			//Log(LOG_INFO) << "UnitTurnBState::think(), abortTurn()";
			_unit->setStatus(STATUS_STANDING);

			// keep this for Faction_Player only, till I figure out the AI better:
			if (factPlayer				// kL
				&& factSide				// kL
				&& _action.targeting)	// kL
			{
				Log(LOG_INFO) << "UnitTurnBState::think(), setStopShot ID = " << _unit->getId();
				_unit->setStopShot(true); // kL
			}
			// kL_note: Can i pop the state (ProjectileFlyBState) here if we came from
			// BattlescapeGame::primaryAction() and as such STOP a unit from shooting
			// elsewhere, if it was turning to do the shot when a newVis unit gets Spotted
		}

		if (_unit->getStatus() == STATUS_STANDING)
			//Log(LOG_INFO) << "UnitTurnBState::think(), popState()";
			_parent->popState();
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
