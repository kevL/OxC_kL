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

#include "UnitPanicBState.h"

#include "../Savegame/BattleUnit.h"


namespace OpenXcom
{

/**
 * Sets up an UnitPanicBState.
 * @param parent	- pointer to the BattlescapeGame
 * @param unit		- pointer to a panicking unit
 */
UnitPanicBState::UnitPanicBState(
		BattlescapeGame* parent,
		BattleUnit* unit)
	:
		BattleState(parent),
		_unit(unit)
{}

/**
 * Deletes the UnitPanicBState.
 */
UnitPanicBState::~UnitPanicBState()
{}

/**
 *
 */
/* void UnitPanicBState::init()
{} */

/**
 * Runs state functionality every cycle.
 * @note Ends panicking for '_unit'.
 */
void UnitPanicBState::think()
{
	if (_unit != NULL)
	{
		if (_unit->isOut() == false)
		{
			_unit->setStatus(STATUS_STANDING);
			_unit->moraleChange(10 + RNG::generate(0,10));
		}

		_unit->setTimeUnits(0);
		_unit->setDashing(false);

//		if (_unit->getFaction() == FACTION_PLAYER)
//		{
//			Log(LOG_INFO) << "UnitPanicBState: setPanicking FALSE";
//			_unit->setPanicking(false);
//		}
	}

	_parent->popState();
	_parent->setupCursor();
}

/**
 * Panicking cannot be cancelled.
 */
/* void UnitPanicBState::cancel()
{} */

}
