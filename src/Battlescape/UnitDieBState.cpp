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

#include "UnitDieBState.h"

#include "BattlescapeState.h"
#include "Camera.h"
#include "InfoboxOKState.h"
#include "InfoboxState.h"
#include "Map.h"
#include "TileEngine.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Logger.h"
//#include "../Engine/Options.h"
//#include "../Engine/RNG.h"
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
 * @param unit			- pointer to a dying BattleUnit
 * @param damageType	- type of damage that caused the death (RuleItem.h)
 * @param noSound		- true to disable the death sound for pre-battle powersource explosions as well as stun (default false)
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

	if (noSound == false) // pre-battle hidden explosion death; needed here to stop Camera CTD.
		_unit->setUnitVisible(); // TEST. has side-effect of keeping stunned victims non-revealed if not already visible.

	if (_unit->getUnitVisible() == true)
	{
		Camera* const deathCam = _parent->getMap()->getCamera();
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
		const std::vector<Node*>* const nodes = _parent->getSave()->getNodes();
		if (nodes == NULL)
			return; // this better not happen.

		for (std::vector<Node*>::const_iterator
				node = nodes->begin();
				node != nodes->end();
				++node)
		{
			if (_parent->getSave()->getTileEngine()->distanceSq(
															(*node)->getPosition(),
															_unit->getPosition(),
															false)
														< 5)
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
{}

/**
 * Initializes this state.
 */
void UnitDieBState::init()
{}

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
	if (_unit->isOut() == true) // and this ought be Status_Dead OR _Unconscious.
	{
		//Log(LOG_INFO) << ". . unit isOut";
//		_parent->getMap()->setUnitDying(false);

		if (_unit->getStatus() == STATUS_UNCONSCIOUS
			&& (_unit->getSpecialAbility() == SPECAB_EXPLODEONDEATH
				|| _unit->getSpecialAbility() == SPECAB_BURN_AND_EXPLODE))
		{
			_unit->instaKill();
		}

		_unit->setDown();

/*		if (_unit->getDiedByFire()) // do this in BattleUnit::damage()
		{
			_unit->setSpawnUnit("");
			_unit->setSpecialAbility(SPECAB_NONE); // SPECAB_EXPLODEONDEATH
		} */

		if (_unit->getSpawnUnit().empty() == false)
//			&& _unit->getDiedByFire() == false) // kL <- could screw with death animations, etc.
		{
			//Log(LOG_INFO) << ". . unit is _spawnUnit -> converting !";
			BattleUnit* const convertedUnit = _parent->convertUnit(
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
			&& _unit->getSpawnUnit().empty() == true)
		{
			Game* const game = _parent->getSave()->getBattleState()->getGame();

			if (_unit->getGeoscapeSoldier() != NULL)
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

				if (message.empty() == false)
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
			int
				liveAliens = 0,
				liveSoldiers = 0;
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
	_parent->getMap()->setUnitDying(false);
	//Log(LOG_INFO) << "UnitDieBState::think() EXIT";
}

/**
 * Unit dying cannot be cancelled.
 */
void UnitDieBState::cancel()
{}

/**
 * Converts unit to a corpse-item.
 */
void UnitDieBState::convertUnitToCorpse()
{
	//Log(LOG_INFO) << "UnitDieBState::convertUnitToCorpse() ID = " << _unit->getId() << " pos " << _unit->getPosition();
	_parent->getSave()->getBattleState()->showPsiButton(false);

	const Position pos = _unit->getPosition();

	// remove the unconscious body item corresponding to this unit,
	// and if it was being carried, keep track of what slot it was in
	const bool carried = (pos == Position(-1,-1,-1));
	if (carried == false)
		_parent->getSave()->removeUnconsciousBodyItem(_unit);

	BattleItem* itemToKeep = NULL;
	const int unitSize = _unit->getArmor()->getSize();

	// move inventory from unit to the ground for non-large units
	const bool drop = (unitSize == 1)
					&& carried == false // kL, i don't think this ever happens (ie, items have already been dropped). So let's bypass this section anyway!
					&& (Options::weaponSelfDestruction == false
						|| _unit->getOriginalFaction() != FACTION_HOSTILE
						|| _unit->getStatus() == STATUS_UNCONSCIOUS);
	if (drop == true)
	{
		for (std::vector<BattleItem*>::const_iterator
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


	if (carried == true) // unconscious unit is being carried when it dies
	{
		// replace the unconscious body item with a corpse in the carrying unit's inventory
		for (std::vector<BattleItem*>::const_iterator
				i = _parent->getSave()->getItems()->begin();
				i != _parent->getSave()->getItems()->end();
				)
		{
			if ((*i)->getUnit() == _unit) // unit is in an inventory, so unit must be a 1x1 unit
			{
				const RuleItem* const corpseRule = _parent->getRuleset()->getItem(_unit->getArmor()->getCorpseBattlescape()[0]);
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
				y < unitSize;
				++y)
		{
			for (int
					x = 0;
					x < unitSize;
					++x)
			{
				tile = _parent->getSave()->getTile(pos + Position(x, y, 0));

				// This block is lifted from TileEngine::explode(), switch(DT_IN).
				if (_unit->getUnitRules() != NULL
					&& _unit->getUnitRules()->isMechanical() == true
					&& RNG::percent(19) == true)
				{
					Tile
						* explTile = tile,
						* explBelow = _parent->getSave()->getTile(explTile->getPosition() + Position(0, 0,-1));

					while (explTile != NULL // safety.
						&& explTile->getPosition().z > 0
						&& explTile->getMapData(MapData::O_OBJECT) == NULL // only floors & content can catch fire.
						&& explTile->getMapData(MapData::O_FLOOR) == NULL
						&& explTile->hasNoFloor(explBelow) == true)
					{
						explTile = explBelow;
						explBelow = _parent->getSave()->getTile(explTile->getPosition() + Position(0, 0,-1));
					}

					if (explTile != NULL // safety.
						&& explTile->getFire() == 0)
					{
						explTile->setFire(explTile->getFuel() + RNG::generate(1, 3)); // Could use a ruleset-factor in here.
						explTile->setSmoke(std::max(
												1,
												std::min(
														6,
														17 - (explTile->getFlammability() / 5))));
					}
				}

				BattleItem* const corpse = new BattleItem(
													_parent->getRuleset()->getItem(_unit->getArmor()->getCorpseBattlescape()[i]),
													_parent->getSave()->getCurrentItemId());
				corpse->setUnit(_unit);

				// check in case unit was displaced by another unit
//				tile = _parent->getSave()->getTile(pos + Position(x, y, 0));
				if (tile != NULL // kL, safety. ( had a CTD when ethereal dies on water )
					&& tile->getUnit() == _unit)
				{
					tile->setUnit(NULL);
				}

				_parent->dropItem(
								pos + Position(x, y, 0),
								corpse,
								true);

				++i;
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
	int sound = -1;

	if (_unit->getType() == "SOLDIER")
	{
		if (_unit->getGender() == GENDER_MALE)
			sound = RNG::generate(111, 116);
		else
			sound = RNG::generate(101, 103);
	}
	else if (_unit->getUnitRules()->getRace() == "STR_CIVILIAN")
	{
		if (_unit->getGender() == GENDER_MALE)
			sound = ResourcePack::MALE_SCREAM[RNG::generate(0, 2)];
		else
			sound = ResourcePack::FEMALE_SCREAM[RNG::generate(0, 2)];
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
