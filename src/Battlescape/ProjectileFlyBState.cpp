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

#include "ProjectileFlyBState.h"

//#define _USE_MATH_DEFINES
//#include <cmath>

#include "AlienBAIState.h"
#include "BattlescapeState.h"
#include "Camera.h"
#include "Explosion.h"
#include "ExplosionBState.h"
#include "Map.h"
#include "Pathfinding.h"
#include "Projectile.h"
#include "TileEngine.h"

//#include "../Engine/Options.h"
//#include "../Engine/RNG.h"
#include "../Engine/Sound.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/RuleItem.h"

#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

/**
 * Sets up a ProjectileFlyBState [0] for a unit shooting.
 * @param parent - pointer to the BattlescapeGame
 * @param action - the current BattleAction struct (BattlescapeGame.h)
 */
ProjectileFlyBState::ProjectileFlyBState( // origin is unitTile
		BattlescapeGame* const parent,
		BattleAction action)
	:
		BattleState(
			parent,
			action),
		_battleSave(parent->getBattleSave()),
		_posOrigin(action.actor->getPosition()),
		_originVoxel(-1,-1,-1), // for BL waypoints
		_targetVoxel(-1,-1,-1),
		_unit(NULL),
		_ammo(NULL),
		_prjItem(NULL),
		_prjImpact(VOXEL_FLOOR),
		_prjVector(0,0,-1),
		_initialized(false),
		_targetFloor(false),
		_initUnitAnim(0)
{
	//Log(LOG_INFO) << "ProjectileFlyBState cTor.";
}

/**
 * Sets up a ProjectileFlyBState [1] for a waypoint.
 * @param parent	- pointer to the BattlescapeGame
 * @param action	- the current BattleAction struct (BattlescapeGame.h)
 * @param posOrigin	- origin Position in tile-space
 */
ProjectileFlyBState::ProjectileFlyBState( // blaster launch, BattlescapeGame::launchAction()
		BattlescapeGame* const parent,
		BattleAction action,
		Position posOrigin)
	:
		BattleState(
			parent,
			action),
		_battleSave(parent->getBattleSave()),
		_posOrigin(posOrigin),
		_originVoxel(-1,-1,-1), // for BL waypoints
		_targetVoxel(-1,-1,-1),
		_unit(NULL),
		_ammo(NULL),
		_prjItem(NULL),
		_prjImpact(VOXEL_FLOOR),
		_prjVector(0,0,-1),
		_initialized(false),
		_targetFloor(false),
		_initUnitAnim(0)
{}

/**
 * Deletes the ProjectileFlyBState.
 */
ProjectileFlyBState::~ProjectileFlyBState()
{
	_parent->setStateInterval(BattlescapeState::STATE_INTERVAL_STANDARD); // kL
}

/**
 * Initializes the sequence:
 * - checks if the shot is valid
 * - determines the target voxel
 */
void ProjectileFlyBState::init()
{
	//Log(LOG_INFO) << "ProjectileFlyBState::init()";
	if (_initialized == true)
		return;

	//Log(LOG_INFO) << "projFlyB init() targetPosTile = " << _action.target;
	_initialized = true;
	_parent->getCurrentAction()->takenXp = false;

	_unit = _action.actor;

	bool popThis = false;

	if (_unit->isOut_t() == true
		|| _action.weapon == NULL
		|| _battleSave->getTile(_action.target) == NULL)
	{
		popThis = true;
	}
	else
	{
		if (_unit->getFaction() == FACTION_PLAYER
			&& _parent->getPanicHandled() == true
			&& _action.type != BA_HIT
			&& _unit->getTimeUnits() < _action.TU)
		{
			_action.result = "STR_NOT_ENOUGH_TIME_UNITS";
			popThis = true;
		}
		else
		{
			_ammo = _action.weapon->getAmmoItem();
			bool fireValid;

			if (_unit->getFaction() != _battleSave->getSide())
			{
				const BattleUnit* const targetUnit = _battleSave->getTile(_action.target)->getUnit();
				fireValid = targetUnit != NULL
						 && targetUnit->isOut_t() == false
						 && targetUnit == _battleSave->getSelectedUnit()
						 && _ammo != NULL;
			}
			else
				fireValid = true;

			if (fireValid == false
				|| _unit->getStopShot() == true)
			{
				_unit->setTimeUnits(_unit->getTimeUnits() + _action.TU);
				popThis = true;
			}
		}
	}

	if (popThis == true)
	{
		_unit->setStopShot(false);
		_parent->popState();

		return;
	}


	// autoshot will default back to snapshot if it's not possible
	// kL_note: This shouldn't happen w/ selectFireMethod() properly in place.
	if (_action.type == BA_AUTOSHOT
		&& _action.weapon->getRules()->getAccuracyAuto() == 0)
	{
		_action.type = BA_SNAPSHOT;
	}
	// Except that Berserk tries to use SnapShot .... needs looking at.


	// snapshot defaults to "hit" if it's a melee weapon (in case of reaction
	// with a melee weapon) for Silacoid attack etc.
	if (_action.weapon->getRules()->getBattleType() == BT_MELEE
		&& (_action.type == BA_SNAPSHOT
			|| _action.type == BA_AUTOSHOT
			|| _action.type == BA_AIMEDSHOT))
	{
		//Log(LOG_INFO) << ". convert shotType to BA_HIT";
		_action.type = BA_HIT;
	}

	switch (_action.type)
	{
		case BA_SNAPSHOT:
		case BA_AUTOSHOT:
		case BA_AIMEDSHOT:
		case BA_LAUNCH:
			//Log(LOG_INFO) << ". . BA_SNAPSHOT, AIMEDSHOT, AUTOSHOT, or LAUNCH";
			if (_ammo == NULL)
			{
				//Log(LOG_INFO) << ". . . no ammo, EXIT";
				_action.result = "STR_NO_AMMUNITION_LOADED";
				popThis = true;
			}
			else if (_ammo->getAmmoQuantity() == 0)
			{
				//Log(LOG_INFO) << ". . . no ammo Quantity, EXIT";
				_action.result = "STR_NO_ROUNDS_LEFT";
				popThis = true;
			}
			else if (_parent->getTileEngine()->distance(
													_unit->getPosition(),
													_action.target) > _action.weapon->getRules()->getMaxRange())
			{
				//Log(LOG_INFO) << ". . . out of range, EXIT";
				_action.result = "STR_OUT_OF_RANGE";
				popThis = true;
			}
		break;

		case BA_THROW:
		{
			//Log(LOG_INFO) << ". . BA_THROW panic = " << (int)(_parent->getPanicHandled() == false);
			const Tile* const tileTarget = _battleSave->getTile(_action.target); // always Valid.
			if (validThrowRange(
							&_action,
							_parent->getTileEngine()->getOriginVoxel(_action),
							tileTarget) == false)
			{
				//Log(LOG_INFO) << ". . . not valid throw range, EXIT";
				_action.result = "STR_OUT_OF_RANGE";
				popThis = true;
			}
			else
			{
				_prjItem = _action.weapon;
				if (tileTarget->getTerrainLevel() == -24
					&& tileTarget->getPosition().z < _battleSave->getMapSizeZ() - 1)
				{
					++_action.target.z;
				}
			}
		}
		break;

		case BA_HIT:
			performMeleeAttack();
			//Log(LOG_INFO) << ". . BA_HIT performMeleeAttack() DONE - EXIT flyBState::init()";
		return;

		case BA_PSIPANIC:
		case BA_PSICONTROL:
		case BA_PSICONFUSE:
		case BA_PSICOURAGE:
			//Log(LOG_INFO) << ". . BA_PSIPANIC/CONTROL/CONFUSE/COURAGE, new ExplosionBState - EXIT flyBState::init()";
			_parent->statePushFront(new ExplosionBState(
													_parent,
													Position(
															_action.target.x * 16 + 8,
															_action.target.y * 16 + 8,
															_action.target.z * 24 + 10),
													_action.weapon,
													_unit));
		return;

		default:
			//Log(LOG_INFO) << ". . default, EXIT";
			popThis = true;
	}

	if (popThis == true)
	{
		_parent->popState();
		return;
	}


	// ** find TARGET voxel ** ->
	const Tile* const tileTarget = _battleSave->getTile(_action.target);

	if (_action.type == BA_THROW
		|| _action.type == BA_LAUNCH)
	{
		//Log(LOG_INFO) << "projFlyB init() B-Launch OR Throw";
		_targetVoxel = Position(
							_action.target.x * 16 + 8,
							_action.target.y * 16 + 8,
							_action.target.z * 24);

		if (_action.type == BA_THROW)
			_targetVoxel.z -= tileTarget->getTerrainLevel();
		else if (_targetFloor == false)
			_targetVoxel.z += 16;
	}
	else if ((_unit->getFaction() == FACTION_PLAYER	// force fire at center of Tile by pressing [CTRL] but *not* SHIFT
			&& (SDL_GetModState() & KMOD_CTRL) != 0	// force fire at Floor w/ [CTRL+ALT]
			&& (SDL_GetModState() & KMOD_SHIFT) == 0
			&& Options::battleForceFire == true)
		|| _parent->getPanicHandled() == false) // note that nonPlayer berserk bypasses this and targets according to targetUnit OR tileParts below_
	{
		//Log(LOG_INFO) << "projFlyB init() Player panic OR Ctrl [!Shift]";
		_targetVoxel = Position(
							_action.target.x * 16 + 8,
							_action.target.y * 16 + 8,
							_action.target.z * 24); // force fire at floor w/ Alt

		if ((SDL_GetModState() & KMOD_ALT) == 0
			|| _parent->getPanicHandled() == false)
		{
			_targetVoxel.z += 10;
		}
	}
	else
	{
		// determine the target voxel.
		// aim at (in this priority)
		//		- the center of the targetUnit, or the floor if target=origin
		//		- the object
		//		- the northwall
		//		- the westwall
		//		- the floor
		// if there is no LoF to the center canTarget*() tries moving the target-voxel outward.
		// Store that voxel.
		//
		// Force Fire keyboard modifiers:
		// note non-Player units cannot target tileParts ... but they might someday.
		// none			- See above^
		// CTRL			- center
		// CTRL+ALT		- floor
		// SHIFT		- northwall
		// SHIFT+CTRL	- westwall
		const Position originVoxel = _parent->getTileEngine()->getOriginVoxel(
																		_action,
																		_battleSave->getTile(_posOrigin));
		if (tileTarget->getUnit() != NULL
			&& (_unit->getFaction() != FACTION_PLAYER
				|| (((SDL_GetModState() & KMOD_SHIFT) == 0
						&& (SDL_GetModState() & KMOD_CTRL) == 0)
					|| Options::battleForceFire == false)))
		{
			//Log(LOG_INFO) << ". tileTarget has unit";
			if (_action.target == _posOrigin
				|| tileTarget->getUnit() == _unit)
			{
				//Log(LOG_INFO) << "projFlyB targetPos[2] = " << _action.target;
				_targetVoxel = Position( // don't shoot yourself but shoot at the floor
									_action.target.x * 16 + 8,
									_action.target.y * 16 + 8,
									_action.target.z * 24);
				//Log(LOG_INFO) << "projFlyB targetVoxel[2] = " << _targetVoxel;
			}
			else
				_parent->getTileEngine()->canTargetUnit( // <- this is a normal shot by xCom or aLiens.
													&originVoxel,
													tileTarget,
													&_targetVoxel,
													_unit);
		}
		else if (tileTarget->getMapData(O_OBJECT) != NULL	// force vs. Object by using CTRL above^
			&& (_unit->getFaction() != FACTION_PLAYER		// bypass Object by pressing SHIFT
				|| (SDL_GetModState() & KMOD_SHIFT) == 0
				|| Options::battleForceFire == false))
		{
			//Log(LOG_INFO) << ". tileTarget has content-object";
			if (_parent->getTileEngine()->canTargetTile(
													&originVoxel,
													tileTarget,
													O_OBJECT,
													&_targetVoxel,
													_unit) == false)
			{
				_targetVoxel = Position(
									_action.target.x * 16 + 8,
									_action.target.y * 16 + 8,
									_action.target.z * 24 + 10);
			}
		}
		else if (tileTarget->getMapData(O_NORTHWALL) != NULL // force Northwall by pressing [SHIFT] but not CTRL
			&& (_unit->getFaction() != FACTION_PLAYER
				|| (SDL_GetModState() & KMOD_CTRL) == 0
				|| Options::battleForceFire == false))
		{
			//Log(LOG_INFO) << ". tileTarget has northwall";
			if (_parent->getTileEngine()->canTargetTile(
													&originVoxel,
													tileTarget,
													O_NORTHWALL,
													&_targetVoxel,
													_unit) == false)
			{
				_targetVoxel = Position(
									_action.target.x * 16 + 8,
									_action.target.y * 16,
									_action.target.z * 24 + 10);
			}
		}
		else if (tileTarget->getMapData(O_WESTWALL) != NULL) // force Westwall by pressing [CTRL+SHIFT]
		{
			//Log(LOG_INFO) << ". tileTarget has westwall";
			if (_parent->getTileEngine()->canTargetTile(
													&originVoxel,
													tileTarget,
													O_WESTWALL,
													&_targetVoxel,
													_unit) == false)
			{
				_targetVoxel = Position(
									_action.target.x * 16,
									_action.target.y * 16 + 8,
									_action.target.z * 24 + 10);
			}
		}
		else if (tileTarget->getMapData(O_FLOOR) != NULL) // forced-shot at Floor is handled above^ [CTRL+ALT]
		{
			//Log(LOG_INFO) << ". tileTarget has floor";
			if (_parent->getTileEngine()->canTargetTile(
													&originVoxel,
													tileTarget,
													O_FLOOR,
													&_targetVoxel,
													_unit) == false)
			{
				_targetVoxel = Position(
									_action.target.x * 16 + 8,
									_action.target.y * 16 + 8,
									_action.target.z * 24 + 2);
			}
		}
		else // target nothing, targets the middle of the tile
		{
			//Log(LOG_INFO) << ". tileTarget is void";
			_targetVoxel = Position(
								_action.target.x * 16 + 8,
								_action.target.y * 16 + 8,
								_action.target.z * 24 + 10);
		}
	}

	//Log(LOG_INFO) << "projFlyB init() targetPosVoxel.x = " << static_cast<float>(_targetVoxel.x) / 16.f;
	//Log(LOG_INFO) << "projFlyB init() targetPosVoxel.y = " << static_cast<float>(_targetVoxel.y) / 16.f;
	//Log(LOG_INFO) << "projFlyB init() targetPosVoxel.z = " << static_cast<float>(_targetVoxel.z) / 24.f;

	if (createNewProjectile() == true)
	{
		_parent->getMap()->setCursorType(CT_NONE); // might be already done in primaryAction()
		_parent->getMap()->getCamera()->stopMouseScrolling();
	}
	//Log(LOG_INFO) << "ProjectileFlyBState::init() EXIT";
}

/**
 * Tries to create a projectile sprite and add it to the map.
 * Calculate its trajectory.
 * @return, true if the projectile was successfully created
 */
bool ProjectileFlyBState::createNewProjectile() // private.
{
	//Log(LOG_INFO) << "ProjectileFlyBState::createNewProjectile()";
	//Log(LOG_INFO) << ". _action_type = " << _action.type;
	++_action.autoShotCount;

	if (_unit->getGeoscapeSoldier() != NULL
		&& (_action.type == BA_SNAPSHOT
			|| _action.type == BA_AUTOSHOT
			|| _action.type == BA_AIMEDSHOT))
	{
		++_unit->getStatistics()->shotsFiredCounter;
	}

	//Log(LOG_INFO) << "projFlyB create() originTile = " << _posOrigin;
	//Log(LOG_INFO) << "projFlyB create() targetVoxel = " << _targetVoxel;
	//Log(LOG_INFO) << "projFlyB create() targetVoxel.x = " << static_cast<float>(_targetVoxel.x) / 16.f;
	//Log(LOG_INFO) << "projFlyB create() targetVoxel.y = " << static_cast<float>(_targetVoxel.y) / 16.f;
	//Log(LOG_INFO) << "projFlyB create() targetVoxel.z = " << static_cast<float>(_targetVoxel.z) / 24.f;

	Projectile* const prj = new Projectile(
										_parent->getResourcePack(),
										_battleSave,
										_action,
										_posOrigin,
										_targetVoxel);

	_parent->getMap()->setProjectile(prj); // add projectile to Map.


	_parent->setStateInterval(16); // set the speed of the state think cycle to 16 ms (roughly one think-cycle per frame)
	int soundId = -1;

	_prjImpact = VOXEL_EMPTY; // let it calculate a trajectory

	if (_action.type == BA_THROW)
	{
		_prjImpact = prj->calculateThrow(_unit->getAccuracy(_action)); // this should probly be TE:validateThrow() - cf. else(error) below_
		//Log(LOG_INFO) << ". BA_THROW, part = " << (int)_prjImpact;

		if (_prjImpact == VOXEL_FLOOR
			|| _prjImpact == VOXEL_UNIT
			|| _prjImpact == VOXEL_OBJECT)
		{
			if (_unit->getFaction() != FACTION_PLAYER
				&& _prjItem->getRules()->isGrenade() == true)
			{
				//Log(LOG_INFO) << ". . auto-prime for AI, unitID " << _unit->getId();
				_prjItem->setFuse(0);
			}

			_prjItem->moveToOwner(NULL);
			_unit->setCache(NULL);
			_parent->getMap()->cacheUnit(_unit);

			soundId = ResourcePack::ITEM_THROW;

			if (_unit->getGeoscapeSoldier() != NULL
				&& _unit->getFaction() == _unit->getOriginalFaction()
				&& _parent->getPanicHandled() == true)
			{
				_unit->addThrowingExp();
			}
		}
		else // unable to throw here; Note that BattleUnit accuracy^ should *not* be considered before this. Unless this is some sort of failsafe/exploit for the AI ...
		{
			//Log(LOG_INFO) << ". . no throw, Voxel_Empty or _Wall or _OutofBounds";
			delete prj;
			_parent->getMap()->setProjectile(NULL);

			_action.result = "STR_UNABLE_TO_THROW_HERE";
			_action.TU = 0;
			_unit->setUnitStatus(STATUS_STANDING);

			_parent->popState();
			return false;
		}
	}
	else if (_action.weapon->getRules()->getArcingShot() == true) // special code for the "spit" trajectory
	{
		_prjImpact = prj->calculateThrow(_unit->getAccuracy(_action)); // this should probly be TE:validateThrow() - cf. else(error) below_
		//Log(LOG_INFO) << ". acid spit, part = " << (int)_prjImpact;

		if (_prjImpact != VOXEL_EMPTY
			 && _prjImpact != VOXEL_OUTOFBOUNDS)
		{
			//Log(LOG_INFO) << ". . spit/ arcing shot";
			if (_prjImpact == VOXEL_OBJECT
				&& _ammo->getRules()->getExplosionRadius() > 0)
			{
				const Tile* const tile = _battleSave->getTile(_parent->getMap()->getProjectile()->getFinalPosition());
				if (tile->getMapData(O_OBJECT)->getBigWall() == BIGWALL_NESW
					|| tile->getMapData(O_OBJECT)->getBigWall() == BIGWALL_NWSE)
				{
//					prj->storeProjectileDirection();		// kL: used to handle explosions against diagonal bigWalls.
					_prjVector = prj->getStrikeVector();	// ^supercedes above^ storeProjectileDirection()
				}
			}

			_unit->startAiming();
			_unit->setCache(NULL);
			_parent->getMap()->cacheUnit(_unit);

			// lift-off
			soundId = _ammo->getRules()->getFireSound();
			if (soundId == -1)
				soundId = _action.weapon->getRules()->getFireSound();

			if (_action.type != BA_LAUNCH)
				_ammo->spendBullet(
								*_battleSave,
								*_action.weapon);
		}
		else // no line of fire; Note that BattleUnit accuracy^ should *not* be considered before this. Unless this is some sort of failsafe/exploit for the AI ...
		{
			//Log(LOG_INFO) << ". . no spit, no LoF, Voxel_Empty";
			delete prj;

			_parent->getMap()->setProjectile(NULL);

			_action.result = "STR_NO_LINE_OF_FIRE";
			_action.TU = 0;
			_unit->setUnitStatus(STATUS_STANDING);

			_parent->popState();
			return false;
		}
	}
	else // shoot weapon
	{
		if (_originVoxel != Position(-1,-1,-1)) // here, origin is a BL waypoint
		{
			_prjImpact = prj->calculateShot( // this should probly be TE:plotLine() - cf. else(error) below_
										_unit->getAccuracy(_action),
										_originVoxel);
			//Log(LOG_INFO) << ". shoot weapon[0], voxelType = " << (int)_prjImpact;
		}
		else // this is non-BL weapon shooting
		{
			_prjImpact = prj->calculateShot(_unit->getAccuracy(_action)); // this should probly be TE:plotLine() - cf. else(error) below_
			//Log(LOG_INFO) << ". shoot weapon[1], voxelType = " << (int)_prjImpact;
			//Log(LOG_INFO) << "prjFlyB accuracy = " << _unit->getAccuracy(_action);
		}
		//Log(LOG_INFO) << ". shoot weapon, voxelType = " << (int)_prjImpact;
		//Log(LOG_INFO) << ". finalTarget = " << prj->getFinalPosition();

		if (_prjImpact != VOXEL_EMPTY
			|| _action.type == BA_LAUNCH)
		{
			//Log(LOG_INFO) << ". . _prjImpact AIM";
			if (_prjImpact == VOXEL_OBJECT
				&& _ammo->getRules()->getExplosionRadius() > 0)
			{
				const Tile* const tile = _battleSave->getTile(_parent->getMap()->getProjectile()->getFinalPosition());
				if (tile->getMapData(O_OBJECT)->getBigWall() == BIGWALL_NESW
					|| tile->getMapData(O_OBJECT)->getBigWall() == BIGWALL_NWSE)
				{
//					prj->storeProjectileDirection();		// kL: used to handle explosions against diagonal bigWalls.
					_prjVector = prj->getStrikeVector();	// ^supercedes above^ storeProjectileDirection()
				}
			}

			if (_action.type == BA_LAUNCH)
				_parent->getMap()->setWaypointAction(); // reveal the Map until waypoint action completes.

			_unit->aim();
			_unit->setCache(NULL);
			_parent->getMap()->cacheUnit(_unit);

			// lift-off
			soundId = _ammo->getRules()->getFireSound();
			if (soundId == -1)
				soundId = _action.weapon->getRules()->getFireSound();

			if (_action.type != BA_LAUNCH)
				_ammo->spendBullet(
								*_battleSave,
								*_action.weapon);
		}
		else // VOXEL_EMPTY, no line of fire; Note that BattleUnit accuracy^ should *not* be considered before this. Unless this is some sort of failsafe/exploit for the AI ...
		{
			//Log(LOG_INFO) << ". no shot, no LoF, Voxel_Empty";
			delete prj;

			_parent->getMap()->setProjectile(NULL);

			_action.result = "STR_NO_LINE_OF_FIRE";
			_action.TU = 0;
			_unit->setUnitStatus(STATUS_STANDING);

			_parent->popState();
			return false;
		}
	}

	if (soundId != -1)
		_parent->getResourcePack()->getSound("BATTLE.CAT", soundId)
									->play(-1, _parent->getMap()->getSoundAngle(_unit->getPosition()));

	if (_unit->getArmor()->getShootFrames() != 0)
		_parent->getMap()->setShowProjectile(false); // postpone showing the Celatid spit-blob till later

	//Log(LOG_INFO) << ". createNewProjectile() ret TRUE";
	return true;
}

/**
 * Animates the projectile as it moves to the next point in its trajectory.
 * @note If the animation is finished the projectile sprite is removed from the
 * battlefield and this state is finished.
 */
void ProjectileFlyBState::think()
{
	//Log(LOG_INFO) << "ProjectileFlyBState::think() " << _unit->getId();
	if (_unit->getUnitStatus() == STATUS_AIMING
		&& _unit->getArmor()->getShootFrames() != 0)
	{
		if (_initUnitAnim == 0)
			_initUnitAnim = 1;

		_unit->keepAiming();

		if (_unit->getAimingPhase() < _unit->getArmor()->getFirePhase())
			return;
	}

	if (_initUnitAnim == 1)
	{
		_initUnitAnim = 2;
		_parent->getMap()->setShowProjectile();
	}

	_battleSave->getBattleState()->clearMouseScrollingState();
	Camera* const camera = _parent->getMap()->getCamera();

	// TODO: refactoring, Store the projectile in this state instead of getting it from the map each time.
	if (_parent->getMap()->getProjectile() == NULL)
	{
		if (_action.type == BA_AUTOSHOT
			&& _action.autoShotCount < _action.weapon->getRules()->getAutoShots()
			&& _unit->isOut_t() == false
//			&& _unit->isOut() == false
			&& _ammo->getAmmoQuantity() != 0
			&& ((_battleSave->getTile(_unit->getPosition()) != NULL
					&& _battleSave->getTile(_unit->getPosition())
						->hasNoFloor(_battleSave->getTile(_unit->getPosition() + Position(0,0,-1))) == false)
				|| _unit->getMoveTypeUnit() == MT_FLY))
		{
			createNewProjectile();

/*			if (_action.cameraPosition.z != -1) // this ought already be in Map draw, as camera follows projectile from barrel of weapon
			{
				camera->setMapOffset(_action.cameraPosition);

//				_parent->getMap()->invalidate();
//				_parent->getMap()->draw();
			} */
		}
		else // think() FINISH.
		{
			//Log(LOG_INFO) << "ProjectileFlyBState::think() -> finish " << _action.actor->getId();
			if (_action.actor->getFaction() != _battleSave->getSide()	// rf -> note that actionActor may not be the actual shooter,
				&& Options::battleSmoothCamera == true)					// but he/she will be on the same Side, doing a reaction shot.
			{
				//Log(LOG_INFO) << "reset Camera for " << _action.actor->getId();
				const std::map<int, Position>* const rfShotList (_battleSave->getTileEngine()->getRfShotList()); // init.
				std::map<int, Position>::const_iterator i = rfShotList->find(_action.actor->getId());

				//for (std::map<int, Position>::const_iterator j = rfShotList->begin(); j != rfShotList->end(); ++j)
				//{ Log(LOG_INFO) << ". . shotList"; Log(LOG_INFO) << ". . " << j->first << " " << j->second; }

				if (i != rfShotList->end()) // note The shotList vector will be cleared in BattlescapeGame::think() after all BattleStates have popped.
				{
					_action.cameraPosition = i->second;
					//Log(LOG_INFO) << ". to " << _action.cameraPosition;
				}
				else
				{
					_action.cameraPosition.z = -1;
					//Log(LOG_INFO) << ". no reset";
				}
			}

			//Log(LOG_INFO) << ". stored cameraPosition " << _action.cameraPosition;
			//Log(LOG_INFO) << ". pauseAfterShot " << (int)camera->getPauseAfterShot();
			if (_action.cameraPosition.z != -1) //&& _action.waypoints.size() < 2)
			{
				//Log(LOG_INFO) << "ProjectileFlyBState::think() FINISH: cameraPosition was Set";
				if (_action.type == BA_THROW // jump screen back to pre-shot position
					|| _action.type == BA_AUTOSHOT
					|| _action.type == BA_SNAPSHOT
					|| _action.type == BA_AIMEDSHOT)
				{
					//Log(LOG_INFO) << "ProjectileFlyBState::think() FINISH: resetting Camera to original pos";
					if (camera->getPauseAfterShot() == true)	// TODO: move 'pauseAfterShot' to the BattleAction struct. done -> but it didn't work; i'm a numby.
//					if (_action.pauseAfterShot == true)			// note that trying to store the camera position in the BattleAction didn't work either ... double numby.
					{
						camera->setPauseAfterShot(false);
						if (_prjImpact != VOXEL_OUTOFBOUNDS)
						{
							//Log(LOG_INFO) << ". . delay - inBounds";
							SDL_Delay(331); // screen-pause when shot hits target before reverting camera to shooter.
						}
						//else Log(LOG_INFO) << ". . final vox OutofBounds - do NOT pause";
					}

					//Log(LOG_INFO) << ". . reset Camera Position " << _action.actor->getId();
					camera->setMapOffset(_action.cameraPosition);
//					_action.cameraPosition = Position(0,0,-1); // reset.

//					_parent->getMap()->draw();
//					_parent->getMap()->invalidate();
				}
			}

			if (_unit->getFaction() == _battleSave->getSide()
				&& _action.type != BA_PSIPANIC
				&& _action.type != BA_PSICONFUSE
				&& _action.type != BA_PSICOURAGE
				&& _action.type != BA_PSICONTROL
				&& _battleSave->getUnitsFalling() == false)
			{
				//Log(LOG_INFO) << "ProjectileFlyBState::think() CALL te::checkReactionFire()";
				//	<< " id-" << _unit->getId()
				//	<< " action.type = " << _action.type
				//	<< " action.TU = " << _action.TU;
				_parent->getTileEngine()->checkReactionFire(		// note: I don't believe that smoke obscuration gets accounted
														_unit,		// for by this call if the current projectile caused cloud.
														_action.TU,	// But that's kinda ok.
														_action.type != BA_HIT);
			}

			//Log(LOG_INFO) << "ProjectileFlyBState::think() current Status = " << (int)_unit->getUnitStatus();
			if (_unit->isOut_t() == false
//				_unit->isOut() == false
				&& _action.type != BA_HIT)
			{
				_unit->setUnitStatus(STATUS_STANDING);
			}
			//Log(LOG_INFO) << "ProjectileFlyBState::think() current Status = " << (int)_unit->getUnitStatus();

//			if (_battleSave->getSide() == FACTION_PLAYER || _battleSave->getDebugMode() == true)
//				_parent->setupCursor(); // <- not yet! Do it in popState() when everything is finished.

			_parent->popState();
		}
	}
	else // projectile VALID in motion -> ! impact !
	{
		//Log(LOG_INFO) << "ProjectileFlyBState::think() -> move Projectile";
		if (_action.type != BA_THROW
			&& _ammo != NULL
			&& _ammo->getRules()->getShotgunPellets() != 0)
		{
			// shotgun pellets move to their terminal location instantly as fast as possible
			_parent->getMap()->getProjectile()->skipTrajectory();
		}

		if (_parent->getMap()->getProjectile()->traceProjectile() == false) // projectile pathing *not* finished
		{
			if (_action.type == BA_THROW)
			{
//				_parent->getMap()->resetCameraSmoothing();
				Position pos = _parent->getMap()->getProjectile()->getPosition(-1);
				pos = Position::toTileSpace(pos);

				if (pos.x > _battleSave->getMapSizeX())
					--pos.x;

				if (pos.y > _battleSave->getMapSizeY())
					--pos.y;

				BattleItem* const item = _parent->getMap()->getProjectile()->getItem();
				if (item->getRules()->getBattleType() == BT_GRENADE
					&& item->getFuse() == 0) //&& Options::battleInstantGrenade == true // -> moved to PrimeGrenadeState (0 cannot be set w/out InstantGrenades)
				{
					_parent->statePushFront(new ExplosionBState( // it's a hot potato set to explode on contact
															_parent,
															_parent->getMap()->getProjectile()->getPosition(-1),
															item,
															_unit));
				}
				else
				{
					_parent->dropItem(pos, item);

					if (_unit->getFaction() == FACTION_HOSTILE
						&& _prjItem->getRules()->getBattleType() == BT_GRENADE)
					{
						_parent->getTileEngine()->setDangerZone(
															pos,
															item->getRules()->getExplosionRadius(),
															_unit);
					}
				}

				_parent->getResourcePack()->getSound("BATTLE.CAT", ResourcePack::ITEM_DROP)
											->play(-1, _parent->getMap()->getSoundAngle(pos));
			}
			else if (_action.type == BA_LAUNCH
				&& _prjImpact == VOXEL_EMPTY
				&& _action.waypoints.size() > 1)
			{
				_posOrigin = _action.waypoints.front();
				_action.waypoints.pop_front();
				_action.target = _action.waypoints.front();

				// launch the next projectile in the waypoint cascade
				ProjectileFlyBState* const wpNext = new ProjectileFlyBState(
																		_parent,
																		_action,
																		_posOrigin); // -> tilePos for BL.
				wpNext->_originVoxel = _parent->getMap()->getProjectile()->getPosition();
//				wpNext->setOriginVoxel(_parent->getMap()->getProjectile()->getPosition()); // !getPosition(-1) -> tada, fixed. // -> voxlPos

				// this follows BL as it hits through waypoints
				camera->centerOnPosition(_posOrigin);

				if (_posOrigin == _action.target)
					wpNext->targetFloor();

				_parent->statePushNext(wpNext);
			}
			else // shoot -> impact.
			{
//				_parent->getMap()->resetCameraSmoothing();
				if (_action.type == BA_LAUNCH) // Launches explode at final waypoint.
				{
					_prjImpact = VOXEL_OBJECT;
					_parent->getMap()->setWaypointAction(false); // reveal the Map until waypoint action completes.
				}

				BattleUnit* const shotAt = _battleSave->getTile(_action.target)->getUnit();
				if (shotAt != NULL // only counts for guns, not throws or launches
					&& shotAt->getGeoscapeSoldier() != NULL)
				{
					++shotAt->getStatistics()->shotAtCounter;
				}

/*				if (_action.type == BA_LAUNCH
					&& _ammo != NULL
					&& _ammo->spendBullet() == false)
				{
					_battleSave->removeItem(_ammo);
					_action.weapon->setAmmoItem(NULL);
				} */
				if (_action.type == BA_LAUNCH
					&& _ammo != NULL)
//					_battleSave->getDebugMode() == false)
				{
					_ammo->spendBullet(
									*_battleSave,
									*_action.weapon);
				}

				if (_prjImpact != VOXEL_OUTOFBOUNDS) // NOT out of map
				{
					// explosions impact not inside the voxel but two steps back;
					// projectiles generally move 2 voxels at a time
					int offset = 0;

					if (_ammo != NULL
						&& _ammo->getRules()->getExplosionRadius() > -1
						&& _prjImpact != VOXEL_UNIT)
					{
						offset = -2; // step back a bit so tileExpl isn't behind a close wall.
					}

					Position voxelExpl = _parent->getMap()->getProjectile()->getPosition(offset);

					Tile* tileTrue;
					if (_prjVector.z != -1)
					{
						const Position pos = Position::toTileSpace(voxelExpl);
						tileTrue = _parent->getBattlescapeState()->getSavedBattleGame()->getTile(pos);
						_parent->getTileEngine()->setTrueTile(tileTrue);

						voxelExpl.x -= _prjVector.x * 16; // note there is no safety on these.
						voxelExpl.y -= _prjVector.y * 16;
					}
					else tileTrue = NULL; // probly not needed.

					//Log(LOG_INFO) << "projFlyB think() new ExplosionBState() voxelExpl " << _parent->getMap()->getProjectile()->getPosition(offset);
					//Log(LOG_INFO) << "projFlyB think() offset " << offset;
					//Log(LOG_INFO) << "projFlyB think() projImpact voxelType " << (int)_prjImpact;
					//Log(LOG_INFO) << "projFlyB think() voxelExpl.x = " << static_cast<float>(_parent->getMap()->getProjectile()->getPosition(offset).x) / 16.f;
					//Log(LOG_INFO) << "projFlyB think() voxelExpl.y = " << static_cast<float>(_parent->getMap()->getProjectile()->getPosition(offset).y) / 16.f;
					//Log(LOG_INFO) << "projFlyB think() voxelExpl.z = " << static_cast<float>(_parent->getMap()->getProjectile()->getPosition(offset).z) / 24.f;
					_parent->statePushFront(new ExplosionBState(
															_parent,
															voxelExpl,
															_ammo,
															_unit,
															NULL,
															_action.type != BA_AUTOSHOT
															|| _action.autoShotCount == _action.weapon->getRules()->getAutoShots()
															|| _action.weapon->getAmmoItem() == NULL));

					// special shotgun behaviour: trace extra projectile paths
					// and add bullet hits at their termination points.
					if (_ammo != NULL)
					{
						const int pellets = _ammo->getRules()->getShotgunPellets();
						if (pellets > 1) // first pellet is done above.
						{
							int i = 1; // one pellet has already been fired above^
							while (i < pellets)
							{
								Projectile* const prj = new Projectile(
																	_parent->getResourcePack(),
																	_battleSave,
																	_action,
																	_posOrigin,
																	_targetVoxel);

								const double accuracy = std::max(0.,
															_unit->getAccuracy(_action) - i * 0.023); // TODO: Create RuleItem variable '_pattern' for this.
								_prjImpact = prj->calculateShot(accuracy); // pellet spread.
								if (_prjImpact != VOXEL_EMPTY)
								{
									prj->skipTrajectory(); // as above: skip the pellet to the end of its path

									if (_prjImpact != VOXEL_OUTOFBOUNDS) // insert an explosion and hit
									{
										Explosion* const expl = new Explosion(
																			prj->getPosition(1),
																			_ammo->getRules()->getHitAnimation());

										_parent->getMap()->getExplosions()->push_back(expl);
										//Log(LOG_INFO) << "ProjectileFlyBState::think() shotHIT = " << (i + 1);
										_battleSave->getTileEngine()->hit(
																		prj->getPosition(1),
																		_ammo->getRules()->getPower(),
																		_ammo->getRules()->getDamageType(),
																		_unit,
																		false,true);
									}
								}

								++i;
								delete prj;
							}
						}
					}

					// Silacoid floorburn was here; moved down to PerformMeleeAttack()

					if (_unit->getOriginalFaction() == FACTION_PLAYER	// This section is only for SoldierDiary mod.
						&& _prjImpact == VOXEL_UNIT)					// see below also; was also for setting aggroState
					{
						BattleUnit* const victim = _battleSave->getTile(
															_parent->getMap()->getProjectile()->getPosition(offset) / Position(16,16,24))
														->getUnit();
						if (victim != NULL) //&& victim->isOut() == false)
						{
							BattleUnitStatistics
								* statsVictim = NULL,
								* statsUnit = NULL;

							if (_unit->getGeoscapeSoldier() != NULL)
								statsUnit = _unit->getStatistics();

							if (victim->getGeoscapeSoldier() != NULL)
								statsVictim = victim->getStatistics();

							if (statsVictim != NULL)
								++statsVictim->hitCounter;

							if (victim->getOriginalFaction() == FACTION_PLAYER
								&& _unit->getOriginalFaction() == FACTION_PLAYER)
							{
								if (statsVictim != NULL)
									++statsVictim->shotByFriendlyCounter;

								if (statsUnit != NULL)
									++statsUnit->shotFriendlyCounter;
							}

							const BattleUnit* const target = _battleSave->getTile(_action.target)->getUnit(); // target (not necessarily who was hit)
							if (target == victim) // hit the target
							{
								if (statsUnit != NULL)
								{
									++statsUnit->shotsLandedCounter;

									const int dist = _parent->getTileEngine()->distance(
																					victim->getPosition(),
																					_unit->getPosition());
									if (dist > 30)
										++statsUnit->longDistanceHitCounter;

									if (dist > static_cast<int>(_unit->getAccuracy(_action) * 35.))
										++statsUnit->lowAccuracyHitCounter;
								}
							}
							// TODO: Lucky Shot counter should go here. For hitting an *unintended* target.
						}
					}

					// ... Let's try something
/*					if (_prjImpact == VOXEL_UNIT)
					{
						BattleUnit* victim = _battleSave->getTile(
																_parent->getMap()->getProjectile()->getPosition(offset) / Position(16, 16, 24))
															->getUnit();
						if (victim
							&& !victim->isOut(true, true)
							&& victim->getOriginalFaction() == FACTION_PLAYER
							&& _unit->getFaction() == FACTION_HOSTILE)
						{
							_unit->setExposed();
						} */ // But this is entirely unnecessary since aLien has already seen and logged the soldier.
/*						if (victim
							&& !victim->isOut(true, true)
							&& victim->getFaction() == FACTION_HOSTILE)
						{
							AlienBAIState* aggro = dynamic_cast<AlienBAIState*>(victim->getCurrentAIState());
							if (aggro != 0)
							{
								aggro->setWasHitBy(_unit);	// is used only for spotting on RA.
								_unit->setExposed();		// might want to remark this! Ok.
								// technically, in the original as I remember it, only
								// a BlasterLaunch (by xCom) would set an xCom soldier Exposed here!
								// Those aLiens had a way of tracing a BL back to its origin ....
								// ... but that's madness.
							}
						}
					} */
				}
				else if (_action.type != BA_AUTOSHOT
					|| _action.autoShotCount == _action.weapon->getRules()->getAutoShots()
					|| _action.weapon->getAmmoItem() == NULL)
				{
					_unit->aim(false);
					_unit->setCache(NULL);
					_parent->getMap()->cacheUnits();
				}
			}

			delete _parent->getMap()->getProjectile();
			_parent->getMap()->setProjectile(NULL);
		}
	}
	//Log(LOG_INFO) << "ProjectileFlyBState::think() EXIT";
}

/**
 * Flying projectiles cannot be cancelled but they can be skipped.
 */
void ProjectileFlyBState::cancel()
{
	Projectile* const projectile = _parent->getMap()->getProjectile();
	if (projectile != NULL)
	{
		projectile->skipTrajectory();

		Camera* const camera = _parent->getMap()->getCamera();
		const Position pos = projectile->getPosition() / Position(16,16,24);
		if (camera->isOnScreen(pos) == false)
			camera->centerOnPosition(pos);
	}
}

/**
 * Validates the throwing range.
 * @param action		- pointer to BattleAction struct (BattlescapeGame.h)
 * @param originVoxel	- reference the origin in voxel-space
 * @param tile			- pointer to the targeted tile
 * @return, true if the range is valid
 */
bool ProjectileFlyBState::validThrowRange( // static.
		const BattleAction* const action,
		const Position& originVoxel,
		const Tile* const tile)
{
	//Log(LOG_INFO) << "ProjectileFlyBState::validThrowRange()";
	const int
		delta_x = action->actor->getPosition().x - action->target.x,
		delta_y = action->actor->getPosition().y - action->target.y;
	const double throwDist = std::sqrt(static_cast<double>((delta_x * delta_x) + (delta_y * delta_y)));

	int weight = action->weapon->getRules()->getWeight();
	if (action->weapon->getAmmoItem() != NULL
		&& action->weapon->getAmmoItem() != action->weapon)
	{
		weight += action->weapon->getAmmoItem()->getRules()->getWeight();
	}

	const int delta_z = originVoxel.z
					  - action->target.z * 24 + tile->getTerrainLevel();
	const double maxDist = static_cast<double>(
						   getMaxThrowDistance( // tile-space
											weight,
											action->actor->getStrength(),
											delta_z)
										+ 8) / 16.;
	// throwing off a building of 1 level lets you throw 2 tiles further than normal range,
	// throwing up to the roof of this building lets you throw 2 tiles less further
/*	int delta_z = action->actor->getPosition().z - action->target.z;
	distance -= static_cast<double>(delta_z); */

	// since getMaxThrowDistance seems to return 1 less than maxDist, use "< throwDist" for this determination:
//	bool ret = static_cast<int>(throwDist) < static_cast<int>(maxDist);
//	const bool ret = throwDist < maxDist;
	//Log(LOG_INFO) << ". throwDist " << (int)throwDist
	//				<< " < maxDist " << (int)maxDist
	//				<< " : return " << ret;
	return (throwDist < maxDist);
}

/**
 * Helper for validThrowRange().
 * @param weight	- the weight of the object
 * @param strength	- the strength of the thrower
 * @param elevation	- the difference in height between the thrower and the target (voxel-space)
 * @return, the maximum throwing range
 */
int ProjectileFlyBState::getMaxThrowDistance( // static.
		int weight,
		int strength,
		int elevation)
{
	//Log(LOG_INFO) << "ProjectileFlyBState::getMaxThrowDistance()";
	double
		z = static_cast<double>(elevation) + 0.5,
		dz = 1.;

	int retDist = 0;
	while (retDist < 4000) // just in case
	{
		retDist += 8;

		if (dz < -1.)
			z -= 8.;
		else
			z += dz * 8.;

		if (z < 0. && dz < 0.) // roll back
		{
			dz = std::max(dz, -1.);
			if (std::abs(dz) > 1e-10) // rollback horizontal
				retDist -= static_cast<int>(z / dz);

			break;
		}

		dz -= static_cast<double>(weight * 50 / strength) / 100.;
		if (dz <= -2.) // become falling
			break;
	}

	//Log(LOG_INFO) << ". retDist = " << retDist / 16;
	return retDist;
}

/**
 * Set the origin voxel.
 * @note Used for the blaster launcher.
 * @param pos - reference the origin voxel
 */
/* void ProjectileFlyBState::setOriginVoxel(const Position& pos) // private.
{
	_originVoxel = pos;
} */

/**
 * Set the boolean flag to angle a blaster bomb towards the floor.
 */
void ProjectileFlyBState::targetFloor() // private.
{
	_targetFloor = true;
}

/**
 * Peforms a melee attack.
 * @note Removed after cosmetic surgery.
 */
void ProjectileFlyBState::performMeleeAttack() // private.
{
	//Log(LOG_INFO) << "flyB:performMeleeAttack() " << _unit->getId();
	_unit->aim();
	_unit->setCache(NULL);
	_parent->getMap()->cacheUnit(_unit);

	_action.target = _battleSave->getTileEngine()->getMeleePosition(_unit);
	//Log(LOG_INFO) << ". target " << _action.target;

	// moved here from ExplosionBState to play a proper hit/miss sFx
	bool success;
	const int pct = static_cast<int>(Round(_unit->getAccuracy(_action) * 100.));
	if (RNG::percent(pct) == true)
		success = true;
	else
		success = false;

	int sound = -1;
	if (success == false)
	{
		if (_ammo->getRules()->getBattleType() == BT_MELEE
			&& _ammo->getRules()->getMeleeSound() != -1		// if there is a hitSound play attackSound, else ITEM_THROW;
			&& _ammo->getRules()->getMeleeHitSound() != -1)	// the hitSound will be used for success.
		{
			sound = _ammo->getRules()->getMeleeSound();
		}
		else
			sound = ResourcePack::ITEM_THROW;
	}
	else
	{
		if (_ammo->getRules()->getBattleType() == BT_MELEE)
		{
			sound = _ammo->getRules()->getMeleeHitSound();
			if (sound == -1)
				sound = _ammo->getRules()->getMeleeSound();
		}
		else
			sound = ResourcePack::ITEM_DROP;
	}

	if (sound != -1)
		_parent->getResourcePack()->getSound("BATTLE.CAT", sound)
									->play(-1, _parent->getMap()->getSoundAngle(_action.target));

	if (_action.weapon->getRules()->getBattleType() == BT_MELEE
		&& _ammo != NULL)
	{
		_ammo->spendBullet(
						*_battleSave,
						*_action.weapon);
	}

	if (_unit->getSpecialAbility() == SPECAB_BURN)
		_battleSave->getTile(_action.target)->ignite(15);

	_parent->getMap()->setCursorType(CT_NONE); // might be already done in primaryAction()


	const BattleUnit* const targetUnit = _battleSave->getTile(_action.target)->getUnit();
	const int height = targetUnit->getHeight() / 2
					 + targetUnit->getFloatHeight()
					 - _battleSave->getTile(_action.target)->getTerrainLevel();

	Position posVoxel;
	_battleSave->getPathfinding()->directionToVector(
												_unit->getDirection(),
												&posVoxel);
	posVoxel = _action.target * Position(16,16,24)
			 + Position(8,8, height) - posVoxel;

	_parent->statePushNext(new ExplosionBState(
											_parent,
											posVoxel,
											_action.weapon,
											_unit,
											NULL,
											true,
											success));
}

}
