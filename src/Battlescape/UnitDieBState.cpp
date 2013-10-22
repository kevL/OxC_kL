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

#include "UnitDieBState.h"
#include "ExplosionBState.h"
#include "TileEngine.h"
#include "BattlescapeState.h"
#include "Map.h"
#include "Camera.h"
#include "../Engine/Game.h"
#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"
#include "../Resource/ResourcePack.h"
#include "../Ruleset/Ruleset.h"
#include "../Engine/Sound.h"
#include "../Engine/RNG.h"
#include "../Engine/Options.h"
#include "../Engine/Language.h"
#include "../Ruleset/Armor.h"
#include "../Ruleset/Unit.h"
#include "InfoboxOKState.h"
#include "InfoboxState.h"
#include "../Savegame/Node.h"


namespace OpenXcom
{

/**
 * Sets up an UnitDieBState.
 * @param parent Pointer to the Battlescape.
 * @param unit Dying unit.
 * @param damageType Type of damage that caused the death.
 * @param noSound Whether to disable the death sound.
 */
UnitDieBState::UnitDieBState(BattlescapeGame* parent, BattleUnit* unit, ItemDamageType damageType, bool noSound)
	:
		BattleState(parent),
		_unit(unit),
		_damageType(damageType),
		_noSound(noSound)
{
	//Log(LOG_INFO) << "Create UnitDieBState";

	// don't show the "fall to death" animation when a unit is blasted with explosives or he is already unconscious
/*kL	if (_damageType == DT_HE || _unit->getStatus() == STATUS_UNCONSCIOUS)
	{
		_unit->startFalling();

		while (_unit->getStatus() == STATUS_COLLAPSING)
		{
			_unit->keepFalling();
		}
	}
	else */
//	{
	if (_unit->getFaction() == FACTION_PLAYER)
	{
		_parent->getMap()->setUnitDying(true);
	}

	if (!_parent->getMap()->getCamera()->isOnScreen(_unit->getPosition()))	// kL
	{
		_parent->getMap()->getCamera()->centerOnPosition(_unit->getPosition());
	}

		// kL_note: this is only necessary when spawning a chryssalid from a zombie. See below
	_originalDir = _unit->getDirection();
	_unit->setDirection(_originalDir);		// kL. Stop Turning f!!!
	_unit->setSpinPhase(-1);

		// kL_note: replaced w/ Savegame/BattleUnit.cpp, BattleUnit::deathPirouette()
//kL		_unit->lookAt(3); // unit goes into status TURNING to prepare for a nice dead animation

	// kL_begin:
	if (_unit->getVisible())
//		&& _parent->getMap()->getCamera()->isOnScreen(_unit->getPosition()))
	{
		if (!_unit->getSpawnUnit().empty())	// nb. getSpawnUnit() is a member of both BattleUnit & Unit...
//			&& _unit->getSpecialAbility() == 0) // this comes into play because Soldiers & Civilians, (health=0) eg, can have SpawnUnit set and SpecAb set too.
													// but should they spin or not spin??? Did they spin because they're already dead...?
	/*
	_ZOMBIE_ -> nospin
	specab: 0
	spawnUnit: STR_CHRYSSALID_TERRORIST
	_ChryssalidTerrorist_ -> spin!!
	specab: 0
	spawnUnit: ""
	*/
			// and don't spin a unit that has just been smitten:
			// specab->RESPAWN, ->zombieUnit!
//			&& !_unit->getZombieUnit().empty()		// not working.
//			&& _unit->getSpecialAbility() == 3)		// not working.
		{
			//Log(LOG_INFO) << _unit->getId() << " is a zombie.";

			_parent->setStateInterval(BattlescapeState::DEFAULT_ANIM_SPEED * 8 / 7); // slow the zombie down so I can watch this....

//			_originalDir = _unit->getDirection(); // facing for zombie->Chryssalid spawns. See above
			// unit goes into status TURNING to prepare for a nice dead animation
			_unit->lookAt(3); // else -> STATUS_STANDING (...), look at the player for the transformation sequence.
			//Log(LOG_INFO) << ". . got back from lookAt() in ctor ...";
		}
		else //if (_unit->getVisible()
			//&& _parent->getMap()->getCamera()->isOnScreen(_unit->getPosition()))
		{
			//Log(LOG_INFO) << _unit->getId() << " is NOT a zombie. initiate Spin!";

			_parent->setStateInterval(BattlescapeState::DEFAULT_ANIM_SPEED * 2 / 7);

			_unit->initDeathSpin(); // -> STATUS_TURNING, death animation spin; Savegame/BattleUnit.cpp
		}
	}
	else // unit is not onScreen and/or not visible.
	{
//		_parent->setStateInterval(BattlescapeState::DEFAULT_ANIM_SPEED);

		if (_unit->getHealth() == 0)
		{
			_unit->instaKill();

			if (!_noSound)
			{
				playDeathSound();
			}
		}
		else
			_unit->knockOut();
	}
	// kL_end.
//	}

	_unit->clearVisibleTiles();
	_unit->clearVisibleUnits();

    if (_unit->getFaction() == FACTION_HOSTILE)
    {
        std::vector<Node* >* nodes = _parent->getSave()->getNodes();
        if (!nodes) return; // this better not happen.

        for (std::vector<Node* >::iterator n = nodes->begin(); n != nodes->end(); ++n)
        {
            if (_parent->getSave()->getTileEngine()->distanceSq((*n)->getPosition(), _unit->getPosition()) < 4)
            {
                (*n)->setType((*n)->getType() | Node::TYPE_DANGEROUS);
            }
        }
    }
}

/**
 * Deletes the UnitDieBState.
 */
UnitDieBState::~UnitDieBState()
{
	//Log(LOG_INFO) << "Delete UnitDieBState";
}

void UnitDieBState::init()
{
}

/**
 * Runs state functionality every cycle.
 * Progresses the death, displays any messages, checks if the mission is over, ...
 */
void UnitDieBState::think()
{
	//Log(LOG_INFO) << "UnitDieBState::think() - " << _unit->getId();

	if (_unit->getStatus() == STATUS_TURNING)
	{
		//Log(LOG_INFO) << ". . STATUS_TURNING";

		// kL_begin:
		if (_unit->getSpinPhase() > -1)
		{
			_parent->setStateInterval(BattlescapeState::DEFAULT_ANIM_SPEED * 2 / 7);

			_unit->contDeathSpin(); // -> STATUS_STANDING 
			//Log(LOG_INFO) << ". . . . got back from contDeathSpin()";
		}
		else
		{
		// kL_end.
			_unit->turn(); // -> STATUS_STANDING
			//Log(LOG_INFO) << ". . . . got back from turn()";
		}
	}
	else if (_unit->getStatus() == STATUS_COLLAPSING)
	{
		//Log(LOG_INFO) << ". . STATUS_COLLAPSING";

		_unit->keepFalling(); // -> STATUS_DEAD or STATUS_UNCONSCIOUS ( ie. isOut() )
	}
	else if (!_unit->isOut())
	{
		//Log(LOG_INFO) << ". . !isOut";

//		_parent->setStateInterval(BattlescapeState::DEFAULT_ANIM_SPEED * 8 / 7);	// kL
		_parent->setStateInterval(BattlescapeState::DEFAULT_ANIM_SPEED);			// kL
		_unit->startFalling(); // -> STATUS_COLLAPSING

		if (!_noSound)		// kL
		{
			playDeathSound();
		}
	}
//	else		// kL*****
	if (_unit->isOut())
	{
		//Log(LOG_INFO) << ". . unit isOut";

		// kL_note: I think this is doubling because I remarked DT_HE in the constructor. nah... Yes!
		// but leave it in 'cause it gives a coolia .. doubling effect !!! In fact,
		// try it for *EVERYONE** ( but it could be a cool effect that is reserved for HE deaths ) <- ok.
		if (!_noSound
			&& _damageType == DT_HE
			&& _unit->getStatus() != STATUS_UNCONSCIOUS)
		{
			playDeathSound();
		}

		if (_unit->getStatus() == STATUS_UNCONSCIOUS
			&& _unit->getSpecialAbility() == SPECAB_EXPLODEONDEATH)
		{
			_unit->instaKill();
		}

		_parent->getMap()->setUnitDying(false);

		if (_unit->getTurnsExposed())
		{
			_unit->setTurnsExposed(255);
		}

		if (!_unit->getSpawnUnit().empty()) // converts the dead zombie to a chryssalid
		{
			//Log(LOG_INFO) << ". . unit is _spawnUnit -> converting !";
			BattleUnit* newUnit = _parent->convertUnit(_unit, _unit->getSpawnUnit());
			newUnit->lookAt(_originalDir);
//			newUnit->lookAt(_originalDir, true);	// kL, fast turn back to original facing. Nope...
//			newUnit->setDirection(_originalDir);	// kL

			newUnit->setCache(0);					// kL
			_parent->getMap()->cacheUnit(newUnit);	// kL

			//Log(LOG_INFO) << ". . got back from lookAt() in think ...";
		}
		else
		{
			convertUnitToCorpse();
		}

		_parent->getTileEngine()->calculateUnitLighting();
		_parent->popState();

		if (_unit->getOriginalFaction() == FACTION_PLAYER
			&& _unit->getSpawnUnit().empty())
		{
			Game* game = _parent->getSave()->getBattleState()->getGame();

			if (_unit->getStatus() == STATUS_DEAD)
			{
				if (_damageType == DT_NONE)
				{
					game->pushState(new InfoboxOKState(game, game->getLanguage()
							->getString("STR_HAS_DIED_FROM_A_FATAL_WOUND", _unit->getGender()).arg(_unit->getName(game->getLanguage()))));
				}
				else if (Options::getBool("battleNotifyDeath"))
				{
					game->pushState(new InfoboxState(game, game->getLanguage()
							->getString("STR_HAS_BEEN_KILLED", _unit->getGender()).arg(_unit->getName(game->getLanguage()))));
				}
			}
			else
			{
				game->pushState(new InfoboxOKState(game, game->getLanguage()
						->getString("STR_HAS_BECOME_UNCONSCIOUS", _unit->getGender()).arg(_unit->getName(game->getLanguage()))));
			}
		}

		// if all units from either faction are killed - auto-end the mission.
		if (_parent->getSave()->getSide() == FACTION_PLAYER && Options::getBool("battleAutoEnd"))
		{
			int liveAliens = 0;
			int liveSoldiers = 0;
			_parent->tallyUnits(liveAliens, liveSoldiers, false);

			if (liveAliens == 0 || liveSoldiers == 0)
			{
				_parent->getSave()->setSelectedUnit(0);
				_parent->statePushBack(0);
			}
		}
	}

	_parent->getMap()->cacheUnit(_unit);
}

/**
 * Unit falling cannot be cancelled.
 */
void UnitDieBState::cancel()
{
}

/**
 * Converts unit to a corpse (item).
 */
void UnitDieBState::convertUnitToCorpse()
{
	_parent->getSave()->getBattleState()->showPsiButton(false);
	_parent->getSave()->removeUnconsciousBodyItem(_unit); // in case the unit was unconscious

	Position lastPosition = _unit->getPosition();

	int size = _unit->getArmor()->getSize() - 1;

	BattleItem* itemToKeep = 0;

	bool dropItems = !Options::getBool("weaponSelfDestruction")
			|| _unit->getOriginalFaction() != FACTION_HOSTILE
			|| _unit->getStatus() == STATUS_UNCONSCIOUS;

	// move inventory from unit to the ground for non-large units
	if (size == 0 && dropItems)
	{
		for (std::vector<BattleItem* >::iterator i = _unit->getInventory()->begin(); i != _unit->getInventory()->end(); ++i)
		{
			_parent->dropItem(_unit->getPosition(), (*i));

			if (!(*i)->getRules()->isFixed())
			{
				(*i)->setOwner(0);
			}
			else
			{
				itemToKeep = *i;
			}
		}
	}

	_unit->getInventory()->clear();

	if (itemToKeep != 0)
	{
		_unit->getInventory()->push_back(itemToKeep);
	}

	_unit->setTile(0); // remove unit-tile link

	if (size == 0)
	{
		BattleItem* corpse = new BattleItem(_parent->getRuleset()->getItem(_unit->getArmor()->getCorpseItem()),_parent->getSave()->getCurrentItemId());
		corpse->setUnit(_unit);
		_parent->dropItem(_unit->getPosition(), corpse, true);

		_parent->getSave()->getTile(lastPosition)->setUnit(0);
	}
	else
	{
		int i = 1;
		for (int y = 0; y <= size; y++)
		{
			for (int x = 0; x <= size; x++)
			{
				std::stringstream ss;
				ss << _unit->getArmor()->getCorpseItem() << i;
				BattleItem* corpse = new BattleItem(_parent->getRuleset()->getItem(ss.str()),_parent->getSave()->getCurrentItemId());
				corpse->setUnit(_unit);

				_parent->getSave()->getTile(lastPosition + Position(x, y, 0))->setUnit(0);
				_parent->dropItem(lastPosition + Position(x, y, 0), corpse, true);

				i++;
			}
		}
	}
}

/**
 * Plays the death sound.
 */
void UnitDieBState::playDeathSound()
{
	if ((_unit->getType() == "SOLDIER" && _unit->getGender() == GENDER_MALE)
		|| _unit->getType() == "MALE_CIVILIAN")
	{
		int iSound = RNG::generate(41, 43);
		Log(LOG_INFO) << "UnitDieBState::playDeathSound(), male iSound = " << iSound;
//kL		_parent->getResourcePack()->getSound("BATTLE.CAT", RNG::generate(41, 43))->play();
		_parent->getResourcePack()->getSound("BATTLE.CAT", iSound)->play();		// kL
	}
	else if ((_unit->getType() == "SOLDIER" && _unit->getGender() == GENDER_FEMALE)
		|| _unit->getType() == "FEMALE_CIVILIAN")
	{
		int iSound = RNG::generate(44, 46);
		Log(LOG_INFO) << "UnitDieBState::playDeathSound(), female iSound = " << iSound;
//kL		_parent->getResourcePack()->getSound("BATTLE.CAT", RNG::generate(44, 46))->play();
		_parent->getResourcePack()->getSound("BATTLE.CAT", iSound)->play();		// kL
	}
	else
	{
		_parent->getResourcePack()->getSound("BATTLE.CAT", _unit->getDeathSound())->play();
	}
}

}
