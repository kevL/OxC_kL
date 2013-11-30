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

#include "ExplosionBState.h"

#include "Camera.h"
#include "BattlescapeState.h"
#include "Explosion.h"
#include "Map.h"
#include "TileEngine.h"
#include "UnitDieBState.h"

#include "../Engine/Game.h"
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
ExplosionBState::ExplosionBState(BattlescapeGame* parent, Position center, BattleItem* item, BattleUnit* unit, Tile* tile, bool lowerWeapon)
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
				&& (_item->getRules()->getDamageType() == DT_HE 
					|| _item->getRules()->getDamageType() == DT_IN 
					|| _item->getRules()->getDamageType() == DT_SMOKE
					|| _item->getRules()->getDamageType() == DT_STUN);
	}
	else if (_tile)
	{
		_power = _tile->getExplosive();
		Log(LOG_INFO) << ". _power(_tile) = " << _power;

		_areaOfEffect = true;
	}
	else // cyberdisc... ?
	{
//kL		_power = 120;
		_power = RNG::generate(61, 120);	// kL
		Log(LOG_INFO) << ". _power(Cyberdisc) = " << _power;

		_areaOfEffect = true;
	}

	Tile* t = _parent->getSave()->getTile(Position(_center.x / 16, _center.y / 16, _center.z / 24));
	if (_areaOfEffect)
	{
		Log(LOG_INFO) << ". . new Explosion(AoE)'s";

		if (_power)
		{
			for (int
					i = 0;
					i < _power / 10;
					i++)
			{
				int X = RNG::generate(-_power / 2, _power / 2);
				int Y = RNG::generate(-_power / 2, _power / 2);

				Position pos = _center;
				pos.x += X; pos.y += Y;

//kL				Explosion* explosion = new Explosion(p, RNG::generate(-3, 6), true);
//				Explosion* explosion = new Explosion(pos, 0, true);
				Explosion* explosion = new Explosion(pos, -3, true);

				_parent->getMap()->getExplosions()->insert(explosion); // add the explosion on the map
			}

//kL			_parent->setStateInterval(BattlescapeState::DEFAULT_ANIM_SPEED);
			_parent->setStateInterval(BattlescapeState::DEFAULT_ANIM_SPEED * 6 / 7);		// kL

			if (_power < 80)
				_parent->getResourcePack()->getSound("BATTLE.CAT", 12)->play();
			else
				_parent->getResourcePack()->getSound("BATTLE.CAT", 5)->play();

			_parent->getMap()->getCamera()->centerOnPosition(t->getPosition(), false);
		}
		else
		{
			_parent->popState();
		}
	}
	else // create a bullet hit, or melee hit, or psi-hit, or acid spit
	{
		Log(LOG_INFO) << ". . new Explosion(point)";

//kL		_parent->setStateInterval(BattlescapeState::DEFAULT_ANIM_SPEED / 2);
		_parent->setStateInterval(BattlescapeState::DEFAULT_ANIM_SPEED * 6 / 7);		// kL

		bool hit = _item->getRules()->getBattleType() == BT_MELEE
				|| _item->getRules()->getBattleType() == BT_PSIAMP;

		Explosion* explosion = new Explosion( // animation.
									_center,
									_item->getRules()->getHitAnimation(),
									false,
									hit);

		_parent->getMap()->getExplosions()->insert(explosion);

		_parent->getResourcePack()->getSound("BATTLE.CAT", _item->getRules()->getHitSound())->play(); // hit sound

		if (_parent->getSave()->getSide() == FACTION_HOSTILE)
			_parent->getMap()->getCamera()->centerOnPosition(t->getPosition(), false);

/*kL		if (hit
			&& t->getVisible())
		{
			_parent->getMap()->getCamera()->centerOnPosition(Position(_center.x / 16, _center.y / 16, _center.z / 24));
		} */
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
			{
				explode();
			}
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
 */
void ExplosionBState::explode()
{
	Log(LOG_INFO) << "ExplosionBState::explode()";

	SavedBattleGame* save = _parent->getSave();

	// after the animation is done, the real explosion/hit takes place
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
														hit);	// kL add.

			if (!_unit->getZombieUnit().empty() // check if this unit turns others into zombies
				&& victim
				&& victim->getArmor()->getSize() == 1
				&& victim->getSpawnUnit().empty()
				&& victim->getSpecialAbility() == SPECAB_NONE)
			{
				//Log(LOG_INFO) << victim->getId() << ": murderer is *zombieUnit*; !spawnUnit -> specab->RESPAWN, ->zombieUnit!";
				// converts the victim to a zombie on death
				victim->setSpecialAbility(SPECAB_RESPAWN);
				victim->setSpawnUnit(_unit->getZombieUnit());
			}
		}
	}


	bool terrainExplosion = false;

	if (_tile)
	{
		save->getTileEngine()->explode(_center, _power, DT_HE, _power / 10);
		terrainExplosion = true;
	}

	if (!_tile && !_item) // explosion not caused by terrain or an item, must be by a unit (cyberdisc)
	{
		save->getTileEngine()->explode(_center, _power, DT_HE, 6);
		terrainExplosion = true;
	}

	// now check for new casualties
	_parent->checkForCasualties(_item, _unit, false, terrainExplosion);

	// if this explosion was caused by a unit shooting, now it's the time to put the gun down
	if (_unit
		&& !_unit->isOut(true, true) // <-- params might cause a graphic anomaly...
		&& _lowerWeapon)
	{
		_unit->aim(false);
	}

	_parent->getMap()->cacheUnits();
	_parent->popState();

	// check for terrain explosions
	Tile* t = save->getTileEngine()->checkForTerrainExplosions();
	if (t)
	{
		Position p = Position(t->getPosition().x * 16, t->getPosition().y * 16, t->getPosition().z * 24);
		_parent->statePushFront(new ExplosionBState(_parent, p, 0, _unit, t));
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
