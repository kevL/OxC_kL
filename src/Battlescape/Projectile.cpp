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

#include "Projectile.h"

#include <cmath>

#include "BattlescapeGame.h"
#include "Position.h"
#include "TileEngine.h"

#include "../aresame.h"

#include "../Engine/Game.h"
#include "../Engine/Options.h"
#include "../Engine/RNG.h"
#include "../Engine/Surface.h"
#include "../Engine/SurfaceSet.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/Armor.h"
#include "../Ruleset/MapData.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/RuleSoldier.h"
#include "../Ruleset/Unit.h"

#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/Tile.h"

//Old #include "../Battlescape/Position.h"
//Old #include "../Battlescape/TileEngine.h"


namespace OpenXcom
{

/**
 * Sets up a UnitSprite with the specified size and position.
 * @param res Pointer to resourcepack.
 * @param save Pointer to battlesavegame.
 * @param action An action.
 * @param origin Position the projectile originates from.
 */
Projectile::Projectile(
		ResourcePack* res,
		SavedBattleGame* save,
		BattleAction action,
		Position origin)
	:
		_res(res),
		_save(save),
		_action(action),
		_origin(origin),
		_position(0)
{
	// this is the number of pixels the sprite will move between frames
	_speed = Options::getInt("battleFireSpeed");

	if (_action.weapon)
	{
		if (_action.type == BA_THROW)
		{
			Log(LOG_INFO) << "Create Projectile -> BA_THROW";

			_speed = _speed * 5 / 7;	// kL
			_sprite =_res->getSurfaceSet("FLOOROB.PCK")->getFrame(getItem()->getRules()->getFloorSprite());
		}
		else // ba_SHOOT!! or hit, or spit.... probly Psi-attack also.
		{
			Log(LOG_INFO) << "Create Projectile -> not BA_THROW";

			_speed = std::max(1, _speed + _action.weapon->getRules()->getBulletSpeed());
		}
	}
}

/**
 * Deletes the Projectile.
 */
Projectile::~Projectile()
{
}

/**
 * Calculates the trajectory for a straight path.
 * @param accuracy, The unit's accuracy.
 * @return, The objectnumber(0-3) or unit(4) or out of map (5) or -1 (no line of fire).
 */
int Projectile::calculateTrajectory(double accuracy)
{
	Log(LOG_INFO) << "Projectile::calculateTrajectory()";

	Position originVoxel, targetVoxel;
	Tile* targetTile = 0;
	BattleUnit* bu = _action.actor;

	originVoxel = _save->getTileEngine()->getOriginVoxel(_action, _save->getTile(_origin)); // Wb.131129


	//int dirYshift[24] = {1, 3, 9, 15, 15, 13, 7, 1,  1, 1, 7, 13, 15, 15, 9, 3,  1, 2, 8, 14, 15, 14, 8, 2};
	//int dirXshift[24] = {9, 15, 15, 13, 8, 1, 1, 3,  7, 13, 15, 15, 9, 3, 1, 1,  8, 14, 15, 14, 8, 2, 1, 2};
	// maybe if i get around to making that function to calculate a firepoint origin for fire
	// point estimations i'll use the array above so i'll leave it commented for the time being.

/*
Wb.131129
	originVoxel = Position(
			_origin.x * 16,
			_origin.y * 16,
			_origin.z * 24);

	// tanks etc. shoot from lower right corner(center of unit) of primary(upper left) part.
	// kL_note: "bu" looks redundant w/ "_action.actor" ...

	// take into account soldier height and terrain level if the projectile is launched from a soldier
	if (_action.actor->getPosition() == _origin)
	{
		// calculate offset of the starting point of the projectile
		originVoxel.z +=
				bu->getHeight()
					+ bu->getFloatHeight()
					-_save->getTile(_origin)->getTerrainLevel()
					- 4; // 2 voxels lower than LoS origin.

		if (originVoxel.z >= (_origin.z + 1) * 24)
		{
			Tile* tileAbove = _save->getTile(_origin + Position(0, 0, 1));
			if (tileAbove
				&& tileAbove->hasNoFloor(0))
			{
				_origin.z++;
			}
			else
			{
				while (originVoxel.z >= (_origin.z + 1) * 24)
				{
					originVoxel.z--;
				}

				originVoxel.z -= 4; // keep originVoxel 4 voxels below any ceiling.
			}
		}

		// originally used the dirXShift and dirYShift as detailed above, this however results in MUCH more predictable results.
		// center Origin in the originTile (or the center of all four tiles for large units):
		int offset = bu->getArmor()->getSize() * 8;
		originVoxel.x += offset;
		originVoxel.y += offset;
	}
	else // _action.actor is NOT in originTile.
	{
		// don't take into account soldier height and terrain level if the
		// projectile is not launched from a soldier (ie. from a waypoint)
		originVoxel.x += 8;
		originVoxel.y += 8;
		originVoxel.z += 12;
	}
*/

	// TARGETTING //
	if (_action.type == BA_LAUNCH
		|| (SDL_GetModState() & KMOD_CTRL) != 0
		|| !_save->getBattleGame()->getPanicHandled())
		// kL_note: Ctrl+LMB could check for tile-parts... cf below.
	{
		// target nothing, targets the middle of the tile
		targetVoxel = Position(
				_action.target.x * 16 + 8,
				_action.target.y * 16 + 8,
				_action.target.z * 24 + 12);	// Wb. changed this to +16
												// prob. to give it a flat trajectory from firing pt.
												// i Like it aimed slightly down perhaps...
	}
	else // non-waypointed attack follows
	{
		// determine the target voxel.
		// aim at the center of the unit, the object, the walls or the floor (in that priority)
		// if there is no LOF to the center, try elsewhere outward.
		// Then store this target voxel.
		targetTile = _save->getTile(_action.target);

		if (targetTile->getUnit() != 0) // aiming at Unit.
		{
			Log(LOG_INFO) << ". targetTile has unit";

			if (_origin == _action.target
				|| targetTile->getUnit() == _action.actor)
			{
				// don't shoot at yourself but shoot at the floor
				// kL_note: this shot at walls in Orig.
				targetVoxel = Position(
						_action.target.x * 16 + 8,
						_action.target.y * 16 + 8,
//kL						_action.target.z * 24);
						_action.target.z * 24 + 1);		// kL
			}
			else	// kL_note: huh? Is this for storing _trajectory??? ... no. might be
					// setting &targetVoxel tho. Or "_action.target" ( targetTile ) even......
			{
				_save->getTileEngine()->canTargetUnit(
						&originVoxel,
						targetTile,
						&targetVoxel,
						bu);
			}
		}
		else if (targetTile->getMapData(MapData::O_OBJECT) != 0) // aiming at content-Object.
		{
			Log(LOG_INFO) << ". targetTile has content-object";

			if (!_save->getTileEngine()->canTargetTile(
					&originVoxel,
					targetTile,
					MapData::O_OBJECT,
					&targetVoxel,
					bu))
			{
				targetVoxel = Position(
						_action.target.x * 16 + 8,
						_action.target.y * 16 + 8,
//kL						_action.target.z * 24 + 8);
						_action.target.z * 24 + 6);		// kL
			}
		}
		else if (targetTile->getMapData(MapData::O_NORTHWALL) != 0) // aiming at Northwall
		{
			Log(LOG_INFO) << ". targetTile has northwall";

			if (!_save->getTileEngine()->canTargetTile(
					&originVoxel,
					targetTile,
					MapData::O_NORTHWALL,
					&targetVoxel,
					bu))
			{
				targetVoxel = Position(
						_action.target.x * 16 + 8,
//kL						_action.target.y * 16,
						_action.target.y * 16 + 1,		// kL
//kL						_action.target.z * 24 + 9);
						_action.target.z * 24 + 10);	// kL
			}
		}
		else if (targetTile->getMapData(MapData::O_WESTWALL) != 0) // aiming at Westwall
		{
			Log(LOG_INFO) << ". targetTile has westwall";

			if (!_save->getTileEngine()->canTargetTile(
					&originVoxel,
					targetTile,
					MapData::O_WESTWALL,
					&targetVoxel,
					bu))
			{
				targetVoxel = Position(
//kL						_action.target.x * 16,
						_action.target.x * 16 + 1,		// kL
						_action.target.y * 16 + 8,
//kL						_action.target.z * 24 + 9);
						_action.target.z * 24 + 10);	// kL
			}
		}
		else if (targetTile->getMapData(MapData::O_FLOOR) != 0) // aiming at Floor
		{
			Log(LOG_INFO) << ". targetTile has floor";

			// kL_note: This is not allowing floortiles to be targetted properly.
			// Wb did an update, so check it out.... +2 voxels on the z-axis
			if (!_save->getTileEngine()->canTargetTile(
					&originVoxel,
					targetTile,
					MapData::O_FLOOR,
					&targetVoxel,
					bu))
			{
				//Log(LOG_INFO) << ". can't target floorTile";

				targetVoxel = Position(
						_action.target.x * 16 + 8,
						_action.target.y * 16 + 8,
//kL						_action.target.z * 24 + 2);
						_action.target.z * 24 + 1);		// kL
			}
		}
		else // aiming at empty space.
		{
			Log(LOG_INFO) << ". targetTile is void";

			// target nothing, targets the middle of the tile
			targetVoxel = Position(
					_action.target.x * 16 + 8,
					_action.target.y * 16 + 8,
					_action.target.z * 24 + 12);
		}


		int test = VOXEL_EMPTY;
		test = _save->getTileEngine()->calculateLine(
												originVoxel,
												targetVoxel,
												false,
												&_trajectory,
												bu);

		//Log(LOG_INFO) << ". test = " << test;

		if (test != VOXEL_EMPTY
				&& !_trajectory.empty()
				&& _action.actor->getFaction() == FACTION_PLAYER
				&& _action.autoShotCounter == 1)
		{
			Position hitPos = Position(
					_trajectory.at(0).x / 16,
					_trajectory.at(0).y / 16,
					_trajectory.at(0).z / 24);

			if (test == VOXEL_UNIT
					&& _save->getTile(hitPos)
					&& _save->getTile(hitPos)->getUnit() == 0)
			{
				// must be poking head up from the belowTile
				hitPos = Position(hitPos.x, hitPos.y, hitPos.z - 1);
			}

			if (hitPos != _action.target
				&& _action.result == "")
			{
				//Log(LOG_INFO) << ". . hitPos != target";

				if (test == VOXEL_NORTHWALL) // re-calculate for Northwall south of targetTile
				{
					//Log(LOG_INFO) << ". . . test == 2";

					if (hitPos.y - 1 != _action.target.y)
//					if (hitPos.y - 1 == _action.target.y)
					{
						//Log(LOG_INFO) << ". . . . no Acu modifi perhaps";

						_trajectory.clear();
						return VOXEL_EMPTY;
/*						return _save->getTileEngine()->calculateLine(
								originVoxel,
								targetVoxel,
								true,
								&_trajectory,
								bu); */
					}
				}
				else if (test == VOXEL_WESTWALL) // re-calculate for Westwall east of targetTile
				{
					//Log(LOG_INFO) << ". . . test == 1";

					if (hitPos.x - 1 != _action.target.x)
//					if (hitPos.x - 1 == _action.target.x)
					{
						//Log(LOG_INFO) << ". . . . no Acu modifi perhaps";
						_trajectory.clear();
						return VOXEL_EMPTY;
/*						return _save->getTileEngine()->calculateLine(
								originVoxel,
								targetVoxel,
								true,
								&_trajectory,
								bu); */
					}
				}
/*				// kL_begin: Projectile::calculateTrajectory() NOW TARGETS FLOORS.
				else if (test == 0)
				{
					//Log(LOG_INFO) << ". . . test == 0";

//					if (hitPos.x - 1 != _action.target.x)
//					if (hitPos.z - 1 == _action.target.z)
//					{
						//Log(LOG_INFO) << ". . . . no Acu modifi perhaps";

					_trajectory.clear();

//					return -1;
					return _save->getTileEngine()->calculateLine(
							originVoxel,
							targetVoxel,
							true,
							&_trajectory,
							bu);
//					}
				} */// kL_end.
				else if (test == VOXEL_UNIT)
				{
					BattleUnit* hitUnit = _save->getTile(hitPos)->getUnit();
					BattleUnit* targetUnit = targetTile->getUnit();

					if (hitUnit != targetUnit)
					{
						_trajectory.clear();

						return VOXEL_EMPTY;
					}
				}
				else // test == 3, or something much higher.
				{
					_trajectory.clear();

					return VOXEL_EMPTY;
				}
			}
		}
	}

	_trajectory.clear();

	bool extendLine = true;
	// even guided missiles drift, but how much is based on
	// the shooter's faction, rather than accuracy.
	// Wb.131216
/*kL	if (_action.type == BA_LAUNCH)
	{
		extendLine = _action.waypoints.size() < 2;

		if (_action.actor->getFaction() == FACTION_PLAYER)
		{
			accuracy = 0.60;
		}
		else
		{
			accuracy = 0.55;
		}
	} */

	// apply some accuracy modifiers. This will result in a new target voxel:
	if (_action.type != BA_LAUNCH) // <- what, no drift??!?
		applyAccuracy(
					originVoxel,
					&targetVoxel,
					accuracy,
					false,
					targetTile,
					extendLine);

	//Log(LOG_INFO) << ". LoF calculated, Acu applied (if not BL)";
	// finally do a line calculation and store this trajectory.
	int ret = _save->getTileEngine()->calculateLine(
													originVoxel,
													targetVoxel,
													true,
													&_trajectory,
													bu);

	Log(LOG_INFO) << ". ret = " << ret;
	return ret;
}

/**
 * Calculates the trajectory for a curved path.
 * @param accuracy, The unit's accuracy.
 * @return int, The objectnumber(0-3) or unit(4) or out of map (5) or -1(hit nothing).
 */
int Projectile::calculateThrow(double accuracy)
{
	Log(LOG_INFO) << "Projectile::calculateThrow(), cf TileEngine::validateThrow()";

	// object blocking - can't throw here
/*
Wb.131129
	if (_action.type == BA_THROW
		&& _save->getTile(_action.target)
		&& _save->getTile(_action.target)->getMapData(MapData::O_OBJECT)
		&& _save->getTile(_action.target)->getMapData(MapData::O_OBJECT)->getTUCost(MT_WALK) == 255)
	{
		return VOXEL_EMPTY;
	}

	Position originVoxel;
	originVoxel = Position(
			_origin.x * 16 + 8,
			_origin.y * 16 + 8,
			_origin.z * 24);
	originVoxel.z += -_save->getTile(_origin)->getTerrainLevel();

	originVoxel.z += bu->getHeight() + bu->getFloatHeight();
	originVoxel.z -= 3;

	if (originVoxel.z >= (_origin.z + 1) * 24)
	{
		Tile* tileAbove = _save->getTile(_origin + Position(0, 0, 1));
		if (!tileAbove
			|| !tileAbove->hasNoFloor(0))
		{
			while (originVoxel.z > (_origin.z + 1) * 24)
			{
				originVoxel.z--;
			}

			originVoxel.z -= 4;
		}
		else
		{
			_origin.z++;
		}
	}
*/

	BattleUnit* bu;
	if (_save->getTile(_origin)->getUnit())
	{
		bu = _save->getTile(_origin)->getUnit();
	}
	else if (_save->getTile(Position(
			_origin.x,
			_origin.y,
			_origin.z - 1))->
					getUnit())
	{
		bu = _save->getTile(Position(
				_origin.x,
				_origin.y,
				_origin.z - 1))->
						getUnit();
	}

	Position originVoxel = _save->getTileEngine()->getOriginVoxel(_action, 0); // Wb.131129

	// determine the target voxel; aim at the center of the floor
	Position targetVoxel;
	targetVoxel = Position(
			(_action.target.x * 16) + 8,
			(_action.target.y * 16) + 8,
//kL			(_action.target.z * 24) + 2);
			(_action.target.z * 24) + 1);		// kL
	targetVoxel.z -= _save->getTile(_action.target)->getTerrainLevel();

	if (_action.type != BA_THROW) // celatid acid-spit
	{
		BattleUnit* targetUnit;
		if (_save->getTile(_action.target)->getUnit())
		{
			targetUnit = _save->getTile(_action.target)->getUnit();
		}

		if (!targetUnit
			&& _action.target.z > 0
			&& _save->getTile(_action.target)->hasNoFloor(0))
		{
			if (_save->getTile(Position(
					_action.target.x,
					_action.target.y,
					_action.target.z - 1))
							->getUnit())
			{
				targetUnit = _save->getTile(Position(
						_action.target.x,
						_action.target.y,
						_action.target.z - 1))
								->getUnit();
			}
		}

		if (targetUnit)
		{
			targetVoxel.z += (targetUnit->getHeight() / 2) + targetUnit->getFloatHeight();
		}
	}

	// we try several different arcs to try and reach our goal.
//	double arc = 0.5; // start with a very low traj.5 seems too low.
	int ret = VOXEL_EMPTY;
	bool found = false;
	double arc = 1.0;

	while (!found && arc < 5.0)
	{
		int check = _save->getTileEngine()->calculateParabola(
														originVoxel,
														targetVoxel,
														false,
														&_trajectory,
														bu,
														arc,
														1.0);

		if (check != VOXEL_OUTOFBOUNDS // out of map
			&& (_trajectory.at(0) / Position(16, 16, 24)) == (targetVoxel / Position(16, 16, 24)))
		{
			ret = check;

			found = true;
		}
		else
		{
			arc += 0.5;
		}

		_trajectory.clear();
	}
	Log(LOG_INFO) << ". arc = " << arc;

	if (arc >= 5.0)
	{
		return VOXEL_EMPTY;
	}

	// apply some accuracy modifiers
	if (accuracy > 1.0) accuracy = 1.0;

	static const double maxDeviation = 0.08;
	static const double minDeviation = 0.0;
	double baseDeviation = (maxDeviation - (maxDeviation * accuracy)) + minDeviation;
//Old	double deviation = RNG::boxMuller(0.0, baseDeviation);
	Log(LOG_INFO) << ". baseDeviation = " << baseDeviation;

	_trajectory.clear();
	int result = VOXEL_OUTOFBOUNDS;

	// finally do a line calculation and store this trajectory.
/*	ret = _save->getTileEngine()->calculateParabola(
			originVoxel,
			targetVoxel,
			true,
			&_trajectory,
			bu,
			arc,
			1. + deviation); */

	// finally do a line calculation and store this trajectory, make sure it's valid.
	while (result == VOXEL_OUTOFBOUNDS)
	{
		double deviation = RNG::boxMuller(0, baseDeviation);
		Log(LOG_INFO) << ". . deviation = " << deviation + 1.0;

		_trajectory.clear();
		result = _save->getTileEngine()->calculateParabola(
													originVoxel,
													targetVoxel,
													true,
													&_trajectory,
													bu,
													arc,
													1.0 + deviation);

		Position endPoint = _trajectory.back();
		endPoint.x /= 16;
		endPoint.y /= 16;
		endPoint.z /= 24;

		// check if the item would land on a tile with a blocking object
		// OLD. let it fly without deviation, it must land on a valid tile in that case
		if (_action.type == BA_THROW
			&& _save->getTile(endPoint)
			&& _save->getTile(endPoint)->getMapData(MapData::O_OBJECT)
			&& _save->getTile(endPoint)->getMapData(MapData::O_OBJECT)->getTUCost(MT_WALK) == 255)
		{
//			_trajectory.clear();
			result = VOXEL_OUTOFBOUNDS;
		}

		// OLD. finally do a line calculation and store this trajectory.
/*		ret = _save->getTileEngine()->calculateParabola(
				originVoxel,
				targetVoxel,
				true,
				&_trajectory,
				bu,
				arc,
				1.); */
	}

	Log(LOG_INFO) << ". ret = " << ret;
	return ret;
}
/*
Wb.131129
int Projectile::calculateThrow(double accuracy)
{
	Tile *targetTile = _save->getTile(_action.target);
	Position origin = _action.actor->getPosition();
		
	Position originVoxel = _save->getTileEngine()->getOriginVoxel(_action, 0);
	Position targetVoxel = _action.target * Position(16,16,24) + Position(8,8, (2 + -targetTile->getTerrainLevel()));

	if (_action.type != BA_THROW)
	{
		BattleUnit *tu = targetTile->getUnit();
		if(!tu && _action.target.z > 0 && targetTile->hasNoFloor(0))
			tu = _save->getTile(_action.target - Position(0, 0, 1))->getUnit();
		if (tu)
		{
			targetVoxel.z += ((tu->getHeight()/2) + tu->getFloatHeight()) - 2;
		}
	}

	double curvature = 0.0;
	int retVal = V_OUTOFBOUNDS;
	if (_save->getTileEngine()->validateThrow(_action, originVoxel, targetVoxel, &curvature, &retVal))
	{
		int test = V_OUTOFBOUNDS;
		// finally do a line calculation and store this trajectory, make sure it's valid.
		while (test == V_OUTOFBOUNDS)
		{
			Position deltas = targetVoxel;
			// apply some accuracy modifiers
			applyAccuracy(originVoxel, &deltas, accuracy, true, _save->getTile(_action.target), false); //calling for best flavor
			deltas -= targetVoxel;
			_trajectory.clear();
			test = _save->getTileEngine()->calculateParabola(originVoxel, targetVoxel, true, &_trajectory, _action.actor, curvature, deltas);

			Position endPoint = _trajectory.back();
			endPoint.x /= 16;
			endPoint.y /= 16;
			endPoint.z /= 24;
			Tile *endTile = _save->getTile(endPoint);
			// check if the item would land on a tile with a blocking object
			if (_action.type == BA_THROW
				&& endTile
				&& endTile->getMapData(MapData::O_OBJECT)
				&& endTile->getMapData(MapData::O_OBJECT)->getTUCost(MT_WALK) == 255)
			{
				test = V_OUTOFBOUNDS;
			}
		}
		return retVal;
	}
	return V_OUTOFBOUNDS;
}
*/

/**
 * Calculates the new target in voxel space, based on the given accuracy modifier.
 * @param origin, Startposition of the trajectory.
 * @param target, Endpoint of the trajectory.
 * @param accuracy, Accuracy modifier.
 * @param keepRange, Whether range affects accuracy. Default = false.
 * @param targetTile, Tile of target. Default = 0.
 * @param extendLine, should this line get extended to maximum distance. Default = false.
 */
void Projectile::applyAccuracy(
		const Position& origin,
		Position* target,
		double accuracy,
		bool keepRange,
		Tile* targetTile,
		bool extendLine)
{
	Log(LOG_INFO) << "Projectile::applyAccuracy()";

	int xdiff = origin.x - target->x;
	int ydiff = origin.y - target->y;
	double realDistance = sqrt(static_cast<double>(xdiff * xdiff) + static_cast<double>(ydiff * ydiff));

	// maxRange is the maximum range a projectile shall ever travel in voxel space
	double maxRange = 16000.0; // 1000 tiles in voxelspace
	if (keepRange)
		maxRange = realDistance;

	if (_action.type == BA_HIT)
		maxRange = 45.0; // up to 2 tiles diagonally (as in the case of reaper v reaper)

	if (Options::getBool("battleRangeBasedAccuracy"))
	{
		double accuracyPenalty = 0.0;

		if (targetTile
			&& targetTile->getUnit())
		{
			BattleUnit* targetUnit = targetTile->getUnit();

			if (targetUnit
				&& targetUnit->getFaction() == FACTION_HOSTILE)
			{
				accuracyPenalty = 0.015 * static_cast<double>(targetTile->getShade()); // Shade can be from 0 to 15
			}
//			else
//				accuracyPenalty = 0.0; // Enemy units can see in the dark.

			// If unit is kneeled, then accuracy reduced by 5%.
			// This is a compromise, because vertical deviation is 2 times less.
			if (targetUnit && targetUnit->isKneeled())
				accuracyPenalty += 0.063;
		}
		else
			accuracyPenalty = 0.015 * static_cast<double>(_save->getGlobalShade()); // Shade can be from 0 (day) to 15 (night).

		// kL_begin: modify rangedBasedAccuracy (shot-modes).
//		baseDeviation = -0.15;
/*		baseDeviation = -0.13;
		switch (_action.type)
		{
			case BA_AUTOSHOT:
				baseDeviation += 0.32 / (accuracy - accuracyPenalty + 0.23);
			break;
			case BA_SNAPSHOT:
//				baseDeviation += 0.28 / (accuracy - accuracyPenalty + 0.24);
				baseDeviation += 0.29 / (accuracy - accuracyPenalty + 0.23);
			break;
			case BA_AIMEDSHOT:
//				baseDeviation += 0.23 / (accuracy - accuracyPenalty + 0.23);
				baseDeviation += 0.24 / (accuracy - accuracyPenalty + 0.23);
			break;

			default:
				baseDeviation += 0.29 / (accuracy - accuracyPenalty + 0.23);
			break;
		} */
		double baseDeviation = 0.0;
		switch (_action.type)
		{
			case BA_AUTOSHOT:
				baseDeviation += 0.18 / (accuracy - accuracyPenalty + 0.2); // was 0.25 (too accurate)
			break;
			case BA_SNAPSHOT:
				baseDeviation += 0.15 / (accuracy - accuracyPenalty + 0.2);
			break;
			case BA_AIMEDSHOT:
				baseDeviation += 0.12 / (accuracy - accuracyPenalty + 0.2);
			break;

			default:
				baseDeviation += 0.15 / (accuracy - accuracyPenalty + 0.2);
			break;
		}
		// kL_end.

		// 0.02 is the min angle deviation for best accuracy (+-3s = 0.02 radian).
		// kL_note: changed to min 0.006 deviation.
//kL		if (baseDeviation < 0.02) baseDeviation = 0.02;
		if (baseDeviation < 0.001)
		{
			Log(LOG_INFO) << ". baseDeviation low-capped @ 0.001";
			baseDeviation = 0.001;		// kL
		}
		else Log(LOG_INFO) << ". baseDeviation = " << baseDeviation;

		// the angle deviations are spread using a normal distribution for baseDeviation (+-3s with precision 99,7%)
		double dH = RNG::boxMuller(0.0, baseDeviation / 6.0); // horizontal miss in radian
		double dV = RNG::boxMuller(0.0, baseDeviation /(6.0 * 1.72));	// kL
//kL		double dV = RNG::boxMuller(0.0, baseDeviation /(6.0 * 2));

		double te = atan2(static_cast<double>(target->y - origin.y), static_cast<double>(target->x - origin.x)) + dH;
		double fi = atan2(static_cast<double>(target->z - origin.z), realDistance) + dV;
		double cos_fi = cos(fi);

		if (extendLine)
		{
			// It is a simple task - to hit a target width of 5-7 voxels. Good luck!
			target->x = static_cast<int>(static_cast<double>(origin.x) + maxRange * cos(te) * cos_fi);
			target->y = static_cast<int>(static_cast<double>(origin.y) + maxRange * sin(te) * cos_fi);
			target->z = static_cast<int>(static_cast<double>(origin.z) + maxRange * sin(fi));
		}

		Log(LOG_INFO) << "Projectile::applyAccuracy() rangeBased EXIT";
		return;
	}

	// Wb's new nonRangeBased target formula. 2013 nov 12
	int xDist = abs(origin.x - target->x);
	int yDist = abs(origin.y - target->y);
	int zDist = abs(origin.z - target->z);
	int xyShift, zShift;

	if (xDist / 2 <= yDist)				// yes, we need to add some x/y non-uniformity
		xyShift = xDist / 4 + yDist;	// and don't ask why, please. it's The Commandment
	else
		xyShift = (xDist + yDist) / 2;	// that's uniform part of spreading

	if (xyShift <= zDist)				// slight z deviation
		zShift = xyShift / 2 + zDist;
	else
		zShift = xyShift + zDist / 2;

	int deviation = RNG::generate(0, 100) - (static_cast<int>(accuracy * 100.0));

	if (deviation > -1)
		deviation += 50;				// add extra spread to "miss" cloud
	else
		deviation += 10;				// accuracy of 109 or greater will become 1 (tightest spread)
	
	deviation = std::max(1, zShift * deviation / 200);	// range ratio
		
	target->x += RNG::generate(0, deviation) - deviation / 2;
	target->y += RNG::generate(0, deviation) - deviation / 2;
	target->z += RNG::generate(0, deviation / 2) / 2 - deviation / 8;
	
	if (extendLine)
	{
		double rotation, tilt;
		rotation = atan2(double(target->y - origin.y), double(target->x - origin.x)) * 180.0 / M_PI;
		tilt = atan2(
				double(target->z - origin.z),
				sqrt(double(target->x - origin.x) * double(target->x - origin.x)
						+ double(target->y - origin.y) * double(target->y - origin.y)))
					* 180.0 / M_PI;

/*	// maxDeviation is the max angle deviation for accuracy 0% in degrees
	double maxDeviation = 2.5;
	// minDeviation is the min angle deviation for accuracy 100% in degrees
	double minDeviation = 0.4;
	double dRot, dTilt;
	double rotation, tilt;
	double baseDeviation = (maxDeviation - (maxDeviation * accuracy)) + minDeviation;

	// the angle deviations are spread using a normal distribution between 0 and baseDeviation
	// check if we hit
	if (RNG::generate(0.0, 1.0) < accuracy)
	{
		// we hit, so no deviation
		dRot = 0.0;
		dTilt = 0.0;
	}
	else
	{
		dRot = RNG::boxMuller(0.0, baseDeviation);
		dTilt = RNG::boxMuller(0.0, baseDeviation / 2.0); // tilt deviation is halved
	}

	rotation = atan2(static_cast<double>(target->y - origin.y), static_cast<double>(target->x - origin.x)) * 180.0 / M_PI;
	tilt = atan2(static_cast<double>(target->z - origin.z),
			sqrt(
					static_cast<double>(target->x - origin.x)
							* static_cast<double>(target->x - origin.x)
						+ static_cast<double>(target->y - origin.y)
							* static_cast<double>(target->y - origin.y)))
						* 180.0 / M_PI;

	// add deviations
	rotation += dRot;
	tilt += dTilt; */

		// calculate new target
		// this new target can be very far out of the map, but we don't care about that right now
		double cos_fi = cos(tilt * M_PI / 180.0);
		double sin_fi = sin(tilt * M_PI / 180.0);
		double cos_te = cos(rotation * M_PI / 180.0);
		double sin_te = sin(rotation * M_PI / 180.0);

		target->x = static_cast<int>(static_cast<double>(origin.x) + maxRange * cos_te * cos_fi);
		target->y = static_cast<int>(static_cast<double>(origin.y) + maxRange * sin_te * cos_fi);
		target->z = static_cast<int>(static_cast<double>(origin.z) + maxRange * sin_fi);
	}

	Log(LOG_INFO) << "Projectile::applyAccuracy() EXIT";
}

/**
 * Moves further in the trajectory.
 * @return, False if the trajectory is finished - no new position exists in the trajectory.
 */
bool Projectile::move()
{
	for (int
			i = 0;
			i < _speed;
			++i)
	{
		_position++;

		if (_position == _trajectory.size())
		{
			_position--;

			return false;
		}
	}

	return true;
}

/**
 * Gets the current position in voxel space.
 * @param offset Offset.
 * @return Position in voxel space.
 */
Position Projectile::getPosition(int offset) const
{
	int posOffset = static_cast<int>(_position) + offset;

	if (posOffset > -1
		&& posOffset < static_cast<int>(_trajectory.size()))
	{
		return _trajectory.at(posOffset);
	}
	else
		return _trajectory.at(_position);
}

/**
 * Gets a particle reference from the projectile surfaces.
 * @param i Index.
 * @return Particle id.
 */
int Projectile::getParticle(int i) const
{
	if (_action.weapon->getRules()->getBulletSprite() == -1)
		return -1;
	else
		return _action.weapon->getRules()->getBulletSprite() + i;
}

/**
 * Gets the project tile item.
 * Returns 0 when there is no item thrown.
 * @return Pointer to BattleItem.
 */
BattleItem* Projectile::getItem() const
{
	if (_action.type == BA_THROW)
		return _action.weapon;
	else
		return 0;
}

/**
 * Gets the bullet sprite.
 * @return Pointer to Surface.
 */
Surface* Projectile::getSprite() const
{
	return _sprite;
}

/**
 * Skips to the end of the trajectory.
 */
void Projectile::skipTrajectory()
{
	_position = _trajectory.size() - 2;
}

}
