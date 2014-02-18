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

#include "ExplosionBState.h"

#include "Camera.h"
#include "BattlescapeState.h"
#include "Explosion.h"
#include "Map.h"
#include "TileEngine.h"
#include "UnitDieBState.h"

#include "../Engine/Game.h"
#include "../Engine/Logger.h"
#include "../Engine/RNG.h"
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
 * Sets up an ExplosionBState.
 * @param parent, Pointer to the BattleScape.
 * @param center, Center position in voxelspace.
 * @param item, Item involved in the explosion (eg grenade).
 * @param unit, Unit involved in the explosion (eg unit throwing the grenade).
 * @param tile, Tile the explosion is on.
 * @param lowerWeapon, Whether the unit causing this explosion should now lower their weapon.
 */
ExplosionBState::ExplosionBState(
		BattlescapeGame* parent,
		Position center,
		BattleItem* item,
		BattleUnit* unit,
		Tile* tile,
		bool lowerWeapon)
	:
		BattleState(parent),
		_unit(unit),
		_center(center),
		_item(item),
		_tile(tile),
		_power(0),
		_areaOfEffect(false),
		_lowerWeapon(lowerWeapon)
{
}

/**
 * Deletes the ExplosionBState.
 */
ExplosionBState::~ExplosionBState()
{
	_parent->setStateInterval(BattlescapeState::DEFAULT_ANIM_SPEED); // kL
}

/**
 * Initializes the explosion.
 * The animation and sound starts here.
 * If the animation is finished, the actual effect takes place.
 */
void ExplosionBState::init()
{
	Log(LOG_INFO) << "ExplosionBState::init()";
//			<< " type = " << (int)_item->getRules()->getDamageType();

	if (_item)
	{
		_power = _item->getRules()->getPower();
		Log(LOG_INFO) << ". _power(_item) = " << _power;

		// heavy explosions, incendiary, smoke or stun bombs create AOE explosions
		// all the rest hits one point:
		// AP, melee (stun or AP), laser, plasma, acid
		_areaOfEffect = _item->getRules()->getBattleType() != BT_MELEE
/*						&& (_item->getRules()->getDamageType() == DT_HE
							|| _item->getRules()->getDamageType() == DT_IN
							|| _item->getRules()->getDamageType() == DT_SMOKE
							|| _item->getRules()->getDamageType() == DT_STUN); */
						&& _item->getRules()->getExplosionRadius() != 0;
	}
	else if (_tile)
	{
		_power = _tile->getExplosive();
		Log(LOG_INFO) << ". _power(_tile) = " << _power;

		_areaOfEffect = true;
	}
	else // cyberdiscs!!!
	{
		_power = RNG::generate(61, 135);
		Log(LOG_INFO) << ". _power(Cyberdisc) = " << _power;

		_areaOfEffect = true;
	}


	Position posCenter = Position(
								_center.x / 16,
								_center.y / 16,
								_center.z / 24);
//	Tile* tileCenter = _parent->getSave()->getTile(posCenter);

	if (_areaOfEffect)
	{
		Log(LOG_INFO) << ". . new Explosion(AoE)";

		if (_power > 0)
		{
			Position posCenter_voxel = _center; // voxelspace
			int
				startFrame = 0; // less than 0 will delay anim-start (total 8 Frames)
//				offset = _power / 2,
//				animQty = _power / 14;

			int radius = 0;
			if (_item)
			{
				radius = _item->getRules()->getExplosionRadius();
				Log(LOG_INFO) << ". . . getExplosionRadius() -> " << radius;
			}
//			if (radius < 1)
			else
				radius = _power / 8; // <- for cyberdiscs & terrain expl.... CTD if using getExplosionRadius(),
			Log(LOG_INFO) << ". . . radius = " << radius;

			int offset = radius * 6, // voxelspace
				animQty = static_cast<int>(
								sqrt(static_cast<double>(radius) * static_cast<double>(_power)))
							/ 6;
			if (animQty < 1)
				animQty = 1;

			Log(LOG_INFO) << ". . . offset(total) = " << offset;
			Log(LOG_INFO) << ". . . animQty = " << animQty;
			for (int
					i = 0;
					i < animQty;
					i++)
			{
				if (i > 0) // 1st exp. is always centered.
				{
					startFrame = RNG::generate(-i, 0) - i / 2;

					posCenter_voxel.x += RNG::generate(-offset, offset);
					posCenter_voxel.y += RNG::generate(-offset, offset);
				}

//				Explosion* explosion = new Explosion(p, startFrame, true);

				Explosion* explosion = new Explosion( // animation
													posCenter_voxel + Position(12, 12, 0), // jogg the anim down a few pixels. Tks.
													startFrame,
													true);

				_parent->getMap()->getExplosions()->insert(explosion); // add the explosion on the map
			}

//kL			_parent->setStateInterval(BattlescapeState::DEFAULT_ANIM_SPEED);
			_parent->setStateInterval(BattlescapeState::DEFAULT_ANIM_SPEED * 8 / 7); // kL

			if (_power < 80)
				_parent->getResourcePack()->getSound("BATTLE.CAT", 12)->play();
			else
				_parent->getResourcePack()->getSound("BATTLE.CAT", 5)->play();

			// kL_begin:
			Camera* explodeCam = _parent->getMap()->getCamera();
			if (!explodeCam->isOnScreen(posCenter))
				explodeCam->centerOnPosition(
											posCenter,
											false);
			else if (explodeCam->getViewLevel() != posCenter.z)
				explodeCam->setViewLevel(posCenter.z);
			// kL_end.
		}
		else
			_parent->popState();
	}
	else // create a bullet hit, or melee hit, or psi-hit, or acid spit
	{
		Log(LOG_INFO) << ". . new Explosion(point)";

		_parent->setStateInterval(std::max(
										1,
										((BattlescapeState::DEFAULT_ANIM_SPEED * 6 / 7) - (10 * _item->getRules()->getExplosionSpeed())))); // kL
//kL										((BattlescapeState::DEFAULT_ANIM_SPEED / 2) - (10 * _item->getRules()->getExplosionSpeed()))));
//kL		_parent->setStateInterval(BattlescapeState::DEFAULT_ANIM_SPEED / 2);
//		_parent->setStateInterval(BattlescapeState::DEFAULT_ANIM_SPEED * 6 / 7); // kL

		bool hit = _item->getRules()->getBattleType() == BT_MELEE
				|| _item->getRules()->getBattleType() == BT_PSIAMP; // includes aLien psi-weapon.

		Explosion* explosion = new Explosion( // animation.
										_center,
										_item->getRules()->getHitAnimation(),
										false,
										hit);

		_parent->getMap()->getExplosions()->insert(explosion);

		_parent->getResourcePack()->getSound(
											"BATTLE.CAT",
											_item->getRules()->getHitSound())
										->play();

		// kL_begin:
		Camera* explodeCam = _parent->getMap()->getCamera();
//		if (_parent->getSave()->getSide() == FACTION_HOSTILE ||
		if (!explodeCam->isOnScreen(posCenter))
		{
			explodeCam->centerOnPosition(
										posCenter,
										false);

		}
		else if (explodeCam->getViewLevel() != posCenter.z)
			explodeCam->setViewLevel(posCenter.z);
		// kL_end.
	}
	Log(LOG_INFO) << "ExplosionBState::init() EXIT";
}

/**
 * Animates explosion sprites. If their animation is finished remove them from the list.
 * If the list is empty, this state is finished and the actual calculations take place.
 */
void ExplosionBState::think()
{
	for (std::set<Explosion*>::const_iterator
			i = _parent->getMap()->getExplosions()->begin(),
				inext = i;
			i != _parent->getMap()->getExplosions()->end();
			i = inext)
	{
		++inext;

		if (!(*i)->animate())
		{
			_parent->getMap()->getExplosions()->erase((*i));

			if (_parent->getMap()->getExplosions()->empty())
				explode();
		}
	}
}

/**
 * Explosions cannot be cancelled.
 */
void ExplosionBState::cancel()
{
}

/**
 * Calculates the effects of the explosion.
 * After the animation is done, the real explosion/hit takes place here!
 */
void ExplosionBState::explode()
{
	Log(LOG_INFO) << "ExplosionBState::explode()";
	SavedBattleGame* save = _parent->getSave();

	if (_item)
	{
		Log(LOG_INFO) << ". _item is VALID";
		if (!_unit
			&& _item->getPreviousOwner())
		{
			_unit = _item->getPreviousOwner();
		}

		if (_areaOfEffect)
		{
			Log(LOG_INFO) << ". . AoE, TileEngine::explode()";
			save->getTileEngine()->explode(
										_center,
										_power,
										_item->getRules()->getDamageType(),
										_item->getRules()->getExplosionRadius(),
										_unit);
		}
		else
		{
			Log(LOG_INFO) << ". . not AoE, TileEngine::hit()";
			bool hit = _item->getRules()->getBattleType() == BT_MELEE;
			BattleUnit* victim = save->getTileEngine()->hit(
														_center,
														_power,
														_item->getRules()->getDamageType(),
														_unit,
														hit); // kL add.

			if (!_item->getRules()->getZombieUnit().empty() // check if this unit turns others into zombies
				&& victim
				&& victim->getArmor()->getSize() == 1
				&& victim->getSpawnUnit().empty()
//				&& victim->getSpecialAbility() == SPECAB_NONE
				&& victim->getOriginalFaction() != FACTION_HOSTILE) // only aLiens & civies
			{
				//Log(LOG_INFO) << victim->getId() << ": murderer is *zombieUnit*; !spawnUnit -> specab->RESPAWN, ->zombieUnit!";
				// converts the victim to a zombie on death
				victim->setSpecialAbility(SPECAB_RESPAWN);
				victim->setSpawnUnit(_item->getRules()->getZombieUnit());
			}
		}
	}


	bool terrainExplosion = false;

	if (_tile)
	{
		save->getTileEngine()->explode(
									_center,
									_power,
									DT_HE,
									_power / 10);
		terrainExplosion = true;
	}
	else if (!_item) // explosion not caused by terrain or an item, must be by a unit (cyberdisc)
	{
		save->getTileEngine()->explode(
									_center,
									_power,
									DT_HE,
									6);
		terrainExplosion = true;
	}


	_parent->checkForCasualties( // now check for new casualties
							_item,
							_unit,
							false,
							terrainExplosion);

	if (_unit // if this explosion was caused by a unit shooting, now it's the time to put the gun down
		&& !_unit->isOut()
		&& _lowerWeapon)
	{
		_unit->aim(false);
	}

	_parent->getMap()->cacheUnits();
	_parent->popState();


	Tile* tile = save->getTileEngine()->checkForTerrainExplosions();
	if (tile) // check for terrain explosions
	{
		Position pVoxel = Position(
								tile->getPosition().x * 16,
								tile->getPosition().y * 16,
								tile->getPosition().z * 24);
		_parent->statePushFront(new ExplosionBState(
												_parent,
												pVoxel,
												0,
												_unit,
												tile));
	}

	if (_item
		&& (_item->getRules()->getBattleType() == BT_GRENADE
			|| _item->getRules()->getBattleType() == BT_PROXIMITYGRENADE))
	{
		for (std::vector<BattleItem*>::iterator
				j = _parent->getSave()->getItems()->begin();
				j != _parent->getSave()->getItems()->end();
				++j)
		{
			if (_item->getId() == (*j)->getId())
			{
				delete *j;
				_parent->getSave()->getItems()->erase(j);

				break;
			}
		}
	}
	Log(LOG_INFO) << "ExplosionBState::explode() EXIT";
}

}
