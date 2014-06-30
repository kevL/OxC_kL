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

#define _USE_MATH_DEFINES

#include "ProjectileFlyBState.h"

#include <cmath>

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
#include "../Engine/Logger.h"
#include "../Engine/Options.h"
#include "../Engine/Sound.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/Armor.h"
#include "../Ruleset/RuleItem.h"

#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

/**
 * Sets up an ProjectileFlyBState.
 */
ProjectileFlyBState::ProjectileFlyBState(
		BattlescapeGame* parent,
		BattleAction action,
		Position origin)
	:
		BattleState(
			parent,
			action),
		_unit(NULL),
		_ammo(NULL),
		_projectileItem(NULL),
		_origin(origin),
		_originVoxel(-1,-1,-1), // kL_note: for BL waypoints
		_projectileImpact(0),
		_initialized(false),
		_targetFloor(false),
		_targetVoxel(-1,-1,-1) // kL. Why is this not initialized in the stock oXc code?
{
}

/**
 *
 */
ProjectileFlyBState::ProjectileFlyBState(
		BattlescapeGame* parent,
		BattleAction action)
	:
		BattleState(
			parent,
			action),
		_unit(NULL),
		_ammo(NULL),
		_originVoxel(-1,-1,-1), // kL_note: for BL waypoints
		_projectileItem(NULL),
		_origin(action.actor->getPosition()),
		_projectileImpact(0),
		_initialized(false),
		_targetFloor(false),
		_targetVoxel(-1,-1,-1) // kL. Why is this not initialized in the stock oXc code?
{
}

/**
 * Deletes the ProjectileFlyBState.
 */
ProjectileFlyBState::~ProjectileFlyBState()
{
}

/**
 * Initializes the sequence:
 * - checks if the shot is valid
 * - calculates the base accuracy
 */
void ProjectileFlyBState::init()
{
	//Log(LOG_INFO) << "ProjectileFlyBState::init()";
	if (_initialized)
	{
		//Log(LOG_INFO) << ". already initialized, EXIT";
		return;
	}
	_initialized = true;
	//Log(LOG_INFO) << ". getStopShot() = " << _action.actor->getStopShot();


	// kL_begin:
/*	BattleUnit* targetUnit = _parent->getSave()->getTile(_action.target)->getUnit();
	if (targetUnit
		&& targetUnit->getFaction() != _parent->getSave()->getSide()
		&& targetUnit->getDashing()) // this really doesn't have to be left here <-
	{
		//Log(LOG_INFO) << ". targetUnit was dashing -> now set FALSE";
		targetUnit->setDashing(false);
	} */ // kL_end. -> I moved this to SavedBattleGame::prepareNewTurn()


//kL	BattleItem* weapon = _action.weapon; // < was a pointer!! kL_note.
//kL	_projectileItem = 0; // already initialized.

	_unit = _action.actor;
	_ammo = _action.weapon->getAmmoItem();

	// **** The first 4 of these SHOULD NEVER happen ****
	// the 4th is wtf: tu ought be spent for this already.
	// They should be coded with a tuRefund() function regardless.
	if (_unit->isOut(true, true))
//		|| _unit->getHealth() == 0
//		|| _unit->getHealth() < _unit->getStunlevel())
	{
		// something went wrong - we can't shoot when dead or unconscious, or if we're about to fall over.
		//Log(LOG_INFO) << ". actor is Out, EXIT";
		_unit->setStopShot(false); // kL
		_parent->popState();

		return;
	}
	else if (!_action.weapon) // can't shoot without weapon
	{
		//Log(LOG_INFO) << ". no weapon, EXIT";
		_unit->setStopShot(false); // kL
		_parent->popState();

		return;
	}
	else if (!_parent->getSave()->getTile(_action.target)) // invalid target position
	{
		//Log(LOG_INFO) << ". no targetPos, EXIT";
		_unit->setStopShot(false); // kL
		_parent->popState();

		return;
	}
	else if (_parent->getPanicHandled()
		&& _action.type != BA_HIT
		&& _action.type != BA_STUN
		&& _unit->getTimeUnits() < _action.TU)
	{
		//Log(LOG_INFO) << ". not enough time units, EXIT";
		_action.result = "STR_NOT_ENOUGH_TIME_UNITS";
		_unit->setStopShot(false); // kL
		_parent->popState();

		return;
	}
	// kL_begin: ProjectileFlyBState::init() Give back time units; pre-end Reaction Fire. +stopShot!!
	else if (_unit->getStopShot())
	{
		// do I have to refund TU's for this??? YES. done
		// when are TU subtracted for a primaryAction firing/throwing action?
		_unit->setStopShot(false);

		_unit->setTimeUnits(_unit->getTimeUnits()
									+ _unit->getActionTUs(
													_action.type,
													_action.weapon));
		_parent->popState();

		//Log(LOG_INFO) << ". stopShot ID = " << _unit->getId() << ", refund TUs. EXIT";
		return;
	}
	else if (_unit->getFaction() != _parent->getSave()->getSide()) // reaction fire
	{
		BattleUnit* targetUnit = _parent->getSave()->getTile(_action.target)->getUnit();
		if (targetUnit)
		{
			if (_ammo == NULL
				|| targetUnit->isOut(true, true)
				|| targetUnit != _parent->getSave()->getSelectedUnit())
			{
				_unit->setTimeUnits(_unit->getTimeUnits()
											+ _unit->getActionTUs(
															_action.type,
															_action.weapon));
				_parent->popState();

				//Log(LOG_INFO) << ". . . reactionFire refund (targetUnit exists) EXIT";
				return;
			}
		}
		else
		{
			_unit->setTimeUnits(_unit->getTimeUnits()
										+ _unit->getActionTUs(
														_action.type,
														_action.weapon));
			_parent->popState();

			//Log(LOG_INFO) << ". . reactionFire refund (no targetUnit) EXIT";
			return;
		} // kL_end.
	}

	// autoshot will default back to snapshot if it's not possible
	// kL_note: should this be done *before* tu expenditure?!! Ok it is,
	// popState() does tu costs
	if (_action.weapon->getRules()->getAccuracyAuto() == 0
		&& _action.type == BA_AUTOSHOT)
	{
		_action.type = BA_SNAPSHOT;
	}

	// snapshot defaults to "hit" if it's a melee weapon
	// (in case of reaction "shots" with a melee weapon),
	// for Silacoid attack, etc.
	// kL_note: not sure that melee reactionFire is properly implemented...
	if (_action.weapon->getRules()->getBattleType() == BT_MELEE
		&& (_action.type == BA_SNAPSHOT
			|| _action.type == BA_AUTOSHOT		// kL
			|| _action.type == BA_AIMEDSHOT))	// kL
	{
		//Log(LOG_INFO) << ". convert BA_SNAPSHOT to BA_HIT";
		_action.type = BA_HIT;
	}

	Tile* endTile = _parent->getSave()->getTile(_action.target);

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
			else if (_action.weapon->getRules()->getMaxRange() > 0 // in case -1 gets used for infinite.
				&& _parent->getTileEngine()->distance(
												_action.actor->getPosition(),
												_action.target)
											> _action.weapon->getRules()->getMaxRange())
/*			else // kL_begin:
			{
				int dist = _parent->getTileEngine()->distance(
														_action.actor->getPosition(),
														_action.target);
				if (dist > _action.weapon->getRules()->getMaxRange()
					|| dist < _action.weapon->getRules()->getMinRange()) */ // kL_end.
			{
				//Log(LOG_INFO) << ". . . out of range, EXIT";
				_action.result = "STR_OUT_OF_RANGE";
				_parent->popState();

				return;
			}
//			}
		break;
		case BA_THROW:
		{
			//Log(LOG_INFO) << ". . BA_THROW";
			Position originVoxel = _parent->getTileEngine()->getOriginVoxel(
																		_action,
																		NULL);
			if (!validThrowRange(
							&_action,
							originVoxel,
							_parent->getSave()->getTile(_action.target)))
			{
				//Log(LOG_INFO) << ". . . not valid throw range, EXIT";
				_action.result = "STR_OUT_OF_RANGE";
				_parent->popState();

				return;
			}

			if (endTile
				&& endTile->getTerrainLevel() == -24
				&& endTile->getPosition().z + 1 < _parent->getSave()->getMapSizeZ())
			{
				_action.target.z += 1;
			}

			_projectileItem = _action.weapon;
		}
		break;
		case BA_HIT:
			//Log(LOG_INFO) << ". . BA_HIT performMeleeAttack()";
			if (!_parent->getTileEngine()->validMeleeRange(
													_action.actor->getPosition(),
													_action.actor->getDirection(),
													_action.actor,
													NULL,
													&_action.target))
			{
				//Log(LOG_INFO) << ". . . out of hit range, EXIT";
				_action.result = "STR_THERE_IS_NO_ONE_THERE";
				_parent->popState();

				return;
			}

//kL		performMeleeAttack();
			//Log(LOG_INFO) << ". . BA_HIT performMeleeAttack() DONE";
//kL			return;
		break;
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
													_action.actor));

			return;
		break;

		default:
			//Log(LOG_INFO) << ". . default, EXIT";
			_parent->popState();

			return;
		break;
	}


	if (_action.type == BA_LAUNCH
		|| (Options::forceFire
			&& (SDL_GetModState() & KMOD_CTRL) != 0
			&& _parent->getSave()->getSide() == FACTION_PLAYER)
		|| !_parent->getPanicHandled())
	{
		_targetVoxel = Position( // target nothing, targets the middle of the tile
							_action.target.x * 16 + 8,
							_action.target.y * 16 + 8,
							_action.target.z * 24 + 12);

		if (_action.type == BA_LAUNCH)
		{
			if (_targetFloor) // kL_note: was, if(_action.target == _origin)
				// launched missiles with two waypoints placed on the same tile: target the floor.
				_targetVoxel.z -= 10;
			else
				// launched missiles go slightly higher than the middle.
				_targetVoxel.z += 4;
		}
	}
	else
	{
		// determine the target voxel.
		// aim at the center of the unit, the object, the walls or the floor (in that priority)
		// if there is no LOF to the center, try elsewhere (more outward).
		// Store this target voxel.
		Tile* targetTile = _parent->getSave()->getTile(_action.target);
		Position hitPos;

		Position originVoxel = _parent->getTileEngine()->getOriginVoxel(
																	_action,
																	_parent->getSave()->getTile(_origin));
		if (targetTile->getUnit() != NULL)
		{
			//Log(LOG_INFO) << ". targetTile has unit";
			if (_origin == _action.target
				|| targetTile->getUnit() == _unit)
			{
				_targetVoxel = Position( // don't shoot yourself but shoot at the floor
									_action.target.x * 16 + 8,
									_action.target.y * 16 + 8,
									_action.target.z * 24);
			}
			else
				_parent->getTileEngine()->canTargetUnit(
													&originVoxel,
													targetTile,
													&_targetVoxel,
													_unit);
		}
		// kL_begin: AutoShot vs. tile w/ dead or stunned Unit just sprays through the middle of the tile.
		// note, however, this also affects attempts to target a tile where there is/was(?) a dead unit...
		// So let's narrow it down some........... ie. to do this correctly I'd need some sort of 'autoShotKill' boolean
		else if (_action.autoShotCount > 1
			&& _action.autoShotKill) // note: If targetUnit is still alive after the first shot, see ABOVE^^
//			&& targetTile->getUnit()
//			&& targetTile->getUnit()->isOut(true, true)) // NOT Working
//		_action.autoShotKill //_action.autoShotCount
//			&& _action.autoShotCount < _action.weapon->getRules()->getAutoShots())
		{
			//Log(LOG_INFO) << ". targetTile vs. Autoshot!";
			_targetVoxel = Position( // target nothing, targets the middle of the tile
								_action.target.x * 16 + 8,
								_action.target.y * 16 + 8,
								_action.target.z * 24 + 12);
		} // kL_end.
		else if (targetTile->getMapData(MapData::O_OBJECT) != NULL)
		{
			//Log(LOG_INFO) << ". targetTile has content-object";
			if (!_parent->getTileEngine()->canTargetTile(
													&originVoxel,
													targetTile,
													MapData::O_OBJECT,
													&_targetVoxel,
													_unit))
			{
				_targetVoxel = Position(
									_action.target.x * 16 + 8,
									_action.target.y * 16 + 8,
									_action.target.z * 24 + 8);
			}
		}
		else if (targetTile->getMapData(MapData::O_NORTHWALL) != NULL)
		{
			//Log(LOG_INFO) << ". targetTile has northwall";
			if (!_parent->getTileEngine()->canTargetTile(
													&originVoxel,
													targetTile,
													MapData::O_NORTHWALL,
													&_targetVoxel,
													_unit))
			{
				_targetVoxel = Position(
									_action.target.x * 16 + 8,
									_action.target.y * 16,
									_action.target.z * 24 + 10);
			}
		}
		else if (targetTile->getMapData(MapData::O_WESTWALL) != NULL)
		{
			//Log(LOG_INFO) << ". targetTile has westwall";
			if (!_parent->getTileEngine()->canTargetTile(
													&originVoxel,
													targetTile,
													MapData::O_WESTWALL,
													&_targetVoxel,
													_unit))
			{
				_targetVoxel = Position(
									_action.target.x * 16,
									_action.target.y * 16 + 8,
									_action.target.z * 24 + 10);
			}
		}
		else if (targetTile->getMapData(MapData::O_FLOOR) != NULL)
		{
			//Log(LOG_INFO) << ". targetTile has floor";
			if (!_parent->getTileEngine()->canTargetTile(
													&originVoxel,
													targetTile,
													MapData::O_FLOOR,
													&_targetVoxel,
													_unit))
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

	if (createNewProjectile())
	{
		_parent->getMap()->setCursorType(CT_NONE);
		_parent->getMap()->getCamera()->stopMouseScrolling();
	}
	//Log(LOG_INFO) << "ProjectileFlyBState::init() EXIT";
}

/**
 * Tries to create a projectile sprite and add it to the map,
 * calculating its trajectory.
 * @return, True if the projectile was successfully created.
 */
bool ProjectileFlyBState::createNewProjectile()
{
	//Log(LOG_INFO) << "ProjectileFlyBState::createNewProjectile() -> create Projectile";
	//Log(LOG_INFO) << ". _action_type = " << _action.type;

	++_action.autoShotCount;

	if (_action.type != BA_THROW
		|| _action.type != BA_LAUNCH)
	{
		_unit->getStatistics()->shotsFiredCounter++;
	}

	int bulletSprite = -1;
	if (_action.type != BA_THROW)
	{
		bulletSprite = _ammo->getRules()->getBulletSprite();
		if (bulletSprite == -1)
			bulletSprite = _action.weapon->getRules()->getBulletSprite();
	}

	Projectile* projectile = new Projectile(
										_parent->getResourcePack(),
										_parent->getSave(),
										_action,
										_origin,
										_targetVoxel,
										bulletSprite);

	_parent->getMap()->setProjectile(projectile); // add the projectile on the map

	// set the speed of the state think cycle to 16 ms (roughly one think-cycle per frame)
//kL	_parent->setStateInterval(1000/60);
	Uint32 interval = static_cast<Uint32>(16);	// kL
	_parent->setStateInterval(interval);		// kL

	_projectileImpact = VOXEL_EMPTY; // let it calculate a trajectory

	if (_action.type == BA_THROW)
	{
		_projectileImpact = projectile->calculateThrow(_unit->getThrowingAccuracy()); // / 100.0
		//Log(LOG_INFO) << ". BA_THROW, part = " << _projectileImpact;
		if (_projectileImpact == VOXEL_FLOOR
			|| _projectileImpact == VOXEL_UNIT
			|| _projectileImpact == VOXEL_OBJECT)
		{
			if (_unit->getFaction() != FACTION_PLAYER
				&& _projectileItem->getRules()->getBattleType() == BT_GRENADE)
			{
				_projectileItem->setFuseTimer(0);
			}

			_projectileItem->moveToOwner(NULL);
			_unit->setCache(NULL);
			_parent->getMap()->cacheUnit(_unit);

			_parent->getResourcePack()->getSound(
												"BATTLE.CAT",
												39)
											->play();

			if (_unit->getOriginalFaction() == FACTION_PLAYER	// kL
				&& _unit->getFaction() == FACTION_PLAYER)		// kL
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
			_unit->setStatus(STATUS_STANDING); // kL
			_parent->popState();

			return false;
		}
	}
	else if (_action.weapon->getRules()->getArcingShot()) // special code for the "spit" trajectory
	{
		_projectileImpact = projectile->calculateThrow(_unit->getFiringAccuracy(
																			_action.type,
																			_action.weapon));
		//Log(LOG_INFO) << ". acid spit, part = " << _projectileImpact;

		if (_projectileImpact != VOXEL_EMPTY
			 && _projectileImpact != VOXEL_OUTOFBOUNDS)
		{
//kL			_unit->aim(true); // set the celatid in an aiming position <- yeah right. not.
//kL			_unit->setCache(0);
//kL			_parent->getMap()->cacheUnit(_unit);

			// and we have a lift-off
			if (_ammo->getRules()->getFireSound() != -1)
				_parent->getResourcePack()->getSound(
												"BATTLE.CAT",
												_ammo->getRules()->getFireSound())
											->play();
			else if (_action.weapon->getRules()->getFireSound() != -1)
				_parent->getResourcePack()->getSound(
												"BATTLE.CAT",
												_action.weapon->getRules()->getFireSound())
											->play();

			if (!_parent->getSave()->getDebugMode()
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
			_action.TU = 0; // kL
			_unit->setStatus(STATUS_STANDING); // kL
			_parent->popState();

			return false;
		}
	}
	else if (_action.type == BA_HIT) // kL. Let's not calculate anything we don't have to for meleeHits!
	{
		//Log(LOG_INFO) << ". melee attack!";// part = " << _projectileImpact;
		// validMeleeRange/target has been validated.
//		_projectileImpact = 4;
		_projectileImpact = projectile->calculateTrajectory(_unit->getFiringAccuracy(
																				_action.type,
																				_action.weapon));
		//Log(LOG_INFO) << ". melee attack!";// part = " << _projectileImpact;

		// Can soldiers swing a club, graphically??
//		_unit->aim(true); // set the soldier in an aiming position
//		_unit->setCache(0);
//		_parent->getMap()->cacheUnit(_unit);

		// and we have a hit!
		//Log(LOG_INFO) << ". melee attack! Play fireSound";// part = " << _projectileImpact;
//		if (_action.weapon->getRules()->getFireSound() != -1)
		if (_action.weapon->getRules()->getMeleeAttackSound() != -1)
			_parent->getResourcePack()->getSound(
											"BATTLE.CAT",
//											_action.weapon->getRules()->getFireSound())
											_action.weapon->getRules()->getMeleeAttackSound())
										->play();
	}
	else // shoot weapon / was do melee attack too
	{
		if (_originVoxel != Position(-1,-1,-1)) // this i believe is BL waypoints.
		{
			_projectileImpact = projectile->calculateTrajectory(
															_unit->getFiringAccuracy(
																				_action.type,
																				_action.weapon),
															_originVoxel);
		}
		else // and this is normal weapon shooting
		{
			_projectileImpact = projectile->calculateTrajectory(_unit->getFiringAccuracy(
																					_action.type,
																					_action.weapon));
		}
		//Log(LOG_INFO) << ". shoot weapon, part = " << _projectileImpact;

		if (_projectileImpact == VOXEL_UNIT)	// kL
			_action.autoShotKill = true;		// kL

		if (_projectileImpact != VOXEL_EMPTY
			|| _action.type == BA_LAUNCH)
		{
			//Log(LOG_INFO) << ". . _projectileImpact !";
			_unit->aim(true); // set the soldier in an aiming position
			_unit->setCache(0);
			_parent->getMap()->cacheUnit(_unit);

			// and we have a lift-off
			if (_ammo->getRules()->getFireSound() != -1)
				_parent->getResourcePack()->getSound(
												"BATTLE.CAT",
												_ammo->getRules()->getFireSound())
											->play();
			else if (_action.weapon->getRules()->getFireSound() != -1)
				_parent->getResourcePack()->getSound(
												"BATTLE.CAT",
												_action.weapon->getRules()->getFireSound())
											->play();

			if (!_parent->getSave()->getDebugMode()
				&& _action.type != BA_LAUNCH
				&& _ammo->spendBullet() == false)
			{
				_parent->getSave()->removeItem(_ammo);
				_action.weapon->setAmmoItem(0);
			}
		}
		else // VOXEL_EMPTY, no line of fire
		{
			//Log(LOG_INFO) << ". no shot, no LoF, Voxel_Empty";
			delete projectile;

			_parent->getMap()->setProjectile(0);
			_action.result = "STR_NO_LINE_OF_FIRE";
			_action.TU = 0; // kL
//kL		_unit->abortTurn();
			_unit->setStatus(STATUS_STANDING); // kL
			_parent->popState();

			return false;
		}
	}

	//Log(LOG_INFO) << ". createNewProjectile() ret TRUE";
	return true;
}

/**
 * Animates the projectile (moves to the next point in its trajectory).
 * If the animation is finished the projectile sprite
 * is removed from the map, and this state is finished.
 */
void ProjectileFlyBState::think()
{
	//Log(LOG_INFO) << "ProjectileFlyBState::think()";
	_parent->getSave()->getBattleState()->clearMouseScrollingState();

	// TODO refactoring : store the projectile in this state, instead of getting it from the map each time.
	if (_parent->getMap()->getProjectile() == NULL)
	{
		Tile
			* t = _parent->getSave()->getTile(_action.actor->getPosition()),
			* tBelow = _parent->getSave()->getTile(_action.actor->getPosition() + Position(0, 0,-1));

		bool
			hasFloor = t && !t->hasNoFloor(tBelow),
			unitCanFly = _action.actor->getArmor()->getMovementType() == MT_FLY;

		if (_action.type == BA_AUTOSHOT
			&& _action.autoShotCount < _action.weapon->getRules()->getAutoShots()
			&& !_action.actor->isOut()
			&& _ammo->getAmmoQuantity() != 0
			&& (hasFloor
				|| unitCanFly))
		{
			createNewProjectile();

			if (_action.cameraPosition.z != -1) // new behavior here (14 jun 2):
			{
				_parent->getMap()->getCamera()->setMapOffset(_action.cameraPosition);
				_parent->getMap()->invalidate();
			}
		}
		else // end think()
		{
			// kL_note: Don't return camera to its pre-firing position.
			// that is, leave it wherever the projectile hits... (or try to.)
/*	BA_NONE,		// 0
	BA_TURN,		// 1
	BA_WALK,		// 2
	BA_PRIME,		// 3
	BA_THROW,		// 4
	BA_AUTOSHOT,	// 5
	BA_SNAPSHOT,	// 6
	BA_AIMEDSHOT,	// 7
	BA_STUN,		// 8
	BA_HIT,			// 9
	BA_USE,			// 10
	BA_LAUNCH,		// 11
	BA_MINDCONTROL,	// 12
	BA_PANIC,		// 13
	BA_RETHINK		// 14 */
			if (_action.cameraPosition.z != -1) // kL_note: a bit of overkill here
			{
				if (_action.type == BA_THROW // kL, don't jump screen after these.
					|| _action.type == BA_STUN
					|| _action.type == BA_HIT
					|| _action.type == BA_USE
					|| _action.type == BA_LAUNCH
					|| _action.type == BA_MINDCONTROL
					|| _action.type == BA_PANIC)
				{
//					_parent->getMap()->getCamera()->setMapOffset(_parent->getMap()->getCamera()->getMapOffset()); // kL
				}
				else if (_action.type == BA_AUTOSHOT // kL, jump screen back to pre-shot position
					|| _action.type == BA_SNAPSHOT
					|| _action.type == BA_AIMEDSHOT)
				{
					_parent->getMap()->getCamera()->setMapOffset(_action.cameraPosition);
				}

				_parent->getMap()->invalidate();
			}

			if (_action.actor->getFaction() == _parent->getSave()->getSide() // kL
				&& _action.type != BA_PANIC
				&& _action.type != BA_MINDCONTROL
				&& !_parent->getSave()->getUnitsFalling())
			{
				//Log(LOG_INFO) << "ProjectileFlyBState::think(), checkReactionFire()"
				//	<< " ID " << _action.actor->getId()
				//	<< " action.type = " << _action.type
				//	<< " action.TU = " << _action.TU;
				_parent->getTileEngine()->checkReactionFire(
														_unit,
														_action.TU); // kL
			}

			if (!_unit->isOut()
				&& _action.type != BA_HIT) // kL_note: huh?
			{
				_unit->setStatus(STATUS_STANDING);
			}

			if (_parent->getSave()->getSide() == FACTION_PLAYER
				|| _parent->getSave()->getDebugMode())
			{
				_parent->setupCursor();
			}

			_parent->popState();
		}
	}
	else // impact !
	{
		if (_action.type != BA_THROW
			&& _ammo
			&& _ammo->getRules()->getShotgunPellets() != 0)
		{
			// shotgun pellets move to their terminal location instantly as fast as possible
			_parent->getMap()->getProjectile()->skipTrajectory();
		}

		if (!_parent->getMap()->getProjectile()->move())
		{
			if (_action.type == BA_THROW)
			{
				_parent->getResourcePack()->getSound(
													"BATTLE.CAT",
													38)
												->play();

				Position pos = _parent->getMap()->getProjectile()->getPosition(-1);
				pos.x /= 16;
				pos.y /= 16;
				pos.z /= 24;

				if (pos.y > _parent->getSave()->getMapSizeY())
					pos.y--;

				if (pos.x > _parent->getSave()->getMapSizeX())
					pos.x--;

				BattleItem* item = _parent->getMap()->getProjectile()->getItem();
				if (Options::battleInstantGrenade
					&& item->getRules()->getBattleType() == BT_GRENADE
					&& item->getFuseTimer() == 0)
				{
					_parent->statePushFront(new ExplosionBState( // it's a hot grenade set to explode immediately
															_parent,
															_parent->getMap()->getProjectile()->getPosition(-1),
															item,
															_action.actor));
				}
				else
				{
					_parent->dropItem(
									pos,
									item);

					if (_unit->getFaction() != FACTION_PLAYER
						&& _projectileItem->getRules()->getBattleType() == BT_GRENADE)
					{
						_parent->getTileEngine()->setDangerZone(
															pos,
															item->getRules()->getExplosionRadius(),
															_action.actor);
					}
				}
			}
			else if (_action.type == BA_LAUNCH
				&& _action.waypoints.size() > 1
				&& _projectileImpact == -1)
			{
				_origin = _action.waypoints.front();
				_action.waypoints.pop_front();
				_action.target = _action.waypoints.front();

				// launch the next projectile in the waypoint cascade
				ProjectileFlyBState* nextWaypoint = new ProjectileFlyBState(
																		_parent,
																		_action,
																		_origin);
				nextWaypoint->setOriginVoxel(_parent->getMap()->getProjectile()->getPosition(-1));

				if (_origin == _action.target)
					nextWaypoint->targetFloor();

				_parent->statePushNext(nextWaypoint);
			}
			else // shoot.
			{
				if (_parent->getSave()->getTile(_action.target)->getUnit()) // Only counts for guns, not throws or launches
					_parent->getSave()->getTile(_action.target)->getUnit()->getStatistics()->shotAtCounter++;

				if (_ammo
					&& _action.type == BA_LAUNCH
					&& _ammo->spendBullet() == false)
				{
					_parent->getSave()->removeItem(_ammo);
					_action.weapon->setAmmoItem(0);
				}

				if (_projectileImpact != VOXEL_OUTOFBOUNDS) // NOT out of map
				{
					// explosions impact not inside the voxel but two steps back;
					// projectiles generally move 2 voxels at a time
					int offset = 0;
					if (_ammo
//kL						&& _ammo->getRules()->getExplosionRadius() != 0
						&& _ammo->getRules()->getExplosionRadius() > 0			// kL
//						&& (_ammo->getRules()->getDamageType() == DT_HE			// kL
//							|| _ammo->getRules()->getDamageType() == DT_IN)		// kL
						&& _projectileImpact != VOXEL_UNIT)
					{
						offset = -2;
					}

					//Log(LOG_INFO) << ". . . . new ExplosionBState()";
					_parent->statePushFront(new ExplosionBState(
															_parent,
															_parent->getMap()->getProjectile()->getPosition(offset),
															_ammo,
															_action.actor,
															NULL,
															_action.type != BA_AUTOSHOT
																|| _action.autoShotCount == _action.weapon->getRules()->getAutoShots()
																|| !_action.weapon->getAmmoItem()));

					// special shotgun behaviour: trace extra projectile paths,
					// and add bullet hits at their termination points.
					if (_ammo
						&& _ammo->getRules()->getShotgunPellets() != 0)
					{
						int bulletSprite = _ammo->getRules()->getBulletSprite();
						if (bulletSprite == -1)
							bulletSprite = _action.weapon->getRules()->getBulletSprite();

						int i = 1;
						while (i != _ammo->getRules()->getShotgunPellets())
						{
							Projectile* proj = new Projectile( // create a projectile
														_parent->getResourcePack(),
														_parent->getSave(),
														_action,
														_origin,
														_targetVoxel,
														bulletSprite);

							_projectileImpact = proj->calculateTrajectory( // let it trace to the point where it hits
																	std::max(
																			0.0,
																			_unit->getFiringAccuracy(
																								_action.type,
																								_action.weapon)
																							- i * 0.05)); // pellet spread.

							if (_projectileImpact != VOXEL_EMPTY)
							{
								proj->skipTrajectory(); // as above: skip the shot to the end of its path

								if (_projectileImpact != VOXEL_OUTOFBOUNDS) // insert an explosion and hit
								{
									Explosion* explosion = new Explosion(
																		proj->getPosition(1),
																		_ammo->getRules()->getHitAnimation(),
																		false,
																		false);

//									_parent->getMap()->getExplosions()->insert(explosion); // kL
									_parent->getMap()->getExplosions()->push_back(explosion); // expl CTD
									_parent->getSave()->getTileEngine()->hit(
																			proj->getPosition(1),
																			_ammo->getRules()->getPower(),
																			_ammo->getRules()->getDamageType(),
																			NULL); // kL_note: was _unit
								}

								++i;
							}

							delete proj;
						}
					}

					if (_unit->getSpecialAbility() == SPECAB_BURNFLOOR)
						_parent->getSave()->getTile(_action.target)->ignite(15);

					if (_projectileImpact == VOXEL_UNIT)
					{
						BattleUnit* victim = _parent->getSave()->getTile(
																	_parent->getMap()->getProjectile()->getPosition(offset) / Position(16, 16, 24))
																->getUnit();
						BattleUnit* target = _parent->getSave()->getTile(_action.target)->getUnit(); // target (not necessarily who we hit)
						if (victim
							&& !victim->isOut())
						{
							victim->getStatistics()->hitCounter++;
							if (_unit->getOriginalFaction() == FACTION_PLAYER
								&& victim->getOriginalFaction() == FACTION_PLAYER)
							{
								victim->getStatistics()->shotByFriendlyCounter++;
								_unit->getStatistics()->shotFriendlyCounter++;
							}

							if (victim == target) // Hit our target
							{
								_unit->getStatistics()->shotsLandedCounter++;
								if (_parent->getTileEngine()->distance(_action.actor->getPosition(), victim->getPosition()) > 30)
									_unit->getStatistics()->longDistanceHitCounter++;

								if (_unit->getFiringAccuracy(
														_action.type,
														_action.weapon) < _parent->getTileEngine()->distance(
																										_action.actor->getPosition(),
																										victim->getPosition()))
								{
									_unit->getStatistics()->lowAccuracyHitCounter++;
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
							_unit->setTurnsExposed(0);
						} */ // kL_end. But this is entirely unnecessary, since aLien has already seen and logged the soldier.
/*kL
						if (victim
							&& !victim->isOut(true, true)
							&& victim->getFaction() == FACTION_HOSTILE)
						{
							AlienBAIState* aggro = dynamic_cast<AlienBAIState*>(victim->getCurrentAIState());
							if (aggro != 0)
							{
								aggro->setWasHit(); // kL_note: is used only for spotting on RA.
								_unit->setTurnsExposed(0); // kL_note: might want to remark this! Ok.
								// technically, in the original as I remember it, only
								// a BlasterLaunch (by xCom) would set an xCom soldier Exposed here!
								// ( Those aLiens had a way of tracing a BL back to its origin ....)
							}
						}
					} */
				}
				else if (_action.type != BA_AUTOSHOT
					|| _action.autoShotCount == _action.weapon->getRules()->getAutoShots()
					|| !_action.weapon->getAmmoItem())
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
 * Flying projectiles cannot be cancelled, but they can be "skipped".
 */
void ProjectileFlyBState::cancel()
{
	Projectile* projectile = _parent->getMap()->getProjectile();
	if (projectile)
	{
		projectile->skipTrajectory();

		Position pos = projectile->getPosition();
		Camera* camera = _parent->getMap()->getCamera();
		if (!camera->isOnScreen(Position(
										pos.x / 16,
										pos.y / 16,
										pos.z / 24)))
//kL										false))
		{
			camera->centerOnPosition(Position(
											pos.x / 16,
											pos.y / 16,
											pos.z / 24));
		}
	}
}

/**
 * Validates the throwing range.
 * @param action - pointer to BattleAction struct (BattlescapeGame.h)
 * @param origin - position of origin in voxelspace
 * @param target - pointer to the targeted Tile
 * @return, true if the range is valid
 */
bool ProjectileFlyBState::validThrowRange(
		BattleAction* action,
		Position origin,
		Tile* target)
{
	//Log(LOG_INFO) << "ProjectileFlyBState::validThrowRange()";

	if (action->type != BA_THROW) // this is a celatid spit.
//		&& target->getUnit())
	{
//		offset_z = target->getUnit()->getHeight() / 2 + target->getUnit()->getFloatHeight();
		return true;
	}

	int weight = action->weapon->getRules()->getWeight();
	if (action->weapon->getAmmoItem()
		&& action->weapon->getAmmoItem() != action->weapon)
	{
		weight += action->weapon->getAmmoItem()->getRules()->getWeight();
	}

	int offset_z = 2; // kL_note: this is prob +1 (.. +2) to get things up off of the lowest voxel of a targetTile.
//	int delta_z = origin.z
//					- (((action->target.z * 24)
//						+ offset_z)
//						- target->getTerrainLevel());
	int delta_z = origin.z // voxelspace
						- (action->target.z * 24)
						- offset_z
						+ target->getTerrainLevel();
	double maxDist = static_cast<double>(
							getMaxThrowDistance( // tilespace
											weight,
											action->actor->getStats()->strength,
											delta_z)
										+ 8)
									/ 16.0;
	// Throwing Distance was roughly = 2.5 \D7 Strength / Weight
//	double range = 2.63 * static_cast<double>(action->actor->getStats()->strength / action->weapon->getRules()->getWeight()); // old code.

	int
		delta_x = action->actor->getPosition().x - action->target.x,
		delta_y = action->actor->getPosition().y - action->target.y;
	double throwDist = sqrt(static_cast<double>((delta_x * delta_x) + (delta_y * delta_y)));

	// throwing off a building of 1 level lets you throw 2 tiles further than normal range,
	// throwing up the roof of this building lets your throw 2 tiles less further
/*	int delta_z = action->actor->getPosition().z - action->target.z;
	distance -= static_cast<double>(delta_z);
	distance -= static_cast<double>(delta_z); */

	// since getMaxThrowDistance seems to return 1 less than maxDist, use "< throwDist" for this determination:
//	bool ret = static_cast<int>(throwDist) < static_cast<int>(maxDist);
	bool ret = throwDist < maxDist;
	//Log(LOG_INFO) << ". throwDist " << (int)throwDist
	//				<< " < maxDist " << (int)maxDist
	//				<< " : return " << ret;

	return ret;
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
		delta_z = 1.0;

	int dist = 0;
	while (dist < 4000) // just in case
	{
		dist += 8;

		if (delta_z < -1.0)
			curZ -= 8.0;
		else
			curZ += delta_z * 8.0;

		if (curZ < 0.0
			&& delta_z < 0.0) // roll back
		{
			delta_z = std::max(delta_z, -1.0);
			if (abs(delta_z) > 1e-10) // rollback horizontal
				dist -= static_cast<int>(curZ / delta_z);

			break;
		}

		delta_z -= static_cast<double>(50 * weight / strength) / 100.0;
		if (delta_z <= -2.0) // become falling
			break;
	}

	//Log(LOG_INFO) << ". dist = " << dist / 16;
	return dist;
}

/**
 * Set the origin voxel, used for the blaster launcher.
 * @param pos the origin voxel.
 */
void ProjectileFlyBState::setOriginVoxel(Position pos)
{
	_originVoxel = pos;
}

/**
 * Set the boolean flag to angle a blaster bomb towards the floor.
 */
void ProjectileFlyBState::targetFloor()
{
	_targetFloor = true;
}

/**
 *
 */
void ProjectileFlyBState::performMeleeAttack()
{
	//Log(LOG_INFO) << "ProjectileFlyBState::performMeleeAttack()";
	BattleUnit* target = _parent->getSave()->getTile(_action.target)->getUnit();
	int height = target->getFloatHeight() + (target->getHeight() / 2);

	Position voxel;
	_parent->getSave()->getPathfinding()->directionToVector(
														_unit->getDirection(),
														&voxel);
	voxel = _action.target * Position(16, 16, 24) + Position(
														8,
														8,
														height - _parent->getSave()->getTile(_action.target)->getTerrainLevel())
													- voxel;

	_unit->aim(true); // set the soldier in an aiming position <- hm, this also sets cacheInvalid ...
	_unit->setCache(NULL);
	_parent->getMap()->cacheUnit(_unit);

	if (_ammo->getRules()->getMeleeAttackSound() != -1) // and we have a lift-off!
//	if (_ammo->getRules()->getFireSound() != -1) // kL
	{
		_parent->getResourcePack()->getSound(
										"BATTLE.CAT",
										_ammo->getRules()->getMeleeAttackSound())
//										_ammo->getRules()->getFireSound()) // kL
									->play();
	}
	else if (_action.weapon->getRules()->getMeleeAttackSound() != -1)
//	else if (_action.weapon->getRules()->getFireSound() != -1) // kL
	{
		_parent->getResourcePack()->getSound(
										"BATTLE.CAT",
										_action.weapon->getRules()->getMeleeAttackSound())
//										_action.weapon->getRules()->getFireSound()) // kL
									->play();
	}

	if (!_parent->getSave()->getDebugMode()
		&& _action.type != BA_LAUNCH
		&& _ammo->spendBullet() == false)
	{
		_parent->getSave()->removeItem(_ammo);
		_action.weapon->setAmmoItem(NULL);
	}

	_parent->getMap()->setCursorType(CT_NONE);

	_parent->statePushNext(new ExplosionBState(
											_parent,
											voxel,
											_action.weapon,
											_action.actor,
											NULL,
											true));

//	_unit->aim(false);						// ajschult: take soldier out of aiming position <- hm, this also sets cacheInvalid ...
//	_unit->setCache(NULL);					// kL
//	_parent->getMap()->cacheUnit(_unit);	// kL
	//Log(LOG_INFO) << "ProjectileFlyBState::performMeleeAttack() EXIT";
}

}
