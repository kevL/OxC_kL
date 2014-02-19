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

#include <cmath>

#include "BattlescapeGame.h"
#include "Position.h"
#include "TileEngine.h"

#include "../aresame.h"

#include "../Engine/Game.h"
#include "../Engine/Logger.h"
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
#include "../Savegame/SavedGame.h" // kL
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
		Position origin,
		Position targetVoxel)
	:
		_res(res),
		_save(save),
		_action(action),
		_origin(origin),
		_targetVoxel(targetVoxel),
		_position(0)
{
	// this is the number of pixels the sprite will move between frames
	_speed = Options::getInt("battleFireSpeed");

	if (_action.weapon)
	{
		if (_action.type == BA_THROW)
		{
			Log(LOG_INFO) << "Create Projectile -> BA_THROW";
			_speed = _speed * 4 / 7; // kL
			_sprite =_res->getSurfaceSet("FLOOROB.PCK")->getFrame(getItem()->getRules()->getFloorSprite());
		}
		else // ba_SHOOT!! or hit, or spit.... probly Psi-attack also.
		{
			Log(LOG_INFO) << "Create Projectile -> not BA_THROW";
			if (_action.weapon->getRules()->getBulletSpeed() != 0)
				_speed = std::max(
								1,
								_speed + _action.weapon->getRules()->getBulletSpeed());
			else if (_action.weapon->getAmmoItem()
				&& _action.weapon->getAmmoItem()->getRules()->getBulletSpeed() != 0)
			{
				_speed = std::max(
								1,
								_speed + _action.weapon->getAmmoItem()->getRules()->getBulletSpeed());
			}
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
	Position originVoxel = _save->getTileEngine()->getOriginVoxel(
															_action,
															_save->getTile(_origin));

	return calculateTrajectory(
							accuracy,
							originVoxel);
}

/**
 * Calculates the trajectory for a straight path.
 * @param accuracy, The unit's accuracy.
 * @return, The objectnumber(0-3) or unit(4) or out of map (5) or -1 (no line of fire).
 */
int Projectile::calculateTrajectory(
		double accuracy,
		Position originVoxel)
{
	Log(LOG_INFO) << "Projectile::calculateTrajectory()";

	Tile* targetTile = _save->getTile(_action.target);
	BattleUnit* targetUnit = targetTile->getUnit();

	BattleUnit* bu = _action.actor;

	int test = _save->getTileEngine()->calculateLine(
												originVoxel,
												_targetVoxel,
												false,
												&_trajectory,
												bu);
	if (test != VOXEL_EMPTY
		&& !_trajectory.empty()
		&& _action.actor->getFaction() == FACTION_PLAYER // kL_note: so aLiens don't even get in here!
		&& _action.autoShotCount == 1
		&& (SDL_GetModState() & KMOD_CTRL) == 0
		&& _save->getBattleGame()->getPanicHandled())
	{
		Position hitPos = Position(
								_trajectory.at(0).x / 16,
								_trajectory.at(0).y / 16,
								_trajectory.at(0).z / 24);

		if (test == VOXEL_UNIT
			&& _save->getTile(hitPos)
			&& _save->getTile(hitPos)->getUnit() == 0)
		{
			hitPos = Position( // must be poking head up from belowTile
							hitPos.x,
							hitPos.y,
							hitPos.z - 1);
		}

		if (hitPos != _action.target
			&& _action.result == "")
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
				BattleUnit* hitUnit = _save->getTile(hitPos)->getUnit();
				if (hitUnit != targetUnit
					&& hitUnit->getVisible()) // kL
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
	// Wb.131216
/*kL	if (_action.type == BA_LAUNCH)
	{
		extendLine = _action.waypoints.size() < 2;

		if (_action.actor->getFaction() == FACTION_PLAYER)
			accuracy = 0.60;
		else
			accuracy = 0.55;
	} */

	// apply some accuracy modifiers. This will result in a new target voxel:
	if (targetUnit						// kL
		&& targetUnit->getDashing())	// kL
	{
		accuracy -= 0.35;				// kL
		Log(LOG_INFO) << ". . . . targetUnit " << targetUnit->getId() << " is Dashing!!! accuracy = " << accuracy;
	}

	if (_action.type != BA_LAUNCH) // kL. Could base BL.. on psiSkill, or sumthin'
		applyAccuracy(
					originVoxel,
					&_targetVoxel,
					accuracy,
					false,
					targetTile);
//kL					extendLine); // default: True

	//Log(LOG_INFO) << ". LoF calculated, Acu applied (if not BL)";
	// finally do a line calculation and store this trajectory.
	return _save->getTileEngine()->calculateLine(
											originVoxel,
											_targetVoxel,
											true,
											&_trajectory,
											bu);
}

/**
 * Calculates the trajectory for a curved path.
 * @param accuracy, The unit's accuracy.
 * @return int, The objectnumber(0-3) or unit(4) or out of map (5) or -1(hit nothing).
 */
int Projectile::calculateThrow(double accuracy)
{
	Log(LOG_INFO) << "Projectile::calculateThrow(), cf TileEngine::validateThrow()";

	BattleUnit* bu = _save->getTile(_origin)->getUnit();
	if (!bu)
	{
		bu = _save->getTile(Position(
								_origin.x,
								_origin.y,
								_origin.z - 1))->
							getUnit();
	}

	Position originVoxel = _save->getTileEngine()->getOriginVoxel(
																_action,
																0);
	Position targetVoxel = Position( // determine the target voxel, aim at the center of the floor
								_action.target.x * 16 + 8,
								_action.target.y * 16 + 8,
								_action.target.z * 24 + 2);
	targetVoxel.z -= _save->getTile(_action.target)->getTerrainLevel();

	BattleUnit* targetUnit = 0;
	if (_action.type != BA_THROW) // celatid acid-spit
	{
		targetUnit = _save->getTile(_action.target)->getUnit();
		if (!targetUnit
			&& _action.target.z > 0
			&& _save->getTile(_action.target)->hasNoFloor(0))
		{
			targetUnit = _save->getTile(Position(
											_action.target.x,
											_action.target.y,
											_action.target.z - 1))->
										getUnit();
		}

		if (targetUnit)
		{
			targetVoxel.z += targetUnit->getHeight() / 2
						+ targetUnit->getFloatHeight()
//kL						- 2;
						+ 2; // kL: midriff +2 voxels

			if (targetUnit->getDashing())	// kL
				accuracy -= 0.35;			// kL
		}
	}


	int ret = VOXEL_OUTOFBOUNDS;

	double arc = 0.0;
	if (_save->getTileEngine()->validateThrow(
										_action,
										originVoxel,
										targetVoxel,
										&arc,
										&ret))
	{
		// finally do a line calculation and store this trajectory, & make sure it's valid.
		int test = VOXEL_OUTOFBOUNDS;
		while (test == VOXEL_OUTOFBOUNDS)
		{
			Position delta = targetVoxel;	// will apply some accuracy modifiers
			applyAccuracy(					// calling for best flavor
						originVoxel,
						&delta,
						accuracy,
						true,
						_save->getTile(_action.target),
						false);

			delta -= targetVoxel;

			_trajectory.clear();

			test = _save->getTileEngine()->calculateParabola(
														originVoxel,
														targetVoxel,
														true,
														&_trajectory,
														_action.actor,
														arc,
														delta);
/*	static const double maxDeviation = 0.125;
	static const double minDeviation = 0.0;
	double baseDeviation = std::max(
								0.0,
								maxDeviation - (maxDeviation * accuracy) + minDeviation);
	Log(LOG_INFO) << ". baseDeviation = " << baseDeviation;


	// finally do a line calculation and store this trajectory, make sure it's valid.
	int ret = VOXEL_OUTOFBOUNDS;
	while (ret == VOXEL_OUTOFBOUNDS)
	{
		_trajectory.clear();

		double deviation = RNG::boxMuller(0.0, baseDeviation);
		Log(LOG_INFO) << ". . boxMuller() deviation = " << deviation + 1.0;
		ret = _save->getTileEngine()->calculateParabola(
													originVoxel,
													targetVoxel,
													true,
													&_trajectory,
													bu,
													arc,
													1.0 + deviation); */
			Position endPoint = _trajectory.back();
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
			}
/*_OLD			ret = _save->getTileEngine()->calculateParabola(
					originVoxel,
					targetVoxel,
					true,
					&_trajectory,
					bu,
					arc,
					1.0); */
		}

		return ret;
	}

	return VOXEL_OUTOFBOUNDS;
}

/**
 * Calculates the new target in voxel space, based on the given accuracy modifier.
 * @param origin, Startposition of the trajectory in voxelspace.
 * @param target, Endpoint of the trajectory in voxelspace.
 * @param accuracy, Accuracy modifier.
 * @param keepRange, Whether range affects accuracy. Default = false.
 * @param targetTile, Tile of target. Default = 0.
 * @param extendLine, should this line get extended to maximum distance. Default = true.
 */
void Projectile::applyAccuracy(
		const Position& origin,
		Position* target,
		double accuracy,
		bool keepRange,
		Tile* targetTile,
		bool extendLine)
{
	Log(LOG_INFO) << "Projectile::applyAccuracy(), accuracy = " << accuracy;

	if (_action.type == BA_HIT)	// kL
		return;					// kL


	int
		delta_x = origin.x - target->x,
		delta_y = origin.y - target->y,
		delta_z = origin.z - target->z; // kL
//kL	double targetDist = sqrt(static_cast<double>(delta_x * delta_x) + static_cast<double>(delta_y * delta_y));
	double targetDist = sqrt(
							  static_cast<double>(delta_x * delta_x)
							+ static_cast<double>(delta_y * delta_y)
							+ static_cast<double>(delta_z * delta_z)); // kL
	Log(LOG_INFO) << ". targetDist = " << targetDist;

	// maxRange is the maximum range a projectile shall ever travel in voxel space
//kL	double maxRange = 16000.0; // vSpace == 1000 tiles in tSpace.
	// kL_note: This is that hypothetically infinite point in space to aim for;
	// the closer it's set, the more accurate shots become.
	double maxRange = 3200.0; // kL. vSpace == 200 tiles in tSpace.
	if (keepRange)
		maxRange = targetDist;
//kL	if (_action.type == BA_HIT)
//kL		maxRange = 45.0; // up to 2 tiles diagonally (as in the case of reaper vs reaper)


	RuleItem* weaponRule = _action.weapon->getRules();
	if (_save->getSide() == FACTION_PLAYER					// kL: only for xCom heheh
		&& Options::getBool("battleUFOExtenderAccuracy")	// kL
		&& !weaponRule->getArcingShot()						// kL
		&& _action.type != BA_THROW)
	{
		Log(LOG_INFO) << ". battleUFOExtenderAccuracy";

		// kL_note: if distance is greater-than the weapon's
		// max range, then ProjectileFlyBState won't allow the shot;
		// so that's already been taken care of ....
		int
			lowerLimit = weaponRule->getMinRange(),
			upperLimit = weaponRule->getAimRange();

//kL		if (Options::getBool("battleUFOExtenderAccuracy"))
//		{
		if (_action.type == BA_SNAPSHOT)
			upperLimit = weaponRule->getSnapRange();
		else if (_action.type == BA_AUTOSHOT)
			upperLimit = weaponRule->getAutoRange();
//		}

		double modifier = 0.0;
		int targetDist_tSpace = static_cast<int>(targetDist / 16.0);
		if (targetDist_tSpace < lowerLimit)
			modifier = static_cast<double>((weaponRule->getDropoff() * (lowerLimit - targetDist_tSpace)) / 100);
		else if (upperLimit < targetDist_tSpace)
			modifier = static_cast<double>((weaponRule->getDropoff() * (targetDist_tSpace - upperLimit)) / 100);

		accuracy = std::max(
							0.0,
							accuracy - modifier);
	}

	if (Options::getBool("battleRangeBasedAccuracy")
		&& !weaponRule->getArcingShot()					// kL
		&& _action.type != BA_THROW)
//kL		&& _action.type != BA_HIT)
	{
		Log(LOG_INFO) << ". battleRangeBasedAccuracy";

//kL		if (_action.type == BA_HIT)
//kL			return;

		double acuPenalty = 0.0;

		if (targetTile
			&& targetTile->getUnit())
		{
			BattleUnit* targetUnit = targetTile->getUnit();
			if (targetUnit) // Shade can be from 0 (day) to 15 (night).
			{
				if (targetUnit->getFaction() == FACTION_HOSTILE)
					acuPenalty = 0.017 * static_cast<double>(targetTile->getShade());

				// If targetUnit is kneeled, then accuracy reduced by ~6%.
				// This is a compromise, because vertical deviation is ~2 times less.
				if (targetUnit->isKneeled())
					acuPenalty += 0.063;
			}
		}
		else // targetting tile-stuffs.
			acuPenalty = 0.017 * static_cast<double>(_save->getGlobalShade());

		// kL_begin: modify rangedBasedAccuracy (shot-modes).
		// NOTE: This should be done on the weapons themselves!!!!
		double baseDeviation = 0.0;
		if (_action.actor->getFaction() == FACTION_PLAYER)
			baseDeviation = 0.1; // give the poor aLiens an aiming advantage over xCom & Mc'd units

		switch (_action.type)
		{
			case BA_AIMEDSHOT:
				baseDeviation += 0.16;
			break;
			case BA_SNAPSHOT:
				baseDeviation += 0.18;
			break;
			case BA_AUTOSHOT:
				baseDeviation += 0.22;
			break;

			default: // throw. Or hit.
				baseDeviation += 0.2;
			break;

		}
		baseDeviation /= accuracy - acuPenalty + 0.16;
		baseDeviation = std::max(
								0.0,
								baseDeviation); // kL_end.

		Log(LOG_INFO) << ". baseDeviation = " << baseDeviation;


		// The angle deviations are spread using a normal distribution:
		double
			dH = RNG::boxMuller(0.0, baseDeviation / 6.0),			// horizontal miss in radians
			dV = RNG::boxMuller(0.0, baseDeviation / (6.0 * 1.78)),	// vertical miss in radians

			te = atan2(
					static_cast<double>(target->y - origin.y),
					static_cast<double>(target->x - origin.x))
				+ dH,
			fi = atan2(
					static_cast<double>(target->z - origin.z),
					targetDist)
				+ dV,
			cos_fi = cos(fi);
			Log(LOG_INFO) << ". . dH = " << dH;
			Log(LOG_INFO) << ". . dV = " << dV;

		if (extendLine) // kL_note: This is for aimed projectiles; always true in my RangedBased here.
		{
			// It is a simple task - to hit a target width of 5-7 voxels. Good luck!
			target->x = static_cast<int>(static_cast<double>(origin.x) + maxRange * cos(te) * cos_fi);
			target->y = static_cast<int>(static_cast<double>(origin.y) + maxRange * sin(te) * cos_fi);
			target->z = static_cast<int>(static_cast<double>(origin.z) + maxRange * sin(fi));
		}

		Log(LOG_INFO) << "Projectile::applyAccuracy() rangeBased EXIT";
		return;
	}


	// kL_note: *** This should now be for Throwing and AcidSpitting only ***
	Log(LOG_INFO) << ". standard accuracy, Throw & AcidSpit";
	// Wb.131112, nonRangeBased target formula.
	// beware of 'extendLine' below for BLs though... comes from calculateTrajectory() above....

/*kL okay, so this is all just garbage:
	int
		xDist = abs(origin.x - target->x),
		yDist = abs(origin.y - target->y),
		zDist = abs(origin.z - target->z),

		xyShift,
		zShift;

	if (xDist / 2 <= yDist)				// yes, we need to add some x/y non-uniformity
		xyShift = xDist / 4 + yDist;	// and don't ask why, please. it's The Commandment
	else
		xyShift = (xDist + yDist) / 2;	// that's uniform part of spreading

	if (xyShift <= zDist)				// slight z deviation
		zShift = xyShift / 2 + zDist;
	else
		zShift = xyShift + zDist / 2;

//kL	int deviation = RNG::generate(0, 100) - (static_cast<int>(accuracy * 100.0));

	if (deviation > -1)
		deviation += 50;				// add extra spread to "miss" cloud
	else
		deviation += 10;				// accuracy of 109 or greater will become 1 (tightest spread)

	deviation = std::max( // range ratio
						1,
						zShift * deviation / 200);

	target->x += RNG::generate(0, deviation) - deviation / 2;
	target->y += RNG::generate(0, deviation) - deviation / 2;
	target->z += RNG::generate(0, deviation / 4) - deviation / 8; */

	// kL_begin:
	int maxThrowRule = 100;
	Soldier* soldier = _save->getGeoscapeSave()->getSoldier(_action.actor->getId());
	if (soldier)
	{
		int maxThrowRule = soldier->getRules()->getStatCaps().throwing;

//		int minThrowRule = _save->getGeoscapeSave()->getSoldier(_action.actor->getId())->getRules()->getMinStats().throwing;
		//Log(LOG_INFO) << ". . minThrowRule = " << minThrowRule;
	}
	//Log(LOG_INFO) << ". . maxThrowRule = " << maxThrowRule;

	double deviation = static_cast<double>(maxThrowRule) - (accuracy * 100.0);
	deviation = std::max(
						0.0,
						deviation * targetDist / 200);

	Log(LOG_INFO) << ". . deviation = " << deviation;

	double
		dx = RNG::boxMuller(0.0, deviation),
		dy = RNG::boxMuller(0.0, deviation),
		dz = RNG::boxMuller(0.0, deviation);

	target->x += static_cast<int>(RNG::generate(0.0, dx) - dx / 2.0);
	target->y += static_cast<int>(RNG::generate(0.0, dy) - dy / 2.0);
	target->z += static_cast<int>(RNG::generate(0.0, dz / 4.0) - dz / 8.0);
	// kL_end.


	if (extendLine) // kL_note: This is for aimed projectiles; always false outside my RangedBased above.
	{
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

		double
			rotation = atan2(
						static_cast<double>(target->y - origin.y),
						static_cast<double>(target->x - origin.x))
						* 180.0 / M_PI,
			tilt = atan2(
						static_cast<double>(target->z - origin.z),
						sqrt(
							static_cast<double>(target->x - origin.x)
									* static_cast<double>(target->x - origin.x)
								+ static_cast<double>(target->y - origin.y)
									* static_cast<double>(target->y - origin.y)))
						* 180.0 / M_PI;

		// calculate new target
		// this new target can be very far out of the map, but we don't care about that right now
		double
			cos_fi = cos(tilt * M_PI / 180.0),
			sin_fi = sin(tilt * M_PI / 180.0),
			cos_te = cos(rotation * M_PI / 180.0),
			sin_te = sin(rotation * M_PI / 180.0);

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
