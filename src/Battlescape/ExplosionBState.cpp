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

#include "ExplosionBState.h"

#include "BattlescapeState.h"
#include "Camera.h"
#include "Explosion.h"
#include "Map.h"
#include "TileEngine.h"
#include "UnitDieBState.h"

#include "../Engine/Game.h"
//#include "../Engine/RNG.h"
#include "../Engine/Sound.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleArmor.h"
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
 * @param meleeSuccess	- true if the (melee) attack was succesful (default false)
 * @param forceCenter	- forces Camera to center on the explosion (default false)
// * @param cosmetic		- new undocumented parameter
 */
ExplosionBState::ExplosionBState(
		BattlescapeGame* parent,
		Position center,
		BattleItem* item,
		BattleUnit* unit,
		Tile* tile,
		bool lowerWeapon,
		bool meleeSuccess,	// kL_add.
		bool forceCenter)	// kL_add.
//		bool cosmetic)
	:
		BattleState(parent),
		_center(center),
		_item(item),
		_unit(unit),
		_tile(tile),
		_lowerWeapon(lowerWeapon),
		_hitSuccess(meleeSuccess),	// kL
		_forceCenter(forceCenter),	// kL
//		_cosmetic(cosmetic),
		_power(0),
		_areaOfEffect(false),
		_pistolWhip(false),
		_hit(false),
		_extend(3) // kL, extra think-cycles before this state is allowed to Pop.
{}

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
	if (_item != NULL)
	{
		if (_item->getRules()->getBattleType() != BT_PSIAMP) // kL: pass by. Let cTor initialization handle it.
		{
			_power = _item->getRules()->getPower();

			// getCurrentAction() only works for player actions: aliens cannot melee attack with rifle butts.
			_pistolWhip = _unit != NULL
					   && _unit->getFaction() == FACTION_PLAYER
					   && _item->getRules()->getBattleType() != BT_MELEE
					   && _parent->getCurrentAction()->type == BA_HIT;

			if (_pistolWhip == true)
				_power = _item->getRules()->getMeleePower();

			// since melee aliens don't use a conventional weapon type, use their strength instead.
			if (_unit != NULL
				&& (_pistolWhip == true
					|| _item->getRules()->getBattleType() == BT_MELEE)
				&& _item->getRules()->isStrengthApplied() == true)
			{
				int extraPower = static_cast<int>(Round(
								 static_cast<double>(_unit->getBaseStats()->strength) * (_unit->getAccuracyModifier() / 2. + 0.5)));

				if (_pistolWhip == true)
					extraPower /= 2; // pistolwhipping adds only 1/2 extraPower.

				if (_unit->isKneeled() == true)
					extraPower /= 2; // kneeled units further half extraPower.

				_power += extraPower;
			}

			// HE, incendiary, smoke or stun bombs create AOE explosions;
			// all the rest hits one point: AP, melee (stun or AP), laser, plasma, acid
			_areaOfEffect = _pistolWhip == false
						 && _item->getRules()->getBattleType() != BT_MELEE
//						 && _item->getRules()->getBattleType() != BT_PSIAMP
						 && _item->getRules()->getExplosionRadius() > -1;
//						 && _cosmetic == false // kL_note: foregoes _pistolwhip
		}
	}
	else if (_tile != NULL)
	{
		_power = _tile->getExplosive();
		_areaOfEffect = true;
	}
	else if (_unit != NULL // cyberdiscs!!! And ... ZOMBIES.
		&& (_unit->getSpecialAbility() == SPECAB_EXPLODEONDEATH
			|| _unit->getSpecialAbility() == SPECAB_BURN_AND_EXPLODE))
	{
		_power = _parent->getRuleset()->getItem(_unit->getArmor()->getCorpseGeoscape())->getPower();
		_power = (RNG::generate(
							_power * 2 / 3,
							_power * 3 / 2)
				+ RNG::generate(
							_power * 2 / 3,
							_power * 3 / 2));
		_power /= 2;
		_areaOfEffect = true;
	}
	else // unhandled cyberdisc!!!
	{
		_power = (RNG::generate(67, 137)
				+ RNG::generate(67, 137));
		_power /= 2;
		_areaOfEffect = true;
	}


	const Position targetPos = Position(
									_center.x / 16,
									_center.y / 16,
									_center.z / 24);

	if (_areaOfEffect == true)
	{
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

					if (RNG::percent(50) == true)
						++frameDelay;
				}

				Explosion* const explosion = new Explosion( // animation
														pos + Position(12,12,0), // jogg the anim down a few pixels. Tks.
														frameStart,
														frameDelay,
														true);

				_parent->getMap()->getExplosions()->push_back(explosion);
			}

			_parent->setStateInterval(BattlescapeState::DEFAULT_ANIM_SPEED); // * 10 / 7);


			int sound = -1;
			if (_item != NULL)
			{
				if (_item->getRules()->getHitSound() != -1) // set item's hitSound to -1 for silent.
					sound = _item->getRules()->getHitSound();
			}
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
														_parent->getMap()->getSoundAngle(targetPos));

			Camera* const exploCam = _parent->getMap()->getCamera();
			if (exploCam->isOnScreen(targetPos) == false
				|| _forceCenter == true)
			{
				exploCam->centerOnPosition(
										targetPos,
										false);
			}
			else if (exploCam->getViewLevel() != targetPos.z)
				exploCam->setViewLevel(targetPos.z);
		}
		else
			_parent->popState();
	}
	else // create a bullet hit, or melee hit, or psi-hit, or acid spit hit
	{
		_hit = _pistolWhip
			|| _item->getRules()->getBattleType() == BT_MELEE
			|| _item->getRules()->getBattleType() == BT_PSIAMP;	// includes aLien psi-weapon.
																// They took this out and use 'bool psi' instead ....
																// supposedly to correct some cursor-stuff that was broke for them.
		int
			anim = _item->getRules()->getHitAnimation(),
			sound = _item->getRules()->getHitSound();

//		if (_cosmetic == true)
		if (_hit == true)
		{
			anim = _item->getRules()->getMeleeAnimation();

			if (_item->getRules()->getBattleType() != BT_PSIAMP)
				sound = -1; // kL, done in ProjectileFlyBState for melee hits.

//			sound = _item->getRules()->getMeleeHitSound(); // this would mute Psi-hit sound.
		}

		if (sound != -1)
			_parent->getResourcePack()->getSoundByDepth(
													_parent->getDepth(),
													sound)->play(
															-1,
															_parent->getMap()->getSoundAngle(targetPos));

//		if (_hitSuccess
//			|| _hit == false)	// note: This would prevent map-reveal on aLien melee attacks.
								// Because reveal depends if Explosions are queued ....
//		{
		int hitResult = 0;
		if (_hit == true)
		{
			if (_hitSuccess == true
				|| _item->getRules()->getBattleType() == BT_PSIAMP)
			{
				hitResult = 1;
			}
			else
				hitResult = -1;
		}

/*		Explosion(
				Position _position,
				int frameStart,
				int frameDelay = 0,
				bool big = false,
				int hit = 0); */
		Explosion* const explosion = new Explosion( // animation. Don't turn the tile
												_center,
												anim,
												0,
												false,
												hitResult); // --> _cosmetic effect bleh.
		_parent->getMap()->getExplosions()->push_back(explosion);

		_parent->setStateInterval(std::max(
										1,
										((BattlescapeState::DEFAULT_ANIM_SPEED * 5 / 7) - (_item->getRules()->getExplosionSpeed() * 10))));
//		}

		Camera* const exploCam = _parent->getMap()->getCamera(); // -> apply more cosmetics
		if (exploCam->isOnScreen(targetPos) == false
			|| _forceCenter == true
			|| (_parent->getSave()->getSide() != FACTION_PLAYER
				&& _item->getRules()->getBattleType() == BT_PSIAMP))
		{
			exploCam->centerOnPosition(
									targetPos,
									false);
		}
		else if (exploCam->getViewLevel() != targetPos.z)
			exploCam->setViewLevel(targetPos.z);
	}
}

/**
 * Animates explosion sprites. If their animation is finished remove them from the list.
 * If the list is empty, this state is finished and the actual calculations take place.
 * kL rewrite: Allow a few extra cycles for explosion animations to dissipate.
 */
void ExplosionBState::think()
{
/*	for (std::list<Explosion*>::const_iterator
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

	if (_parent->getMap()->getExplosions()->empty() == true)
		--_extend; // not working as intended; needs to go to Explosion class, so that explosions-vector doesn't 'empty' so fast.

	if (_extend < 1)
		explode(); */
	for (std::list<Explosion*>::const_iterator
			i = _parent->getMap()->getExplosions()->begin();
			i != _parent->getMap()->getExplosions()->end();
			)
	{
		if ((*i)->animate() == false)
		{

			delete *i;
			i = _parent->getMap()->getExplosions()->erase(i);

			if (_parent->getMap()->getExplosions()->empty() == true)
			{
				explode();
				return;
			}
		}
		else
		{
			++i;
		}
	}
}

/**
 * Explosions cannot be cancelled.
 */
void ExplosionBState::cancel()
{}

/**
 * Calculates the effects of an attack.
 * After the animation is done, the real explosion/hit takes place here!
 * kL_note: This function passes to TileEngine::explode() or TileEngine::hit()
 * depending on if it came from a bullet/psi/melee/spit or an actual explosion;
 * that is, "explode" here means "attack has happened". Typically called from
 * either ProjectileFlyBState::think() or BattlescapeGame::endTurnPhase()/checkForProximityGrenades()
 */
void ExplosionBState::explode()
{
	if (_item != NULL
		&& _item->getRules()->getBattleType() == BT_PSIAMP)
	{
		_parent->popState();
		return;
	}

	// kL_note: melee Hit success/failure, and hit/miss sound-FX, are determined in ProjectileFlyBState.

	const SavedBattleGame* const save = _parent->getSave();
	TileEngine* const tileEngine = save->getTileEngine();

	if (_hit == true)
	{
		// kL_note: Try moving this to TileEngine::hit()
		// so I can tell whether Firing XP is to be awarded or not, there.
		// dang, screws up multiple shot per turn.
		save->getBattleGame()->getCurrentAction()->type = BA_NONE;

		if (_unit != NULL)
		{
			if (_unit->isOut() == false)
			{
				_unit->aim(false);
				_unit->setCache(NULL);
			}

			if (_unit->getGeoscapeSoldier() != NULL
				&& _unit->getFaction() == _unit->getOriginalFaction())
			{
				_unit->addMeleeExp(); // 1xp for swinging

				if (_hitSuccess == true)
				{
					const BattleUnit* const targetUnit = save->getTile(_center / Position(16,16,24))->getUnit();
					if (targetUnit != NULL
						&& targetUnit->getFaction() == FACTION_HOSTILE)
					{
						_unit->addMeleeExp(); // +1xp for hitting an aLien OR Mc'd xCom agent
					}
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

	if (_item != NULL)
	{
		if (_unit == NULL
			&& _item->getPreviousOwner() != NULL)
		{
			_unit = _item->getPreviousOwner();
		}

		if (_areaOfEffect == true)
		{
			tileEngine->explode(
							_center,
							_power,
							_item->getRules()->getDamageType(),
							_item->getRules()->getExplosionRadius(),
							_unit,
							_item->getRules()->getBattleType() == BT_GRENADE
								|| _item->getRules()->getBattleType() == BT_PROXIMITYGRENADE);

			tileEngine->setProjectileDirection(-1); // kL
		}
		else // -> if !_cosmetics
		{
			ItemDamageType damageType = _item->getRules()->getDamageType();
			if (_pistolWhip == true)
				damageType = DT_STUN;

			BattleUnit* const victim = tileEngine->hit(
													_center,
													_power,
													damageType,
													_unit,
													_hit);

			if (_item->getRules()->getZombieUnit().empty() == false
				&& victim != NULL
				&& victim->getArmor()->getSize() == 1
				&& (victim->getGeoscapeSoldier() != NULL
					|| victim->getUnitRules()->isMechanical() == false)
				&& victim->getSpawnUnit().empty() == true
				&& victim->getOriginalFaction() != FACTION_HOSTILE)
			{
				//Log(LOG_INFO) << victim->getId() << " murderer has zombieUnit string";
				victim->setSpawnUnit(_item->getRules()->getZombieUnit());
			}
		}
	}


	bool terrain = false;

	if (_tile != NULL)
	{
		ItemDamageType damageType;

		switch (_tile->getExplosiveType())
		{
			case 0: damageType = DT_HE;		break;
			case 5: damageType = DT_IN;		break;
			case 6: damageType = DT_STUN;	break;

			default:
				damageType = DT_SMOKE;
		}

		if (damageType != DT_HE)
			_tile->setExplosive(0,0, true);

		tileEngine->explode(
						_center,
						_power,
						damageType,
						_power / 10);
		terrain = true;
	}
	else if (_item == NULL) // explosion not caused by terrain or an item, must be a cyberdisc
	{
		int radius;
		if (_unit != NULL
			&& (_unit->getSpecialAbility() == SPECAB_EXPLODEONDEATH
				|| _unit->getSpecialAbility() == SPECAB_BURN_AND_EXPLODE))
		{
			radius = _parent->getRuleset()->getItem(_unit->getArmor()->getCorpseGeoscape())->getExplosionRadius();
		}
		else
			radius = 6;

		tileEngine->explode(
						_center,
						_power,
						DT_HE,
						radius);
		terrain = true;
	}


//	if (!_cosmetic)
	_parent->checkForCasualties(
							_item,
							_unit,
							false,
							terrain);


	if (_unit != NULL // if this hit/explosion was caused by a unit, now put the weapon down
		&& _unit->isOut() == false
		&& _lowerWeapon == true)
	{
		_unit->aim(false);
		_unit->setCache(NULL);
	}

	_parent->getMap()->cacheUnits();
	_parent->popState();


	if (_item != NULL
		&& (_item->getRules()->getBattleType() == BT_GRENADE
			|| _item->getRules()->getBattleType() == BT_PROXIMITYGRENADE))
	{
		for (std::vector<BattleItem*>::const_iterator
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


	Tile* const tile = tileEngine->checkForTerrainExplosions(); // check for more exploding tiles
	if (tile != NULL)
	{
		Position pVoxel = Position(
								tile->getPosition().x * 16 + 8,
								tile->getPosition().y * 16 + 8,
								tile->getPosition().z * 24 + 10);
		_parent->statePushFront(new ExplosionBState(
												_parent,
												pVoxel,
												NULL,
												_unit,
												tile,
												false,
												false,
												_forceCenter));
	}
}

}
