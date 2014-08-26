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

#include "UnitWalkBState.h"

#include "BattleAIState.h"
#include "BattlescapeState.h"
#include "Camera.h"
//kL #include "ExplosionBState.h"
#include "Map.h"
#include "Pathfinding.h"
#include "ProjectileFlyBState.h"
#include "UnitFallBState.h"
#include "TileEngine.h"

#include "../Engine/Game.h"
#include "../Engine/Logger.h"
#include "../Engine/Options.h"
#include "../Engine/Sound.h"
#include "../Engine/Surface.h" // kL, for turning on/off visUnit indicators.

#include "../Resource/ResourcePack.h"

#include "../Ruleset/Armor.h"

#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

/**
 * Sets up a UnitWalkBState.
 * @param parent - Pointer to the Battlescape.
 * @param action - A battle action struct.
 */
UnitWalkBState::UnitWalkBState(
		BattlescapeGame* parent,
		BattleAction action)
	:
		BattleState(
			parent,
			action),
		_unit(NULL),
		_pf(NULL),
		_terrain(NULL),
		_falling(false),
		_preStepTurn(false),
		_unitsSpotted(0),
		_preStepCost(0),
		_tileSwitchDone(false),
		_onScreen(false),
		_walkCam(NULL),
		_start(-1),
		_kneelCheck(true)
{
	//Log(LOG_INFO) << "Create UnitWalkBState";
}

/**
 * Deletes the UnitWalkBState.
 */
UnitWalkBState::~UnitWalkBState()
{
	//Log(LOG_INFO) << "Delete UnitWalkBState";
}

/**
 * Initializes the state.
 */
void UnitWalkBState::init()
{
	_unit = _action.actor;
	//Log(LOG_INFO) << "\nUnitWalkBState::init() unitID = " << _unit->getId();

	setNormalWalkSpeed();

	_pf = _parent->getPathfinding();
	_terrain = _parent->getTileEngine();
	_walkCam = _parent->getMap()->getCamera();

	// kL_note: This is used only for aLiens
	_unitsSpotted = _unit->getUnitsSpottedThisTurn().size();

	//Log(LOG_INFO) << ". walking from " << _unit->getPosition() << " to " << _action.target;

	_start = _pf->getStartDirection();
	//Log(LOG_INFO) << ". strafe = " << (int)_action.strafe;
	//Log(LOG_INFO) << ". StartDirection(init) = " << _start;
	//Log(LOG_INFO) << ". getDirection(init) = " << _unit->getDirection();
	if (!_action.strafe						// not strafing
		&& -1 < _start && _start < 8		// moving but not up or down
		&& _start != _unit->getDirection())	// not facing in direction of movement
	{
		// kL_note: if unit is not facing in the direction that it's about to walk toward...
		// This makes the unit expend tu's if it spots a new alien when turning, but stops before actually walking.
		_preStepTurn = true;
	}

	doFallCheck(); // kL
	//Log(LOG_INFO) << "UnitWalkBState::init() EXIT";
}

/**
 * Runs state functionality every cycle.
 */
void UnitWalkBState::think()
{
	//Log(LOG_INFO) << "\n***** UnitWalkBState::think() : " << _unit->getId();
	if (_unit->isOut(true, true))
	{
		//Log(LOG_INFO) << ". . isOut() abort.";

		_pf->abortPath();
		_parent->popState();

		return;
	}

	_onScreen = _unit->getVisible();
//				&& (_walkCam->isOnScreen(_unit->getPosition(), true)
//					|| _walkCam->isOnScreen(_unit->getDestination(), true)); // kL
	//Log(LOG_INFO) << ". _onScreen = " << _onScreen;


// _oO **** STATUS WALKING **** Oo_ // #2
	if (_unit->getStatus() == STATUS_WALKING
		|| _unit->getStatus() == STATUS_FLYING)
	{
		//Log(LOG_INFO) << "STATUS_WALKING or FLYING : " << _unit->getId();
		// kL_begin:
//		if (_unit->getVisible())
		if (_onScreen)
		{
			//Log(LOG_INFO) << ". onScreen";
			int dest_z = _unit->getDestination().z;

			if (_walkCam->isOnScreen(
								_unit->getPosition())
//								true))
				&& _walkCam->getViewLevel() < dest_z)
			{
				//Log(LOG_INFO) << ". . setViewLevel(dest_z)";
				_walkCam->setViewLevel(dest_z);
			}
		} // kL_end.

		if (!doStatusWalk())
			return;


	// _oO **** STATUS STANDING end **** Oo_ // #3
		// kL_note: walkPhase reset as the unit completes its transition to the next tile
		if (_unit->getStatus() == STATUS_STANDING)
		{
			//Log(LOG_INFO) << "Hey we got to STATUS_STANDING in UnitWalkBState _WALKING or _FLYING !!!" ;
			// kL_begin:
//			if (_unit->getVisible())
			if (_onScreen)
			{
				if (_unit->getFaction() != FACTION_PLAYER
					&& !_walkCam->isOnScreen(
										_unit->getPosition()))
//kL									true))
				{
					_walkCam->centerOnPosition(_unit->getPosition());
				}
				else if (_walkCam->isOnScreen(
										_unit->getPosition()))
//										true))
				{
					//Log(LOG_INFO) << ". onScreen";
					int dest_z = _unit->getDestination().z;

					if (_walkCam->getViewLevel() > dest_z
						&& (_pf->getPath().size() == 0
							|| _pf->getPath().back() != _pf->DIR_UP))
					{
						//Log(LOG_INFO) << ". . setViewLevel(dest_z)";
						_walkCam->setViewLevel(dest_z);
					}
				}
			} // kL_end.

			if (!doStatusStand_end())
				return;
			else
				_parent->getBattlescapeState()->refreshVisUnits();
		}
		else if (_onScreen) // still walking ... make sure the unit sprites are up to date
		{
			//Log(LOG_INFO) << ". _onScreen : still walking ...";
			if (_pf->getStrafeMove())
			{
				//Log(LOG_INFO) << ". . strafe, face = " << _unit->getDirection();
				int face = _unit->getDirection(); // direction of travel

				_unit->setDirection(
								_unit->getFaceDirection(),
								false);

				_unit->setCache(NULL); // kL, might play around with Strafe anim's ......
				_parent->getMap()->cacheUnit(_unit);

				_unit->setDirection(
								face,
								false);
			}
			else
			{
				//Log(LOG_INFO) << ". . no strafe, cacheUnit()";
				_unit->setCache(NULL); // kL, might play around with non-Strafe anim's ......
				_parent->getMap()->cacheUnit(_unit);
			}
		}
	}


// _oO **** STATUS STANDING **** Oo_ // #1 & #4
	if (_unit->getStatus() == STATUS_STANDING
		|| _unit->getStatus() == STATUS_PANICKING)
	{
		//Log(LOG_INFO) << "STATUS_STANDING or PANICKING : " << _unit->getId();

		if (!doStatusStand())
			return;
		else // kL_begin: Destination is not valid until *after* doStatusStand() runs.
		{
//			if (_unit->getVisible())
			if (_onScreen)
			{
				//Log(LOG_INFO) << ". onScreen";
				int pos_z = _unit->getPosition().z;

				if (_unit->getFaction() != FACTION_PLAYER
					&& !_walkCam->isOnScreen(
										_unit->getPosition()))
//kL									true))
				{
					_walkCam->centerOnPosition(_unit->getPosition());
				}
				else if (_walkCam->isOnScreen(
											_unit->getPosition()))
//											true))
				{
					//Log(LOG_INFO) << ". cam->onScreen";
					int dest_z = _unit->getDestination().z;

					//Log(LOG_INFO) << ". dest_z == pos_z : " << (dest_z == pos_z);
					//Log(LOG_INFO) << ". dest_z > pos_z : " << (dest_z > pos_z);
					//Log(LOG_INFO) << ". _walkCam->getViewLevel() < pos_z : " << (_walkCam->getViewLevel() < pos_z);
					if (dest_z == pos_z
						|| (dest_z > pos_z
//							&& _walkCam->getViewLevel() < pos_z))
							&& _walkCam->getViewLevel() < dest_z))
//							&& _pf->getPath().size() > 0))
//							&& _unit->getPosition().x != _unit->getDestination().x
//							&& _unit->getPosition().y != _unit->getDestination().y))
					{
						//Log(LOG_INFO) << ". . setViewLevel(pos_z)";
						_walkCam->setViewLevel(pos_z);
					}
				}
			}
		} // kL_end.
	}


// _oO **** STATUS TURNING **** Oo_
	if (_unit->getStatus() == STATUS_TURNING) // turning during walking costs no tu
	{
		//Log(LOG_INFO) << "STATUS_TURNING : " << _unit->getId();
		doStatusTurn();
	}
	//Log(LOG_INFO) << "think() : " << _unit->getId() << " EXIT ";
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
	//Log(LOG_INFO) << "***** UnitWalkBState::doStatusStand() : " << _unit->getId();
	int dir = _pf->getStartDirection();
	//Log(LOG_INFO) << ". StartDirection = " << dir;

	Tile* tile = _parent->getSave()->getTile(_unit->getPosition());
	bool gravLift = dir >= _pf->DIR_UP // Assumes tops & bottoms of gravLifts always have floors/ceilings.
					&& tile->getMapData(MapData::O_FLOOR)
					&& tile->getMapData(MapData::O_FLOOR)->isGravLift();

	if (dir > -1
		&& _kneelCheck				== true		// check if unit is kneeled
		&& _unit->isKneeled()		== true		// unit is kneeled
		&& gravLift					== false	// not on a gravLift
		&& _pf->getPath().empty()	== false)	// not the final tile of path; that is, the unit is actually going to move.
	{
		//Log(LOG_INFO) << ". kneeled, and path Valid";
		_kneelCheck = false;

		if (_parent->kneel(
						_unit,
						false))
		{
			//Log(LOG_INFO) << ". . Stand up";
			_unit->setCache(NULL);
			_parent->getMap()->cacheUnit(_unit);

			if (_terrain->checkReactionFire(_unit)) // unit got fired upon - stop.
			{
				_pf->abortPath();
				_parent->popState();

				return false;
			}
		}
		else
		{
			//Log(LOG_INFO) << ". . don't stand: not enough TU";
			_action.result = "STR_NOT_ENOUGH_TIME_UNITS";

			_pf->abortPath();
			_parent->popState();

			return false;
		}
	}

	_tileSwitchDone = false;

	if (visForUnits())
	{
		//if (Options::traceAI) { Log(LOG_INFO) << "Uh-oh! Company!"; }
		//Log(LOG_INFO) << "Uh-oh! STATUS_STANDING or PANICKING Company!";
		//if (_unit->getFaction() == FACTION_PLAYER) Log(LOG_INFO) << ". . _newVis = TRUE, postPathProcedures";
		//else if (_unit->getFaction() != FACTION_PLAYER) Log(LOG_INFO) << ". . _newUnitSpotted = TRUE, postPathProcedures";

		if (_unit->getFaction() != FACTION_PLAYER)	// kL, Can civies hideForTurn ?
			_unit->_hidingForTurn = false;			// 'cause, clearly we're not hidden now!!1

		_unit->setCache(NULL);					// kL. Calls to cacheUnit() are bogus without setCache(NULL) first...!
		_parent->getMap()->cacheUnit(_unit);	// although _cacheInvalid might be set elsewhere but i doubt it.

		postPathProcedures();

		return false;
	}

/*kL	if (_onScreen
		|| _parent->getSave()->getDebugMode())
	{
		setNormalWalkSpeed();
	}
	else
//kL	_parent->setStateInterval(0);
		_parent->setStateInterval(11); // kL
	*/
//	setNormalWalkSpeed(); // kL: Done in init()

//	int dir = _pf->getStartDirection();
	//Log(LOG_INFO) << ". getStartDirection() dir = " << dir;

	if (_falling)
	{
		dir = _pf->DIR_DOWN;
		//Log(LOG_INFO) << ". . _falling, dir = " << dir;
	}

	if (dir != -1)
	{
		//Log(LOG_INFO) << "enter (dir!=-1) : " << _unit->getId();
		if (_pf->getStrafeMove())
		{
			if (_unit->getTurretType() > -1)
			{
				_unit->setFaceDirection((_start + 4) %8);

				int turretOffset = _unit->getTurretDirection() - _unit->getDirection();
				_unit->setTurretDirection((turretOffset + _start + 4) %8);
			}
			else
			{
				//Log(LOG_INFO) << ". . strafeMove, setFaceDirection() -> " << _unit->getDirection();
				_unit->setFaceDirection(_unit->getDirection());
			}
		}

		//Log(LOG_INFO) << ". getTUCost() & destination";
		// gets tu cost, but also gets the destination position.
		Position destination;
		int tu = _pf->getTUCost(
							_unit->getPosition(),
							dir,
							&destination,
							_unit,
							NULL,
							false);
		//Log(LOG_INFO) << ". tu = " << tu;

		Tile* tileDest = _parent->getSave()->getTile(destination);
		// kL_note: should this include neutrals? (ie != FACTION_PLAYER; see also 32tu inflation...)
		if (_unit->getFaction() == FACTION_HOSTILE
			&& tileDest
			&& tileDest->getFire() > 0)
		{
			//Log(LOG_INFO) << ". . subtract tu inflation for a fireTile";
			// we artificially inflate the TU cost by 32 points in getTUCost
			// under these conditions, so we have to deflate it here.
			tu -= 32;
			//Log(LOG_INFO) << ". . subtract tu inflation for a fireTile DONE";
		}

		if (_falling)
		{
			//Log(LOG_INFO) << ". . falling, set tu 0";
			tu = 0;
		}

		int
			energy = tu,
			tuTest = tu;

		if (!gravLift)
		{
			if (_action.run // allow dash when moving vertically 1 tile (or more).
				|| (_action.strafe
					&& dir >= _pf->DIR_UP))
			{
//				_unit->setDashing(true);	// will be removed either on next movement click
											// or at end of Player turn. -> No good, move it
											// back to BattlescapeGame::primaryAction() because
											// it's needed for setting the animation speed in init()
//				tu = tu * 3 / 4;
//				energy = energy * 3 / 2;

				tu -= _pf->getOpenDoor();
				tu = (tu * 3 / 4) + _pf->getOpenDoor();

				energy -= _pf->getOpenDoor();
				energy = energy * 3 / 2;
			}

			std::string armorType = _unit->getArmor()->getType();

			if (_unit->hasFlightSuit()
				&& _pf->getMovementType() == MT_FLY)
			{
				energy -= 2; // zippy.
			}
			else if (_unit->hasPowerSuit()
				|| (_unit->hasFlightSuit()
					&& _pf->getMovementType() == MT_WALK))
			{
				energy -= 1; // good stuff
			}
			// else if (coveralls){} // normal energy expenditure
			else if (armorType == "STR_PERSONAL_ARMOR_UC")
				energy += 1; // *clunk*clunk*
		}
		else // gravLift
		{
			//Log(LOG_INFO) << ". . using GravLift";
			energy = 0;
		}

		if (energy < 0)
			energy = 0;

		//Log(LOG_INFO) << ". check tu + stamina, etc. TU = " << tu;
		//Log(LOG_INFO) << ". unit->TU = " << _unit->getTimeUnits();
		if (tu > _unit->getTimeUnits())
		{
			//Log(LOG_INFO) << ". . tu > _unit->TU()";
			if (_parent->getPanicHandled()
//kL			&& tu < 255)
				&& tuTest < 255)
			{
				//Log(LOG_INFO) << ". send warning: not enough TU";
				_action.result = "STR_NOT_ENOUGH_TIME_UNITS";
			}

			_pf->abortPath();

			_unit->setCache(NULL);
			_parent->getMap()->cacheUnit(_unit);

			_parent->popState();

			return false;
		}
		else if (energy > _unit->getEnergy())
		{
			//Log(LOG_INFO) << ". . energy > _unit->getEnergy()";
			if (_parent->getPanicHandled())
				_action.result = "STR_NOT_ENOUGH_ENERGY";

			_pf->abortPath();

			_unit->setCache(NULL);
			_parent->getMap()->cacheUnit(_unit);

			_parent->popState();

			return false;
		}
		else if (_parent->getPanicHandled()
			&& _parent->checkReservedTU(_unit, tu) == false)
		{
			//Log(LOG_INFO) << ". . checkReservedTU(_unit, tu) == false";
			_pf->abortPath();

			_unit->setCache(NULL);
			_parent->getMap()->cacheUnit(_unit);

			return false;
		}
		// we are looking in the wrong way, turn first (unless strafing)
		// we are not using the turn state, because turning during walking costs no tu
		else if (dir != _unit->getDirection()
			&& dir < _pf->DIR_UP
			&& _pf->getStrafeMove() == false)
		{
			//Log(LOG_INFO) << ". . dir != _unit->getDirection() -> turn";
			_unit->lookAt(dir);

			_unit->setCache(NULL);
			_parent->getMap()->cacheUnit(_unit);

			return false;
		}
		else if (dir < _pf->DIR_UP) // now open doors (if any)
		{
			//Log(LOG_INFO) << ". . check for doors";
			int door = _terrain->unitOpensDoor(
											_unit,
											false,
											dir);
			if (door == 3) // ufo door still opening ...
			{
				//Log(LOG_INFO) << ". . . door #3";
				return false; // don't start walking yet, wait for the ufo door to open
			}
			else if (door == 0) // normal door
			{
				//Log(LOG_INFO) << ". . . door #0";
				_parent->getResourcePack()->getSound("BATTLE.CAT", 3)->play();
			}
			else if (door == 1) // ufo door
			{
				//Log(LOG_INFO) << ". . . door #1";
				_parent->getResourcePack()->getSound("BATTLE.CAT", 20)->play();

				return false; // don't start walking yet, wait for the ufo door to open
			}
		}

		// kL_begin: proxy blows up in face after door opens - copied doStatusStand_end()
		if (_parent->checkForProximityGrenades(_unit))
			// kL_add: Put checkForSilacoid() here!
		{
			_parent->popState();
//			postPathProcedures(); // .. one or the other i suppose.

			return false;
		} // kL_end

		//Log(LOG_INFO) << ". check size for obstacles";
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
				//Log(LOG_INFO) << ". . check obstacle(unit)";
				BattleUnit* unitInMyWay = _parent->getSave()->getTile(destination + Position(x, y, 0))->getUnit();
				BattleUnit* unitBelowMyWay = NULL;

				Tile* belowDest = _parent->getSave()->getTile(destination + Position(x, y,-1));
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
					//Log(LOG_INFO) << ". . . obstacle(unit) -> abortPath()";
					_action.TU = 0;
					_pf->abortPath();

					_unit->setCache(NULL);
					_parent->getMap()->cacheUnit(_unit);

					_parent->popState();

					return false;
				}
			}
		}

		//Log(LOG_INFO) << ". dequeuePath()";
		dir = _pf->dequeuePath(); // now start moving

		if (_falling)
		{
			//Log(LOG_INFO) << ". . falling, _pf->DIR_DOWN";
			dir = _pf->DIR_DOWN;
		}
		//Log(LOG_INFO) << ". dequeuePath() dir = " << dir;

//		if (dir == _pf->DIR_UP)	// kL
//			_walkCam->up();		// kL

		if (_unit->spendTimeUnits(tu)		// These were checked above and don't really need to
			&& _unit->spendEnergy(energy))	// be checked again here. Only subtract required.
		{
			//Log(LOG_INFO) << ". . WalkBState: spend TU & Energy";
			Tile* tileBelow = _parent->getSave()->getTile(_unit->getPosition() + Position(0, 0,-1));

			//Log(LOG_INFO) << ". . WalkBState: startWalking()";
			_unit->startWalking(
							dir,
							destination,
							tileBelow,
							_onScreen);

			_preStepTurn = false;
		}

		// make sure the unit sprites are up to date
		// kL_note: This could probably go up under spend tu+energy. but.....
		// And since Status_Stand algorithm doesn't actually draw anything
		// REMARK It.
/*kL	if (_onScreen)
		{
			//Log(LOG_INFO) << ". . _onScreen";
			if (_pf->getStrafeMove())
			{
				//Log(LOG_INFO) << ". . . (_onScreen) -> _pf->getStrafeMove()";

				// This is where we fake out the strafe movement direction so the unit "moonwalks"
				int face = _unit->getDirection();
//				int face = dir; // kL

				_unit->setDirection(_unit->getFaceDirection());
//				_unit->setCache(NULL); // kL
//				_parent->getMap()->cacheUnit(_unit); // kL ( see far above, re. strafe fake-out moonwalking )

				_unit->setDirection(face);
//				_unit->setDirection(dir);

				//Log(LOG_INFO) << ". . . end strafeMove()";
			}
//			else // kL
//			{
			//Log(LOG_INFO) << ". . (_onScreen) -> cacheUnit()";

//			_unit->setCache(NULL); // kL
			_parent->getMap()->cacheUnit(_unit);
//			}

			//Log(LOG_INFO) << ". . end (_onScreen)";
		} */
		//Log(LOG_INFO) << ". EXIT (dir!=-1) : " << _unit->getId();
	}
	else // dir == -1
	{
		//Log(LOG_INFO) << ". unit direction = " << _unit->getDirection();
		//Log(LOG_INFO) << ". . postPathProcedures()";
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
	//Log(LOG_INFO) << "***** UnitWalkBState::doStatusWalk() : " << _unit->getId();
	Tile* tBelow = NULL;

	if (_parent->getSave()->getTile(_unit->getDestination())->getUnit() == NULL  // next tile must be not occupied
		// kL_note: and, if not flying, the position directly below the tile must not be occupied...
		// Had that happen with a sectoid left standing in the air because a cyberdisc was 2 levels below it.
		// btw, these have probably been already checked...
		|| _parent->getSave()->getTile(_unit->getDestination())->getUnit() == _unit)
	{
		//Log(LOG_INFO) << ". WalkBState, keepWalking()";
		playMovementSound();

		tBelow = _parent->getSave()->getTile(_unit->getPosition() + Position(0, 0,-1));
		_unit->keepWalking( // advances _walkPhase
						tBelow,
						_onScreen);
	}
	else if (!_falling)
	{
		//Log(LOG_INFO) << ". WalkBState, !falling Abort path";
		_unit->lookAt( // turn to blocking unit
					_unit->getDestination(),
					_unit->getTurretType() != -1);

		_pf->abortPath();
		_unit->setStatus(STATUS_STANDING);
	}

	// unit moved from one tile to the other, update the tiles & investigate new flooring
	if (!_tileSwitchDone
		&& _unit->getPosition() != _unit->getLastPosition())
		// ( _pos != _lastpos ) <- set equal at Start, _walkPhase == 0.
		// this clicks over in keepWalking(_walkPhase == middle)
		// rougly _walkPhase == 4
		// when _pos sets equal to _destination.
		// So. How does _falling ever equate TRUE, if it has to be flagged on _walkPhase == 0 only?
	{
		//Log(LOG_INFO) << ". tile switch from _lastpos to _destination";
		// BattleUnit::startWalking() sets _lastpos = _pos, then
		// BattleUnit::keepWalking (_walkPhase == middle) sets _pos = _destination
		_tileSwitchDone = true;

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
				tBelow = _parent->getSave()->getTile(_unit->getPosition() + Position(x, y,-1));

				if ( //kL _unit->getArmor()->getMovementType() == MT_FLY ||
					!_parent->getSave()->getTile(
											_unit->getPosition() + Position(x, y, 0))
										->hasNoFloor(tBelow))
				{
					//Log(LOG_INFO) << ". . . hasFloor ( fallCheck set FALSE )";
					fallCheck = false;
				}

				//Log(LOG_INFO) << ". . remove unit from previous tile";
				_parent->getSave()->getTile(
										_unit->getLastPosition() + Position(x, y, 0))
									->setUnit(NULL);
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
				//Log(LOG_INFO) << ". . set unit on new tile";
				_parent->getSave()->getTile(
										_unit->getPosition() + Position(x, y, 0))
									->setUnit(
											_unit,
											_parent->getSave()->getTile(
																	_unit->getPosition() + Position(x, y,-1)));
			}
		}

		_falling = fallCheck
					&& _unit->getPosition().z != 0
//kL				&& _unit->getTile()->hasNoFloor(tileBelow) // kL_note: Done above.
					&& _pf->getMovementType() != MT_FLY;	//_unit->getArmor()->getMovementType() != MT_FLY
															// -> sorta move to doStatusStand_end(), perhaps
//						|| (_pf->isModALT()						// <- note: these two should be the same as
//							&& _unit->getTurretType() == -1));	// (_pf->getMovementType() != MT_FLY) preceding.
//kL				&& _unit->getWalkingPhase() == 0; // <- set @ startWalking() and @ end of keepWalking()

		if (_falling)
		{
			//Log(LOG_INFO) << ". falling";
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
					tBelow = _parent->getSave()->getTile(_unit->getPosition() + Position(x, y,-1));
					//if (tBelow) Log(LOG_INFO) << ". . otherTileBelow exists";

					if (tBelow
						&& tBelow->getUnit())
					{
						//Log(LOG_INFO) << ". . . another unit already occupies lower tile";

						_falling = false;

						_pf->dequeuePath();
						_parent->getSave()->addFallingUnit(_unit);

						//Log(LOG_INFO) << "UnitWalkBState::think(), addFallingUnit() ID " << _unit->getId();
						_parent->statePushFront(new UnitFallBState(_parent));

						return false;
					}
					//else Log(LOG_INFO) << ". . otherTileBelow Does NOT contain other unit";
				}
			}
		}
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
	//Log(LOG_INFO) << "***** UnitWalkBState::doStatusStand_end() : " << _unit->getId();

//kL	_parent->getSave()->getBattleState()->updateSoldierInfo(false); // update the TU display.

	_tileSwitchDone = false;

	if (_unit->getFaction() != FACTION_PLAYER)
		_unit->setVisible(false);

	if (_unit->getSpecialAbility() == SPECAB_BURNFLOOR) // if the unit burns floortiles, burn floortiles
	{
		// kL_add: Put burnedBySilacoid() here! etc
		_unit->getTile()->ignite(1);

		Position pos = _unit->getPosition() * Position(16, 16, 24)
						+ Position(
								8,
								8,
								-(_unit->getTile()->getTerrainLevel()));
		_parent->getTileEngine()->hit(
									pos,
									_unit->getStats()->strength, // * _unit->getAccuracyModifier(),
									DT_IN,
									_unit);
	}

	_terrain->calculateUnitLighting(); // move our personal lighting with us

	// This needs to be done *before* the calculateFOV(pos)
	// or else any newVis will be marked Visible before
	// visForUnits() catches the new unit that is !Visible.
	bool newVis = visForUnits();

	// This calculates or 'refreshes' the Field of View
	// of all units within maximum distance (20 tiles) of this unit.
	_terrain->calculateFOV(_unit->getPosition());

	if (_parent->checkForProximityGrenades(_unit))
		// kL_add: Put checkForSilacoid() here!
	{
		_parent->popState();

		return false;
	}
	else if (newVis)
	{
		//if (_unit->getFaction() == FACTION_PLAYER) Log(LOG_INFO) << ". . _newVis = TRUE, Abort path";
		//else if (_unit->getFaction() != FACTION_PLAYER) Log(LOG_INFO) << ". . _newUnitSpotted = TRUE, Abort path";

		_unit->setCache(NULL);
		_parent->getMap()->cacheUnit(_unit);

		_pf->abortPath();
		_parent->popState();

		return false;
	}
	else if (!_falling) // check for reaction fire
	{
		//Log(LOG_INFO) << ". . WalkBState: NOT falling, checkReactionFire()";

		if (_terrain->checkReactionFire(_unit)) // unit got fired upon - stop walking
		{
			//Log(LOG_INFO) << ". . . cacheUnit";
			_unit->setCache(NULL);
			_parent->getMap()->cacheUnit(_unit);

			_pf->abortPath();
			_parent->popState();

			return false;
		}
		//else Log(LOG_INFO) << ". . WalkBState: checkReactionFire() FALSE... no caching";
	}
//	else	// <<-- Looks like we gotta make it fall here!!! (if unit *ends* its total walk sequence on empty air.
			// And, fall *before* spotting new units, else Abort will likely make it float...
//	{
		//Log(LOG_INFO) << ". . WalkBState: falling";
//		_unit->setCache(NULL);
//		_parent->getMap()->cacheUnit(_unit);
//	}

	return true;
}

/**
 * This function turns unit during movement.
 * Called from think()
 */
void UnitWalkBState::doStatusTurn()
{
	//Log(LOG_INFO) << "***** UnitWalkBState::doStatusTurn() : " << _unit->getId();
	if (_preStepTurn)	// turning during walking costs no tu
		_preStepCost++;		// except before the first step.

	_unit->turn();

//	if (_onScreen) // kL, old code. // make sure the unit sprites are up to date
//	{
		//Log(LOG_INFO) << ". cacheUnit()";

	_unit->setCache(NULL);
	_parent->getMap()->cacheUnit(_unit);
//	}

	// calculateFOV() is unreliable for setting the _newUnitSpotted bool,
	// as it can be called from various other places in the code, ie:
	// doors opening (& explosions/terrain destruction?), and that messes up the result.
	// kL_note: But let's do it anyway!
	if (visForUnits())
	{
		if (_preStepTurn)
			_unit->spendTimeUnits(_preStepCost);

		//Log(LOG_INFO) << "Egads! STATUS_TURNING reveals new units!!! I must pause!";
		//if (_unit->getFaction() == FACTION_PLAYER) Log(LOG_INFO) << ". . _newVis = TRUE, Abort path, popState";
		//else if (_unit->getFaction() != FACTION_PLAYER) Log(LOG_INFO) << ". . _newUnitSpotted = TRUE, Abort path, popState";

		if (_unit->getFaction() != FACTION_PLAYER)
			_unit->_hidingForTurn = false;

		_pf->abortPath();
		_unit->setStatus(STATUS_STANDING);

		_unit->setCache(NULL);
		_parent->getMap()->cacheUnit(_unit);

		_parent->popState();
	}
	else
		_parent->getBattlescapeState()->refreshVisUnits();
}

/**
 * Handles some calculations when the path is finished.
 */
void UnitWalkBState::postPathProcedures()
{
	//Log(LOG_INFO) << "UnitWalkBState::postPathProcedures(), unit = " << _unit->getId();
	_tileSwitchDone = false;
	_action.TU = 0;

	if (_unit->getFaction() != FACTION_PLAYER)
	{
		int dir = _action.finalFacing;

		if (_action.finalAction)
			_unit->dontReselect();

		if (_unit->getCharging() != NULL)
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
														dir))
			{
				BattleAction action;
				action.actor		= _unit;
				action.target		= _unit->getCharging()->getPosition();
				action.weapon		= _unit->getMainHandWeapon();
				action.type			= BA_HIT;
				action.TU			= _unit->getActionTUs(
														action.type,
														action.weapon);
				action.targeting	= true;

				_unit->setCharging(NULL);
				_parent->statePushBack(new ProjectileFlyBState(
															_parent,
															action));
			}
		}
		else if (_unit->_hidingForTurn)
		{
			dir = _unit->getDirection() + 4;

			_unit->_hidingForTurn = false;
			_unit->dontReselect();
		}

		if (dir != -1)
		{
			_unit->lookAt(dir %8);

			while (_unit->getStatus() == STATUS_TURNING)
			{
				_unit->turn();
				_parent->getTileEngine()->calculateFOV(_unit);
				// kL_note: might need newVis/newUnitSpotted -> abort
			}

			// kL_note: These should prob be done before this pt.
//			_unit->setCache(NULL);					// kL
//			_parent->getMap()->cacheUnit(_unit);	// kL
		}
	}
	else if (!_parent->getPanicHandled()) // todo: set the unit to aggrostate and try to find cover
		_unit->setTimeUnits(0);


	_terrain->calculateUnitLighting();
//kL	_terrain->calculateFOV(_unit);
	_terrain->calculateFOV(_unit->getPosition()); // kL, in case unit opened a door and stopped without doing Status_WALKING

	_unit->setCache(NULL);
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
		//Log(LOG_INFO) << "UnitWalkBState::visForUnits() : Faction_Player, vis = " << newVis;
	}
	else
	{
		newVis = _terrain->calculateFOV(_unit) // <- 14.07.06
					&& _unit->getUnitsSpottedThisTurn().size() > _unitsSpotted
					&& !_action.desperate
					&& !_unit->getCharging()
					&& _parent->getPanicHandled();
		//Log(LOG_INFO) << "UnitWalkBState::visForUnits() : Faction_!Player, vis = " << newVis;
	}

	return newVis;
}

/**
 * Sets the animation speed of soldiers or aliens.
 */
void UnitWalkBState::setNormalWalkSpeed()
{
	if (_unit->getFaction() == FACTION_PLAYER)
	{
		if (_action.run) //|| _unit->getDashing())
			_parent->setStateInterval(static_cast<Uint32>(Options::battleXcomSpeed * 3 / 4));
		else
			_parent->setStateInterval(static_cast<Uint32>(Options::battleXcomSpeed));
	}
	else
		_parent->setStateInterval(static_cast<Uint32>(Options::battleAlienSpeed));
}

/**
 * Handles the stepping sounds.
 */
void UnitWalkBState::playMovementSound()
{
	if (!_unit->getVisible()
		&& !_parent->getSave()->getDebugMode())
//kL		|| (!_walkCam->isOnScreen(_unit->getPosition(), true)
//kL			&& !_walkCam->isOnScreen(_unit->getDestination(), true)))
	{
		return;
	}

	if (_unit->getMoveSound() != -1)
	{
		if (_unit->getWalkingPhase() == 0) // if a sound is configured in the ruleset, play it.
		{
			_parent->getResourcePack()->getSound(
												"BATTLE.CAT",
												_unit->getMoveSound())
											->play();
		}
	}
	else
	{
		if (_unit->getStatus() == STATUS_WALKING)
		{
			Tile* t = _unit->getTile();
			Tile* tBelow = _parent->getSave()->getTile(t->getPosition() + Position(0, 0,-1));

			if (_unit->getWalkingPhase() == 3) // play footstep sound 1
			{
				if (t->getFootstepSound(tBelow))
//					&& _unit->getRaceString() != "STR_ETHEREAL")
				{
					_parent->getResourcePack()->getSound(
														"BATTLE.CAT",
														23 + (t->getFootstepSound(tBelow) * 2))
													->play();
				}
			}
			else if (_unit->getWalkingPhase() == 7) // play footstep sound 2
			{
				if (t->getFootstepSound(tBelow))
//					&& _unit->getRaceString() != "STR_ETHEREAL")
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
			if (_unit->getWalkingPhase() == 1) // play default flying sound
			{
				if (_falling)
				{
					if (groundCheck(1))
						_parent->getResourcePack()->getSound( // thunk.
															"BATTLE.CAT",
															38)
														->play();
				}
				else if (!_unit->isFloating())
				{
					_parent->getResourcePack()->getSound( // GravLift
														"BATTLE.CAT",
														40)
													->play();
				}
				else //if (!_falling)
				{
					_parent->getResourcePack()->getSound( // hoverSound
														"BATTLE.CAT",
														15)
													->play();
				}
			}
		}
	}
}

/**
 * kL. For determining if a flying unit turns flight off at start of movement.
 * NOTE: _falling should always be false when this is called in init().
 * NOTE: And unit must be capable of flight for this to be relevant.
 * NOTE: This could get problemmatic if/when falling onto nonFloors like water,
 *		 and/or if there is another unit on tileBelow.
 */
void UnitWalkBState::doFallCheck() // kL
{
	if (_pf->getMovementType() == MT_FLY //_unit->getArmor()->getMovementType() != MT_FLY
		|| _unit->getPosition().z == 0
//		|| _pf->isModALT() == false
		|| groundCheck() == true)
	{
		return;
	}

	_falling = true;
}

/**
 * kL. Checks if there is ground below when unit is falling.
 * NOTE: Pathfinding already has a function canFallDown() that could be used for
 * a couple places here in UnitWalkBState; does not have 'descent' though.
 @param descent - how many levels below current to check for ground (default 0)
 */
bool UnitWalkBState::groundCheck(int descent) // kL
{
	Tile* tBelow = NULL;
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
			tBelow = _parent->getSave()->getTile(_unit->getPosition() + Position(x, y, -descent - 1));
			if (!_parent->getSave()->getTile(
										_unit->getPosition() + Position(x, y, -descent))
									->hasNoFloor(tBelow))
			{
				return true;
			}
		}
	}

	return false;
}

}
