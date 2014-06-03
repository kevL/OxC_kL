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
		_lowerWeapon(lowerWeapon),
		_pistolWhip(false)
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
	//Log(LOG_INFO) << "ExplosionBState::init()";
//			<< " type = " << (int)_item->getRules()->getDamageType();

	if (_item)
	{
		_power = _item->getRules()->getPower();

		_pistolWhip = _item->getRules()->getBattleType() != BT_MELEE
					&& _parent->getCurrentAction()->type == BA_HIT;
		if (_pistolWhip)
		{
			_power = _item->getRules()->getMeleePower();
		}

		// since melee aliens don't use a conventional weapon type, we use their strength instead.
		if ( //_item->getRules()->getBattleType() == BT_MELEE &&
			_item->getRules()->isStrengthApplied())
		{
			_power += _unit->getStats()->strength;
		}
		//Log(LOG_INFO) << ". _power(_item) = " << _power;

		// heavy explosions, incendiary, smoke or stun bombs create AOE explosions
		// all the rest hits one point:
		// AP, melee (stun or AP), laser, plasma, acid
		_areaOfEffect = !_pistolWhip
						&& _item->getRules()->getBattleType() != BT_MELEE
						&& _item->getRules()->getBattleType() != BT_PSIAMP		// kL
//kL						&& _item->getRules()->getExplosionRadius() != 0		// <- worrisome, kL_note.
						&& _item->getRules()->getExplosionRadius() > -1;		// kL
	}
	else if (_tile)
	{
		_power = _tile->getExplosive();
		//Log(LOG_INFO) << ". _power(_tile) = " << _power;

		_areaOfEffect = true;
	}
	else if (_unit // cyberdiscs!!!
		&& _unit->getSpecialAbility() == SPECAB_EXPLODEONDEATH)
	{
		_power = _parent->getRuleset()->getItem(_unit->getArmor()->getCorpseGeoscape())->getPower();
		_power = RNG::generate(
							_power * 2 / 3,
							_power * 3 / 2); // kL

		_areaOfEffect = true;
	}
	else // unhandled cyberdiscs!!! ( see Xcom1Ruleset.rul )
	{
		_power = RNG::generate(66, 138);
		//Log(LOG_INFO) << ". _power(Cyberdisc) = " << _power;

		_areaOfEffect = true;
	}


	Position posCenter = Position(
								_center.x / 16,
								_center.y / 16,
								_center.z / 24);
//	Tile* tileCenter = _parent->getSave()->getTile(posCenter);

	if (_areaOfEffect)
	{
		//Log(LOG_INFO) << ". . new Explosion(AoE)";
		if (_power > 0)
		{
			Position posCenter_voxel = _center; // voxelspace
			int
				startFrame = 0, // less than 0 will delay anim-start (total 8 Frames)
				radius = 0;

			if (_item)
			{
				radius = _item->getRules()->getExplosionRadius();
				//Log(LOG_INFO) << ". . . getExplosionRadius() -> " << radius;
			}
			else
				radius = _power / 8; // <- for cyberdiscs & terrain expl.
			//Log(LOG_INFO) << ". . . radius = " << radius;

			if (radius < 0)
				radius = 0;

			int
				offset = radius * 5, // voxelspace
				animQty = static_cast<int>(
								sqrt(static_cast<double>(radius) * static_cast<double>(_power)))
							/ 5;
			if (animQty < 1)
				animQty = 1;

			//Log(LOG_INFO) << ". . . offset(total) = " << offset;
			//Log(LOG_INFO) << ". . . animQty = " << animQty;
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
												posCenter_voxel + Position(10, 10, 0), // jogg the anim down a few pixels. Tks.
												startFrame,
												true);

//				_parent->getMap()->getExplosions()->insert(explosion); // kL
				_parent->getMap()->getExplosions()->push_back(explosion); // add the explosion on the map // expl CTD
			}

//kL		_parent->setStateInterval(BattlescapeState::DEFAULT_ANIM_SPEED / 2);
			_parent->setStateInterval(BattlescapeState::DEFAULT_ANIM_SPEED * 8 / 7); // kL

			if (_power < 76)
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
		//Log(LOG_INFO) << ". . new Explosion(point)";
		_parent->setStateInterval(std::max(
										1,
										((BattlescapeState::DEFAULT_ANIM_SPEED * 6 / 7) - (10 * _item->getRules()->getExplosionSpeed())))); // kL
//kL									((BattlescapeState::DEFAULT_ANIM_SPEED / 2) - (10 * _item->getRules()->getExplosionSpeed()))));
//kL	_parent->setStateInterval(BattlescapeState::DEFAULT_ANIM_SPEED / 2);
//		_parent->setStateInterval(BattlescapeState::DEFAULT_ANIM_SPEED * 6 / 7); // kL

		bool hit = _pistolWhip
				|| _item->getRules()->getBattleType() == BT_MELEE
				|| _item->getRules()->getBattleType() == BT_PSIAMP; // includes aLien psi-weapon.
		int
			anim = _item->getRules()->getHitAnimation(),
			sound = _item->getRules()->getHitSound();

		if (hit)
		{
			anim = _item->getRules()->getMeleeAnimation();
//			sound = _item->getRules()->getMeleeHitSound(); // kL, but this mutes Psi-hit sound.
		}

		if (sound != -1) // bullet hit sound
		{
			_parent->getResourcePack()->getSound(
											"BATTLE.CAT",
											sound)
										->play();
		}

		Explosion* explosion = new Explosion( // animation.
										_center,
										anim,
										false,
										hit);
//		_parent->getMap()->getExplosions()->insert(explosion); // kL
		_parent->getMap()->getExplosions()->push_back(explosion); // expl CTD

//kL		_parent->getMap()->getCamera()->setViewLevel(_center.z / 24);

//		BattleUnit* target = tileCenter->getUnit();
//		BattleUnit* target = _parent->getSave()->getTile(_action.target)->getUnit();
//		if (hit && _parent->getSave()->getSide() == FACTION_HOSTILE && target && target->getFaction() == FACTION_PLAYER)
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
	//Log(LOG_INFO) << "ExplosionBState::init() EXIT";
}

/**
 * Animates explosion sprites. If their animation is finished remove them from the list.
 * If the list is empty, this state is finished and the actual calculations take place.
 */
void ExplosionBState::think()
{
//	for (std::set<Explosion*>::const_iterator // kL
//			i = _parent->getMap()->getExplosions()->begin(),
//				next = i;
//			i != _parent->getMap()->getExplosions()->end();
//			i = next)
	for (std::list<Explosion*>::const_iterator // expl CTD
			i = _parent->getMap()->getExplosions()->begin();
			i != _parent->getMap()->getExplosions()->end();
			)
	{
//		++next; // kL

		if (!(*i)->animate())
		{
			delete *i; // delete CTD
//			_parent->getMap()->getExplosions()->erase(i); // kL
			i = _parent->getMap()->getExplosions()->erase(i); // expl CTD, delete CTD

			if (_parent->getMap()->getExplosions()->empty())
			{
				explode();

				return;
			}
		}
		else // expl CTD
			++i;
	}
}

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
 * either ProjectileFlyBState::think() or BattlescapeGame::endTurn()/checkForProximityGrenades()
 */
void ExplosionBState::explode()
{
	//Log(LOG_INFO) << "ExplosionBState::explode()";
	SavedBattleGame* save = _parent->getSave();
	TileEngine* tileEngine = save->getTileEngine(); // kL

	// last minute adjustment: determine if we actually
	if (_parent->getCurrentAction()->type == BA_HIT
		|| _parent->getCurrentAction()->type == BA_STUN)
	{
		save->getBattleGame()->getCurrentAction()->type = BA_NONE;
		if (_unit
			&& !_unit->isOut())
		{
			_unit->aim(false);
			_unit->setCache(0);
		}

		BattleUnit* targetUnit = save->getTile(_center / Position(16, 16, 24))->getUnit();
		if (!RNG::percent(static_cast<int>(_unit->getFiringAccuracy(
												BA_HIT,
//												_parent->getCurrentAction()->type, // kL
												_item)
											* 100.0)))
		{
			_parent->getMap()->cacheUnits();
			_parent->popState();

			return;
		}
		else if (targetUnit
			&& targetUnit->getOriginalFaction() == FACTION_HOSTILE
			&& _unit->getOriginalFaction() == FACTION_PLAYER)
		{
			_unit->addMeleeExp();
		}

		// kL_note: This should play only on a successful hit; there's another sound for the attack-swing.
		if (_item->getRules()->getMeleeHitSound() != -1)
		{
			_parent->getResourcePack()->getSound(
											"BATTLE.CAT",
											_item->getRules()->getMeleeHitSound())
										->play();
		}
	}

	if (_item)
	{
		//Log(LOG_INFO) << ". _item is VALID";
		if (!_unit
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
							_item->getRules()->getExplosionRadius(),	// what about cyberdiscs/terrain explosions?
																		// I need to introduce a _blastRadius class_var for ExplosionBState;
																		// could use _areaOfEffect w/ -1 meaning !NOT!
																		// Because I'd want to keep 0 for single tile explosions.
							_unit);
		}
		else
		{
			//Log(LOG_INFO) << ". . not AoE, TileEngine::hit()";
			bool hit = _pistolWhip											// kL
						|| _item->getRules()->getBattleType() == BT_MELEE	// kL
						|| _item->getRules()->getBattleType() == BT_PSIAMP;	// kL
			// kL_note: basically, PsiAmp is needed 'round here only for its animation to play.

			ItemDamageType type = _item->getRules()->getDamageType();
			if (_pistolWhip)
				type = DT_STUN;

			BattleUnit* victim = tileEngine->hit(
												_center,
												_power,
												type,
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

	// kL_begin:
	if (_item
		&& _item->getRules()->getBattleType() == BT_PSIAMP)
	{
//		_parent->getMap()->cacheUnits();
		_parent->popState();

		return;
	} // kL_end.


	bool terrainExplosion = false;

	if (_tile)
	{
		tileEngine->explode(
						_center,
						_power,
						DT_HE,
						_power / 10);
		terrainExplosion = true;
	}
	else if (!_item) // explosion not caused by terrain or an item, must be by a unit (cyberdisc)
	{
		int radius = 6;
		if (_unit
			&& _unit->getSpecialAbility() == SPECAB_EXPLODEONDEATH)
		{
			radius = _parent->getRuleset()->getItem(_unit->getArmor()->getCorpseGeoscape())->getExplosionRadius();
		}

		tileEngine->explode(
						_center,
						_power,
						DT_HE,
						radius);
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
		_unit->setCache(0);
	}

	_parent->getMap()->cacheUnits();
	_parent->popState();


	Tile* tile = tileEngine->checkForTerrainExplosions();
	if (tile) // check for terrain explosions
	{
		Position pVoxel = Position(
								tile->getPosition().x * 16,
								tile->getPosition().y * 16,
								tile->getPosition().z * 24);
		pVoxel += Position(8, 8, 0);
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
	//Log(LOG_INFO) << "ExplosionBState::explode() EXIT";
}

}
