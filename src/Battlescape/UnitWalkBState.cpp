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

#include "UnitWalkBState.h"

#include "BattlescapeState.h"
#include "Camera.h"
#include "Map.h"
//#include "MeleeAttackBState.h"
#include "Pathfinding.h"
#include "ProjectileFlyBState.h"
#include "UnitFallBState.h"
#include "TileEngine.h"

//#include "../Engine/Options.h"
#include "../Engine/Sound.h"
#include "../Engine/Surface.h" // for turning on/off visUnit indicators.

#include "../Interface/Bar.h"
#include "../Interface/NumberText.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleArmor.h"

#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

/**
 * Sets up a UnitWalkBState.
 * @param parent - pointer to the BattlescapeGame
 * @param action - the BattleAction struct (BattlescapeGame.h)
 */
UnitWalkBState::UnitWalkBState(
		BattlescapeGame* parent,
		BattleAction action) // these BattleActions had better be assignments/copies ... not references.
	:
		BattleState(
			parent,
			action),
		_unit(action.actor),
		_pf(parent->getPathfinding()),
		_terrain(parent->getTileEngine()),
		_falling(false),
		_preStepTurn(false),
		_unitsSpotted(0),
		_preStepCost(0),
		_tileSwitchDone(false),
		_onScreen(false),
		_walkCam(parent->getMap()->getCamera()),
		_dirStart(-1),
		_kneelCheck(true)
{}

/**
 * Deletes the UnitWalkBState.
 */
UnitWalkBState::~UnitWalkBState()
{
	_parent->setStateInterval(static_cast<Uint32>(BattlescapeState::DEFAULT_ANIM_SPEED)); // kL
}

/**
 * Initializes the state.
 */
void UnitWalkBState::init()
{
//	_unit = _action.actor;

	//Log(LOG_INFO) << "\nUnitWalkBState::init() unitID = " << _unit->getId();

//	_pf = _parent->getPathfinding();
	_pf->setPathingUnit(_unit);

//	_terrain = _parent->getTileEngine();
//	_walkCam = _parent->getMap()->getCamera();

//	setWalkSpeed();


	// kL_note: This is used only for aLiens
	_unitsSpotted = _unit->getUnitsSpottedThisTurn().size();

	//Log(LOG_INFO) << ". walking from " << _unit->getPosition() << " to " << _action.target;

	_dirStart = _pf->getStartDirection();
	//Log(LOG_INFO) << ". strafe = " << (int)_action.strafe;
	//Log(LOG_INFO) << ". StartDirection(init) = " << _dirStart;
	//Log(LOG_INFO) << ". getDirection(init) = " << _unit->getDirection();
	if (_action.strafe == false					// not strafing
		&& _dirStart > -1 && _dirStart < 8		// moving but not up or down
		&& _dirStart != _unit->getDirection())	// not facing in direction of movement
	{
		// kL_note: if unit is not facing in the direction that it's about to
		// walk toward... This makes the unit expend tu's if it spots a new
		// alien when turning, but stops before actually walking. Also expends
		// tu if cancel() is called before first step.
		_preStepTurn = true;
	}

//	_unit->setFaceDirection(_unit->getDirection());

	doFallCheck(); // kL
	//Log(LOG_INFO) << "UnitWalkBState::init() EXIT";
}

/**
 * Runs state functionality every cycle.
 */
void UnitWalkBState::think()
{
	//Log(LOG_INFO) << "\n***** UnitWalkBState::think() : " << _unit->getId();
	if (_unit->isOut(true, true) == true)
	{
		//Log(LOG_INFO) << ". . isOut() abort.";
		_pf->abortPath();
		_parent->popState();

		return;
	}

//	int size = _unit->getArmor()->getSize() - 1;
//	bool onScreen = (_unit->getVisible() && _parent->getMap()->getCamera()->isOnScreen(_unit->getPosition(), true, size, false));
	_onScreen = _unit->getUnitVisible();
//				&& (_walkCam->isOnScreen(_unit->getPosition())
//					|| _walkCam->isOnScreen(_unit->getDestination()));
	//Log(LOG_INFO) << ". _onScreen = " << _onScreen;


// _oO **** STATUS WALKING **** Oo_ // #2
	if (_unit->getStatus() == STATUS_WALKING
		|| _unit->getStatus() == STATUS_FLYING)
	{
		//Log(LOG_INFO) << "STATUS_WALKING or FLYING : " << _unit->getId();
//		if (_unit->getVisible())
		if (_onScreen == true)
		{
			//Log(LOG_INFO) << ". onScreen";
			const int dest_z = _unit->getDestination().z;
			if (_walkCam->isOnScreen(_unit->getPosition()) == true
				&& _walkCam->getViewLevel() < dest_z)
			{
				//Log(LOG_INFO) << ". . setViewLevel(dest_z)";
				_walkCam->setViewLevel(dest_z);
			}
		}

		if (doStatusWalk() == false)
		{
			//Log(LOG_INFO) << ". . doStatusWalk() FALSE return";
			return;
		}


	// _oO **** STATUS STANDING end **** Oo_ // #3
		// kL_note: walkPhase reset as the unit completes its transition to the next tile
		if (_unit->getStatus() == STATUS_STANDING)
		{
			//Log(LOG_INFO) << "STATUS_STANDING_end in UnitWalkBState _WALKING or _FLYING !!!" ;
//			if (_unit->getVisible())
			if (_onScreen == true)
			{
				if (_unit->getFaction() != FACTION_PLAYER
					&& _walkCam->isOnScreen(_unit->getPosition()) == false)
				{
					_walkCam->centerOnPosition(_unit->getPosition());
				}
				else if (_walkCam->isOnScreen(_unit->getPosition()) == true)
				{
					//Log(LOG_INFO) << ". onScreen";
					const int dest_z = _unit->getDestination().z;
					if (_walkCam->getViewLevel() > dest_z
						&& (_pf->getPath().size() == 0
							|| _pf->getPath().back() != _pf->DIR_UP))
					{
						//Log(LOG_INFO) << ". . setViewLevel(dest_z)";
						_walkCam->setViewLevel(dest_z);
					}
				}
			}

			if (doStatusStand_end() == false)
			{
				//Log(LOG_INFO) << ". . doStatusStand_end() FALSE return";
				return;
			}
			else if (_parent->getPanicHandled() == true)
				_parent->getBattlescapeState()->refreshVisUnits();
		}
		else if (_onScreen == true) // still walking ... make sure the unit sprites are up to date
		{
			//Log(LOG_INFO) << ". _onScreen : still walking ...";
//			if (_pf->getStrafeMove() == true) // NOTE: This could be trimmed, because I had to make tanks use getFaceDirection() in UnitSprite::drawRoutine2() anyway ...
			if (_action.strafe == true)
			{
				//Log(LOG_INFO) << ". WALKING strafe, unitDir = " << _unit->getDirection();
				//Log(LOG_INFO) << ". WALKING strafe, faceDir = " << _unit->getFaceDirection();
				const int dirStrafe = _unit->getDirection(); // direction of travel
				_unit->setDirection(
								_unit->getFaceDirection(),
								false);

//				_unit->setCache(NULL); // kL, might play around with Strafe anim's ......
				_parent->getMap()->cacheUnit(_unit);
				_unit->setDirection(
								dirStrafe,
								false);
			}
			else
			{
				//Log(LOG_INFO) << ". WALKING no strafe, cacheUnit()";
				_unit->setCache(NULL); // might play around with non-Strafe anim's ......
				_parent->getMap()->cacheUnit(_unit);
			}
		}
	}


// _oO **** STATUS STANDING **** Oo_ // #1 & #4
	if (_unit->getStatus() == STATUS_STANDING
		|| _unit->getStatus() == STATUS_PANICKING)
	{
		//Log(LOG_INFO) << "STATUS_STANDING or PANICKING : " << _unit->getId();
		if (doStatusStand() == false)
		{
			//Log(LOG_INFO) << ". . doStatusStand() FALSE return";
			return;
		}
		else // Destination is not valid until *after* doStatusStand() runs.
		{
//			if (_unit->getVisible())
			if (_onScreen == true)
			{
				//Log(LOG_INFO) << ". onScreen";
				const int pos_z = _unit->getPosition().z;

				if (_unit->getFaction() != FACTION_PLAYER
					&& _walkCam->isOnScreen(_unit->getPosition()) == false)
				{
					_walkCam->centerOnPosition(_unit->getPosition());
				}
				else if (_walkCam->isOnScreen(_unit->getPosition()) == true)
				{
					//Log(LOG_INFO) << ". cam->onScreen";
					int dest_z = _unit->getDestination().z;
					//Log(LOG_INFO) << ". dest_z == pos_z : " << (dest_z == pos_z);
					//Log(LOG_INFO) << ". dest_z > pos_z : " << (dest_z > pos_z);
					//Log(LOG_INFO) << ". _walkCam->getViewLevel() < pos_z : " << (_walkCam->getViewLevel() < pos_z);
					if (dest_z == pos_z
						|| (dest_z > pos_z
							&& _walkCam->getViewLevel() < dest_z))
					{
						//Log(LOG_INFO) << ". . setViewLevel(pos_z)";
						_walkCam->setViewLevel(pos_z);
					}
				}
			}
		}
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
		&& _parent->getPanicHandled() == true)
	{
		if (_preStepTurn == true)
		{
			_unit->spendTimeUnits(_preStepCost);

			_preStepCost = 0;
			_preStepTurn = false;
		}

		_pf->abortPath();
	}
}

/**
 * This begins unit movement. And may end unit movement.
 * Called from think()...
 * @return, true to continue moving, false to exit think()
 */
bool UnitWalkBState::doStatusStand() // private.
{
	//Log(LOG_INFO) << "***** UnitWalkBState::doStatusStand() : " << _unit->getId();
	int dir = _pf->getStartDirection();
	//Log(LOG_INFO) << ". StartDirection = " << dir;

	const Tile* const tile = _parent->getSave()->getTile(_unit->getPosition());
	const bool gravLift = dir >= _pf->DIR_UP // Assumes tops & bottoms of gravLifts always have floors/ceilings.
					   && tile->getMapData(O_FLOOR) != NULL
					   && tile->getMapData(O_FLOOR)->isGravLift();

	setWalkSpeed(gravLift);

	if (dir != -1
		&& _kneelCheck == true				// check if unit is kneeled
		&& _unit->isKneeled() == true		// unit is kneeled
		&& gravLift == false				// not on a gravLift
		&& _pf->getPath().empty() == false)	// not the final tile of path; that is, the unit is actually going to move.
	{
		//Log(LOG_INFO) << ". kneeled, and path Valid";
		_kneelCheck = false;

		if (_parent->kneel(_unit) == true)
		{
			//Log(LOG_INFO) << ". . Stand up";
			_unit->setCache(NULL);
			_parent->getMap()->cacheUnit(_unit);

			if (_terrain->checkReactionFire(_unit) == true) // unit got fired upon - stop.
			{
				_pf->abortPath();
				_parent->popState();
				return false;
			}
		}
		else
		{
			//Log(LOG_INFO) << ". . don't stand: not enough TU";
			_action.result = "STR_NOT_ENOUGH_TIME_UNITS"; // note: redundant w/ kneel() error messages ...

			_pf->abortPath();
			_parent->popState();
			return false;
		}
	}

	if (visForUnits() == true)
	{
		//if (Options::traceAI) { Log(LOG_INFO) << "Uh-oh! Company!"; }
		//Log(LOG_INFO) << "Uh-oh! STATUS_STANDING or PANICKING Company!";
		//if (_unit->getFaction() == FACTION_PLAYER) Log(LOG_INFO) << ". . _newVis = TRUE, postPathProcedures";
		//else if (_unit->getFaction() != FACTION_PLAYER) Log(LOG_INFO) << ". . _newUnitSpotted = TRUE, postPathProcedures";

		if (_unit->getFaction() != FACTION_PLAYER)	// kL, Can civies hideForTurn ?
			_unit->setHiding(false);				// 'cause, clearly we're not hidden now!!1

		_unit->setCache(NULL);					// kL. Calls to cacheUnit() are bogus without setCache(NULL) first...!
		_parent->getMap()->cacheUnit(_unit);	// although _cacheInvalid might be set elsewhere but i doubt it.

		postPathProcedures();
		return false;
	}

	_tileSwitchDone = false;

	//Log(LOG_INFO) << ". getStartDirection() dir = " << dir;

	if (_falling == true)
	{
		dir = _pf->DIR_DOWN;
		//Log(LOG_INFO) << ". . _falling, dir = " << dir;
	}

	if (dir != -1)
	{
		//Log(LOG_INFO) << "enter (dir!=-1) : " << _unit->getId();
//		if (_pf->getStrafeMove() == true
		if (_action.strafe == true
			&& _pf->getPath().empty() == false) // <- don't bother with this if it's the end of movement/ State.
		{
			if (_unit->getGeoscapeSoldier() != NULL
				|| _unit->getUnitRules()->isMechanical() == false)
			{
				//Log(LOG_INFO) << ". STANDING strafeMove, setFaceDirection() -> " << _unit->getDirection();
				_unit->setFaceDirection(_unit->getDirection());
			}
			else
			{
				const int dirStrafe = (_dirStart + 4) % 8;
				_unit->setFaceDirection(dirStrafe);
				//Log(LOG_INFO) << ". STANDING strafeTank, setFaceDirection() -> " << dirStrafe;

				if (_unit->getTurretType() != -1)
				{
					const int turretOffset = _unit->getTurretDirection() - _unit->getDirection();
					_unit->setTurretDirection((turretOffset + dirStrafe) % 8); // might not need modulo there ... not sure. Occuppied w/ other things atm.
					//Log(LOG_INFO) << ". STANDING strafeTank, setTurretDirection() -> " << (turretOffset + dirStrafe);
				}
			}
		}
		//else Log(LOG_INFO) << ". STANDING no strafe.";

		//Log(LOG_INFO) << ". getTUCostPF() & dest";
		Position dest;
		int
			tuCost = _pf->getTUCostPF( // gets tu cost but also sets the destination position.
								_unit->getPosition(),
								dir,
								&dest),
			tuTest,
			staCost;
		//Log(LOG_INFO) << ". tuCost = " << tuCost;

		const Tile* const destTile = _parent->getSave()->getTile(dest);
		// kL_note: should this include neutrals? (ie != FACTION_PLAYER; see also 32tu inflation...)
		if (   destTile != NULL
			&& destTile->getFire() > 0
			&& _unit->getFaction() != FACTION_PLAYER
			&& _unit->getArmor()->getDamageModifier(DT_IN) > 0.f)
		{
			//Log(LOG_INFO) << ". . subtract tu inflation for a fireTile";
			// The TU cost was artificially inflated by 32 points in getTUCostPF
			// so it has to be deflated again here under the same conditions.
			// See: Pathfinding::getTUCostPF(), where TU cost was inflated.
			tuCost -= 32;
			//Log(LOG_INFO) << ". . subtract tu inflation for a fireTile DONE";
		}

		if (_falling == true)
		{
			//Log(LOG_INFO) << ". . falling, set tuCost 0";
			tuCost =
			tuTest =
			staCost = 0;
		}
//		const int tuTest = tuCost;
//		int staCost = tuCost;
		else //if (_falling == false)
		{
			tuTest =
			staCost = tuCost;

			if (gravLift == false)
			{
				if (_action.dash == true // allow dash when moving vertically 1 tile (or more).
					|| (_action.strafe == true
						&& dir >= _pf->DIR_UP))
				{
					//Log(LOG_INFO) << ". . dash OR up/down-strafe, tuCost[0] = " << tuCost;
					tuCost -= _pf->getOpenDoor();
					//Log(LOG_INFO) << ". . dash OR up/down-strafe, tuCost[1] = " << tuCost;
					tuCost = (tuCost * 3 / 4) + _pf->getOpenDoor();
					//Log(LOG_INFO) << ". . dash OR up/down-strafe, tuCost[2] = " << tuCost;

					staCost -= _pf->getOpenDoor();
					staCost = staCost * 3 / 2;
				}

				staCost -= _unit->getArmor()->getAgility();
				if (staCost < 0) staCost = 0;
			}
			else // gravLift
			{
				//Log(LOG_INFO) << ". . using GravLift";
				staCost = 0;
			}
		}

		//Log(LOG_INFO) << ". check tuCost + stamina, etc. TU = " << tuCost;
		//Log(LOG_INFO) << ". unit->TU = " << _unit->getTimeUnits();
//		if (tuCost > _unit->getTimeUnits())
		if (tuCost - _pf->getOpenDoor() > _unit->getTimeUnits())
		{
			//Log(LOG_INFO) << ". . tuCost > _unit->TU()";
			if (_parent->getPanicHandled() == true
				&& tuTest < 255)
			{
				//Log(LOG_INFO) << ". send warning: not enough TU";
				_action.result = "STR_NOT_ENOUGH_TIME_UNITS";
			}

			_unit->setCache(NULL);
			_parent->getMap()->cacheUnit(_unit);

			_pf->abortPath();
			_parent->popState();

			return false;
		}
		else if (staCost > _unit->getEnergy())
		{
			//Log(LOG_INFO) << ". . staCost > _unit->getEnergy()";
			if (_parent->getPanicHandled() == true)
				_action.result = "STR_NOT_ENOUGH_ENERGY";

			_unit->setCache(NULL);
			_parent->getMap()->cacheUnit(_unit);

			_pf->abortPath();
			_parent->popState();

			return false;
		}
		else if (_parent->getPanicHandled() == true
			&& _parent->checkReservedTU(
									_unit,
									tuCost) == false)
		{
			//Log(LOG_INFO) << ". . checkReservedTU(_unit, tuCost) == false";
			_unit->setCache(NULL);
			_parent->getMap()->cacheUnit(_unit);

			_pf->abortPath();

			return false;
		}
		// unit is looking in the wrong way so turn first - unless strafe
		// Do not use the turn state because turning during walking doesn't cost TU.
		else if (dir != _unit->getDirection()
			&& dir < _pf->DIR_UP
			&& _action.strafe == false)
//			&& _pf->getStrafeMove() == false)
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
			int sound = -1;
			const int door = _terrain->unitOpensDoor(
												_unit,
												false,
												dir);

			if (door == 0) // normal door
			{
				//Log(LOG_INFO) << ". . . door #0";
				sound = ResourcePack::DOOR_OPEN;
			}
			else if (door == 1) // ufo door
			{
				//Log(LOG_INFO) << ". . . door #1";
				sound = ResourcePack::SLIDING_DOOR_OPEN;
			}
			else if (door == 3) // ufo door still opening ...
			{
				//Log(LOG_INFO) << ". . . door #3 ret FALSE";
				return false; // don't start walking yet, wait for the ufo door to open
			}

			if (sound != -1)
			{
				_parent->getResourcePack()->getSound(
												"BATTLE.CAT",
												sound)
											->play(
												-1,
												_parent->getMap()->getSoundAngle(_unit->getPosition()));

				if (sound == ResourcePack::SLIDING_DOOR_OPEN)
				{
					//Log(LOG_INFO) << ". . . door #1 ret FALSE";
					return false; // don't start walking yet, wait for the ufo door to open
				}
			}
			//Log(LOG_INFO) << ". door DONE";
		}

		// proxy blows up in face after door opens - copied doStatusStand_end()
		if (_parent->checkForProximityGrenades(_unit) == true) // kL_add: Put checkForSilacoid() here!
		{
			_parent->popState();
//			postPathProcedures(); // .. one or the other i suppose.

			return false;
		}

		//Log(LOG_INFO) << ". check size for obstacles";
		const int unitSize = _unit->getArmor()->getSize() - 1;
		for (int
				x = unitSize;
				x != -1;
				--x)
		{
			for (int
					y = unitSize;
					y != -1;
					--y)
			{
				//Log(LOG_INFO) << ". . check obstacle(unit)";
				const BattleUnit
					* const unitInMyWay = _parent->getSave()->getTile(dest + Position(x,y,0))->getUnit(),
					* unitBelowMyWay = NULL;

				const Tile* const belowDest = _parent->getSave()->getTile(dest + Position(x,y,-1));
				if (belowDest != NULL)
					unitBelowMyWay = belowDest->getUnit();

				// can't walk into units in this tile, or on top of other units sticking their head into this tile
				if (_falling == false
					&& ((unitInMyWay != NULL
							&& unitInMyWay != _unit)
						|| (belowDest != NULL
							&& unitBelowMyWay != NULL
							&& unitBelowMyWay != _unit
							&& unitBelowMyWay->getFloatHeight()
										+ unitBelowMyWay->getHeight()
										- belowDest->getTerrainLevel() >= 24 + 4)))
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

		dir = _pf->dequeuePath(); // now start moving
		//Log(LOG_INFO) << ". dequeuePath() dir[0] = " << dir;

		if (_falling == true)
		{
			//Log(LOG_INFO) << ". . falling, _pf->DIR_DOWN";
			dir = _pf->DIR_DOWN;
		}
		//Log(LOG_INFO) << ". dequeuePath() dir[1] = " << dir;

		if (_unit->spendTimeUnits(tuCost) == true	// These were checked above and don't really need to
			&& _unit->spendEnergy(staCost) == true)	// be checked again here. Only subtract is required. but whatver
		{
			//Log(LOG_INFO) << ". . WalkBState: spend TU & Energy";
			const Tile* const tileBelow = _parent->getSave()->getTile(_unit->getPosition() + Position(0,0,-1));
			//Log(LOG_INFO) << ". . WalkBState: startWalking()";
			_unit->startWalking(
							dir,
							dest,
							tileBelow);
//							_onScreen);

//			if (_unit->getMoveSound() != -1)
//			{
				//Log(LOG_INFO) << "doStatusStand() playSound";
				//Log(LOG_INFO) << ". walkPhase = " << _unit->getWalkPhase();
				//Log(LOG_INFO) << ". pos " << _unit->getPosition();
//				playMovementSound();
//			}
			_preStepTurn = false;
		}

		// make sure the unit sprites are up to date
		// kL_note: This could probably go up under spend tu+energy. but.....
		// And since Status_Stand algorithm doesn't actually draw anything
		// REMARK It.
		//Log(LOG_INFO) << ". EXIT (dir!=-1) : " << _unit->getId();
	}
	else // dir == -1
	{
		//Log(LOG_INFO) << ". unit direction = " << _unit->getDirection();
		//Log(LOG_INFO) << ". . CALL postPathProcedures()";
		postPathProcedures();
		return false;
	}

	return true;
}

/**
 * This continues unit movement.
 * Called from think()...
 * @return, true to continue moving, false to exit think()
 */
bool UnitWalkBState::doStatusWalk() // private.
{
	//Log(LOG_INFO) << "***** UnitWalkBState::doStatusWalk() : " << _unit->getId();
	const Tile* tileBelow;
//	Position pos = _unit->getPosition();

	if (_parent->getSave()->getTile(_unit->getDestination())->getUnit() == NULL  // next tile must be not occupied
		// kL_note: and, if not flying, the position directly below the tile must not be occupied...
		// Had that happen with a sectoid left standing in the air because a cyberdisc was 2 levels below it.
		// btw, these have probably been already checked...
		|| _parent->getSave()->getTile(_unit->getDestination())->getUnit() == _unit)
	{
		//Log(LOG_INFO) << ". WalkBState, keepWalking()";
//		if (_unit->getMoveSound() == -1)
		playMovementSound();

//		bool onScreenBoundary = (_unit->getVisible() && _parent->getMap()->getCamera()->isOnScreen(_unit->getPosition(), true, size, true));
//		_unit->keepWalking(tileBelow, onScreenBoundary); // advances the phase
		tileBelow = _parent->getSave()->getTile(_unit->getPosition() + Position(0,0,-1));
		_unit->keepWalking( // advances _walkPhase
						tileBelow,
						_onScreen);
	}
	else if (_falling == false)
	{
		//Log(LOG_INFO) << ". WalkBState, !falling Abort path";
		_unit->lookAt( // turn to blocking unit
				_unit->getDestination(),
				_unit->getTurretType() != -1);

		_pf->abortPath();
		_unit->setStatus(STATUS_STANDING);
	}

	//Log(LOG_INFO) << ". . unitPos " << _unit->getPosition();
	// unit moved from one tile to the other, update the tiles & investigate new flooring
	if (_tileSwitchDone == false
		&& _unit->getPosition() != _unit->getLastPosition())
		// ( _pos != _lastpos ) <- set equal at Start, _walkPhase == 0.
		// this clicks over in keepWalking(_walkPhase == middle)
		// rougly _walkPhase == 4
		// when _pos sets equal to _destination.
		// So. How does _falling ever equate TRUE, if it has to be flagged on _walkPhase == 0 only?
	{
//		if (!_parent->getMap()->getCamera()->isOnScreen(_unit->getPosition(), true, size, false) && _unit->getFaction() != FACTION_PLAYER && _unit->getVisible())

		//Log(LOG_INFO) << ". tile switch from _lastpos to _destination";
		// BattleUnit::startWalking() sets _lastpos = _pos, then
		// BattleUnit::keepWalking (_walkPhase == middle) sets _pos = _destination
		_tileSwitchDone = true;

		bool fallCheck = true;
//		Position posSized;

		const int unitSize = _unit->getArmor()->getSize() - 1;
		for (int
				x = unitSize;
				x != -1;
				--x)
		{
			for (int
					y = unitSize;
					y != -1;
					--y)
			{
//				posSized = pos + Position(x,y,0);
				tileBelow = _parent->getSave()->getTile(_unit->getPosition() + Position(x,y,-1));
				if (_parent->getSave()->getTile(_unit->getPosition() + Position(x,y,0))->hasNoFloor(tileBelow) == false) // _unit->getMovementType() == MT_FLY ||
				{
					//Log(LOG_INFO) << ". . . hasFloor ( fallCheck set FALSE )";
					fallCheck = false;
				}

				//Log(LOG_INFO) << ". . remove unit from previous tile";
				_parent->getSave()->getTile(_unit->getLastPosition() + Position(x,y,0))->setUnit(NULL);
			}
		} // -> might move to doStatusStand_end()

		for (int
				x = unitSize;
				x != -1;
				--x)
		{
			for (int
					y = unitSize;
					y != -1;
					--y)
			{
				//Log(LOG_INFO) << ". . set unit on new tile";
//				posSized = pos + Position(x,y,0);
				_parent->getSave()->getTile(_unit->getPosition() + Position(x,y,0))
										->setUnit(
												_unit,
												_parent->getSave()->getTile(_unit->getPosition() + Position(x,y,-1)));
				//Log(LOG_INFO) << ". . . NEW unitPos " << _unit->getPosition();

//				Tile* const nextTile = _parent->getSave()->getTile(_unit->getPosition() + Position(x,y,0));
//				nextTile->setUnit(
//								_unit,
//								_parent->getSave()->getTile(_unit->getPosition() + Position(x,y,-1)));

//				if (_unit->getFaction() == FACTION_PLAYER)	// This ensures that a unit getting hit by RF will be in a discovered/visible Tile.
//				{											// ... although CalcFOV should handle it, the latter has problems especially on top-level tiling.
//					nextTile->setTileVisible();				// ->fixed. 'void' tiles were not being saved to file as 'discovered'
//					nextTile->setDiscovered(true, 2);
//				}
			}
		}

		_falling = fallCheck
				&& _unit->getPosition().z != 0
				&& _pf->getMoveTypePathing() != MT_FLY;

		if (_falling == true)
		{
			//Log(LOG_INFO) << ". falling";
//			pos = _unit->getPosition();

			for (int
					x = unitSize;
					x != -1;
					--x)
			{
				for (int
						y = unitSize;
						y != -1;
						--y)
				{
					tileBelow = _parent->getSave()->getTile(_unit->getPosition() + Position(x,y,-1));
					//if (tileBelow) Log(LOG_INFO) << ". . otherTileBelow exists";
					if (tileBelow != NULL
						&& tileBelow->getUnit() != NULL)
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
 * @return, true to continue moving, false to exit think()
 */
bool UnitWalkBState::doStatusStand_end() // private.
{
	//Log(LOG_INFO) << "***** UnitWalkBState::doStatusStand_end() : " << _unit->getId();
//	_tileSwitchDone = false;

	if (_unit->getFaction() != FACTION_PLAYER)
		_unit->setUnitVisible(false);
	else
	{
		const BattlescapeState* const battleState = _parent->getSave()->getBattleState();

		double stat = static_cast<double>(_unit->getBaseStats()->tu);
		const int tu = _unit->getTimeUnits();
		battleState->getTimeUnitsField()->setValue(static_cast<unsigned>(tu));
		battleState->getTimeUnitsBar()->setValue(std::ceil(
											static_cast<double>(tu) / stat * 100.));

		stat = static_cast<double>(_unit->getBaseStats()->stamina);
		const int energy = _unit->getEnergy();
		battleState->getEnergyField()->setValue(static_cast<unsigned>(energy));
		battleState->getEnergyBar()->setValue(std::ceil(
											static_cast<double>(energy) / stat * 100.));
	}

	if (_falling == false
		&& _unit->getSpecialAbility() == SPECAB_BURN) // if the unit burns floortiles, burn floortiles
//		|| _unit->getSpecialAbility() == SPECAB_BURN_AND_EXPLODE)
	{
		// kL_add: Put burnedBySilacoid() here! etc
		_unit->getTile()->ignite(1);

		const Position pos = _unit->getPosition() * Position(16,16,24)
						   + Position(
									8,8,
									-(_unit->getTile()->getTerrainLevel()));
		_parent->getTileEngine()->hit(
									pos,
									_unit->getBaseStats()->strength, // * _unit->getAccuracyModifier(),
									DT_IN,
									_unit);
	}

	_terrain->calculateUnitLighting(); // move our personal lighting with us

	// This needs to be done *before* the calculateFOV(pos)
	// or else any newVis will be marked Visible before
	// visForUnits() catches the new unit that is !Visible.
	bool newVis = visForUnits();

	// This calculates or 'refreshes' the Field of View
	// of all units within maximum distance (20 tiles) of current unit.
	_terrain->calculateFOV(
						_unit->getPosition(),
						true);

	if (_parent->checkForProximityGrenades(_unit) == true) // kL_add: Put checkForSilacoid() here!
	{
		_parent->popState();
		return false;
	}
	else if (newVis == true)
	{
		//if (_unit->getFaction() == FACTION_PLAYER) Log(LOG_INFO) << ". . _newVis = TRUE, Abort path";
		//else if (_unit->getFaction() != FACTION_PLAYER) Log(LOG_INFO) << ". . _newUnitSpotted = TRUE, Abort path";

		_unit->setCache(NULL);
		_parent->getMap()->cacheUnit(_unit);

		_pf->abortPath();
		_parent->popState();

		return false;
	}
	else if (_falling == false) // check for reaction fire
	{
		//Log(LOG_INFO) << ". . WalkBState: NOT falling, checkReactionFire()";

		if (_terrain->checkReactionFire(_unit) == true) // unit got fired upon - stop walking
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

	return true;
}

/**
 * This function turns unit during movement.
 * Called from think()
 */
void UnitWalkBState::doStatusTurn() // private.
{
	//Log(LOG_INFO) << "***** UnitWalkBState::doStatusTurn() : " << _unit->getId();
	if (_preStepTurn)	// turning during walking costs no tu
		++_preStepCost;	// except before the first step.

	_unit->turn();

	_unit->setCache(NULL);
	_parent->getMap()->cacheUnit(_unit);

	// calculateFOV() is unreliable for setting the _newUnitSpotted bool as it
	// can be called from various other places in the code, ie: doors opening
	// (& explosions/terrain destruction) and that messes up the result.
	// kL_note: But let's do it anyway! ps. Fixed
	if (visForUnits() == true)
	{
		if (_preStepTurn == true)
		{
			_unit->spendTimeUnits(_preStepCost);

			_preStepCost = 0;
			_preStepTurn = false;
		}

		//Log(LOG_INFO) << "Egads! STATUS_TURNING reveals new units!!! I must pause!";
		//if (_unit->getFaction() == FACTION_PLAYER) Log(LOG_INFO) << ". . _newVis = TRUE, Abort path, popState";
		//else if (_unit->getFaction() != FACTION_PLAYER) Log(LOG_INFO) << ". . _newUnitSpotted = TRUE, Abort path, popState";

		if (_unit->getFaction() != FACTION_PLAYER)
			_unit->setHiding(false);

		_pf->abortPath();
		_unit->setStatus(STATUS_STANDING);

//		_unit->setCache(NULL);
//		_parent->getMap()->cacheUnit(_unit);

		_parent->popState();
	}
	else if (_parent->getPanicHandled() == true)
		_parent->getBattlescapeState()->refreshVisUnits();
}

/**
 * Handles some calculations when the path is finished.
 */
void UnitWalkBState::postPathProcedures() // private.
{
	//Log(LOG_INFO) << "UnitWalkBState::postPathProcedures(), unit = " << _unit->getId();
//	_tileSwitchDone = false;
	_action.TU = 0;

	if (_unit->getFaction() != FACTION_PLAYER)
	{
		int finalDir = _action.finalFacing;

		if (_action.finalAction == true)
			_unit->dontReselect();

		if (_unit->getCharging() != NULL)
		{
			//Log(LOG_INFO) << ". . charging = TRUE";
			const Position targetPos = _unit->getCharging()->getPosition();
			finalDir = _parent->getTileEngine()->getDirectionTo(
															_unit->getPosition(),
															targetPos);
			// kL_notes (pre-above):
			// put an appropriate facing direction here
			// don't stare at a wall. Get if aggro, face closest xCom op <- might be done somewhere already.
			// Cheat: face closest xCom op based on a percentage (perhaps alien 'value' or rank)
			// cf. void AggroBAIState::setAggroTarget(BattleUnit *unit)
			// and bool TileEngine::calculateFOV(BattleUnit *unit)

			if (_parent->getTileEngine()->validMeleeRange(
													_unit,
													_action.actor->getCharging(),
													finalDir))
			{
				BattleAction action;
				action.actor = _unit;
				action.target = targetPos;
				action.targeting = true;
				action.type = BA_HIT;
				action.weapon = _unit->getMainHandWeapon();
//				action.weapon = _unit->getMeleeWeapon();

// if (action.weapon == NULL)
				const std::string meleeWeapon = _unit->getMeleeWeapon();
				bool instaWeapon = false;

				if (meleeWeapon == "STR_FIST")
					action.weapon = _parent->getFist();
				else if (meleeWeapon.empty() == false)
				{
					bool found = false;

					for (std::vector<BattleItem*>::const_iterator
							i = _unit->getInventory()->begin();
							i != _unit->getInventory()->end();
							++i)
					{
						if ((*i)->getRules()->getType() == meleeWeapon)
						{
							// note this ought be conformed w/ bgen.addAlien equipped items to
							// ensure radical (or standard) BT_MELEE weapons get equipped in hand;
							// but for now just grab the meleeItem wherever it was equipped ...
							found = true;
							action.weapon = *i;

							break;
						}
					}

					if (found == false)
					{
						instaWeapon = true;
						action.weapon = new BattleItem(
													_parent->getRuleset()->getItem(meleeWeapon),
													_parent->getSave()->getNextItemId());
						action.weapon->setOwner(_unit);
					}
				}
				else if (action.weapon != NULL
					&& action.weapon->getRules()->getBattleType() != BT_MELEE
					&& action.weapon->getRules()->getBattleType() != BT_FIREARM)
				{
					action.weapon = NULL;
				}


				_unit->setCharging(NULL);

				if (action.weapon != NULL) // also checked in getActionTUs() & ProjectileFlyBState::init()
				{
					action.TU = _unit->getActionTUs(
												action.type,
												action.weapon);

//					_parent->statePushBack(new MeleeAttackBState(_parent, action));
					_parent->statePushBack(new ProjectileFlyBState(
																_parent,
																action));

					if (instaWeapon == true)
						_parent->getSave()->removeItem(action.weapon);
				}
			}
		}
		else if (_unit->isHiding() == true)
		{
//			finalDir = _unit->getDirection() + 4; // just remove this so I don't have to look at Sectopod arses.
			_unit->setHiding(false);
			_unit->dontReselect();
		}

		if (finalDir == -1)
			finalDir = getFinalDirection();

		if (finalDir != -1)
		{
			_unit->lookAt(finalDir % 8);

			while (_unit->getStatus() == STATUS_TURNING)
			{
				_unit->turn();
				_parent->getTileEngine()->calculateFOV(_unit);
				// kL_note: might need newVis/newUnitSpotted -> abort
			}
		}
	}
	else if (_parent->getPanicHandled() == false) // TODO: set the unit to aggrostate and try to find cover
		_unit->setTimeUnits(0);


	_terrain->calculateUnitLighting();

//	_terrain->calculateFOV(_unit);
	_terrain->calculateFOV( // in case unit opened a door and stopped without doing Status_WALKING
						_unit->getPosition(),
						true);


	_unit->setCache(NULL);
	_parent->getMap()->cacheUnit(_unit);

	if (_falling == false)
	{
		//Log(LOG_INFO) << ". postPath falling FALSE popState";
		_parent->popState();
	}
	//else Log(LOG_INFO) << ". postPath falling TRUE";
}

/**
 * Gets a suitable final facing direction for aLiens.
 * @return, direction to face
 */
int UnitWalkBState::getFinalDirection() const // private.
{
	const int
		diff = static_cast<int>(_parent->getBattlescapeState()->getSavedGame()->getDifficulty()),
		alienRank = _unit->getRankInt();

	if (RNG::percent((diff + 1) * 20 - alienRank * 5) == false)
		return -1;

	const BattleUnit* faceUnit = NULL;

	int testDist = 255;
	for (std::vector<BattleUnit*>::const_iterator
			i = _parent->getSave()->getUnits()->begin();
			i != _parent->getSave()->getUnits()->end();
			++i)
	{
		if ((*i)->getFaction() == FACTION_PLAYER
			&& (*i)->isOut(true, true) == false
			&& (*i)->getExposed() != -1
			&& (*i)->getExposed() <= _unit->getIntelligence())
		{
			const int dist = _parent->getSave()->getTileEngine()->distance(
																	(*i)->getPosition(),
																	_unit->getPosition());
			if (dist < testDist)
			{
				testDist = dist;
				faceUnit = *i;
			}
		}
	}

	if (faceUnit != NULL)
		return _unit->directionTo(faceUnit->getPosition());

	return -1;
}

/**
 * Checks visibility for new opponents.
 * @return, true if a new enemy is spotted
 */
bool UnitWalkBState::visForUnits() const // private.
{
	if (_falling == true
		|| _parent->getPanicHandled() == false)
	{
		return false;
	}

	bool ret = _terrain->calculateFOV(_unit);

	if (_unit->getFaction() != FACTION_PLAYER)
	{
		ret = ret
		   && _unit->getUnitsSpottedThisTurn().size() > _unitsSpotted
		   && _action.desperate == false
		   && _unit->getCharging() == NULL;
	}

	return ret;
}

/**
 * Sets the animation speed of units.
 * @param gravLift - true if moving up/down a gravLift
 */
void UnitWalkBState::setWalkSpeed(bool gravLift) const // private.
{
	int interval;
	if (_unit->getFaction() == FACTION_PLAYER)
	{
		if (_action.dash == true)
			interval = Options::battleXcomSpeed * 2 / 3;
		else
			interval = Options::battleXcomSpeed;
	}
	else
		interval = Options::battleAlienSpeed;

	if (gravLift == true)
		interval = interval * 7 / 4;

	_parent->setStateInterval(static_cast<Uint32>(interval));
}

/**
 * Handles walking/ flying/ other movement sounds.
 */
void UnitWalkBState::playMovementSound() const // private.
{
	if (_unit->getUnitVisible() == false
		&& _parent->getSave()->getDebugMode() == false)
	{
		return;
	}

	int sound = -1;

	if (_unit->getMoveSound() != -1)
	{
		if (_unit->getWalkPhase() == 0)
		{
			if (_unit->getStatus() == STATUS_FLYING
				&& _unit->isFloating() == false
				&& _falling == false)
			{
				sound = 40; // GravLift note: isFloating() might be redundant w/ (_falling=false). See below_
			}
			else
				sound = _unit->getMoveSound();
		}
	}
	else
	{
		if (_unit->getStatus() == STATUS_WALKING)
		{
			const int walkPhase = _unit->getWalkPhase();
			if (walkPhase == 3
				|| walkPhase == 7)
			{
				const Tile
					* const tile = _unit->getTile(),
					* const belowTile = _parent->getSave()->getTile(tile->getPosition() + Position(0,0,-1));

				const int stepSound = tile->getFootstepSound(belowTile);
				if (stepSound > -1)
				{
					if (walkPhase == 3)
						sound = stepSound * 2 + ResourcePack::WALK_OFFSET + 1;
					else //if (walkPhase == 7)
						sound = stepSound * 2 + ResourcePack::WALK_OFFSET;
				}
			}
		}
		else if (_unit->getStatus() == STATUS_FLYING)
		{
			if (_unit->getWalkPhase() == 0)
			{
				if (_falling == false)
				{
					if (_unit->isFloating() == false) // GravLift note: isFloating() might be redundant w/ (_falling=false). See above^
						sound = 40;
					else
					{
						if (_unit->getUnitRules() != NULL
							&& _unit->getUnitRules()->isMechanical() == true)
						{
							sound = ResourcePack::FLYING_SOUND;		// hoverSound flutter
						}
						else
							sound = ResourcePack::FLYING_SOUND_HQ;	// HQ hoverSound
					}
				}
			}
			else if (_unit->getWalkPhase() == 7
				&& groundCheck() == true
				&& (_falling == true
					|| (_unit->isFloating() == true
						&& _pf->getMoveTypePathing() == MT_WALK)))
			{
				sound = ResourcePack::ITEM_DROP; // *thunk*
			}
		}
	}

	if (sound != -1)
		_parent->getResourcePack()->getSound(
										"BATTLE.CAT",
										sound)
									->play(
										-1,
										_parent->getMap()->getSoundAngle(_unit->getPosition()));
}

/**
 * For determining if a flying unit turns flight off at start of movement.
 * @note '_falling' should always be false when this is called in init().
 * @note And unit must be capable of flight for this to be relevant.
 * @note This could get problematic if/when falling onto nonFloors like water
 * and/or if there is another unit on tileBelow.
 */
void UnitWalkBState::doFallCheck() // private.
{
	if (_pf->getMoveTypePathing() == MT_FLY
		|| _unit->getPosition().z == 0
		|| groundCheck() == true)
	{
		return;
	}

	_falling = true;
}

/**
 * Checks if there is ground below when unit is falling.
 * @note Pathfinding already has a function canFallDown() that could be used.
 @return, true if unit hits a Floor
 */
bool UnitWalkBState::groundCheck() const // private.
{
	const Tile* tileBelow;
	Position pos;

	const int unitSize = _unit->getArmor()->getSize() - 1;
	for (int
			x = unitSize;
			x != -1;
			--x)
	{
		for (int
				y = unitSize;
				y != -1;
				--y)
		{
			pos = _unit->getPosition() + Position(x,y,0);
			tileBelow = _parent->getSave()->getTile(pos + Position(0,0,-1));
			if (_parent->getSave()->getTile(pos + Position(0,0,0))
										->hasNoFloor(tileBelow) == false)
			{
				return true;
			}
		}
	}

	return false;
}

}
