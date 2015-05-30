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

#include "Projectile.h"

//#define _USE_MATH_DEFINES
//#include <cmath>
//#include "../fmath.h"

#include "Map.h"
#include "Camera.h"
#include "Particle.h"
#include "Pathfinding.h"
#include "TileEngine.h"

//#include "../Engine/Game.h"
//#include "../Engine/Options.h"
//#include "../Engine/RNG.h"
//#include "../Engine/Surface.h"
#include "../Engine/SurfaceSet.h"

#include "../Resource/ResourcePack.h"

//#include "../Ruleset/RuleArmor.h"
//#include "../Ruleset/MapData.h"
//#include "../Ruleset/RuleItem.h"
#include "../Ruleset/RuleSoldier.h"
//#include "../Ruleset/RuleUnit.h"

#include "../Savegame/BattleItem.h"
//#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
//#include "../Savegame/Soldier.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

/**
 * Creates a Projectile on the battle map and calculates its movement.
 * @param res			- pointer to ResourcePack
 * @param save			- pointer to SavedBattleGame
 * @param action		- the BattleAction (BattlescapeGame.h)
 * @param origin		- position that this projectile originates at in tilespace
 * @param targetVoxel	- position that this projectile is targeted at in voxelspace
// * @param ammo			- pointer to the ammo that produced this projectile if applicable
 */
Projectile::Projectile(
		ResourcePack* res,
		SavedBattleGame* save,
		BattleAction action,
		Position origin,
		Position targetVoxel)
//		BattleItem* ammo) // -> 'bullet'
	:
		_res(res),
		_save(save),
		_action(action),
		_origin(origin),
		_targetVoxel(targetVoxel),
		_position(0),
		_bulletSprite(-1),
		_reversed(false)
//		_vaporColor(-1),
//		_vaporDensity(-1),
//		_vaporProbability(5)
{
	//Log(LOG_INFO) << "\n";
	//Log(LOG_INFO) << "cTor origin = " << origin;
	//Log(LOG_INFO) << "cTor target = " << targetVoxel << " tSpace " << (targetVoxel / Position(16,16,24));

	//Log(LOG_INFO) << "Projectile";
	//Log(LOG_INFO) << ". action.weapon = " << _action.weapon->getRules()->getType();
	//Log(LOG_INFO) << ". bullet = " << _action.weapon->getAmmoItem()->getRules()->getType();

	_speed = Options::battleFireSpeed; // this is the number of pixels the sprite will move between frames

	if (_action.weapon != NULL)
	{
		if (_action.type == BA_THROW)
		{
			//Log(LOG_INFO) << "Create Projectile -> BA_THROW";
			_throwSprite =_res->getSurfaceSet("FLOOROB.PCK")->getFrame(getItem()->getRules()->getFloorSprite());
			_speed = _speed * 3 / 7;
		}
		else // ba_SHOOT!! or hit, or spit
		{
			//Log(LOG_INFO) << "Create Projectile -> not BA_THROW";
			if (_action.weapon->getRules()->getArcingShot() == true)
				_speed = _speed * 4 / 7;

			const BattleItem* const bullet = _action.weapon->getAmmoItem(); // the weapon itself if not-req'd. eg, lasers/melee
			if (bullet != NULL) // try to get the required info from the bullet
			{
				_bulletSprite = bullet->getRules()->getBulletSprite();

//				_speed = std::max(1, _speed + bullet->getRules()->getBulletSpeed());
				_speed += bullet->getRules()->getBulletSpeed();

//				_vaporColor = bullet->getRules()->getVaporColor();
//				_vaporDensity = bullet->getRules()->getVaporDensity();
//				_vaporProbability = bullet->getRules()->getVaporProbability();
			}

			// if no bullet or the bullet doesn't contain the required info see what the weapon has to offer.
			if (_bulletSprite == -1)
				_bulletSprite = _action.weapon->getRules()->getBulletSprite();

			if (_speed == Options::battleFireSpeed)
				_speed += _action.weapon->getRules()->getBulletSpeed();

//			if (bullet == NULL
//				|| bullet != _action.weapon)
//				|| bullet->getRules()->getBulletSpeed() == 0)
//			{
//				_speed = std::max(1, _speed + _action.weapon->getRules()->getBulletSpeed());
//			}

//			if (_vaporColor == -1)
//				_vaporColor = _action.weapon->getRules()->getVaporColor();
//			if (_vaporDensity == -1)
//				_vaporDensity = _action.weapon->getRules()->getVaporDensity();
//			if (_vaporProbability == 5) // uhhh why.
//				_vaporProbability = _action.weapon->getRules()->getVaporProbability();
		}
	}

	if (_speed < 1)
		_speed = 1;

	if (_bulletSprite == -1)
	{
		std::wostringstream woststr;
		woststr << L"WARNING: no bullet sprite";
		if (_action.weapon != NULL)
			woststr << L" for " << _action.weapon->getRules()->getType().c_str();
		if (_action.weapon->getAmmoItem() != NULL)
			woststr << L" w/ " << _action.weapon->getAmmoItem()->getRules()->getType().c_str();
		Log(LOG_INFO) << woststr.str().c_str();
	}

/*	NE	 0	reversed
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
	NNE		not reversed */
	if ((targetVoxel.x - origin.x) + (targetVoxel.y - origin.y) > -1)
		_reversed = true;
}

/**
 * Deletes the Projectile.
 */
Projectile::~Projectile()
{}

/**
 * Calculates the trajectory for a straight path.
 * @note This is a wrapper for calculateTrajectory() below - it calculates and
 * passes on an originVoxel.
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
	return calculateTrajectory(
							accuracy,
							_save->getTileEngine()->getOriginVoxel(
															_action,
															_save->getTile(_origin)));
}

/**
 * Calculates the trajectory for a straight path.
 * @note First determines if there is LoF then calculates and stores a modified
 * trajectory that is actually pathed.
 * @param accuracy		- accuracy of the projectile's trajectory (a BattleUnit's accuracy)
 * @param originVoxel	- for Blaster launch; ie trajectories that start at a position other than unit's
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
	const Tile* const targetTile = _save->getTile(_action.target);
	const BattleUnit* const targetUnit = targetTile->getUnit();

	if (_action.actor->getFaction() == FACTION_PLAYER // kL_note: aLiens don't even get in here!
		&& _action.autoShotCount == 1
		&& _action.type != BA_LAUNCH
		&& _save->getBattleGame()->getPanicHandled() == true
		&& (Options::forceFire == false
			|| (SDL_GetModState() & KMOD_CTRL) == 0))
	{
		//Log(LOG_INFO) << ". autoshotCount[0] = " << _action.autoShotCount;
		const VoxelType testVox = static_cast<VoxelType>(
								  _save->getTileEngine()->calculateLine(
																	originVoxel,
																	_targetVoxel,
																	false,
																	&_trajectory,
																	_action.actor));
		//Log(LOG_INFO) << ". testVox = " << (int)testVox;

		if (testVox != VOXEL_EMPTY
			&& _trajectory.empty() == false)
		{
			Position testPos = Position(
									_trajectory.at(0).x / 16,
									_trajectory.at(0).y / 16,
									_trajectory.at(0).z / 24);

			if (testVox == VOXEL_UNIT)
			{
				const Tile* const endTile = _save->getTile(testPos);
				if (endTile != NULL
					&& endTile->getUnit() == NULL)
				{
//					testPos += (Position(0,0,-1));
					testPos = Position( // must be poking head up from tileBelow
									testPos.x,
									testPos.y,
									testPos.z - 1);
				}
			}


			if (testPos != _action.target
				&& _action.result.empty() == true)
			{
				if (testVox == VOXEL_NORTHWALL)
				{
					if (testPos.y - 1 != _action.target.y)
					{
						_trajectory.clear();
						return VOXEL_EMPTY;
					}
				}
				else if (testVox == VOXEL_WESTWALL)
				{
					if (testPos.x - 1 != _action.target.x)
					{
						_trajectory.clear();
						return VOXEL_EMPTY;
					}
				}
				else if (testVox == VOXEL_UNIT)
				{
					const BattleUnit* const testUnit = _save->getTile(testPos)->getUnit();
					if (testUnit != targetUnit
						&& testUnit->getUnitVisible() == true)
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
	}

	_trajectory.clear();

	//Log(LOG_INFO) << ". autoshotCount[1] = " << _action.autoShotCount;

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


	if (targetUnit != NULL
		&& targetUnit->getDashing() == true)
	{
		accuracy -= 0.16;
	}

	if (_action.type != BA_LAUNCH // Could base BL.. on psiSkill, or sumthin'
		&& originVoxel / Position(16,16,24) != _targetVoxel / Position(16,16,24))
	{
		//Log(LOG_INFO) << ". preAcu target = " << _targetVoxel << " tSpace " << (_targetVoxel / Position(16,16,24));
		applyAccuracy( // apply some accuracy modifiers. This will result in a new target voxel:
					originVoxel,
					&_targetVoxel,
					accuracy,
					false,
					targetTile);
		//Log(LOG_INFO) << ". postAcu target = " << _targetVoxel << " tSpace " << (_targetVoxel / Position(16,16,24));
	}

	const int ret = _save->getTileEngine()->calculateLine( // finally do a line calculation and store this trajectory.
													originVoxel,
													_targetVoxel,
													true,
													&_trajectory,
													_action.actor);
	//Log(LOG_INFO) << ". trajBegin = " << _trajectory.front() << " tSpace " << (_trajectory.front() / Position(16,16,24));
	//Log(LOG_INFO) << ". trajFinal = " << _trajectory.back() << " tSpace " << (_trajectory.back() / Position(16,16,24));
	//Log(LOG_INFO) << ". RET voxelType = " << ret;

	return ret;

/*	return _save->getTileEngine()->calculateLine( // finally do a line calculation and store this trajectory.
											originVoxel,
											_targetVoxel,
											true,
											&_trajectory,
											_action.actor); */
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
/*	BattleUnit* bu = _save->getTile(_origin)->getUnit();
	if (bu == NULL)
		bu = _save->getTile(Position(
								_origin.x,
								_origin.y,
								_origin.z - 1))->getUnit(); */

	Position targetVoxel = Position( // determine the target voxel, aim at the center of the floor
								_action.target.x * 16 + 8,
								_action.target.y * 16 + 8,
								_action.target.z * 24 + 2 - _save->getTile(_action.target)->getTerrainLevel());

	if (_action.type != BA_THROW) // ie. celatid acid-spit
	{
		const BattleUnit* targetUnit = _save->getTile(_action.target)->getUnit();
		if (targetUnit == NULL
			&& _action.target.z > 0
			&& _save->getTile(_action.target)->hasNoFloor(NULL))
		{
			targetUnit = _save->getTile(Position(
											_action.target.x,
											_action.target.y,
											_action.target.z - 1))->getUnit();
		}

		if (targetUnit != NULL)
		{
			targetVoxel.z += targetUnit->getHeight() / 2
						   + targetUnit->getFloatHeight();
//kL					   - 2;
//						   + 2; // kL: midriff +2 voxels

			if (targetUnit->getDashing() == true)
				accuracy -= 0.16; // acid-spit, arcing shot.
		}
	}


	const Position originVoxel = _save->getTileEngine()->getOriginVoxel(
																	_action,
																	NULL);
	int ret = static_cast<int>(VOXEL_OUTOFBOUNDS);
	double arc;
	if (_save->getTileEngine()->validateThrow(
										_action,
										originVoxel,
										targetVoxel,
										&arc,
										&ret) == true)
	{
		// Do a parabola calculation and store that trajectory - make sure it's valid.
		VoxelType test = VOXEL_OUTOFBOUNDS;
		while (test == VOXEL_OUTOFBOUNDS)
		{
			Position delta = targetVoxel;
			applyAccuracy(
						originVoxel,
						&delta,
						accuracy,
						true,
						_save->getTile(_action.target),
						false);

			delta -= targetVoxel;

			_trajectory.clear();

			test = static_cast<VoxelType>(_save->getTileEngine()->calculateParabola(
																				originVoxel,
																				targetVoxel,
																				true,
																				&_trajectory,
																				_action.actor,
																				arc,
																				delta));

			// Don't let thrown items land on diagonal bigwalls.
			// this prevents exploiting blast-propagation routine to both sides of a bigWall.diag
			// See also TileEngine::validateThrow()
			if (_action.type == BA_THROW)
			{
				const Tile* const targetTile = _save->getTile(_trajectory.back() / Position(16,16,24)); // _trajectory.at(0) <- see TileEngine::validateThrow()
				if (targetTile != NULL
					&& targetTile->getMapData(MapData::O_OBJECT) != NULL
					&& (targetTile->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_NESW
						|| targetTile->getMapData(MapData::O_OBJECT)->getBigWall() == Pathfinding::BIGWALL_NWSE))
//					&& (action.weapon->getRules()->getBattleType() == BT_GRENADE
//						|| action.weapon->getRules()->getBattleType() == BT_PROXIMITYGRENADE)
//					&& _action.target->getMapData(MapData::O_OBJECT)->getTUCostObject(MT_WALK) == 255)
				{
					test = VOXEL_OUTOFBOUNDS; // prevent Grenades from landing on diagonal BigWalls.
				}
			}
		}

		return ret;
	}

	return static_cast<int>(VOXEL_OUTOFBOUNDS);
}

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
		delta_y = origin.y - target->y;
//		delta_z = origin.z - target->z; // kL_add. <- messes up vertical shots. Do not use.

	const double targetDist = std::sqrt(
							  static_cast<double>((delta_x * delta_x) + (delta_y * delta_y)));
//							  static_cast<double>((delta_x * delta_x) + (delta_y * delta_y) + (delta_z * delta_z)));
	//Log(LOG_INFO) << ". targetDist = " << targetDist;

	double range; // the maximum distance a projectile shall ever travel in voxel space
//	double range = 16000.; // vSpace == ~1000 tiles in tSpace.
//	double range = 3200.0; // vSpace == ~200 tiles in tSpace. <-kL
	if (keepRange == true)
		range = targetDist;
	else
		range = 3200.;
	//Log(LOG_INFO) << ". range = " << range;

//	if (_action.type == BA_HIT)
//		range = 45.0; // up to 2 tiles diagonally (as in the case of reaper vs reaper)


	const RuleItem* const itRule = _action.weapon->getRules(); // <- after reading up, 'const' is basically worthless & wordy waste of effort.
	if (_action.type != BA_THROW
		&& itRule->getArcingShot() == false)
	{
		if (Options::battleUFOExtenderAccuracy == true			// kL: only for xCom heheh ->
			&& _action.actor->getFaction() == FACTION_PLAYER)	// not so sure that this should be Player only
		{
			//Log(LOG_INFO) << ". battleUFOExtenderAccuracy";

			// kL_note: if distance is greater-than the weapon's
			// max range, then ProjectileFlyBState won't allow the shot;
			// so that's already been taken care of ....
			const int shortLimit = itRule->getMinRange();

			int longLimit;
			if (_action.type == BA_SNAPSHOT)
				longLimit = itRule->getSnapRange();
			else if (_action.type == BA_AUTOSHOT)
				longLimit = itRule->getAutoRange();
			else
				longLimit = itRule->getAimRange();

			double rangeModifier;
			const int targetDist_tSpace = static_cast<int>(targetDist / 16.);
			if (targetDist_tSpace < shortLimit)
				rangeModifier = static_cast<double>(((shortLimit - targetDist_tSpace) * itRule->getDropoff())) / 100.;
			else if (longLimit < targetDist_tSpace)
				rangeModifier = static_cast<double>(((targetDist_tSpace - longLimit) * itRule->getDropoff())) / 100.;
			else
				rangeModifier = 0.;

			accuracy = std::max(
							0.,
							accuracy - rangeModifier);
		}

		if (Options::battleRangeBasedAccuracy == true)
		{
			//Log(LOG_INFO) << ". battleRangeBasedAccuracy";
			if (targetTile != NULL)
			{
				accuracy -= targetTile->getSmoke() * 0.01;

				const BattleUnit* const targetUnit = targetTile->getUnit();
				if (targetUnit != NULL)
				{
					if (_action.actor->getOriginalFaction() != FACTION_HOSTILE)
						accuracy -= static_cast<double>(targetTile->getShade()) * 0.01;

					// If targetUnit is kneeled, then accuracy reduced by ~6%.
					// This is a compromise, because vertical deviation is ~2 times less.
					if (targetUnit->isKneeled() == true)
						accuracy -= 0.07;
				}
			}
			else if (_action.actor->getOriginalFaction() != FACTION_HOSTILE) // targeting tile-stuffs.
				accuracy -= 0.01 * static_cast<double>(_save->getGlobalShade());

			if (_action.type == BA_AUTOSHOT)
				accuracy -= static_cast<double>(_action.autoShotCount - 1) * 0.03;

			if (_action.actor->getFaction() == _action.actor->getOriginalFaction()) // if not MC'd take a morale hit to accuracy
				accuracy -= static_cast<double>(10 - ((_action.actor->getMorale() + 9) / 10)) / 100.;

			accuracy = std::max(
							0.01,
							accuracy);


			double
				dH,dV;

			const int autoHit = static_cast<int>(std::ceil(accuracy * 20.)); // chance for Bulls-eye.
			if (RNG::percent(autoHit) == false)
//			if (false)
			{
				double deviation;
				if (_action.actor->getFaction() == FACTION_HOSTILE)
					deviation = 0.15;	// give the poor aLiens an aiming advantage over xCom & Mc'd units
										// DO IT IN RULESET! not here. Okay do it here because of faction/MC-thing ...
				else
					deviation = 0.21;	// for Player

				deviation /= accuracy + 0.13;
				deviation = std::max(
									0.01,
									deviation);
				//Log(LOG_INFO) << ". deviation = " << deviation;


				// The angle deviations are spread using a normal distribution:
				dH = RNG::boxMuller(0., deviation /  6.);			// horizontal miss in radians
				dV = RNG::boxMuller(0., deviation / (6. * 1.69));	// vertical miss in radians
			}
			else
			{
				dH =
				dV = 0.;
			}


			double
				te = 0., // avoid VC++ linker warnings
				fi = 0.,
				cos_fi;
			bool
				calcHori,
				calcVert;

			if (target->y != origin.y
				|| target->x != origin.x)
			{
				calcHori = true;
				te = std::atan2(
							static_cast<double>(target->y - origin.y),
							static_cast<double>(target->x - origin.x))
						+ dH;
			}
			else
				calcHori = false;

			if (target->z != origin.z
				|| AreSame(targetDist, 0.) == false)
			{
				calcVert = true;
				fi = std::atan2(
							static_cast<double>(target->z - origin.z),
							targetDist)
						+ dV,
				cos_fi = std::cos(fi);
			}
			else
			{
				calcVert = false;
				cos_fi = 1.;
			}

//			if (extendLine == true) // kL_note: This is for aimed projectiles; always true in my RangedBased here.
//			{
			//Log(LOG_INFO) << "Projectile::applyAccuracy() extendLine";
			// It is a simple task - to hit a target width of 5-7 voxels. Good luck!
			if (calcHori == true)
			{
				target->x = static_cast<int>(Round(static_cast<double>(origin.x) + range * std::cos(te) * cos_fi));
				target->y = static_cast<int>(Round(static_cast<double>(origin.y) + range * std::sin(te) * cos_fi));
			}

			if (calcVert == true)
				target->z = static_cast<int>(Round(static_cast<double>(origin.z) + range * std::sin(fi)));
//			}

			//Log(LOG_INFO) << ". x = " << target->x;
			//Log(LOG_INFO) << ". y = " << target->y;
			//Log(LOG_INFO) << ". z = " << target->z;
		}
		//Log(LOG_INFO) << "Projectile::applyAccuracy() rangeBased EXIT";
	}
	else // *** This is for Throwing and AcidSpitt only ***
	{
		accuracy = accuracy * 50. + 69.3; // arbitrary adjustment.

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
								// - that is, this *OUGHT NEVER RUN* in this Build.
		{
			Log(LOG_INFO) << ". Projectile::applyAccuracy() ERROR : extendLine";
/*			double maxDeviation = 2.5; // maxDeviation is the max angle deviation for accuracy 0% in degrees
			double minDeviation = 0.4; // minDeviation is the min angle deviation for accuracy 100% in degrees
			double dRot, dTilt;
			double rotation, tilt;
			double baseDeviation = (maxDeviation - (maxDeviation * accuracy)) + minDeviation;

			// the angle deviations are spread using a normal distribution between 0 and baseDeviation
			if (RNG::generate(0., 1.) < accuracy) // check if we hit
			{
				dRot = 0.; // we hit, so no deviation
				dTilt = 0.;
			}
			else
			{
				dRot = RNG::boxMuller(0., baseDeviation);
				dTilt = RNG::boxMuller(0., baseDeviation / 2.); // tilt deviation is halved
			}
			rotation += dRot; // add deviations
			tilt += dTilt; */

			const double
				rotation = std::atan2(
									static_cast<double>(target->y - origin.y),
									static_cast<double>(target->x - origin.x))
								* 180. / M_PI,
				tilt = std::atan2(
								static_cast<double>(target->z - origin.z),
								std::sqrt(
									  static_cast<double>(target->x - origin.x) * static_cast<double>(target->x - origin.x)
									+ static_cast<double>(target->y - origin.y) * static_cast<double>(target->y - origin.y)))
								* 180. / M_PI;

			// calculate new target
			// this new target can be very far out of the map, but we don't care about that right now
			const double
				cos_fi = std::cos(tilt * M_PI / 180.),
				sin_fi = std::sin(tilt * M_PI / 180.),
				cos_te = std::cos(rotation * M_PI / 180.),
				sin_te = std::sin(rotation * M_PI / 180.);

			target->x = static_cast<int>(static_cast<double>(origin.x) + range * cos_te * cos_fi);
			target->y = static_cast<int>(static_cast<double>(origin.y) + range * sin_te * cos_fi);
			target->z = static_cast<int>(static_cast<double>(origin.z) + range * sin_fi);
		}
		//Log(LOG_INFO) << "Projectile::applyAccuracy() EXIT";
	}
}

/**
 * Moves further along the trajectory-path.
 * @return, true while trajectory is pathing; false when finished - no new position exists in the trajectory vector
 */
bool Projectile::traceProjectile()
{
	for (int
			i = 0;
			i != _speed;
			++i)
	{
		++_position;

		if (_position == _trajectory.size())
		{
			--_position; // ie. don't pass the end of the _trajectory vector
			return false;
		}

/*		if (_save->getDepth() > 0
			&& _vaporColor != -1
			&& _action.type != BA_THROW
			&& RNG::percent(_vaporProbability) == true)
		{
			addVaporCloud();
		} */
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
	offset += static_cast<int>(_position);
	if (offset > -1
		&& offset < static_cast<int>(_trajectory.size()))
	{
		return _trajectory.at(static_cast<size_t>(offset));
	}

	return _trajectory.at(_position);
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

	return NULL;
}

/**
 * Gets the thrown object's sprite.
 * @return, pointer to Surface
 */
Surface* Projectile::getSprite() const
{
	return _throwSprite;
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
	return _trajectory.front() / Position(16,16,24);
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
	return _trajectory.back() / Position(16,16,24);
}

/**
 * Stores the final direction of a missile or thrown-object
 * for use by TileEngine blast propagation - against diagonal BigWalls.
 */
void Projectile::storeProjectileDirection() const
{
	int dir = -1;

	const size_t trajSize = _trajectory.size();

	if (trajSize > 2)
	{
		const Position
			finalPos = _trajectory.back(),
			prePos = _trajectory.at(trajSize - 3);

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

		const Position relPos = Position(x,y,0);
		Pathfinding::vectorToDirection(relPos, dir);
	}

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
/* void Projectile::addVaporCloud() // private.
{
	Tile* const tile = _save->getTile(_trajectory.at(_position) / Position(16,16,24));
	if (tile != NULL)
	{
		Position
			tilePos,
			voxelPos;

		_save->getBattleGame()->getMap()->getCamera()->convertMapToScreen(
																	_trajectory.at(_position) / Position(16,16,24),
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
												static_cast<float>(voxelPos.x - tilePos.x + RNG::seedless(0,4) - 2),
												static_cast<float>(voxelPos.y - tilePos.y + RNG::generate(0,4) - 2),
												static_cast<float>(RNG::seedless(48,224)),
												static_cast<Uint8>(_vaporColor),
												static_cast<Uint8>(RNG::seedless(32,44)));
			tile->addParticle(particle);
		}
	}
} */

/**
 * Gets a pointer to the BattleAction actor directly.
 * @return, pointer to the currently acting BattleUnit
 */
BattleUnit* Projectile::getActor() const
{
	return _action.actor;
}

}
