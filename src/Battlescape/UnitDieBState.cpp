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

#include "BattlescapeState.h"
#include "Camera.h"
#include "ExplosionBState.h"
#include "InfoboxOKState.h"
#include "InfoboxState.h"
#include "Map.h"
#include "TileEngine.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/RNG.h"
#include "../Engine/Sound.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/Armor.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/Unit.h"

#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/Node.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

/**
 * Sets up an UnitDieBState.
 * @param parent Pointer to the Battlescape.
 * @param unit Dying unit.
 * @param damageType Type of damage that caused the death.
 * @param noSound Whether to disable the death sound.
 */
UnitDieBState::UnitDieBState(
		BattlescapeGame* parent,
		BattleUnit* unit,
		ItemDamageType damageType,
		bool noSound)
	:
		BattleState(parent),
		_unit(unit),
		_damageType(damageType),
		_noSound(noSound)
{
	Log(LOG_INFO) << "Create UnitDieBState";

	_unit->clearVisibleTiles();
	_unit->clearVisibleUnits();

	_originalDir = _unit->getDirection();
	_unit->setDirection(_originalDir);
	_unit->setSpinPhase(-1);

	if (_unit->getFaction() == FACTION_HOSTILE)
	{
		//Log(LOG_INFO) << ". . unit is Faction_Hostile";
		std::vector<Node*>* nodes = _parent->getSave()->getNodes();
		if (!nodes) return; // this better not happen.

		for (std::vector<Node*>::iterator
				node = nodes->begin();
				node != nodes->end();
				++node)
		{
			if (_parent->getSave()->getTileEngine()->distanceSq(
															(*node)->getPosition(),
															_unit->getPosition())
														< 4)
			{
				(*node)->setType((*node)->getType() | Node::TYPE_DANGEROUS);
			}
		}
	}
	Log(LOG_INFO) << "Create UnitDieBState EXIT cTor";
}

/**
 * Deletes the UnitDieBState.
 */
UnitDieBState::~UnitDieBState()
{
	Log(LOG_INFO) << "Delete UnitDieBState";
}

/**
 *
 */
void UnitDieBState::init()
{
	Log(LOG_INFO) << "UnitDieBState::init()";
	if (!_noSound)
		playDeathSound();

	if (_unit->getVisible())
	{
		Camera* deathCam = _parent->getMap()->getCamera();
		if (!deathCam->isOnScreen(_unit->getPosition()))
			deathCam->centerOnPosition(_unit->getPosition());

		if (_unit->getFaction() == FACTION_PLAYER)
			_parent->getMap()->setUnitDying(true);

		if (!_unit->getSpawnUnit().empty())
		{
			_parent->setStateInterval(BattlescapeState::DEFAULT_ANIM_SPEED * 8 / 7);
			_unit->lookAt(3);
		}
		else
		{
			_parent->setStateInterval(BattlescapeState::DEFAULT_ANIM_SPEED * 2 / 7);
			_unit->initDeathSpin();
		}
	}
	else
	{
		if (_unit->getHealth() == 0)
			_unit->instaKill();
		else
			_unit->knockOut();
	}
}

/**
 * Runs state functionality every cycle.
 * Progresses the death, displays any messages, checks if the mission is over, ...
 */
void UnitDieBState::think()
{
	Log(LOG_INFO) << "UnitDieBState::think() ID " << _unit->getId();
// #1
	if (_unit->getStatus() == STATUS_TURNING)
	{
		//Log(LOG_INFO) << ". . STATUS_TURNING";
		if (_unit->getSpinPhase() > -1)
			_unit->contDeathSpin(); // -> STATUS_STANDING 
		else
			_unit->turn(); // -> STATUS_STANDING
	}
// #3
	else if (_unit->getStatus() == STATUS_COLLAPSING)
	{
		//Log(LOG_INFO) << ". . STATUS_COLLAPSING";
		_unit->keepFalling(); // -> STATUS_DEAD or STATUS_UNCONSCIOUS ( ie. isOut() )
	}
// #2
	else if (!_unit->isOut()) // this ought be Status_Standing also.
	{
		//Log(LOG_INFO) << ". . !isOut";
		_parent->setStateInterval(BattlescapeState::DEFAULT_ANIM_SPEED);
		_unit->startFalling(); // -> STATUS_COLLAPSING
	}

// #4
	if (_unit->isOut()) // and this ought be Status_Dead.
	{
		//Log(LOG_INFO) << ". . unit isOut";
		_parent->getMap()->setUnitDying(false);

		if (_unit->getStatus() == STATUS_UNCONSCIOUS
			&& _unit->getSpecialAbility() == SPECAB_EXPLODEONDEATH)
		{
			_unit->instaKill();
		}

		if (_unit->getTurnsExposed() < 255)
			_unit->setTurnsExposed(255);

		if (!_unit->getSpawnUnit().empty()) // converts the dead zombie to a chryssalid
		{
			//Log(LOG_INFO) << ". . unit is _spawnUnit -> converting !";
			BattleUnit* newUnit = _parent->convertUnit(
													_unit,
													_unit->getSpawnUnit());
			newUnit->lookAt(_originalDir); // kL_note: This seems to need a state to initiate turn() ...

			newUnit->setCache(0);					// kL
			_parent->getMap()->cacheUnit(newUnit);	// kL

			//Log(LOG_INFO) << ". . got back from lookAt() in think ...";
		}
		else
			convertUnitToCorpse();


		_parent->getTileEngine()->calculateUnitLighting();
		_parent->popState();

		if (_unit->getOriginalFaction() == FACTION_PLAYER
			&& _unit->getSpawnUnit().empty())
		{
			Game* game = _parent->getSave()->getBattleState()->getGame();

			if (_unit->getStatus() == STATUS_DEAD)
			{
				if (_unit->getArmor()->getSize() == 1)
				{
					if (_damageType == DT_NONE)
						game->pushState(new InfoboxOKState(
														game,
														game->getLanguage()->getString(
																					"STR_HAS_DIED_FROM_A_FATAL_WOUND",
																					_unit->getGender())
																				.arg(_unit->getName(game->getLanguage()))));
					else if (Options::getBool("battleNotifyDeath"))
						game->pushState(new InfoboxOKState(
														game,
														game->getLanguage()->getString(
																					"STR_HAS_BEEN_KILLED",
																					_unit->getGender())
																				.arg(_unit->getName(game->getLanguage()))));
				}
			}
			else
				game->pushState(new InfoboxOKState(
												game,
												game->getLanguage()->getString(
																			"STR_HAS_BECOME_UNCONSCIOUS",
																			_unit->getGender())
																		.arg(_unit->getName(game->getLanguage()))));
		}

		// if all units from either faction are killed - auto-end the mission.
		if (_parent->getSave()->getSide() == FACTION_PLAYER
			&& Options::getBool("battleAutoEnd"))
		{
			int liveAliens = 0;
			int liveSoldiers = 0;
			_parent->tallyUnits(
							liveAliens,
							liveSoldiers,
							false);

			if (liveAliens == 0
				|| liveSoldiers == 0)
			{
				_parent->getSave()->setSelectedUnit(0);
				_parent->requestEndTurn();
			}
		}
	}

	_parent->getMap()->cacheUnit(_unit);
	Log(LOG_INFO) << "UnitDieBState::think() EXIT";
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

	BattleItem* itemToKeep = 0;
	int size = _unit->getArmor()->getSize();

	bool dropItems = !Options::getBool("weaponSelfDestruction")
						|| _unit->getOriginalFaction() != FACTION_HOSTILE
						|| _unit->getStatus() == STATUS_UNCONSCIOUS;

	// move inventory from unit to the ground for non-large units
	if (size == 1
		&& dropItems)
	{
		for (std::vector<BattleItem*>::iterator
				i = _unit->getInventory()->begin();
				i != _unit->getInventory()->end();
				++i)
		{
			_parent->dropItem(
							_unit->getPosition(),
							*i);

			if (!(*i)->getRules()->isFixed())
				(*i)->setOwner(0);
			else
				itemToKeep = *i;
		}
	}

	_unit->getInventory()->clear();

	if (itemToKeep != 0)
		_unit->getInventory()->push_back(itemToKeep);

	_unit->setTile(0); // remove unit-tile link


	Position lastPos = _unit->getPosition();
	Tile* tSelf = 0;
	int i = 0;

	for (int
			y = 0;
			y < size;
			y++)
	{
		for (int
				x = 0;
				x < size;
				x++)
		{
			BattleItem* corpse = new BattleItem(
											_parent->getRuleset()->getItem(_unit->getArmor()->getCorpseBattlescape()[i]),
											_parent->getSave()->getCurrentItemId());
			corpse->setUnit(_unit);

			// check in case unit was displaced by another unit
			tSelf = _parent->getSave()->getTile(lastPos + Position(x, y, 0));
			if (tSelf->getUnit() == _unit)
				tSelf->setUnit(0);

			_parent->dropItem(
							lastPos + Position(x, y, 0),
							corpse,
							true);

			i++;
		}
	}
}

/**
 * Plays the death sound.
 */
void UnitDieBState::playDeathSound()
{
	if ((_unit->getType() == "SOLDIER"
			&& _unit->getGender() == GENDER_MALE)
		|| _unit->getType() == "MALE_CIVILIAN")
	{
//kL		_parent->getResourcePack()->getSound("BATTLE.CAT", RNG::generate(41, 43))->play();
		int iSound = RNG::generate(41, 43);
		//Log(LOG_INFO) << "UnitDieBState::playDeathSound(), male iSound = " << iSound;
		_parent->getResourcePack()->getSound(
											"BATTLE.CAT",
											iSound)
										->play();
	}
	else if ((_unit->getType() == "SOLDIER"
			&& _unit->getGender() == GENDER_FEMALE)
		|| _unit->getType() == "FEMALE_CIVILIAN")
	{
//kL		_parent->getResourcePack()->getSound("BATTLE.CAT", RNG::generate(44, 46))->play();
		int iSound = RNG::generate(44, 46);
		//Log(LOG_INFO) << "UnitDieBState::playDeathSound(), female iSound = " << iSound;
		_parent->getResourcePack()->getSound(
											"BATTLE.CAT",
											iSound)
										->play();
	}
	else
		_parent->getResourcePack()->getSound(
											"BATTLE.CAT",
											_unit->getDeathSound())
										->play();
}

}
