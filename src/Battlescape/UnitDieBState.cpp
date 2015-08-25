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
#include "Map.h"
#include "TileEngine.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Options.h"
#include "../Engine/Sound.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/Node.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

/**
 * Creates a UnitDieBState.
 * @param parent	- pointer to BattlescapeGame
 * @param unit		- pointer to a dying BattleUnit
 * @param dType		- type of damage that caused the death (RuleItem.h)
 * @param noSound	- true to disable the death sound for pre-battle powersource explosions as well as stun (default false)
 */
UnitDieBState::UnitDieBState(
		BattlescapeGame* const parent,
		BattleUnit* const unit,
		const ItemDamageType dType,
		const bool noSound)
	:
		BattleState(parent),
		_unit(unit),
		_dType(dType),
		_noSound(noSound),
		_battleSave(parent->getBattleSave()),
		_doneScream(false),
		_extraTicks(0)
{
//	_unit->clearVisibleTiles();
	_unit->clearHostileUnits();

	if (_noSound == false)			// pre-battle hidden explosion death; needed here to stop Camera CTD.
		_unit->setUnitVisible();	// Has side-effect of keeping stunned victims non-revealed if not already visible.

	if (_unit->getUnitVisible() == true)
	{
		Camera* const deathCam = _parent->getMap()->getCamera(); // <- added to think() also.
		if (deathCam->isOnScreen(_unit->getPosition()) == false)
			deathCam->centerOnPosition(_unit->getPosition());

		if (_unit->getFaction() == FACTION_PLAYER)
			_parent->getMap()->setUnitDying();

		if (_unit->getSpawnUnit().empty() == false)
		{
			_parent->setStateInterval(BattlescapeState::STATE_INTERVAL_STANDARD * 8 / 7);
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
		const std::vector<Node*>* const nodeList = _battleSave->getNodes();
		if (nodeList != NULL) // this better not happen.
		{
			for (std::vector<Node*>::const_iterator
					i = nodeList->begin();
					i != nodeList->end();
					++i)
			{
				if (_battleSave->getTileEngine()->distanceSq(
														(*i)->getPosition(),
														_unit->getPosition(),
														false) < 5)
				{
					(*i)->setNodeType((*i)->getNodeType() | Node::TYPE_DANGEROUS);
				}
			}
		}
	}
}

/**
 * Deletes the UnitDieBState.
 */
UnitDieBState::~UnitDieBState()
{
	_parent->setStateInterval(BattlescapeState::STATE_INTERVAL_STANDARD); // kL
}

/**
 * Initializes this state.
 */
//void UnitDieBState::init(){}

/**
 * Runs state functionality every cycle.
 * @note Progresses the death sequence, displays any messages, checks if the
 * mission is over, etc.
 */
void UnitDieBState::think()
{
// #0
	if (_noSound == false
		&& _doneScream == false)
	{
		_doneScream = true;

		if (_unit->getOverDose() == false)
			_unit->playDeathSound();

		if (_unit->getUnitVisible() == true)
		{
			// This was also done in cTor, but terrain Explosions can/do change camera before the turn/spin & collapse happen.
			Camera* const deathCam = _parent->getMap()->getCamera();
			if (deathCam->isOnScreen(_unit->getPosition()) == false)
				deathCam->centerOnPosition(_unit->getPosition());
			else if (_unit->getPosition().z != deathCam->getViewLevel())
				deathCam->setViewLevel(_unit->getPosition().z);
		}
	}

// #1
	if (_unit->getStatus() == STATUS_TURNING)
	{
		if (_unit->getSpinPhase() != -1)
		{
			_parent->setStateInterval(BattlescapeState::STATE_INTERVAL_STANDARD * 2 / 7);
			_unit->contDeathSpin(); // -> STATUS_STANDING
		}
		else // spawn conversion is going to happen
			_unit->turn(); // -> STATUS_STANDING
	}
// #3
	else if (_unit->getStatus() == STATUS_COLLAPSING)
	{
		_unit->keepFalling(); // -> STATUS_DEAD or STATUS_UNCONSCIOUS ( ie. isOut() )
	}
// #2
//	else if (_unit->isOut() == false) // this ought be Status_Standing/Disabled also.
	else if (_unit->isOut_t(OUT_STAT) == false) // this ought be Status_Standing/Disabled also.
	{
		_parent->setStateInterval(BattlescapeState::STATE_INTERVAL_STANDARD);
		_unit->startFalling(); // -> STATUS_COLLAPSING

		if (_unit->getSpawnUnit().empty() == false)
		{
			while (_unit->getStatus() == STATUS_COLLAPSING)
				_unit->keepFalling(); // -> STATUS_DEAD or STATUS_UNCONSCIOUS ( ie. isOut() ) -> goto #4
		}
	}

// #6
	if (_extraTicks == 1)
	{
		bool moreDead = false;
		for (std::vector<BattleUnit*>::const_iterator
				i = _battleSave->getUnits()->begin();
				i != _battleSave->getUnits()->end();
				++i)
		{
			if ((*i)->getAboutToDie() == true)
			{
				moreDead = true;
				break;
			}
		}
		if (moreDead == false)
			_parent->getMap()->setUnitDying(false);

		_parent->getTileEngine()->calculateUnitLighting();
		_parent->popState();

		// need to freshen visUnit-indicators in case other units were hiding behind the one who just fell
		_battleSave->getBattleState()->updateSoldierInfo(false);

		if (_unit->getGeoscapeSoldier() != NULL
			&& _unit->getOriginalFaction() == FACTION_PLAYER)
		{
			std::string stInfo;
			if (_unit->getStatus() == STATUS_DEAD)
			{
				if (_dType == DT_NONE
					&& _unit->getSpawnUnit().empty() == true)
				{
					stInfo = "STR_HAS_DIED_FROM_A_FATAL_WOUND";
				}
				else if (Options::battleNotifyDeath == true)
					stInfo = "STR_HAS_BEEN_KILLED";
			}
			else
				stInfo = "STR_HAS_BECOME_UNCONSCIOUS";

			if (stInfo.empty() == false)
			{
				Game* const game = _battleSave->getBattleState()->getGame();
				const Language* const lang = game->getLanguage();
				game->pushState(new InfoboxOKState(lang->getString(
																stInfo,
																_unit->getGender())
															.arg(_unit->getName(lang))));
			}
		}
/*		// if all units from either faction are killed - auto-end the mission.
		if (Options::battleAutoEnd == true
			&& _battleSave->getSide() == FACTION_PLAYER)
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
				_battleSave->setSelectedUnit(NULL);
				_parent->cancelCurrentAction(true);

				_parent->requestEndTurn();
			}
		} */
	}
// #5
//	else if (_extraTicks > 0)
//		++_extraTicks;
// #4
//	else if (_unit->isOut() == true)
	else if (_unit->isOut_t(OUT_STAT) == true) // and this ought be Status_Dead OR _Unconscious.
	{
		_extraTicks = 1;

		if (_unit->getStatus() == STATUS_UNCONSCIOUS
			&& _unit->getSpecialAbility() == SPECAB_EXPLODE)
		{
			_unit->instaKill();
		}

		_unit->putDown();

		if (_unit->getSpawnUnit().empty() == true)
			convertToCorpse();
		else
			_parent->convertUnit(
							_unit,
							_unit->getSpawnUnit());
	}

	_unit->setCache(NULL);
	_parent->getMap()->cacheUnit(_unit);
}

/**
 * Converts unit to a corpse-item.
 * @note This is used also for units that go unconscious.
 * @note Dead or Unconscious units get a NULL-Tile ptr but keep track of the
 * Position of their death.
 */
void UnitDieBState::convertToCorpse() // private.
{
	_battleSave->getBattleState()->showPsiButton(false);

	const Position pos = _unit->getPosition();

	// remove the unconscious body item corresponding to this unit,
	// and if it was being carried, keep track of what slot it was in
	const bool carried = (pos == Position(-1,-1,-1));
	if (carried == false)
		_battleSave->removeCorpse(_unit);

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
				i = _battleSave->getItems()->begin();
				i != _battleSave->getItems()->end();
				++i)
		{
			if ((*i)->getUnit() == _unit) // unit is in an inventory so unit must be a 1x1 unit
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
				tile = _battleSave->getTile(pos + Position(x,y,0));

				// This block is lifted from TileEngine::explode(), switch(DT_IN).
				if (_unit->getUnitRules() != NULL
					&& _unit->getUnitRules()->isMechanical() == true)
				{
					if (RNG::percent(6) == true)
					{
						explTile = tile;
						explTile_b = _battleSave->getTile(explTile->getPosition() + Position(0,0,-1));

						while (explTile != NULL // safety.
							&& explTile->getPosition().z > 0
							&& explTile->getMapData(O_OBJECT) == NULL // only floors & content can catch fire.
							&& explTile->getMapData(O_FLOOR) == NULL
							&& explTile->hasNoFloor(explTile_b) == true)
						{
							explTile = explTile_b;
							explTile_b = _battleSave->getTile(explTile->getPosition() + Position(0,0,-1));
						}

						if (explTile != NULL // safety.
							&& explTile->getFire() == 0)
						{
							explTile->addFire(explTile->getFuel() + RNG::generate(1,2)); // Could use a ruleset-factor in here.
							explTile->addSmoke(std::max(
													1,
													std::min(
														6,
														explTile->getFlammability() / 10)));

							if (soundPlayed == false)
							{
								soundPlayed = true;
								_parent->getResourcePack()->getSound(
																"BATTLE.CAT",
																ResourcePack::SMALL_EXPLOSION)
															->play(
																-1,
																_parent->getMap()->getSoundAngle(_unit->getPosition()));
							}
						}
					}

					tile->addSmoke(RNG::generate(0,2)); // more smoke ...
				}

				BattleItem* const corpse = new BattleItem(
													_parent->getRuleset()->getItem(_unit->getArmor()->getCorpseBattlescape()[--part]),
													_battleSave->getNextItemId());
				corpse->setUnit(_unit);

				if (tile != NULL					// kL, safety. (had a CTD when ethereal dies on water).
					&& tile->getUnit() == _unit)	// <- check in case unit was displaced by another unit ... that sounds pretty darn shakey.
				{									// TODO: iterate over all mapTiles searching for the unit and NULLing any tile's association to it.
					tile->setUnit(NULL);
				}

				_parent->dropItem(
								pos + Position(x,y,0),
								corpse,
								true);
			}
		}

		// expose any units that were hiding behind dead unit
		_parent->getTileEngine()->calculateFOV(
											pos,
											true);
	}
}

/**
 * Unit dying cannot be cancelled.
 */
//void UnitDieBState::cancel()
//{}

}
