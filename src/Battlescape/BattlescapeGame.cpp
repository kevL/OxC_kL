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
//#include "MeleeAttackBState.h"
#include "NextTurnState.h"
#include "Pathfinding.h"
#include "ProjectileFlyBState.h"
//#include "PsiAttackBState.h"
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

#include "../Ruleset/RuleArmor.h"
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
 * @param battleSave	- pointer to the SavedBattleGame
 * @param parentState	- pointer to the parent BattlescapeState
 */
BattlescapeGame::BattlescapeGame(
		SavedBattleGame* battleSave,
		BattlescapeState* parentState)
	:
		_battleSave(battleSave),
		_parentState(parentState),
		_playerPanicHandled(true),
		_AIActionCounter(0),
		_AISecondMove(false),
		_playedAggroSound(false),
		_endTurnRequested(false)
//		_endTurnProcessed(false)
{
	//Log(LOG_INFO) << "Create BattlescapeGame";
	_debugPlay = false;

	cancelCurrentAction();
	checkForCasualties(
					NULL,
					NULL,
					true);

//	_currentAction.actor = NULL;
//	_currentAction.type = BA_NONE;
//	_currentAction.targeting = false;
	_currentAction.clearAction();

	_universalFist = new BattleItem(
								getRuleset()->getItem("STR_FIST"),
								battleSave->getNextItemId());
	_alienPsi = new BattleItem(
							getRuleset()->getItem("ALIEN_PSI_WEAPON"),
							battleSave->getNextItemId());

	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		(*i)->setBattleForUnit(this);
	}
	// sequence of instantiations:
	// - SavedBattleGame
	// - BattlescapeGenerator
	// - BattlescapeState
	// - this.
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
 * Initializes the Battlescape game.
 */
void BattlescapeGame::init()
{
	if (_battleSave->getSide() == FACTION_PLAYER
		&& _battleSave->getTurn() > 1)
	{
		_playerPanicHandled = false;
	}
}

/**
 * Checks for units panicking or falling and so on.
 */
void BattlescapeGame::think()
{
	//Log(LOG_INFO) << "BattlescapeGame::think()";
	// nothing is happening - see if they need some alien AI or units panicking or what have you
	if (_states.empty() == true)
	{
		//Log(LOG_INFO) << "BattlescapeGame::think() - states NOT Empty.";
		if (_battleSave->getSide() != FACTION_PLAYER) // it's a non player side (ALIENS or CIVILIANS)
		{
			if (_debugPlay == false)
			{
				if (_battleSave->getSelectedUnit() != NULL)
				{
					if (handlePanickingUnit(_battleSave->getSelectedUnit()) == false)
					{
						//Log(LOG_INFO) << "BattlescapeGame::think() call handleAI() ID " << _battleSave->getSelectedUnit()->getId();
						handleAI(_battleSave->getSelectedUnit());
					}
				}
				else
				{
					if (_battleSave->selectNextFactionUnit(
														true,
														_AISecondMove) == NULL)
					{
						if (_battleSave->getDebugMode() == false)
						{
							_endTurnRequested = true;
							statePushBack(NULL); // end AI turn
						}
						else
						{
							_battleSave->selectNextFactionUnit();
							_debugPlay = true;
						}
					}
				}
			}
		}
		else // it's a player side && not all panicking units have been handled
		{
			if (_playerPanicHandled == false)
			{
				//Log(LOG_INFO) << "bg:think() . panic Handled is FALSE";
				_playerPanicHandled = handlePanickingPlayer();
				_battleSave->getBattleState()->updateSoldierInfo();
			}
			//else Log(LOG_INFO) << "bg:think() . panic Handled is TRUE";

			_parentState->updateExperienceInfo(); // kL
		}

		if (_battleSave->getUnitsFalling() == true)
		{
			//Log(LOG_INFO) << "BattlescapeGame::think(), get/setUnitsFalling() ID " << _battleSave->getSelectedUnit()->getId();
			statePushFront(new UnitFallBState(this));
			_battleSave->setUnitsFalling(false);
		}
	}
	//Log(LOG_INFO) << "BattlescapeGame::think() EXIT";
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
 * @note This is a very important function. It is called by a BattleState
 * (walking, projectile is flying, explosions, etc.) at the moment that state
 * has finished the current BattleAction. Check the result of that BattleAction
 * here and do all the aftermath. The state is then popped off the list.
 */
void BattlescapeGame::popState()
{
	//Log(LOG_INFO) << "BattlescapeGame::popState() qtyStates = " << (int)_states.size();
//	if (Options::traceAI)
//	{
//		Log(LOG_INFO) << "BattlescapeGame::popState() #" << _AIActionCounter << " with "
//		<< (_battleSave->getSelectedUnit()? _battleSave->getSelectedUnit()->getTimeUnits(): -9999) << " TU";
//	}

	if (_states.empty() == false)
	{
		//Log(LOG_INFO) << ". states NOT Empty";
		const BattleAction action = _states.front()->getAction();
		bool actionFailed = false;

		if ((_battleSave->getSide() == FACTION_PLAYER || _debugPlay == true)
			&& action.actor != NULL
			&& action.actor->getFaction() == FACTION_PLAYER
			&& action.result.empty() == false // This queries the warning string message.
			&& _playerPanicHandled == true)
		{
			//Log(LOG_INFO) << ". actionFailed";
			actionFailed = true;
			_parentState->warning(action.result);

			// remove action.Cursor if error.Message (eg, not enough tu's)
			if (   action.result.compare("STR_NOT_ENOUGH_TIME_UNITS") == 0
				|| action.result.compare("STR_NO_AMMUNITION_LOADED") == 0
				|| action.result.compare("STR_NO_ROUNDS_LEFT") == 0)
			{
				switch (action.type)
				{
					case BA_LAUNCH: // see also, cancelCurrentAction()
						_currentAction.waypoints.clear();
	//					_parentState->showLaunchButton(false);
	//					_parentState->getMap()->getWaypoints()->clear(); // might be done already.
	//				break;

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
			}
		}

		//Log(LOG_INFO) << ". move Front-state to _deleted.";
		_deleted.push_back(_states.front());

		//Log(LOG_INFO) << ". states.Popfront";
		_states.pop_front();


		// handle the end of this unit's actions
		if (action.actor != NULL
			&& noActionsPending(action.actor) == true)
		{
			//Log(LOG_INFO) << ". noActionsPending for state actor";
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
					&& _battleSave->getSelectedUnit() != NULL
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

				if (_battleSave->getSide() == FACTION_PLAYER)
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
									|| (action.weapon->usesAmmo() == true
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
									|| (action.weapon->usesAmmo() == true
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
									|| (action.weapon->usesAmmo() == true
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

				if (_battleSave->getSide() != FACTION_PLAYER
					&& _debugPlay == false)
				{
					BattleUnit* selUnit = _battleSave->getSelectedUnit();
					 // AI does three things per unit, before switching to the next, or it got killed before doing the second thing
					if (_AIActionCounter > 2
						|| selUnit == NULL
						|| selUnit->isOut_t() == true)
//						|| selUnit->isOut(true, true) == true)
					{
						if (selUnit != NULL)
						{
							selUnit->setCache(NULL);
							getMap()->cacheUnit(selUnit);
						}

						_AIActionCounter = 0;

						if (_states.empty() == true
							&& _battleSave->selectNextFactionUnit(true) == NULL)
						{
							if (_battleSave->getDebugMode() == false)
							{
								_endTurnRequested = true;
								statePushBack(NULL); // end AI turn
							}
							else
							{
								_battleSave->selectNextFactionUnit();
								_debugPlay = true;
							}
						}

						selUnit = _battleSave->getSelectedUnit();
						if (selUnit != NULL)
							getMap()->getCamera()->centerOnPosition(selUnit->getPosition());
					}
				}
				else if (_debugPlay == true)
				{
					setupCursor();
					_parentState->getGame()->getCursor()->setVisible();
				}
			}
		}


		//Log(LOG_INFO) << ". uh yeah";
		if (_states.empty() == false)
		{
			//Log(LOG_INFO) << ". states NOT Empty [1]";
			if (_states.front() == NULL) // end turn request?
			{
				//Log(LOG_INFO) << ". states.front() == NULL";
				while (_states.empty() == false)
				{
					//Log(LOG_INFO) << ". cycle through NULL-states Front";
					if (_states.front() == NULL)
					{
						//Log(LOG_INFO) << ". pop Front";
						_states.pop_front();
					}
					else
						break;
				}

				if (_states.empty() == true)
				{
					//Log(LOG_INFO) << ". states Empty -> endTurnPhase()";
					endTurnPhase();
					//Log(LOG_INFO) << ". endTurnPhase() DONE return";
					return;
				}
				else
				{
					//Log(LOG_INFO) << ". states NOT Empty -> prep back state w/ NULL";
					_states.push_back(NULL);
				}
			}

			//Log(LOG_INFO) << ". states.front()->init()";
			_states.front()->init(); // init the next state in queue
		}

		// the currently selected unit died or became unconscious or disappeared inexplicably
		if (_battleSave->getSelectedUnit() == NULL
			|| _battleSave->getSelectedUnit()->isOut_t() == true)
//			|| _battleSave->getSelectedUnit()->isOut(true, true) == true)
		{
			//Log(LOG_INFO) << ". unit incapacitated: cancelAction & deSelect)";
			cancelCurrentAction();

			_battleSave->setSelectedUnit(NULL);
			//if (_battleSave->getSelectedUnit() != NULL) Log(LOG_INFO) << "selectUnit " << _battleSave->getSelectedUnit()->getId();
			//else Log(LOG_INFO) << "NO UNIT SELECTED";

			if (_battleSave->getSide() == FACTION_PLAYER) // kL
			{
				//Log(LOG_INFO) << ". enable cursor";
				getMap()->setCursorType(CT_NORMAL);
				_parentState->getGame()->getCursor()->setVisible();
			}
		}

		if (_battleSave->getSide() == FACTION_PLAYER) // kL
		{
			//Log(LOG_INFO) << ". updateSoldierInfo()";
			_parentState->updateSoldierInfo(); // calcFoV ought have been done by now ...
		}
	}
	//Log(LOG_INFO) << "BattlescapeGame::popState() EXIT";
}

/**
 * Determines whether there are any actions pending for a given unit.
 * @param unit - pointer to a BattleUnit
 * @return, true if there are no actions pending
 */
bool BattlescapeGame::noActionsPending(const BattleUnit* const unit) const // private
{
	if (_states.empty() == false)
	{
		for (std::list<BattleState*>::const_iterator
				i = _states.begin();
				i != _states.end();
				++i)
		{
			if (*i != NULL
				&& (*i)->getAction().actor == unit)
			{
				return false;
			}
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
 * Handles the processing of the AI states of a unit.
 * @param unit - pointer to a BattleUnit
 */
void BattlescapeGame::handleAI(BattleUnit* const unit)
{
	//Log(LOG_INFO) << "BattlescapeGame::handleAI() " << unit->getId();
	if (unit->getTimeUnits() == 0) // kL, was <6
		unit->dontReselect();

	if (_AIActionCounter > 1
		|| unit->reselectAllowed() == false)
	{
		if (_battleSave->selectNextFactionUnit(
											true,
											_AISecondMove) == NULL)
		{
			if (_battleSave->getDebugMode() == false)
			{
				_endTurnRequested = true;
				//Log(LOG_INFO) << "BattlescapeGame::handleAI() statePushBack(end AI turn)";
				statePushBack(NULL); // end AI turn
			}
			else
			{
				_battleSave->selectNextFactionUnit();
				_debugPlay = true;
			}
		}

		if (_battleSave->getSelectedUnit() != NULL)
		{
			_parentState->updateSoldierInfo(); // what's this doing here these are aLiens or Civies calcFoV is done below

			getMap()->getCamera()->centerOnPosition(_battleSave->getSelectedUnit()->getPosition());

			if (_battleSave->getSelectedUnit()->getId() <= unit->getId())
				_AISecondMove = true;
		}

		_AIActionCounter = 0;

		//Log(LOG_INFO) << "BattlescapeGame::handleAI() Pre-EXIT";
		return;
	}


	unit->setUnitVisible(false);

	// might need this: populate _visibleUnit for a newly-created alien
	//Log(LOG_INFO) << "BattlescapeGame::handleAI(), calculateFOV() call";
	_battleSave->getTileEngine()->calculateFOV(unit->getPosition());
		// it might also help chryssalids realize they've zombified someone and need to move on;
		// it should also hide units when they've killed the guy spotting them;
		// it's also for good luck

	BattleAIState* ai = unit->getCurrentAIState();
//	const BattleAIState* const ai = unit->getCurrentAIState();
	if (ai == NULL)
	{
		// for some reason the unit had no AI routine assigned..
		//Log(LOG_INFO) << "BattlescapeGame::handleAI() !ai, assign AI";
		if (unit->getFaction() == FACTION_HOSTILE)
			unit->setAIState(new AlienBAIState(
											_battleSave,
											unit,
											NULL));
		else
			unit->setAIState(new CivilianBAIState(
											_battleSave,
											unit,
											NULL));
	}
//	_battleSave->getPathfinding()->setPathingUnit(unit);	// decided to do this in AI states;
															// things might be changing the pathing
															// unit or Pathfinding relevance .....

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

	if (unit->getCharging() != NULL
		&& _playedAggroSound == false)
	{
		_playedAggroSound = true;

		const int sound = unit->getAggroSound();
		if (sound != -1)
			getResourcePack()->getSound(
									"BATTLE.CAT",
									sound)
								->play(
									-1,
									getMap()->getSoundAngle(unit->getPosition()));
	}
	//Log(LOG_INFO) << ". getCharging DONE";


//	std::wostringstream ss; // debug.

	if (action.type == BA_WALK)
	{
//		ss << L"Walking to " << action.target;
//		_parentState->debug(ss.str());

		Pathfinding* const pf = _battleSave->getPathfinding();
		pf->setPathingUnit(action.actor);

		if (_battleSave->getTile(action.target) != NULL)
			pf->calculate(
						action.actor,
						action.target);

		if (pf->getStartDirection() != -1)
			statePushBack(new UnitWalkBState(
											this,
											action));
	}
	//Log(LOG_INFO) << ". BA_WALK DONE";


	if (   action.type == BA_SNAPSHOT
		|| action.type == BA_AUTOSHOT
		|| action.type == BA_AIMEDSHOT
		|| action.type == BA_THROW
		|| action.type == BA_HIT
		|| action.type == BA_MINDCONTROL
		|| action.type == BA_PANIC
		|| action.type == BA_LAUNCH)
	{
//		ss.clear();
//		ss << L"Attack type = " << action.type
//				<< ", target = " << action.target
//				<< ", weapon = " << action.weapon->getRules()->getName().c_str();
//		_parentState->debug(ss.str());

		//Log(LOG_INFO) << ". . in action.type";
		if (action.type == BA_MINDCONTROL
			|| action.type == BA_PANIC)
		{
//			statePushBack(new PsiAttackBState(this, action)); // post-cosmetic
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
//				statePushBack(new MeleeAttackBState(this, action));
				bool instaWeapon = false;

				if (action.weapon != _universalFist
					&& meleeWeapon.empty() == false)
				{
					bool found = false;
					for (std::vector<BattleItem*>::const_iterator
							i = unit->getInventory()->begin();
							i != unit->getInventory()->end();
							++i)
					{
						if ((*i)->getRules()->getType() == meleeWeapon)
						{
							// note this ought be conformed w/ bgen.addAlien equipped items to
							// ensure radical (or standard) BT_MELEE weapons get equipped in hand;
							// but for now just grab the meleeItem wherever it was equipped ...
							found = true;
							action.weapon = *i;

							break;
						}
					}

					if (found == false)
					{
						instaWeapon = true;
						action.weapon = new BattleItem(
													getRuleset()->getItem(meleeWeapon),
													_battleSave->getNextItemId());
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
						_battleSave->removeItem(action.weapon);
				}

				return;
			}
		}

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
//			const bool success = _battleSave->getTileEngine()->psiAttack(&action);
			//Log(LOG_INFO) << ". . . success = " << success;
			if (_battleSave->getTileEngine()->psiAttack(&action) == true)
			{
				const BattleUnit* const unit = _battleSave->getTile(action.target)->getUnit();
				Game* const game = _parentState->getGame();
				std::wstring wst;
				if (action.type == BA_MINDCONTROL)
					wst = game->getLanguage()->getString(
													"STR_IS_UNDER_ALIEN_CONTROL",
													unit->getGender())
												.arg(unit->getName(game->getLanguage()))
												.arg(action.value);
				else // Panic Atk
					wst = game->getLanguage()->getString("STR_MORALE_ATTACK_SUCCESSFUL")
												.arg(action.value);

				game->pushState(new InfoboxState(wst));
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

		if (_battleSave->selectNextFactionUnit(
											true,
											_AISecondMove) == NULL)
		{
			if (_battleSave->getDebugMode() == false)
			{
				_endTurnRequested = true;
				//Log(LOG_INFO) << "BattlescapeGame::handleAI() statePushBack(end AI turn) 2";
				statePushBack(NULL); // end AI turn
			}
			else
			{
				_battleSave->selectNextFactionUnit();
				_debugPlay = true;
			}
		}

		if (_battleSave->getSelectedUnit() != NULL)
		{
			_parentState->updateSoldierInfo();
			getMap()->getCamera()->centerOnPosition(_battleSave->getSelectedUnit()->getPosition());

			if (_battleSave->getSelectedUnit()->getId() <= unit->getId())
				_AISecondMove = true;
		}
	}
	//Log(LOG_INFO) << "BattlescapeGame::handleAI() EXIT";
}

/**
 * Handles the result of non target actions like priming a grenade or performing
 * a melee attack or using a medikit.
 */
void BattlescapeGame::handleNonTargetAction()
{
	if (_currentAction.targeting == false)
	{
		_currentAction.cameraPosition = Position(0,0,-1);

		bool showWarning = false;

		// NOTE: These actions are done partly in ActionMenuState::btnActionMenuClick() and
		// this subsequently handles a greater or lesser proportion of the resultant niceties.
 		if (_currentAction.type == BA_PRIME
			|| _currentAction.type == BA_DEFUSE)
		{
			if (_currentAction.actor->spendTimeUnits(_currentAction.TU) == false)
				_parentState->warning("STR_NOT_ENOUGH_TIME_UNITS");
			else
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
		}
		else if (_currentAction.type == BA_USE)
		{
			if (_currentAction.result.empty() == false)
				showWarning = true;

			if (_currentAction.targetUnit != NULL)
			{
				_battleSave->reviveUnit(_currentAction.targetUnit);
				_currentAction.targetUnit = NULL;
			}
		}
		else if (_currentAction.type == BA_LAUNCH)
		{
			if (_currentAction.result.empty() == false)
				showWarning = true;
		}
		else if (_currentAction.type == BA_HIT)
		{
			if (_currentAction.result.empty() == false)
				showWarning = true;
			else
			{
				if (_currentAction.actor->spendTimeUnits(_currentAction.TU) == false)
					_parentState->warning("STR_NOT_ENOUGH_TIME_UNITS");
				else
				{
//					statePushBack(new MeleeAttackBState(this, _currentAction)); // And remove 'return;' below_
					statePushBack(new ProjectileFlyBState(
														this,
														_currentAction));
					return;
				}
			}
		}
		else if (_currentAction.type == BA_DROP)
		{
			if (_currentAction.result.empty() == false)
				showWarning = true;
			else
			{
				if (_currentAction.actor->getPosition().z > 0)
					_battleSave->getTileEngine()->applyGravity(_currentAction.actor->getTile());

				getResourcePack()->getSound(
										"BATTLE.CAT",
										ResourcePack::ITEM_DROP)
									->play(
										-1,
										getMap()->getSoundAngle(_currentAction.actor->getPosition()));
			}
		}

		if (showWarning == true)
		{
			_parentState->warning(_currentAction.result);
			_currentAction.result.clear();
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
		_currentAction.actor = _battleSave->getSelectedUnit();

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
 * Determines whether a playable unit is selected.
 * @note Normally only player side units can be selected but in debug mode the
 * aLiens can play too :)
 * @note Is used to see if stats can be displayed and action buttons will work.
 * @return, true if a playable unit is selected
 */
bool BattlescapeGame::playableUnitSelected()
{
	return _battleSave->getSelectedUnit() != NULL
		&& (_battleSave->getSide() == FACTION_PLAYER
			|| _battleSave->getDebugMode() == true);
}

/**
 * Toggles the kneel/stand status of a unit.
 * @param bu - pointer to a BattleUnit
 * @return, true if the action succeeded
 */
bool BattlescapeGame::kneel(BattleUnit* const bu)
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
					&& _battleSave->getKneelReserved() == true))
			{
				if (bu->getTimeUnits() >= tu)
				{
					if (tu == 3
						|| (tu == 10
							&& bu->spendEnergy(std::max(
													0,
													5 - bu->getArmor()->getAgility())) == true))
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
		const int energyCost = std::max(
									0,
									5 - bu->getArmor()->getAgility());

		if (bu->getTimeUnits() > 9
			&& bu->getEnergy() >= energyCost)
		{
			bu->spendTimeUnits(10);
			bu->spendEnergy(energyCost);

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
 * @note This starts the switchover
 *	- popState()
 *	- handleState()
 *	- statePushBack()
 */
void BattlescapeGame::endTurnPhase() // private.
{
	//Log(LOG_INFO) << "bg::endTurnPhase()";
	_debugPlay = false;
	_AISecondMove = false;
	_parentState->showLaunchButton(false);

	_currentAction.targeting = false;
	_currentAction.type = BA_NONE;
	_currentAction.waypoints.clear();
	getMap()->getWaypoints()->clear();

	Position pos;

//	if (_endTurnProcessed == false)
//	{
	for (size_t // check for hot grenades on the ground
			i = 0;
			i != _battleSave->getMapSizeXYZ();
			++i)
	{
		for (std::vector<BattleItem*>::const_iterator
				j = _battleSave->getTiles()[i]->getInventory()->begin();
				j != _battleSave->getTiles()[i]->getInventory()->end();
				)
		{
			if ((*j)->getRules()->getBattleType() == BT_GRENADE
				&& (*j)->getFuseTimer() != -1
				&& (*j)->getFuseTimer() < 2) // it's a grenade to explode now
			{
				pos.x = _battleSave->getTiles()[i]->getPosition().x * 16 + 8;
				pos.y = _battleSave->getTiles()[i]->getPosition().y * 16 + 8;
				pos.z = _battleSave->getTiles()[i]->getPosition().z * 24 - _battleSave->getTiles()[i]->getTerrainLevel();

				statePushNext(new ExplosionBState(
												this,
												pos,
												*j,
												(*j)->getPreviousOwner()));
				_battleSave->removeItem(*j);

				statePushBack(NULL);
				return;
			}

			++j;
		}
	}

	if (_battleSave->getTileEngine()->closeUfoDoors() != 0
		&& ResourcePack::SLIDING_DOOR_CLOSE != -1) // try, close doors between grenade & terrain explosions
	{
		getResourcePack()->getSound( // ufo door closed
								"BATTLE.CAT",
								ResourcePack::SLIDING_DOOR_CLOSE)
							->play();
	}
//	}

	// check for terrain explosions
	Tile* tile = _battleSave->getTileEngine()->checkForTerrainExplosions();
	if (tile != NULL)
	{
		pos = Position(
					tile->getPosition().x * 16 + 8,
					tile->getPosition().y * 16 + 8,
					tile->getPosition().z * 24 + 10);

		// kL_note: This seems to be screwing up.
		// Further info: what happens is that an explosive part of a tile gets destroyed by fire
		// during an endTurn sequence, has its setExplosive() set, then is somehow triggered
		// by the next projectile hit against whatever.
		statePushNext(new ExplosionBState(
										this,
										pos,
										NULL,
										NULL,
										tile));

//		tile = _battleSave->getTileEngine()->checkForTerrainExplosions();

		statePushBack(NULL);	// this will repeatedly call another endTurnPhase() so there's
		return;					// no need to continue this one till all explosions are done.
								// The problem arises because _battleSave->endBattlePhase() below
								// causes *more* destruction of explosive objects, that won't explode
								// until some later instantiation of ExplosionBState .....
								//
								// As to why this doesn't simply loop like other calls to
								// do terrainExplosions, i don't know.
	}

//	if (_endTurnProcessed == false)
//	{
	if (_battleSave->getSide() != FACTION_NEUTRAL) // tick down grenade timers
	{
		for (std::vector<BattleItem*>::const_iterator
				i = _battleSave->getItems()->begin();
				i != _battleSave->getItems()->end();
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

	// THE NEXT 3 BLOCKS could get Quirky. (ie: tiles vs. units, tallyUnits, tiles vs. ground-items)
	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if ((*i)->getFaction() == _battleSave->getSide()
			&& (*i)->isOut() == false)
		{
			tile = (*i)->getTile();
			if (tile != NULL
				&& (tile->getSmoke() != 0
					|| tile->getFire() != 0))
//				&& (*i)->getHealth() > 0
//				&& ((*i)->getGeoscapeSoldier() != NULL
//					|| (*i)->getUnitRules()->isMechanical() == false))
			{
				tile->hitStuff(); // Damage tile's unit w/ Smoke & Fire at end of unit's faction's Turn-phase.
			}
		}
	}


	if (_battleSave->endBattlePhase() == true) // <- This rolls over Faction-turns. TRUE means FullTurn has ended.
	{
		for (size_t
				i = 0;
				i != _battleSave->getMapSizeXYZ();
				++i)
		{
			tile = _battleSave->getTiles()[i];
			if ((tile->getSmoke() != 0
					|| tile->getFire() != 0)
				&& tile->getInventory()->empty() == false)
			{
				tile->hitStuff(_battleSave); // Damage tile's items w/ Fire at end of each full-turn.
			}
		}

		for (std::vector<BattleUnit*>::const_iterator // reset the takenExpl(smoke) and takenFire flags on every unit.
				i = _battleSave->getUnits()->begin();
				i != _battleSave->getUnits()->end();
				++i)
		{
			(*i)->setTakenExpl(false); // even if Status_Dead, just do it.
			(*i)->setTakenFire(false);
		}
	}
	// best just to do another call to checkForTerrainExplosions()/ ExplosionBState in there ....
	// -> SavedBattleGame::spreadFireSmoke()
	// Or here
	// ... done it in NextTurnState.

		// check AGAIN for terrain explosions
/*		tile = _battleSave->getTileEngine()->checkForTerrainExplosions();
		if (tile != NULL)
		{
			pos = Position(
						tile->getPosition().x * 16 + 8,
						tile->getPosition().y * 16 + 8,
						tile->getPosition().z * 24 + 10);
			statePushNext(new ExplosionBState(
											this,
											pos,
											NULL,
											NULL,
											tile));
			statePushBack(NULL);
			_endTurnProcessed = true;
			return;
		} */
		// kL_note: you know what, I'm just going to let my quirky solution run
		// for a while. BYPASS _endTurnProcessed
//	}
//	_endTurnProcessed = false;

	if (_battleSave->getDebugMode() == false)
	{
		if (_battleSave->getSide() == FACTION_PLAYER)
		{
			setupCursor();
			_battleSave->getBattleState()->toggleIcons(true);
		}
		else
		{
			getMap()->setCursorType(CT_NONE);
			_battleSave->getBattleState()->toggleIcons(false);
		}
	}


	checkForCasualties();

	int // if all units from either faction are killed - the mission is over.
		liveAliens,
		liveSoldiers;
	const bool pacified = tallyUnits(
								liveAliens,
								liveSoldiers);

	if (_battleSave->allObjectivesDestroyed() == true)
	{
		_parentState->finishBattle(
								false,
								liveSoldiers);
		return;
	}

	const bool battleComplete = liveAliens == 0
							 || liveSoldiers == 0;

	if (battleComplete == false)
	{
		showInfoBoxQueue();
		_parentState->updateSoldierInfo();

		if (playableUnitSelected() == true) // <- only Faction_Player or Debug-mode
		{
			getMap()->getCamera()->centerOnPosition(_battleSave->getSelectedUnit()->getPosition());
			setupCursor();
		}
	}

	if (_endTurnRequested == true)
	{
		if (_battleSave->getSide() != FACTION_NEUTRAL
			|| battleComplete == true)
		{
			_parentState->getGame()->pushState(new NextTurnState(
															_battleSave,
															_parentState,
															pacified));
		}
	}

	_endTurnRequested = false;
}

/**
 * Checks for casualties and adjusts morale accordingly.
 * @note Also checks if Alien Base Control was destroyed in a BaseAssault tactical.
 * @param weapon		- pointer to the weapon responsible (default NULL)
 * @param attacker		- pointer to credit the kill (default NULL)
 * @param hiddenExpl	- true for UFO Power Source explosions at the start of battlescape (default false)
 * @param terrainExpl	- true for terrain explosions (default false)
 */
void BattlescapeGame::checkForCasualties(
		const BattleItem* const weapon,
		BattleUnit* attacker,
		bool hiddenExpl,
		bool terrainExpl)
{
	//Log(LOG_INFO) << "BattlescapeGame::checkForCasualties()"; if (attacker != NULL) Log(LOG_INFO) << ". id-" << attacker->getId();

	// If the victim was killed by the attacker's death explosion,
	// fetch who killed the attacker and make THAT the attacker!
	if (attacker != NULL)
	{
		if (attacker->getStatus() == STATUS_DEAD
			&& attacker->getMurdererId() != 0
			&& attacker->getUnitRules() != NULL
			&& attacker->getUnitRules()->getSpecialAbility() == SPECAB_EXPLODE)
		{
			for (std::vector<BattleUnit*>::const_iterator
					i = _battleSave->getUnits()->begin();
					i != _battleSave->getUnits()->end();
					++i)
			{
				if ((*i)->getId() == attacker->getMurdererId())
				{
					attacker = *i;
					break;
				}
			}
		}

		// attacker gets Exposed if a spotter is still conscious
		// NOTE: Useful only after Melee attacks. Firearms & explosives handle
		// things differently ... see note in TileEngine::checkReactionFire().
		//Log(LOG_INFO) << ". check for spotters Qty = " << (int)attacker->getUnitSpotters()->size();
		if (attacker->getUnitSpotters()->empty() == false)
		{
			for (std::list<BattleUnit*>::const_iterator // -> not sure what happens if RF-trigger kills Cyberdisc that kills aLien .....
					i = attacker->getUnitSpotters()->begin();
					i != attacker->getUnitSpotters()->end();
					++i)
			{
//				if ((*i)->getHealth() != 0
//					&& (*i)->getHealth() > (*i)->getStun())
				if ((*i)->isOut_t(OUT_HLTH_STUN) == false)
				{
					attacker->setExposed(); // defender has been spotted on Player turn.
					break;
				}
			}

			attacker->getUnitSpotters()->clear();
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


	if (attacker != NULL
		&& attacker->getGeoscapeSoldier() != NULL)
	{
		killStatMission = _battleSave->getGeoscapeSave()->getMissionStatistics()->size();

		killStatTurn = _battleSave->getTurn() * 3;
		if (_battleSave->getSide() == FACTION_HOSTILE)
			killStatTurn += 1;
		else if (_battleSave->getSide() == FACTION_NEUTRAL)
			killStatTurn += 2;

		if (weapon != NULL)
		{
			killStatWeapon =
			killStatWeaponAmmo = weapon->getRules()->getName();
		}

		const BattleItem* item = attacker->getItem("STR_RIGHT_HAND");
		const RuleItem* itRule;
		if (item != NULL)
		{
			itRule = item->getRules();
			for (std::vector<std::string>::const_iterator
					i = itRule->getCompatibleAmmo()->begin();
					i != itRule->getCompatibleAmmo()->end();
					++i)
			{
				if (*i == killStatWeaponAmmo)
					killStatWeapon = itRule->getName();
			}
		}

		item = attacker->getItem("STR_LEFT_HAND");
		if (item != NULL)
		{
			itRule = item->getRules();
			for (std::vector<std::string>::const_iterator
					i = itRule->getCompatibleAmmo()->begin();
					i != itRule->getCompatibleAmmo()->end();
					++i)
			{
				if (*i == killStatWeaponAmmo)
					killStatWeapon = itRule->getName();
			}
		}
	}


	bool
		dead,
		stunned,
		converted,
		bypass;

	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
//		dead = ((*i)->getHealth() == 0);
//		stunned = ((*i)->getHealth() <= (*i)->getStun());
		dead = (*i)->isOut_t(OUT_DEAD);
		stunned = (*i)->isOut_t(OUT_STUNNED);

		converted =
		bypass = false;

		if (dead == false) // for converting infected units that aren't dead.
		{
			if ((*i)->getSpawnUnit() == "STR_ZOMBIE") // human->zombie (nobody cares about zombie->chryssalid)
			{
				converted = true;
				convertUnit(
						*i,
						(*i)->getSpawnUnit());
			}
			else if (stunned == false)
				bypass = true;
		}

		if (bypass == false)
		{
			BattleUnit* const victim = *i; // kL

			// Awards: decide victim race and rank
			if (attacker != NULL
				&& attacker->getGeoscapeSoldier() != NULL)
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


			if ((dead == true
					&& victim->getStatus() != STATUS_DEAD
					&& victim->getStatus() != STATUS_COLLAPSING	// kL_note: is this really needed ....
					&& victim->getStatus() != STATUS_TURNING	// kL: may be set by UnitDieBState cTor
					&& victim->getStatus() != STATUS_DISABLED)	// kL
					// STATUS_TIME_OUT
				|| converted == true)
			{
				if (dead == true)
					(*i)->setStatus(STATUS_DISABLED);

				// attacker's Morale Bonus & diary ->
				if (attacker != NULL)
				{
					(*i)->killedBy(attacker->getFaction()); // used in DebriefingState.
					//Log(LOG_INFO) << "BSG::checkForCasualties() " << victim->getId() << " killedBy = " << (int)attacker->getFaction();

					if (attacker->isFearable() == true)
					{
						if (attacker->getGeoscapeSoldier() != NULL)
						{
							attacker->getStatistics()->kills.push_back(new BattleUnitKills(
																					killStatRank,
																					killStatRace,
																					killStatWeapon,
																					killStatWeaponAmmo,
																					victim->getFaction(),
																					STATUS_DEAD,
																					killStatMission,
																					killStatTurn,
																					killStatPoints));
							victim->setMurdererId(attacker->getId());
						}

						int bonus;
						if (attacker->getOriginalFaction() == FACTION_PLAYER)
						{
							bonus = _battleSave->getMoraleModifier();

							if (attacker->getFaction() == FACTION_PLAYER	// not MC'd
								&& attacker->getGeoscapeSoldier() != NULL)	// is Soldier
							{
								attacker->addKillCount();
							}
						}
						else if (attacker->getOriginalFaction() == FACTION_HOSTILE)
							bonus = _battleSave->getMoraleModifier(NULL, false);
						else
							bonus = 0;

						// attacker's Valor
						if ((attacker->getOriginalFaction() == FACTION_HOSTILE
								&& victim->getOriginalFaction() == FACTION_PLAYER)
							|| (attacker->getOriginalFaction() == FACTION_PLAYER
								&& victim->getOriginalFaction() == FACTION_HOSTILE))
						{
							const int courage = 10 * bonus / 100;
							attacker->moraleChange(courage); // double what rest of squad gets below
						}
						// attacker (mc'd or not) will get a penalty with friendly fire (mc'd or not)
						// ... except aLiens, who don't care.
						else if (attacker->getOriginalFaction() == FACTION_PLAYER
							&& victim->getOriginalFaction() == FACTION_PLAYER)
						{
							int chagrin = 5000 / bonus; // huge chagrin!
							if (victim->getUnitRules() != NULL
								&& victim->getUnitRules()->isMechanical() == true)
							{
								chagrin /= 2;
							}
							attacker->moraleChange(-chagrin);
						}
						else if (victim->getOriginalFaction() == FACTION_NEUTRAL) // civilian kills
						{
							if (attacker->getOriginalFaction() == FACTION_PLAYER)
							{
								const int dishonor = 2000 / bonus;
								attacker->moraleChange(-dishonor);
							}
							else if (attacker->getOriginalFaction() == FACTION_HOSTILE)
								attacker->moraleChange(20); // no leadership bonus for aLiens executing civies: it's their job.
						}
					}
				}

				// cycle through units and do all faction
//				if (victim->getFaction() != FACTION_NEUTRAL) // civie deaths now affect other Factions.
//				{
				// penalty for the death of a unit; civilians & MC'd aLien units return 100.
				const int loss = _battleSave->getMoraleModifier(victim);
				// These two are factions (aTeam & bTeam leaderships mitigate losses).
				int
					aTeam, // winners
					bTeam; // losers

				if (victim->getOriginalFaction() == FACTION_HOSTILE)
				{
					aTeam = _battleSave->getMoraleModifier();
					bTeam = _battleSave->getMoraleModifier(NULL, false);
				}
				else // victim is xCom or civilian
				{
					aTeam = _battleSave->getMoraleModifier(NULL, false);
					bTeam = _battleSave->getMoraleModifier();
				}

				for (std::vector<BattleUnit*>::const_iterator // do bystander FACTION changes:
						j = _battleSave->getUnits()->begin();
						j != _battleSave->getUnits()->end();
						++j)
				{
//					if ((*j)->isOut(true, true) == false
					if ((*j)->isOut_t() == false
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
							if (moraleLoss > 0) // pure safety, ain't gonna happen really.
							{
								moraleLoss = moraleLoss * loss * 2 / bTeam;
								if (converted == true)
									moraleLoss = (moraleLoss * 5 + 3) / 4; // extra loss if xCom or civie turns into a Zombie.
								else if (victim->getUnitRules() != NULL
									&& victim->getUnitRules()->isMechanical() == true)
								{
									moraleLoss /= 2;
								}

								(*j)->moraleChange(-moraleLoss);
							}
/*							if (attacker
								&& attacker->getFaction() == FACTION_PLAYER
								&& victim->getFaction() == FACTION_HOSTILE)
							{
								attacker->setExposed(); // interesting
								//Log(LOG_INFO) << ". . . . attacker Exposed";
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
//				}

				if (converted == false)
				{
					if (weapon != NULL)
						statePushNext(new UnitDieBState( // kL_note: This is where units get sent to DEATH!
													this,
													*i,
													weapon->getRules()->getDamageType()));
					else // hidden or terrain explosion or death by fatal wounds
					{
						if (hiddenExpl == true) // this is instant death from UFO powersources, without screaming sounds
							statePushNext(new UnitDieBState(
														this,
														*i,
														DT_HE,
														true));
						else
						{
							if (terrainExpl == true)
								statePushNext(new UnitDieBState(
															this,
															*i,
															DT_HE));
							else // no attacker and no terrain explosion; must be fatal wounds
								statePushNext(new UnitDieBState(
															this,
															*i,
															DT_NONE)); // DT_NONE -> STR_HAS_DIED_FROM_A_FATAL_WOUND
						}
					}
				}
			}
			else if (stunned == true
				&& victim->getStatus() != STATUS_DEAD
				&& victim->getStatus() != STATUS_UNCONSCIOUS
				&& victim->getStatus() != STATUS_COLLAPSING	// kL_note: is this really needed ....
				&& victim->getStatus() != STATUS_TURNING	// kL_note: may be set by UnitDieBState cTor
				&& victim->getStatus() != STATUS_DISABLED)	// kL
				// STATUS_TIME_OUT
			{
				(*i)->setStatus(STATUS_DISABLED); // kL

				if (attacker != NULL
					&& attacker->getGeoscapeSoldier() != NULL)
				{
					attacker->getStatistics()->kills.push_back(new BattleUnitKills(
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
	}


	if (hiddenExpl == false)
	{
		if (_battleSave->getSide() == FACTION_PLAYER)
		{
			const BattleUnit* const unit = _battleSave->getSelectedUnit();
			if (unit != NULL)
				_parentState->showPsiButton(unit->getOriginalFaction() == FACTION_HOSTILE
										 && unit->getBaseStats()->psiSkill > 0
										 && unit->isOut_t() == false);
//										 && unit->isOut(true, true) == false);
			else
				_parentState->showPsiButton(false);
		}

		if (_battleSave->getTacticalType() == TCT_BASEASSAULT
			&& _battleSave->getDestroyed() == false)
		{
			bool controlDestroyed = true;
			for (size_t
					i = 0;
					i != _battleSave->getMapSizeXYZ();
					++i)
			{
				if (_battleSave->getTiles()[i]->getMapData(O_OBJECT) != NULL
					&& _battleSave->getTiles()[i]->getMapData(O_OBJECT)->getSpecialType() == UFO_NAVIGATION)
				{
					controlDestroyed = false;
					break;
				}
			}

			if (controlDestroyed == true)
			{
				_battleSave->setDestroyed();

				Game* const game = _parentState->getGame();
				game->pushState(new InfoboxOKState(game->getLanguage()->getString("STR_ALIEN_BASE_CONTROL_DESTROYED")));
			}
		}
	}
}

/**
 * Shows the infoboxes in the queue if any.
 */
void BattlescapeGame::showInfoBoxQueue() // private.
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
 * Checks against reserved time units.
 * @param bu	- pointer to a unit
 * @param tu	- # of time units to check against
 * @param test	- true to suppress error messages (default false)
 * @return, true if unit has enough time units - go!
 */
bool BattlescapeGame::checkReservedTU(
		BattleUnit* bu,
		int tu,
		bool test)
{
	if (bu->getFaction() != _battleSave->getSide()
		|| _battleSave->getSide() == FACTION_NEUTRAL)
	{
		return tu <= bu->getTimeUnits();
	}


	BattleActionType actReserved = _battleSave->getBATReserved(); // avoid changing _batReserved here.

	if (_battleSave->getSide() == FACTION_HOSTILE
		&& _debugPlay == false)
	{
		const AlienBAIState* const ai = dynamic_cast<AlienBAIState*>(bu->getCurrentAIState());
		if (ai != NULL)
			actReserved = ai->getReservedAIAction();

		const int extraReserve = RNG::generate(0,10); // kL, added in below ->

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
	if (_battleSave->getKneelReserved() == true
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
			|| _battleSave->getKneelReserved() == true)
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
				switch (_battleSave->getBATReserved())
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
bool BattlescapeGame::handlePanickingPlayer() // private.
{
	//Log(LOG_INFO) << "bg::handlePanickingPlayer()";
	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if ((*i)->getFaction() == FACTION_PLAYER
			&& (*i)->getOriginalFaction() == FACTION_PLAYER
			&& handlePanickingUnit(*i) == true)
		{
			//Log(LOG_INFO) << ". ret FALSE " << (*i)->getId();
			return false;
		}
	}

	//Log(LOG_INFO) << ". ret TRUE";
	return true;
}

/**
 * Handles panicking units.
 * @return, true if unit is panicking
 */
bool BattlescapeGame::handlePanickingUnit(BattleUnit* const unit) // private.
{
	//Log(LOG_INFO) << "bg::handlePanickingUnit() - " << unit->getId();
	const UnitStatus status = unit->getStatus();

	if (status == STATUS_PANICKING
		|| status == STATUS_BERSERK)
	{
		_parentState->getMap()->setCursorType(CT_NONE);
		_battleSave->setSelectedUnit(unit);

		if (unit->getUnitVisible() == true // show panicking message
			|| Options::noAlienPanicMessages == false)
		{
			getMap()->getCamera()->centerOnPosition(unit->getPosition());

			Game* const game = _parentState->getGame();
			std::string st;
			if (status == STATUS_PANICKING)
				st = "STR_HAS_PANICKED";
			else
				st = "STR_HAS_GONE_BERSERK";

			game->pushState(new InfoboxState(
										game->getLanguage()->getString(
																	st,
																	unit->getGender())
																.arg(unit->getName(game->getLanguage()))));
		}

		unit->setStatus(STATUS_STANDING);

		BattleAction ba;
		ba.actor = unit;

		int tu = unit->getBaseStats()->tu;
		unit->setTimeUnits(tu); // start w/ fresh TUs

		switch (status)
		{
			case STATUS_PANICKING:
			{
				//Log(LOG_INFO) << ". PANIC";
				BattleItem* item;
				if (RNG::percent(75) == true)
				{
					item = unit->getItem("STR_RIGHT_HAND");
					if (item != NULL)
						dropItem(
								unit->getPosition(),
								item,
								false,
								true);
				}

				if (RNG::percent(75) == true)
				{
					item = unit->getItem("STR_LEFT_HAND");
					if (item != NULL)
						dropItem(
								unit->getPosition(),
								item,
								false,
								true);
				}

				unit->setCache(NULL);

				Pathfinding* const pf = _battleSave->getPathfinding();
				pf->setPathingUnit(unit);

				tu = RNG::generate(10, tu);
				std::vector<int> reachable = pf->findReachable(
															unit,
															tu);
				const size_t
					pick = static_cast<size_t>(RNG::generate(
														0,
														reachable.size() - 1)),
					tileId = static_cast<size_t>(reachable[pick]);

				_battleSave->getTileCoords(
										tileId,
										&ba.target.x,
										&ba.target.y,
										&ba.target.z);
				pf->calculate(
							ba.actor,
							ba.target,
							NULL,
							tu);

				if (pf->getStartDirection() != -1)
				{
					ba.actor->setDashing();
					ba.dash = true;
					ba.type = BA_WALK;
					// wait a sec, if there are two+ units doing panic-movement
					// won't their paths need to be recalculated as their respective
					// UnitWalkBStates get taken care of one after the next ...
					// guess not, they all get different ba-targets.
					statePushBack(new UnitWalkBState(
													this,
													ba));
				}
			}
			break; // End_case_PANICKING.

			case STATUS_BERSERK:	// berserk - do some weird turning around and then aggro
			{						// towards an enemy unit or shoot towards random place
				//Log(LOG_INFO) << ". BERSERK";
				ba.type = BA_TURN;
				const int pivotQty = RNG::generate(2,5);
				for (int
						i = 0;
						i != pivotQty;
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

				ba.weapon = unit->getMainHandWeapon();
				if (ba.weapon == NULL)
					ba.weapon = unit->getGrenade();

				// TODO: run up to another unit and slug it with the Universal Fist.
				// Or w/ an already-equipped melee weapon

				if (ba.weapon != NULL)
//					&& (_battleSave->getDepth() != 0
//						|| ba.weapon->getRules()->isWaterOnly() == false))
				{
					if (ba.weapon->getRules()->getBattleType() == BT_FIREARM)
					{
						const size_t
							mapSize = _battleSave->getMapSizeXYZ(),
							pick = static_cast<size_t>(RNG::generate(
																0,
																static_cast<int>(mapSize) - 1));
						Tile* const targetTile = _battleSave->getTiles()[pick];
						ba.target = targetTile->getPosition();
						if (_battleSave->getTile(ba.target) != NULL)
						{
							statePushBack(new UnitTurnBState(
															this,
															ba,
															false));

							ba.cameraPosition = _battleSave->getBattleState()->getMap()->getCamera()->getMapOffset();
							ba.type = BA_SNAPSHOT;
							const int actionTu = ba.actor->getActionTUs(
																	ba.type,
																	ba.weapon);

							for (int // fire shots until unit runs out of TUs
									i = 0;
									i != 10;
									++i)
							{
								if (ba.actor->spendTimeUnits(actionTu) == true)
									statePushBack(new ProjectileFlyBState(
																		this,
																		ba));
								else
									break;
							}
						}
					}
					else if (ba.weapon->getRules()->getBattleType() == BT_GRENADE)
					{
						if (ba.weapon->getFuseTimer() == -1)
							ba.weapon->setFuseTimer(0); // yeh set timer even if throw is invalid.

						for (int // try a few times to get a tile to throw to.
								i = 0;
								i != 50;
								++i)
						{
							ba.target = Position(
											unit->getPosition().x + RNG::generate(-20,20),
											unit->getPosition().y + RNG::generate(-20,20),
											unit->getPosition().z);

							if (_battleSave->getTile(ba.target) != NULL)
							{
								statePushBack(new UnitTurnBState(
																this,
																ba,
																false));

								const Position
									originVoxel = _battleSave->getTileEngine()->getOriginVoxel(ba),
									targetVoxel = Position(
														ba.target.x * 16 + 8,
														ba.target.y * 16 + 8,
														ba.target.z * 24 - _battleSave->getTile(ba.target)->getTerrainLevel());

								if (_battleSave->getTileEngine()->validateThrow(
																			ba,
																			originVoxel,
																			targetVoxel) == true)
								{
									ba.cameraPosition = _battleSave->getBattleState()->getMap()->getCamera()->getMapOffset();
									ba.type = BA_THROW;
									statePushBack(new ProjectileFlyBState(
																		this,
																		ba));
									break;
								}
							}
						}
					}
				}

				unit->setTimeUnits(unit->getBaseStats()->tu); // replace the TUs from shooting
				ba.type = BA_NONE;
			} // End_case_BERSERK.
		}

		statePushBack(new UnitPanicBState( // TimeUnits can be reset only after everything else occurs
										this,
										unit));

//		unit->moraleChange(10 + RNG::generate(0,10)); // <- now done in UnitPanicBState
		return true;
	}

	return false;
}

/**
  * Cancels the current action the user had selected (firing, throwing, etc).
  * @param force - force the action to be cancelled (default false)
  * @return, true if action was cancelled
  */
bool BattlescapeGame::cancelCurrentAction(bool force)
{
	if (Options::battleNewPreviewPath != PATH_NONE
		&& _battleSave->getPathfinding()->removePreview() == true)
	{
		return true;
	}

	if (_states.empty() == true
		|| force == true)
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

				if (_battleSave->getSide() == FACTION_PLAYER) // kL
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
 * @note This appears to be for Player's units only.
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
	//if (_battleSave->getSelectedUnit()) Log(LOG_INFO) << ". ID " << _battleSave->getSelectedUnit()->getId();
	_currentAction.actor = _battleSave->getSelectedUnit();
	BattleUnit* const targetUnit = _battleSave->selectUnit(targetPos);

	if (_currentAction.actor != NULL
		&& _currentAction.targeting == true)
	{
		//Log(LOG_INFO) << ". . _currentAction.targeting";
		_currentAction.strafe = false;

		if (_currentAction.type == BA_LAUNCH) // click to set BL waypoints.
		{
			if (static_cast<int>(_currentAction.waypoints.size()) < _currentAction.weapon->getRules()->isWaypoints())
			{
				//Log(LOG_INFO) << ". . . . BA_LAUNCH";
				_parentState->showLaunchButton();
				_currentAction.waypoints.push_back(targetPos);
				getMap()->getWaypoints()->push_back(targetPos);
			}
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
//							getResourcePack()->getSoundByDepth(
//															_battleSave->getDepth(),
							getResourcePack()->getSound(
													"BATTLE.CAT",
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
						_currentAction.cameraPosition = Position(0,0,-1); // kL, don't adjust the camera when coming out of ProjectileFlyB/ExplosionB states


//						statePushBack(new PsiAttackBState(this, _currentAction));
						//Log(LOG_INFO) << ". . . . . . new ProjectileFlyBState";
						statePushBack(new ProjectileFlyBState(
															this,
															_currentAction));

						if (_currentAction.actor->getTimeUnits() >= _currentAction.TU) // kL_note: WAIT, check this *before* all the stuff above!!!
						{
							if (getTileEngine()->psiAttack(&_currentAction) == true)
							{
								//Log(LOG_INFO) << ". . . . . . Psi successful";
								Game* const game = _parentState->getGame(); // show an infobox if successful

								std::wstring wst;
								if (_currentAction.type == BA_PANIC)
									wst = game->getLanguage()->getString("STR_MORALE_ATTACK_SUCCESSFUL")
																	.arg(_currentAction.value);
								else // Mind-control
									wst = game->getLanguage()->getString("STR_MIND_CONTROL_SUCCESSFUL")
																	.arg(_currentAction.value);

								game->pushState(new InfoboxState(wst));


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

//								if (_battleSave->getSelectedUnit()->getFaction() != FACTION_PLAYER)
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
			if (Options::battleConfirmFireMode == true)
			{
				_currentAction.waypoints.clear();
				getMap()->getWaypoints()->clear();
			}

//			getMap()->setCursorType(CT_NONE);
//			_parentState->getGame()->getCursor()->setVisible(false);

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
	else // select unit, or spin/ MOVE .......
	{
		//Log(LOG_INFO) << ". . NOT _currentAction.targeting";
		bool allowPreview = (Options::battleNewPreviewPath != PATH_NONE);

		if (targetUnit != NULL // select unit
			&& targetUnit != _currentAction.actor
			&& (targetUnit->getUnitVisible() == true
				|| _debugPlay == true))
		{
			if (targetUnit->getFaction() == _battleSave->getSide())
			{
				_battleSave->setSelectedUnit(targetUnit);
				_parentState->updateSoldierInfo();

				cancelCurrentAction();
				setupCursor();

				_currentAction.actor = targetUnit;
			}
		}
		else if (playableUnitSelected() == true)
		{
			Pathfinding* const pf = _battleSave->getPathfinding();
			pf->setPathingUnit(_currentAction.actor);

			const bool
				modifCTRL = (SDL_GetModState() & KMOD_CTRL) != 0,
				modifALT = (SDL_GetModState() & KMOD_ALT) != 0,
				isMech = _currentAction.actor->getUnitRules() != NULL
					  && _currentAction.actor->getUnitRules()->isMechanical();

//			_currentAction.dash = false;
//			_currentAction.actor->setDashing(false);

			if (targetUnit != NULL // spin 180 degrees
				&& targetUnit == _currentAction.actor
				&& modifCTRL == false
				&& modifALT == false
				&& isMech == false
				&& _currentAction.actor->getArmor()->getSize() == 1) // small units only
			{
				if (allowPreview == true)
					pf->removePreview();

				Position screenPixel;
				getMap()->getCamera()->convertMapToScreen(
														targetPos,
														&screenPixel);
				screenPixel += getMap()->getCamera()->getMapOffset();

				Position mousePixel;
				getMap()->findMousePosition(mousePixel);

				if (mousePixel.x > screenPixel.x + 16)
					_currentAction.actor->setTurnDirection(-1);
				else
					_currentAction.actor->setTurnDirection(1);

				Pathfinding::directionToVector(
											(_currentAction.actor->getDirection() + 4) % 8,
											&_currentAction.target);
				_currentAction.target += targetPos;
//				_currentAction.strafe = false;

				statePushBack(new UnitTurnBState(
											this,
											_currentAction));
			}
			else // handle pathPreview and MOVE
			{
				if (allowPreview == true
					&& (_currentAction.target != targetPos
						|| pf->isModCTRL() != modifCTRL
						|| pf->isModALT() != modifALT))
				{
					pf->removePreview();
				}

/* Take care of this muck in Pathfinding:
				_currentAction.strafe = (Options::strafe == true)
									&& ((modifCTRL == true		// soldier strafe
											&& isMech == false)
										|| (modifALT == true	// tank reverse gear, 1 tile only
											&& isMech == true));
				//Log(LOG_INFO) << ". primary action: Strafe";

				const Position
					actorPos = _currentAction.actor->getPosition(),
					pos = Position(
								targetPos.x - actorPos.x,
								targetPos.y - actorPos.y,
								0);
				const int
//					dist = _battleSave->getTileEngine()->distance(
//																actorPos,
//																targetPos),
					dirUnit = _currentAction.actor->getDirection();

				int dir;
				pf->vectorToDirection(pos, dir);

				const size_t pathSize = pf->getPath().size();

				//Log(LOG_INFO) << ". strafe = " << (int)_currentAction.strafe;
				//Log(LOG_INFO) << ". isSoldier = " << (int)(_currentAction.actor->getGeoscapeSoldier() != NULL);
				//Log(LOG_INFO) << ". diff_Z = " << (int)(actorPos.z != targetPos.z);
				//Log(LOG_INFO) << ". dist = " << dist;
				//Log(LOG_INFO) << ". pathSize = " << (int)pathSize;
				//Log(LOG_INFO) << ". dirUnit = " << dirUnit;
				//Log(LOG_INFO) << ". dir = " << dir;

				if (_currentAction.strafe == true
					&& _currentAction.actor->getGeoscapeSoldier() != NULL
					&& (actorPos.z != targetPos.z
//						|| dist > 1
						|| pathSize > 1
						|| (actorPos.z == targetPos.z
//							&& dist < 2
							&& pathSize == 1
							&& dirUnit == dir)))
				{
					_currentAction.strafe = false;
					_currentAction.dash = true;
					_currentAction.actor->setDashing();
				} */


				_currentAction.target = targetPos;
				pf->calculate( // CREATE the Path.
							_currentAction.actor,
							_currentAction.target);

				if (pf->getStartDirection() != -1)
				{
					if (allowPreview == true
						&& pf->previewPath() == false)
					{
						pf->removePreview();
						allowPreview = false;
					}

					if (allowPreview == false) // -= start walking =- //
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
	//Log(LOG_INFO) << "bsg:primaryAction() EXIT w/ strafe = " << (int)_currentAction.strafe << " / dash = " << (int)_currentAction.dash;
	//Log(LOG_INFO) << "BattlescapeGame::primaryAction() EXIT";
}

/**
 * Activates secondary action (right click).
 * @param posTarget - reference a Position on the map
 */
void BattlescapeGame::secondaryAction(const Position& posTarget)
{
	//Log(LOG_INFO) << "BattlescapeGame::secondaryAction()";
	_currentAction.actor = _battleSave->getSelectedUnit();

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
	_parentState->showLaunchButton(false);

	getMap()->getWaypoints()->clear();
	_currentAction.target = _currentAction.waypoints.front();

	getMap()->setCursorType(CT_NONE);
	_parentState->getGame()->getCursor()->setVisible(false);

//	_currentAction.cameraPosition = getMap()->getCamera()->getMapOffset();

	_states.push_back(new ProjectileFlyBState(
											this,
											_currentAction));
	statePushFront(new UnitTurnBState( // first of all turn towards the target
									this,
									_currentAction));
}

/**
 * Handler for the psi button.
 */
void BattlescapeGame::psiButtonAction()
{
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
 * @note Used only by tactical icon buttons.
 * @param unit	- a unit
 * @param dir	- direction DIR_UP or DIR_DOWN
 */
void BattlescapeGame::moveUpDown(
		const BattleUnit* const unit,
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

	Pathfinding* const pf = _battleSave->getPathfinding();
//	pf->setPathingUnit(_currentAction.actor); // set in BattlescapeState::btnUnitUp/DownClick()
	pf->calculate(
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
 * Drops an item to the floor and affects it with gravity then recalculates FoV
 * if it's a light-source.
 * @param pos			- reference position to place the item
 * @param item			- pointer to the item
 * @param newItem		- true if this is a new item (default false)
 * @param removeItem	- true to remove the item from the owner (default false)
 */
void BattlescapeGame::dropItem(
		const Position& pos,
		BattleItem* const item,
		bool newItem,
		bool removeItem)
{
	if (_battleSave->getTile(pos) == NULL		// don't spawn anything outside of bounds
		|| item->getRules()->isFixed() == true)	// don't ever drop fixed items
	{
		return;
	}

	_battleSave->getTile(pos)->addItem(
									item,
									getRuleset()->getInventory("STR_GROUND"));

	if (item->getUnit() != NULL)
		item->getUnit()->setPosition(pos);

	if (newItem == true)
		_battleSave->getItems()->push_back(item);
//	else if (_battleSave->getSide() != FACTION_PLAYER)
//		item->setTurnFlag(true);

	if (removeItem == true)
		item->moveToOwner(NULL);
	else if (item->getRules()->getBattleType() != BT_GRENADE
		&& item->getRules()->getBattleType() != BT_PROXYGRENADE)
	{
		item->setOwner(NULL);
	}

	getTileEngine()->applyGravity(_battleSave->getTile(pos));

	if (item->getRules()->getBattleType() == BT_FLARE)
	{
		getTileEngine()->calculateTerrainLighting();
		getTileEngine()->recalculateFOV();
	}
}

/**
 * Converts a unit into a unit of another type.
 * @param unit		- pointer to a unit to convert
 * @param conType	- reference the type of unit to convert to
 * @return, pointer to the new unit
 */
BattleUnit* BattlescapeGame::convertUnit(
		BattleUnit* unit,
		const std::string& conType)
{
	//Log(LOG_INFO) << "BattlescapeGame::convertUnit() " << conType;
	const bool visible = unit->getUnitVisible();

	_battleSave->getBattleState()->showPsiButton(false);
	_battleSave->removeCorpse(unit); // in case the unit was unconscious

	unit->instaKill();
	unit->setSpecialAbility(SPECAB_NONE);

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

	unit->setTile(NULL);
	_battleSave->getTile(unit->getPosition())->setUnit(NULL);


	std::ostringstream armorType;
	armorType << getRuleset()->getUnit(conType)->getArmor();

	const int
		difficulty = static_cast<int>(_parentState->getGame()->getSavedGame()->getDifficulty()),
		month = _parentState->getGame()->getSavedGame()->getMonthsPassed();

	BattleUnit* const conUnit = new BattleUnit(
											getRuleset()->getUnit(conType),
											FACTION_HOSTILE,
											_battleSave->getUnits()->back()->getId() + 1,
											getRuleset()->getArmor(armorType.str()),
											difficulty,
//											getDepth(),
											month,
											this);

	const Position posUnit = unit->getPosition();
	_battleSave->getTile(posUnit)->setUnit(
										conUnit,
										_battleSave->getTile(posUnit + Position(0,0,-1)));
	conUnit->setPosition(posUnit);
	conUnit->setTimeUnits(0);

	int dir;
	if (conType == "STR_ZOMBIE")
		dir = RNG::generate(0,7); // or, (unit->getDirection())
	else
		dir = 3;
	conUnit->setDirection(dir);

	_battleSave->getUnits()->push_back(conUnit);

	conUnit->setAIState(new AlienBAIState(
										_battleSave,
										conUnit,
										NULL));

	std::string terrorWeapon = getRuleset()->getUnit(conType)->getRace().substr(4);
	terrorWeapon += "_WEAPON";
	BattleItem* const item = new BattleItem(
										getRuleset()->getItem(terrorWeapon),
										_battleSave->getNextItemId());
	item->moveToOwner(conUnit);
	item->setSlot(getRuleset()->getInventory("STR_RIGHT_HAND"));
	_battleSave->getItems()->push_back(item);

//	conUnit->setCache(NULL);
	getMap()->cacheUnit(conUnit);

	conUnit->setUnitVisible(visible);

	getTileEngine()->applyGravity(conUnit->getTile());
//	getTileEngine()->calculateUnitLighting(); // <- done in UnitDieBState
	getTileEngine()->calculateFOV(conUnit->getPosition());

//	conUnit->getCurrentAIState()->think();

	return conUnit;
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
	return _battleSave;
}

/**
 * Gets the tilengine.
 * @return, pointer to TileEngine
 */
TileEngine* BattlescapeGame::getTileEngine() const
{
	return _battleSave->getTileEngine();
}

/**
 * Gets the pathfinding.
 * @return, pointer to Pathfinding
 */
Pathfinding* BattlescapeGame::getPathfinding() const
{
	return _battleSave->getPathfinding();
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
		BattleItem* const targetItem = surveyItems(action);								// pick the best available item
		if (targetItem != NULL
			&& worthTaking(targetItem, action) == true)									// make sure it's worth taking
		{
			if (targetItem->getTile()->getPosition() == action->actor->getPosition())	// if we're already standing on it...
			{
				if (takeItemFromGround(targetItem, action) == 0							// try to pick it up
					&& targetItem->getAmmoItem() == NULL)								// if it isn't loaded or it is ammo
				{
					action->actor->checkAmmo();											// try to load the weapon
				}
			}
			else if (targetItem->getTile()->getUnit() == NULL							// if nobody's standing on it
				|| targetItem->getTile()->getUnit()->isOut(true, true))					// we should try to get to it.
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
	// And no one else is standing on it.
	for (std::vector<BattleItem*>::const_iterator
			i = _battleSave->getItems()->begin();
			i != _battleSave->getItems()->end();
			++i)
	{
		if ((*i)->getSlot() != NULL
			&& (*i)->getSlot()->getId() == "STR_GROUND"
			&& (*i)->getTile() != NULL
			&& ((*i)->getTile()->getUnit() == NULL
				|| (*i)->getTile()->getUnit() == action->actor)
//			&& (*i)->getTurnFlag()
			&& (*i)->getRules()->getAttraction() != 0)
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
			/ ((_battleSave->getTileEngine()->distance(
													action->actor->getPosition(),
													(*i)->getTile()->getPosition()) * 2)
												+ 1);
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
		 - (_battleSave->getTileEngine()->distance(
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
						i != 4;
						++i)
				{
					if (action->actor->getItem("STR_BELT", i) == NULL)
					{
						item->moveToOwner(action->actor);
						item->setSlot(getRuleset()->getInventory("STR_BELT"));
						item->setSlotX(i);

						placed = true;
						break;
					}
				}
			}
		break;

		case BT_GRENADE:
		case BT_PROXYGRENADE:
			for (int
					i = 0;
					i != 4;
					++i)
			{
				if (action->actor->getItem("STR_BELT", i) == NULL)
				{
					item->moveToOwner(action->actor);
					item->setSlot(getRuleset()->getInventory("STR_BELT"));
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
				item->setSlot(getRuleset()->getInventory("STR_RIGHT_HAND"));

				placed = true;
			}
		break;

		case BT_MEDIKIT:
		case BT_SCANNER:
			if (action->actor->getItem("STR_BACK_PACK") == NULL)
			{
				item->moveToOwner(action->actor);
				item->setSlot(getRuleset()->getInventory("STR_BACK_PACK"));

				placed = true;
			}
		break;

		case BT_MINDPROBE:
			if (action->actor->getItem("STR_LEFT_HAND") == NULL)
			{
				item->moveToOwner(action->actor);
				item->setSlot(getRuleset()->getInventory("STR_LEFT_HAND"));

				placed = true;
			}
	}

	return placed;
}

/**
 * Sets the TU reserved type as a BattleAction.
 * @param bat - a battleactiontype (BattlescapeGame.h)
 */
void BattlescapeGame::setReservedAction(BattleActionType bat)
{
	_battleSave->setBATReserved(bat);
}

/**
 * Returns the action type that is reserved.
 * @return, the BattleActionType that is reserved
 */
BattleActionType BattlescapeGame::getReservedAction() const
{
	return _battleSave->getBATReserved();
}

/**
 * Tallies the living units in the game.
 * @param liveAliens	- reference in which to store the live alien tally
 * @param liveSoldiers	- reference in which to store the live XCom tally
 * @return, true if all aliens are dead or pacified independent of allowPsionicCapture option
 */
bool BattlescapeGame::tallyUnits(
		int& liveAliens,
		int& liveSoldiers) const
{
	bool ret = true;

	liveSoldiers =
	liveAliens = 0;

	for (std::vector<BattleUnit*>::const_iterator
			j = _battleSave->getUnits()->begin();
			j != _battleSave->getUnits()->end();
			++j)
	{
		if ((*j)->isOut() == false)
		{
			if ((*j)->getOriginalFaction() == FACTION_HOSTILE)
			{
				if ((*j)->getFaction() != FACTION_PLAYER
					|| Options::allowPsionicCapture == false)
				{
					++liveAliens;
				}

				if ((*j)->getFaction() == FACTION_HOSTILE)
					ret = false;
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

	//Log(LOG_INFO) << "bg:tallyUnits() ret = " << ret << "; Sol = " << liveSoldiers << "; aLi = " << liveAliens;
	return ret;
}

/**
 * Sets the kneel reservation setting.
 * @param reserved - true to reserve an extra 4 TUs to kneel
 */
void BattlescapeGame::setKneelReserved(bool reserved)
{
//	_kneelReserved = reserved;
	_battleSave->setKneelReserved(reserved);
}

/**
 * Gets the kneel reservation setting.
 * @return, kneel reservation setting
 */
bool BattlescapeGame::getKneelReserved()
{
	return _battleSave->getKneelReserved();

//	if (_battleSave->getSelectedUnit()
//		&& _battleSave->getSelectedUnit()->getGeoscapeSoldier())
//	{
//		return _kneelReserved;
//	}

//	return false;
}

/**
 * Checks if a unit has moved next to a proximity grenade.
 * @note Checks one tile around the unit in every direction. For a large unit
 * check every tile the unit occupies.
 * @param unit - pointer to a BattleUnit
 * @return, true if a proximity grenade is triggered
 */
bool BattlescapeGame::checkForProximityGrenades(BattleUnit* const unit)
{
	int unitSize = unit->getArmor()->getSize() - 1;
	for (int
			x = unitSize;
			x != -1;
			--x)
	{
		for (int
				y = unitSize;
				y != -1;
				--y)
		{
			for (int
					tx = -1;
					tx != 2;
					++tx)
			{
				for (int
						ty = -1;
						ty != 2;
						++ty)
				{
					Tile* const tile = _battleSave->getTile(unit->getPosition()
																   + Position(x,y,0)
																   + Position(tx,ty,0));
					if (tile != NULL)
					{
						for (std::vector<BattleItem*>::const_iterator
								i = tile->getInventory()->begin();
								i != tile->getInventory()->end();
								++i)
						{
							if ((*i)->getRules()->getBattleType() == BT_PROXYGRENADE
								&& (*i)->getFuseTimer() != -1)
							{
								int dir; // cred: animal310 - http://openxcom.org/bugs/openxcom/issues/765
								_battleSave->getPathfinding()->vectorToDirection(
																			Position(tx,ty,0),
																			dir);
								//Log(LOG_INFO) << "dir = " << dir;
								if (_battleSave->getPathfinding()->isBlockedPath(
																			_battleSave->getTile(unit->getPosition() + Position(x,y,0)),
																			dir,
																			unit) == false)	// kL try passing in OBJECT_SELF as a missile target to kludge for closed doors.
								{															// there *might* be a problem if the Proxy is on a non-walkable tile ....
									Position pos;

									pos.x = tile->getPosition().x * 16 + 8;
									pos.y = tile->getPosition().y * 16 + 8;
									pos.z = tile->getPosition().z * 24 - tile->getTerrainLevel();

									statePushNext(new ExplosionBState(
																	this,
																	pos,
																	*i,
																	(*i)->getPreviousOwner()));
									_battleSave->removeItem(*i); // does/should this even be done (also done at end of ExplosionBState) -> causes a double-explosion if remarked here.

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
/* const int BattlescapeGame::getDepth() const
{
	return _battleSave->getDepth();
} */

/**
 * Gets the BattlescapeState.
 * @note For turning on/off the visUnits indicators from UnitWalk/TurnBStates.
 * @return, pointer to BattlescapeState
 */
BattlescapeState* BattlescapeGame::getBattlescapeState() const
{
	return _parentState;
}

/**
 * Gets the universal fist.
 * @return, the universal fist!!
 */
BattleItem* BattlescapeGame::getFist() const
{
	return _universalFist;
}

/**
 * Gets the universal alienPsi weapon.
 * @return, the alienPsi BattleItem
 */
BattleItem* BattlescapeGame::getAlienPsi() const
{
	return _alienPsi;
}

}
