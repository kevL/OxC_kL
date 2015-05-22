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

#include "../Engine/Game.h"
//#include "../Engine/Logger.h"
//#include "../Engine/Options.h"
//#include "../Engine/RNG.h"
#include "../Engine/Sound.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/RuleItem.h"

#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

/**
 * Sets up a ProjectileFlyBState [0].
 * @param parent - pointer to the BattlescapeGame
 * @param action - the current BattleAction struct (BattlescapeGame.h)
 * @param origin - an origin Position
 */
ProjectileFlyBState::ProjectileFlyBState(
		BattlescapeGame* parent,
		BattleAction action,
		Position origin)
	:
		BattleState(
			parent,
			action),
		_origin(origin),
		_originVoxel(-1,-1,-1), // for BL waypoints
		_targetVoxel(-1,-1,-1),
		_unit(NULL),
		_ammo(NULL),
		_projectileItem(NULL),
		_projectileImpact(0),
		_initialized(false),
		_targetFloor(false),
		_initUnitAnim(0)
{}

/**
 * Sets up a ProjectileFlyBState [1].
 * @param parent - pointer to the BattlescapeGame
 * @param action - the current BattleAction struct (BattlescapeGame.h)
 */
ProjectileFlyBState::ProjectileFlyBState( // blaster launch, BattlescapeGame::launchAction()
		BattlescapeGame* parent,
		BattleAction action)
	:
		BattleState(
			parent,
			action),
		_origin(action.actor->getPosition()),
		_originVoxel(-1,-1,-1), // for BL waypoints
		_targetVoxel(-1,-1,-1),
		_unit(NULL),
		_ammo(NULL),
		_projectileItem(NULL),
		_projectileImpact(0),
		_initialized(false),
		_targetFloor(false),
		_initUnitAnim(0)
{}

/**
 * Deletes the ProjectileFlyBState.
 */
ProjectileFlyBState::~ProjectileFlyBState()
{
	_parent->setStateInterval(static_cast<Uint32>(BattlescapeState::DEFAULT_ANIM_SPEED)); // kL
}

/**
 * Initializes the sequence:
 * - checks if the shot is valid
 * - calculates the base accuracy
 */
void ProjectileFlyBState::init()
{
	//Log(LOG_INFO) << "ProjectileFlyBState::init()";
	if (_initialized == true)
		return;

	_initialized = true;


	_parent->getCurrentAction()->takenXP = false;

	_unit = _action.actor;

	if (_action.weapon != NULL)
		_ammo = _action.weapon->getAmmoItem(); // the weapon itself if not-req'd. eg, lasers/melee


	const bool targetTileValid = (_parent->getSave()->getTile(_action.target) != NULL);
	bool
		reactionValid = false,
		popThis = false;

	if (targetTileValid == true)
	{
		const BattleUnit* const targetUnit = _parent->getSave()->getTile(_action.target)->getUnit();
		reactionValid = targetUnit != NULL
					 && targetUnit->isOut(true, true) == false
					 && targetUnit == _parent->getSave()->getSelectedUnit()
					 && _ammo != NULL;
	}


	if (_unit->isOut(true, true) == true
		|| _action.weapon == NULL
		|| targetTileValid == false)
	{
		popThis = true;
	}
	else if (_parent->getPanicHandled() == true
		&& _action.type != BA_HIT // done in ActionMenuState -> Exactly. So do NOT re-consider it here.
		&& _unit->getTimeUnits() < _action.TU)
	{
		_action.result = "STR_NOT_ENOUGH_TIME_UNITS";
		popThis = true;
	}
	else if (_unit->getStopShot() == true
		|| (_unit->getFaction() != _parent->getSave()->getSide()
			&& reactionValid == false))
	{
		_unit->setTimeUnits(_unit->getTimeUnits() + _action.TU);
		popThis = true;
	}


	if (popThis == true)
	{
		_unit->setStopShot(false);
		_parent->popState();

		return;
	}


	// removed post-cosmetics
	// autoshot will default back to snapshot if it's not possible
	// kL_note: This shouldn't happen w/ selectFireMethod() properly in place.
	if (_action.type == BA_AUTOSHOT
		&& _action.weapon->getRules()->getAccuracyAuto() == 0)
	{
		_action.type = BA_SNAPSHOT;
	}

	// removed post-cosmetics
	// snapshot defaults to "hit" if it's a melee weapon
	// (in case of reaction "shots" with a melee weapon),
	// for Silacoid attack, etc.
	// kL_note: not sure that melee reactionFire is properly implemented...
	if (_action.weapon->getRules()->getBattleType() == BT_MELEE
		&& (   _action.type == BA_SNAPSHOT
			|| _action.type == BA_AUTOSHOT
			|| _action.type == BA_AIMEDSHOT))
	{
		//Log(LOG_INFO) << ". convert shotType to BA_HIT";
		_action.type = BA_HIT;
	}

	// reaction fire, added post-cosmetics
//		_unit->lookAt(_action.target, _unit->getTurretType() != -1);
//		while (_unit->getStatus() == STATUS_TURNING)
//			_unit->turn();

	const Tile* const destTile = _parent->getSave()->getTile(_action.target);

	switch (_action.type)
	{
		case BA_SNAPSHOT:
		case BA_AIMEDSHOT:
		case BA_AUTOSHOT:
		case BA_LAUNCH:
			//Log(LOG_INFO) << ". . BA_SNAPSHOT, AIMEDSHOT, AUTOSHOT, or LAUNCH";
			if (_ammo == NULL)
			{
				//Log(LOG_INFO) << ". . . no ammo, EXIT";
				_action.result = "STR_NO_AMMUNITION_LOADED";

				_parent->popState();
				return;
			}
			else if (_ammo->getAmmoQuantity() == 0)
			{
				//Log(LOG_INFO) << ". . . no ammo Quantity, EXIT";
				_action.result = "STR_NO_ROUNDS_LEFT";

				_parent->popState();
				return;
			}
			else if ( //_action.weapon->getRules()->getMaxRange() > 0 &&	// in case -1 gets used for infinite, or no shot allowed.
																			// But this is just stupid. RuleItem defaults _maxRange @ 200
				_parent->getTileEngine()->distance(
												_unit->getPosition(),
												_action.target) > _action.weapon->getRules()->getMaxRange())
			{
				//Log(LOG_INFO) << ". . . out of range, EXIT";
				_action.result = "STR_OUT_OF_RANGE";

				_parent->popState();
				return;
			}
		break;

		case BA_THROW:
		{
			//Log(LOG_INFO) << ". . BA_THROW";
			const Position originVoxel = _parent->getTileEngine()->getOriginVoxel(
																			_action,
																			NULL);
			if (validThrowRange(
							&_action,
							originVoxel,
							destTile) == false)
			{
				//Log(LOG_INFO) << ". . . not valid throw range, EXIT";
				_action.result = "STR_OUT_OF_RANGE";

				_parent->popState();
				return;
			}

			if (destTile != NULL
				&& destTile->getTerrainLevel() == -24
				&& destTile->getPosition().z + 1 < _parent->getSave()->getMapSizeZ())
			{
				_action.target.z += 1;
			}

			_projectileItem = _action.weapon;
		}
		break;

		// removed post-cosmetics
		case BA_HIT:
			performMeleeAttack();
			//Log(LOG_INFO) << ". . BA_HIT performMeleeAttack() DONE";
		return;

		// removed post-cosmetics
		case BA_PANIC:
		case BA_MINDCONTROL:
			//Log(LOG_INFO) << ". . BA_PANIC/MINDCONTROL, new ExplosionBState, EXIT";
			_parent->statePushFront(new ExplosionBState(
													_parent,
													Position(
															(_action.target.x * 16) + 8,
															(_action.target.y * 16) + 8,
															(_action.target.z * 24) + 10),
													_action.weapon,
													_unit));
		return;

		default:
			//Log(LOG_INFO) << ". . default, EXIT";
			_parent->popState();
		return;
	}


	if (_action.type == BA_LAUNCH
		|| (_unit->getFaction() == FACTION_PLAYER //_parent->getSave()->getSide() -> note: don't let ReactionFire in here by holding CRTL.
			&& (SDL_GetModState() & KMOD_CTRL) != 0 // force fire by pressing CTRL but not SHIFT
			&& (SDL_GetModState() & KMOD_SHIFT) == 0
			&& Options::forceFire == true)
		|| _parent->getPanicHandled() == false)
	{
		//Log(LOG_INFO) << "projFlyB init() targetPosTile[0] = " << _action.target;
		_targetVoxel = Position( // target nothing, targets the middle of the tile
							_action.target.x * 16 + 8,
							_action.target.y * 16 + 8,
							_action.target.z * 24 + 10);

		if (_action.type == BA_LAUNCH)
		{
			if (_targetFloor == true) // kL_note: was, if(_action.target==_origin)
				_targetVoxel.z -= 10; // launched missiles with two waypoints placed on the same tile: target the floor.
			else
				_targetVoxel.z += 6; // launched missiles go higher than the middle.

			//Log(LOG_INFO) << "projFlyB init() targetPosVoxel[0].x = " << static_cast<float>(_targetVoxel.x) / 16.f;
			//Log(LOG_INFO) << "projFlyB init() targetPosVoxel[0].y = " << static_cast<float>(_targetVoxel.y) / 16.f;
			//Log(LOG_INFO) << "projFlyB init() targetPosVoxel[0].z = " << static_cast<float>(_targetVoxel.z) / 24.f;
		}
		else if ((SDL_GetModState() & KMOD_ALT) != 0) // CTRL+ALT targets floor even if a unit is at targetTile.
		{
			//Log(LOG_INFO) << "projFlyB targetPos[1] = " << _action.target;
			_targetVoxel.z -= 10; // force fire at floor (useful for HE ammo-types)
		}
	}
	else
	{
		// determine the target voxel.
		// aim at (in this priority)
		//		- the center of the targetUnit, or the floor if origin=target
		//		- the content-object
		//		- the north wall
		//		- the west wall
		//		- the floor
		// if there is no LOF to the center try elsewhere (more outward).
		// Store that target voxel.
		const Position originVoxel = _parent->getTileEngine()->getOriginVoxel(
																		_action,
																		_parent->getSave()->getTile(_origin));
		const Tile* const targetTile = _parent->getSave()->getTile(_action.target);

		if (targetTile->getUnit() != NULL)
		{
			//Log(LOG_INFO) << ". targetTile has unit";
			if (_origin == _action.target
				|| targetTile->getUnit() == _unit)
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
													targetTile,
													&_targetVoxel,
													_unit);
		}
		else if (targetTile->getMapData(MapData::O_OBJECT) != NULL	// bypass Content-Object when pressing SHIFT
			&& (SDL_GetModState() & KMOD_SHIFT) == 0)				// force vs. Object by using CTRL above^
		{
			//Log(LOG_INFO) << ". targetTile has content-object";
			if (_parent->getTileEngine()->canTargetTile(
													&originVoxel,
													targetTile,
													MapData::O_OBJECT,
													&_targetVoxel,
													_unit) == false)
			{
				_targetVoxel = Position(
									_action.target.x * 16 + 8,
									_action.target.y * 16 + 8,
									_action.target.z * 24 + 10);
			}
		}
		else if (targetTile->getMapData(MapData::O_NORTHWALL) != NULL	// force Northwall when pressing SHIFT but not CTRL
			&& (SDL_GetModState() & KMOD_CTRL) == 0)
		{
			//Log(LOG_INFO) << ". targetTile has northwall";
			if (_parent->getTileEngine()->canTargetTile(
													&originVoxel,
													targetTile,
													MapData::O_NORTHWALL,
													&_targetVoxel,
													_unit) == false)
			{
				_targetVoxel = Position(
									_action.target.x * 16 + 8,
									_action.target.y * 16,
									_action.target.z * 24 + 10);
			}
		}
		else if (targetTile->getMapData(MapData::O_WESTWALL) != NULL)	// force Westwall when pressing SHIFT+CTRL
		{
			//Log(LOG_INFO) << ". targetTile has westwall";
			if (_parent->getTileEngine()->canTargetTile(
													&originVoxel,
													targetTile,
													MapData::O_WESTWALL,
													&_targetVoxel,
													_unit) == false)
			{
				_targetVoxel = Position(
									_action.target.x * 16,
									_action.target.y * 16 + 8,
									_action.target.z * 24 + 10);
			}
		}
		else if (targetTile->getMapData(MapData::O_FLOOR) != NULL) // CTRL+ALT forced-shot is handled above^
		{
			//Log(LOG_INFO) << ". targetTile has floor";
			if (_parent->getTileEngine()->canTargetTile(
													&originVoxel,
													targetTile,
													MapData::O_FLOOR,
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
			//Log(LOG_INFO) << ". targetTile is void";
			_targetVoxel = Position(
								_action.target.x * 16 + 8,
								_action.target.y * 16 + 8,
								_action.target.z * 24 + 12);
		}
	}

	if (createNewProjectile() == true)
	{
		_parent->getMap()->setCursorType(CT_NONE);
		_parent->getMap()->getCamera()->stopMouseScrolling();
	}
	//Log(LOG_INFO) << "ProjectileFlyBState::init() EXIT";
}

/**
 * Tries to create a projectile sprite and add it to the map.
 * Calculate its trajectory.
 * @return, true if the projectile was successfully created
 */
bool ProjectileFlyBState::createNewProjectile()
{
	//Log(LOG_INFO) << "ProjectileFlyBState::createNewProjectile() -> create Projectile";
	//Log(LOG_INFO) << ". _action_type = " << _action.type;
	++_action.autoShotCount;

	if (_unit->getGeoscapeSoldier() != NULL
		&& (_action.type != BA_THROW
			|| _action.type != BA_LAUNCH))
	{
		++_unit->getStatistics()->shotsFiredCounter;
	}

	//Log(LOG_INFO) << "projFlyB create() originTile = " << _origin;
	//Log(LOG_INFO) << "projFlyB create() targetVoxel = " << _targetVoxel;
	//Log(LOG_INFO) << "projFlyB create() targetVoxel.x = " << static_cast<float>(_targetVoxel.x) / 16.f;
	//Log(LOG_INFO) << "projFlyB create() targetVoxel.y = " << static_cast<float>(_targetVoxel.y) / 16.f;
	//Log(LOG_INFO) << "projFlyB create() targetVoxel.z = " << static_cast<float>(_targetVoxel.z) / 24.f;

	Projectile* const projectile = new Projectile(
											_parent->getResourcePack(),
											_parent->getSave(),
											_action,
											_origin,
											_targetVoxel,
											_ammo);

	_parent->getMap()->setProjectile(projectile); // else, add the projectile on the map


	_parent->setStateInterval(16); // set the speed of the state think cycle to 16 ms (roughly one think-cycle per frame)
	int sound = -1;

	_projectileImpact = VOXEL_EMPTY; // let it calculate a trajectory

	if (_action.type == BA_THROW)
	{
		_projectileImpact = projectile->calculateThrow(_unit->getThrowingAccuracy());
		//Log(LOG_INFO) << ". BA_THROW, part = " << _projectileImpact;

		if (   _projectileImpact == VOXEL_FLOOR
			|| _projectileImpact == VOXEL_UNIT
			|| _projectileImpact == VOXEL_OBJECT)
		{
			if (_unit->getFaction() != FACTION_PLAYER
				&& (   _projectileItem->getRules()->getBattleType() == BT_GRENADE
					|| _projectileItem->getRules()->getBattleType() == BT_PROXIMITYGRENADE))
			{
				//Log(LOG_INFO) << ". . auto-prime for AI, unitID " << _unit->getId();
				_projectileItem->setFuseTimer(0);
			}

			_projectileItem->moveToOwner(NULL);
			_unit->setCache(NULL);
			_parent->getMap()->cacheUnit(_unit);

			sound = ResourcePack::ITEM_THROW;

			if (_unit->getGeoscapeSoldier() != NULL
				&& _unit->getFaction() == _unit->getOriginalFaction())
			{
				_unit->addThrowingExp();
			}
		}
		else // unable to throw here
		{
			//Log(LOG_INFO) << ". . no throw, Voxel_Empty or _Wall or _OutofBounds";
			delete projectile;

			_parent->getMap()->setProjectile(NULL);

			_action.result = "STR_UNABLE_TO_THROW_HERE";
			_action.TU = 0;
			_unit->setStatus(STATUS_STANDING);

			_parent->popState();
			return false;
		}
	}
	else if (_action.weapon->getRules()->getArcingShot() == true) // special code for the "spit" trajectory
	{
		_projectileImpact = projectile->calculateThrow(_unit->getFiringAccuracy(
																			_action.type,
																			_action.weapon));
		//Log(LOG_INFO) << ". acid spit, part = " << _projectileImpact;

		if (_projectileImpact != VOXEL_EMPTY
			 && _projectileImpact != VOXEL_OUTOFBOUNDS)
		{
			_unit->startAiming();
			_unit->setCache(NULL);
			_parent->getMap()->cacheUnit(_unit);

			// lift-off
			if (_ammo->getRules()->getFireSound() != -1)
				sound = _ammo->getRules()->getFireSound();
			else if (_action.weapon->getRules()->getFireSound() != -1)
				sound = _action.weapon->getRules()->getFireSound();

			if (_parent->getSave()->getDebugMode() == false
				&& _action.type != BA_LAUNCH
				&& _ammo->spendBullet() == false)
			{
				_parent->getSave()->removeItem(_ammo);
				_action.weapon->setAmmoItem(NULL);
			}
		}
		else // no line of fire
		{
			//Log(LOG_INFO) << ". . no spit, no LoF, Voxel_Empty";
			delete projectile;

			_parent->getMap()->setProjectile(NULL);

			_action.result = "STR_NO_LINE_OF_FIRE";
			_action.TU = 0;
			_unit->setStatus(STATUS_STANDING);

			_parent->popState();
			return false;
		}
	}
	else // shoot weapon
	{
		if (_originVoxel != Position(-1,-1,-1)) // here, origin is a BL waypoint
		{
			_projectileImpact = projectile->calculateTrajectory(
															_unit->getFiringAccuracy(
																				_action.type,
																				_action.weapon),
															_originVoxel);
			//Log(LOG_INFO) << ". shoot weapon[0], voxelType = " << _projectileImpact;
		}
		else // this is non-BL weapon shooting
		{
			_projectileImpact = projectile->calculateTrajectory(_unit->getFiringAccuracy(
																					_action.type,
																					_action.weapon));
			//Log(LOG_INFO) << ". shoot weapon[1], voxelType = " << _projectileImpact;
		}
		//Log(LOG_INFO) << ". shoot weapon, voxelType = " << _projectileImpact;
		//Log(LOG_INFO) << ". finalTarget = " << projectile->getFinalTarget();

		projectile->storeProjectileDirection(); // kL


		if (_projectileImpact != VOXEL_EMPTY
			|| _action.type == BA_LAUNCH)
		{
			//Log(LOG_INFO) << ". . _projectileImpact AIM";
			if (_action.type == BA_LAUNCH)
				_parent->getMap()->setWaypointAction(); // reveal the Map until waypoint action completes.

			_unit->aim();
			_unit->setCache(NULL);
			_parent->getMap()->cacheUnit(_unit);

			// lift-off
			if (_ammo->getRules()->getFireSound() != -1)
				sound = _ammo->getRules()->getFireSound();
			else if (_action.weapon->getRules()->getFireSound() != -1)
				sound = _action.weapon->getRules()->getFireSound();

			if (_parent->getSave()->getDebugMode() == false
				&& _action.type != BA_LAUNCH
				&& _ammo->spendBullet() == false)
			{
				_parent->getSave()->removeItem(_ammo);
				_action.weapon->setAmmoItem(NULL);
			}
		}
		else // VOXEL_EMPTY, no line of fire
		{
			//Log(LOG_INFO) << ". no shot, no LoF, Voxel_Empty";
			delete projectile;

			_parent->getMap()->setProjectile(NULL);

			_action.result = "STR_NO_LINE_OF_FIRE";
			_action.TU = 0;
			_unit->setStatus(STATUS_STANDING);

			_parent->popState();
			return false;
		}
	}

	if (sound != -1)
		_parent->getResourcePack()->getSoundByDepth(
												_parent->getDepth(),
												sound)
											->play(
												-1,
												_parent->getMap()->getSoundAngle(_unit->getPosition()));

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
	//Log(LOG_INFO) << "ProjectileFlyBState::think()";
	if (_unit->getStatus() == STATUS_AIMING
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

	_parent->getSave()->getBattleState()->clearMouseScrollingState();
	Camera* const camera = _parent->getMap()->getCamera();

	// TODO refactoring: Store the projectile in this state, instead of getting it from the map each time.
	if (_parent->getMap()->getProjectile() == NULL)
	{
		if (_action.type == BA_AUTOSHOT
			&& _action.autoShotCount < _action.weapon->getRules()->getAutoShots()
			&& _unit->isOut() == false
			&& _ammo != NULL // kL
			&& _ammo->getAmmoQuantity() != 0
			&& ((_parent->getSave()->getTile(_unit->getPosition()) != NULL
					&& _parent->getSave()->getTile(_unit->getPosition())
						->hasNoFloor(_parent->getSave()->getTile(_unit->getPosition() + Position(0,0,-1))) == false)
				|| _unit->getMoveTypeUnit() == MT_FLY))
		{
			createNewProjectile();

			if (_action.cameraPosition.z != -1)
			{
				camera->setMapOffset(_action.cameraPosition);
//				_parent->getMap()->invalidate();
				_parent->getMap()->draw(); // kL
			}
		}
		else // think() FINISH.
		{
			if (_action.cameraPosition.z != -1)
//				&& _action.waypoints.size() < 2)
			{
				//Log(LOG_INFO) << "ProjectileFlyBState::think() FINISH: cameraPosition was Set";
/*				if (   _action.type == BA_STUN // kL, don't jump screen after these.
					|| _action.type == BA_HIT
					|| _action.type == BA_USE
					|| _action.type == BA_LAUNCH
					|| _action.type == BA_MINDCONTROL
					|| _action.type == BA_PANIC)
				{
//					camera->setMapOffset(camera->getMapOffset());
				}
				else */
				if (   _action.type == BA_THROW // jump screen back to pre-shot position
					|| _action.type == BA_AUTOSHOT
					|| _action.type == BA_SNAPSHOT
					|| _action.type == BA_AIMEDSHOT)
				{
					//Log(LOG_INFO) << "ProjectileFlyBState::think() FINISH: resetting Camera to original pos";
					if (camera->getPauseAfterShot() == true)
					{
						camera->setPauseAfterShot(false);
						SDL_Delay(336); // kL, screen-pause when shot hits target before reverting camera to shooter.
					}

					camera->setMapOffset(_action.cameraPosition);
//					_parent->getMap()->invalidate();
					_parent->getMap()->draw();	// kL
				}
			}

			if (_unit->getFaction() == _parent->getSave()->getSide()
				&& _action.type != BA_PANIC
				&& _action.type != BA_MINDCONTROL
				&& _parent->getSave()->getUnitsFalling() == false)
			{
				//Log(LOG_INFO) << "ProjectileFlyBState::think(), checkReactionFire()"
				//	<< " ID " << _unit->getId()
				//	<< " action.type = " << _action.type
				//	<< " action.TU = " << _action.TU;
				_parent->getTileEngine()->checkReactionFire(			// note: I don't believe that smoke obscuration gets accounted
														_unit,			// for by this call if the current projectile caused cloud.
														_action.TU);	// But that's kinda ok.
			}

			if (_unit->isOut() == false
				&& _action.type != BA_HIT) // kL_note: huh? -> ie. melee & psi attacks shouldn't even get in here. But code needs cosmetic surgery .....
			{
				_unit->setStatus(STATUS_STANDING);
			}

			if (_parent->getSave()->getSide() == FACTION_PLAYER
				|| _parent->getSave()->getDebugMode() == true)
			{
				_parent->setupCursor();
			}

			_parent->popState();
		}
	}
	else // projectile VALID in motion -> ! impact !
	{
		if (_action.type != BA_THROW
			&& _ammo != NULL
			&& _ammo->getRules()->getShotgunPellets() != 0)
		{
			// shotgun pellets move to their terminal location instantly as fast as possible
			_parent->getMap()->getProjectile()->skipTrajectory();
		}

		if (_parent->getMap()->getProjectile()->traceProjectile() == false) // missile pathing finished
		{
			if (_action.type == BA_THROW)
			{
//				_parent->getMap()->resetCameraSmoothing();
				Position pos = _parent->getMap()->getProjectile()->getPosition(-1);
				pos.x /= 16;
				pos.y /= 16;
				pos.z /= 24;

				if (pos.x > _parent->getSave()->getMapSizeX())
					--pos.x;

				if (pos.y > _parent->getSave()->getMapSizeY())
					--pos.y;

				BattleItem* const item = _parent->getMap()->getProjectile()->getItem();
				if (item->getRules()->getBattleType() == BT_GRENADE
					&& item->getFuseTimer() == 0)
					// && Options::battleInstantGrenade == true // -> moved to PrimeGrenadeState (0 cannot be set w/out InstantGrenades)
				{
					_parent->statePushFront(new ExplosionBState( // it's a hot potato set to explode on contact
															_parent,
															_parent->getMap()->getProjectile()->getPosition(-1),
															item,
															_unit));
				}
				else
				{
					_parent->dropItem(
									pos,
									item);

					if (_unit->getFaction() == FACTION_HOSTILE
						&& _projectileItem->getRules()->getBattleType() == BT_GRENADE)
					{
						_parent->getTileEngine()->setDangerZone(
															pos,
															item->getRules()->getExplosionRadius(),
															_unit);
					}
				}

				_parent->getResourcePack()->getSoundByDepth(
														_parent->getDepth(),
														ResourcePack::ITEM_DROP)
													->play(
														-1,
														_parent->getMap()->getSoundAngle(pos));
			}
			else if (_action.type == BA_LAUNCH
				&& _projectileImpact == VOXEL_EMPTY
				&& _action.waypoints.size() > 1)
			{
				_origin = _action.waypoints.front();
				_action.waypoints.pop_front();
				_action.target = _action.waypoints.front();

				// launch the next projectile in the waypoint cascade
				ProjectileFlyBState* const toNextWp = new ProjectileFlyBState(
																			_parent,
																			_action,
																			_origin); // -> tilePos

				toNextWp->setOriginVoxel(_parent->getMap()->getProjectile()->getPosition()); // !getPosition(-1) -> tada, fixed. // -> voxlPos

				// this follows BL as it hits through waypoints
				camera->centerOnPosition(_origin);

				if (_origin == _action.target)
					toNextWp->targetFloor();

				_parent->statePushNext(toNextWp);
			}
			else // shoot.
			{
//				_parent->getMap()->resetCameraSmoothing();
				if (_action.type == BA_LAUNCH) // kL: Launches explode at final waypoint.
				{
					_projectileImpact = VOXEL_OBJECT;
					_parent->getMap()->setWaypointAction(false); // reveal the Map until waypoint action completes.
				}

				BattleUnit* const shotAt = _parent->getSave()->getTile(_action.target)->getUnit();
				if (shotAt != NULL // only counts for guns, not throws or launches
					&& shotAt->getGeoscapeSoldier() != NULL)
				{
					++shotAt->getStatistics()->shotAtCounter;
				}

				if (_action.type == BA_LAUNCH
					&& _ammo != NULL
					&& _ammo->spendBullet() == false)
				{
					_parent->getSave()->removeItem(_ammo);
					_action.weapon->setAmmoItem(NULL);
				}

				if (_projectileImpact != VOXEL_OUTOFBOUNDS) // NOT out of map
				{
					// explosions impact not inside the voxel but two steps back;
					// projectiles generally move 2 voxels at a time
					int offset = 0;

					if (_ammo != NULL
						&& _ammo->getRules()->getExplosionRadius() > -1
						&& _projectileImpact != VOXEL_UNIT)
					{
						offset = -2; // step back a bit so tileExpl isn't behind a close wall.
					}

					//Log(LOG_INFO) << "projFlyB think() new ExplosionBState() explCenter " << _parent->getMap()->getProjectile()->getPosition(offset);
					//Log(LOG_INFO) << "projFlyB think() offset " << offset;
					//Log(LOG_INFO) << "projFlyB think() projImpact voxelType " << _projectileImpact;
					//Log(LOG_INFO) << "projFlyB think() explCenter.x = " << static_cast<float>(_parent->getMap()->getProjectile()->getPosition(offset).x) / 16.f;
					//Log(LOG_INFO) << "projFlyB think() explCenter.y = " << static_cast<float>(_parent->getMap()->getProjectile()->getPosition(offset).y) / 16.f;
					//Log(LOG_INFO) << "projFlyB think() explCenter.z = " << static_cast<float>(_parent->getMap()->getProjectile()->getPosition(offset).z) / 24.f;
					_parent->statePushFront(new ExplosionBState(
															_parent,
															_parent->getMap()->getProjectile()->getPosition(offset),
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
						const int shot = _ammo->getRules()->getShotgunPellets();
						if (shot > 1) // first pellet is done above.
						{
							int i = 0;
							while (i != shot - 1)
							{
								Projectile* const proj = new Projectile(
																	_parent->getResourcePack(),
																	_parent->getSave(),
																	_action,
																	_origin,
																	_targetVoxel,
																	_ammo);

								_projectileImpact = proj->calculateTrajectory(
																		std::max(
																				0.,
																				_unit->getFiringAccuracy(
																									_action.type,
																									_action.weapon)
																								- i * 0.023)); // pellet spread.
								if (_projectileImpact != VOXEL_EMPTY)
								{
									proj->skipTrajectory(); // as above: skip the shot to the end of its path

									if (_projectileImpact != VOXEL_OUTOFBOUNDS) // insert an explosion and hit
									{
										Explosion* const expl = new Explosion(
																			proj->getPosition(1),
																			_ammo->getRules()->getHitAnimation());

										_parent->getMap()->getExplosions()->push_back(expl);
										//Log(LOG_INFO) << "ProjectileFlyBState::think() shotHIT = " << (i + 1);
										_parent->getSave()->getTileEngine()->hit(
																				proj->getPosition(1),
																				_ammo->getRules()->getPower(),
																				_ammo->getRules()->getDamageType(),
																				_unit);
									}
								}

								++i;
								delete proj;
							}
						}
					}

					// Silacoid floorburn was here; moved down to PerformMeleeAttack()

					if (_unit->getOriginalFaction() == FACTION_PLAYER	// This section is only for SoldierDiary mod.
						&& _projectileImpact == VOXEL_UNIT)				// see below also; was also for setting aggroState
					{
						BattleUnit* const victim = _parent->getSave()->getTile(
															_parent->getMap()->getProjectile()->getPosition(offset) / Position(16,16,24))
														->getUnit();
						if (victim != NULL)
//							&& victim->isOut() == false)
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

							const BattleUnit* const target = _parent->getSave()->getTile(_action.target)->getUnit(); // target (not necessarily who was hit)
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

									if (dist > static_cast<int>(_unit->getFiringAccuracy(
																					_action.type,
																					_action.weapon) * 35.))
									{
										++statsUnit->lowAccuracyHitCounter;
									}
								}
							}
						}
					}
					// kL_note: Could take this section out (i had removed it for a while ..)
					// ... Let's try something
/*kL
					if (_projectileImpact == VOXEL_UNIT)
					{
						BattleUnit* victim = _parent->getSave()->getTile(
																	_parent->getMap()->getProjectile()->getPosition(offset) / Position(16, 16, 24))
																->getUnit();
						// kL_begin:
						if (victim
							&& !victim->isOut(true, true)
							&& victim->getOriginalFaction() == FACTION_PLAYER
							&& _unit->getFaction() == FACTION_HOSTILE)
						{
							_unit->setExposed();
						} */ // kL_end. But this is entirely unnecessary, since aLien has already seen and logged the soldier.
/*kL
						if (victim
							&& !victim->isOut(true, true)
							&& victim->getFaction() == FACTION_HOSTILE)
						{
							AlienBAIState* aggro = dynamic_cast<AlienBAIState*>(victim->getCurrentAIState());
							if (aggro != 0)
							{
								aggro->setWasHitBy(_unit);	// kL_note: is used only for spotting on RA.
								_unit->setExposed();		// kL_note: might want to remark this! Ok.
								// technically, in the original as I remember it, only
								// a BlasterLaunch (by xCom) would set an xCom soldier Exposed here!
								// ( Those aLiens had a way of tracing a BL back to its origin ....)
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
 * Flying projectiles cannot be cancelled, but they can be skipped.
 */
void ProjectileFlyBState::cancel()
{
	Projectile* const projectile = _parent->getMap()->getProjectile();
	if (projectile != NULL)
	{
		projectile->skipTrajectory();

		Camera* const camera = _parent->getMap()->getCamera();
		const Position pos = projectile->getPosition() / Position(16,16,24);
//		if (!_parent->getMap()->getCamera()->isOnScreen(Position(p.x/16, p.y/16, p.z/24), false, 0, false))
		if (camera->isOnScreen(pos) == false)
			camera->centerOnPosition(pos);
	}
}

/**
 * Validates the throwing range.
 * @param action - pointer to BattleAction struct (BattlescapeGame.h)
 * @param origin - reference the origin in voxelspace
 * @param target - pointer to the targeted Tile
 * @return, true if the range is valid
 */
bool ProjectileFlyBState::validThrowRange(
		const BattleAction* const action,
		const Position& origin,
		const Tile* const target)
{
	//Log(LOG_INFO) << "ProjectileFlyBState::validThrowRange()";
	if (action->type != BA_THROW) // this is a celatid spit.
//		&& target->getUnit())
	{
//		offset_z = target->getUnit()->getHeight() / 2 + target->getUnit()->getFloatHeight();
		return true;
	}

	int weight = action->weapon->getRules()->getWeight();
	if (action->weapon->getAmmoItem() != NULL
		&& action->weapon->getAmmoItem() != action->weapon)
	{
		weight += action->weapon->getAmmoItem()->getRules()->getWeight();
	}

	const int
		offset_z = 2, // kL_note: this is prob +1 (.. +2) to get things up off the lowest voxel of a targetTile.
//	int delta_z = origin.z
//					- (((action->target.z * 24)
//						+ offset_z)
//						- target->getTerrainLevel());
		delta_z = origin.z // voxelspace
				- action->target.z * 24
				- offset_z
				+ target->getTerrainLevel();
	const double maxDist = static_cast<double>(
						   getMaxThrowDistance( // tilespace
											weight,
											static_cast<int>(Round(
												static_cast<double>(action->actor->getBaseStats()->strength) * (action->actor->getAccuracyModifier() / 2. + 0.5))),
											delta_z)
										+ 8)
									/ 16.;
	// Throwing Distance was roughly = 2.5 \D7 Strength / Weight
//	double range = 2.63 * static_cast<double>(action->actor->getBaseStats()->strength / action->weapon->getRules()->getWeight()); // old code.

	const int
		delta_x = action->actor->getPosition().x - action->target.x,
		delta_y = action->actor->getPosition().y - action->target.y;
	const double throwDist = std::sqrt(static_cast<double>((delta_x * delta_x) + (delta_y * delta_y)));

	// throwing off a building of 1 level lets you throw 2 tiles further than normal range,
	// throwing up the roof of this building lets your throw 2 tiles less further
/*	int delta_z = action->actor->getPosition().z - action->target.z;
	distance -= static_cast<double>(delta_z);
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
 * Validates the throwing range.
 * @param weight	- the weight of the object
 * @param strength	- the strength of the thrower
 * @param level		- the difference in height between the thrower and the target
 * @return, the maximum throwing range
 */
int ProjectileFlyBState::getMaxThrowDistance(
		int weight,
		int strength,
		int level)
{
	//Log(LOG_INFO) << "ProjectileFlyBState::getMaxThrowDistance()";
	double
		curZ = static_cast<double>(level) + 0.5,
		delta_z = 1.;

	int dist = 0;
	while (dist < 4000) // just in case
	{
		dist += 8;

		if (delta_z < -1.)
			curZ -= 8.;
		else
			curZ += delta_z * 8.;

		if (curZ < 0.
			&& delta_z < 0.) // roll back
		{
			delta_z = std::max(delta_z, -1.);
			if (std::abs(delta_z) > 1e-10) // rollback horizontal
				dist -= static_cast<int>(curZ / delta_z);

			break;
		}

		delta_z -= static_cast<double>(weight * 50 / strength) / 100.;
		if (delta_z <= -2.) // become falling
			break;
	}

	//Log(LOG_INFO) << ". dist = " << dist / 16;
	return dist;
}

/**
 * Set the origin voxel.
 * @note Used for the blaster launcher.
 * @param pos - reference the origin voxel
 */
void ProjectileFlyBState::setOriginVoxel(const Position& pos) // private.
{
	_originVoxel = pos;
}

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
void ProjectileFlyBState::performMeleeAttack()
{
	//Log(LOG_INFO) << "ProjectileFlyBState::performMeleeAttack()";
	const BattleUnit* const buTarget = _parent->getSave()->getTile(_action.target)->getUnit();
	const int height = buTarget->getFloatHeight() + (buTarget->getHeight() / 2);

	Position voxel;
	_parent->getSave()->getPathfinding()->directionToVector(
														_unit->getDirection(),
														&voxel);
	voxel = _action.target * Position(16,16,24) + Position(
														8,8,
														height - _parent->getSave()->getTile(_action.target)->getTerrainLevel())
													- voxel;

	_unit->aim();
	_unit->setCache(NULL);
	_parent->getMap()->cacheUnit(_unit);

	//Log(LOG_INFO) << ". meleeAttack, weapon = " << _action.weapon->getRules()->getType();
	//Log(LOG_INFO) << ". meleeAttack, ammo = " << _ammo->getRules()->getType();
	// kL: from ExplosionBState, moved here to play a proper hit/miss sFx
	bool success = false;
	const int percent = static_cast<int>(Round(_unit->getFiringAccuracy(
																	BA_HIT,
//																	_ammo) // Ammo is the weapon since (melee==true). Not necessarily ...
																	_action.weapon) * 100.));
	//Log(LOG_INFO) << ". ID " << _unit->getId() << " weapon " << _action.weapon->getRules()->getType() << " hit percent = " << percent;
	if (RNG::percent(percent) == true)
	{
		//Log(LOG_INFO) << ". success";
		success = true;
	}

	int sound = -1;
	if (success == false)
	{
		//Log(LOG_INFO) << ". meleeAttack MISS, unitID " << _unit->getId();
		sound = ResourcePack::ITEM_THROW;

		if (_ammo->getRules()->getBattleType() == BT_MELEE
			&& _ammo->getRules()->getMeleeSound() != -1
			&& _ammo->getRules()->getMeleeHitSound() != -1)	// if there is a hitSound play attackSound, else ITEM_THROW;
															// the hitSound will be used for success.
		{
			sound = _ammo->getRules()->getMeleeSound();
		}
	}
	else
	{
		//Log(LOG_INFO) << ". meleeAttack HIT, unitID " << _unit->getId();
		sound = ResourcePack::ITEM_DROP;

		if (_ammo->getRules()->getBattleType() == BT_MELEE)
		{
			if (_ammo->getRules()->getMeleeHitSound() != -1)
				sound = _ammo->getRules()->getMeleeHitSound();
			else if (_ammo->getRules()->getMeleeSound() != -1)
				sound = _ammo->getRules()->getMeleeSound();
		}
	}

	if (sound != -1)
		_parent->getResourcePack()->getSoundByDepth(
												_parent->getDepth(),
												sound)
											->play(
												-1,
												_parent->getMap()->getSoundAngle(_action.target));

	if (_parent->getSave()->getDebugMode() == false
		&& _action.weapon->getRules()->getBattleType() == BT_MELEE
		&& _ammo != NULL
		&& _ammo->spendBullet() == false)
	{
		_parent->getSave()->removeItem(_ammo);

		if (_action.weapon != NULL) // kL, in case the weapon just spent itself as a bullet -- jic.
			_action.weapon->setAmmoItem(NULL);
	}

	if (_unit->getSpecialAbility() == SPECAB_BURNFLOOR
		|| _unit->getSpecialAbility() == SPECAB_BURN_AND_EXPLODE)
	{
		_parent->getSave()->getTile(_action.target)->ignite(15);
	}

	_parent->getMap()->setCursorType(CT_NONE);

	_parent->statePushNext(new ExplosionBState(
											_parent,
											voxel,
											_action.weapon,
											_unit,
											NULL,
											true,
											success)); // kL_add.
	//Log(LOG_INFO) << "ProjectileFlyBState::performMeleeAttack() EXIT";
}

}
