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

#include "UnitWalkBState.h"

#include "BattleAIState.h"
#include "BattlescapeState.h"
#include "Camera.h"
#include "ExplosionBState.h"
#include "Map.h"
#include "Pathfinding.h"
#include "ProjectileFlyBState.h"
#include "UnitFallBState.h"
#include "TileEngine.h"

#include "../Engine/Game.h"
#include "../Engine/Logger.h"
#include "../Engine/Options.h"
#include "../Engine/Sound.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/Armor.h"

#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

/**
 * Sets up an UnitWalkBState.
 */
UnitWalkBState::UnitWalkBState(
		BattlescapeGame* parent,
		BattleAction action)
	:
		BattleState(
			parent,
			action),
		_unit(0),
		_pf(0),
		_terrain(0),
		_falling(false),
		_turnBeforeFirstStep(false),
		_unitsSpotted(0),
		_preMovementCost(0),
		_tileSwitchDone(false), // kL
		_onScreen(false),
		_walkCam(0)
{
	Log(LOG_INFO) << "Create UnitWalkBState";
}

/**
 * Deletes the UnitWalkBState.
 */
UnitWalkBState::~UnitWalkBState()
{
	Log(LOG_INFO) << "Delete UnitWalkBState";
}

/**
 * Initializes the state.
 */
void UnitWalkBState::init()
{
	_unit = _action.actor;
	Log(LOG_INFO) << "\nUnitWalkBState::init() unitID = " << _unit->getId();

	setNormalWalkSpeed();

	_pf = _parent->getPathfinding();
	_terrain = _parent->getTileEngine();
//	_target = _action.target;
	_walkCam = _parent->getMap()->getCamera();

	// kL_note: This is used only for aLiens
	_unitsSpotted = _unit->getUnitsSpottedThisTurn().size();

	Log(LOG_INFO) << ". walking from " << _unit->getPosition() << " to " << _action.target;

	int dir = _pf->getStartDirection();
	//Log(LOG_INFO) << ". StartDirection(init) = " << dir;
	//Log(LOG_INFO) << ". getDirection(init) = " << _unit->getDirection();
	if (!_action.strafe						// not strafing
		&& -1 < dir && dir < 8				// moving but not up or down
		&& dir != _unit->getDirection())	// not facing in direction of movement
	{
		// kL_note: if unit is not facing in the direction that it's about to walk toward...
		// This makes the unit expend tu's if it spots a new alien when turning, but stops before actually walking.
		_turnBeforeFirstStep = true;
	}

	Log(LOG_INFO) << "UnitWalkBState::init() EXIT";
}

/**
 * Runs state functionality every cycle.
 */
void UnitWalkBState::think()
{
	Log(LOG_INFO)	<< "\n***** UnitWalkBState::think() : " << _unit->getId();
					//<< " _walkPhase = " << _unit->getWalkingPhase() << " *****";

	if (_unit->isOut(true, true))
	{
		Log(LOG_INFO) << ". . isOut() abort.";

		_pf->abortPath();
		_parent->popState();

		return;
	}
	//else Log(LOG_INFO) << ". . unit health: " << _unit->getHealth();

	// kL_note: moved here from doStatusWalk()
	// kL_note: Let's try this, maintain camera focus centered on Visible aliens during (un)hidden movement
	if (_unit->getVisible()								// kL
		&& _unit->getFaction() != FACTION_PLAYER		// kL
		&& !_walkCam->isOnScreen(_unit->getPosition())) // kL_TEST!
	{
		_walkCam->centerOnPosition(_unit->getPosition());
	}

	_onScreen = _unit->getVisible()
				&& (_walkCam->isOnScreen(_unit->getPosition())
					|| _walkCam->isOnScreen(_unit->getDestination())); // kL
	Log(LOG_INFO) << ". _onScreen = " << _onScreen;


// _oO **** STATUS WALKING **** Oo_
	if (_unit->getStatus() == STATUS_WALKING
		|| _unit->getStatus() == STATUS_FLYING)
	{
		Log(LOG_INFO) << "STATUS_WALKING or FLYING : " << _unit->getId();

		if (!doStatusWalk()) return;


	// _oO **** STATUS STANDING end **** Oo_
		// kL_note: walkPhase reset as the unit completes its transition to the next tile
		if (_unit->getStatus() == STATUS_STANDING)
		{
			Log(LOG_INFO) << "Hey we got to STATUS_STANDING in UnitWalkBState _WALKING or _FLYING !!!" ;
//			_falling = false; // kL

			// kL_begin: if the unit changed level, camera changes level with it.
			if (_walkCam->getViewLevel() != _unit->getPosition().z)
				_walkCam->setViewLevel(_unit->getPosition().z);
			// kL_end.

			if (!doStatusStand_end()) return;
		}
		else if (_onScreen) // still walking....
		{
			Log(LOG_INFO) << ". _onScreen : still walking ...";

			// make sure the unit sprites are up to date
			if (_pf->getStrafeMove())
			{
				Log(LOG_INFO) << ". . strafe";
				// This is where we fake out the strafe movement direction so the unit "moonwalks"
				int dirStrafe = _unit->getDirection();
//				int dirStrafe = _pf->getStartDirection(); // kL
//				int dirStrafe = dir; // kL

				_unit->setDirection(_unit->getFaceDirection());
				_parent->getMap()->cacheUnit(_unit); // draw unit.

				_unit->setDirection(dirStrafe);
//				_unit->setDirection(dir); // kL
			}
			else
			{
				Log(LOG_INFO) << ". . no strafe, cacheUnit()";
				_parent->getMap()->cacheUnit(_unit);
			}
		}
	}


// _oO **** STATUS STANDING **** Oo_
	if (_unit->getStatus() == STATUS_STANDING
		|| _unit->getStatus() == STATUS_PANICKING)
	{
		Log(LOG_INFO) << "STATUS_STANDING or PANICKING : " << _unit->getId();

		if (!doStatusStand()) return;
	}


// _oO **** STATUS TURNING **** Oo_
	if (_unit->getStatus() == STATUS_TURNING) // turning during walking costs no tu
	{
		Log(LOG_INFO) << "STATUS_TURNING : " << _unit->getId();

		doStatusTurn();
	}

	Log(LOG_INFO) << "think() : " << _unit->getId() << " EXIT ";
}

/**
 * Aborts unit walking.
 */
void UnitWalkBState::cancel()
{
	if (_parent->getSave()->getSide() == FACTION_PLAYER
		&& _parent->getPanicHandled())
	{
		_pf->abortPath();
	}
}

/**
 * This begins unit movement.
 * Called from think()...
 * @return bool, False to exit think(), true to continue
 */
bool UnitWalkBState::doStatusStand()
{
	//Log(LOG_INFO) << ". _onScreen = " << _onScreen;
	int dir = _pf->getStartDirection();
	Log(LOG_INFO) << ". StartDirection = " << dir;

	if (_unit->isKneeled()
		&& -1 < dir && dir < 8)	// ie. *not* up or down
	{
		Log(LOG_INFO) << ". kneeled, and path UpDown INVALID";

		if (_parent->kneel(
						_unit,
						false))
		{
			Log(LOG_INFO) << ". . Stand up";

			_unit->setCache(0);
			_parent->getMap()->cacheUnit(_unit);
		}
		else
		{
			_action.result = "STR_NOT_ENOUGH_TIME_UNITS";

			_pf->abortPath();
			_parent->popState();

			return false;
		}
	}

	_tileSwitchDone = false;				// kL
//	_unit->setCache(0);						// kL
//	_parent->getMap()->cacheUnit(_unit);	// kL

	bool newVis = visForUnits();
	if (newVis)
	{
		//if (_parent->getSave()->getTraceSetting()) { Log(LOG_INFO) << "Uh-oh! Company!"; }
		Log(LOG_INFO) << "Uh-oh! STATUS_STANDING or PANICKING Company!";
		//if (_unit->getFaction() == FACTION_PLAYER) Log(LOG_INFO) << ". . _newVis = TRUE, postPathProcedures";
		//else if (_unit->getFaction() != FACTION_PLAYER) Log(LOG_INFO) << ". . _newUnitSpotted = TRUE, postPathProcedures";

		_unit->_hidingForTurn = false; // clearly we're not hidden now

		_unit->setCache(0);	// kL. Calls to cacheUnit() are bogus without setCache(0) first...!
							// although _cacheInvalid might be set elsewhere but i doubt it.
		_parent->getMap()->cacheUnit(_unit);

		postPathProcedures();

		return false;
	}

/*kL	if (_onScreen
		|| _parent->getSave()->getDebugMode())
	{
		setNormalWalkSpeed();
	}
	else
//kL		_parent->setStateInterval(0);
		_parent->setStateInterval(11); // kL
		// kL_note: mute footstep sounds. Trying...
	*/
//	setNormalWalkSpeed(); // kL: Done in init()

//	int dir = _pf->getStartDirection();
//	Log(LOG_INFO) << ". getStartDirection() dir = " << dir;

	if (_falling)
	{
		dir = _pf->DIR_DOWN;
		Log(LOG_INFO) << ". . _falling, dir = " << dir;
	}

	if (dir != -1)
	{
		Log(LOG_INFO) << "enter (dir!=-1) : " << _unit->getId();

		if (_pf->getStrafeMove())
		{
			int dirFace = _unit->getDirection();
			_unit->setFaceDirection(dirFace);

			Log(LOG_INFO) << ". . strafeMove, setFaceDirection() <- " << dirFace;
		}

		Log(LOG_INFO) << ". pos 1";

		// gets tu cost, but also gets the destination position.
		Position destination;
		int tu = _pf->getTUCost(
							_unit->getPosition(),
							dir,
							&destination,
							_unit,
							0,
							false);
		Log(LOG_INFO) << ". tu = " << tu;

		// kL_note: should this include neutrals? (ie != FACTION_PLAYER; see also 32tu inflation...)
		if (_unit->getFaction() == FACTION_HOSTILE
			&& _parent->getSave()->getTile(destination)->getFire() > 0)
		{
			Log(LOG_INFO) << ". . subtract tu inflation for a fireTile";
			// we artificially inflate the TU cost by 32 points in getTUCost under these conditions, so we have to deflate it here.
			tu -= 32;
			Log(LOG_INFO) << ". . subtract tu inflation for a fireTile DONE";
		}

		Log(LOG_INFO) << ". pos 2";

		if (_falling)
			tu = 0;

//kL		int energy = tu;
		// kL_begin: UnitWalkBState::think(), no stamina required to go up/down GravLifts.
		int energy = 0;

//		if (!(_parent->getSave()->getTile(_unit->getPosition())->getMapData(MapData::O_FLOOR)->isGravLift()
//			&& _parent->getSave()->getTile(destination)->getMapData(MapData::O_FLOOR)->isGravLift()))
		if (_parent->getSave()->getTile(_unit->getPosition())->getMapData(MapData::O_FLOOR)
			&& _parent->getSave()->getTile(_unit->getPosition())->getMapData(MapData::O_FLOOR)->isGravLift()
			&& _parent->getSave()->getTile(destination)
			&& _parent->getSave()->getTile(destination)->getMapData(MapData::O_FLOOR)
			&& _parent->getSave()->getTile(destination)->getMapData(MapData::O_FLOOR)->isGravLift())
		{
			Log(LOG_INFO) << ". . NOT using GravLift";
			energy = tu;
		} // kL_end.

		Log(LOG_INFO) << ". pos 3";

		if (_action.run)
		{
//kL			tu *= 0.75;
//kL			energy *= 1.5;
			tu = tu * 3 / 4;			// kL
			energy = energy * 3 / 2;	// kL
		}

//kL		if (dir >= Pathfinding::DIR_UP)
//kL		{
//kL			energy = 0;
//kL		}

		Log(LOG_INFO) << ". pos 4";

		if (tu > _unit->getTimeUnits())
		{
			Log(LOG_INFO) << ". . tu > _unit->getTimeUnits()";

			if (_parent->getPanicHandled()
				&& tu < 255)
			{
				_action.result = "STR_NOT_ENOUGH_TIME_UNITS";
			}

			_pf->abortPath();

			_unit->setCache(0);
			_parent->getMap()->cacheUnit(_unit);

			_parent->popState();

			return false;
		}
		else if (energy / 2 > _unit->getEnergy())
		{
			Log(LOG_INFO) << ". . energy / 2 > _unit->getEnergy()";

			if (_parent->getPanicHandled())
			{
				_action.result = "STR_NOT_ENOUGH_ENERGY";
			}

			_pf->abortPath();

			_unit->setCache(0);
			_parent->getMap()->cacheUnit(_unit);

			_parent->popState();

			return false;
		}
		else if (_parent->getPanicHandled()
			&& _parent->checkReservedTU(_unit, tu) == false)
		{
			Log(LOG_INFO) << ". . checkReservedTU(_unit, tu) == false";

			_pf->abortPath();

			_unit->setCache(0);
			_parent->getMap()->cacheUnit(_unit);

			return false;
		}
		// we are looking in the wrong way, turn first (unless strafing)
		// we are not using the turn state, because turning during walking costs no tu
		else if (dir != _unit->getDirection()
			&& dir < _pf->DIR_UP
			&& !_pf->getStrafeMove())
		{
			Log(LOG_INFO) << ". . dir != _unit->getDirection() -> turn";

			_unit->lookAt(dir);

			_unit->setCache(0);
			_parent->getMap()->cacheUnit(_unit);

			return false;
		}
		else if (dir < _pf->DIR_UP) // now open doors (if any)
		{
			Log(LOG_INFO) << ". . check for doors";

			int door = _terrain->unitOpensDoor(
											_unit,
											false,
											dir);
			if (door == 3)
			{
				Log(LOG_INFO) << ". . . door #3";
				return false; // don't start walking yet, wait for the ufo door to open
			}
			else if (door == 0)
			{
				Log(LOG_INFO) << ". . . door #0";
				_parent->getResourcePack()->getSound("BATTLE.CAT", 3)->play(); // normal door
			}
			else if (door == 1)
			{
				Log(LOG_INFO) << ". . . door #1";
				_parent->getResourcePack()->getSound("BATTLE.CAT", 20)->play(); // ufo door

				return false; // don't start walking yet, wait for the ufo door to open
			}
		}

		Log(LOG_INFO) << ". pos 5";

		int size = _unit->getArmor()->getSize() - 1;
		for (int
				x = size;
				x > -1;
				--x)
		{
			for (int
					y = size;
					y > -1;
					--y)
			{
				Log(LOG_INFO) << ". . check obstacle(unit)";

				BattleUnit* unitInMyWay = _parent->getSave()->getTile(destination + Position(x, y, 0))->getUnit();

				BattleUnit* unitBelowMyWay = 0;
				Tile* belowDest = _parent->getSave()->getTile(destination + Position(x, y, -1));
				if (belowDest)
					unitBelowMyWay = belowDest->getUnit();

				// can't walk into units in this tile, or on top of other units sticking their head into this tile
				if (!_falling
					&& ((unitInMyWay
							&& unitInMyWay != _unit)
						|| (belowDest
							&& unitBelowMyWay
							&& unitBelowMyWay != _unit
							&& unitBelowMyWay->getFloatHeight()
										+ unitBelowMyWay->getHeight()
										- belowDest->getTerrainLevel()
									>= 24 + 4)))
							// 4+ voxels poking into the tile above, we don't kick people in the head here at XCom.
							// kL_note: this appears to be only +2 in Pathfinding....
				{
					Log(LOG_INFO) << ". . . obstacle(unit) -> abortPath()";

					_action.TU = 0;
					_pf->abortPath();

					_unit->setCache(0);
					_parent->getMap()->cacheUnit(_unit);

					_parent->popState();

					return false;
				}
			}
		}

		//Log(LOG_INFO) << ". dequeuePath()";
		dir = _pf->dequeuePath(); // now start moving

		if (_falling)
			dir = _pf->DIR_DOWN;
		Log(LOG_INFO) << ". dequeuePath() dir = " << dir;

		if (dir == _pf->DIR_UP)	// kL
			_walkCam->up();		// kL

		if (_unit->spendTimeUnits(tu)		// These were checked above and don't really need to
			&& _unit->spendEnergy(energy))	// be checked again here. Only subtract required.
		{
			//Log(LOG_INFO) << ". . WalkBState: spend TU & Energy";

			Tile* tileBelow = _parent->getSave()->getTile(_unit->getPosition() + Position(0, 0, -1));

			Log(LOG_INFO) << ". . WalkBState: startWalking()";
			_unit->startWalking(
							dir,
							destination,
							tileBelow,
							_onScreen);

			_turnBeforeFirstStep = false;
		}

		// make sure the unit sprites are up to date
		// kL_note: This could probably go up under spend tu+energy. but.....
		// And since Status_Stand algorithm doesn't actually draw anything
		// REMARK It.
/*kL		if (_onScreen)
		{
			//Log(LOG_INFO) << ". . _onScreen";
			if (_pf->getStrafeMove())
			{
				Log(LOG_INFO) << ". . . (_onScreen) -> _pf->getStrafeMove()";

				// This is where we fake out the strafe movement direction so the unit "moonwalks"
				int dirStrafe = _unit->getDirection();
//				int dirStrafe = dir; // kL
	
				_unit->setDirection(_unit->getFaceDirection());
//				_parent->getMap()->cacheUnit(_unit); // kL ( see far above, re. strafe fake-out moonwalking )

				_unit->setDirection(dirStrafe);
//				_unit->setDirection(dir);

				Log(LOG_INFO) << ". . . end strafeMove()";
			}
//			else // kL
//			{
			Log(LOG_INFO) << ". . (_onScreen) -> cacheUnit()";

//			_unit->setCache(0); // kL
			_parent->getMap()->cacheUnit(_unit);
//			}

			Log(LOG_INFO) << ". . end (_onScreen)";
		} */
		Log(LOG_INFO) << ". EXIT (dir!=-1) : " << _unit->getId();
	}
	else // dir == -1
	{
		Log(LOG_INFO) << ". . postPathProcedures()";

		postPathProcedures();

		return false;
	}

	return true;
}

/**
 * This continues unit movement.
 * Called from think()...
 * @return bool, False to exit think(), true to continue
 */
bool UnitWalkBState::doStatusWalk()
{
	Tile* tBelow = 0;

	if (_parent->getSave()->getTile(_unit->getDestination())->getUnit() == 0  // next tile must be not occupied
		// kL_note: and, if not flying, the position directly below the tile must not be occupied...
		// Had that happen with a sectoid left standing in the air because a cyberdisc was 2 levels below it.
		// btw, these have probably been already checked...
		|| _parent->getSave()->getTile(_unit->getDestination())->getUnit() == _unit)
	{
		Log(LOG_INFO) << ". WalkBState, keepWalking()";
		playMovementSound();

		tBelow = _parent->getSave()->getTile(_unit->getPosition() + Position(0, 0, -1));
		_unit->keepWalking( // advances _walkPhase
						tBelow,
						_onScreen);
	}
	else if (!_falling)
	{
		Log(LOG_INFO) << ". WalkBState, !falling Abort path";
		_unit->lookAt( // turn to blocking unit
					_unit->getDestination(),
					_unit->getTurretType() != -1);

		_pf->abortPath();
	}

	// unit moved from one tile to the other, update the tiles & investigate new flooring
	if (!_tileSwitchDone // kL
		&& _unit->getPosition() != _unit->getLastPosition())
	{
		Log(LOG_INFO) << ". tile switch from _lastpos to _destination";
		// BattleUnit::startWalking() sets _lastpos = _pos, then
		// BattleUnit::keepWalking (_walkPhase == middle) sets _pos = _destination
		_tileSwitchDone = true; // kL

		bool fallCheck = true;

		int size = _unit->getArmor()->getSize() - 1;
		for (int
				x = size;
				x > -1;
				x--)
		{
			for (int
					y = size;
					y > -1;
					y--)
			{
				tBelow = _parent->getSave()->getTile(_unit->getPosition() + Position(x, y, -1));

				if ( //kL _unit->getArmor()->getMovementType() == MT_FLY ||
					!_parent->getSave()->getTile(
											_unit->getPosition() + Position(x, y, 0))
										->hasNoFloor(tBelow))
				{
					Log(LOG_INFO) << ". . . hasFloor ( fallCheck set FALSE )";
					fallCheck = false;
				}

				Log(LOG_INFO) << ". . remove unit from previous tile";
				_parent->getSave()->getTile(
										_unit->getLastPosition() + Position(x, y, 0))
									->setUnit(0);
			}
		} // -> might move to doStatusStand_end()

		for (int
				x = size;
				x > -1;
				x--)
		{
			for (int
					y = size;
					y > -1;
					y--)
			{
				Log(LOG_INFO) << ". . set unit on new tile";
				_parent->getSave()->getTile(
										_unit->getPosition() + Position(x, y, 0))
									->setUnit(
											_unit,
											_parent->getSave()->getTile(
																	_unit->getPosition() + Position(x, y, -1)));
			}
		}

		_falling = fallCheck
					&& _unit->getPosition().z != 0
//kL					&& _unit->getTile()->hasNoFloor(tileBelow) // kL_note: Done above.
					&& _unit->getArmor()->getMovementType() != MT_FLY // -> sorta move to doStatusStand_end()
					&& _unit->getWalkingPhase() == 0; // <- set @ startWalking() and @ end of keepWalking()

		if (_falling)
		{
			Log(LOG_INFO) << ". falling";
			for (int
					x = size;
					x > -1;
					--x)
			{
				for (int
						y = size;
						y > -1;
						--y)
				{
					tBelow = _parent->getSave()->getTile(_unit->getPosition() + Position(x, y, -1));
					if (tBelow) Log(LOG_INFO) << ". . otherTileBelow exists";

					if (tBelow
						&& tBelow->getUnit())
					{
						Log(LOG_INFO) << ". . . another unit already occupies lower tile";

						_falling = false;

						_pf->dequeuePath();
						_parent->getSave()->addFallingUnit(_unit);

						Log(LOG_INFO) << "UnitWalkBState::think(), addFallingUnit() ID " << _unit->getId();
						_parent->statePushFront(new UnitFallBState(_parent));

						return false;
					}
					else Log(LOG_INFO) << ". . otherTileBelow Does NOT contain other unit";
				}
			}
		}

		// kL_note: try moving this up into think().
		// kL_note: Let's try this, maintain camera focus centered on Visible aliens during (un)hidden movement
/*		if (_unit->getVisible()								// kL
			&& _unit->getFaction() != FACTION_PLAYER		// kL
			&& !_walkCam->isOnScreen(_unit->getPosition())) // kL_TEST!
		{
			_walkCam->centerOnPosition(_unit->getPosition());
		}

		// kL_begin: if the unit changed level, camera changes level with it.
		if (_walkCam->getViewLevel() != _unit->getPosition().z)
		{
			int delta_z = _unit->getPosition().z - _walkCam->getViewLevel();
			if (delta_z > 0)
				_walkCam->up();
			else
				_walkCam->down();
		} // kL_end. */
//kL		_walkCam->setViewLevel(_unit->getPosition().z);
	}

	return true;
}

/**
 * This function ends unit movement.
 * Called from think()...
 * @return bool, False to exit think(), true to continue
 */
bool UnitWalkBState::doStatusStand_end()
{
// kL_begin: Try doing the _falling check at the end of each tile's walk-cycle
/*	Tile* tileBelow = _parent->getSave()->getTile(_unit->getPosition() + Position(0, 0, -1));
	bool fallCheck = true;

	int size = _unit->getArmor()->getSize() - 1;
	for (int
			x = size;
			x > -1;
			x--)
	{
		for (int
				y = size;
				y > -1;
				y--)
		{
			Tile* otherTileBelow = _parent->getSave()->getTile(_unit->getPosition() + Position(x, y, -1));

			if (_unit->getArmor()->getMovementType() == MT_FLY
				|| !_parent->getSave()->getTile(
											_unit->getPosition()
												+ Position(x, y, 0))->hasNoFloor(otherTileBelow))
			{
				Log(LOG_INFO) << ". . . WalkBState, hasFloor or is Flying ( fallCheck set FALSE )";
				fallCheck = false;
			}

			Log(LOG_INFO) << ". . WalkBState, remove unit from previous tile";
			_parent->getSave()->getTile(_unit->getLastPosition() + Position(x, y, 0))->setUnit(0);
		}
	}

	_falling = fallCheck
		&& _unit->getPosition().z != 0
		&& _unit->getTile()->hasNoFloor(tileBelow)
		&& _unit->getArmor()->getMovementType() != MT_FLY; */
//kL		&& _unit->getWalkingPhase() == 0; // <- set @ startWalking() and @ end of keepWalking()
// kL_end.
//	_falling = false; <- don't forget to turn it off somewhere!!!


	_tileSwitchDone = false; // kL

	// if the unit burns floortiles, burn floortiles
	if (_unit->getSpecialAbility() == SPECAB_BURNFLOOR)
	{
		_unit->getTile()->ignite(1);

		Position here = _unit->getPosition() * Position(16, 16, 24)
						+ Position(
								8,
								8,
								-(_unit->getTile()->getTerrainLevel()));
		_parent->getTileEngine()->hit(
									here,
									_unit->getStats()->strength,
									DT_IN,
									_unit);
	}

	_terrain->calculateUnitLighting(); // move our personal lighting with us

	if (_unit->getFaction() != FACTION_PLAYER)
		_unit->setVisible(false);


	// This needs to be done *before* the calculateFOV(pos)
	// or else any newVis will be marked Visible before
	// visForUnits() catches that new unit as !Visible.
	bool newVis = visForUnits();

	// This calculates or 'refreshes' the Field of View
	// of all units within maximum distance (20 tiles) of this unit.
	_terrain->calculateFOV(_unit->getPosition());

/*	_newUnitSpotted = !_falling
						&& !_action.desperate
						&& _parent->getPanicHandled()
						&& _unitsSpotted < _unit->getUnitsSpottedThisTurn().size()
						&& _unit->getFaction() != FACTION_PLAYER; */

	if (_parent->checkForProximityGrenades(_unit))
	{
		_parent->popState();

		return false;
	}

	if (newVis)
	{
		if (_unit->getFaction() == FACTION_PLAYER) Log(LOG_INFO) << ". . _newVis = TRUE, Abort path";
		else if (_unit->getFaction() != FACTION_PLAYER) Log(LOG_INFO) << ". . _newUnitSpotted = TRUE, Abort path";

		_pf->abortPath();

		_unit->setCache(0);
		_parent->getMap()->cacheUnit(_unit);

		_parent->popState();

		return false;
	}

	if (!_falling) // check for reaction fire
	{
		Log(LOG_INFO) << ". . WalkBState: NOT falling, checkReactionFire()";

		if (_terrain->checkReactionFire(_unit)) // unit got fired upon - stop walking
		{
			Log(LOG_INFO) << ". . . cacheUnit";

			_unit->setCache(0);
			_parent->getMap()->cacheUnit(_unit);

			_pf->abortPath();
			_parent->popState();

			return false;
		}
		else
		{
			Log(LOG_INFO) << ". . WalkBState: checkReactionFire() FALSE... no caching";
		}
	}
	else // <<-- Looks like we gotta make it fall here!!! (if unit *ends* its total walk sequence on empty air.
			// And, fall *before* spotting new units, else Abort will likely make it float...
	{
		Log(LOG_INFO) << ". . WalkBState: falling";

//		_unit->setCache(0);
//		_parent->getMap()->cacheUnit(_unit);
	}

	return true;
}

/**
 * This function turns unit during movement.
 * Called from think()
 */
void UnitWalkBState::doStatusTurn()
{
	if (_turnBeforeFirstStep) // except before the first step.
		_preMovementCost++;

	_unit->turn();

	if (_onScreen) // make sure the unit sprites are up to date
	{
		Log(LOG_INFO) << ". cacheUnit()";

		_unit->setCache(0);
		_parent->getMap()->cacheUnit(_unit);
	}

	// calculateFOV() is unreliable for setting the _newUnitSpotted bool,
	// as it can be called from various other places in the code, ie:
	// doors opening (& explosions/terrain destruction?), and that messes up the result.
	// kL_note: But let's do it anyway!
	bool newVis = visForUnits(); // kL
	if (newVis) // kL
	{
		if (_turnBeforeFirstStep)
			_unit->spendTimeUnits(_preMovementCost);

		Log(LOG_INFO) << "Egads! STATUS_TURNING reveals new units!!! I must pause!";
		if (_unit->getFaction() == FACTION_PLAYER) Log(LOG_INFO) << ". . _newVis = TRUE, Abort path, popState";
		else if (_unit->getFaction() != FACTION_PLAYER) Log(LOG_INFO) << ". . _newUnitSpotted = TRUE, Abort path, popState";

		_unit->_hidingForTurn = false;

		_pf->abortPath();

		_unit->setCache(0);
		_parent->getMap()->cacheUnit(_unit);

		_parent->popState();
	}
}

/**
 * Handles some calculations when the path is finished.
 */
void UnitWalkBState::postPathProcedures()
{
	Log(LOG_INFO) << "UnitWalkBState::postPathProcedures(), unit = " << _unit->getId();

	_tileSwitchDone = false;	// kL
//	_falling = false;			// kL
	_action.TU = 0;

	if (_unit->getFaction() != FACTION_PLAYER)
	{
		int dir = _action.finalFacing;

		if (_action.finalAction)
			_unit->dontReselect();

		if (_unit->getCharging() != 0)
		{
			dir = _parent->getTileEngine()->getDirectionTo(
														_unit->getPosition(),
														_unit->getCharging()->getPosition());
			// kL_notes (pre-above):
			// put an appropriate facing direction here
			// don't stare at a wall. Get if aggro, face closest xCom op <- might be done somewhere already.
			// Cheat: face closest xCom op based on a percentage (perhaps alien 'value' or rank)
			// cf. void AggroBAIState::setAggroTarget(BattleUnit *unit)
			// and bool TileEngine::calculateFOV(BattleUnit *unit)
			//Log(LOG_INFO) << ". . charging = TRUE";

			if (_parent->getTileEngine()->validMeleeRange(
														_unit,
														_action.actor->getCharging(),
														_unit->getDirection()))
			{
				BattleAction action;
				action.actor = _unit;
				action.target = _unit->getCharging()->getPosition();
				action.weapon = _unit->getMainHandWeapon();
				action.type = BA_HIT;
				action.TU = _unit->getActionTUs(action.type, action.weapon);
				action.targeting = true;

				_unit->setCharging(0);
				_parent->statePushBack(new ProjectileFlyBState(
															_parent,
															action));
			}
		}
		else if (_unit->_hidingForTurn)
			dir = _unit->getDirection() + 4;

		if (dir != -1)
		{
			dir = dir %8;
			_unit->lookAt(dir);

			while (_unit->getStatus() == STATUS_TURNING)
			{
				_unit->turn();
				_parent->getTileEngine()->calculateFOV(_unit);
				// kL_note: might need newVis/newUnitSpotted -> abort
			}

			_unit->setCache(0);
			_parent->getMap()->cacheUnit(_unit);

		}
	}
	else if (!_parent->getPanicHandled())
		// todo: set the unit to aggrostate and try to find cover
		_unit->setTimeUnits(0);


	_terrain->calculateUnitLighting();
	_terrain->calculateFOV(_unit);

	_unit->setCache(0);
	_parent->getMap()->cacheUnit(_unit);

	if (!_falling)
		_parent->popState();
}

/**
 * kL. Checks visibility for new opponents.
 * @return, True if a new enemy is spotted
 */
bool UnitWalkBState::visForUnits()
{
	if (_falling)
		return false;

	bool newVis = false;

	if (_unit->getFaction() == FACTION_PLAYER)
	{
		newVis = _terrain->calculateFOV(_unit);
		Log(LOG_INFO) << "UnitWalkBState::visForUnits() : Faction_Player, vis = " << newVis;
	}
	else
	{
		newVis = _unit->getUnitsSpottedThisTurn().size() > _unitsSpotted
					&& !_action.desperate
					&& !_unit->getCharging()
					&& _parent->getPanicHandled();
		Log(LOG_INFO) << "UnitWalkBState::visForUnits() : Faction_!Player, vis = " << newVis;
	}

	return newVis;
}

/**
 * Sets the animation speed of soldiers or aliens.
 */
void UnitWalkBState::setNormalWalkSpeed()
{
	if (_unit->getFaction() == FACTION_PLAYER)
		_parent->setStateInterval(Options::getInt("battleXcomSpeed"));
	else
		_parent->setStateInterval(Options::getInt("battleAlienSpeed"));
}

/**
 * Handles the stepping sounds.
 */
void UnitWalkBState::playMovementSound()
{
	if ((!_unit->getVisible()
			&& !_parent->getSave()->getDebugMode())
		|| !_walkCam->isOnScreen(_unit->getPosition()))
	{
		return;
	}

	if (_unit->getMoveSound() != -1)
	{
		if (_unit->getWalkingPhase() == 0) // if a sound is configured in the ruleset, play that one
			_parent->getResourcePack()->getSound(
												"BATTLE.CAT",
												_unit->getMoveSound())
											->play();
	}
	else
	{
		if (_unit->getStatus() == STATUS_WALKING)
		{
			Tile* t = _unit->getTile();
			Tile* tBelow = _parent->getSave()->getTile(t->getPosition() + Position(0, 0, -1));

			if (_unit->getWalkingPhase() == 3) // play footstep sound 1
			{
				if (t->getFootstepSound(tBelow)
					&& _unit->getRaceString() != "STR_ETHEREAL")
				{
					_parent->getResourcePack()->getSound(
														"BATTLE.CAT",
														23 + (t->getFootstepSound(tBelow) * 2))
													->play();
				}
			}

			if (_unit->getWalkingPhase() == 7) // play footstep sound 2
			{
				if (t->getFootstepSound(tBelow)
					&& _unit->getRaceString() != "STR_ETHEREAL")
				{
					_parent->getResourcePack()->getSound(
														"BATTLE.CAT",
														22 + (t->getFootstepSound(tBelow) * 2))
													->play();
				}
			}
		}
		else if (_unit->getStatus() == STATUS_FLYING)
		{
			if (_unit->getWalkingPhase() == 1 // play default flying sound
				&& !_falling)
			{
				_parent->getResourcePack()->getSound(
													"BATTLE.CAT",
													15)
												->play();
			}
		}
	}
}

}
