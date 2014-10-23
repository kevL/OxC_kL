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
#include "../Ruleset/Ruleset.h"

#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

/**
 * Sets up an ExplosionBState.
 * @param parent		- pointer to the BattlescapeGame
 * @param center		- center position in voxelspace
 * @param item			- pointer to item involved in the explosion (eg grenade)
 * @param unit			- pointer to unit involved in the explosion (eg unit throwing the grenade, cyberdisc, etc)
 * @param tile			- pointer to tile the explosion is on (default NULL)
 * @param lowerWeapon	- true to tell the unit causing this explosion to lower their weapon (default false)
 * @param success		- true if the (melee) attack was succesful (default false)
 */
ExplosionBState::ExplosionBState(
		BattlescapeGame* parent,
		Position center,
		BattleItem* item,
		BattleUnit* unit,
		Tile* tile,
		bool lowerWeapon,
		bool success) // kL_add.
	:
		BattleState(parent),
		_center(center),
		_item(item),
		_unit(unit),
		_tile(tile),
		_lowerWeapon(lowerWeapon),
		_hitSuccess(success), // kL
		_power(0),
		_areaOfEffect(false),
		_pistolWhip(false),
		_hit(false),
		_extend(3) // kL, extra think-cycles before this state Pops.
{
	//Log(LOG_INFO) << "Create ExplosionBState center = " << center;
}

/**
 * Deletes the ExplosionBState.
 */
ExplosionBState::~ExplosionBState()
{
	_parent->setStateInterval(BattlescapeState::DEFAULT_ANIM_SPEED); // kL
}

/**
 * Initializes the explosion. The animation and sound starts here.
 * If the animation is finished, the actual effect takes place.
 */
void ExplosionBState::init()
{
	//Log(LOG_INFO) << "ExplosionBState::init()";
	if (_item != NULL)
	{
		//Log(LOG_INFO) << ". type = " << (int)_item->getRules()->getDamageType();
		if (_item->getRules()->getBattleType() != BT_PSIAMP) // kL: pass by. Let cTor initialization handle it.
		{
			_power = _item->getRules()->getPower();

			// getCurrentAction() only works for player actions: aliens cannot melee attack with rifle butts.
			_pistolWhip = _unit != NULL
					   && _unit->getFaction() == FACTION_PLAYER
					   && _item->getRules()->getBattleType() != BT_MELEE
					   && _parent->getCurrentAction()->type == BA_HIT;

			if (_pistolWhip)
				_power = _item->getRules()->getMeleePower();

			// since melee aliens don't use a conventional weapon type, use their strength instead.
			if (_unit != NULL
				&& (_pistolWhip
					|| _item->getRules()->getBattleType() == BT_MELEE)
				&& _item->getRules()->isStrengthApplied())
			{
				int str = static_cast<int>(Round(
							static_cast<double>(_unit->getStats()->strength) * (_unit->getAccuracyModifier() / 2.0 + 0.5)));

				if (_pistolWhip)
					str /= 2; // pistolwhipping adds only 1/2 str.

				_power += str;
			}
			//Log(LOG_INFO) << ". _power(_item) = " << _power;

			// HE, incendiary, smoke or stun bombs create AOE explosions;
			// all the rest hits one point: AP, melee (stun or AP), laser, plasma, acid
			_areaOfEffect = _pistolWhip == false
						 && _item->getRules()->getBattleType() != BT_MELEE
//						 && _item->getRules()->getBattleType() != BT_PSIAMP
						 && _item->getRules()->getExplosionRadius() > -1;
		}
	}
	else if (_tile != NULL)
	{
		_power = _tile->getExplosive();
		//Log(LOG_INFO) << ". _power(_tile) = " << _power;

		_areaOfEffect = true;
	}
	else if (_unit // cyberdiscs!!! And ... ZOMBIES.
		&& _unit->getSpecialAbility() == SPECAB_EXPLODEONDEATH)
	{
		_power = _parent->getRuleset()->getItem(_unit->getArmor()->getCorpseGeoscape())->getPower();
		_power = (RNG::generate(
							_power * 2 / 3,
							_power * 3 / 2)
				+ RNG::generate(
							_power * 2 / 3,
							_power * 3 / 2))
			/ 2;

		_areaOfEffect = true;
	}
	else // unhandled cyberdisc!!!
	{
		_power = (RNG::generate(67, 137)
				+ RNG::generate(67, 137))
			/ 2;
		//Log(LOG_INFO) << ". _power(Cyberdisc) = " << _power;

		_areaOfEffect = true;
	}


	Position centerPos = Position(
								_center.x / 16,
								_center.y / 16,
								_center.z / 24);

	if (_areaOfEffect)
	{
		//Log(LOG_INFO) << ". . new Explosion(AoE)";
		if (_power > 0)
		{
			Position pos = _center; // voxelspace
			int
				frameStart = ResourcePack::EXPLOSION_OFFSET,
				frameDelay = 0,
				radius = 0,
				qty = _power,
				offset;

			if (_item != NULL)
			{
				frameStart = _item->getRules()->getHitAnimation();
				radius = _item->getRules()->getExplosionRadius();
				//Log(LOG_INFO) << ". . . getExplosionRadius() -> " << radius;

				if (_item->getRules()->getDamageType() == DT_SMOKE
					|| _item->getRules()->getDamageType() == DT_STUN)
				{
//					frameStart = 8;
					qty = qty * 2 / 3; // smoke & stun bombs do fewer anims.
				}
				else
					qty = qty * 3 / 2; // bump this up.
			}
			else
				radius = _power / 9; // <- for cyberdiscs & terrain expl.
			//Log(LOG_INFO) << ". . . radius = " << radius;

			if (radius < 0)
				radius = 0;

			offset = radius * 6; // voxelspace
//			qty = static_cast<int>(sqrt(static_cast<double>(radius) * static_cast<double>(qty))) / 3;
			qty = radius * qty / 100;
			if (qty < 1
				|| offset == 0)
			{
				qty = 1;
			}

			if (_parent->getDepth() > 0)
				frameStart -= Explosion::FRAMES_EXPLODE;

			//Log(LOG_INFO) << ". . . offset(total) = " << offset;
			//Log(LOG_INFO) << ". . . qty = " << qty;
			for (int
					i = 0;
					i < qty;
					++i)
			{
				if (i > 0) // bypass 1st explosion: it's always centered w/out any delay.
				{
//					pos.x += RNG::generate(-offset, offset); // these cause anims to sweep across the battlefield.
//					pos.y += RNG::generate(-offset, offset);
					pos.x = _center.x + RNG::generate(-offset, offset);
					pos.y = _center.y + RNG::generate(-offset, offset);

					if (RNG::generate(0, 1))
						frameDelay++;
				}

				Explosion* explosion = new Explosion( // animation
												pos + Position(11, 11, 0), // jogg the anim down a few pixels. Tks.
												frameStart,
												frameDelay,
												true);

				_parent->getMap()->getExplosions()->push_back(explosion);
			}

			_parent->setStateInterval(BattlescapeState::DEFAULT_ANIM_SPEED); // * 10 / 7);


			int sound = -1;

			if (_item != NULL)
				sound = _item->getRules()->getHitSound();
			else if (_power < 73)
				sound = ResourcePack::SMALL_EXPLOSION;
			else
				sound = ResourcePack::LARGE_EXPLOSION;

			if (sound != -1)
				_parent->getResourcePack()->getSoundByDepth(
														_parent->getDepth(),
														sound)
													->play(
														-1,
														_parent->getMap()->getSoundAngle(centerPos));

			Camera* exploCam = _parent->getMap()->getCamera();
			if (exploCam->isOnScreen(centerPos) == false)
			{
				exploCam->centerOnPosition(
										centerPos,
										false);
			}
			else if (exploCam->getViewLevel() != centerPos.z)
				exploCam->setViewLevel(centerPos.z);
		}
		else
			_parent->popState();
	}
	else // create a bullet hit, or melee hit, or psi-hit, or acid spit hit
	{
		//Log(LOG_INFO) << ". . new Explosion(point)";
		_hit = _pistolWhip
			|| _item->getRules()->getBattleType() == BT_MELEE
			|| _item->getRules()->getBattleType() == BT_PSIAMP;	// includes aLien psi-weapon.
																// They took this out and use 'bool psi' instead ....
																// supposedly to correct some cursor-stuff that was broke for them.
		int
			anim = _item->getRules()->getHitAnimation(),
			sound = _item->getRules()->getHitSound();

		if (_hit)
		{
			anim = _item->getRules()->getMeleeAnimation();

			if (_item->getRules()->getBattleType() != BT_PSIAMP)
				sound = -1; // kL, done in ProjectileFlyBState for melee hits.

//			sound = _item->getRules()->getMeleeHitSound(); // this would mute Psi-hit sound.
		}

		if (sound != -1)
			_parent->getResourcePack()->getSoundByDepth(
													_parent->getDepth(),
													sound)
												->play(
													-1,
													_parent->getMap()->getSoundAngle(centerPos));

//		if (_hitSuccess
//			|| _hit == false)	// note: This would prevent map-reveal on aLien melee attacks.
								// Because reveal depends if Explosions are queued ....
//		{

		int hitResult = 0;
		if (_hit)
		{
			if (_hitSuccess
				|| _item->getRules()->getBattleType() == BT_PSIAMP)
			{
				hitResult = 1;
			}
			else
				hitResult = -1;
		}

		//Log(LOG_INFO) << ". . ExplosionB::init() center = " << _center;
		Explosion* explosion = new Explosion( // animation. Don't turn the tile
										_center,
										anim,
										0,
										false,
										hitResult);
		_parent->getMap()->getExplosions()->push_back(explosion);

		_parent->setStateInterval(std::max(
										1,
										((BattlescapeState::DEFAULT_ANIM_SPEED * 5 / 7) - (10 * _item->getRules()->getExplosionSpeed()))));
//		}

		Camera* exploCam = _parent->getMap()->getCamera();
		if (exploCam->isOnScreen(centerPos) == false
			|| (_parent->getSave()->getSide() != FACTION_PLAYER
				&& _item->getRules()->getBattleType() == BT_PSIAMP))
		{
			exploCam->centerOnPosition(
									centerPos,
									false);
		}
		else if (exploCam->getViewLevel() != centerPos.z)
			exploCam->setViewLevel(centerPos.z);
	}
	//Log(LOG_INFO) << "ExplosionBState::init() EXIT";
}

/**
 * Animates explosion sprites. If their animation is finished remove them from the list.
 * If the list is empty, this state is finished and the actual calculations take place.
 * kL rewrite: Allow a few extra cycles for explosion animations to dissipate.
 */
void ExplosionBState::think()
{
	for (std::list<Explosion*>::const_iterator
			i = _parent->getMap()->getExplosions()->begin();
			i != _parent->getMap()->getExplosions()->end();
			)
	{
		if ((*i)->animate() == false)
		{
			delete *i;
			i = _parent->getMap()->getExplosions()->erase(i);
		}
		else
			++i;
	}

	if (_parent->getMap()->getExplosions()->empty())
		_extend--; // not working as intended; needs to go to Explosion class, so that explosions-vector doesn't 'empty' so fast.

	if (_extend < 1)
	{
		explode();
		return;
	}
}
/*	for (std::list<Explosion*>::const_iterator
			i = _parent->getMap()->getExplosions()->begin();
			i != _parent->getMap()->getExplosions()->end();
			)
	{
		if (!(*i)->animate())
		{
			delete *i;
			i = _parent->getMap()->getExplosions()->erase(i);
			if (_parent->getMap()->getExplosions()->empty())
			{
				explode();
				return;
			}
		}
		else
			++i;
	} */

/**
 * Explosions cannot be cancelled.
 */
void ExplosionBState::cancel()
{
}

/**
 * Calculates the effects of an attack.
 * After the animation is done, the real explosion/hit takes place here!
 * kL_note: This function passes to TileEngine::explode() or TileEngine::hit()
 * depending on if it came from a bullet/psi/melee/spit or an actual explosion;
 * that is, "explode" here means "attack has happened". Typically called from
 * either ProjectileFlyBState::think() or BattlescapeGame::endGameTurn()/checkForProximityGrenades()
 */
void ExplosionBState::explode()
{
	//Log(LOG_INFO) << "ExplosionBState::explode()";
	if (_item != NULL
		&& _item->getRules()->getBattleType() == BT_PSIAMP)
	{
		_parent->popState();
		return;
	}


	// kL_note: melee Hit success/failure, and hit/miss sound-FX, are determined in ProjectileFlyBState.

	SavedBattleGame* save = _parent->getSave();
	TileEngine* tileEngine = save->getTileEngine();

	if (_hit)
	{
		// kL_note: Try moving this to TileEngine::hit()
		// so I can tell whether Firing XP is to be awarded or not, there.
		// dang, screws up multiple shot per turn.
		save->getBattleGame()->getCurrentAction()->type = BA_NONE;

		if (_unit != NULL
			&& _unit->isOut() == false)
		{
			_unit->aim(false);
			_unit->setCache(NULL);
		}

		if (_unit != NULL
			&& _unit->getOriginalFaction() == FACTION_PLAYER
			&& _unit->getFaction() == FACTION_PLAYER)
		{
			_unit->addMeleeExp(); // 1xp for swinging

			if (_hitSuccess)
			{
				BattleUnit* targetUnit = save->getTile(_center / Position(16, 16, 24))->getUnit();
				if (targetUnit
					&& targetUnit->getFaction() == FACTION_HOSTILE)
				{
					_unit->addMeleeExp(); // +1xp for hitting an aLien OR Mc'd xCom agent
				}
			}
		}

		if (_hitSuccess == false) // MISS.
		{
			_parent->getMap()->cacheUnits();

			_parent->popState();
			return;
		}
	}

	if (_item)
	{
		//Log(LOG_INFO) << ". _item is VALID";
		if (_unit == NULL
			&& _item->getPreviousOwner())
		{
			_unit = _item->getPreviousOwner();
		}

		if (_areaOfEffect)
		{
			//Log(LOG_INFO) << ". . AoE, TileEngine::explode()";
			tileEngine->explode(
							_center,
							_power,
							_item->getRules()->getDamageType(),
							_item->getRules()->getExplosionRadius(),
							_unit);
		}
		else
		{
			//Log(LOG_INFO) << ". . not AoE, -> TileEngine::hit()";
			ItemDamageType type = _item->getRules()->getDamageType();
			if (_pistolWhip)
				type = DT_STUN;

			//Log(LOG_INFO) << ". . ExplosionB::explode() center = " << _center;
			BattleUnit* victim = tileEngine->hit(
												_center,
												_power,
												type,
												_unit,
												_hit);

			if (_item->getRules()->getZombieUnit().empty() == false // check if this unit turns others into zombies
				&& victim
				&& victim->getArmor()->getSize() == 1
				&& (victim->getGeoscapeSoldier() != NULL
//					|| (victim->getUnitRules() &&
					|| victim->getUnitRules()->getMechanical() == false)
//				&& victim->getTurretType() == -1
				&& victim->getSpawnUnit().empty() == true
//				&& victim->getSpecialAbility() == SPECAB_NONE // kL
				&& victim->getOriginalFaction() != FACTION_HOSTILE) // only xCom & civies
			{
				//Log(LOG_INFO) << victim->getId() << ": murderer is *zombieUnit*; !spawnUnit -> specab->RESPAWN, ->zombieUnit!";
				victim->setSpawnUnit(_item->getRules()->getZombieUnit());
				victim->setSpecialAbility(SPECAB_RESPAWN);
			}
		}
	}


	bool terrain = false;

	if (_tile != NULL)
	{
		tileEngine->explode(
						_center,
						_power,
						DT_HE,
						_power / 10);
		terrain = true;
	}
	else if (_item == NULL) // explosion not caused by terrain or an item, must be a cyberdisc
	{
		int radius = 6;
		if (_unit != NULL
			&& _unit->getSpecialAbility() == SPECAB_EXPLODEONDEATH)
		{
			radius = _parent->getRuleset()->getItem(_unit->getArmor()->getCorpseGeoscape())->getExplosionRadius();
		}

		tileEngine->explode(
						_center,
						_power,
						DT_HE,
						radius);
		terrain = true;
	}


	_parent->checkForCasualties(
							_item,
							_unit,
							false,
							terrain);

	if (_unit != NULL // if this explosion was caused by a unit shooting, now put the gun down
		&& _unit->isOut() == false
		&& _lowerWeapon)
	{
		_unit->aim(false);
		_unit->setCache(NULL);
	}

	_parent->getMap()->cacheUnits();
	_parent->popState();


	Tile* tile = tileEngine->checkForTerrainExplosions();
	if (tile != NULL) // check for terrain explosions
	{
		Position pVoxel = Position(
								tile->getPosition().x * 16 + 8,
								tile->getPosition().y * 16 + 8,
								tile->getPosition().z * 24 + 2);
		_parent->statePushFront(new ExplosionBState(
												_parent,
												pVoxel,
												NULL,
												_unit,
												tile));
	}

	if (_item != NULL
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
	//Log(LOG_INFO) << "ExplosionBState::explode() EXIT";
}

}
