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

#define _USE_MATH_DEFINES

#include "BattlescapeGame.h"

#include <cmath>
#include <sstream>
#include <typeinfo>

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
#include "../Engine/Logger.h"
#include "../Engine/Options.h"
#include "../Engine/RNG.h"
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
#include "../Savegame/SoldierDiary.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

bool BattlescapeGame::_debugPlay = false;


/**
 * Initializes all the elements in the Battlescape screen.
 * @param save Pointer to the save game.
 * @param parentState Pointer to the parent battlescape state.
 */
BattlescapeGame::BattlescapeGame(
		SavedBattleGame* save,
		BattlescapeState* parentState)
	:
		_save(save),
		_parentState(parentState),
		_playedAggroSound(false),
		_endTurnRequested(false),
		_kneelReserved(false)
{
	//Log(LOG_INFO) << "Create BattlescapeGame";
	_tuReserved				= BA_NONE;
	_playerTUReserved		= BA_NONE;
	_debugPlay				= false;
	_playerPanicHandled		= true;
	_AIActionCounter		= 0;
	_AISecondMove			= false;
	_currentAction.actor	= NULL;

	checkForCasualties(
					NULL,
					NULL,
					true);

	cancelCurrentAction();

	_currentAction.type			= BA_NONE;
	_currentAction.targeting	= false;

	for (std::vector<BattleUnit*>::iterator // kL
			bu = _save->getUnits()->begin();
			bu != _save->getUnits()->end();
			++bu)
	{
		(*bu)->setBattleGame(this);
	}
}

/**
 * Delete BattlescapeGame.
 */
BattlescapeGame::~BattlescapeGame()
{
	//Log(LOG_INFO) << "Delete BattlescapeGame";

// delete CTD_begin:
// THIS CAUSES ctd BECAUSE, WHEN RELOADING FROM Main-Menu,
// A new BATTLESCAPE_GAME IS CREATED *BEFORE* THE OLD ONE IS dTor'D
// ( i think ) So this loop 'deletes' the fresh BattlescapeGame's states .....
// leaving the old states that should (have) be(en) deleted intact.
	for (std::list<BattleState*>::iterator
			i = _states.begin();
			i != _states.end();
			++i)
	{
		delete *i;
	}
// *cough* so much for that hypothesis

//kL	cleanupDeleted();	// <- there it is ! Yet it works in NextTurnState ???
							// Added it to DebriefingState instead of here. (see)
// delete CTD_end.
}

/**
 * Checks for units panicking or falling and so on.
 */
void BattlescapeGame::think()
{
	//Log(LOG_INFO) << "BattlescapeGame::think()";

	// nothing is happening - see if we need some alien AI or units panicking or what have you
	if (_states.empty())
	{
		if (_save->getSide() != FACTION_PLAYER) // it's a non player side (ALIENS or CIVILIANS)
		{
			if (!_debugPlay)
			{
				if (_save->getSelectedUnit())
				{
					if (!handlePanickingUnit(_save->getSelectedUnit()))
//						&& _save->getSelectedUnit()->getFaction() != FACTION_PLAYER) // kL, safety.
					{
						//Log(LOG_INFO) << "BattlescapeGame::think() call handleAI() ID " << _save->getSelectedUnit()->getId();
						handleAI(_save->getSelectedUnit());
					}
				}
				else
				{
					if (_save->selectNextFactionUnit(
												true,
												_AISecondMove) == 0)
					{
						if (!_save->getDebugMode())
						{
							_endTurnRequested = true;
							statePushBack(0); // end AI turn
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
			if (!_playerPanicHandled)
			{
				_playerPanicHandled = handlePanickingPlayer();
				_save->getBattleState()->updateSoldierInfo();
			}
		}

		if (_save->getUnitsFalling())
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
 * @param unit, Pointer to a BattleUnit.
 */
void BattlescapeGame::handleAI(BattleUnit* unit)
{
	//Log(LOG_INFO) << "BattlescapeGame::handleAI() " << unit->getId();
	_tuReserved = BA_NONE;


	if (unit->getTimeUnits() < 4) // kL
		unit->dontReselect();

	if (//kL unit->getTimeUnits() < 6 ||
		_AIActionCounter > 1
		|| !unit->reselectAllowed())
	{
		if (_save->selectNextFactionUnit(
									true,
									_AISecondMove)
								== 0)
		{
			if (!_save->getDebugMode())
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

		if (_save->getSelectedUnit())
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

/*	if (unit->getMainHandWeapon()
		&& unit->getMainHandWeapon()->getRules()->getBattleType() == BT_FIREARM)
	{
		switch (unit->getAggression())
		{
			case 0:
				_tuReserved = BA_AIMEDSHOT;
			break;
			case 1:
				_tuReserved = BA_AUTOSHOT;
			break;
			case 2:
				_tuReserved = BA_SNAPSHOT;

			default:
			break;
		}
	} */ // kL, was moved to the aLienAI class.
	//Log(LOG_INFO) << ". aggressionReserved DONE";

	unit->setVisible(false);

	// might need this: populate _visibleUnit for a newly-created alien
	//Log(LOG_INFO) << "BattlescapeGame::handleAI(), calculateFOV() call";
	_save->getTileEngine()->calculateFOV(unit->getPosition());
		// it might also help chryssalids realize they've zombified someone and need to move on;
		// it should also hide units when they've killed the guy spotting them;
		// it's also for good luck

	BattleAIState* ai = unit->getCurrentAIState();
	if (!ai)
	{
		//Log(LOG_INFO) << "BattlescapeGame::handleAI() !ai, assign AI";

		// for some reason the unit had no AI routine assigned..
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

	_AIActionCounter++;
	if (_AIActionCounter == 1)
	{
		_playedAggroSound = false;
		unit->_hidingForTurn = false;

		if (Options::traceAI) Log(LOG_INFO) << "#" << unit->getId() << "--" << unit->getType();
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
		_parentState->debug(L"Rethink");
		unit->think(&action);
	}
	//Log(LOG_INFO) << ". BA_RETHINK DONE";


	_AIActionCounter = action.number;

	//Log(LOG_INFO) << ". pre hunt for weapon";
	if (!unit->getMainHandWeapon()
		|| !unit->getMainHandWeapon()->getAmmoItem())
	{
		//Log(LOG_INFO) << ". . no mainhand weapon or no ammo";
		if (unit->getOriginalFaction() == FACTION_HOSTILE)
//kL			&& unit->getVisibleUnits()->size() == 0)
		{
			//Log(LOG_INFO) << ". . . call findItem()";
			findItem(&action);
		}
	}
	//Log(LOG_INFO) << ". findItem DONE";


	if (unit->getCharging() != NULL)
	{
		if (unit->getAggroSound() != -1
			&& !_playedAggroSound)
		{
			_playedAggroSound = true;

			getResourcePack()->getSound(
									"BATTLE.CAT",
									unit->getAggroSound())
								->play();
		}
	}
	//Log(LOG_INFO) << ". getCharging DONE";


	std::wostringstream ss; // debug.

	if (action.type == BA_WALK)
	{
		ss << L"Walking to " << action.target;
		_parentState->debug(ss.str());

		if (_save->getTile(action.target))
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
			action.weapon = new BattleItem(
									_parentState->getGame()->getRuleset()->getItem("ALIEN_PSI_WEAPON"),
									_save->getCurrentItemId());
			action.TU = action.weapon->getRules()->getTUUse();

			if (!action.weapon->getRules()->getFlatRate()) // kL
				action.TU = static_cast<int>(
								floor(static_cast<float>(action.actor->getStats()->tu * action.TU) / 100.f)); // kL
		}
		else
			statePushBack(new UnitTurnBState(
											this,
											action));
		//Log(LOG_INFO) << ". . create Psi weapon DONE";


		ss.clear();
		ss << L"Attack type=" << action.type
				<< " target=" << action.target
				<< " weapon=" << action.weapon->getRules()->getName().c_str();
		_parentState->debug(ss.str());

		//Log(LOG_INFO) << ". attack action.Type = " << action.type
		//		<< ", action.Target = " << action.target
		//		<< " action.Weapon = " << action.weapon->getRules()->getName().c_str();


		//Log(LOG_INFO) << ". . call ProjectileFlyBState()";
		statePushBack(new ProjectileFlyBState(
											this,
											action));
		//Log(LOG_INFO) << ". . ProjectileFlyBState DONE";

		if (action.type == BA_MINDCONTROL
			|| action.type == BA_PANIC)
		{
			//Log(LOG_INFO) << ". . . in action.type Psi";

			bool success = _save->getTileEngine()->psiAttack(&action);
			//Log(LOG_INFO) << ". . . success = " << success;
			if (success
				&& action.type == BA_MINDCONTROL)
			{
				//Log(LOG_INFO) << ". . . inside success MC";
				// show a little infobox with the name of the unit and "... is under alien control"
				BattleUnit* unit = _save->getTile(action.target)->getUnit();
				Game* game = _parentState->getGame();
				game->pushState(new InfoboxState(game->getLanguage()->getString(
																		"STR_IS_UNDER_ALIEN_CONTROL",
																		unit->getGender())
																			.arg(unit->getName(game->getLanguage()))));
			}
			//Log(LOG_INFO) << ". . . success MC Done";

			_save->removeItem(action.weapon);
			//Log(LOG_INFO) << ". . . Psi weapon removed.";
		}
	}
	//Log(LOG_INFO) << ". . action.type DONE";

	if (action.type == BA_NONE)
	{
		//Log(LOG_INFO) << ". . in action.type None";
		_parentState->debug(L"Idle");
		_AIActionCounter = 0;

		if (_save->selectNextFactionUnit(
									true,
									_AISecondMove)
								== NULL)
		{
			if (!_save->getDebugMode())
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

		if (_save->getSelectedUnit())
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
 * Toggles the Kneel/Standup status of the unit.
 * @param bu, Pointer to a unit.
 * @return, True if the action succeeded.
 */
bool BattlescapeGame::kneel(
		BattleUnit* bu,
		bool calcFoV)
{
	//Log(LOG_INFO) << "BattlescapeGame::kneel()";
	if (bu->getType() == "SOLDIER")
	{
		if (!bu->isFloating()) // kL_note: This prevents flying soldiers from 'kneeling' .....
		{
			int tu = 4;
			if (bu->isKneeled())
				tu = 8;

			if (checkReservedTU(bu, tu)
				|| (tu == 4 //kL !bu->isKneeled()
					&& _kneelReserved))
			{
				if (bu->spendTimeUnits(tu))
				{
					bu->kneel(!bu->isKneeled());
					// kneeling or standing up can reveal new terrain or units. I guess. -> sure can!
					// but updateSoldierInfo() also does does calculateFOV(), so ...
//					getTileEngine()->calculateFOV(bu);

					getMap()->cacheUnits();
//kL					_parentState->updateSoldierInfo(false); // <- also does calculateFOV() !
						// wait... shouldn't one of those calcFoV's actually trigger!! ? !
						// Hopefully it's done after returning, in another updateSoldierInfo... or newVis check.
						// So.. I put this in BattlescapeState::btnKneelClick() instead; updates will
						// otherwise be handled by walking or what have you. Doing it this way conforms
						// updates/FoV checks with my newVis routines.

//kL					getTileEngine()->checkReactionFire(bu);
						// ditto..

					return true;
				}
				else // not enough tu
					_parentState->warning("STR_NOT_ENOUGH_TIME_UNITS");
			}
			else // tu Reserved
				_parentState->warning("STR_TIME_UNITS_RESERVED");
		}
		else // floating
			_parentState->warning("STR_ACTION_NOT_ALLOWED_FLOAT");
	}
	else // alien.
		_parentState->warning("STR_ACTION_NOT_ALLOWED_ALIEN");

	return false;
}

/**
 * Ends the turn.
 */
void BattlescapeGame::endTurn()
{
	//Log(LOG_INFO) << "BattlescapeGame::endTurn()";

// delete CTD_begin:
// THIS GOT MOVED TO NEXT_TURN_STATE
/*	for (std::list<BattleState*>::iterator i = _deleted.begin(); i != _deleted.end(); ++i)
	{
		delete *i;
	}
	_deleted.clear(); */
// delete CTD_end.

	_tuReserved		= _playerTUReserved;
	_debugPlay		= false;
	_AISecondMove	= false;
	_parentState->showLaunchButton(false);

	_currentAction.targeting	= false;
	_currentAction.type			= BA_NONE;
	_currentAction.waypoints.clear();

	getMap()->getWaypoints()->clear();

//	if (_save->getTileEngine()->closeUfoDoors())
//		getResourcePack()->getSound("BATTLE.CAT", 21)->play(); // ufo door closed

	Position pos;
	for (int
			i = 0;
			i < _save->getMapSizeXYZ();
			++i) // check for hot grenades on the ground
	{
		for (std::vector<BattleItem*>::iterator
				it = _save->getTiles()[i]->getInventory()->begin();
				it != _save->getTiles()[i]->getInventory()->end();
				)
		{
			if ((*it)->getRules()->getBattleType() == BT_GRENADE
				&& (*it)->getFuseTimer() == 0) // it's a grenade to explode now
			{
				pos.x = _save->getTiles()[i]->getPosition().x * 16 + 8;
				pos.y = _save->getTiles()[i]->getPosition().y * 16 + 8;
				pos.z = _save->getTiles()[i]->getPosition().z * 24 - _save->getTiles()[i]->getTerrainLevel();

				statePushNext(new ExplosionBState(
												this,
												pos,
												*it,
												(*it)->getPreviousOwner()));
				_save->removeItem(*it);

				statePushBack(NULL);

				//Log(LOG_INFO) << "BattlescapeGame::endTurn(), hot grenades EXIT";
				return;
			}

			++it;
		}
	}
	//Log(LOG_INFO) << ". done grenades";

	if (_save->getTileEngine()->closeUfoDoors()) // try, close doors between grenade & terrain explosions
		getResourcePack()->getSound("BATTLE.CAT", 21)->play(); // ufo door closed

	// check for terrain explosions
	Tile* t = _save->getTileEngine()->checkForTerrainExplosions();
	if (t)
	{
		Position pos = Position(
//kL							t->getPosition().x * 16,
//kL							t->getPosition().y * 16,
							t->getPosition().x * 16 + 8, // kL
							t->getPosition().y * 16 + 8, // kL
							t->getPosition().z * 24);

		// kL_note: This seems to be screwing up for preBattle powersource explosions; they
		// wait until the first turn, either endTurn or perhaps checkForCasualties or like that.
		statePushNext(new ExplosionBState(
										this,
										pos,
										NULL,
										NULL,
										t));

		t = _save->getTileEngine()->checkForTerrainExplosions();

		statePushBack(NULL);

		//Log(LOG_INFO) << "BattlescapeGame::endTurn(), terrain explosions EXIT";
		return;
	}
	//Log(LOG_INFO) << ". done explosions";

	if (_save->getSide() != FACTION_NEUTRAL) // tick down grenade timers
	{
		for (std::vector<BattleItem*>::iterator
				grenade = _save->getItems()->begin();
				grenade != _save->getItems()->end();
				++grenade)
		{
			if ((*grenade)->getOwner() == NULL // kL
				&& (*grenade)->getRules()->getBattleType() == BT_GRENADE
//kL				|| (*grenade)->getRules()->getBattleType() == BT_PROXIMITYGRENADE)
				&& (*grenade)->getFuseTimer() > 0)
			{
				(*grenade)->setFuseTimer((*grenade)->getFuseTimer() - 1);
			}
		}
	}
	//Log(LOG_INFO) << ". done !neutral";

	// kL_begin: battleUnit is burning...
	// Fire damage is also in Savegame/BattleUnit::prepareNewTurn(), on fire
	// see also, Savegame/Tile::prepareNewTurn(), catch fire on fire tile
	// fire damage by hit is caused by TileEngine::explode()
	for (std::vector<BattleUnit*>::iterator
			j = _save->getUnits()->begin();
			j != _save->getUnits()->end();
			++j)
	{
		if ((*j)->getFaction() == _save->getSide())
		{
			// Catch fire first! do it here

//kL			if (!(*j)->getTookFire()
//kL				&& (*j)->getFire() > 0)
			int fire = (*j)->getFire(); // kL
			if (fire > 0) // kL
			{
//kL				(*j)->setFire(-1);
				(*j)->setFire(fire - 1); // kL

				int fireDamage = static_cast<int>(
									(*j)->getArmor()->getDamageModifier(DT_IN)
										* static_cast<float>(RNG::generate(3, 9)));
				//Log(LOG_INFO) << ". . endTurn() ID " << (*j)->getId() << " fireDamage = " << fireDamage;
				(*j)->setHealth((*j)->getHealth() - fireDamage);

				if ((*j)->getHealth() < 0)
					(*j)->setHealth(0);
			}
		}
	}
	// that just might work...
	// kL_end.
	//Log(LOG_INFO) << ". done fire damage";

	// if all units from either faction are killed - the mission is over.
	int liveAliens = 0;
	int liveSoldiers = 0;
	// we'll tally them NOW, so that any infected units will... change
	tallyUnits(
			liveAliens,
			liveSoldiers,
			true);
	//Log(LOG_INFO) << ". done tallyUnits";

	_save->endTurn(); // <- this rolls over Faction-turns.
	//Log(LOG_INFO) << ". done save->endTurn";

	if (_save->getSide() == FACTION_PLAYER)
		setupCursor();
	else
		getMap()->setCursorType(CT_NONE);


	checkForCasualties(
					NULL,
					NULL);
	//Log(LOG_INFO) << ". done checkForCasualties";

	// turn off MCed alien lighting.
	_save->getTileEngine()->calculateUnitLighting();

	if (_save->isObjectiveDestroyed())
	{
		_parentState->finishBattle(
								false,
								liveSoldiers);

		//Log(LOG_INFO) << "BattlescapeGame::endTurn(), objectiveDestroyed EXIT";
		return;
	}

	if (liveAliens > 0
		&& liveSoldiers > 0)
	{
		showInfoBoxQueue();

		_parentState->updateSoldierInfo();

		if (playableUnitSelected()) // <- only Faction_Player (or Debug-mode)
		{
			getMap()->getCamera()->centerOnPosition(_save->getSelectedUnit()->getPosition());
			setupCursor();
		}
	}
	//Log(LOG_INFO) << ". done updates";

	bool battleComplete = liveAliens == 0
						|| liveSoldiers == 0;

	if (_endTurnRequested
		&& (_save->getSide() != FACTION_NEUTRAL
			|| battleComplete))
	{
		//Log(LOG_INFO) << ". . pushState(nextTurnState)";
		_parentState->getGame()->pushState(new NextTurnState(
														_save,
														_parentState));
	}

	_endTurnRequested = false;
	//Log(LOG_INFO) << "BattlescapeGame::endTurn() EXIT";
}

/**
 * Checks for casualties and adjusts morale accordingly.
 * @param weapon	- need to know this; for a HE explosion there is an instant death
 * @param slayer	- this is needed to credit the kill
 * @param hidden	- true for UFO Power Source explosions at the start of battlescape
 * @param terrain	- true for terrain explosions
 */
void BattlescapeGame::checkForCasualties(
		BattleItem* weapon,
		BattleUnit* slayer,
		bool hidden,
		bool terrain)
{
	//Log(LOG_INFO) << "BattlescapeGame::checkForCasualties()";
	std::string
		killStatRank,
		killStatRace,
		killStatWeapon,
		killStatWeaponAmmo;
	int
		killStatMission,
		killStatTurn;

	// If the victim was killed by the slayer's death explosion,
	// fetch who killed the murderer and make THAT the murderer!
	if (slayer
		&& !slayer->getGeoscapeSoldier()
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
				slayer = (*i);
		}
	}

	// kL_note: what about tile explosions
	killStatWeapon = "STR_WEAPON_UNKNOWN";
	killStatWeaponAmmo = "STR_WEAPON_UNKNOWN";

	if (slayer != NULL)
		killStatMission = _save->getGeoscapeSave()->getMissionStatistics()->size();

	if (_save->getSide() == FACTION_PLAYER)
		killStatTurn = _save->getTurn() * 3;
	else if (_save->getSide() == FACTION_HOSTILE)
		killStatTurn = _save->getTurn() * 3 + 1;
	else if (_save->getSide() == FACTION_NEUTRAL)
		killStatTurn = _save->getTurn() * 3 + 2;

	// Fetch the murder weapon
	if (slayer
		&& slayer->getGeoscapeSoldier())
	{
		if (weapon)
		{
			killStatWeaponAmmo = weapon->getRules()->getName();
			killStatWeapon = killStatWeaponAmmo;
		}

		BattleItem* weapon2 = slayer->getItem("STR_RIGHT_HAND");
		if (weapon2)
		{
			for (std::vector<std::string>::iterator
					c = weapon2->getRules()->getCompatibleAmmo()->begin();
					c != weapon2->getRules()->getCompatibleAmmo()->end();
					++c)
			{
				if (*c == killStatWeaponAmmo)
					killStatWeapon = weapon2->getRules()->getName();
			}
		}

		weapon2 = slayer->getItem("STR_LEFT_HAND");
		if (weapon2)
		{
			for (std::vector<std::string>::iterator
					c = weapon2->getRules()->getCompatibleAmmo()->begin();
					c != weapon2->getRules()->getCompatibleAmmo()->end();
					++c)
			{
				if (*c == killStatWeaponAmmo)
					killStatWeapon = weapon2->getRules()->getName();
			}
		}
	}

	for (std::vector<BattleUnit*>::iterator
			buCasualty = _save->getUnits()->begin();
			buCasualty != _save->getUnits()->end();
			++buCasualty)
	{
		BattleUnit* victim = *buCasualty; // kL

		// Awards: decide victim race and rank
		if (victim->getGeoscapeSoldier()							// Soldiers
				&& victim->getOriginalFaction() == FACTION_PLAYER)
		{
			killStatRank = victim->getGeoscapeSoldier()->getRankString();
			killStatRace = "STR_HUMAN";
		}
		else if (victim->getOriginalFaction() == FACTION_PLAYER)	// HWPs
		{
			killStatRank = "STR_HEAVY_WEAPONS_PLATFORM_LC";
			killStatRace = "STR_TANK";
		}
		else if (victim->getOriginalFaction() == FACTION_HOSTILE)	// Aliens
		{
			killStatRank = victim->getUnitRules()->getRank();
			killStatRace = victim->getUnitRules()->getRace();
		}
		else if (victim->getOriginalFaction() == FACTION_NEUTRAL)	// Civilians
		{
			killStatRank = "STR_CIVILIAN";
			killStatRace = "STR_HUMAN";
		}
		else														// Error
		{
			killStatRank = "STR_UNKNOWN";
			killStatRace = "STR_UNKNOWN";
		}

		if (victim->getHealth() == 0
			&& victim->getStatus() != STATUS_DEAD
			&& victim->getStatus() != STATUS_COLLAPSING	// kL_note: is this really needed ....
			&& victim->getStatus() != STATUS_TURNING	// kL: may be set by UnitDieBState cTor
			&& victim->getStatus() != STATUS_DISABLED)	// kL
		{
			(*buCasualty)->setStatus(STATUS_DISABLED);	// kL
			//Log(LOG_INFO) << ". DEAD victim = " << victim->getId();

			if (slayer
				&& slayer->getTurretType() == -1) // not a Tank.
			{
				//Log(LOG_INFO) << ". slayer = " << slayer->getId();
				if (slayer->getGeoscapeSoldier()
					&& slayer->getFaction() == FACTION_PLAYER)
				{
					slayer->getStatistics()->kills.push_back(new BattleUnitKills(
																			killStatRank,
																			killStatRace,
																			killStatWeapon,
																			killStatWeaponAmmo,
																			victim->getFaction(),
																			STATUS_DEAD,
																			killStatMission,
																			killStatTurn));
					victim->setMurdererId(slayer->getId());
				}

				(*buCasualty)->killedBy(slayer->getFaction()); // used in DebriefingState.

				int bonus = 100;
				if (slayer->getOriginalFaction() == FACTION_PLAYER)
				{
					slayer->addKillCount();
					bonus = _save->getMoraleModifier();
				}
				else if (slayer->getOriginalFaction() == FACTION_HOSTILE)
					bonus = _save->getMoraleModifier(NULL, false);

				// slayer's boost
				if ((victim->getOriginalFaction() == FACTION_PLAYER
						&& slayer->getOriginalFaction() == FACTION_HOSTILE)
					|| (victim->getOriginalFaction() == FACTION_HOSTILE
						&& slayer->getOriginalFaction() == FACTION_PLAYER))
				{
					int boost = 10 * bonus / 100;
					slayer->moraleChange(boost); // doubles what rest of squad gets below
					//Log(LOG_INFO) << ". . slayer boost +" << boost;
				}

				// slayer (mc'd or not) will get a penalty with friendly fire (mc'd or not)
				// ... except aLiens, who don't care.
				if (victim->getOriginalFaction() == FACTION_PLAYER
					&& slayer->getOriginalFaction() == FACTION_PLAYER)
				{
					int hit = 5000 / bonus;
					slayer->moraleChange(-hit); // huge hit!
					//Log(LOG_INFO) << ". . FF hit, slayer -" << hit;
				}

				if (victim->getOriginalFaction() == FACTION_NEUTRAL) // civilian kills
				{
					if (slayer->getOriginalFaction() == FACTION_PLAYER)
					{
						int civdeath = 2000 / bonus;
						slayer->moraleChange(-civdeath);
						//Log(LOG_INFO) << ". . civdeath by xCom, soldier -" << civdeath;
					}
					else if (slayer->getOriginalFaction() == FACTION_HOSTILE)
					{
						slayer->moraleChange(20); // no leadership bonus for aLiens executing civies: it's their job.
						//Log(LOG_INFO) << ". . civdeath by aLien, slayer +" << 20;
					}
				}
			}

			// cycle through units and do all faction
//kL		if (victim->getFaction() != FACTION_NEUTRAL) // civie deaths now affect other Factions.
//			{
			// penalty for the death of a unit; civilians return standard 100.
			int solo = _save->getMoraleModifier(victim);
			// these two are faction bonuses ('losers' mitigates the loss of solo, 'winners' boosts solo)
			int
				losers = 100,
				winners = 100;

			if (victim->getOriginalFaction() == FACTION_HOSTILE)
			{
				losers = _save->getMoraleModifier(NULL, false);
				winners = _save->getMoraleModifier();
			}
			else
			{
				losers = _save->getMoraleModifier();
				winners = _save->getMoraleModifier(NULL, false);
			}
			// civilians are unaffected by leadership above. They use standard 100.

			// do bystander FACTION changes:
			for (std::vector<BattleUnit*>::iterator
					buOther = _save->getUnits()->begin();
					buOther != _save->getUnits()->end();
					++buOther)
			{
				if (!(*buOther)->isOut(true, true)
//					&& (*buOther)->getArmor()->getSize() == 1) // not a large unit
					&& (*buOther)->getTurretType() == -1) // not a Tank.
				{
					if (victim->getOriginalFaction() == (*buOther)->getOriginalFaction()
						|| (victim->getOriginalFaction() == FACTION_NEUTRAL				// for civie-death,
							&& (*buOther)->getFaction() == FACTION_PLAYER				// non-Mc'd xCom takes hit
							&& (*buOther)->getOriginalFaction() != FACTION_HOSTILE)		// but not Mc'd aLiens
						|| (victim->getOriginalFaction() == FACTION_PLAYER				// for death of xCom unit,
							&& (*buOther)->getOriginalFaction() == FACTION_NEUTRAL))	// civies take hit.
					{
						// losing team(s) all get a morale loss,
						// based on their individual Bravery & rank of unit that was killed
						int bravery = (110 - (*buOther)->getStats()->bravery) / 10;
						if (bravery > 0)
						{
							bravery = solo * 2 * bravery / losers;
							(*buOther)->moraleChange(-bravery);
						}

						//Log(LOG_INFO) << ". . . loser -" << bravery;
/*kL
						if (slayer
							&& slayer->getFaction() == FACTION_PLAYER
							&& victim->getFaction() == FACTION_HOSTILE)
						{
							slayer->setTurnsExposed(0); // interesting
							//Log(LOG_INFO) << ". . . . slayer Exposed";
						} */
					}
					// note this is unaffected by the rank of the dead unit...
					else if ((victim->getOriginalFaction() == FACTION_HOSTILE
							&& ((*buOther)->getOriginalFaction() == FACTION_PLAYER
								|| (*buOther)->getOriginalFaction() == FACTION_NEUTRAL))
						|| ((*buOther)->getOriginalFaction() == FACTION_HOSTILE
							&& (victim->getOriginalFaction() == FACTION_PLAYER
								|| victim->getOriginalFaction() == FACTION_NEUTRAL)))
					{
						// winning team(s) all get a morale boost
						int boost = winners / 10;
						(*buOther)->moraleChange(boost);

						//Log(LOG_INFO) << ". . . winner +" << boost;
					}
				}
			}
//			}

			if (weapon)
				statePushNext(new UnitDieBState( // kL_note: This is where units get sent to DEATH!
											this,
											*buCasualty,
											weapon->getRules()->getDamageType(),
											false));
			else
			{
				if (hidden) // this is instant death from UFO powersources, without screaming sounds
				{
					//Log(LOG_INFO) << ". . hidden-PS!!!";
					statePushNext(new UnitDieBState(
												this,
												*buCasualty,
												DT_HE,
												true));
					//Log(LOG_INFO) << ". . hidden-PS!!! done.";
				}
				else
				{
					if (terrain)
						statePushNext(new UnitDieBState(
													this,
													*buCasualty,
													DT_HE,
													false));
					else // no slayer, and no terrain explosion, must be fatal wounds
						statePushNext(new UnitDieBState(
													this,
													*buCasualty,
													DT_NONE,
													false)); // DT_NONE = STR_HAS_DIED_FROM_A_FATAL_WOUND
				}
			}
		}
		else if (victim->getStunlevel() > victim->getHealth() - 1
			&& victim->getStatus() != STATUS_DEAD
			&& victim->getStatus() != STATUS_UNCONSCIOUS
			&& victim->getStatus() != STATUS_COLLAPSING	// kL_note: is this really needed ....
			&& victim->getStatus() != STATUS_TURNING	// kL_note: may be set by UnitDieBState cTor
			&& victim->getStatus() != STATUS_DISABLED)	// kL
		{
			//Log(LOG_INFO) << ". STUNNED victim = " << victim->getId();
			if (slayer
				&& slayer->getGeoscapeSoldier()
				&& slayer->getFaction() == FACTION_PLAYER)
			{
				slayer->getStatistics()->kills.push_back(new BattleUnitKills(
																		killStatRank,
																		killStatRace,
																		killStatWeapon,
																		killStatWeaponAmmo,
																		victim->getFaction(),
																		STATUS_UNCONSCIOUS,
																		killStatMission,
																		killStatTurn));
			}

			if (victim
				&& victim->getGeoscapeSoldier())
			{
				victim->getStatistics()->wasUnconcious = true;
			}

			(*buCasualty)->setStatus(STATUS_DISABLED); // kL

			statePushNext(new UnitDieBState( // kL_note: This is where units get set to STUNNED
										this,
										*buCasualty,
										DT_STUN,
										true));
		}
	}

	if (!hidden // showPsiButton() ought already be false at mission start.
		&& _save->getSide() == FACTION_PLAYER)
	{
		BattleUnit* bu = _save->getSelectedUnit();
		if (bu)
			_parentState->showPsiButton(
									bu->getOriginalFaction() == FACTION_HOSTILE
									&& bu->getStats()->psiSkill > 0
									&& !bu->isOut(true, true));
	}
	//Log(LOG_INFO) << "BattlescapeGame::checkForCasualties() EXIT";
}

/**
 * Shows the infoboxes in the queue (if any).
 */
void BattlescapeGame::showInfoBoxQueue()
{
	for (std::vector<InfoboxOKState*>::iterator
			i = _infoboxQueue.begin();
			i != _infoboxQueue.end();
			++i)
	{
		_parentState->getGame()->pushState(*i);
	}

	_infoboxQueue.clear();
}

/**
 * Handles the result of non target actions,
 * like priming a grenade or performing a melee attack.
 */
void BattlescapeGame::handleNonTargetAction()
{
	//Log(LOG_INFO) << "BattlescapeGame::handleNonTargetAction()";
	if (!_currentAction.targeting)
	{
		_currentAction.cameraPosition = Position(0, 0,-1);

		if (_currentAction.type == BA_PRIME
			|| _currentAction.type == BA_DEFUSE)
//			&& _currentAction.value > -1)
		{
			//Log(LOG_INFO) << "BattlescapeGame::handleNonTargetAction() BA_PRIME";
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
			//Log(LOG_INFO) << "BattlescapeGame::handleNonTargetAction() BA_USE or BA_LAUNCH";
			if (_currentAction.result.length() > 0)
			{
				_parentState->warning(_currentAction.result);
				_currentAction.result = "";
			}

			_save->reviveUnconsciousUnits();
		}
		else if (_currentAction.type == BA_HIT)
		{
			//Log(LOG_INFO) << "BattlescapeGame::handleNonTargetAction() BA_HIT";
			if (_currentAction.result.length() > 0)
			{
				_parentState->warning(_currentAction.result);
				_currentAction.result = "";
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
	getMap()->refreshSelectorPosition(); // kL

	if (_currentAction.targeting)
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

		if (_currentAction.actor)
			getMap()->setCursorType(
								CT_NORMAL,
								_currentAction.actor->getArmor()->getSize());
		else // kL
			getMap()->setCursorType( // kL
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
	return _save->getSelectedUnit() != 0
		&& (_save->getSide() == FACTION_PLAYER
			|| _save->getDebugMode());
}

/**
 * Gives time slice to the front state.
 */
void BattlescapeGame::handleState()
{
	if (!_states.empty())
	{
		// end turn request?
		if (_states.front() == 0)
		{
			_states.pop_front();

			endTurn();

			return;
		}
		else
			_states.front()->think();


//kL		getMap()->invalidate(); // redraw map
		getMap()->draw(); // kL, old code!! -> Map::draw()
	}
}

/**
 * Pushes a state to the front of the queue and starts it.
 * @param bs Battlestate.
 */
void BattlescapeGame::statePushFront(BattleState* bs)
{
	_states.push_front(bs);
	bs->init();
}

/**
 * Pushes a state as the next state after the current one.
 * @param bs Battlestate.
 */
void BattlescapeGame::statePushNext(BattleState* bs)
{
	if (_states.empty())
	{
		_states.push_front(bs);
		bs->init();
	}
	else
		_states.insert(++_states.begin(), bs);
}

/**
 * Pushes a state to the back.
 * @param bs Battlestate.
 */
void BattlescapeGame::statePushBack(BattleState* bs)
{
	if (_states.empty())
	{
		_states.push_front(bs);

		if (_states.front() == 0) // end turn request
		{
			_states.pop_front();
			endTurn();

			return;
		}
		else
			bs->init();
	}
	else
		_states.push_back(bs);
}

/**
 * Removes the current state.
 *
 * This is a very important function. It is called by a BattleState (walking, projectile
 * is flying, explosions,...) at the moment this state has finished its action.
 * Here we check the result of that action and do all the aftermath.
 * The state is popped off the list.
 */
void BattlescapeGame::popState()
{
	//Log(LOG_INFO) << "BattlescapeGame::popState()";
	if (Options::traceAI)
	{
		Log(LOG_INFO) << "BattlescapeGame::popState() #" << _AIActionCounter << " with "
			<< (_save->getSelectedUnit()? _save->getSelectedUnit()->getTimeUnits(): -9999) << " TU";
	}

	if (_states.empty())
	{
		//Log(LOG_INFO) << ". states.Empty -> return";
		return;
	}

	bool actionFailed = false;

	BattleAction action = _states.front()->getAction();
	if (action.actor
		&& action.actor->getFaction() == FACTION_PLAYER
		&& action.result.length() > 0 // kL_note: This queries the warning string message.
		&& _playerPanicHandled
		&& (_save->getSide() == FACTION_PLAYER || _debugPlay))
	{
		//Log(LOG_INFO) << ". actionFailed";
		_parentState->warning(action.result);

		actionFailed = true;

		// kL_begin: BattlescapeGame::popState(), remove action.Cursor if not enough tu's (ie, error.Message)
		if (action.result == "STR_NOT_ENOUGH_TIME_UNITS"
			|| action.result == "STR_NO_AMMUNITION_LOADED"
			|| action.result == "STR_NO_ROUNDS_LEFT")
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

	_deleted.push_back(_states.front()); // delete CTD

	//Log(LOG_INFO) << ". states.Popfront";
	_states.pop_front();
	//Log(LOG_INFO) << ". states.Popfront DONE";


//	getMap()->refreshSelectorPosition(); // kL, I moved this to setupCursor()

	// handle the end of this unit's actions
	if (action.actor
		&& noActionsPending(action.actor))
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
			if (action.targeting
				&& _save->getSelectedUnit()
				&& !actionFailed)
			{
				action.actor->spendTimeUnits(action.TU);
					// kL_query: Does this happen **before** ReactionFire/getReactor()?
					// no. not for shooting, but for throwing it does; actually no it doesn't.
					//
					// wtf, now RF works fine. NO IT DOES NOT.
				//Log(LOG_INFO) << ". . ID " << action.actor->getId()
				//	<< " spendTU = " << action.TU
				//	<< ", currentTU = " << action.actor->getTimeUnits();
			}

			if (_save->getSide() == FACTION_PLAYER)
			{
				//Log(LOG_INFO) << ". side -> Faction_Player";

				// after throwing, the cursor returns to default cursor, after shooting it stays in
				// targeting mode and the player can shoot again in the same mode (autoshot/snap/aimed)
				// kL_note: unless he/she is out of tu's
/*kL
				if ((action.type == BA_THROW || action.type == BA_LAUNCH) && !actionFailed)
				{
					// clean up the waypoints
					if (action.type == BA_LAUNCH)
					{
						_currentAction.waypoints.clear();
					}
					cancelCurrentAction(true);
				} */

				// kL_begin:
				if (!actionFailed)
				{
					int curTU = action.actor->getTimeUnits();

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
																action.weapon))
							{
								cancelCurrentAction(true);
							}
						break;
						case BA_AUTOSHOT:
							//Log(LOG_INFO) << ". AutoShot, TU percent = " << (float)action.weapon->getRules()->getTUAuto();
							if (curTU < action.actor->getActionTUs(
																BA_AUTOSHOT,
																action.weapon))
							{
								cancelCurrentAction(true);
							}
						break;
						case BA_AIMEDSHOT:
							//Log(LOG_INFO) << ". AimedShot, TU percent = " << (float)action.weapon->getRules()->getTUAimed();
							if (curTU < action.actor->getActionTUs(
																BA_AIMEDSHOT,
																action.weapon))
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
				_parentState->updateSoldierInfo(); // kL

//				getMap()->refreshSelectorPosition(); // kL
				setupCursor();
				_parentState->getGame()->getCursor()->setVisible(true);
				//Log(LOG_INFO) << ". end NOT actionFailed";
			}
		}
		else // not FACTION_PLAYER
		{
			//Log(LOG_INFO) << ". action -> NOT Faction_Player";
			action.actor->spendTimeUnits(action.TU); // spend TUs

			if (_save->getSide() != FACTION_PLAYER
				&& !_debugPlay)
			{
				 // AI does three things per unit, before switching to the next, or it got killed before doing the second thing
				if (_AIActionCounter > 2
					|| _save->getSelectedUnit() == NULL
					|| _save->getSelectedUnit()->isOut())
				{
					if (_save->getSelectedUnit())
					{
						_save->getSelectedUnit()->setCache(NULL);
						getMap()->cacheUnit(_save->getSelectedUnit());
					}

					_AIActionCounter = 0;

					if (_states.empty()
						&& _save->selectNextFactionUnit(true) == NULL)
					{
						if (!_save->getDebugMode())
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

					if (_save->getSelectedUnit())
						getMap()->getCamera()->centerOnPosition(_save->getSelectedUnit()->getPosition());
				}
			}
			else if (_debugPlay)
			{
//				getMap()->refreshSelectorPosition(); // kL
				setupCursor();
				_parentState->getGame()->getCursor()->setVisible(true);
			}
		}
	}

//	getMap()->refreshSelectorPosition(); // kL
	//Log(LOG_INFO) << ". uhm yeah";

	if (!_states.empty())
	{
		//Log(LOG_INFO) << ". NOT states.Empty";

		if (_states.front() == 0) // end turn request?
		{
			//Log(LOG_INFO) << ". states.front() == 0";
			while (!_states.empty())
			{
				//Log(LOG_INFO) << ". while (!_states.empty()";
				if (_states.front() == 0)
					_states.pop_front();
				else
					break;
			}

			if (_states.empty())
			{
				//Log(LOG_INFO) << ". endTurn()";
				endTurn();

				//Log(LOG_INFO) << ". endTurn() DONE return";
				return;
			}
			else
			{
				//Log(LOG_INFO) << ". states.front() != 0";
				_states.push_back(0);
			}
		}

		//Log(LOG_INFO) << ". states.front()->init()";
		_states.front()->init(); // init the next state in queue
	}

	// the currently selected unit died or became unconscious or disappeared inexplicably
	if (_save->getSelectedUnit() == NULL
		|| _save->getSelectedUnit()->isOut(true, true))
	{
		//Log(LOG_INFO) << ". huh hey wot)";
		cancelCurrentAction();
		_save->setSelectedUnit(NULL); // kL_note: seems redundant .....

//		getMap()->refreshSelectorPosition(); // kL
		getMap()->setCursorType(CT_NORMAL);
		_parentState->getGame()->getCursor()->setVisible(true);
	}

	if (_save->getSide() == FACTION_PLAYER) // kL
	{
		//Log(LOG_INFO) << ". updateSoldierInfo()";
 // kL	_parentState->updateSoldierInfo(); // that should be necessary only on xCom turns. See above^
		_parentState->updateSoldierInfo(); // kL: calcFoV ought have been done by now ...
	}

	//Log(LOG_INFO) << "BattlescapeGame::popState() EXIT";
}

/**
 * Determines whether there are any actions pending for the given unit.
 * @param bu BattleUnit.
 * @return True if there are no actions pending.
 */
bool BattlescapeGame::noActionsPending(BattleUnit* bu)
{
	if (_states.empty())
		return true;

	for (std::list<BattleState*>::iterator
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
 * @param interval, An interval in ms.
 */
void BattlescapeGame::setStateInterval(Uint32 interval)
{
	_parentState->setStateInterval(interval);
}

/**
 * Checks against reserved time units.
 * @param bu	- pointer to a unit
 * @param tu	- # of time units to check against
 * @param test	- true to suppress error messages
 * @return, true if unit has tu+ time units ( go! )
 */
bool BattlescapeGame::checkReservedTU(
		BattleUnit* bu,
		int tu,
		bool test)
{
	BattleActionType actionReserved = _tuReserved; // avoid changing _tuReserved here.

	// aLiens reserve TUs as a percentage rather than just enough for a single action.
	if (_save->getSide() != FACTION_PLAYER)
	{
		if (_save->getSide() == FACTION_NEUTRAL)
			return (tu <= bu->getTimeUnits());


		int rand = RNG::generate(0, 10); // kL, added in below ->

		// kL_note: This could use some tweaking, for the poor aLiens:
		switch (actionReserved)
		{
			case BA_SNAPSHOT:
				return (tu + rand + (bu->getStats()->tu / 3) <= bu->getTimeUnits());		// 33%
			break;
			case BA_AUTOSHOT:
//kL			return (tu + ((bu->getStats()->tu / 5) * 2) <= bu->getTimeUnits());
				return (tu + rand + (bu->getStats()->tu * 2 / 5) <= bu->getTimeUnits());	// 40%
			break;
			case BA_AIMEDSHOT:
				return (tu + rand + (bu->getStats()->tu / 2) <= bu->getTimeUnits());		// 50%
			break;

			default:
				return (tu + rand <= bu->getTimeUnits());
			break;
		}
	}

	// ** Below here is for xCom soldiers exclusively ***
	// ( which i don't care about )

	// check TUs against slowest weapon if we have two weapons
	BattleItem* slowestWeapon = bu->getMainHandWeapon(false);
	// kL_note: Use getActiveHand() instead, if xCom wants to reserve TU.
	// kL_note: make sure this doesn't work on aLiens, because getMainHandWeapon()
	// returns grenades and that can easily cause problems. Probably could cause
	// problems for xCom too, if xCom wants to reserve TU's in this manner.
	// note: won't return grenades anymore.
	// note note: did more work on getMainHandWeapon()

	// if the weapon has no autoshot, reserve TUs for snapshot
	if (bu->getActionTUs(
					_tuReserved,
					slowestWeapon)
				== 0
		&& _tuReserved == BA_AUTOSHOT)
	{
		actionReserved = BA_SNAPSHOT;
	}

	// likewise, if we don't have a snap shot available, try aimed.
	if (bu->getActionTUs(
					actionReserved,
					slowestWeapon)
				== 0
		&& _tuReserved == BA_SNAPSHOT)
	{
		actionReserved = BA_AIMEDSHOT;
	}

	const int tuKneel = (_kneelReserved && bu->getType() == "SOLDIER")? 4: 0;

	if ((actionReserved != BA_NONE
			|| _kneelReserved)
		&& tu + tuKneel + bu->getActionTUs(actionReserved, slowestWeapon) > bu->getTimeUnits()
		&& (tuKneel + bu->getActionTUs(actionReserved, slowestWeapon) <= bu->getTimeUnits()
			|| test))
	{
		if (!test)
		{
			if (tuKneel)
			{
				switch (actionReserved)
				{
					case BA_NONE:
						_parentState->warning("STR_TIME_UNITS_RESERVED_FOR_KNEELING");
					break;

					default:
						_parentState->warning("STR_TIME_UNITS_RESERVED_FOR_KNEELING_AND_FIRING");
					break;
				}
			}
			else
			{
				switch (actionReserved)
				{
					case BA_SNAPSHOT:
						_parentState->warning("STR_TIME_UNITS_RESERVED_FOR_SNAP_SHOT");
					break;
					case BA_AUTOSHOT:
						_parentState->warning("STR_TIME_UNITS_RESERVED_FOR_AUTO_SHOT");
					break;
					case BA_AIMEDSHOT:
						_parentState->warning("STR_TIME_UNITS_RESERVED_FOR_AIMED_SHOT");
					break;

					default:
					break;
				}
			}
		}

		return false;
	}

	return true;
}

/**
 * Picks the first soldier that is panicking.
 * @return True when all panicking is over.
 */
bool BattlescapeGame::handlePanickingPlayer()
{
	for (std::vector<BattleUnit*>::iterator
			j = _save->getUnits()->begin();
			j != _save->getUnits()->end();
			++j)
	{
		if ((*j)->getFaction() == FACTION_PLAYER
			&& (*j)->getOriginalFaction() == FACTION_PLAYER
			&& handlePanickingUnit(*j))
		{
			return false;
		}
	}

	return true;
}

/**
 * Common function for handling panicking units.
 * @return, False when unit not in panicking mode.
 */
bool BattlescapeGame::handlePanickingUnit(BattleUnit* unit)
{
	UnitStatus status = unit->getStatus();

	if (status != STATUS_PANICKING
		&& status != STATUS_BERSERK)
	{
		return false;
	}

	//Log(LOG_INFO) << "unit Panic/Berserk : " << unit->getId() << " / " << unit->getMorale();
	_save->setSelectedUnit(unit);
//	unit->setVisible(true); // kL

	_parentState->getMap()->setCursorType(CT_NONE);

	// show a little infobox with the name of the unit and "... is panicking"
	if (unit->getVisible()
		|| !Options::noAlienPanicMessages)
	{
		getMap()->getCamera()->centerOnPosition(unit->getPosition());

		Game* game = _parentState->getGame();
		if (status == STATUS_PANICKING)
			game->pushState(new InfoboxState(game->getLanguage()->getString("STR_HAS_PANICKED", unit->getGender())
																			.arg(unit->getName(game->getLanguage()))));
		else
			game->pushState(new InfoboxState(game->getLanguage()->getString("STR_HAS_GONE_BERSERK", unit->getGender())
																			.arg(unit->getName(game->getLanguage()))));
	}

//kL	unit->abortTurn(); // makes the unit go to status STANDING :p
	unit->setStatus(STATUS_STANDING); // kL :P

	int flee = RNG::generate(0, 99);
	BattleAction ba;
	ba.actor = unit;

	switch (status)
	{
		case STATUS_PANICKING: // 1/2 chance to freeze and 1/2 chance try to flee
			if (flee <= 50)
			{
				BattleItem* item = unit->getItem("STR_RIGHT_HAND");
				if (item)
					dropItem(
							unit->getPosition(),
							item,
							false,
							true);

				item = unit->getItem("STR_LEFT_HAND");
				if (item)
					dropItem(
							unit->getPosition(),
							item,
							false,
							true);

				unit->setCache(0);

				ba.target = Position(
								unit->getPosition().x + RNG::generate(-5, 5),
								unit->getPosition().y + RNG::generate(-5, 5),
								unit->getPosition().z);
				if (_save->getTile(ba.target)) // only walk towards it when the place exists
				{
					_save->getPathfinding()->calculate(
													ba.actor,
													ba.target);

					statePushBack(new UnitWalkBState(
													this,
													ba));
				}
			}
		break;
		case STATUS_BERSERK:	// berserk - do some weird turning around and then aggro
								// towards an enemy unit or shoot towards random place
			ba.type = BA_TURN;
			for (int
					i = 0;
					i < 4;
					i++)
			{
				ba.target = Position(
								unit->getPosition().x + RNG::generate(-5, 5),
								unit->getPosition().y + RNG::generate(-5, 5),
								unit->getPosition().z);
				statePushBack(new UnitTurnBState(
												this,
												ba));
			}

			for (std::vector<BattleUnit*>::iterator
					j = unit->getVisibleUnits()->begin();
					j != unit->getVisibleUnits()->end();
					++j)
			{
				ba.target = (*j)->getPosition();
				statePushBack(new UnitTurnBState(
												this,
												ba));
			}

			if (_save->getTile(ba.target) != 0)
			{
				ba.weapon = unit->getMainHandWeapon();
				if (ba.weapon)
				{
					if (ba.weapon->getRules()->getBattleType() == BT_FIREARM)
					{
						ba.type = BA_SNAPSHOT;
						int tu = ba.actor->getActionTUs(
													ba.type,
													ba.weapon);

						for (int // fire shots until unit runs out of TUs
								i = 0;
								i < 10;
								i++)
						{
							if (!ba.actor->spendTimeUnits(tu))
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

			unit->setTimeUnits(unit->getStats()->tu); // replace the TUs from shooting
			ba.type = BA_NONE;
		break;

		default:
		break;
	}

	statePushBack(new UnitPanicBState( // Time units can only be reset after everything else occurs
									this,
									ba.actor));

	unit->moraleChange(+15);

	//Log(LOG_INFO) << "unit Panic/Berserk : " << unit->getId() << " / " << unit->getMorale();
	return true;
}

/**
  * Cancels the current action the user had selected (firing, throwing,..)
  * @param bForce - force the action to be cancelled
  * @return, true if action was cancelled
  */
bool BattlescapeGame::cancelCurrentAction(bool bForce)
{
	//Log(LOG_INFO) << "BattlescapeGame::cancelCurrentAction()";
	bool bPreviewed = (Options::battleNewPreviewPath != PATH_NONE);

	if (_save->getPathfinding()->removePreview()
		&& bPreviewed)
	{
		return true;
	}

	if (_states.empty()
		|| bForce)
	{
		if (_currentAction.targeting)
		{
			if (_currentAction.type == BA_LAUNCH
				&& !_currentAction.waypoints.empty())
			{
				_currentAction.waypoints.pop_back();

				if (!getMap()->getWaypoints()->empty())
					getMap()->getWaypoints()->pop_back();

				if (_currentAction.waypoints.empty())
					_parentState->showLaunchButton(false);

				return true;
			}
			else
			{
				if (Options::battleConfirmFireMode
					&& !_currentAction.waypoints.empty())
				{
					_currentAction.waypoints.pop_back();
					getMap()->getWaypoints()->pop_back();

					return true;
				}

				_currentAction.targeting = false;
				_currentAction.type = BA_NONE;

				setupCursor();
				getMap()->refreshSelectorPosition(); // kL
				_parentState->getGame()->getCursor()->setVisible(true);

				return true;
			}
		}
	}
	else if (!_states.empty()
		&& _states.front() != 0)
	{
		_states.front()->cancel();

		return true;
	}

	return false;
}

/**
 * Gets a pointer to access action members directly.
 * @return, Pointer to action.
 */
BattleAction* BattlescapeGame::getCurrentAction()
{
	return &_currentAction;
}

/**
 * Determines whether an action is currently going on?
 * @return, true or false.
 */
bool BattlescapeGame::isBusy()
{
	return !_states.empty();
}

/**
 * Activates primary action (left click).
 * @param pos, Position on the map.
 */
void BattlescapeGame::primaryAction(const Position& pos)
{
	//Log(LOG_INFO) << "BattlescapeGame::primaryAction()"; // unitID = " << _currentAction.actor->getId();
	// kL_debug:
//	std::string sUnit = "none selected";
/*	int iUnit = 0;
	if (_save->getSelectedUnit())
	{
		iUnit = _save->getSelectedUnit()->getId();
		//Log(LOG_INFO) << ". selectedUnit " << iUnit;
	}
	else if (_currentAction.actor)
	{
		//Log(LOG_INFO) << ". action.Actor " << iUnit;
		iUnit = _currentAction.actor->getId();
	} */ // kL_end.
	//Log(LOG_INFO) << ". action.TU = " << _currentAction.TU;


	bool bPreviewed = (Options::battleNewPreviewPath != PATH_NONE);

	if (_currentAction.targeting
		&& _save->getSelectedUnit())
	{
		//Log(LOG_INFO) << ". . _currentAction.targeting";
		_currentAction.strafe = false; // kL

		if (_currentAction.type == BA_LAUNCH) // click to set BL waypoints.
		{
			//Log(LOG_INFO) << ". . . . BA_LAUNCH";
			_parentState->showLaunchButton(true);

			_currentAction.waypoints.push_back(pos);
			getMap()->getWaypoints()->push_back(pos);
		}
		else if (_currentAction.type == BA_USE
			&& _currentAction.weapon->getRules()->getBattleType() == BT_MINDPROBE)
		{
			//Log(LOG_INFO) << ". . . . BA_USE -> BT_MINDPROBE";
			if (_save->selectUnit(pos)
				&& _save->selectUnit(pos)->getFaction() != _save->getSelectedUnit()->getFaction()
				&& _save->selectUnit(pos)->getVisible())
			{
				if (!_currentAction.weapon->getRules()->isLOSRequired()
					|| std::find(
							_currentAction.actor->getVisibleUnits()->begin(),
							_currentAction.actor->getVisibleUnits()->end(),
							_save->selectUnit(pos))
						!= _currentAction.actor->getVisibleUnits()->end())
				{
					if (_currentAction.actor->spendTimeUnits(_currentAction.TU)) // kL_note: Should this be getActionTUs() to account for flatRates?
					{
						_parentState->getGame()->getResourcePack()->getSound(
																		"BATTLE.CAT",
																		_currentAction.weapon->getRules()->getHitSound())
																	->play();
						_parentState->getGame()->pushState(new UnitInfoState(
																		_save->selectUnit(pos),
																		_parentState,
																		false,
																		true));

						cancelCurrentAction();
						// kL_note: where is updateSoldierInfo() - is that done when unitInfoState is dismissed?
					}
					else
					{
						cancelCurrentAction(); // kL
						_parentState->warning("STR_NOT_ENOUGH_TIME_UNITS");
					}
				}
				else
				{
//					cancelCurrentAction(); // kL
					_parentState->warning("STR_NO_LINE_OF_FIRE");
				}
			}
		}
		else if (_currentAction.type == BA_PANIC
			|| _currentAction.type == BA_MINDCONTROL)
		{
			//Log(LOG_INFO) << ". . . . BA_PANIC or BA_MINDCONTROL";
			if (_save->selectUnit(pos)
				&& _save->selectUnit(pos)->getFaction() != _save->getSelectedUnit()->getFaction()
				&& _save->selectUnit(pos)->getVisible())
			{
				bool builtinpsi = !_currentAction.weapon;
				if (builtinpsi)
				{
					_currentAction.weapon = new BattleItem(
													_parentState->getGame()->getRuleset()->getItem("ALIEN_PSI_WEAPON"),
													_save->getCurrentItemId());
				}

				_currentAction.TU = _currentAction.actor->getActionTUs(
																	_currentAction.type,
																	_currentAction.weapon);
				_currentAction.target = pos;
				if (!_currentAction.weapon->getRules()->isLOSRequired()
					|| std::find(
							_currentAction.actor->getVisibleUnits()->begin(),
							_currentAction.actor->getVisibleUnits()->end(),
							_save->selectUnit(pos))
						!= _currentAction.actor->getVisibleUnits()->end())
				{
					// get the sound/animation started
//kL				getMap()->setCursorType(CT_NONE);
//kL				_parentState->getGame()->getCursor()->setVisible(false);
//kL				_currentAction.cameraPosition = getMap()->getCamera()->getMapOffset();
					_currentAction.cameraPosition = Position(0, 0,-1); // kL (don't adjust the camera when coming out of ProjectileFlyB/ExplosionB states)


					//Log(LOG_INFO) << ". . . . . . new ProjectileFlyBState";
					statePushBack(new ProjectileFlyBState(
														this,
														_currentAction));

					if (_currentAction.TU <= _currentAction.actor->getTimeUnits()) // kL_note: WAIT, check this *before* all the stuff above!!!
					{
						if (getTileEngine()->psiAttack(&_currentAction))
						{
							//Log(LOG_INFO) << ". . . . . . Psi successful";
							Game* game = _parentState->getGame(); // show a little infobox if it's successful
							if (_currentAction.type == BA_PANIC)
							{
								//Log(LOG_INFO) << ". . . . . . . . BA_Panic";
								game->pushState(new InfoboxState(game->getLanguage()->getString("STR_MORALE_ATTACK_SUCCESSFUL")));
							}
							else //if (_currentAction.type == BA_MINDCONTROL)
							{
								//Log(LOG_INFO) << ". . . . . . . . BA_MindControl";
								game->pushState(new InfoboxState(game->getLanguage()->getString("STR_MIND_CONTROL_SUCCESSFUL")));
							}

							//Log(LOG_INFO) << ". . . . . . updateSoldierInfo()";
//kL						_parentState->updateSoldierInfo();
							_parentState->updateSoldierInfo(false); // kL
							//Log(LOG_INFO) << ". . . . . . updateSoldierInfo() DONE";


							// kL_begin: BattlescapeGame::primaryAction(), not sure where this bit came from.....
							// it doesn't seem to be in the official oXc but it might
							// stop some (psi-related) crashes i'm getting; (no, it was something else..)
							// but then it probably never runs because I doubt that selectedUnit can be other than xCom.
							// (yes, selectedUnit is currently operating unit of *any* faction)
//							if (_save->getSelectedUnit()->getFaction() != FACTION_PLAYER)
//							{
//							_currentAction.targeting = false;
//							_currentAction.type = BA_NONE;
//							}
//							setupCursor();


							// kL_note: might need to put a refresh (redraw/blit) cursor here;
							// else it 'sticks' for a moment at its previous position.
//							_parentState->getMap()->refreshSelectorPosition();			// kL

//							getMap()->setCursorType(CT_NONE);							// kL
//							_parentState->getGame()->getCursor()->setVisible(false);	// kL
//							_parentState->getMap()->refreshSelectorPosition();			// kL
							// kL_end.

							//Log(LOG_INFO) << ". . . . . . inVisible cursor, DONE";
						}
					}
					else													// kL
					{
						cancelCurrentAction();								// kL
						_parentState->warning("STR_NOT_ENOUGH_TIME_UNITS");	// kL
					}
				}
				else
				{
//					cancelCurrentAction();									// kL
					_parentState->warning("STR_NO_LINE_OF_FIRE");
				}

				if (builtinpsi)
				{
					_save->removeItem(_currentAction.weapon);
					_currentAction.weapon = 0;
				}
			}
		}
		else if (Options::battleConfirmFireMode
			&& (_currentAction.waypoints.empty()
				|| pos != _currentAction.waypoints.front()))
		{
			_currentAction.waypoints.clear();
			_currentAction.waypoints.push_back(pos);

			getMap()->getWaypoints()->clear();
			getMap()->getWaypoints()->push_back(pos);
		}
		else
		{
			//Log(LOG_INFO) << ". . . . Firing or Throwing";
			getMap()->setCursorType(CT_NONE);

			if (Options::battleConfirmFireMode)
			{
				_currentAction.waypoints.clear();
				getMap()->getWaypoints()->clear();
			}

			_parentState->getGame()->getCursor()->setVisible(false);

			_currentAction.target = pos;
			_currentAction.cameraPosition = getMap()->getCamera()->getMapOffset();

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
		_currentAction.actor = _save->getSelectedUnit();

		BattleUnit* unit = _save->selectUnit(pos);
		if (unit
			&& unit != _save->getSelectedUnit()
			&& (unit->getVisible() || _debugPlay))
		{
			//  -= select unit =-
			if (unit->getFaction() == _save->getSide())
			{
				_save->setSelectedUnit(unit);
				_parentState->updateSoldierInfo();

				cancelCurrentAction();
				setupCursor();

				_currentAction.actor = unit;
			}
		}
		else if (playableUnitSelected())
		{
			bool mod_CTRL = (SDL_GetModState() & KMOD_CTRL) != 0;
			bool mod_ALT = (SDL_GetModState() & KMOD_ALT) != 0; // kL
			if (bPreviewed
				&& (_currentAction.target != pos
					|| _save->getPathfinding()->isModCTRL() != mod_CTRL
					|| _save->getPathfinding()->isModALT() != mod_ALT)) // kL
			{
				_save->getPathfinding()->removePreview();
			}

			_currentAction.actor->setDashing(false); // kL
			_currentAction.run = false;
			_currentAction.strafe = Options::strafe
									&& ((mod_CTRL
											&& _save->getSelectedUnit()->getTurretType() == -1)
										|| (mod_ALT // tank, reverse gear 1 tile only.
											&& _save->getSelectedUnit()->getTurretType() > -1));
			//Log(LOG_INFO) << ". primary action: Strafe";
			if (_currentAction.strafe
				&& (_save->getTileEngine()->distance(
												_currentAction.actor->getPosition(),
												pos)
											> 1
					|| _currentAction.actor->getPosition().z != pos.z) // kL
				&& _save->getSelectedUnit()->getTurretType() == -1) // kL: tanks don't dash.
			{
				_currentAction.actor->setDashing(true); // kL, do this in UnitWalkBState
				// kL_note: I just realized that action.run could be used instead of set/getDashing() ...
				// leave it as is for now; for use in TileEngine, reaction fire
				_currentAction.run = true;
				_currentAction.strafe = false;
			}

			_currentAction.target = pos;
			_save->getPathfinding()->calculate( // get the Path.
											_currentAction.actor,
											_currentAction.target);

			if (_save->getPathfinding()->getStartDirection() != -1) // kL -> assumes removePreview() doesn't change StartDirection
			{
				if (bPreviewed
					&& !_save->getPathfinding()->previewPath())
//kL				&& _save->getPathfinding()->getStartDirection() != -1)
				{
					//Log(LOG_INFO) << "primary: bPreviewed";
					_save->getPathfinding()->removePreview();
					bPreviewed = false;
				}

				if (!bPreviewed)
//kL				&& _save->getPathfinding()->getStartDirection() != -1)
				{
					//Log(LOG_INFO) << "primary: !bPreviewed";
					//  -= start walking =-
					getMap()->setCursorType(CT_NONE);
					_parentState->getGame()->getCursor()->setVisible(false);

					statePushBack(new UnitWalkBState(
													this,
													_currentAction));
				}
			}
		}
	}
	//Log(LOG_INFO) << "BattlescapeGame::primaryAction() EXIT";
}

/**
 * Activates secondary action (right click).
 * @param pos Position on the map.
 */
void BattlescapeGame::secondaryAction(const Position& pos)
{
	//Log(LOG_INFO) << "BattlescapeGame::secondaryAction()";
	_currentAction.actor = _save->getSelectedUnit();

	Position unitPos = _currentAction.actor->getPosition();	// kL
	if (pos == unitPos)										// kL
	{
		// could put just about anything in here Orelly.
		_currentAction.actor = NULL;						// kL

		return;												// kL
	}

	// -= turn to or open door =-
	_currentAction.target = pos;
	_currentAction.strafe = Options::strafe
							&& (SDL_GetModState() & KMOD_CTRL) != 0
							&& _save->getSelectedUnit()->getTurretType() > -1;

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
	_currentAction.weapon		= 0;
	_currentAction.targeting	= true;
	_currentAction.type			= BA_PANIC;
	_currentAction.TU			= 25;	// kL_note: this is just a default i guess;
										// otherwise it should be getActionTUs()
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
	//Log(LOG_INFO) << "BattlescapeGame::moveUpDown()";
	_currentAction.target = unit->getPosition();

	if (dir == Pathfinding::DIR_UP)
	{
		if ((SDL_GetModState() & KMOD_ALT) != 0) // kL
			return;

		_currentAction.target.z++;
	}
	else
		_currentAction.target.z--;

	getMap()->setCursorType(CT_NONE);
	_parentState->getGame()->getCursor()->setVisible(false);

	// kL_note: taking this out so I can go up/down *kneeled* on GravLifts.
	// might be a problem with soldiers in flying suits, later...
/*kL	if (_save->getSelectedUnit()->isKneeled())
	{
		//Log(LOG_INFO) << "BattlescapeGame::moveUpDown()";

		kneel(_save->getSelectedUnit());
	} */

	// kL_begin:
	_currentAction.strafe = false; // kL, redundancy checks ......
	_currentAction.run = false;
	_currentAction.actor->setDashing(false);

	if (Options::strafe
		&& (SDL_GetModState() & KMOD_CTRL) != 0
		&& unit->getArmor()->getSize() == 1)
	{
		_currentAction.run = true;
		_currentAction.actor->setDashing(true);
	} // kL_end.

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
	//Log(LOG_INFO) << "BattlescapeGame::requestEndTurn()";
	cancelCurrentAction();

	if (!_endTurnRequested)
	{
		_endTurnRequested = true;

		statePushBack(0);
	}

	//Log(LOG_INFO) << "BattlescapeGame::requestEndTurn() EXIT";
}

/**
 * Sets the TU reserved type.
 * @param tur A battleactiontype.
 * @param player is this requested by the player?
 */
void BattlescapeGame::setTUReserved(
		BattleActionType tur,
		bool player)
{
	_tuReserved = tur;

	if (player)
	{
		_playerTUReserved = tur;
		_save->setTUReserved(tur);
	}
}

/**
 * Drops an item to the floor and affects it with gravity.
 * @param position Position to spawn the item.
 * @param item Pointer to the item.
 * @param newItem Bool whether this is a new item.
 * @param removeItem Bool whether to remove the item from the owner.
 */
void BattlescapeGame::dropItem(
		const Position& position,
		BattleItem* item,
		bool newItem,
		bool removeItem)
{
	Position pos = position;

	// don't spawn anything outside of bounds
	if (_save->getTile(pos) == 0)
		return;

	// don't ever drop fixed items
	if (item->getRules()->isFixed())
		return;

	_save->getTile(pos)->addItem(item, getRuleset()->getInventory("STR_GROUND"));

	if (item->getUnit())
		item->getUnit()->setPosition(pos);

	if (newItem)
		_save->getItems()->push_back(item);
	else if (_save->getSide() != FACTION_PLAYER)
		item->setTurnFlag(true);

	if (removeItem)
		item->moveToOwner(0);
	else if (item->getRules()->getBattleType() != BT_GRENADE
		&& item->getRules()->getBattleType() != BT_PROXIMITYGRENADE)
	{
		item->setOwner(0);
	}

	getTileEngine()->applyGravity(_save->getTile(pos));

	if (item->getRules()->getBattleType() == BT_FLARE)
	{
		getTileEngine()->calculateTerrainLighting();
		getTileEngine()->calculateFOV(position);
	}
}

/**
 * Converts a unit into a unit of another type.
 * @param unit		- pointer to a unit to convert
 * @param newType	- the type of unit to convert to
 * @return, pointer to the new unit
 */
BattleUnit* BattlescapeGame::convertUnit(
		BattleUnit* unit,
		std::string newType)
{
	getSave()->getBattleState()->showPsiButton(false);
	// in case the unit was unconscious
	getSave()->removeUnconsciousBodyItem(unit);

//	int origDir = unit->getDirection(); // kL
	unit->instaKill();

	if (Options::battleNotifyDeath
		&& unit->getFaction() == FACTION_PLAYER
		&& unit->getOriginalFaction() == FACTION_PLAYER)
	{
		_parentState->getGame()->pushState(new InfoboxState(_parentState->getGame()->getLanguage()->getString("STR_HAS_BEEN_KILLED", unit->getGender())
																							.arg(unit->getName(_parentState->getGame()->getLanguage()))));
	}

	for (std::vector<BattleItem*>::iterator
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
	getSave()->getTile(unit->getPosition())->setUnit(NULL);


	std::ostringstream newArmor;
	newArmor << getRuleset()->getUnit(newType)->getArmor();

	std::string terroristWeapon = getRuleset()->getUnit(newType)->getRace().substr(4);
	terroristWeapon += "_WEAPON";
	RuleItem* newItem = getRuleset()->getItem(terroristWeapon);

	int difficulty = static_cast<int>(_parentState->getGame()->getSavedGame()->getDifficulty());
	int month = _parentState->getGame()->getSavedGame()->getMonthsPassed(); // kL
	BattleUnit* newUnit = new BattleUnit(
									getRuleset()->getUnit(newType),
									FACTION_HOSTILE,
									_save->getUnits()->back()->getId() + 1,
									getRuleset()->getArmor(newArmor.str()),
									difficulty,
									month, // kL_add.
									this); // kL_add
	// kL_note: what about setting _zombieUnit=true ? It's not generic but it's the only case, afaict

//kL	if (!difficulty) // kL_note: moved to BattleUnit::adjustStats()
//kL		newUnit->halveArmor();

	getSave()->getTile(unit->getPosition())->setUnit(
												newUnit,
												_save->getTile(unit->getPosition() + Position(0, 0,-1)));
	newUnit->setPosition(unit->getPosition());
	newUnit->setDirection(3);
//	newUnit->setDirection(origDir);		// kL
//	this->getMap()->cacheUnit(newUnit);	// kL, try this. ... damn!
	newUnit->setCache(0);
	newUnit->setTimeUnits(0);

	getSave()->getUnits()->push_back(newUnit);
	getMap()->cacheUnit(newUnit);
	newUnit->setAIState(new AlienBAIState(
										getSave(),
										newUnit,
										0));

	BattleItem* bi = new BattleItem(
								newItem,
								getSave()->getCurrentItemId());
	bi->moveToOwner(newUnit);
	bi->setSlot(getRuleset()->getInventory("STR_RIGHT_HAND"));
	getSave()->getItems()->push_back(bi);

	getTileEngine()->applyGravity(newUnit->getTile());
	getTileEngine()->calculateFOV(newUnit->getPosition());

	if (unit->getFaction() == FACTION_PLAYER)
		newUnit->setVisible(true);

//	newUnit->getCurrentAIState()->think();

	return newUnit;
}

/**
 * Gets the map.
 * @return map.
 */
Map* BattlescapeGame::getMap()
{
	return _parentState->getMap();
}

/**
 * Gets the save.
 * @return save.
 */
SavedBattleGame* BattlescapeGame::getSave()
{
	return _save;
}

/**
 * Gets the tilengine.
 * @return tilengine.
 */
TileEngine* BattlescapeGame::getTileEngine()
{
	return _save->getTileEngine();
}

/**
 * Gets the pathfinding.
 * @return pathfinding.
 */
Pathfinding* BattlescapeGame::getPathfinding()
{
	return _save->getPathfinding();
}

/**
 * Gets the resourcepack.
 * @return resourcepack.
 */
ResourcePack* BattlescapeGame::getResourcePack()
{
	return _parentState->getGame()->getResourcePack();
}

/**
 * Gets the ruleset.
 * @return ruleset.
 */
const Ruleset* BattlescapeGame::getRuleset() const
{
	return _parentState->getGame()->getRuleset();
}

/**
 * Tries to find an item and pick it up if possible.
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
					if (!targetItem->getAmmoItem())										// if it isn't loaded or it is ammo
						action->actor->checkAmmo();										// try to load our weapon
				}
			}
			else if (!targetItem->getTile()->getUnit()									// if we're not standing on it, we should try to get to it.
				|| targetItem->getTile()->getUnit()->isOut(true, true))
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
 * @param action A pointer to the action being performed.
 * @return The item to attempt to take.
 */
BattleItem* BattlescapeGame::surveyItems(BattleAction* action)
{
	//Log(LOG_INFO) << "BattlescapeGame::surveyItems()";
	std::vector<BattleItem*> droppedItems;

	// first fill a vector with items on the ground
	// that were dropped on the alien turn [not]
	// and have an attraction value.
	// kL_note: And no one else is standing on it.
	for (std::vector<BattleItem*>::iterator
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

	BattleItem* targetItem = 0;
	int maxWorth = 0;

	// now select the most suitable candidate depending on attractiveness and distance
	for (std::vector<BattleItem*>::iterator
			i = droppedItems.begin();
			i != droppedItems.end();
			++i)
	{
		int currentWorth =
				(*i)->getRules()->getAttraction() /
					((_save->getTileEngine()->distance(
													action->actor->getPosition(),
													(*i)->getTile()->getPosition())
												* 2)
											+ 1);
		if (currentWorth > maxWorth)
		{
			maxWorth = currentWorth;
			targetItem = *i;
		}
	}

	return targetItem;
}

/**
 * Assesses whether this item is worth trying to pick up, taking into account
 * how many units we see, whether or not the Weapon has ammo, and if we have
 * ammo FOR it, or if it's ammo, checks if we have the weapon to go with it;
 * assesses the attraction value of the item and compares it with the distance
 * to the object, then returns false anyway.
 * @param item, The item to attempt to take.
 * @param action, A pointer to the action being performed.
 * @return, FALSE. (unless mods have been made to ruleset/code)
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

		// it's always going to be worth while [NOT] to try and take a blaster launcher, apparently;
		// too bad the aLiens don't know how to use them very well
//kL		if (!item->getRules()->isWaypoint() &&
	if (item->getRules()->getBattleType() != BT_AMMO)
	{
		// we only want weapons that HAVE ammo, or weapons that we have ammo for
		bool ammoFound = true;
		if (!item->getAmmoItem())
		{
			ammoFound = false;

			for (std::vector<BattleItem*>::iterator
					i = action->actor->getInventory()->begin();
					i != action->actor->getInventory()->end()
						&& !ammoFound;
					++i)
			{
				if ((*i)->getRules()->getBattleType() == BT_AMMO)
				{
					for (std::vector<std::string>::iterator
							j = item->getRules()->getCompatibleAmmo()->begin();
							j != item->getRules()->getCompatibleAmmo()->end()
								&& !ammoFound;
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

		if (!ammoFound)
			return false;
	}

	if (item->getRules()->getBattleType() == BT_AMMO)
	{
		// similar to the above, but this time we're checking if the ammo is suitable for a weapon we have.
		bool weaponFound = false;
		for (std::vector<BattleItem*>::iterator
				i = action->actor->getInventory()->begin();
				i != action->actor->getInventory()->end()
					&& !weaponFound;
				++i)
		{
			if ((*i)->getRules()->getBattleType() == BT_FIREARM)
			{
				for (std::vector<std::string>::iterator
						j = (*i)->getRules()->getCompatibleAmmo()->begin();
						j != (*i)->getRules()->getCompatibleAmmo()->end()
							&& !weaponFound;
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

		if (!weaponFound)
			return false;
	}
//	}

//	if (worth)
//	{
	// use bad logic to determine if we'll have room for the item
	int freeSlots = 25;

	for (std::vector<BattleItem*>::iterator
			i = action->actor->getInventory()->begin();
			i != action->actor->getInventory()->end();
			++i)
	{
		freeSlots -= (*i)->getRules()->getInventoryHeight() * (*i)->getRules()->getInventoryWidth();
	}

	int size = item->getRules()->getInventoryHeight() * item->getRules()->getInventoryWidth();
	if (freeSlots < size)
		return false;
//	}

	// return false for any item that we aren't standing directly
	// on top of with an attraction value less than 6 (aka always) [NOT.. anymore]

	return
		(worth -
			(_save->getTileEngine()->distance(
											action->actor->getPosition(),
											item->getTile()->getPosition()) * 2))
//			> 5;
			> 0;
}

/**
 * Picks the item up from the ground.
 *
 * At this point we've decided it's worth our while to grab this item, so we
 * try to do just that. First we check to make sure we have time units, then
 * that we have space (using horrifying logic!) then we attempt to actually
 * recover the item.
 * @param item, The item to attempt to take.
 * @param action, A pointer to the action being performed.
 * @return, 0 if successful, 1 for no-TUs, 2 for not-enough-room, 3 for won't-fit and -1 for something-went-horribly-wrong.
 */
int BattlescapeGame::takeItemFromGround(
		BattleItem* item,
		BattleAction* action)
{
	//Log(LOG_INFO) << "BattlescapeGame::takeItemFromGround()";
	const int TAKEITEM_ERROR	= -1;
	const int TAKEITEM_SUCCESS	=  0;
	const int TAKEITEM_NOTU		=  1;
	const int TAKEITEM_NOSPACE	=  2;
	const int TAKEITEM_NOFIT	=  3;

	int freeSlots = 25;

	// make sure we have time units
	if (action->actor->getTimeUnits() < 6)
		return TAKEITEM_NOTU;
	else
	{
		// check to make sure we have enough space by checking all the sizes of items in our inventory
		for (std::vector<BattleItem*>::iterator
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
			if (takeItem(item, action))
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
 * @param item, The item to attempt to take.
 * @param action, A pointer to the action being performed.
 * @return, Whether or not the item was successfully retrieved.
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
				if (action->actor->getItem("STR_RIGHT_HAND")->setAmmoItem(item) == NULL)
					placed = true;
			}
			else
			{
				for (int
						i = 0;
						i < 4;
						++i)
				{
					if (!action->actor->getItem("STR_BELT", i))
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
				if (!action->actor->getItem("STR_BELT", i))
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
			if (!action->actor->getItem("STR_RIGHT_HAND"))
			{
				item->moveToOwner(action->actor);
				item->setSlot(rules->getInventory("STR_RIGHT_HAND"));

				placed = true;
			}
		break;
		case BT_MEDIKIT:
		case BT_SCANNER:
			if (!action->actor->getItem("STR_BACK_PACK"))
			{
				item->moveToOwner(action->actor);
				item->setSlot(rules->getInventory("STR_BACK_PACK"));

				placed = true;
			}
		break;
		case BT_MINDPROBE:
			if (!action->actor->getItem("STR_LEFT_HAND"))
			{
				item->moveToOwner(action->actor);
				item->setSlot(rules->getInventory("STR_LEFT_HAND"));

				placed = true;
			}
		break;
		default:
		break;
	}

	return placed;
}

/**
 * Returns the action type that is reserved.
 * @return, The type of action that is reserved.
 */
BattleActionType BattlescapeGame::getReservedAction()
{
	return _tuReserved;
}

/**
 * Tallies the living units in the game and, if required, converts units into their spawn unit.
 * @param &liveAliens, The integer in which to store the live alien tally.
 * @param &liveSoldiers, The integer in which to store the live XCom tally.
 * @param convert, Should we convert infected units?
 */
void BattlescapeGame::tallyUnits(
		int& liveAliens,
		int& liveSoldiers,
		bool convert)
{
	//Log(LOG_INFO) << "BattlescapeGame::tallyUnits()";
	liveSoldiers = 0;
	liveAliens = 0;

	if (convert)
	{
		for (std::vector<BattleUnit*>::iterator
				j = _save->getUnits()->begin();
				j != _save->getUnits()->end();
				++j)
		{
//kL		if ((*j)->getHealth() > 0
			if ((*j)->getSpecialAbility() == SPECAB_RESPAWN)
			{
				//Log(LOG_INFO) << "BattlescapeGame::tallyUnits() " << (*j)->getId() << " : health > 0, SPECAB_RESPAWN -> converting unit!";

				(*j)->setSpecialAbility(SPECAB_NONE);
				convertUnit(
						*j,
						(*j)->getSpawnUnit());

				j = _save->getUnits()->begin();
			}
		}
	}

	for (std::vector<BattleUnit*>::iterator
			j = _save->getUnits()->begin();
			j != _save->getUnits()->end();
			++j)
	{
		if (!(*j)->isOut())
		{
			if ((*j)->getOriginalFaction() == FACTION_HOSTILE)
			{
				if (!Options::allowPsionicCapture
					|| (*j)->getFaction() != FACTION_PLAYER)
				{
					liveAliens++;
				}
			}
			else if ((*j)->getOriginalFaction() == FACTION_PLAYER)
			{
				if ((*j)->getFaction() == FACTION_PLAYER)
					liveSoldiers++;
				else
					liveAliens++;
			}
		}
	}

	//Log(LOG_INFO) << "BattlescapeGame::tallyUnits() EXIT";
}

/**
 * Sets the kneel reservation setting.
 * @param reserved Should we reserve an extra 4 TUs to kneel?
 */
void BattlescapeGame::setKneelReserved(bool reserved)
{
	_kneelReserved = reserved;
	_save->setKneelReserved(reserved);
}

/**
 * Gets the kneel reservation setting.
 * @return Kneel reservation setting.
 */
bool BattlescapeGame::getKneelReserved()
{
	if (_save->getSelectedUnit()
		&& _save->getSelectedUnit()->getGeoscapeSoldier())
	{
		return _kneelReserved;
	}

	return false;
}

/**
 * Checks if a unit has moved next to a proximity grenade.
 * Checks one tile around the unit in every direction.
 * For a large unit we check every tile it occupies.
 * @param unit - pointer to a unit
 * @return, true if a proximity grenade was triggered
 */
bool BattlescapeGame::checkForProximityGrenades(BattleUnit* unit)
{
	int size = unit->getArmor()->getSize() - 1;
	for (int
			x = size;
			x > -1;
			x--)
	{
		for (int
				y = size;
				y > -1;
				y--)
		{
			for (int
					tx = -1;
					tx < 2;
					tx++)
			{
				for (int
						ty = -1;
						ty < 2;
						ty++)
				{
					Tile* t = _save->getTile(unit->getPosition() + Position(x, y, 0) + Position(tx, ty, 0));

					int dir; // cred: animal310 - http://openxcom.org/bugs/openxcom/issues/765
					_save->getPathfinding()->vectorToDirection(
															Position(tx, ty, 0),
															dir);

					if (t
						&& _save->getPathfinding()->isBlocked(
															_save->getTile(Position(x, y, 0)),
															NULL,
															dir)
														== false)
					{
						for (std::vector<BattleItem*>::iterator
								i = t->getInventory()->begin();
								i != t->getInventory()->end();
								++i)
						{
							if ((*i)->getRules()->getBattleType() == BT_PROXIMITYGRENADE
								&& (*i)->getFuseTimer() == 0)
							{
								Position pos;
								pos.x = t->getPosition().x * 16 + 8;
								pos.y = t->getPosition().y * 16 + 8;
								pos.z = t->getPosition().z * 24 + t->getTerrainLevel();

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

	return false;
}

/**
 *
 */
void BattlescapeGame::cleanupDeleted()
{
	for (std::list<BattleState*>::iterator
			i = _deleted.begin();
			i != _deleted.end();
			++i)
	{
		delete *i;
	}

	_deleted.clear();
}

/**
 * kL. Gets the BattlescapeState.
 * For turning on/off the visUnits indicators from UnitWalk/TurnBStates
 */
BattlescapeState* BattlescapeGame::getBattlescapeState() const
{
	return _parentState;
}

}
