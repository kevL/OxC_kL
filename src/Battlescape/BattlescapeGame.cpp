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

#include "BattlescapeGame.h"

//#define _USE_MATH_DEFINES
//#include <cmath>
//#include <sstream>
//#include <typeinfo>

#include "AbortMissionState.h"
#include "AlienBAIState.h"
#include "BattlescapeState.h"
#include "BattleState.h"
#include "Camera.h"
#include "CivilianBAIState.h"
#include "ExplosionBState.h"
#include "InfoboxOKState.h"
#include "InfoboxState.h"
#include "Map.h"
#include "NextTurnState.h"
#include "Pathfinding.h"
#include "ProjectileFlyBState.h"
#include "TileEngine.h"
#include "UnitDieBState.h"
#include "UnitFallBState.h"
#include "UnitInfoState.h"
#include "UnitPanicBState.h"
#include "UnitTurnBState.h"
#include "UnitWalkBState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Options.h"
//#include "../Engine/RNG.h"
#include "../Engine/Sound.h"

#include "../Interface/Cursor.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/Armor.h"
#include "../Ruleset/RuleInventory.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

bool BattlescapeGame::_debugPlay = false;


/**
 * Initializes all the elements in the Battlescape screen.
 * @param save			- pointer to the SavedBattleGame
 * @param parentState	- pointer to the parent BattlescapeState
 */
BattlescapeGame::BattlescapeGame(
		SavedBattleGame* save,
		BattlescapeState* parentState)
	:
		_save(save),
		_parentState(parentState),
		_playedAggroSound(false),
		_endTurnRequested(false)
{
	//Log(LOG_INFO) << "Create BattlescapeGame";
	_debugPlay = false;
	_playerPanicHandled = true;
	_AIActionCounter = 0;
	_AISecondMove = false;
	_currentAction.actor = NULL;

	checkForCasualties(
					NULL,
					NULL,
					true);

	cancelCurrentAction();

	_currentAction.type = BA_NONE;
	_currentAction.targeting = false;

	_universalFist = new BattleItem(
								getRuleset()->getItem("STR_FIST"),
								save->getCurrentItemId());
	_alienPsi = new BattleItem(
							getRuleset()->getItem("ALIEN_PSI_WEAPON"),
							save->getCurrentItemId());

	for (std::vector<BattleUnit*>::const_iterator // kL
			i = _save->getUnits()->begin();
			i != _save->getUnits()->end();
			++i)
	{
		(*i)->setBattleGame(this);
	}
}

/**
 * Delete BattlescapeGame.
 */
BattlescapeGame::~BattlescapeGame()
{
	for (std::list<BattleState*>::const_iterator
			i = _states.begin();
			i != _states.end();
			++i)
	{
		delete *i;
	}

	delete _universalFist;
	delete _alienPsi;
}

/**
 * Checks for units panicking or falling and so on.
 */
void BattlescapeGame::think()
{
	//Log(LOG_INFO) << "BattlescapeGame::think()";

	// nothing is happening - see if we need some alien AI or units panicking or what have you
	if (_states.empty() == true)
	{
		if (_save->getSide() != FACTION_PLAYER) // it's a non player side (ALIENS or CIVILIANS)
		{
			if (_debugPlay == false)
			{
				if (_save->getSelectedUnit() != NULL)
				{
					if (handlePanickingUnit(_save->getSelectedUnit()) == false)
					{
						//Log(LOG_INFO) << "BattlescapeGame::think() call handleAI() ID " << _save->getSelectedUnit()->getId();
						handleAI(_save->getSelectedUnit());
					}
				}
				else
				{
					if (_save->selectNextFactionUnit(
												true,
												_AISecondMove) == NULL)
					{
						if (_save->getDebugMode() == false)
						{
							_endTurnRequested = true;
							statePushBack(NULL); // end AI turn
						}
						else
						{
							_save->selectNextFactionUnit();
							_debugPlay = true;
						}
					}
				}
			}
		}
		else // it's a player side && we have not handled all panicking units
		{
			if (_playerPanicHandled == false)
			{
				_playerPanicHandled = handlePanickingPlayer();
				_save->getBattleState()->updateSoldierInfo();
			}

			_parentState->updateExperienceInfo(); // kL
		}

		if (_save->getUnitsFalling() == true)
		{
			//Log(LOG_INFO) << "BattlescapeGame::think(), get/setUnitsFalling() ID " << _save->getSelectedUnit()->getId();
			statePushFront(new UnitFallBState(this));
			_save->setUnitsFalling(false);
		}
	}
	//Log(LOG_INFO) << "BattlescapeGame::think() EXIT";
}

/**
 * Initializes the Battlescape game.
 */
void BattlescapeGame::init()
{
	if (_save->getSide() == FACTION_PLAYER
		&& _save->getTurn() > 1)
	{
		_playerPanicHandled = false;
	}
}

/**
 * Handles the processing of the AI states of a unit.
 * @param unit - pointer to a BattleUnit
 */
void BattlescapeGame::handleAI(BattleUnit* unit)
{
	//Log(LOG_INFO) << "BattlescapeGame::handleAI() " << unit->getId();
	if (unit->getTimeUnits() < 4) // kL
		unit->dontReselect();

	if ( //kL unit->getTimeUnits() < 6 ||
		_AIActionCounter > 1
		|| unit->reselectAllowed() == false)
	{
		if (_save->selectNextFactionUnit(
									true,
									_AISecondMove) == NULL)
		{
			if (_save->getDebugMode() == false)
			{
				_endTurnRequested = true;
				//Log(LOG_INFO) << "BattlescapeGame::handleAI() statePushBack(end AI turn)";
				statePushBack(NULL); // end AI turn
			}
			else
			{
				_save->selectNextFactionUnit();
				_debugPlay = true;
			}
		}

		if (_save->getSelectedUnit() != NULL)
		{
			_parentState->updateSoldierInfo();

			getMap()->getCamera()->centerOnPosition(_save->getSelectedUnit()->getPosition());

			if (_save->getSelectedUnit()->getId() <= unit->getId())
				_AISecondMove = true;
		}

		_AIActionCounter = 0;

		//Log(LOG_INFO) << "BattlescapeGame::handleAI() Pre-EXIT";
		return;
	}


	unit->setUnitVisible(false);

	// might need this: populate _visibleUnit for a newly-created alien
	//Log(LOG_INFO) << "BattlescapeGame::handleAI(), calculateFOV() call";
	_save->getTileEngine()->calculateFOV(unit->getPosition());
		// it might also help chryssalids realize they've zombified someone and need to move on;
		// it should also hide units when they've killed the guy spotting them;
		// it's also for good luck

	const BattleAIState* ai = unit->getCurrentAIState();
	if (ai == NULL)
	{
		// for some reason the unit had no AI routine assigned..
		//Log(LOG_INFO) << "BattlescapeGame::handleAI() !ai, assign AI";
		if (unit->getFaction() == FACTION_HOSTILE)
			unit->setAIState(new AlienBAIState(
											_save,
											unit,
											NULL));
		else
			unit->setAIState(new CivilianBAIState(
											_save,
											unit,
											NULL));

		ai = unit->getCurrentAIState();
	}

	++_AIActionCounter;
	if (_AIActionCounter == 1)
	{
		_playedAggroSound = false;
		unit->setHiding(false);
		//if (Options::traceAI) Log(LOG_INFO) << "#" << unit->getId() << "--" << unit->getType();
	}
	//Log(LOG_INFO) << ". _AIActionCounter DONE";


	// this cast only works when ai was already AlienBAIState at heart
//	AlienBAIState* aggro = dynamic_cast<AlienBAIState*>(ai);

	//Log(LOG_INFO) << ". Declare action";
	BattleAction action;
	//Log(LOG_INFO) << ". Define action.actor";
	action.actor = unit;
	//Log(LOG_INFO) << ". Define action.number";
	action.number = _AIActionCounter;
	//Log(LOG_INFO) << ". unit->think(&action)";
	unit->think(&action);
	//Log(LOG_INFO) << ". _unit->think() DONE";

	if (action.type == BA_RETHINK)
	{
//		_parentState->debug(L"Rethink");
		unit->think(&action);
	}
	//Log(LOG_INFO) << ". BA_RETHINK DONE";


	_AIActionCounter = action.number;

	//Log(LOG_INFO) << ". pre hunt for weapon";
	if (unit->getOriginalFaction() == FACTION_HOSTILE
		&& unit->getMainHandWeapon() == NULL)
//		&& unit->getVisibleUnits()->size() == 0)
	// TODO: and, if either no innate meleeWeapon, or a visible hostile is not within say 5 tiles.
	{
		//Log(LOG_INFO) << ". . no mainhand weapon or no ammo";
		//Log(LOG_INFO) << ". . . call findItem()";
		findItem(&action);
	}
	//Log(LOG_INFO) << ". findItem DONE";

	if (unit->getCharging() != NULL)
	{
		if (unit->getAggroSound() != -1
			&& _playedAggroSound == false)
		{
			_playedAggroSound = true;
			getResourcePack()->getSoundByDepth(
											_save->getDepth(),
											unit->getAggroSound())
										->play(
											-1,
											getMap()->getSoundAngle(unit->getPosition()));
		}
	}
	//Log(LOG_INFO) << ". getCharging DONE";


//	std::wostringstream ss; // debug.

	if (action.type == BA_WALK)
	{
//		ss << L"Walking to " << action.target;
//		_parentState->debug(ss.str());

		if (_save->getTile(action.target) != NULL)
			_save->getPathfinding()->calculate(
											action.actor,
											action.target);
//											_save->getTile(action.target)->getUnit());

		if (_save->getPathfinding()->getStartDirection() != -1)
			statePushBack(new UnitWalkBState(
											this,
											action));
	}
	//Log(LOG_INFO) << ". BA_WALK DONE";


	if (action.type == BA_SNAPSHOT
		|| action.type == BA_AUTOSHOT
		|| action.type == BA_AIMEDSHOT
		|| action.type == BA_THROW
		|| action.type == BA_HIT
		|| action.type == BA_MINDCONTROL
		|| action.type == BA_PANIC
		|| action.type == BA_LAUNCH)
	{
		//Log(LOG_INFO) << ". . in action.type";
		if (action.type == BA_MINDCONTROL
			|| action.type == BA_PANIC)
		{
			//Log(LOG_INFO) << ". . do Psi";
			action.weapon = _alienPsi; // kL
			action.TU = unit->getActionTUs(
										action.type,
										action.weapon);
		}
		else
		{
			statePushBack(new UnitTurnBState(
											this,
											action));

			if (action.type == BA_HIT)
			{
				const std::string meleeWeapon = unit->getMeleeWeapon();
				bool instaWeapon = false;

				if (action.weapon != _universalFist
					&& meleeWeapon.empty() == false)
				{
					bool found = false;
					for (std::vector<BattleItem*>::const_iterator
							i = unit->getInventory()->begin();
							i != unit->getInventory()->end()
								&& found == false;
							++i)
					{
						if ((*i)->getRules()->getType() == meleeWeapon)
						{
							// note this ought be conformed w/ bgen.addAlien equipped items to
							// ensure radical (or standard) BT_MELEE weapons get equipped in hand;
							// but for now just grab the meleeItem wherever it was equipped ...
							found = true;
							action.weapon = *i;
						}
					}

					if (found == false)
					{
						instaWeapon = true;
						action.weapon = new BattleItem(
													getRuleset()->getItem(meleeWeapon),
													_save->getCurrentItemId());
						action.weapon->setOwner(unit);
					}
				}
				else if (action.weapon != NULL
					&& action.weapon->getRules()->getBattleType() != BT_MELEE
					&& action.weapon->getRules()->getBattleType() != BT_FIREARM)
				{
					action.weapon = NULL;
				}

				if (action.weapon != NULL) // also checked in getActionTUs() & ProjectileFlyBState::init()
				{
					action.TU = unit->getActionTUs(
												action.type,
												action.weapon);

					statePushBack(new ProjectileFlyBState(
														this,
														action));

					if (instaWeapon == true)
						_save->removeItem(action.weapon);
				}

				return;
			}
		}

//		ss.clear();
//		ss << L"Attack type = " << action.type
//				<< ", target = " << action.target
//				<< ", weapon = " << action.weapon->getRules()->getName().c_str();
//		_parentState->debug(ss.str());

		//Log(LOG_INFO) << ". attack action.Type = " << action.type
		//		<< ", action.Target = " << action.target
		//		<< " action.Weapon = " << action.weapon->getRules()->getName().c_str();


		//Log(LOG_INFO) << ". . call ProjectileFlyBState()";
		statePushBack(new ProjectileFlyBState(
											this,
											action));
		//Log(LOG_INFO) << ". . ProjectileFlyBState DONE";

		if (action.type == BA_PANIC
			|| action.type == BA_MINDCONTROL)
		{
			//Log(LOG_INFO) << ". . . in action.type Psi";
			const bool success = _save->getTileEngine()->psiAttack(&action);
			//Log(LOG_INFO) << ". . . success = " << success;
			if (success == true)
			{
				std::string st;
				if (action.type == BA_MINDCONTROL)
					st = "STR_IS_UNDER_ALIEN_CONTROL";
				else
					st = "STR_MORALE_ATTACK_SUCCESSFUL";

				const BattleUnit* const unit = _save->getTile(action.target)->getUnit();
				Game* const game = _parentState->getGame();
				game->pushState(new InfoboxState(game->getLanguage()->getString(
																		st,
																		unit->getGender())
																			.arg(unit->getName(game->getLanguage()))));
			}
			//Log(LOG_INFO) << ". . . done Psi.";
		}
	}
	//Log(LOG_INFO) << ". . action.type DONE";

	if (action.type == BA_NONE)
	{
		//Log(LOG_INFO) << ". . in action.type None";
//		_parentState->debug(L"Idle");
		_AIActionCounter = 0;

		if (_save->selectNextFactionUnit(
									true,
									_AISecondMove) == NULL)
		{
			if (_save->getDebugMode() == false)
			{
				_endTurnRequested = true;
				//Log(LOG_INFO) << "BattlescapeGame::handleAI() statePushBack(end AI turn) 2";
				statePushBack(NULL); // end AI turn
			}
			else
			{
				_save->selectNextFactionUnit();
				_debugPlay = true;
			}
		}

		if (_save->getSelectedUnit() != NULL)
		{
			_parentState->updateSoldierInfo();

			getMap()->getCamera()->centerOnPosition(_save->getSelectedUnit()->getPosition());

			if (_save->getSelectedUnit()->getId() <= unit->getId())
				_AISecondMove = true;
		}
	}
	//Log(LOG_INFO) << "BattlescapeGame::handleAI() EXIT";
}

/**
 * Toggles the kneel/stand status of a unit.
 * @param bu - pointer to a BattleUnit
 * @return, true if the action succeeded
 */
bool BattlescapeGame::kneel(BattleUnit* bu)
{
	//Log(LOG_INFO) << "BattlescapeGame::kneel()";
	if (bu->getGeoscapeSoldier() != NULL
		&& bu->getFaction() == bu->getOriginalFaction())
	{
		if (bu->isFloating() == false) // kL_note: This prevents flying soldiers from 'kneeling' .....
		{
			int tu;
			if (bu->isKneeled() == true)
				tu = 10;
			else
				tu = 3;

			if (checkReservedTU(bu, tu) == true
				|| (tu == 3
					&& _save->getKneelReserved() == true))
			{
				if (bu->getTimeUnits() >= tu)
				{
					if (tu == 3
						|| (tu == 10
							&& bu->spendEnergy(3) == true))
					{
						bu->spendTimeUnits(tu);
						bu->kneel(bu->isKneeled() == false);
						// kneeling or standing up can reveal new terrain or units. I guess. -> sure can!
						// but updateSoldierInfo() also does does calculateFOV(), so ...
//						getTileEngine()->calculateFOV(bu);

						getMap()->cacheUnits();
//kL						_parentState->updateSoldierInfo(false); // <- also does calculateFOV() !
							// wait... shouldn't one of those calcFoV's actually trigger!! ? !
							// Hopefully it's done after returning, in another updateSoldierInfo... or newVis check.
							// So.. I put this in BattlescapeState::btnKneelClick() instead; updates will
							// otherwise be handled by walking or what have you. Doing it this way conforms
							// updates/FoV checks with my newVis routines.

//kL						getTileEngine()->checkReactionFire(bu);
							// ditto..

						return true;
					}
					else
						_parentState->warning("STR_NOT_ENOUGH_ENERGY");
				}
				else
					_parentState->warning("STR_NOT_ENOUGH_TIME_UNITS");
			}
			else
				_parentState->warning("STR_TIME_UNITS_RESERVED");
		}
		else
			_parentState->warning("STR_ACTION_NOT_ALLOWED_FLOAT");
	}
	else if (bu->getGeoscapeSoldier() != NULL) // MC'd xCom agent, trying to stand & walk by AI.
	{
		if (bu->getTimeUnits() > 9
			&& bu->getEnergy() > 2)
		{
			bu->spendTimeUnits(10);
			bu->spendEnergy(3);

			bu->kneel(false);
			getMap()->cacheUnits();

			return true;
		}
	}
	else // MOB has Unit-rules
	{
		if (bu->getUnitRules()->isMechanical() == false)
			_parentState->warning("STR_ACTION_NOT_ALLOWED_ALIEN");
	}

	return false;
}

/**
 * Ends the turn.
 * This starts the switchover
 *	- popState()
 *	- handleState()
 *	- statePushBack()
 */
void BattlescapeGame::endTurnPhase()
{
	_debugPlay = false;
	_AISecondMove = false;
	_parentState->showLaunchButton(false);

	_currentAction.targeting = false;
	_currentAction.type = BA_NONE;
	_currentAction.waypoints.clear();
	getMap()->getWaypoints()->clear();

	Position pos;
	for (int // check for hot grenades on the ground
			i = 0;
			i != _save->getMapSizeXYZ();
			++i)
	{
		for (std::vector<BattleItem*>::const_iterator
				j = _save->getTiles()[i]->getInventory()->begin();
				j != _save->getTiles()[i]->getInventory()->end();
				)
		{
			if ((*j)->getRules()->getBattleType() == BT_GRENADE
				&& (*j)->getFuseTimer() != -1
				&& (*j)->getFuseTimer() < 2) // it's a grenade to explode now
			{
				pos.x = _save->getTiles()[i]->getPosition().x * 16 + 8;
				pos.y = _save->getTiles()[i]->getPosition().y * 16 + 8;
				pos.z = _save->getTiles()[i]->getPosition().z * 24 - _save->getTiles()[i]->getTerrainLevel();

				statePushNext(new ExplosionBState(
												this,
												pos,
												*j,
												(*j)->getPreviousOwner()));
				_save->removeItem(*j);

				statePushBack(NULL);
				return;
			}

			++j;
		}
	}

	if (_save->getTileEngine()->closeUfoDoors() != 0
		&& ResourcePack::SLIDING_DOOR_CLOSE != -1) // try, close doors between grenade & terrain explosions
	{
		getResourcePack()->getSoundByDepth( // ufo door closed
										_save->getDepth(),
										ResourcePack::SLIDING_DOOR_CLOSE)
									->play();
	}

	// check for terrain explosions
	Tile* tile = _save->getTileEngine()->checkForTerrainExplosions();
	if (tile != NULL)
	{
		pos = Position(
					tile->getPosition().x * 16 + 8,
					tile->getPosition().y * 16 + 8,
					tile->getPosition().z * 24 + 10);

		// kL_note: This seems to be screwing up.
		// Further info: what happens is that an explosive part of a tile gets destroyed by fire
		// during an endTurn sequence, has it's setExplosive() set, then is somehow triggered
		// by the next projectile hit against whatever.
		statePushNext(new ExplosionBState(
										this,
										pos,
										NULL,
										NULL,
										tile));

//		tile = _save->getTileEngine()->checkForTerrainExplosions();

		statePushBack(NULL);	// this will repeatedly call another endTurnPhase() so there's
		return;					// no need to continue this one till all explosions are done.
								// The problem arises because _save->endBattlePhase() below
								// causes destruction of explosive objects, that don't explode
								// until some later instantiation of ExplosionBState .....
								//
								// As to why this doesn't simply loop like other calls to
								// do terrainExplosions i don't know.
	}

	if (_save->getSide() != FACTION_NEUTRAL) // tick down grenade timers
	{
		for (std::vector<BattleItem*>::const_iterator
				i = _save->getItems()->begin();
				i != _save->getItems()->end();
				++i)
		{
			if ((*i)->getOwner() == NULL
				&& (*i)->getRules()->getBattleType() == BT_GRENADE
				&& (*i)->getFuseTimer() > 1)
			{
				(*i)->setFuseTimer((*i)->getFuseTimer() - 1);
			}
		}
	}

	// THE NEXT 3 SECTIONS could get Quirky. (ie: tiles vs. units, tallyUnits, tiles vs. ground-items)
	for (std::vector<BattleUnit*>::const_iterator
			i = _save->getUnits()->begin();
			i != _save->getUnits()->end();
			++i)
	{
		if ((*i)->getFaction() == _save->getSide())
		{
			tile = (*i)->getTile();
			if (tile != NULL
				&& (*i)->getHealth() > 0
				&& ((*i)->getGeoscapeSoldier() != NULL
					|| (*i)->getUnitRules()->isMechanical() == false))
			{
				tile->endTilePhase(); // damage tile's unit w/ Fire & Smoke at end of its faction's Turn-phase.
			}
		}
	}


	// if all units from either faction are killed - the mission is over.
	// ... should this go after checkForCasualties() below ...
	int
		liveAliens = 0,
		liveSoldiers = 0;
	// we'll tally them NOW, so that any infected units will... change
	tallyUnits(
			liveAliens,
			liveSoldiers,
			true);


	if (_save->endBattlePhase() == true) // <- This rolls over Faction-turns.
	{
		size_t mapSize = static_cast<size_t>(_save->getMapSizeXYZ());
		for (size_t
				i = 0;
				i != mapSize;
				++i)
		{
			tile = _save->getTiles()[i];
			if (tile->getInventory()->empty() == false)
				tile->endTilePhase(_save); // damage tile's items w/ Fire at end of each full-turn.
		}

		for (std::vector<BattleUnit*>::const_iterator // reset the takenExpl(smoke) and takenFire flags on every unit.
				i = _save->getUnits()->begin();
				i != _save->getUnits()->end();
				++i)
		{
			(*i)->setTakenExpl(false); // even if Status_Dead, just do it.
			(*i)->setTakenFire(false);
		}
	}
	// best just to do another call to checkForTerrainExplosions()/ ExplosionBState in there ....
	// -> SavedBattleGame::prepareBattleTurn()
	// Or here
	// ... done it in NextTurnState.

	// check AGAIN for terrain explosions
/*	tile = _save->getTileEngine()->checkForTerrainExplosions();
	if (tile != NULL)
	{
		pos = Position(
					tile->getPosition().x * 16 + 8,
					tile->getPosition().y * 16 + 8,
					tile->getPosition().z * 24 + 10);
//		statePushNext(new ExplosionBState(
		statePushBack(new ExplosionBState(
										this,
										pos,
										NULL,
										NULL,
										tile));
	} */


	if (_save->getDebugMode() == false)
	{
		if (_save->getSide() == FACTION_PLAYER)
		{
			setupCursor();
			_save->getBattleState()->toggleIcons(true);
		}
		else
		{
			getMap()->setCursorType(CT_NONE);
			_save->getBattleState()->toggleIcons(false);
		}
	}

	checkForCasualties(
					NULL,
					NULL);

/*	// ... moved here from before endBattlePhase() above ...
	// not sure how tallyUnits() interacts w/ checkForCasualties()
	int
		liveAliens = 0,
		liveSoldiers = 0;
	// we'll tally them NOW, so that any infected units will... change
	tallyUnits(
			liveAliens,
			liveSoldiers,
			true); */

	if (_save->allObjectivesDestroyed() == true)
	{
		_parentState->finishBattle(
								false,
								liveSoldiers);
		return;
	}

	if (liveAliens > 0
		&& liveSoldiers > 0)
	{
		showInfoBoxQueue();

		_parentState->updateSoldierInfo();

		if (playableUnitSelected() == true) // <- only Faction_Player (or Debug-mode)
		{
			getMap()->getCamera()->centerOnPosition(_save->getSelectedUnit()->getPosition());
			setupCursor();
		}
	}

	const bool battleComplete = liveAliens == 0
							 || liveSoldiers == 0;

	if (_endTurnRequested == true
		&& (_save->getSide() != FACTION_NEUTRAL
			|| battleComplete == true))
	{
		_parentState->getGame()->pushState(new NextTurnState(
														_save,
														_parentState));
	}

	_endTurnRequested = false;
}

/**
 * Checks for casualties and adjusts morale accordingly.
 * @param weapon	- pointer to the weapon responsible
 * @param slayer	- pointer to credit the kill
 * @param hidden	- true for UFO Power Source explosions at the start of battlescape (default false)
 * @param terrain	- true for terrain explosions (default false)
 */
void BattlescapeGame::checkForCasualties(
		const BattleItem* const weapon,
		BattleUnit* slayer,
		bool hidden,
		bool terrain)
{
	// If the victim was killed by the slayer's death explosion,
	// fetch who killed the murderer and make THAT the murderer!
	if (slayer != NULL
		&& slayer->getGeoscapeSoldier() == NULL
		&& slayer->getUnitRules()->getSpecialAbility() == SPECAB_EXPLODEONDEATH
		&& slayer->getStatus() == STATUS_DEAD
		&& slayer->getMurdererId() != 0)
	{
		for (std::vector<BattleUnit*>::const_iterator
				i = _save->getUnits()->begin();
				i != _save->getUnits()->end();
				++i)
		{
			if ((*i)->getId() == slayer->getMurdererId())
				slayer = *i;
		}
	}
	// kL_note: what about tile explosions

	std::string
		killStatRace = "STR_UNKNOWN",
		killStatRank = "STR_UNKNOWN",
		killStatWeapon = "STR_WEAPON_UNKNOWN",
		killStatWeaponAmmo = "STR_WEAPON_UNKNOWN";
	int
		killStatMission = 0,
		killStatTurn = 0,
		killStatPoints = 0;


	if (slayer != NULL
		&& slayer->getGeoscapeSoldier() != NULL)
	{
		killStatMission = _save->getGeoscapeSave()->getMissionStatistics()->size();

		killStatTurn = _save->getTurn() * 3;
		if (_save->getSide() == FACTION_HOSTILE)
			killStatTurn += 1;
		else if (_save->getSide() == FACTION_NEUTRAL)
			killStatTurn += 2;

		if (weapon != NULL)
		{
			killStatWeaponAmmo = weapon->getRules()->getName();
			killStatWeapon = killStatWeaponAmmo;
		}

		const BattleItem* ammo = slayer->getItem("STR_RIGHT_HAND");
		if (ammo != NULL)
		{
			for (std::vector<std::string>::const_iterator
					i = ammo->getRules()->getCompatibleAmmo()->begin();
					i != ammo->getRules()->getCompatibleAmmo()->end();
					++i)
			{
				if (*i == killStatWeaponAmmo)
					killStatWeapon = ammo->getRules()->getName();
			}
		}

		ammo = slayer->getItem("STR_LEFT_HAND");
		if (ammo != NULL)
		{
			for (std::vector<std::string>::const_iterator
					i = ammo->getRules()->getCompatibleAmmo()->begin();
					i != ammo->getRules()->getCompatibleAmmo()->end();
					++i)
			{
				if (*i == killStatWeaponAmmo)
					killStatWeapon = ammo->getRules()->getName();
			}
		}
	}

	for (std::vector<BattleUnit*>::const_iterator
			i = _save->getUnits()->begin();
			i != _save->getUnits()->end();
			++i)
	{
		const bool
			death = ((*i)->getHealth() == 0),
			stun = ((*i)->getHealth() <= (*i)->getStun());

		if (death == false
			&& stun == false)
		{
			continue;
		}

		BattleUnit* const victim = *i; // kL

		// Awards: decide victim race and rank
		if (slayer != NULL
			&& slayer->getGeoscapeSoldier() != NULL)
		{
			killStatPoints = victim->getValue();

			if (victim->getOriginalFaction() == FACTION_PLAYER)	// <- xCom DIED
			{
				killStatPoints = -killStatPoints;

				if (victim->getGeoscapeSoldier() != NULL)	// Soldier
				{
					killStatRace = "STR_HUMAN";
					killStatRank = victim->getGeoscapeSoldier()->getRankString();
				}
				else										// HWP
				{
					killStatRace = "STR_TANK";
					killStatRank = "STR_HEAVY_WEAPONS_PLATFORM_LC";
				}
			}
			else if (victim->getOriginalFaction() == FACTION_HOSTILE)	// <- aLien DIED
			{
				killStatRace = victim->getUnitRules()->getRace();
				killStatRank = victim->getUnitRules()->getRank();
			}
			else if (victim->getOriginalFaction() == FACTION_NEUTRAL)	// <- Civilian DIED
			{
				killStatPoints = -killStatPoints * 2;
				killStatRace = "STR_HUMAN";
				killStatRank = "STR_CIVILIAN";
			}
		}


		if (death == true
			&& victim->getStatus() != STATUS_DEAD
			&& victim->getStatus() != STATUS_COLLAPSING	// kL_note: is this really needed ....
			&& victim->getStatus() != STATUS_TURNING	// kL: may be set by UnitDieBState cTor
			&& victim->getStatus() != STATUS_DISABLED)	// kL
		{
			(*i)->setStatus(STATUS_DISABLED); // kL

			// slayer's Morale Bonus & diary ->
			if (slayer != NULL)
			{
				(*i)->killedBy(slayer->getFaction()); // used in DebriefingState.

				if (slayer->isFearable() == true)
				{
					if (slayer->getGeoscapeSoldier() != NULL)
					{
						slayer->getStatistics()->kills.push_back(new BattleUnitKills(
																				killStatRank,
																				killStatRace,
																				killStatWeapon,
																				killStatWeaponAmmo,
																				victim->getFaction(),
																				STATUS_DEAD,
																				killStatMission,
																				killStatTurn,
																				killStatPoints));
						victim->setMurdererId(slayer->getId());
					}

					int bonus = 0;
					if (slayer->getOriginalFaction() == FACTION_PLAYER)
					{
						slayer->addKillCount();
						bonus = _save->getMoraleModifier();
					}
					else if (slayer->getOriginalFaction() == FACTION_HOSTILE)
						bonus = _save->getMoraleModifier(NULL, false);

					// slayer's Valor
					if ((slayer->getOriginalFaction() == FACTION_HOSTILE
							&& victim->getOriginalFaction() == FACTION_PLAYER)
						|| (slayer->getOriginalFaction() == FACTION_PLAYER
							&& victim->getOriginalFaction() == FACTION_HOSTILE))
					{
						const int courage = 10 * bonus / 100;
						slayer->moraleChange(courage); // double what rest of squad gets below
					}
					// slayer (mc'd or not) will get a penalty with friendly fire (mc'd or not)
					// ... except aLiens, who don't care.
					else if (slayer->getOriginalFaction() == FACTION_PLAYER
						&& victim->getOriginalFaction() == FACTION_PLAYER)
					{
						int chagrin = 5000 / bonus; // huge chagrin!
						if (victim->getUnitRules() != NULL
							&& victim->getUnitRules()->isMechanical() == true)
						{
							chagrin /= 2;
						}
						slayer->moraleChange(-chagrin);
					}
					else if (victim->getOriginalFaction() == FACTION_NEUTRAL) // civilian kills
					{
						if (slayer->getOriginalFaction() == FACTION_PLAYER)
						{
							const int dishonor = 2000 / bonus;
							slayer->moraleChange(-dishonor);
						}
						else if (slayer->getOriginalFaction() == FACTION_HOSTILE)
							slayer->moraleChange(20); // no leadership bonus for aLiens executing civies: it's their job.
					}
				}
			}

			// cycle through units and do all faction
//			if (victim->getFaction() != FACTION_NEUTRAL) // civie deaths now affect other Factions.
//			{
			// penalty for the death of a unit; civilians & MC'd aLien units return 100.
			const int loss = _save->getMoraleModifier(victim);
			// These two are factions (aTeam & bTeam leaderships mitigate losses).
			int
				aTeam, // winners
				bTeam; // losers

			if (victim->getOriginalFaction() == FACTION_HOSTILE)
			{
				aTeam = _save->getMoraleModifier();
				bTeam = _save->getMoraleModifier(NULL, false);
			}
			else // victim is xCom or civilian
			{
				aTeam = _save->getMoraleModifier(NULL, false);
				bTeam = _save->getMoraleModifier();
			}

			for (std::vector<BattleUnit*>::const_iterator // do bystander FACTION changes:
					j = _save->getUnits()->begin();
					j != _save->getUnits()->end();
					++j)
			{
				if ((*j)->isOut(true, true) == false
					&& (*j)->isFearable() == true) // not mechanical. Or a ZOMBIE!!
				{
					if ((*j)->getOriginalFaction() == victim->getOriginalFaction()
						|| (victim->getOriginalFaction() == FACTION_NEUTRAL			// for civie-death,
							&& (*j)->getFaction() == FACTION_PLAYER						// non-Mc'd xCom takes hit
							&& (*j)->getOriginalFaction() != FACTION_HOSTILE)			// but not Mc'd aLiens
						|| (victim->getOriginalFaction() == FACTION_PLAYER			// for death of xCom unit,
							&& (*j)->getOriginalFaction() == FACTION_NEUTRAL))			// civies take hit.
					{
						// losing team(s) all get a morale loss
						// based on their individual Bravery & rank of unit that was killed
						int moraleLoss = (110 - (*j)->getBaseStats()->bravery) / 10;
						if (moraleLoss > 0)
						{
							moraleLoss = -(moraleLoss * loss * 2 / bTeam);
							(*j)->moraleChange(moraleLoss);
						}
/*kL
						if (slayer
							&& slayer->getFaction() == FACTION_PLAYER
							&& victim->getFaction() == FACTION_HOSTILE)
						{
							slayer->setTurnsExposed(0); // interesting
							//Log(LOG_INFO) << ". . . . slayer Exposed";
						} */
					}
					else if ((((*j)->getOriginalFaction() == FACTION_PLAYER
								|| (*j)->getOriginalFaction() == FACTION_NEUTRAL)
							&& victim->getOriginalFaction() == FACTION_HOSTILE)
						|| ((*j)->getOriginalFaction() == FACTION_HOSTILE
							&& (victim->getOriginalFaction() == FACTION_PLAYER
								|| victim->getOriginalFaction() == FACTION_NEUTRAL)))
					{
						// winning faction(s) all get a morale boost unaffected by rank of the dead unit
						(*j)->moraleChange(aTeam / 10);
					}
				}
			}
//			}

			if (weapon != NULL)
				statePushNext(new UnitDieBState( // kL_note: This is where units get sent to DEATH!
											this,
											*i,
											weapon->getRules()->getDamageType()));
			else // hidden or terrain explosion, or death by fatal wounds
			{
				if (hidden == true) // this is instant death from UFO powersources, without screaming sounds
				{
					statePushNext(new UnitDieBState(
												this,
												*i,
												DT_HE,
												true));
				}
				else
				{
					if (terrain == true)
						statePushNext(new UnitDieBState(
													this,
													*i,
													DT_HE));
					else // no slayer, and no terrain explosion, must be fatal wounds
						statePushNext(new UnitDieBState(
													this,
													*i,
													DT_NONE)); // DT_NONE = STR_HAS_DIED_FROM_A_FATAL_WOUND
				}
			}
		}
		else if (stun == true
			&& victim->getStatus() != STATUS_DEAD
			&& victim->getStatus() != STATUS_UNCONSCIOUS
			&& victim->getStatus() != STATUS_COLLAPSING	// kL_note: is this really needed ....
			&& victim->getStatus() != STATUS_TURNING	// kL_note: may be set by UnitDieBState cTor
			&& victim->getStatus() != STATUS_DISABLED)	// kL
		{
			(*i)->setStatus(STATUS_DISABLED); // kL

			if (slayer != NULL
				&& slayer->getGeoscapeSoldier() != NULL)
			{
				slayer->getStatistics()->kills.push_back(new BattleUnitKills(
																		killStatRank,
																		killStatRace,
																		killStatWeapon,
																		killStatWeaponAmmo,
																		victim->getFaction(),
																		STATUS_UNCONSCIOUS,
																		killStatMission,
																		killStatTurn,
																		killStatPoints));
			}

			if (victim != NULL
				&& victim->getGeoscapeSoldier() != NULL)
			{
				victim->getStatistics()->wasUnconscious = true;
			}

			statePushNext(new UnitDieBState( // kL_note: This is where units get set to STUNNED
										this,
										*i,
										DT_STUN,
										true));
		}
	}

	if (hidden == false // showPsiButton() ought already be false at mission start.
		&& _save->getSide() == FACTION_PLAYER)
	{
		const BattleUnit* const unit = _save->getSelectedUnit();
		if (unit != NULL)
			_parentState->showPsiButton(
									unit->getOriginalFaction() == FACTION_HOSTILE
									&& unit->getBaseStats()->psiSkill > 0
									&& unit->isOut(true, true) == false);
	}
}

/**
 * Shows the infoboxes in the queue if any.
 */
void BattlescapeGame::showInfoBoxQueue()
{
	for (std::vector<InfoboxOKState*>::const_iterator
			i = _infoboxQueue.begin();
			i != _infoboxQueue.end();
			++i)
	{
		_parentState->getGame()->pushState(*i);
	}

	_infoboxQueue.clear();
}

/**
 * Handles the result of non target actions
 * like priming a grenade or performing a melee attack.
 */
void BattlescapeGame::handleNonTargetAction()
{
	if (_currentAction.targeting == false)
	{
		_currentAction.cameraPosition = Position(0,0,-1);

		// NOTE: These actions are done partly in ActionMenuState::btnActionMenuClick() and
		// this subsequently handles a greater or lesser proportion of the resultant niceties.
 		if (_currentAction.type == BA_PRIME
			|| _currentAction.type == BA_DEFUSE)
		{
			if (_currentAction.actor->spendTimeUnits(_currentAction.TU))
			{
				_currentAction.weapon->setFuseTimer(_currentAction.value);

				if (_currentAction.value == -1)
					_parentState->warning("STR_GRENADE_IS_DEACTIVATED");
				else if (_currentAction.value == 0)
					_parentState->warning("STR_GRENADE_IS_ACTIVATED");
				else
					_parentState->warning(
										"STR_GRENADE_IS_ACTIVATED_",
										true,
										_currentAction.value);
			}
			else
				_parentState->warning("STR_NOT_ENOUGH_TIME_UNITS");
		}
		else if (_currentAction.type == BA_USE
			|| _currentAction.type == BA_LAUNCH)
		{
			if (_currentAction.result.empty() == false)
			{
				_parentState->warning(_currentAction.result);
				_currentAction.result.clear();
			}

			_save->reviveUnconsciousUnits();
		}
		else if (_currentAction.type == BA_HIT)
		{
			if (_currentAction.result.empty() == false)
			{
				_parentState->warning(_currentAction.result);
				_currentAction.result.clear();
			}
			else
			{
				if (_currentAction.actor->spendTimeUnits(_currentAction.TU))
				{
					statePushBack(new ProjectileFlyBState(
														this,
														_currentAction));
					return;
				}
				else
					_parentState->warning("STR_NOT_ENOUGH_TIME_UNITS");
			}
		}
		else if (_currentAction.type == BA_DROP)
		{
			if (_currentAction.result.empty() == false)
			{
				_parentState->warning(_currentAction.result); // "STR_NOT_ENOUGH_TIME_UNITS"
				_currentAction.result.clear();
			}
			else
			{
				if (_currentAction.actor->getPosition().z > 0)
					_save->getTileEngine()->applyGravity(_currentAction.actor->getTile());

				getResourcePack()->getSoundByDepth(
												getDepth(),
												ResourcePack::ITEM_DROP)
											->play(
												-1,
												getMap()->getSoundAngle(_currentAction.actor->getPosition()));
			}
		}

		_currentAction.type = BA_NONE;
		_parentState->updateSoldierInfo();
	}

	setupCursor();
}

/**
 * Sets the cursor according to the selected action.
 */
void BattlescapeGame::setupCursor()
{
	getMap()->refreshSelectorPosition();

	if (_currentAction.targeting == true)
	{
		if (_currentAction.type == BA_THROW)
			getMap()->setCursorType(CT_THROW);
		else if (_currentAction.type == BA_MINDCONTROL
			|| _currentAction.type == BA_PANIC
			|| _currentAction.type == BA_USE)
		{
			getMap()->setCursorType(CT_PSI);
		}
		else if (_currentAction.type == BA_LAUNCH)
			getMap()->setCursorType(CT_WAYPOINT);
		else
			getMap()->setCursorType(CT_AIM);
	}
	else
	{
		_currentAction.actor = _save->getSelectedUnit();

		if (_currentAction.actor != NULL)
			getMap()->setCursorType(
								CT_NORMAL,
								_currentAction.actor->getArmor()->getSize());
		else
			getMap()->setCursorType(
								CT_NORMAL);
	}
}

/**
 * Determines whether a playable unit is selected. Normally only player side
 * units can be selected, but in debug mode one can play with aliens too :)
 * Is used to see if stats can be displayed and action buttons will work.
 * @return, True if a playable unit is selected.
 */
bool BattlescapeGame::playableUnitSelected()
{
	return _save->getSelectedUnit() != NULL
		&& (_save->getSide() == FACTION_PLAYER
			|| _save->getDebugMode() == true);
}

/**
 * Gives time slice to the front state.
 */
void BattlescapeGame::handleState()
{
	if (_states.empty() == false)
	{
		if (_states.front() == NULL) // possible End Turn request
		{
			_states.pop_front();
			endTurnPhase();

			return;
		}

		_states.front()->think();

		getMap()->draw(); // kL, old code!! Less clunky when scrolling the battlemap.
//		getMap()->invalidate(); // redraw map
	}
}

/**
 * Pushes a state to the front of the queue and starts it.
 * @param battleState - pointer to BattleState
 */
void BattlescapeGame::statePushFront(BattleState* const battleState)
{
	_states.push_front(battleState);
	battleState->init();
}

/**
 * Pushes a state as the next state after the current one.
 * @param battleState - pointer to BattleState
 */
void BattlescapeGame::statePushNext(BattleState* const battleState)
{
	if (_states.empty() == true)
	{
		_states.push_front(battleState);
		battleState->init();
	}
	else
		_states.insert(
					++_states.begin(),
					battleState);
}

/**
 * Pushes a state to the back.
 * @param battleState - pointer to BattleState
 */
void BattlescapeGame::statePushBack(BattleState* const battleState)
{
	if (_states.empty() == true)
	{
		_states.push_front(battleState);

		if (_states.front() == NULL) // possible End Turn request
		{
			_states.pop_front();
			endTurnPhase();
		}
		else
			battleState->init();
	}
	else
		_states.push_back(battleState);
}

/**
 * Removes the current state.
 * This is a very important function. It is called by a BattleState (walking,
 * projectile is flying, explosions, etc.) at the moment that state has
 * finished the current BattleAction. Here we check the result of that
 * BattleAction and do all the aftermath. The state is popped off the list.
 */
void BattlescapeGame::popState()
{
	//Log(LOG_INFO) << "BattlescapeGame::popState()";
//	if (Options::traceAI)
//	{
//		Log(LOG_INFO) << "BattlescapeGame::popState() #" << _AIActionCounter << " with "
//			<< (_save->getSelectedUnit()? _save->getSelectedUnit()->getTimeUnits(): -9999) << " TU";
//	}

	if (_states.empty() == true)
	{
		//Log(LOG_INFO) << ". states.Empty -> return";
		return;
	}

	bool actionFailed = false;

	const BattleAction action = _states.front()->getAction();
	if (action.actor != NULL
		&& action.actor->getFaction() == FACTION_PLAYER
		&& action.result.length() > 0 // kL_note: This queries the warning string message.
		&& _playerPanicHandled == true
		&& (_save->getSide() == FACTION_PLAYER
			|| _debugPlay == true))
	{
		//Log(LOG_INFO) << ". actionFailed";
		actionFailed = true;
		_parentState->warning(action.result);

		// kL_begin: BattlescapeGame::popState(), remove action.Cursor if not enough tu's (ie, error.Message)
		if (action.result.compare("STR_NOT_ENOUGH_TIME_UNITS") == 0
			|| action.result.compare("STR_NO_AMMUNITION_LOADED") == 0
			|| action.result.compare("STR_NO_ROUNDS_LEFT") == 0)
		{
			switch (action.type)
			{
//				case BA_LAUNCH:
//					_currentAction.waypoints.clear();
				case BA_THROW:
				case BA_SNAPSHOT:
				case BA_AIMEDSHOT:
				case BA_AUTOSHOT:
				case BA_PANIC:
				case BA_MINDCONTROL:
					cancelCurrentAction(true);
				break;
				case BA_USE:
					if (action.weapon->getRules()->getBattleType() == BT_MINDPROBE)
						cancelCurrentAction(true);
				break;
			}
		} // kL_end.
	}

	_deleted.push_back(_states.front());

	//Log(LOG_INFO) << ". states.Popfront";
	_states.pop_front();
	//Log(LOG_INFO) << ". states.Popfront DONE";


	// handle the end of this unit's actions
	if (action.actor != NULL
		&& noActionsPending(action.actor) == true)
	{
		//Log(LOG_INFO) << ". noActionsPending";
		if (action.actor->getFaction() == FACTION_PLAYER)
		{
			//Log(LOG_INFO) << ". actor -> Faction_Player";

			// spend TUs of "target triggered actions" (shooting, throwing) only;
			// the other actions' TUs (healing,scanning,..) are already take care of.
			// kL_note: But let's do this **before** checkReactionFire(), so aLiens
			// get a chance .....! Odd thing is, though, that throwing triggers
			// RF properly while shooting doesn't .... So, I'm going to move this
			// to primaryAction() anyways. see what happens and what don't
			//
			// Note: psi-attack, mind-probe uses this also, I think.
			// BUT, in primaryAction() below, mind-probe expends TU there, while
			// psi-attack merely checks for TU there, and shooting/throwing
			// didn't even Care about TU there until I put it in there, and took
			// it out here in order to get Reactions vs. shooting to work correctly
			// (throwing already did work to trigger reactions, somehow).
			if (action.targeting == true
				&& _save->getSelectedUnit() != NULL
				&& actionFailed == false)
			{
				//Log(LOG_INFO) << ". . ID " << action.actor->getId() << " currentTU = " << action.actor->getTimeUnits();
				action.actor->spendTimeUnits(action.TU);
					// kL_query: Does this happen **before** ReactionFire/getReactor()?
					// no. not for shooting, but for throwing it does; actually no it doesn't.
					//
					// wtf, now RF works fine. NO IT DOES NOT.
				//Log(LOG_INFO) << ". . ID " << action.actor->getId() << " currentTU = " << action.actor->getTimeUnits() << " spent TU = " << action.TU;
			}

			if (_save->getSide() == FACTION_PLAYER)
			{
				//Log(LOG_INFO) << ". side -> Faction_Player";

				// after throwing, the cursor returns to default cursor, after shooting it stays in
				// targeting mode and the player can shoot again in the same mode (autoshot/snap/aimed)
				// kL_note: unless he/she is out of tu's

				if (actionFailed == false) // kL_begin:
				{
					//Log(LOG_INFO) << ". popState -> Faction_Player | action.type = " << action.type;
					//if (action.weapon != NULL)
					//{
						//Log(LOG_INFO) << ". popState -> Faction_Player | action.weapon = " << action.weapon->getRules()->getType();
						//if (action.weapon->getAmmoItem() != NULL)
						//{
							//Log(LOG_INFO) << ". popState -> Faction_Player | ammo = " << action.weapon->getAmmoItem()->getRules()->getType();
							//Log(LOG_INFO) << ". popState -> Faction_Player | ammoQty = " << action.weapon->getAmmoItem()->getAmmoQuantity();
						//}
					//}
					const int curTU = action.actor->getTimeUnits();

					switch (action.type)
					{
						case BA_LAUNCH:
							_currentAction.waypoints.clear();
						case BA_THROW:
							cancelCurrentAction(true);
						break;
						case BA_SNAPSHOT:
							//Log(LOG_INFO) << ". SnapShot, TU percent = " << (float)action.weapon->getRules()->getTUSnap();
							if (curTU < action.actor->getActionTUs(
																BA_SNAPSHOT,
																action.weapon)
								|| (action.weapon->needsAmmo() == true
									&& (action.weapon->getAmmoItem() == NULL
										|| action.weapon->getAmmoItem()->getAmmoQuantity() == 0)))
							{
								cancelCurrentAction(true);
							}
						break;
						case BA_AUTOSHOT:
							//Log(LOG_INFO) << ". AutoShot, TU percent = " << (float)action.weapon->getRules()->getTUAuto();
							if (curTU < action.actor->getActionTUs(
																BA_AUTOSHOT,
																action.weapon)
								|| (action.weapon->needsAmmo() == true
									&& (action.weapon->getAmmoItem() == NULL
										|| action.weapon->getAmmoItem()->getAmmoQuantity() == 0)))
							{
								cancelCurrentAction(true);
							}
						break;
						case BA_AIMEDSHOT:
							//Log(LOG_INFO) << ". AimedShot, TU percent = " << (float)action.weapon->getRules()->getTUAimed();
							if (curTU < action.actor->getActionTUs(
																BA_AIMEDSHOT,
																action.weapon)
								|| (action.weapon->needsAmmo() == true
									&& (action.weapon->getAmmoItem() == NULL
										|| action.weapon->getAmmoItem()->getAmmoQuantity() == 0)))
							{
								cancelCurrentAction(true);
							}
						break;
/*						case BA_PANIC:
							if (curTU < action.actor->getActionTUs(BA_PANIC, action.weapon))
								cancelCurrentAction(true);
						break;
						case BA_MINDCONTROL:
							if (curTU < action.actor->getActionTUs(BA_MINDCONTROL, action.weapon))
								cancelCurrentAction(true);
						break; */
/*						case BA_USE:
							if (action.weapon->getRules()->getBattleType() == BT_MINDPROBE
								&& curTU < action.actor->getActionTUs(BA_MINDCONTROL, action.weapon))
								cancelCurrentAction(true);
						break; */
					}
				} // kL_end.

				// kL_note: This is done at the end of this function also, but somehow it's
				// not updating visUnit indicators when watching a unit die and expose a second
				// enemy unit behind the first. btw, calcFoV ought have been done by now ...
//				_parentState->updateSoldierInfo(); // kL
				// I put in a call in UnitDieBState ...

				setupCursor();
				_parentState->getGame()->getCursor()->setVisible();
				//Log(LOG_INFO) << ". end NOT actionFailed";
			}
		}
		else // action.actor is not FACTION_PLAYER
		{
			//Log(LOG_INFO) << ". action -> NOT Faction_Player";
			action.actor->spendTimeUnits(action.TU); // spend TUs

			if (_save->getSide() != FACTION_PLAYER
				&& _debugPlay == false)
			{
				 // AI does three things per unit, before switching to the next, or it got killed before doing the second thing
				if (_AIActionCounter > 2
					|| _save->getSelectedUnit() == NULL
					|| _save->getSelectedUnit()->isOut(true, true) == true)
				{
					if (_save->getSelectedUnit() != NULL)
					{
						_save->getSelectedUnit()->setCache(NULL);
						getMap()->cacheUnit(_save->getSelectedUnit());
					}

					_AIActionCounter = 0;

					if (_states.empty() == true
						&& _save->selectNextFactionUnit(true) == NULL)
					{
						if (_save->getDebugMode() == false)
						{
							_endTurnRequested = true;
							statePushBack(NULL); // end AI turn
						}
						else
						{
							_save->selectNextFactionUnit();
							_debugPlay = true;
						}
					}

					if (_save->getSelectedUnit() != NULL)
						getMap()->getCamera()->centerOnPosition(_save->getSelectedUnit()->getPosition());
				}
			}
			else if (_debugPlay == true)
			{
				setupCursor();
				_parentState->getGame()->getCursor()->setVisible();
			}
		}
	}

	//Log(LOG_INFO) << ". uhm yeah";

	if (_states.empty() == false)
	{
		//Log(LOG_INFO) << ". NOT states.Empty";
		if (_states.front() == NULL) // end turn request?
		{
			//Log(LOG_INFO) << ". states.front() == 0";
			while (_states.empty() == false)
			{
				//Log(LOG_INFO) << ". while (!_states.empty()";
				if (_states.front() == NULL)
					_states.pop_front();
				else
					break;
			}

			if (_states.empty() == true)
			{
				//Log(LOG_INFO) << ". endTurnPhase()";
				endTurnPhase();
				//Log(LOG_INFO) << ". endTurnPhase() DONE return";
				return;
			}
			else
			{
				//Log(LOG_INFO) << ". states.front() != 0";
				_states.push_back(NULL);
			}
		}

		//Log(LOG_INFO) << ". states.front()->init()";
		_states.front()->init(); // init the next state in queue
	}

	// the currently selected unit died or became unconscious or disappeared inexplicably
	if (_save->getSelectedUnit() == NULL
		|| _save->getSelectedUnit()->isOut(true, true) == true)
	{
		//Log(LOG_INFO) << ". unit incapacitated: cancelAction & deSelect)";
		cancelCurrentAction();

		_save->setSelectedUnit(NULL);

		if (_save->getSide() == FACTION_PLAYER) // kL
		{
			getMap()->setCursorType(CT_NORMAL);
			_parentState->getGame()->getCursor()->setVisible();
		}
	}

	if (_save->getSide() == FACTION_PLAYER) // kL
	{
		//Log(LOG_INFO) << ". updateSoldierInfo()";
		_parentState->updateSoldierInfo(); // calcFoV ought have been done by now ...
	}
	//Log(LOG_INFO) << "BattlescapeGame::popState() EXIT";
}

/**
 * Determines whether there are any actions pending for the given unit.
 * @param bu - pointer to a BattleUnit
 * @return, true if there are no actions pending
 */
bool BattlescapeGame::noActionsPending(BattleUnit* bu)
{
	if (_states.empty() == true)
		return true;

	for (std::list<BattleState*>::const_iterator
			i = _states.begin();
			i != _states.end();
			++i)
	{
		if (*i != NULL
			&& (*i)->getAction().actor == bu)
		{
			return false;
		}
	}

	return true;
}

/**
 * Sets the timer interval for think() calls of the state.
 * @param interval - interval in ms
 */
void BattlescapeGame::setStateInterval(Uint32 interval)
{
	_parentState->setStateInterval(interval);
}

/**
 * Checks against reserved time units.
 * @param bu	- pointer to a unit
 * @param tu	- # of time units to check against
 * @param test	- true to suppress error messages (default false)
 * @return, true if unit has enough time units ( go! )
 */
bool BattlescapeGame::checkReservedTU(
		BattleUnit* bu,
		int tu,
		bool test)
{
	if (bu->getFaction() != _save->getSide()
		|| _save->getSide() == FACTION_NEUTRAL)
	{
		return tu <= bu->getTimeUnits();
	}


	BattleActionType actReserved = _save->getBATReserved(); // avoid changing _batReserved here.

	if (_save->getSide() == FACTION_HOSTILE
		&& _debugPlay == false)
	{
		const AlienBAIState* const ai = dynamic_cast<AlienBAIState*>(bu->getCurrentAIState());
		if (ai != NULL)
			actReserved = ai->getReservedAIAction();

		const int extraReserve = RNG::generate(0, 10); // kL, added in below ->

		// kL_note: This could use some tweaking, for the poor aLiens:
		switch (actReserved) // aLiens reserve TUs as a percentage rather than just enough for a single action.
		{
			case BA_SNAPSHOT:
				return (tu + extraReserve + (bu->getBaseStats()->tu / 3) <= bu->getTimeUnits());		// 33%

			case BA_AUTOSHOT:
				return (tu + extraReserve + (bu->getBaseStats()->tu * 2 / 5) <= bu->getTimeUnits());	// 40%

			case BA_AIMEDSHOT:
				return (tu + extraReserve + (bu->getBaseStats()->tu / 2) <= bu->getTimeUnits());		// 50%

			default:
				return (tu <= bu->getTimeUnits()); // + extraReserve
		}
	}

	// ** Below here is for xCom soldiers exclusively ***
	// ( which i don't care about - except that this is also used for pathPreviews in Pathfinding object )

	// check TUs against slowest weapon if we have two weapons
//	const BattleItem* const weapon = bu->getMainHandWeapon(false);
	// kL: Use getActiveHand() instead, if xCom wants to reserve TU & for pathPreview.
	const BattleItem* const weapon = bu->getItem(bu->getActiveHand());

	// if reserved for Aimed shot drop to Auto shot
	if (actReserved == BA_AIMEDSHOT
		&& bu->getActionTUs(
						BA_AIMEDSHOT,
						weapon) == 0)
	{
		actReserved = BA_AUTOSHOT;
	}

	// if reserved for Auto shot drop to Snap shot
	if (actReserved == BA_AUTOSHOT
		&& bu->getActionTUs(
						BA_AUTOSHOT,
						weapon) == 0)
	{
		actReserved = BA_SNAPSHOT;
	}

	// if reserved for Snap shot try Auto shot
	if (actReserved == BA_SNAPSHOT
		&& bu->getActionTUs(
						BA_SNAPSHOT,
						weapon) == 0)
	{
		actReserved = BA_AUTOSHOT;
	}

	// if reserved for Auto shot try Aimed shot
	if (actReserved == BA_AUTOSHOT
		&& bu->getActionTUs(
						BA_AUTOSHOT,
						weapon) == 0)
	{
		actReserved = BA_AIMEDSHOT;
	}

	int tuKneel;
	if (_save->getKneelReserved() == true
		&& bu->getGeoscapeSoldier() != NULL
		&& bu->isKneeled() == false)
	{
		tuKneel = 3;
	}
	else
		tuKneel = 0;

	// if no Aimed shot is available revert to bat_NONE
	if (actReserved == BA_AIMEDSHOT
		&& bu->getActionTUs(
						BA_AIMEDSHOT,
						weapon) == 0)
	{
		if (tuKneel != 0)
			actReserved = BA_NONE;
		else
			return true;
	}

	if ((actReserved != BA_NONE
			|| _save->getKneelReserved() == true)
		&& tu + tuKneel + bu->getActionTUs(
										actReserved,
										weapon) > bu->getTimeUnits()
		&& (tuKneel + bu->getActionTUs(
									actReserved,
									weapon) <= bu->getTimeUnits()
			|| test == true))
	{
		if (test == false)
		{
			if (tuKneel != 0)
			{
				switch (actReserved)
				{
					case BA_NONE:
						_parentState->warning("STR_TIME_UNITS_RESERVED_FOR_KNEELING");
					break;

					default:
						_parentState->warning("STR_TIME_UNITS_RESERVED_FOR_KNEELING_AND_FIRING");
				}
			}
			else
			{
				switch (_save->getBATReserved())
				{
					case BA_SNAPSHOT:
						_parentState->warning("STR_TIME_UNITS_RESERVED_FOR_SNAP_SHOT");
					break;
					case BA_AUTOSHOT:
						_parentState->warning("STR_TIME_UNITS_RESERVED_FOR_AUTO_SHOT");
					break;
					case BA_AIMEDSHOT:
						_parentState->warning("STR_TIME_UNITS_RESERVED_FOR_AIMED_SHOT");
				}
			}
		}

		return false;
	}

	return true;
}

/**
 * Picks the first soldier that is panicking.
 * @return, true when all panicking is over
 */
bool BattlescapeGame::handlePanickingPlayer()
{
	for (std::vector<BattleUnit*>::const_iterator
			j = _save->getUnits()->begin();
			j != _save->getUnits()->end();
			++j)
	{
		if ((*j)->getFaction() == FACTION_PLAYER
			&& (*j)->getOriginalFaction() == FACTION_PLAYER
			&& handlePanickingUnit(*j) == true)
		{
			return false;
		}
	}

	return true;
}

/**
 * Common function for handling panicking units.
 * @return, false when unit not in panicking mode
 */
bool BattlescapeGame::handlePanickingUnit(BattleUnit* unit)
{
	const UnitStatus status = unit->getStatus();

	if (status != STATUS_PANICKING
		&& status != STATUS_BERSERK)
	{
		return false;
	}

	//Log(LOG_INFO) << "unit Panic/Berserk : " << unit->getId() << " / " << unit->getMorale();
	_save->setSelectedUnit(unit);
//	unit->setUnitVisible(); // kL

	_parentState->getMap()->setCursorType(CT_NONE);

	// show a little infobox with the name of the unit and "... is panicking"
	if (unit->getUnitVisible() == true
		|| Options::noAlienPanicMessages == false)
	{
		getMap()->getCamera()->centerOnPosition(unit->getPosition());

		Game* const game = _parentState->getGame();
		if (status == STATUS_PANICKING)
			game->pushState(new InfoboxState(game->getLanguage()->getString("STR_HAS_PANICKED", unit->getGender())
																			.arg(unit->getName(game->getLanguage()))));
		else
			game->pushState(new InfoboxState(game->getLanguage()->getString("STR_HAS_GONE_BERSERK", unit->getGender())
																			.arg(unit->getName(game->getLanguage()))));
	}

//	unit->abortTurn(); // makes the unit go to status STANDING :p
	unit->setStatus(STATUS_STANDING); // kL :P

	BattleAction ba;
	ba.actor = unit;

	switch (status)
	{
		case STATUS_PANICKING:
			if (RNG::percent(50) == true) // 50:50 freeze or flee.
			{
				BattleItem* item = unit->getItem("STR_RIGHT_HAND");
				if (item != NULL)
					dropItem(
							unit->getPosition(),
							item,
							false,
							true);

				item = unit->getItem("STR_LEFT_HAND");
				if (item != NULL)
					dropItem(
							unit->getPosition(),
							item,
							false,
							true);

				unit->setCache(NULL);

				for (int // try a few times to get a tile to run to.
						i = 0;
						i < 20;
						++i)
				{
					ba.target = Position(
									unit->getPosition().x + RNG::generate(-5,5),
									unit->getPosition().y + RNG::generate(-5,5),
									unit->getPosition().z);

					if (ba.target.z > 0
						&& i > 9)
					{
						--ba.target.z;

						if (ba.target.z > 0
							&& i > 14)
						{
							--ba.target.z;
						}
					}

					if (_save->getTile(ba.target) != NULL)
					{
						_save->getPathfinding()->calculate(
														ba.actor,
														ba.target);

						if (_save->getPathfinding()->getStartDirection() != -1)
						{
							statePushBack(new UnitWalkBState(
															this,
															ba));
							break;
						}
					}
				}
			}
		break;
		case STATUS_BERSERK:	// berserk - do some weird turning around and then aggro
								// towards an enemy unit or shoot towards random place
			ba.type = BA_TURN;
			for (int
					i = 0;
					i < 4;
					++i)
			{
				ba.target = Position(
								unit->getPosition().x + RNG::generate(-5,5),
								unit->getPosition().y + RNG::generate(-5,5),
								unit->getPosition().z);
				statePushBack(new UnitTurnBState(
												this,
												ba,
												false));
			}

			for (std::vector<BattleUnit*>::const_iterator
					j = unit->getVisibleUnits()->begin();
					j != unit->getVisibleUnits()->end();
					++j)
			{
				ba.target = (*j)->getPosition();
				statePushBack(new UnitTurnBState(
												this,
												ba,
												false));
			}

			if (_save->getTile(ba.target) != NULL)
			{
				ba.weapon = unit->getMainHandWeapon();
				if (ba.weapon == NULL)				// kL
					ba.weapon = unit->getGrenade();	// kL

				// TODO: run up to another unit and slug it with the Universal Fist.
				// Or w/ an already-equipped melee weapon

				if (ba.weapon != NULL
					&& (_save->getDepth() != 0
						|| ba.weapon->getRules()->isWaterOnly() == false))
				{
					if (ba.weapon->getRules()->getBattleType() == BT_FIREARM)
					{
						ba.type = BA_SNAPSHOT;
						const int tu = ba.actor->getActionTUs(
															ba.type,
															ba.weapon);

						for (int // fire shots until unit runs out of TUs
								i = 0;
								i < 10;
								++i)
						{
							if (ba.actor->spendTimeUnits(tu) == false)
								break;

							statePushBack(new ProjectileFlyBState(
																this,
																ba));
						}
					}
					else if (ba.weapon->getRules()->getBattleType() == BT_GRENADE)
					{
						if (ba.weapon->getFuseTimer() == -1)
							ba.weapon->setFuseTimer(0);

						ba.type = BA_THROW;
						statePushBack(new ProjectileFlyBState(
															this,
															ba));
					}
				}
			}

			unit->setTimeUnits(unit->getBaseStats()->tu); // replace the TUs from shooting
			ba.type = BA_NONE;
	}

	statePushBack(new UnitPanicBState( // Time units can only be reset after everything else occurs
									this,
									ba.actor));

	unit->moraleChange(+15);

	//Log(LOG_INFO) << "unit Panic/Berserk : " << unit->getId() << " / " << unit->getMorale();
	return true;
}

/**
  * Cancels the current action the user had selected (firing, throwing, etc).
  * @param bForce - force the action to be cancelled
  * @return, true if action was cancelled
  */
bool BattlescapeGame::cancelCurrentAction(bool bForce)
{
	const bool bPreviewed = (Options::battleNewPreviewPath != PATH_NONE);

	if (_save->getPathfinding()->removePreview() == true
		&& bPreviewed == true)
	{
		return true;
	}

	if (_states.empty() == true
		|| bForce == true)
	{
		if (_currentAction.targeting == true)
		{
			if (_currentAction.type == BA_LAUNCH
				&& _currentAction.waypoints.empty() == false)
			{
				_currentAction.waypoints.pop_back();

				if (getMap()->getWaypoints()->empty() == false)
					getMap()->getWaypoints()->pop_back();

				if (_currentAction.waypoints.empty() == true)
					_parentState->showLaunchButton(false);

				return true;
			}
			else
			{
				if (Options::battleConfirmFireMode == true
					&& _currentAction.waypoints.empty() == false)
				{
					_currentAction.waypoints.pop_back();
					getMap()->getWaypoints()->pop_back();

					return true;
				}

//				_currentAction.clearAction(); // kL
				_currentAction.targeting = false;
				_currentAction.type = BA_NONE;
//				_currentAction.TU = 0; // kL

				if (_save->getSide() == FACTION_PLAYER) // kL
				{
					setupCursor();
					_parentState->getGame()->getCursor()->setVisible();
				}

				return true;
			}
		}
	}
	else if (_states.empty() == false
		&& _states.front() != NULL)
	{
		_states.front()->cancel();
		return true;
	}

	return false;
}

/**
 * Gets a pointer to access BattleAction struct members directly.
 * @return, pointer to BattleAction
 */
BattleAction* BattlescapeGame::getCurrentAction()
{
	return &_currentAction;
}

/**
 * Determines whether an action is currently going on.
 * @return, true or false
 */
bool BattlescapeGame::isBusy() const
{
	return (_states.empty() == false);
}

/**
 * Activates primary action (left click).
 * @param targetPos - reference a Position on the map
 */
void BattlescapeGame::primaryAction(const Position& targetPos)
{
	//Log(LOG_INFO) << "BattlescapeGame::primaryAction()";
	//if (_save->getSelectedUnit()) Log(LOG_INFO) << ". ID " << _save->getSelectedUnit()->getId();
	bool bPreviewed = (Options::battleNewPreviewPath != PATH_NONE);

	_currentAction.actor = _save->getSelectedUnit();
	BattleUnit* const targetUnit = _save->selectUnit(targetPos);

	if (_currentAction.actor != NULL
		&& _currentAction.targeting == true)
	{
		//Log(LOG_INFO) << ". . _currentAction.targeting";
		_currentAction.strafe = false;

		if (_currentAction.type == BA_LAUNCH // click to set BL waypoints.
			&& static_cast<int>(_currentAction.waypoints.size()) < _currentAction.weapon->getRules()->isWaypoints())
		{
			//Log(LOG_INFO) << ". . . . BA_LAUNCH";
			_parentState->showLaunchButton();
			_currentAction.waypoints.push_back(targetPos);
			getMap()->getWaypoints()->push_back(targetPos);
		}
		else if (_currentAction.type == BA_USE
			&& _currentAction.weapon->getRules()->getBattleType() == BT_MINDPROBE)
		{
			//Log(LOG_INFO) << ". . . . BA_USE -> BT_MINDPROBE";
			if (targetUnit != NULL
				&& targetUnit->getFaction() != _currentAction.actor->getFaction()
				&& targetUnit->getUnitVisible() == true)
			{
				if (_currentAction.weapon->getRules()->isLOSRequired() == false
					|| std::find(
							_currentAction.actor->getVisibleUnits()->begin(),
							_currentAction.actor->getVisibleUnits()->end(),
							targetUnit) != _currentAction.actor->getVisibleUnits()->end())
				{
					if (getTileEngine()->distance( // in Range
											_currentAction.actor->getPosition(),
											_currentAction.target) <= _currentAction.weapon->getRules()->getMaxRange())
					{
						if (_currentAction.actor->spendTimeUnits(_currentAction.TU) == true)
						{
							_parentState->getGame()->getResourcePack()->getSoundByDepth(
																					_save->getDepth(),
																					_currentAction.weapon->getRules()->getHitSound())
																				->play(
																					-1,
																					getMap()->getSoundAngle(targetPos));

							_parentState->getGame()->pushState(new UnitInfoState(
																			targetUnit,
																			_parentState,
																			false,
																			true));
						}
						else
						{
							cancelCurrentAction();
							_parentState->warning("STR_NOT_ENOUGH_TIME_UNITS");
						}
					}
					else
						_parentState->warning("STR_OUT_OF_RANGE");
				}
				else
					_parentState->warning("STR_NO_LINE_OF_FIRE");

				cancelCurrentAction();
			}
		}
		else if (_currentAction.type == BA_PANIC
			|| _currentAction.type == BA_MINDCONTROL)
		{
			//Log(LOG_INFO) << ". . . . BA_PANIC or BA_MINDCONTROL";
			if (targetUnit != NULL
				&& targetUnit->getFaction() != _currentAction.actor->getFaction()
				&& targetUnit->getUnitVisible() == true)
			{
				bool aLienPsi = (_currentAction.weapon == NULL);
				if (aLienPsi == true)
					_currentAction.weapon = _alienPsi;

				_currentAction.target = targetPos;
				_currentAction.TU = _currentAction.actor->getActionTUs(
																	_currentAction.type,
																	_currentAction.weapon);

				if (_currentAction.weapon->getRules()->isLOSRequired() == false
					|| std::find(
							_currentAction.actor->getVisibleUnits()->begin(),
							_currentAction.actor->getVisibleUnits()->end(),
							targetUnit) != _currentAction.actor->getVisibleUnits()->end())
				{
					if (getTileEngine()->distance( // in Range
											_currentAction.actor->getPosition(),
											_currentAction.target) <= _currentAction.weapon->getRules()->getMaxRange())
					{
						// get the sound/animation started
//						getMap()->setCursorType(CT_NONE);
//						_parentState->getGame()->getCursor()->setVisible(false);
//						_currentAction.cameraPosition = getMap()->getCamera()->getMapOffset();
						_currentAction.cameraPosition = Position(0, 0,-1); // kL (don't adjust the camera when coming out of ProjectileFlyB/ExplosionB states)


						//Log(LOG_INFO) << ". . . . . . new ProjectileFlyBState";
						statePushBack(new ProjectileFlyBState(
															this,
															_currentAction));

						if (_currentAction.actor->getTimeUnits() >= _currentAction.TU) // kL_note: WAIT, check this *before* all the stuff above!!!
						{
							if (getTileEngine()->psiAttack(&_currentAction) == true)
							{
								//Log(LOG_INFO) << ". . . . . . Psi successful";
								Game* const game = _parentState->getGame(); // show a little infobox if it's successful
								if (_currentAction.type == BA_PANIC)
								{
									//Log(LOG_INFO) << ". . . . . . . . BA_Panic";
									game->pushState(new InfoboxState(game->getLanguage()->getString("STR_MORALE_ATTACK_SUCCESSFUL")));
								}
								else // BA_MINDCONTROL
								{
									//Log(LOG_INFO) << ". . . . . . . . BA_MindControl";
									game->pushState(new InfoboxState(game->getLanguage()->getString("STR_MIND_CONTROL_SUCCESSFUL")));
								}

								//Log(LOG_INFO) << ". . . . . . updateSoldierInfo()";
								_parentState->updateSoldierInfo(false);
								//Log(LOG_INFO) << ". . . . . . updateSoldierInfo() DONE";


								// kL_begin: BattlescapeGame::primaryAction(), not sure where this bit came from.....
								// it doesn't seem to be in the official oXc but it might
								// stop some (psi-related) crashes i'm getting; (no, it was something else..),
								// but then it probably never runs because I doubt that selectedUnit can be other than xCom.
								// (yes, selectedUnit is currently operating unit of *any* faction)
								// BUT -> primaryAction() here is never called by the AI; only by Faction_Player ...
								// BUT <- it has to be, because this is how aLiens do their 'builtInPsi'
								//
								// ... it's for player using aLien to do Psi ...
								//
								// I could do a test Lol

//								if (_save->getSelectedUnit()->getFaction() != FACTION_PLAYER)
//								{
//								_currentAction.targeting = false;
//								_currentAction.type = BA_NONE;
//								}
//								setupCursor();

//								getMap()->setCursorType(CT_NONE);							// kL
//								_parentState->getGame()->getCursor()->setVisible(false);	// kL
//								_parentState->getMap()->refreshSelectorPosition();			// kL
								// kL_end.

								//Log(LOG_INFO) << ". . . . . . inVisible cursor, DONE";
							}
						}
						else
						{
							cancelCurrentAction();
							_parentState->warning("STR_NOT_ENOUGH_TIME_UNITS");
						}
					}
					else
						_parentState->warning("STR_OUT_OF_RANGE");
				}
				else
					_parentState->warning("STR_NO_LINE_OF_FIRE");


				if (aLienPsi == true)
					_currentAction.weapon = NULL;
			}
		}
		else if (Options::battleConfirmFireMode == true
			&& (_currentAction.waypoints.empty() == true
				|| targetPos != _currentAction.waypoints.front()))
		{
			_currentAction.waypoints.clear();
			_currentAction.waypoints.push_back(targetPos);

			getMap()->getWaypoints()->clear();
			getMap()->getWaypoints()->push_back(targetPos);
		}
		else
		{
			//Log(LOG_INFO) << ". . . . FIRING or THROWING";
			getMap()->setCursorType(CT_NONE);

			if (Options::battleConfirmFireMode == true)
			{
				_currentAction.waypoints.clear();
				getMap()->getWaypoints()->clear();
			}

			_parentState->getGame()->getCursor()->setVisible(false);

			_currentAction.target = targetPos;
			_currentAction.cameraPosition = getMap()->getCamera()->getMapOffset();
			//Log(LOG_INFO) << "BattlescapeGame::primaryAction() setting cameraPosition to mapOffset";

			_states.push_back(new ProjectileFlyBState(
													this,
													_currentAction));

			statePushFront(new UnitTurnBState( // first of all turn towards the target
											this,
											_currentAction));
//			}
			// kL_note: else give message Out-of-TUs, or this must be done elsewhere already...
			// probably in ProjectileFlyBState... great. not.
			// I imagine updateSoldierInfo() is done somewhere down the line....... It's done at the end of popState()
		}
	}
	else // unit MOVE .......
	{
		//Log(LOG_INFO) << ". . NOT _currentAction.targeting";
		if (targetUnit != NULL
			&& targetUnit != _currentAction.actor
			&& (targetUnit->getUnitVisible() == true
				|| _debugPlay == true))
		{
			if (targetUnit->getFaction() == _save->getSide())
			{
				_save->setSelectedUnit(targetUnit);
				_parentState->updateSoldierInfo();

				cancelCurrentAction();
				setupCursor();

				_currentAction.actor = targetUnit; // select unit
			}
		}
		else if (playableUnitSelected() == true)
		{
			Pathfinding* const pf = _save->getPathfinding();
			const bool
				mod_CTRL = (SDL_GetModState() & KMOD_CTRL) != 0,
				mod_ALT = (SDL_GetModState() & KMOD_ALT) != 0,
				isTank = _currentAction.actor->getUnitRules() != NULL
					  && _currentAction.actor->getUnitRules()->isMechanical();

			_currentAction.dash = false;
			_currentAction.actor->setDashing(false);

			if (targetUnit != NULL // spin 180 degrees
				&& targetUnit == _currentAction.actor
				&& mod_CTRL == false
				&& mod_ALT == false
				&& isTank == false
				&& _currentAction.actor->getArmor()->getSize() == 1) // small units only
			{
				if (bPreviewed == true)
					pf->removePreview();

				Position screenPixel;
				getMap()->getCamera()->convertMapToScreen(
														targetPos,
														&screenPixel);
				screenPixel += getMap()->getCamera()->getMapOffset();

				Position mousePixel;
				getMap()->findMousePosition(mousePixel);

				if (mousePixel.x > screenPixel.x + 16)
					targetUnit->setTurnDirection(-1);
				else
					targetUnit->setTurnDirection(1);

				Pathfinding::directionToVector(
											(targetUnit->getDirection() + 4) %8,
											&_currentAction.target);
				_currentAction.target += targetPos;
				_currentAction.strafe = false;

				statePushBack(new UnitTurnBState(
											this,
											_currentAction));
			}
			else
			{
				if (bPreviewed == true
					&& (_currentAction.target != targetPos
						|| pf->isModCTRL() != mod_CTRL
						|| pf->isModALT() != mod_ALT))
				{
					pf->removePreview();
				}

				_currentAction.strafe = (Options::strafe == true)
									&& ((mod_CTRL == true		// soldier strafe
											&& isTank == false)
										|| (mod_ALT == true		// tank reverse gear, 1 tile only
											&& isTank == true));
				//Log(LOG_INFO) << ". primary action: Strafe";

				const Position
					actorPos = _currentAction.actor->getPosition(),
					pos = Position(
								targetPos.x - actorPos.x,
								targetPos.y - actorPos.y,
								0);
				const int
					dist = _save->getTileEngine()->distance(
														actorPos,
														targetPos),
					dirUnit = _currentAction.actor->getDirection();
				int dir;
				pf->vectorToDirection(pos, dir);

				if (_currentAction.strafe == true
					&& _currentAction.actor->getGeoscapeSoldier() != NULL
					&& (actorPos.z != targetPos.z
						|| dist > 1
						|| (actorPos.z == targetPos.z
							&& dist < 2
							&& dirUnit == dir)))
				{
					_currentAction.strafe = false;
					_currentAction.dash = true;
					_currentAction.actor->setDashing(true);
				}

				_currentAction.target = targetPos;
				pf->calculate( // GET the Path.
							_currentAction.actor,
							_currentAction.target);

				// assumes both previewPath() & removePreview() don't change StartDirection
				if (pf->getStartDirection() != -1)
				{
					if (bPreviewed == true
						&& pf->previewPath() == false)
//						&& pf->getStartDirection() != -1)
					{
						pf->removePreview();
						bPreviewed = false;
					}

					if (bPreviewed == false) // -= start walking =- //
//						&& pf->getStartDirection() != -1)
					{
						getMap()->setCursorType(CT_NONE);
						_parentState->getGame()->getCursor()->setVisible(false);

						statePushBack(new UnitWalkBState(
														this,
														_currentAction));
					}
				}
			}
		}
	}
	//Log(LOG_INFO) << "BattlescapeGame::primaryAction() EXIT";
}

/**
 * Activates secondary action (right click).
 * @param posTarget - reference a Position on the map
 */
void BattlescapeGame::secondaryAction(const Position& posTarget)
{
	//Log(LOG_INFO) << "BattlescapeGame::secondaryAction()";
	_currentAction.actor = _save->getSelectedUnit();

	if (_currentAction.actor->getPosition() == posTarget)
	{
		// could put just about anything in here Orelly.
		_parentState->btnKneelClick(NULL);
		return;
	}

	// -= turn to or open door =-
	_currentAction.target = posTarget;
	_currentAction.strafe = Options::strafe
						 && (SDL_GetModState() & KMOD_CTRL) != 0
						 && _currentAction.actor->getTurretType() > -1;

	statePushBack(new UnitTurnBState(
									this,
									_currentAction));
}

/**
 * Handler for the blaster launcher button.
 */
void BattlescapeGame::launchAction()
{
	//Log(LOG_INFO) << "BattlescapeGame::launchAction()";
	_parentState->showLaunchButton(false);

	getMap()->getWaypoints()->clear();
	_currentAction.target = _currentAction.waypoints.front();

	getMap()->setCursorType(CT_NONE);
	_parentState->getGame()->getCursor()->setVisible(false);

	_currentAction.cameraPosition = getMap()->getCamera()->getMapOffset();

	_states.push_back(new ProjectileFlyBState(
											this,
											_currentAction));

	statePushFront(new UnitTurnBState(
									this,
									_currentAction)); // first of all turn towards the target
	//Log(LOG_INFO) << "BattlescapeGame::launchAction() EXIT";
}

/**
 * Handler for the psi button.
 */
void BattlescapeGame::psiButtonAction()
{
	//Log(LOG_INFO) << "BattlescapeGame::psiButtonAction()";
	_currentAction.weapon = 0;
	_currentAction.targeting = true;
	_currentAction.type = BA_PANIC;
	_currentAction.TU = 25;	// kL_note: this is just a default i guess;
							// otherwise it should be getActionTUs().
							// It gets overridden later ...
	setupCursor();
}


/**
 * Moves a unit up or down.
 * @param unit	- a unit
 * @param dir	- direction DIR_UP or DIR_DOWN
 */
void BattlescapeGame::moveUpDown(
		BattleUnit* unit,
		int dir)
{
	_currentAction.target = unit->getPosition();

	if (dir == Pathfinding::DIR_UP)
	{
		if ((SDL_GetModState() & KMOD_ALT) != 0)
			return;

		++_currentAction.target.z;
	}
	else
		--_currentAction.target.z;

	getMap()->setCursorType(CT_NONE);
	_parentState->getGame()->getCursor()->setVisible(false);

	_currentAction.strafe = false; // redundancy checks ......
	_currentAction.dash = false;
	_currentAction.actor->setDashing(false);

	if (Options::strafe == true
		&& (SDL_GetModState() & KMOD_CTRL) != 0
		&& unit->getGeoscapeSoldier() != NULL)
	{
		_currentAction.dash = true;
		_currentAction.actor->setDashing(true);
	}

	_save->getPathfinding()->calculate(
									_currentAction.actor,
									_currentAction.target);

	statePushBack(new UnitWalkBState(
									this,
									_currentAction));
}

/**
 * Requests the end of the turn (waits for explosions etc to really end the turn).
 */
void BattlescapeGame::requestEndTurn()
{
	cancelCurrentAction();

	if (_endTurnRequested == false)
	{
		_endTurnRequested = true;
		statePushBack(NULL);
	}
}

/**
 * Drops an item to the floor and affects it with gravity.
 * @param position		- reference position to place the item
 * @param item			- pointer to the item
 * @param newItem		- true if this is a new item
 * @param removeItem	- true to remove the item from the owner
 */
void BattlescapeGame::dropItem(
		const Position& position,
		BattleItem* item,
		bool newItem,
		bool removeItem)
{
	if (_save->getTile(position) == NULL		// don't spawn anything outside of bounds
		|| item->getRules()->isFixed() == true)	// don't ever drop fixed items
	{
		return;
	}

	_save->getTile(position)->addItem(
								item,
								getRuleset()->getInventory("STR_GROUND"));

	if (item->getUnit() != NULL)
		item->getUnit()->setPosition(position);

	if (newItem == true)
		_save->getItems()->push_back(item);
//	else if (_save->getSide() != FACTION_PLAYER)
//		item->setTurnFlag(true);

	if (removeItem == true)
		item->moveToOwner(NULL);
	else if (item->getRules()->getBattleType() != BT_GRENADE
		&& item->getRules()->getBattleType() != BT_PROXIMITYGRENADE)
	{
		item->setOwner(NULL);
	}

	getTileEngine()->applyGravity(_save->getTile(position));

	if (item->getRules()->getBattleType() == BT_FLARE)
	{
		getTileEngine()->calculateTerrainLighting();
		getTileEngine()->calculateFOV(position);
	}
}

/**
 * Converts a unit into a unit of another type.
 * @param unit			- pointer to a unit to convert
 * @param convertType	- reference the type of unit to convert to
 * @param dirFace		- the direction to face after converting (default 3)
 * @return, pointer to the new unit
 */
BattleUnit* BattlescapeGame::convertUnit(
		BattleUnit* unit,
		const std::string& convertType,
		int dirFace) // kL_add.
{
	//Log(LOG_INFO) << "BattlescapeGame::convertUnit() " << convertType;
	const bool visible = unit->getUnitVisible();

	_save->getBattleState()->showPsiButton(false);
	_save->removeUnconsciousBodyItem(unit); // in case the unit was unconscious

	unit->instaKill();
	unit->setSpecialAbility(SPECAB_NONE);	// kL
	unit->setRespawn(false);				// kL

	if (Options::battleNotifyDeath == true
		&& unit->getFaction() == FACTION_PLAYER
		&& unit->getOriginalFaction() == FACTION_PLAYER)
	{
		Language* const lang = _parentState->getGame()->getLanguage();
		_parentState->getGame()->pushState(new InfoboxState(lang->getString(
																		"STR_HAS_BEEN_KILLED",
																		unit->getGender())
																	.arg(unit->getName(lang))));
	}

	for (std::vector<BattleItem*>::const_iterator
			i = unit->getInventory()->begin();
			i != unit->getInventory()->end();
			++i)
	{
		dropItem(
				unit->getPosition(),
				*i);
		(*i)->setOwner(NULL);
	}

	unit->getInventory()->clear();

	// remove unit-tile link
	unit->setTile(NULL);
	_save->getTile(unit->getPosition())->setUnit(NULL);


	std::ostringstream newArmor;
	newArmor << getRuleset()->getUnit(convertType)->getArmor();

	const int
		difficulty = static_cast<int>(_parentState->getGame()->getSavedGame()->getDifficulty()),
		month = _parentState->getGame()->getSavedGame()->getMonthsPassed();

	BattleUnit* const convertedUnit = new BattleUnit(
												getRuleset()->getUnit(convertType),
												FACTION_HOSTILE,
												_save->getUnits()->back()->getId() + 1,
												getRuleset()->getArmor(newArmor.str()),
												difficulty,
												getDepth(),
												month,
												this);
	// note: what about setting _zombieUnit=true? It's not generic but it's the only case, afaict

//	if (!difficulty) // kL_note: moved to BattleUnit::adjustStats()
//		convertedUnit->halveArmor();

	_save->getTile(unit->getPosition())->setUnit(
											convertedUnit,
											_save->getTile(unit->getPosition() + Position(0,0,-1)));
	convertedUnit->setPosition(unit->getPosition());

	convertedUnit->setTimeUnits(0);

	if (convertType == "STR_ZOMBIE")
		dirFace = RNG::generate(0,7);
	convertedUnit->setDirection(dirFace);

	_save->getUnits()->push_back(convertedUnit);

	convertedUnit->setCache(NULL);
	getMap()->cacheUnit(convertedUnit);

	convertedUnit->setAIState(new AlienBAIState(
											_save,
											convertedUnit,
											NULL));

	std::string terroristWeapon = getRuleset()->getUnit(convertType)->getRace().substr(4);
	terroristWeapon += "_WEAPON";
	BattleItem* const item = new BattleItem(
										getRuleset()->getItem(terroristWeapon),
										_save->getCurrentItemId());
	item->moveToOwner(convertedUnit);
	item->setSlot(getRuleset()->getInventory("STR_RIGHT_HAND"));
	_save->getItems()->push_back(item);

	getTileEngine()->applyGravity(convertedUnit->getTile());
	getTileEngine()->calculateFOV(convertedUnit->getPosition());

	convertedUnit->setUnitVisible(visible);
//	convertedUnit->getCurrentAIState()->think();

	return convertedUnit;
}

/**
 * Gets the map.
 * @return, pointer to Map
 */
Map* BattlescapeGame::getMap() const
{
	return _parentState->getMap();
}

/**
 * Gets the battle game save data object.
 * @return, pointer to SavedBattleGame
 */
SavedBattleGame* BattlescapeGame::getSave() const
{
	return _save;
}

/**
 * Gets the tilengine.
 * @return, pointer to TileEngine
 */
TileEngine* BattlescapeGame::getTileEngine() const
{
	return _save->getTileEngine();
}

/**
 * Gets the pathfinding.
 * @return, pointer to Pathfinding
 */
Pathfinding* BattlescapeGame::getPathfinding() const
{
	return _save->getPathfinding();
}

/**
 * Gets the resourcepack.
 * @return, pointer to ResourcePack
 */
ResourcePack* BattlescapeGame::getResourcePack() const
{
	return _parentState->getGame()->getResourcePack();
}

/**
 * Gets the ruleset.
 * @return, pointer to Ruleset
 */
const Ruleset* const BattlescapeGame::getRuleset() const
{
	return _parentState->getGame()->getRuleset();
}

/**
 * Tries to find an item and pick it up if possible.
 * @param action - pointer to the current BattleAction struct
 */
void BattlescapeGame::findItem(BattleAction* action)
{
	//Log(LOG_INFO) << "BattlescapeGame::findItem()";
	if (action->actor->getRankString() != "STR_LIVE_TERRORIST")							// terrorists don't have hands.
	{
		BattleItem* targetItem = surveyItems(action);									// pick the best available item
		if (targetItem
			&& worthTaking(targetItem, action))											// make sure it's worth taking
																						// haha, this always evaluates FALSE!!!
		{
			if (targetItem->getTile()->getPosition() == action->actor->getPosition())	// if we're already standing on it...
			{
				if (takeItemFromGround(targetItem, action) == 0)						// try to pick it up
				{
					if (targetItem->getAmmoItem() == NULL)								// if it isn't loaded or it is ammo
						action->actor->checkAmmo();										// try to load our weapon
				}
			}
			else if (targetItem->getTile()->getUnit() == NULL							// if nobody's standing on it
				|| targetItem->getTile()->getUnit()->isOut(true, true))						// we should try to get to it.
			{
				action->target = targetItem->getTile()->getPosition();
				action->type = BA_WALK;
			}
		}
	}
}

/**
 * Searches through items on the map that were dropped
 * on an alien turn, then picks the most "attractive" one.
 * @param action - pointer to the current BattleAction struct
 * @return, the BattleItem to go for
 */
BattleItem* BattlescapeGame::surveyItems(BattleAction* action)
{
	//Log(LOG_INFO) << "BattlescapeGame::surveyItems()";
	std::vector<BattleItem*> droppedItems;

	// first fill a vector with items on the ground
	// that were dropped on the alien turn [not]
	// and have an attraction value.
	// kL_note: And no one else is standing on it.
	for (std::vector<BattleItem*>::const_iterator
			i = _save->getItems()->begin();
			i != _save->getItems()->end();
			++i)
	{
		if ((*i)->getSlot()
			&& (*i)->getSlot()->getId() == "STR_GROUND"
			&& (*i)->getTile()
			&& ((*i)->getTile()->getUnit() == NULL				// kL
				|| (*i)->getTile()->getUnit() == action->actor)	// kL
//kL		&& (*i)->getTurnFlag()
			&& (*i)->getRules()->getAttraction())
		{
			droppedItems.push_back(*i);
		}
	}

	BattleItem* item = NULL;
	int
		worth = 0,
		test;

	// now select the most suitable candidate depending on attractiveness and distance
	for (std::vector<BattleItem*>::const_iterator
			i = droppedItems.begin();
			i != droppedItems.end();
			++i)
	{
		test = (*i)->getRules()->getAttraction()
				/ ((_save->getTileEngine()->distance(
												action->actor->getPosition(),
												(*i)->getTile()->getPosition()) * 2) + 1);
		if (test > worth)
		{
			worth = test;
			item = *i;
		}
	}

	return item;
}

/**
 * Assesses whether this item is worth trying to pick up, taking into account
 * how many units we see, whether or not the Weapon has ammo, and if we have
 * ammo FOR it, or if it's ammo, checks if we have the weapon to go with it;
 * assesses the attraction value of the item and compares it with the distance
 * to the object, then returns false anyway.
 * @param item		- pointer to a BattleItem to go for
 * @param action	- pointer to the current BattleAction struct
 * @return, true if the item is worth going for
 */
bool BattlescapeGame::worthTaking(
		BattleItem* item,
		BattleAction* action)
{
	//Log(LOG_INFO) << "BattlescapeGame::worthTaking()";
	int worth = item->getRules()->getAttraction();
	if (worth == 0)
		return false;

	// don't even think about making a move for that gun if you can see a target, for some reason
	// (maybe this should check for enemies spotting the tile the item is on?)
//kL	if (action->actor->getVisibleUnits()->empty()) // kL_note: this also appears in HandleAI() above.
//	{
		// retrieve an insignificantly low value from the ruleset.
//kL, above,		worth = item->getRules()->getAttraction();

		// it's always going to be worthwhile [NOT] to try and take a blaster launcher, apparently;
		// too bad the aLiens don't know how to use them very well
//kL		if (!item->getRules()->isWaypoints() &&
	if (item->getRules()->getBattleType() != BT_AMMO)
	{
		// we only want weapons that HAVE ammo, or weapons that we have ammo for
		bool ammoFound = true;
		if (item->getAmmoItem() == NULL)
		{
			ammoFound = false;

			for (std::vector<BattleItem*>::const_iterator
					i = action->actor->getInventory()->begin();
					i != action->actor->getInventory()->end()
						&& ammoFound == false;
					++i)
			{
				if ((*i)->getRules()->getBattleType() == BT_AMMO)
				{
					for (std::vector<std::string>::const_iterator
							j = item->getRules()->getCompatibleAmmo()->begin();
							j != item->getRules()->getCompatibleAmmo()->end()
								&& ammoFound == false;
							++j)
					{
						if ((*i)->getRules()->getName() == *j)
						{
							ammoFound = true;
							break;
						}
					}
				}
			}
		}

		if (ammoFound == false)
			return false;
	}

	if (item->getRules()->getBattleType() == BT_AMMO)
	{
		// similar to the above, but this time we're checking if the ammo is suitable for a weapon we have.
		bool weaponFound = false;

		for (std::vector<BattleItem*>::const_iterator
				i = action->actor->getInventory()->begin();
				i != action->actor->getInventory()->end()
					&& weaponFound == false;
				++i)
		{
			if ((*i)->getRules()->getBattleType() == BT_FIREARM)
			{
				for (std::vector<std::string>::const_iterator
						j = (*i)->getRules()->getCompatibleAmmo()->begin();
						j != (*i)->getRules()->getCompatibleAmmo()->end()
							&& weaponFound == false;
						++j)
				{
					if ((*i)->getRules()->getName() == *j)
					{
						weaponFound = true;
						break;
					}
				}
			}
		}

		if (weaponFound == false)
			return false;
	}
//	}

//	if (worth)
//	{
	// use bad logic to determine if we'll have room for the item
	int freeSlots = 25;

	for (std::vector<BattleItem*>::const_iterator
			i = action->actor->getInventory()->begin();
			i != action->actor->getInventory()->end();
			++i)
	{
		freeSlots -= (*i)->getRules()->getInventoryHeight() * (*i)->getRules()->getInventoryWidth();
	}

	if (freeSlots < item->getRules()->getInventoryHeight() * item->getRules()->getInventoryWidth())
		return false;
//	}

	// return false for any item that we aren't standing directly
	// on top of with an attraction value less than 6 (aka always) [NOT.. anymore]

	return (worth
		 - (_save->getTileEngine()->distance(
											action->actor->getPosition(),
											item->getTile()->getPosition()) * 2) > 0); // was 5
}

/**
 * Picks the item up from the ground.
 * At this point we've decided it's worthwhile to grab this item, so
 * try to do just that. First check to make sure actor has time units,
 * then that there is space (using horrifying logic!) attempt to
 * actually recover the item.
 * @param item		- pointer to a BattleItem to go for
 * @param action	- pointer to the current BattleAction struct
 * @return,	 0 - successful
 *			 1 - no-TUs
 *			 2 - not-enough-room
 *			 3 - won't-fit
 *			-1 - something-went-horribly-wrong
 */
int BattlescapeGame::takeItemFromGround(
		BattleItem* item,
		BattleAction* action)
{
	//Log(LOG_INFO) << "BattlescapeGame::takeItemFromGround()";
	const int
		TAKEITEM_ERROR		= -1,
		TAKEITEM_SUCCESS	=  0,
		TAKEITEM_NOTU		=  1,
		TAKEITEM_NOSPACE	=  2,
		TAKEITEM_NOFIT		=  3;

	int freeSlots = 25;

	// make sure we have time units
	if (action->actor->getTimeUnits() < 6) // should replace that w/ Ground-to-Hand TU rule.
		return TAKEITEM_NOTU;
	else
	{
		// check to make sure we have enough space by checking all the sizes of items in our inventory
		for (std::vector<BattleItem*>::const_iterator
				i = action->actor->getInventory()->begin();
				i != action->actor->getInventory()->end();
				++i)
		{
			freeSlots -= (*i)->getRules()->getInventoryHeight() * (*i)->getRules()->getInventoryWidth();
		}

		if (freeSlots < item->getRules()->getInventoryHeight() * item->getRules()->getInventoryWidth())
			return TAKEITEM_NOSPACE;
		else
		{
			// check that the item will fit in our inventory, and if so, take it
			if (takeItem(item, action) == true)
			{
				action->actor->spendTimeUnits(6);
				item->getTile()->removeItem(item);

				return TAKEITEM_SUCCESS;
			}
			else
				return TAKEITEM_NOFIT;
		}
	}

	// shouldn't ever end up here (yeah, right..)
	return TAKEITEM_ERROR;
}

/**
 * Tries to fit an item into the unit's inventory, return false if you can't.
 * @param item		- pointer to a BattleItem to go for
 * @param action	- pointer to the current BattleAction struct
 * @return, true if the item was successfully retrieved
 */
bool BattlescapeGame::takeItem(
		BattleItem* item,
		BattleAction* action)
{
	//Log(LOG_INFO) << "BattlescapeGame::takeItem()";
	bool placed = false;
	Ruleset* rules = _parentState->getGame()->getRuleset();

	switch (item->getRules()->getBattleType())
	{
		case BT_AMMO: // find equipped weapons that can be loaded with this ammo
			if (action->actor->getItem("STR_RIGHT_HAND")
				&& action->actor->getItem("STR_RIGHT_HAND")->getAmmoItem() == NULL)
			{
				if (action->actor->getItem("STR_RIGHT_HAND")->setAmmoItem(item) == 0)
					placed = true;
			}
			else
			{
				for (int
						i = 0;
						i < 4;
						++i)
				{
					if (action->actor->getItem("STR_BELT", i) == NULL)
					{
						item->moveToOwner(action->actor);
						item->setSlot(rules->getInventory("STR_BELT"));
						item->setSlotX(i);

						placed = true;
						break;
					}
				}
			}
		break;
		case BT_GRENADE:
		case BT_PROXIMITYGRENADE:
			for (int
					i = 0;
					i < 4;
					++i)
			{
				if (action->actor->getItem("STR_BELT", i) == NULL)
				{
					item->moveToOwner(action->actor);
					item->setSlot(rules->getInventory("STR_BELT"));
					item->setSlotX(i);

					placed = true;
					break;
				}
			}
		break;
		case BT_FIREARM:
		case BT_MELEE:
			if (action->actor->getItem("STR_RIGHT_HAND") == NULL)
			{
				item->moveToOwner(action->actor);
				item->setSlot(rules->getInventory("STR_RIGHT_HAND"));

				placed = true;
			}
		break;
		case BT_MEDIKIT:
		case BT_SCANNER:
			if (action->actor->getItem("STR_BACK_PACK") == NULL)
			{
				item->moveToOwner(action->actor);
				item->setSlot(rules->getInventory("STR_BACK_PACK"));

				placed = true;
			}
		break;
		case BT_MINDPROBE:
			if (action->actor->getItem("STR_LEFT_HAND") == NULL)
			{
				item->moveToOwner(action->actor);
				item->setSlot(rules->getInventory("STR_LEFT_HAND"));

				placed = true;
			}
		break;
	}

	return placed;
}

/**
 * Sets the TU reserved type as a BattleAction.
 * @param bat - a battleactiontype (BattlescapeGame.h)
 */
void BattlescapeGame::setReservedAction(BattleActionType bat)
{
	_save->setBATReserved(bat);
}

/**
 * Returns the action type that is reserved.
 * @return, the BattleActionType that is reserved
 */
BattleActionType BattlescapeGame::getReservedAction() const
{
	return _save->getBATReserved();
}

/**
 * Tallies the living units in the game and, if required, converts units into their spawn unit.
 * @param liveAliens	- reference in which to store the live alien tally
 * @param liveSoldiers	- reference in which to store the live XCom tally
 * @param convert		- true to convert infected units (default false)
 */
void BattlescapeGame::tallyUnits(
		int& liveAliens,
		int& liveSoldiers,
		bool convert)
{
	liveSoldiers = 0;
	liveAliens = 0;

	if (convert == true)
	{
		for (std::vector<BattleUnit*>::const_iterator
				j = _save->getUnits()->begin();
				j != _save->getUnits()->end();
				++j)
		{
			if ((*j)->getHealth() > 0							// this converts infected but still living victims at endTurn().
//				&& (*j)->getSpecialAbility() == SPECAB_RESPAWN)	// Chryssalids inflict SPECAB_RESPAWN, in ExplosionBState (melee hits)
				&& (*j)->getRespawn() == true)
			{
				//Log(LOG_INFO) << "BattlescapeGame::tallyUnits() " << (*j)->getId() << " : health > 0, SPECAB_RESPAWN -> converting unit!";
//kL			(*j)->setSpecialAbility(SPECAB_NONE); // do this in convertUnit()
				convertUnit(
						*j,
						(*j)->getSpawnUnit());

				j = _save->getUnits()->begin();
			}
		}
	}

	for (std::vector<BattleUnit*>::const_iterator
			j = _save->getUnits()->begin();
			j != _save->getUnits()->end();
			++j)
	{
		if ((*j)->isOut() == false)
		{
			if ((*j)->getOriginalFaction() == FACTION_HOSTILE)
			{
				if (Options::allowPsionicCapture == false
					|| (*j)->getFaction() != FACTION_PLAYER)
				{
					++liveAliens;
				}
			}
			else if ((*j)->getOriginalFaction() == FACTION_PLAYER)
			{
				if ((*j)->getFaction() == FACTION_PLAYER)
					++liveSoldiers;
				else
					++liveAliens;
			}
		}
	}
}

/**
 * Sets the kneel reservation setting.
 * @param reserved - true to reserve an extra 4 TUs to kneel
 */
void BattlescapeGame::setKneelReserved(bool reserved)
{
//	_kneelReserved = reserved;
	_save->setKneelReserved(reserved);
}

/**
 * Gets the kneel reservation setting.
 * @return, kneel reservation setting
 */
bool BattlescapeGame::getKneelReserved()
{
	return _save->getKneelReserved();

//	if (_save->getSelectedUnit()
//		&& _save->getSelectedUnit()->getGeoscapeSoldier())
//	{
//		return _kneelReserved;
//	}

//	return false;
}

/**
 * Checks if a unit has moved next to a proximity grenade.
 * Checks one tile around the unit in every direction.
 * For a large unit we check every tile it occupies.
 * @param unit - pointer to a BattleUnit
 * @return, true if a proximity grenade was triggered
 */
bool BattlescapeGame::checkForProximityGrenades(BattleUnit* unit)
{
	int unitSize = unit->getArmor()->getSize() - 1;
	for (int
			x = unitSize;
			x > -1;
			--x)
	{
		for (int
				y = unitSize;
				y > -1;
				--y)
		{
			for (int
					tx = -1;
					tx < 2;
					++tx)
			{
				for (int
						ty = -1;
						ty < 2;
						++ty)
				{
					Tile* const tile = _save->getTile(unit->getPosition() + Position(x, y, 0) + Position(tx, ty, 0));
					if (tile != NULL)
					{
						for (std::vector<BattleItem*>::const_iterator
								i = tile->getInventory()->begin();
								i != tile->getInventory()->end();
								++i)
						{
							if ((*i)->getRules()->getBattleType() == BT_PROXIMITYGRENADE
								&& (*i)->getFuseTimer() != -1)
							{
								int dir; // cred: animal310 - http://openxcom.org/bugs/openxcom/issues/765
								_save->getPathfinding()->vectorToDirection(
																		Position(tx, ty, 0),
																		dir);
								//Log(LOG_INFO) << "dir = " << dir;
								if (_save->getPathfinding()->isBlocked(
																	_save->getTile(unit->getPosition() + Position(x, y, 0)),
																	NULL,
																	dir,
																	unit) // kL try passing in OBJECT_SELF as a missile target to kludge for closed doors.
																== false) // there *might* be a problem if the Proxy is on a non-walkable tile ....
								{
									Position pos;

									pos.x = tile->getPosition().x * 16 + 8;
									pos.y = tile->getPosition().y * 16 + 8;
									pos.z = tile->getPosition().z * 24 - tile->getTerrainLevel();

									statePushNext(new ExplosionBState(
																	this,
																	pos,
																	*i,
																	(*i)->getPreviousOwner()));

									getSave()->removeItem(*i);

									unit->setCache(NULL);
									getMap()->cacheUnit(unit);

									return true;
								}
							}
						}
					}
				}
			}
		}
	}

	return false;
}

/**
 * Cleans up all the deleted states.
 */
void BattlescapeGame::cleanupDeleted()
{
	for (std::list<BattleState*>::const_iterator
			i = _deleted.begin();
			i != _deleted.end();
			++i)
	{
		delete *i;
	}

	_deleted.clear();
}

/**
 * Gets the depth of the battlescape.
 * @return, the depth of the battlescape
 */
const int BattlescapeGame::getDepth() const
{
	return _save->getDepth();
}

/**
 * kL. Gets the BattlescapeState.
 * For turning on/off the visUnits indicators from UnitWalk/TurnBStates
 * @return, pointer to BattlescapeState
 */
BattlescapeState* BattlescapeGame::getBattlescapeState() const
{
	return _parentState;
}

/**
 * kL. Gets the universal fist.
 * @return, the universal fist!!
 */
BattleItem* BattlescapeGame::getFist() const // kL
{
	return _universalFist;
}

/**
 * kL. Gets the universal alienPsi weapon.
 * @return, the alienPsi BattleItem
 */
BattleItem* BattlescapeGame::getAlienPsi() const // kL
{
	return _alienPsi;
}

}
