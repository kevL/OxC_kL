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

#include "../Ruleset/RuleArmor.h"
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
		_doneScream(false),
		_extraTicks(0)
{
	//Log(LOG_INFO) << "Create UnitDieBState ID = " << _unit->getId();
	_unit->clearVisibleTiles();
	_unit->clearVisibleUnits();

	if (_noSound == false)			// pre-battle hidden explosion death; needed here to stop Camera CTD.
		_unit->setUnitVisible();	// TEST. has side-effect of keeping stunned victims non-revealed if not already visible.

	if (_unit->getUnitVisible() == true)
	{
		Camera* const deathCam = _parent->getMap()->getCamera(); // <- added to think() also.
		if (deathCam->isOnScreen(_unit->getPosition()) == false)
			deathCam->centerOnPosition(_unit->getPosition());

		if (_unit->getFaction() == FACTION_PLAYER)
			_parent->getMap()->setUnitDying();

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
															false) < 5)
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
 * Progresses the death, displays any messages, checks if the mission is over, etc.
 */
void UnitDieBState::think()
{
	//Log(LOG_INFO) << "UnitDieBState::think() ID " << _unit->getId();
	if (_noSound == false
		&& _doneScream == false)
	{
		_doneScream = true;
		playDeathSound();

		if (_unit->getUnitVisible() == true)
		{
			// This was also done in cTor, but terrain Explosions can/do change camera before the turn/spin & collapse happen.
			Camera* const deathCam = _parent->getMap()->getCamera();
			if (deathCam->isOnScreen(_unit->getPosition()) == false
				|| _unit->getPosition().z != deathCam->getViewLevel())
			{
				deathCam->centerOnPosition(_unit->getPosition());
			}
		}
	}

// #1
	if (_unit->getStatus() == STATUS_TURNING)
	{
		//Log(LOG_INFO) << ". . STATUS_TURNING";
		if (_unit->getSpinPhase() != -1)
		{
			_parent->setStateInterval(BattlescapeState::DEFAULT_ANIM_SPEED * 2 / 7);
			_unit->contDeathSpin(); // -> STATUS_STANDING
		}
		else // spawn conversion is going to happen
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

		if (_unit->getSpawnUnit().empty() == false)
		{
			while (_unit->getStatus() == STATUS_COLLAPSING)
				_unit->keepFalling(); // -> STATUS_DEAD or STATUS_UNCONSCIOUS ( ie. isOut() ) -> goto #4
		}
	}

// #6
	if (_extraTicks == 2)
	{
		_parent->getMap()->setUnitDying(false);

		_parent->getTileEngine()->calculateUnitLighting();
		_parent->popState();

		// need to freshen visUnit-indicators in case other units were hiding behind the one who just fell
		_parent->getSave()->getBattleState()->updateSoldierInfo(false);

		if (_unit->getGeoscapeSoldier() != NULL
			&& _unit->getOriginalFaction() == FACTION_PLAYER)
		{
			std::string message;
			if (_unit->getStatus() == STATUS_DEAD)
			{
				if (_damageType == DT_NONE
					&& _unit->getSpawnUnit().empty() == true)
				{
					message = "STR_HAS_DIED_FROM_A_FATAL_WOUND";
				}
				else if (Options::battleNotifyDeath == true)
					message = "STR_HAS_BEEN_KILLED";
			}
			else
				message = "STR_HAS_BECOME_UNCONSCIOUS";

			if (message.empty() == false)
			{
				Game* const game = _parent->getSave()->getBattleState()->getGame();
				const Language* const lang = game->getLanguage();
				game->pushState(new InfoboxOKState(lang->getString(
																message,
																_unit->getGender())
															.arg(_unit->getName(lang))));
			}
		}

		// if all units from either faction are killed - auto-end the mission.
		if (_parent->getSave()->getSide() == FACTION_PLAYER
			&& Options::battleAutoEnd == true)
		{
			int
				liveAliens,
				liveSoldiers;
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
// #5
	else if (_extraTicks == 1)
		++_extraTicks;
// #4
	else if (_unit->isOut() == true) // and this ought be Status_Dead OR _Unconscious.
	{
		//Log(LOG_INFO) << ". . unit isOut";
		++_extraTicks;

		if (_unit->getStatus() == STATUS_UNCONSCIOUS
			&& (_unit->getSpecialAbility() == SPECAB_EXPLODEONDEATH
				|| _unit->getSpecialAbility() == SPECAB_BURN_AND_EXPLODE))
		{
			_unit->instaKill();
		}

		_unit->setDown();

		if (_unit->getSpawnUnit().empty() == true)
			convertToCorpse();
		else
		{
			//Log(LOG_INFO) << ". . unit is _spawnUnit -> converting !";
			_parent->convertUnit(
							_unit,
							_unit->getSpawnUnit());
		}
	}

	_parent->getMap()->cacheUnit(_unit);
	//Log(LOG_INFO) << "UnitDieBState::think() EXIT";
}

/**
 * Unit dying cannot be cancelled.
 */
void UnitDieBState::cancel()
{}

/**
 * Converts unit to a corpse-item.
 * Note that this is used also for units going unconscious.
 */
void UnitDieBState::convertToCorpse() // private.
{
	_parent->getSave()->getBattleState()->showPsiButton(false);

	const Position pos = _unit->getPosition();

	// remove the unconscious body item corresponding to this unit,
	// and if it was being carried, keep track of what slot it was in
	const bool carried = (pos == Position(-1,-1,-1));
	if (carried == false)
		_parent->getSave()->removeCorpse(_unit);

	const int unitSize = _unit->getArmor()->getSize() - 1;

	// move inventory from unit to the ground for non-large units
	const bool drop = unitSize == 0
				   && carried == false
				   && (Options::weaponSelfDestruction == false
						|| _unit->getOriginalFaction() != FACTION_HOSTILE
						|| _unit->getStatus() == STATUS_UNCONSCIOUS);
	if (drop == true)
	{
		std::vector<BattleItem*> itemsToKeep;

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
				itemsToKeep.push_back(*i);
		}

		_unit->getInventory()->clear();

		for (std::vector<BattleItem*>::const_iterator
				i = itemsToKeep.begin();
				i != itemsToKeep.end();
				++i)
		{
			_unit->getInventory()->push_back(*i);
		}
	}

	_unit->setTile(NULL); // remove unit-tile link


	if (carried == true) // unconscious unit is being carried when it dies
	{
		// replace the unconscious body item with a corpse in the carrying unit's inventory
		for (std::vector<BattleItem*>::const_iterator
				i = _parent->getSave()->getItems()->begin();
				i != _parent->getSave()->getItems()->end();
				++i)
		{
			if ((*i)->getUnit() == _unit) // unit is in an inventory, so unit must be a 1x1 unit
			{
				(*i)->convertToCorpse(_parent->getRuleset()->getItem(_unit->getArmor()->getCorpseBattlescape()[0]));
				break;
			}
		}
	}
	else
	{
		Tile
			* tile,
			* explTile,
			* explTile_b;
		bool soundPlayed = false;
		size_t part = static_cast<size_t>((unitSize + 1) * (unitSize + 1));

		for (int // count downward to original position so that dropItem() correctly positions large units @ their NW quadrant.
				y = unitSize;
				y != -1;
				--y)
		{
			for (int
					x = unitSize;
					x != -1;
					--x)
			{
				tile = _parent->getSave()->getTile(pos + Position(x,y,0));

				// This block is lifted from TileEngine::explode(), switch(DT_IN).
				if (_unit->getUnitRules() != NULL
					&& _unit->getUnitRules()->isMechanical() == true
					&& RNG::percent(19) == true)
				{
					explTile = tile;
					explTile_b = _parent->getSave()->getTile(explTile->getPosition() + Position(0,0,-1));

					while (explTile != NULL // safety.
						&& explTile->getPosition().z > 0
						&& explTile->getMapData(MapData::O_OBJECT) == NULL // only floors & content can catch fire.
						&& explTile->getMapData(MapData::O_FLOOR) == NULL
						&& explTile->hasNoFloor(explTile_b) == true)
					{
						explTile = explTile_b;
						explTile_b = _parent->getSave()->getTile(explTile->getPosition() + Position(0,0,-1));
					}

					if (explTile != NULL // safety.
						&& explTile->getFire() == 0)
					{
						explTile->setFire(explTile->getFuel() + RNG::generate(1,3)); // Could use a ruleset-factor in here.
						explTile->setSmoke(std::max(
												1,
												std::min(
														6,
														17 - (explTile->getFlammability() / 5))));

						if (soundPlayed == false)
						{
							soundPlayed = true;
							_parent->getResourcePack()->getSoundByDepth(
																	_parent->getDepth(),
																	ResourcePack::SMALL_EXPLOSION)
																->play(
																	-1,
																	_parent->getMap()->getSoundAngle(_unit->getPosition()));
						}
					}
				}

				BattleItem* const corpse = new BattleItem(
													_parent->getRuleset()->getItem(_unit->getArmor()->getCorpseBattlescape()[--part]),
													_parent->getSave()->getNextItemId());
				corpse->setUnit(_unit);

				if (tile != NULL					// kL, safety. ( had a CTD when ethereal dies on water ).
					&& tile->getUnit() == _unit)	// <- check in case unit was displaced by another unit
				{
					tile->setUnit(NULL);
				}

				_parent->dropItem(
								pos + Position(x,y,0),
								corpse,
								true);
			}
		}

		// expose any units that were hiding behind dead unit
		_parent->getTileEngine()->calculateFOV(pos);
	}
}

/**
 * Plays the death sound.
 * kL rewrite to include NwN2 hits & screams.
 */
void UnitDieBState::playDeathSound() // private.
{
	int sound = -1;

	if (_unit->getType() == "SOLDIER")
	{
		if (_unit->getGender() == GENDER_MALE)
			sound = RNG::generate(111,116);
		else
			sound = RNG::generate(101,103);
	}
	else if (_unit->getUnitRules()->getRace() == "STR_CIVILIAN")
	{
		if (_unit->getGender() == GENDER_MALE)
			sound = ResourcePack::MALE_SCREAM[RNG::generate(0,2)];
		else
			sound = ResourcePack::FEMALE_SCREAM[RNG::generate(0,2)];
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
