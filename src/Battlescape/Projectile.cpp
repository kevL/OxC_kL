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

#include "Projectile.h"

//#include <cmath>

//#include "../fmath.h"

#include "Map.h"
#include "Camera.h"
#include "Particle.h"
#include "Pathfinding.h"
#include "TileEngine.h"

#include "../Engine/Game.h"
//#include "../Engine/Options.h"
//#include "../Engine/RNG.h"
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
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

/**
 * Creates a Projectile on the battle map and calculates its movement.
 * @param res			- pointer to ResourcePack
 * @param save			- pointer to SavedBattleGame
 * @param action		- the BattleAction (BattlescapeGame.h)
 * @param origin		- position that this projectile originates at
 * @param targetVoxel	- position that this projectile is targeted at
 * @param ammo			- pointer to the ammo that produced this projectile when applicable
 */
Projectile::Projectile(
		ResourcePack* res,
		SavedBattleGame* save,
		BattleAction action,
		Position origin,
		Position targetVoxel,
		BattleItem* ammo)
	:
		_res(res),
		_save(save),
		_action(action),
		_origin(origin),
		_targetVoxel(targetVoxel),
		_position(0),
		_bulletSprite(-1),
		_reversed(false),
		_vaporColor(-1),
		_vaporDensity(-1),
		_vaporProbability(5)
{
	//Log(LOG_INFO) << "Create Projectile origin = " << origin;
	//Log(LOG_INFO) << "Create Projectile target = " << targetVoxel;

	_speed = Options::battleFireSpeed; // this is the number of pixels the sprite will move between frames

	if (_action.weapon != NULL)
	{
		if (_action.type == BA_THROW)
		{
			//Log(LOG_INFO) << "Create Projectile -> BA_THROW";
			_sprite =_res->getSurfaceSet("FLOOROB.PCK")->getFrame(getItem()->getRules()->getFloorSprite());
			_speed = _speed * 3 / 7;
		}
		else // ba_SHOOT!! or hit, or spit.... probly Psi-attack also.
		{
			//Log(LOG_INFO) << "Create Projectile -> not BA_THROW";
			if (_action.weapon->getRules()->getArcingShot())
				_speed = _speed * 5 / 7;

			if (ammo != NULL) // try to get all the required info from the ammo, if present
			{
				_bulletSprite = ammo->getRules()->getBulletSprite();
				_vaporColor = ammo->getRules()->getVaporColor();
				_vaporDensity = ammo->getRules()->getVaporDensity();
				_vaporProbability = ammo->getRules()->getVaporProbability();
				_speed = std::max(
								1,
								_speed + ammo->getRules()->getBulletSpeed());
			}

			// no ammo, or the ammo didn't contain the info we wanted, see what the weapon has on offer.
			if (_bulletSprite == -1)
				_bulletSprite = _action.weapon->getRules()->getBulletSprite();

			if (_vaporColor == -1)
				_vaporColor = _action.weapon->getRules()->getVaporColor();

			if (_vaporDensity == -1)
				_vaporDensity = _action.weapon->getRules()->getVaporDensity();

			if (_vaporProbability == 5) // uhhh why.
				_vaporProbability = _action.weapon->getRules()->getVaporProbability();

			if (ammo == NULL
				|| ammo != _action.weapon
				|| ammo->getRules()->getBulletSpeed() == 0)
			{
				_speed = std::max(
								1,
								_speed + _action.weapon->getRules()->getBulletSpeed());
			}
		}
	}

	if ((targetVoxel.x - origin.x) + (targetVoxel.y - origin.y) > -1)
		_reversed = true;
	/*
	NE	 0	reversed
	ENE		reversed
	E	 1	reversed
	ESE		reversed
	SE	 2	reversed
	SSE		reversed
	S	 1	reversed
	SSW		reversed
	SW	 0	reversed
	WSW		not reversed
	W	-1	not reversed
	WNW		not reversed
	NW	-2	not reversed
	NNW		not reversed
	N	-1	not reversed
	NNE		not reversed
	*/
}

/**
 * Deletes the Projectile.
 */
Projectile::~Projectile()
{
}

/**
 * Calculates the trajectory for a straight path.
 * This is a wrapper for calculateTrajectory() below; it calculates and passes on an originVoxel.
 * @param accuracy - accuracy of the projectile's trajectory (a battleunit's accuracy)
 * @return,  -1 nothing to hit / no line of fire
 *			0-3 tile-part (floor / westwall / northwall / content)
 *			  4 unit
 *			  5 out-of-map
 * VOXEL_EMPTY			// -1
 * VOXEL_FLOOR			//  0
 * VOXEL_WESTWALL		//  1
 * VOXEL_NORTHWALL		//  2
 * VOXEL_OBJECT			//  3
 * VOXEL_UNIT			//  4
 * VOXEL_OUTOFBOUNDS	//  5
 */
int Projectile::calculateTrajectory(double accuracy)
{
	Position originVoxel = _save->getTileEngine()->getOriginVoxel(
															_action,
															_save->getTile(_origin));
	return calculateTrajectory(
							accuracy,
							originVoxel);
}

/**
 * Calculates the trajectory for a straight path.
 * First determines if there is LoF, then calculates & stores a modified trajectory that is actually pathed.
 * @param accuracy - accuracy of the projectile's trajectory (a BattleUnit's accuracy)
 * @return,  -1 nothing to hit / no line of fire
 *			0-3 tile-part (floor / westwall / northwall / content)
 *			  4 unit
 *			  5 out-of-map
 * VOXEL_EMPTY			// -1
 * VOXEL_FLOOR			//  0
 * VOXEL_WESTWALL		//  1
 * VOXEL_NORTHWALL		//  2
 * VOXEL_OBJECT			//  3
 * VOXEL_UNIT			//  4
 * VOXEL_OUTOFBOUNDS	//  5
 */
int Projectile::calculateTrajectory(
		double accuracy,
		Position originVoxel)
{
	//Log(LOG_INFO) << "Projectile::calculateTrajectory()";
	//Log(LOG_INFO) << ". originVoxel = " << originVoxel;

	const Tile* const targetTile = _save->getTile(_action.target);
	const BattleUnit* const targetUnit = targetTile->getUnit();

	BattleUnit* const actor = _action.actor;
	const int test = _save->getTileEngine()->calculateLine(
														originVoxel,
														_targetVoxel,
														false,
														&_trajectory,
														actor);
	if (test != VOXEL_EMPTY
		&& _trajectory.empty() == false
		&& _action.actor->getFaction() == FACTION_PLAYER // kL_note: so aLiens don't even get in here!
		&& _action.autoShotCount == 1
		&& (!Options::forceFire
			|| (SDL_GetModState() & KMOD_CTRL) == 0)
		&& _save->getBattleGame()->getPanicHandled()
		&& _action.type != BA_LAUNCH)
	{
		Position hitPos = Position(
								_trajectory.at(0).x / 16,
								_trajectory.at(0).y / 16,
								_trajectory.at(0).z / 24);
		//Log(LOG_INFO) << ". hitPos = " << hitPos;

		if (test == VOXEL_UNIT
			&& _save->getTile(hitPos) != NULL
			&& _save->getTile(hitPos)->getUnit() == NULL)
		{
			hitPos = Position( // must be poking head up from belowTile
							hitPos.x,
							hitPos.y,
							hitPos.z - 1);
		}

		if (hitPos != _action.target
			&& _action.result.empty() == true)
		{
			if (test == VOXEL_NORTHWALL)
			{
				if (hitPos.y - 1 != _action.target.y)
				{
					_trajectory.clear();
					return VOXEL_EMPTY;
				}
			}
			else if (test == VOXEL_WESTWALL)
			{
				if (hitPos.x - 1 != _action.target.x)
				{
					_trajectory.clear();
					return VOXEL_EMPTY;
				}
			}
			else if (test == VOXEL_UNIT)
			{
				const BattleUnit* const hitUnit = _save->getTile(hitPos)->getUnit();
				if (hitUnit != targetUnit
					&& hitUnit->getVisible() == true)
				{
					_trajectory.clear();
					return VOXEL_EMPTY;
				}
			}
			else
			{
				_trajectory.clear();
				return VOXEL_EMPTY;
			}
		}
	}

	_trajectory.clear();

//	bool extendLine = true;
	// even guided missiles drift, but how much is based on
	// the shooter's faction, rather than accuracy.
/*kL	if (_action.type == BA_LAUNCH)
	{
		extendLine = _action.waypoints.size() < 2;

		if (_action.actor->getFaction() == FACTION_PLAYER)
			accuracy = 0.60;
		else
			accuracy = 0.55;
	} */

	// apply some accuracy modifiers. This will result in a new target voxel:
	if (targetUnit != NULL
		&& targetUnit->getDashing() == true)
	{
		accuracy -= 0.18;
		//Log(LOG_INFO) << ". . . . targetUnit " << targetUnit->getId() << " is Dashing!!! accuracy = " << accuracy;
	}

	if (_action.type != BA_LAUNCH // Could base BL.. on psiSkill, or sumthin'
		&& originVoxel / Position(16, 16, 24) != _targetVoxel / Position(16, 16, 24)) // somthing wrong with targeting unitTile itself. always comes out as +1 tile east
	{
		applyAccuracy(
					originVoxel,
					&_targetVoxel,
					accuracy,
					false,
					targetTile);
	}
	//Log(LOG_INFO) << ". origin applyAccuracy() = " << originVoxel;
	//Log(LOG_INFO) << ". target applyAccuracy() = " << _targetVoxel;

	//Log(LOG_INFO) << ". LoF calculated, Acu applied (if not BL)";
	// finally do a line calculation and store this trajectory.
	return _save->getTileEngine()->calculateLine(
											originVoxel,
											_targetVoxel,
											true,
											&_trajectory,
											actor);
}

/**
 * Calculates the trajectory for a parabolic path.
 * @param accuracy - accuracy of the projectile's trajectory (a battleunit's accuracy)
 * @return,  -1 nothing to hit / no line of fire
 *			0-3 tile-part (floor / westwall / northwall / content)
 *			  4 unit
 *			  5 out-of-map
 * VOXEL_EMPTY			// -1
 * VOXEL_FLOOR			//  0
 * VOXEL_WESTWALL		//  1
 * VOXEL_NORTHWALL		//  2
 * VOXEL_OBJECT			//  3
 * VOXEL_UNIT			//  4
 * VOXEL_OUTOFBOUNDS	//  5
 */
int Projectile::calculateThrow(double accuracy)
{
	//Log(LOG_INFO) << "Projectile::calculateThrow()"; //, cf TileEngine::validateThrow()";
/*	BattleUnit* bu = _save->getTile(_origin)->getUnit();
	if (bu == NULL)
		bu = _save->getTile(Position(
								_origin.x,
								_origin.y,
								_origin.z - 1))->getUnit();
*/

	const Position originVoxel = _save->getTileEngine()->getOriginVoxel(
																	_action,
																	NULL);
	Position targetVoxel = Position( // determine the target voxel, aim at the center of the floor
								_action.target.x * 16 + 8,
								_action.target.y * 16 + 8,
								_action.target.z * 24 + 2);
	targetVoxel.z -= _save->getTile(_action.target)->getTerrainLevel();

	const BattleUnit* targetUnit = NULL;
	if (_action.type != BA_THROW) // celatid acid-spit
	{
		targetUnit = _save->getTile(_action.target)->getUnit();
		if (targetUnit == NULL
			&& _action.target.z > 0
			&& _save->getTile(_action.target)->hasNoFloor(0))
		{
			targetUnit = _save->getTile(Position(
											_action.target.x,
											_action.target.y,
											_action.target.z - 1))->getUnit();
		}

		if (targetUnit != NULL)
		{
			targetVoxel.z += targetUnit->getHeight() / 2
						   + targetUnit->getFloatHeight()
//kL					   - 2;
						   + 2; // kL: midriff +2 voxels

			if (targetUnit->getDashing() == true)
				accuracy -= 0.18; // acid-spit, arcing shot.
		}
	}


	int ret = VOXEL_OUTOFBOUNDS;

	double arc;
	if (_save->getTileEngine()->validateThrow(
										_action,
										originVoxel,
										targetVoxel,
										&arc,
										&ret) == true)
	{
		//Log(LOG_INFO) << ". validateThrow() TRUE";
		// Finally do a parabola calculation and store that trajectory - make sure it's valid.
		int test = VOXEL_OUTOFBOUNDS;
		while (test == VOXEL_OUTOFBOUNDS)
		{
			Position targetDelta = targetVoxel; // apply some accuracy modifiers
			applyAccuracy(
						originVoxel,
						&targetDelta,
						accuracy,
						true,
						_save->getTile(_action.target),
						false);

			targetDelta -= targetVoxel;

			_trajectory.clear();

			test = _save->getTileEngine()->calculateParabola(
														originVoxel,
														targetVoxel,
														true,
														&_trajectory,
														_action.actor,
														arc,
														targetDelta);
			//Log(LOG_INFO) << ". . calculateParabola() = " << test;

			// kL_begin: See also TileEngine::validateThrow()
			if (_action.type == BA_THROW)
			{
				const Tile* const targetTile = _save->getTile(_trajectory.back() / Position(16, 16, 24));
				if (targetTile != NULL
					&& targetTile->getMapData(MapData::O_OBJECT) != NULL
					&& (targetTile->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_NESW
						|| targetTile->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_NWSE))
//					&& (action.weapon->getRules()->getBattleType() == BT_GRENADE
//						|| action.weapon->getRules()->getBattleType() == BT_PROXIMITYGRENADE)
//					&& _action.target->getMapData(MapData::O_OBJECT)->getTUCost(MT_WALK) == 255)
				{
					test = VOXEL_OUTOFBOUNDS; // prevent Grenades from landing on diagonal BigWalls.
				}
			} // kL_end.
		}

		return ret;
	}
	//Log(LOG_INFO) << ". validateThrow() FALSE";
	return VOXEL_OUTOFBOUNDS;
}
/*	static const double maxDeviation = 0.125;
	static const double minDeviation = 0.0;
	double baseDeviation = std::max(
								0.0,
								maxDeviation - (maxDeviation * accuracy) + minDeviation);
	//Log(LOG_INFO) << ". baseDeviation = " << baseDeviation;


	// finally do a line calculation and store this trajectory, make sure it's valid.
	int ret = VOXEL_OUTOFBOUNDS;
	while (ret == VOXEL_OUTOFBOUNDS)
	{
		_trajectory.clear();

		double deviation = RNG::boxMuller(0.0, baseDeviation);
		//Log(LOG_INFO) << ". . boxMuller() deviation = " << deviation + 1.0;
		ret = _save->getTileEngine()->calculateParabola(
													originVoxel,
													targetVoxel,
													true,
													&_trajectory,
													bu,
													arc,
													1.0 + deviation); */

/*			Position endPoint = _trajectory.back();
			endPoint.x /= 16;
			endPoint.y /= 16;
			endPoint.z /= 24;

			Tile* endTile = _save->getTile(endPoint);

			// check if the item would land on a tile with a blocking object
			// _OLD: let it fly without deviation, it must land on a valid tile in that case.
			// kL_note: Am i sure I want this? xCom_orig let stuff land on nonwalkable tiles...!!!
			// and, uh, couldn't this lead to a potentially infinite loop???
			if (_action.type == BA_THROW
				&& endTile
				&& endTile->getMapData(MapData::O_OBJECT)
				&& endTile->getMapData(MapData::O_OBJECT)->getTUCost(MT_WALK) == 255)
			{
				test = VOXEL_OUTOFBOUNDS;
			} */

/*_OLD			ret = _save->getTileEngine()->calculateParabola(
					originVoxel,
					targetVoxel,
					true,
					&_trajectory,
					bu,
					arc,
					1.0); */

/**
 * Calculates the new target in voxel space, based on a given accuracy modifier.
 * @param origin		- reference the start position of the trajectory in voxelspace
 * @param target		- pointer to an end position of the trajectory in voxelspace
 * @param accuracy		- accuracy modifier
 * @param keepRange		- true if range affects accuracy (default = false)
 * @param targetTile	- pointer to tile of the target (default = NULL)
 * @param extendLine	- true if this line should extend to maximum distance on the battle map (default = true)
 */
void Projectile::applyAccuracy(
		const Position& origin,
		Position* const target,
		double accuracy,
		const bool keepRange,
		const Tile* const targetTile,
		const bool extendLine)
{
	//Log(LOG_INFO) << "Projectile::applyAccuracy(), accuracy = " << accuracy;
	if (_action.type == BA_HIT)
		return;


	const int
		delta_x = origin.x - target->x,
		delta_y = origin.y - target->y,
		delta_z = origin.z - target->z; // kL_add.

	const double targetDist = sqrt(
								  static_cast<double>(delta_x * delta_x)
								+ static_cast<double>(delta_y * delta_y)
								+ static_cast<double>(delta_z * delta_z)); // kL_add.
	//Log(LOG_INFO) << ". targetDist = " << targetDist;

	// maxRange is the maximum range a projectile shall ever travel in voxel space
	double maxRange = 16000.; // vSpace == 1000 tiles in tSpace.
	// kL_note: This is that hypothetically infinite point in space to aim for;
	// the closer it's set, the more accurate shots become. I suspect ....
//	double maxRange = 3200.0; // kL. vSpace == 200 tiles in tSpace.
	if (keepRange == true)
		maxRange = targetDist;
	//Log(LOG_INFO) << ". maxRange = " << maxRange;

//kL	if (_action.type == BA_HIT)
//kL		maxRange = 45.0; // up to 2 tiles diagonally (as in the case of reaper vs reaper)


	const RuleItem* const rule = _action.weapon->getRules(); // <- after reading up, 'const' is basically worthless & wordy waste of effort.
	if (_action.type != BA_THROW
		&& rule->getArcingShot() == false)
	{
		if (Options::battleUFOExtenderAccuracy
//			&& _save->getSide() == FACTION_PLAYER) // kL: only for xCom heheh
			&& _action.actor->getFaction() == FACTION_PLAYER)
		{
			//Log(LOG_INFO) << ". battleUFOExtenderAccuracy";

			// kL_note: if distance is greater-than the weapon's
			// max range, then ProjectileFlyBState won't allow the shot;
			// so that's already been taken care of ....
			const int shortLimit = rule->getMinRange();
			int longLimit = rule->getAimRange();

			if (_action.type == BA_SNAPSHOT)
				longLimit = rule->getSnapRange();
			else if (_action.type == BA_AUTOSHOT)
				longLimit = rule->getAutoRange();

			double modifier = 0.;

			const int targetDist_tSpace = static_cast<int>(targetDist / 16.);
			if (targetDist_tSpace < shortLimit)
				modifier = static_cast<double>((rule->getDropoff() * (shortLimit - targetDist_tSpace)) / 100);
			else if (longLimit < targetDist_tSpace)
				modifier = static_cast<double>((rule->getDropoff() * (targetDist_tSpace - longLimit)) / 100);

			accuracy = std::max(
								0.,
								accuracy - modifier);
		}

		if (Options::battleRangeBasedAccuracy)
		{
			//Log(LOG_INFO) << ". battleRangeBasedAccuracy";
			double acuPenalty = 0.;

			if (targetTile)
			{
				const int smoke = targetTile->getSmoke();
				if (smoke > 0)
					acuPenalty += smoke * 0.01;

				if (targetTile->getUnit() != NULL)
				{
					const BattleUnit* const targetUnit = targetTile->getUnit();

					if (_action.actor->getOriginalFaction() != FACTION_HOSTILE)
						acuPenalty += 0.01 * static_cast<double>(targetTile->getShade());

					// If targetUnit is kneeled, then accuracy reduced by ~6%.
					// This is a compromise, because vertical deviation is ~2 times less.
					if (targetUnit->isKneeled() == true)
						acuPenalty += 0.076;
				}
			}
			else if (_action.actor->getOriginalFaction() != FACTION_HOSTILE) // targeting tile-stuffs.
				acuPenalty += 0.01 * static_cast<double>(_save->getGlobalShade());

			if (_action.type == BA_AUTOSHOT)
				acuPenalty += (_action.autoShotCount - 1) * 0.03;


			double deviation = 0.21;
			if (_action.actor->getFaction() == FACTION_HOSTILE)
				deviation -= 0.06; // give the poor aLiens an aiming advantage over xCom & Mc'd units
			// DO IT IN RULESET! not here. Okay do it here because of faction/MC-thing ...

			deviation /= accuracy - acuPenalty + 0.13;
			deviation = std::max(
								0.01,
								deviation);
			//Log(LOG_INFO) << ". deviation = " << deviation;


			// The angle deviations are spread using a normal distribution:
			const double
				dH = RNG::boxMuller(0., deviation / 6.),			// horizontal miss in radians
				dV = RNG::boxMuller(0., deviation / (6. * 1.77)),	// vertical miss in radians

				te = atan2(
						static_cast<double>(target->y - origin.y),
						static_cast<double>(target->x - origin.x))
					+ dH,
				fi = atan2(
						static_cast<double>(target->z - origin.z),
						targetDist)
					+ dV,
				cos_fi = cos(fi);
				//Log(LOG_INFO) << ". . dH = " << dH;
				//Log(LOG_INFO) << ". . dV = " << dV;

			if (extendLine == true) // kL_note: This is for aimed projectiles; always true in my RangedBased here.
			{
				//Log(LOG_INFO) << "Projectile::applyAccuracy() extendLine";
				// It is a simple task - to hit a target width of 5-7 voxels. Good luck!
				target->x = static_cast<int>(Round(static_cast<double>(origin.x) + maxRange * cos(te) * cos_fi));
				target->y = static_cast<int>(Round(static_cast<double>(origin.y) + maxRange * sin(te) * cos_fi));
				target->z = static_cast<int>(Round(static_cast<double>(origin.z) + maxRange * sin(fi)));
			}
			//Log(LOG_INFO) << "Projectile::applyAccuracy() x = " << target->x;
			//Log(LOG_INFO) << "Projectile::applyAccuracy() y = " << target->y;
			//Log(LOG_INFO) << "Projectile::applyAccuracy() z = " << target->z;

			//Log(LOG_INFO) << "Projectile::applyAccuracy() rangeBased EXIT";
		}

		return;
	}


	// kL_note: *** This is for Throwing and AcidSpitt only ***
	accuracy = accuracy * 50. + 65.; // arbitrary adjustment.

	double perfectToss = 100.;
	const Soldier* const soldier = _save->getGeoscapeSave()->getSoldier(_action.actor->getId());
	if (soldier != NULL)
		perfectToss = static_cast<double>(soldier->getRules()->getStatCaps().throwing);

	double deviation = perfectToss - accuracy;
	deviation = std::max(
						0.,
						deviation * targetDist / 100.);

	const double
		dx = RNG::boxMuller(0., deviation) / 4.,
		dy = RNG::boxMuller(0., deviation) / 4.,
		dz = RNG::boxMuller(0., deviation) / 6.;

	target->x += static_cast<int>(Round(dx));
	target->y += static_cast<int>(Round(dy));
	target->z += static_cast<int>(Round(dz));


	if (extendLine == true)	// note: This is for aimed projectiles; always false outside RangedBased above
							// - that is, this ought never run in this Build.
	{
		//Log(LOG_INFO) << ". Projectile::applyAccuracy() ERROR : extendLine";
/*		double maxDeviation = 2.5; // maxDeviation is the max angle deviation for accuracy 0% in degrees
		double minDeviation = 0.4; // minDeviation is the min angle deviation for accuracy 100% in degrees
		double dRot, dTilt;
		double rotation, tilt;
		double baseDeviation = (maxDeviation - (maxDeviation * accuracy)) + minDeviation;

		// the angle deviations are spread using a normal distribution between 0 and baseDeviation
		if (RNG::generate(0.0, 1.0) < accuracy) // check if we hit
		{
			dRot = 0.0; // we hit, so no deviation
			dTilt = 0.0;
		}
		else
		{
			dRot = RNG::boxMuller(0.0, baseDeviation);
			dTilt = RNG::boxMuller(0.0, baseDeviation / 2.0); // tilt deviation is halved
		}
		rotation += dRot; // add deviations
		tilt += dTilt; */

		const double
			rotation = atan2(
							static_cast<double>(target->y - origin.y),
							static_cast<double>(target->x - origin.x))
						* 180.0 / M_PI,
			tilt = atan2(
						static_cast<double>(target->z - origin.z),
						sqrt(
							  static_cast<double>(target->x - origin.x) * static_cast<double>(target->x - origin.x)
							+ static_cast<double>(target->y - origin.y) * static_cast<double>(target->y - origin.y)))
						* 180.0 / M_PI;

		// calculate new target
		// this new target can be very far out of the map, but we don't care about that right now
		const double
			cos_fi = cos(tilt * M_PI / 180.0),
			sin_fi = sin(tilt * M_PI / 180.0),
			cos_te = cos(rotation * M_PI / 180.0),
			sin_te = sin(rotation * M_PI / 180.0);

		target->x = static_cast<int>(static_cast<double>(origin.x) + maxRange * cos_te * cos_fi);
		target->y = static_cast<int>(static_cast<double>(origin.y) + maxRange * sin_te * cos_fi);
		target->z = static_cast<int>(static_cast<double>(origin.z) + maxRange * sin_fi);
	}
	//Log(LOG_INFO) << "Projectile::applyAccuracy() EXIT";
}

/**
 * Moves further along the trajectory-path.
 * @return, true while trajectory is pathing; false when finished - no new position exists in the trajectory vector
 */
bool Projectile::traceProjectile()
{
	for (int
			i = 0;
			i < _speed;
			++i)
	{
		_position++;

		if (_position == _trajectory.size())
		{
			_position--; // ie. don't pass the end of the _trajectory vector
			return false;
		}

		if (_save->getDepth() > 0
			&& _vaporColor != -1
			&& _action.type != BA_THROW
			&& RNG::percent(_vaporProbability))
		{
			addVaporCloud();
		}
	}

	return true;
}

/**
 * Gets the current position in voxel space.
 * @param offset - offset (default 0)
 * @return, position in voxel space
 */
Position Projectile::getPosition(int offset) const
{
	Position ret;

	const int posOffset = static_cast<int>(_position) + offset;

	if (posOffset > -1
		&& posOffset < static_cast<int>(_trajectory.size()))
	{
		ret = _trajectory.at(static_cast<size_t>(posOffset));
		//Log(LOG_INFO) << "projPos[0] " << ret;
//		return _trajectory.at(posOffset);
	}
	else
	{
		ret = _trajectory.at(_position);
		//Log(LOG_INFO) << "projPos[1] " << ret;
//		return _trajectory.at(_position);
	}

	return ret;
}

/**
 * Gets a particle reference from the projectile surfaces.
 * @param i - index
 * @return, particle id
 */
int Projectile::getParticle(int i) const
{
	if (_bulletSprite != -1)
		return _bulletSprite + i;
	else
		return -1;
}

/**
 * Gets the projectile item.
 * @return, pointer to a BattleItem (returns NULL when no item is thrown)
 */
BattleItem* Projectile::getItem() const
{
	if (_action.type == BA_THROW)
		return _action.weapon;
	else
		return NULL;
}

/**
 * Gets the bullet sprite.
 * @return, pointer to Surface
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
//	_position = _trajectory.size() - 1; // old code
	while (traceProjectile() == true);
}

/**
 * Gets the Position of origin for the projectile.
 * Instead of using the actor's position use the voxel origin
 * translated to a tile position; this is a workaround for large units.
 * @return, origin as a tile position
 */
Position Projectile::getOrigin()
{
	return _trajectory.front() / Position(16, 16, 24);
}

/**
 * Gets the INTENDED target for this projectile.
 * It is important to note that we do not use the final position
 * of the projectile here, but rather the targeted tile.
 * @return, intended target as a tile position
 */
Position Projectile::getTarget() const
{
	return _action.target;
}

/**
 * Gets the ACTUAL target for this projectile.
 * It is important to note that we use the final position of the projectile here.
 * @return, trajectory finish as a tile position
 */
Position Projectile::getFinalTarget() const
{
	return _trajectory.back() / Position(16, 16, 24);
}

/**
 * Stores the final direction of a missile or thrown-object
 * for use by TileEngine blast propagation - against diagonal BigWalls.
 */
void Projectile::storeProjectileDirection() const
{
	int dir = -1;

	const size_t trajSize = _trajectory.size();
	//Log(LOG_INFO) << "Proj:storeDir, size = " << trajSize;

	if (trajSize > 2)
	{
		//Log(LOG_INFO) << ". pos.back " << _trajectory.back();
		//Log(LOG_INFO) << ". pos.back -2 " << _trajectory.at(trajSize - 3);
		const Position
			finalPos = _trajectory.back(),
			prePos = _trajectory.at(trajSize - 3); // lastPos & pre-lastPos seem to be the same. So subtract 2 from lastPos

		int
			x = 0,
			y = 0;

		if (finalPos.x - prePos.x != 0)
		{
			if (finalPos.x - prePos.x > 0)
				x = 1;
			else
				x = -1;
		}

		if (finalPos.y - prePos.y != 0)
		{
			if (finalPos.y - prePos.y > 0)
				y = 1;
			else
				y = -1;
		}

		const Position relPos = Position(x, y, 0);
		//Log(LOG_INFO) << ". relPos " << relPos;
		Pathfinding::vectorToDirection(relPos, dir);
	}

	//Log(LOG_INFO) << ". . dir = " << dir;
	_save->getTileEngine()->setProjectileDirection(dir);
}

/**
 * Gets if this projectile is to be drawn left to right or right to left.
 * @return, true if this is to be drawn in reverse order
 */
bool Projectile::isReversed() const
{
	return _reversed;
}

/**
 * Adds a cloud of vapor at the projectile's current position.
 */
void Projectile::addVaporCloud()
{
	Tile* const tile = _save->getTile(_trajectory.at(_position) / Position(16, 16, 24));
	if (tile != NULL)
	{
		Position
			tilePos,
			voxelPos;

		_save->getBattleGame()->getMap()->getCamera()->convertMapToScreen(
																	_trajectory.at(_position) / Position(16, 16, 24),
																	&tilePos);
		tilePos += _save->getBattleGame()->getMap()->getCamera()->getMapOffset();
		_save->getBattleGame()->getMap()->getCamera()->convertVoxelToScreen(
																	_trajectory.at(_position),
																	&voxelPos);

		for (int
				i = 0;
				i != _vaporDensity;
				++i)
		{
			Particle* const particle = new Particle(
												static_cast<float>(voxelPos.x - tilePos.x + RNG::generate(0, 6) - 3),
												static_cast<float>(voxelPos.y - tilePos.y + RNG::generate(0, 6) - 3),
												static_cast<float>(RNG::generate(64, 214)),
												static_cast<Uint8>(_vaporColor),
												19);
			tile->addParticle(particle);
		}
	}
}

/**
 * kL. Gets a pointer to the BattleAction actor directly.
 * @return, pointer to the currently acting BattleUnit
 */
BattleUnit* Projectile::getActor() const // kL
{
	return _action.actor;
}

}
