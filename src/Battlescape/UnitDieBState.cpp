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

#include "UnitDieBState.h"

#include "BattlescapeState.h"
#include "Camera.h"
#include "InfoboxOKState.h"
#include "InfoboxState.h"
#include "Map.h"
#include "TileEngine.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Logger.h"
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
 * Creates a UnitDieBState.
 * @param parent		- pointer to BattlescapeGame
 * @param unit			- pointer to a dying unit
 * @param damageType	- type of damage that caused the death (RuleItem.h)
 * @param noSound		- true to disable the death sound (for pre-battle powersource explosions)
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
		_noSound(noSound),
		_doneScream(false)
{
	//Log(LOG_INFO) << "Create UnitDieBState ID = " << _unit->getId();
	_unit->clearVisibleTiles();
	_unit->clearVisibleUnits();

	if (_unit->getVisible())
	{
		Camera* deathCam = _parent->getMap()->getCamera();
		if (deathCam->isOnScreen(_unit->getPosition()) == false)
			deathCam->centerOnPosition(_unit->getPosition());

		if (_unit->getFaction() == FACTION_PLAYER)
			_parent->getMap()->setUnitDying(true);

		if (_unit->getSpawnUnit().empty() == false)
		{
			_parent->setStateInterval(BattlescapeState::DEFAULT_ANIM_SPEED * 8 / 7);
			_unit->lookAt(3); // inits STATUS_TURNING if not facing correctly. Else STATUS_STANDING
		}
		else
			_unit->initDeathSpin(); // inits STATUS_TURNING
	}
	else
	{
		if (_unit->getHealth() == 0)
			_unit->instaKill(); // STATUS_DEAD
		else
			_unit->knockOut(); // STATUS_UNCONSCIOUS if has SPECAB::spawnUnit. Else sets health0 / stun=health
	}

	if (_unit->getFaction() == FACTION_HOSTILE)
	{
		//Log(LOG_INFO) << ". . unit is Faction_Hostile";
		std::vector<Node*>* nodes = _parent->getSave()->getNodes();
		if (nodes == NULL)
			return; // this better not happen.

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
	//Log(LOG_INFO) << "Create UnitDieBState EXIT cTor";
}

/**
 * Deletes the UnitDieBState.
 */
UnitDieBState::~UnitDieBState()
{
	//Log(LOG_INFO) << "Delete UnitDieBState";
}

/**
 *
 */
void UnitDieBState::init()
{
	//Log(LOG_INFO) << "UnitDieBState::init()";
	//Log(LOG_INFO) << "UnitDieBState::init() EXIT";
}

/**
 * Runs state functionality every cycle.
 * Progresses the death, displays any messages, checks if the mission is over, ...
 */
void UnitDieBState::think()
{
	//Log(LOG_INFO) << "UnitDieBState::think() ID " << _unit->getId();
	if (_noSound == false
		&& _doneScream == false)
	{
		_doneScream = true;

		playDeathSound();
	}

// #1
	if (_unit->getStatus() == STATUS_TURNING)
	{
		//Log(LOG_INFO) << ". . STATUS_TURNING";
		if (_unit->getSpinPhase() > -1)
		{
			_parent->setStateInterval(BattlescapeState::DEFAULT_ANIM_SPEED * 2 / 7);
			_unit->contDeathSpin(); // -> STATUS_STANDING
		}
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
	else if (_unit->isOut() == false) // this ought be Status_Standing/Disabled also.
	{
		//Log(LOG_INFO) << ". . !isOut";
		_parent->setStateInterval(BattlescapeState::DEFAULT_ANIM_SPEED);
		_unit->startFalling(); // -> STATUS_COLLAPSING
	}

// #4
	if (_unit->isOut()) // and this ought be Status_Dead OR _Unconscious.
	{
		//Log(LOG_INFO) << ". . unit isOut";
		_parent->getMap()->setUnitDying(false);

		if (_unit->getStatus() == STATUS_UNCONSCIOUS
			&& _unit->getSpecialAbility() == SPECAB_EXPLODEONDEATH)
		{
			_unit->instaKill();
		}

		_unit->setTurnsExposed(255);

/*		if (_unit->getDiedByFire()) // do this in BattleUnit::damage()
		{
			_unit->setSpawnUnit("");
			_unit->setSpecialAbility(SPECAB_NONE); // SPECAB_EXPLODEONDEATH
		} */

		if (_unit->getSpawnUnit().empty() == false)
//			&& _unit->getDiedByFire() == false) // kL <- could screw with death animations, etc.
		{
			//Log(LOG_INFO) << ". . unit is _spawnUnit -> converting !";
			BattleUnit* convertedUnit = _parent->convertUnit(
															_unit,
															_unit->getSpawnUnit());

//			convertedUnit->lookAt(_originalDir); // kL_note: This seems to need a state to initiate turn() ...
//TEST		_battleSave->getBattleGame()->statePushBack(new UnitTurnBState(_battleSave->getBattleGame(), action));
//TEST		statePushFront(new UnitTurnBState(this, _currentAction)); // first of all turn towards the target

			convertedUnit->setCache(NULL);
			_parent->getMap()->cacheUnit(convertedUnit);

			//Log(LOG_INFO) << ". . got back from lookAt() in think ...";
		}
		else
			convertUnitToCorpse();


		_parent->getTileEngine()->calculateUnitLighting();
		_parent->popState();

		// need to freshen visUnits in case another unit was hiding behind the one who just fell ...
		// that is, it shows up on the Battlescape, but won't trigger the visUnits indicators
		// until/ unless the viewer calls BattlescapeState::updateSoldierInfo()
//		_parent->getSave()->getTileEngine()->calculateFOV(_unit->getPosition());
		// that is actually done already at the end of TileEngine::hit() & explode()
		// so, might have to updateSoldierInfo() here, there, or perhaps in BattlescapeGame ......
		_parent->getSave()->getBattleState()->updateSoldierInfo(false); // kL


		if (_unit->getOriginalFaction() == FACTION_PLAYER
			&& _unit->getSpawnUnit().empty())
		{
			Game* game = _parent->getSave()->getBattleState()->getGame();
/*			if (_unit->getStatus() == STATUS_DEAD)
			{
				if (_unit->getArmor()->getSize() == 1)
				{
					if (_damageType == DT_NONE)
						game->pushState(new InfoboxOKState(game->getLanguage()->getString(
																					"STR_HAS_DIED_FROM_A_FATAL_WOUND",
																					_unit->getGender())
																				.arg(_unit->getName(game->getLanguage()))));
					else if (Options::battleNotifyDeath)
						game->pushState(new InfoboxOKState(game->getLanguage()->getString(
																					"STR_HAS_BEEN_KILLED",
																					_unit->getGender())
																				.arg(_unit->getName(game->getLanguage()))));
				}
			}
			else
				game->pushState(new InfoboxOKState(game->getLanguage()->getString(
																			"STR_HAS_BECOME_UNCONSCIOUS",
																			_unit->getGender())
																		.arg(_unit->getName(game->getLanguage())))); */
//			if (_unit->getArmor()->getSize() == 1)
			if (_unit->getType() == "SOLDIER")
			{
				std::string message;

				if (_unit->getStatus() == STATUS_DEAD)
				{
					if (_damageType == DT_NONE)
						message = "STR_HAS_DIED_FROM_A_FATAL_WOUND";
					else if (Options::battleNotifyDeath)
						message = "STR_HAS_BEEN_KILLED";
				}
				else
					message = "STR_HAS_BECOME_UNCONSCIOUS";

				if (message != "")
					game->pushState(new InfoboxOKState(game->getLanguage()->getString(
																					message,
																					_unit->getGender())
																				.arg(_unit->getName(game->getLanguage()))));
			}
		}

		// if all units from either faction are killed - auto-end the mission.
		if (_parent->getSave()->getSide() == FACTION_PLAYER
			&& Options::battleAutoEnd)
		{
			int liveAliens = 0;
			int liveSoldiers = 0;
			_parent->tallyUnits(
							liveAliens,
							liveSoldiers);

			if (liveAliens == 0
				|| liveSoldiers == 0)
			{
				_parent->getSave()->setSelectedUnit(NULL);
				_parent->cancelCurrentAction(true);

				_parent->requestEndTurn();
			}
		}
	}

	_parent->getMap()->cacheUnit(_unit);
	//Log(LOG_INFO) << "UnitDieBState::think() EXIT";
}

/**
 * Unit dying cannot be cancelled.
 */
void UnitDieBState::cancel()
{
}

/**
 * Converts unit to a corpse-item.
 */
void UnitDieBState::convertUnitToCorpse()
{
	//Log(LOG_INFO) << "UnitDieBState::convertUnitToCorpse() ID = " << _unit->getId() << " pos " << _unit->getPosition();
	_parent->getSave()->getBattleState()->showPsiButton(false);

	Position pos = _unit->getPosition();

	// remove the unconscious body item corresponding to this unit,
	// and if it was being carried, keep track of what slot it was in
	bool carried = (pos == Position(-1,-1,-1));
	if (carried == false)
		_parent->getSave()->removeUnconsciousBodyItem(_unit);

	BattleItem* itemToKeep = NULL;
	int size = _unit->getArmor()->getSize();

	// move inventory from unit to the ground for non-large units
	bool drop = size == 1
				&& carried == false // kL, i don't think this ever happens (ie, items have already been dropped). So let's bypass this section anyway!
				&& (Options::weaponSelfDestruction == false
					|| _unit->getOriginalFaction() != FACTION_HOSTILE
					|| _unit->getStatus() == STATUS_UNCONSCIOUS);
	if (drop)
	{
		for (std::vector<BattleItem*>::iterator
				i = _unit->getInventory()->begin();
				i != _unit->getInventory()->end();
				++i)
		{
			_parent->dropItem(
							pos,
							*i);

			if ((*i)->getRules()->isFixed() == false)
				(*i)->setOwner(NULL);
			else
				itemToKeep = *i;
		}
	}

	_unit->getInventory()->clear();

	if (itemToKeep != NULL)
		_unit->getInventory()->push_back(itemToKeep);

	_unit->setTile(NULL); // remove unit-tile link


	if (carried) // unconscious unit is being carried when it dies
	{
		// replace the unconscious body item with a corpse in the carrying unit's inventory
		for (std::vector<BattleItem*>::iterator
				i = _parent->getSave()->getItems()->begin();
				i != _parent->getSave()->getItems()->end();
				)
		{
			if ((*i)->getUnit() == _unit) // unit is in an inventory, so unit must be a 1x1 unit
			{
				RuleItem* corpseRule = _parent->getRuleset()->getItem(_unit->getArmor()->getCorpseBattlescape()[0]);
				(*i)->convertToCorpse(corpseRule);

				break;
			}

			++i;
		}
	}
	else
	{
		int i = 0;
		Tile* tile = NULL;

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
				tile = _parent->getSave()->getTile(pos + Position(x, y, 0));
				if (tile // kL, safety. ( had a CTD when ethereal dies on water )
					&& tile->getUnit() == _unit)
				{
					tile->setUnit(NULL);
				}

				_parent->dropItem(
								pos + Position(x, y, 0),
								corpse,
								true);

				i++;
			}
		}
	}
	//Log(LOG_INFO) << "UnitDieBState::convertUnitToCorpse() EXIT";
}

/**
 * Plays the death sound.
 * kL rewrite to include NwN2 hits & screams.
 */
void UnitDieBState::playDeathSound()
{
	//Log(LOG_INFO) << "UnitDieBState::playDeathSound()";
	int sound = -1;

	if (_unit->getType() == "MALE_CIVILIAN")
		sound = ResourcePack::MALE_SCREAM[RNG::generate(0, 2)];
	else if (_unit->getType() == "FEMALE_CIVILIAN")
		sound = ResourcePack::FEMALE_SCREAM[RNG::generate(0, 2)];
	else if (_unit->getType() == "SOLDIER")
	{
		if (_unit->getGender() == GENDER_MALE)
		{
			sound = RNG::generate(111, 116);
			//Log(LOG_INFO) << "death Male, sound = " << sound;
		}
		else if (_unit->getGender() == GENDER_FEMALE)
		{
			sound = RNG::generate(101, 103);
			//Log(LOG_INFO) << "death Female, sound = " << sound;
		}
	}
	else
		sound = _unit->getDeathSound();


	if (sound != -1)
		_parent->getResourcePack()->getSoundByDepth(
												_parent->getDepth(),
												sound)
											->play(
												-1,
												_parent->getMap()->getSoundAngle(_unit->getPosition()));
}

}
