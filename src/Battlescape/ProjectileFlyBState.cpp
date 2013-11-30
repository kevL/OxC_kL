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

#define _USE_MATH_DEFINES

#include "ProjectileFlyBState.h"

#include <cmath>

#include "AlienBAIState.h"
#include "Camera.h"
#include "ExplosionBState.h"
#include "Map.h"
#include "Pathfinding.h"
#include "Projectile.h"
#include "TileEngine.h"

#include "../Engine/Game.h"
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
ProjectileFlyBState::ProjectileFlyBState(BattlescapeGame* parent, BattleAction action, Position origin)
	:
		BattleState(parent, action),
		_unit(0),
		_ammo(0),
		_projectileItem(0),
		_origin(origin),
		_projectileImpact(0),
		_initialized(false)
{
}

/**
 *
 */
ProjectileFlyBState::ProjectileFlyBState(BattlescapeGame* parent, BattleAction action)
	:
		BattleState(parent, action),
		_unit(0),
		_ammo(0),
		_projectileItem(0),
		_origin(action.actor->getPosition()),
		_projectileImpact(0),
		_initialized(false)
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
 * - checks if the shot is valid,
 * - calculates the base accuracy.
 */
void ProjectileFlyBState::init()
{
	Log(LOG_INFO) << "ProjectileFlyBState::init()";

	if (_initialized)
	{
		Log(LOG_INFO) << ". already initialized, EXIT";
		return;
	}

	_initialized = true;

	BattleItem* weapon = _action.weapon;
	_projectileItem = 0;

	if (!weapon) // can't shoot without weapon
	{
		Log(LOG_INFO) << ". no weapon, EXIT";
		_parent->popState();

		return;
	}

	if (!_parent->getSave()->getTile(_action.target)) // invalid target position
	{
		Log(LOG_INFO) << ". no target, EXIT";
		_parent->popState();

		return;
	}

	if (_parent->getPanicHandled()
		&& _action.actor->getTimeUnits() < _action.TU)
	{
		Log(LOG_INFO) << ". not enough time units, EXIT";
		_action.result = "STR_NOT_ENOUGH_TIME_UNITS";
		_parent->popState();

		return;
	}

	_unit = _action.actor;
	_ammo = weapon->getAmmoItem();

	if (_unit->isOut(true, true))
//		|| _unit->getHealth() == 0
//		|| _unit->getHealth() < _unit->getStunlevel())
	{
		// something went wrong - we can't shoot when dead or unconscious, or if we're about to fall over.
		Log(LOG_INFO) << ". actor is Out, EXIT";
		_parent->popState();

		return;
	}

	// kL_begin: ProjectileFlyBState::init() Give back time units; pre-end Reaction Fire.
	if (_unit->getFaction() != _parent->getSave()->getSide()) // reaction fire
	{
		if (_parent->getSave()->getTile(_action.target)->getUnit())
		{
			BattleUnit* targetUnit = _parent->getSave()->getTile(_action.target)->getUnit();
		
			if (_ammo == 0
				|| targetUnit->isOut(true, true)
				|| targetUnit != _parent->getSave()->getSelectedUnit())
			{
				_unit->setTimeUnits(_unit->getTimeUnits() + _unit->getActionTUs(_action.type, _action.weapon));
				_parent->popState();

				Log(LOG_INFO) << ". . reactionFire refund (targetUnit exists) EXIT";
				return;
			}
		}
		else
		{
			_unit->setTimeUnits(_unit->getTimeUnits() + _unit->getActionTUs(_action.type, _action.weapon));
			_parent->popState();

			Log(LOG_INFO) << ". . reactionFire refund (no targetUnit) EXIT";
			return;
		} // kL_end.

		// no ammo or target is dead: give the time units back and cancel the shot.
/*kL: above.		if (_ammo == 0
			|| !_parent->getSave()->getTile(_action.target)->getUnit()
			|| _parent->getSave()->getTile(_action.target)->getUnit()->isOut(true, true)
			|| _parent->getSave()->getTile(_action.target)->getUnit() != _parent->getSave()->getSelectedUnit())
		{
			_unit->setTimeUnits(_unit->getTimeUnits() + _unit->getActionTUs(_action.type, _action.weapon));
			_parent->popState();

			return;
		} */
	}

	// autoshot will default back to snapshot if it's not possible
	// kL_note: should this be done *before* tu expenditure?!! Ok it is,
	// popState() does tu costs
	if (weapon->getRules()->getAccuracyAuto() == 0
		&& _action.type == BA_AUTOSHOT)
	{
		_action.type = BA_SNAPSHOT;
	}

	// snapshot defaults to "hit" if it's a melee weapon
	// (in case of reaction "shots" with a melee weapon),
	// for Silacoid attack, etc.
	// kL_note: not sure that melee reactionFire is properly implemented...
	if (weapon->getRules()->getBattleType() == BT_MELEE
		&& _action.type == BA_SNAPSHOT)
	{
		//Log(LOG_INFO) << ". convert BA_SNAPSHOT to BA_HIT";
		_action.type = BA_HIT;
	}


	switch (_action.type)
	{
		case BA_SNAPSHOT:
		case BA_AIMEDSHOT:
		case BA_AUTOSHOT:
		case BA_LAUNCH:
			Log(LOG_INFO) << ". . BA_SNAPSHOT/AIMEDSHOT/AUTOSHOT/LAUNCH";

			if (_ammo == 0)
			{
				Log(LOG_INFO) << ". . . no ammo, EXIT";

				_action.result = "STR_NO_AMMUNITION_LOADED";
				_parent->popState();

				return;
			}

			if (_ammo->getAmmoQuantity() == 0)
			{
				Log(LOG_INFO) << ". . . no ammo Quantity, EXIT";

				_action.result = "STR_NO_ROUNDS_LEFT";
				_parent->popState();

				return;
			}

			if (weapon->getRules()->getRange() != 0
				&& _parent->getTileEngine()->distance(_action.actor->getPosition(), _action.target) > weapon->getRules()->getRange())
			{
				Log(LOG_INFO) << ". . . out of range, EXIT";

				_action.result = "STR_OUT_OF_RANGE";
				_parent->popState();

				return;
			}
		break;
		case BA_THROW:
		{
			Log(LOG_INFO) << ". . BA_THROW";

//Wb.			Position originVoxel = _parent->getTileEngine()->getSightOriginVoxel(_unit) - Position(0, 0, 2);
			Position originVoxel = _parent->getTileEngine()->getOriginVoxel(_action, 0);	// Wb.

			if (!validThrowRange(&_action, originVoxel, _parent->getSave()->getTile(_action.target)))
			{
				Log(LOG_INFO) << ". . . not valid throw range, EXIT";

				_action.result = "STR_OUT_OF_RANGE";
				_parent->popState();

				return;
			}

			_projectileItem = weapon;
		}
		break;
		case BA_HIT:
			Log(LOG_INFO) << ". . BA_HIT";

			if (!_parent->getTileEngine()->validMeleeRange(
					_action.actor->getPosition(),
					_action.actor->getDirection(),
					_action.actor,
					0))
			{
				Log(LOG_INFO) << ". . . out of hit range, EXIT";

				_action.result = "STR_THERE_IS_NO_ONE_THERE";
				_parent->popState();

				return;
			}
		break;
		case BA_PANIC:
		case BA_MINDCONTROL:
			Log(LOG_INFO) << ". . BA_PANIC/MINDCONTROL, new ExplosionBState, EXIT";

			_parent->statePushFront(new ExplosionBState(
					_parent,
					Position(
						(_action.target.x * 16) + 8,
						(_action.target.y * 16) + 8,
						(_action.target.z * 24) + 10),
					weapon,
					_action.actor));

			return;
		break;

		default:
			Log(LOG_INFO) << ". . default, EXIT";

			_parent->popState();

			return;
		break;
	}

	createNewProjectile();
	Log(LOG_INFO) << "ProjectileFlyBState::init() EXIT";
}

/**
 * Tries to create a projectile sprite and add it to the map,
 * calculating its trajectory.
 * @return, True if the projectile was successfully created.
 */
bool ProjectileFlyBState::createNewProjectile()
{
	Log(LOG_INFO) << "ProjectileFlyBState::createNewProjectile() -> create Projectile";

	++_action.autoShotCounter;

	Projectile* projectile = new Projectile(
									_parent->getResourcePack(),
									_parent->getSave(),
									_action,
									_origin);

	// add the projectile on the map
	_parent->getMap()->setProjectile(projectile);

	// set the speed of the state think cycle to 16 ms (roughly one think cycle per frame)
//kL	_parent->setStateInterval(1000/60);
//	Uint32 interval = static_cast<Uint32>(50.0 / 3.0);	// kL
	Uint32 interval = static_cast<Uint32>(16);			// kL
	_parent->setStateInterval(interval);				// kL

	// let it calculate a trajectory
	_projectileImpact = VOXEL_EMPTY;

	if (_action.type == BA_THROW)
	{
		_projectileImpact = projectile->calculateThrow(_unit->getThrowingAccuracy());
		Log(LOG_INFO) << ". BA_THROW, part = " << _projectileImpact;

		if (_projectileImpact == VOXEL_FLOOR
			|| _projectileImpact == VOXEL_UNIT
			|| _projectileImpact == VOXEL_OBJECT)
		{
			if (_unit->getFaction() != FACTION_PLAYER
				&& _projectileItem->getRules()->getBattleType() == BT_GRENADE)
			{
				_projectileItem->setExplodeTurn(0);
			}

			_projectileItem->moveToOwner(0);
			_unit->setCache(0);
			_parent->getMap()->cacheUnit(_unit);

			_parent->getResourcePack()->getSound("BATTLE.CAT", 39)->play();

			_unit->addThrowingExp();
		}
		else // unable to throw here
		{
			Log(LOG_INFO) << ". . no throw, Voxel_Empty or _Wall or _OutofBounds";

			delete projectile;

			_parent->getMap()->setProjectile(0);
			_action.result = "STR_UNABLE_TO_THROW_HERE";
			_action.TU = 0;
			_parent->popState();

			return false;
		}
	}
	else if (_action.weapon->getRules()->getArcingShot()) // special code for the "spit" trajectory
	{
		_projectileImpact = projectile->calculateThrow(_unit->getFiringAccuracy(_action.type, _action.weapon));
		Log(LOG_INFO) << ". acid spit, part = " << _projectileImpact;

		if (_projectileImpact != VOXEL_EMPTY)
		{
			_unit->aim(true); // set the soldier in an aiming position
			_parent->getMap()->cacheUnit(_unit);

			// and we have a lift-off
			if (_action.weapon->getRules()->getFireSound() != -1)
				_parent->getResourcePack()->getSound("BATTLE.CAT", _action.weapon->getRules()->getFireSound())->play();

			if (!_parent->getSave()->getDebugMode()
				&& _action.type != BA_LAUNCH
				&& _ammo->spendBullet() == false)
			{
				_parent->getSave()->removeItem(_ammo);
				_action.weapon->setAmmoItem(0);
			}
		}
		else // no line of fire
		{
			Log(LOG_INFO) << ". . no spit, no LoF, Voxel_Empty";

			delete projectile;

			_parent->getMap()->setProjectile(0);
			_action.result = "STR_NO_LINE_OF_FIRE";
			_parent->popState();

			return false;
		}
	}
	else if (_action.type == BA_HIT) // kL. Let's not calculate anything we don't have to for meleeHits!
	{
		// validMeleeRange/target has been validated.
//		_projectileImpact = 4;
		_projectileImpact = projectile->calculateTrajectory(_unit->getFiringAccuracy(_action.type, _action.weapon));
		Log(LOG_INFO) << ". melee attack! part = " << _projectileImpact;

		_unit->aim(true); // set the soldier in an aiming position
		_parent->getMap()->cacheUnit(_unit);

		// and we have a hit!
		if (_action.weapon->getRules()->getFireSound() != -1)
			_parent->getResourcePack()->getSound("BATTLE.CAT", _action.weapon->getRules()->getFireSound())->play();

/*		if (!_parent->getSave()->getDebugMode()
			&& _action.type != BA_LAUNCH
			&& _ammo->spendBullet() == false)
		{
			_parent->getSave()->removeItem(_ammo);
			_action.weapon->setAmmoItem(0);
		} */
	}
	else // shoot weapon / was do melee attack too
	{
		// kL_note: what, you recalculate the trajectory after it was already done above,
		// and, don't even use it other than to find out if it already hit or not?!??
		// Not to mention that melee attacks ***don't even need to use a trajectory***
		_projectileImpact = projectile->calculateTrajectory(_unit->getFiringAccuracy(_action.type, _action.weapon));
		Log(LOG_INFO) << ". shoot weapon, part = " << _projectileImpact;

		if (_projectileImpact != VOXEL_EMPTY
			|| _action.type == BA_LAUNCH)
		{
			Log(LOG_INFO) << ". . _projectileImpact !";

			_unit->aim(true); // set the soldier in an aiming position
			_parent->getMap()->cacheUnit(_unit);

			// and we have a lift-off
			if (_action.weapon->getRules()->getFireSound() != -1)
				_parent->getResourcePack()->getSound("BATTLE.CAT", _action.weapon->getRules()->getFireSound())->play();

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
			Log(LOG_INFO) << ". no shot, no LoF, Voxel_Empty";

			delete projectile;

			_parent->getMap()->setProjectile(0);
			_action.result = "STR_NO_LINE_OF_FIRE";
			_parent->popState();

			return false;
		}
	}

	Log(LOG_INFO) << ". createNewProjectile() ret TRUE";
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

	/* TODO refactoring : store the projectile in this state, instead of getting it from the map each time? */
	if (_parent->getMap()->getProjectile() == 0)
	{
		Tile* t = _parent->getSave()->getTile(_action.actor->getPosition());
		Tile* tBelow = _parent->getSave()->getTile(_action.actor->getPosition() + Position(0, 0, -1));

		bool hasFloor = t && !t->hasNoFloor(tBelow);
		bool unitCanFly = _action.actor->getArmor()->getMovementType() == MT_FLY;

		if (_action.type == BA_AUTOSHOT
			&& _action.autoShotCounter < _action.weapon->getRules()->getAutoShots()
			&& !_action.actor->isOut()
			&& _ammo->getAmmoQuantity() != 0
			&& (hasFloor || unitCanFly))
		{
			createNewProjectile();
		}
		else
		{
			if (_action.cameraPosition.z != -1)
			{
				_parent->getMap()->getCamera()->setMapOffset(_action.cameraPosition);
			}

			if (_action.type != BA_PANIC
				&& _action.type != BA_MINDCONTROL
				&& !_parent->getSave()->getUnitsFalling())
			{
				_parent->getTileEngine()->checkReactionFire(_unit);
			}

			if (!_action.actor->isOut())
			{
				_unit->abortTurn();
			}

			_parent->popState();
		}
	}
	else // impact !
	{
		if (!_parent->getMap()->getProjectile()->move())
		{
			if (_action.type == BA_THROW)
			{
				_parent->getResourcePack()->getSound("BATTLE.CAT", 38)->play();

				Position pos = _parent->getMap()->getProjectile()->getPosition(-1);
				pos.x /= 16;
				pos.y /= 16;
				pos.z /= 24;

				if (pos.y > _parent->getSave()->getMapSizeY())
				{
					pos.y--;
				}
				if (pos.x > _parent->getSave()->getMapSizeX())
				{
					pos.x--;
				}

				BattleItem* item = _parent->getMap()->getProjectile()->getItem();
				if (Options::getBool("battleInstantGrenade")
					&& item->getRules()->getBattleType() == BT_GRENADE
					&& item->getExplodeTurn() == 0)
				{
					// it's a hot grenade to explode immediately
					_parent->statePushFront(new ExplosionBState(
							_parent,
							_parent->getMap()->getProjectile()->getPosition(-1),
							item,
							_action.actor));
				}
				else
				{
					_parent->dropItem(pos, item);
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
				_parent->statePushNext(new ProjectileFlyBState(_parent, _action, _origin));
			}
			else
			{
				if (_ammo
					&& _action.type == BA_LAUNCH
					&& _ammo->spendBullet() == false)
				{
					_parent->getSave()->removeItem(_ammo);
					_action.weapon->setAmmoItem(0);
				}

				if (_projectileImpact != 5) // NOT out of map
				{
					// explosions impact not inside the voxel but two steps back (projectiles generally move 2 voxels at a time)
					int offset = 0;
					if (_ammo
						&& (_ammo->getRules()->getDamageType() == DT_HE
							|| _ammo->getRules()->getDamageType() == DT_IN))
					{
						offset = -2;
					}

					//Log(LOG_INFO) << ". . . . new ExplosionBState()";
					_parent->statePushFront(new ExplosionBState(
															_parent, _parent->getMap()->getProjectile()->getPosition(offset),
															_ammo,
															_action.actor,
															0,
															_action.type != BA_AUTOSHOT
																|| _action.autoShotCounter == _action.weapon->getRules()->getAutoShots()
																|| !_action.weapon->getAmmoItem()));

					if (_unit->getSpecialAbility() == SPECAB_BURNFLOOR)
					{
						_parent->getSave()->getTile(_action.target)->ignite(15);
					}

					if (_projectileImpact == 4)
					{
						BattleUnit* victim = _parent->getSave()
								->getTile(_parent->getMap()->getProjectile()->getPosition(offset) / Position(16, 16, 24))->getUnit();
						if (victim
							&& !victim->isOut()
							&& victim->getFaction() == FACTION_HOSTILE)
						{
							AlienBAIState* aggro = dynamic_cast<AlienBAIState*>(victim->getCurrentAIState());
							if (aggro != 0)
							{
								aggro->setWasHit();
								_unit->setTurnsExposed(0);
							}
						}
					}
				}
				else if (_action.type != BA_AUTOSHOT
					|| _action.autoShotCounter == _action.weapon->getRules()->getAutoShots()
					|| !_action.weapon->getAmmoItem())
				{
					_unit->aim(false);
					_parent->getMap()->cacheUnits();
				}
			}

			delete _parent->getMap()->getProjectile();
			_parent->getMap()->setProjectile(0);
		}
	}

	//Log(LOG_INFO) << "ProjectileFlyBState::think() EXIT";
}

/**
 * Flying projectiles cannot be cancelled, but they can be "skipped".
 */
void ProjectileFlyBState::cancel()
{
	if (_parent->getMap()->getProjectile())
	{
		_parent->getMap()->getProjectile()->skipTrajectory();

		Position p = _parent->getMap()->getProjectile()->getPosition();
		if (!_parent->getMap()->getCamera()->isOnScreen(Position(p.x / 16, p.y / 16, p.z / 24)))
			_parent->getMap()->getCamera()->centerOnPosition(Position(p.x / 16, p.y / 16, p.z / 24));
	}
}

/**
 * Validates the throwing range.
 * @param action, The BattleAction struct
 * @param origin, Position of origin in voxelspace
 * @param target, The target tile
 * @return, True when the range is valid
 */
bool ProjectileFlyBState::validThrowRange(
		BattleAction* action,
		Position origin,
		Tile* target)
{
	Log(LOG_INFO) << "ProjectileFlyBState::validThrowRange()";

	int weight = action->weapon->getRules()->getWeight();
	if (action->weapon->getAmmoItem()
		&& action->weapon->getAmmoItem() != action->weapon)
	{
		weight += action->weapon->getAmmoItem()->getRules()->getWeight();
	}

	int offset_z = 2;				// kL_note: this is prob +1 (.. +2) to get things up off of the lowest voxel of a targetTile.
	if (action->type != BA_THROW	// kL_note: huh? if *NOT* throw???
		&& target->getUnit())		// but if there is a unit in targetTile??
									// ah okay, this is a celatid spit.
	{
		offset_z = target->getUnit()->getHeight() / 2 + target->getUnit()->getFloatHeight();
	}

	int delta_z = origin.z - (((action->target.z * 24) + offset_z) - target->getTerrainLevel());
	double maxDistance = static_cast<double>(getMaxThrowDistance(weight, action->actor->getStats()->strength, delta_z) + 8) / 16.;
	// Throwing Distance was roughly = 2.5 \D7 Strength / Weight
//	double range = 2.63 * static_cast<double>(action->actor->getStats()->strength / action->weapon->getRules()->getWeight()); // old code.

	int delta_x = action->actor->getPosition().x - action->target.x;
	int delta_y = action->actor->getPosition().y - action->target.y;
	double realDistance = sqrt(static_cast<double>((delta_x * delta_x) + (delta_y * delta_y)));

	// throwing off a building of 1 level lets you throw 2 tiles further than normal range,
	// throwing up the roof of this building lets your throw 2 tiles less further
/*	int delta_z = action->actor->getPosition().z - action->target.z;
	distance -= static_cast<double>(delta_z);
	distance -= static_cast<double>(delta_z); */

	// since getMaxThrowDistance seems to return 1 less than maxDistance, use "< realDistance" for this determination:
	bool ret = static_cast<int>(realDistance) < static_cast<int>(maxDistance);
	Log(LOG_INFO) << ". realDistance " << (int)realDistance << " < maxDistance " << (int)maxDistance << " : return " << ret;

	return ret;
}

/**
 *
 */
int ProjectileFlyBState::getMaxThrowDistance(int weight, int strength, int level)
{
	Log(LOG_INFO) << "ProjectileFlyBState::getMaxThrowDistance()";

	double curZ = level + 0.5;
	double delta_z = 1.0;

	int dist = 0;
	while (dist < 4000) // just in case
	{
		dist += 8;
		if (delta_z < -1.0)
			curZ -= 8.0;
		else
			curZ += delta_z * 8.0;

		if (curZ < 0.0 && delta_z < 0.0) // roll back
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

	Log(LOG_INFO) << ". dist = " << dist / 16;
	return dist;
}

}
