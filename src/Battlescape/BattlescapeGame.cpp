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

#include "AlienBAIState.h"
#include "BattlescapeState.h"
#include "BattleState.h"
#include "Camera.h"
#include "CivilianBAIState.h"
#include "ExplosionBState.h"
#include "Explosion.h" // Execute.
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

#include "../Ruleset/AlienDeployment.h"
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
		_endTurnRequested(false),
		_firstInit(true),
		_executeProgress(false)
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
	//Log(LOG_INFO) << "bg: init()";
	if (_firstInit == true)
	{
		_firstInit = false;
		_battleSave->getTileEngine()->recalculateFOV();
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
		//Log(LOG_INFO) << "BattlescapeGame::think() - _states is Empty. Clear rfShotList";
		_battleSave->getTileEngine()->getRfShotList()->clear();

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
		else // it's a player side
		{
			if (_playerPanicHandled == false) // not all panicking units have been handled
			{
				//Log(LOG_INFO) << "bg:think() . panic Handled is FALSE";
				_playerPanicHandled = handlePanickingPlayer();
				//Log(LOG_INFO) << "bg:think() . panic Handled = " << _playerPanicHandled;

				if (_playerPanicHandled == true)
				{
					_battleSave->getTileEngine()->recalculateFOV();
					_battleSave->getBattleState()->updateSoldierInfo();
				}
			}
			else
			{
				//Log(LOG_INFO) << "bg:think() . panic Handled is TRUE";
				_parentState->updateExperienceInfo();
			}
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
 * Gives a time slice to the front state.
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
			if (action.result.compare("STR_NOT_ENOUGH_TIME_UNITS") == 0
				|| action.result.compare("STR_NO_AMMUNITION_LOADED") == 0
				|| action.result.compare("STR_NO_ROUNDS_LEFT") == 0)
			{
				switch (action.type)
				{
					case BA_LAUNCH: // see also, cancelCurrentAction()
						_currentAction.waypoints.clear();
					case BA_THROW:
					case BA_SNAPSHOT:
					case BA_AIMEDSHOT:
					case BA_AUTOSHOT:
					case BA_PSIPANIC:
					case BA_PSICONFUSE:
					case BA_PSICOURAGE:
					case BA_PSICONTROL:
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


		if (action.actor != NULL // handle the end of this unit's actions
			&& noActionsPending(action.actor) == true)
		{
			//Log(LOG_INFO) << ". noActionsPending for state actor";
			if (action.actor->getFaction() == FACTION_PLAYER)
			{
				//Log(LOG_INFO) << ". actor -> Faction_Player";
				// spend TUs of "target triggered actions" (shooting, throwing) only;
				// the other actions' TUs (healing, scanning, etc) are already take care of.
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

					// after throwing the cursor returns to default cursor;
					// after shooting it stays in targeting mode and the player
					// can shoot again in the same mode (autoshot/snap/aimed)
					// unless he/she is out of tu's and/or ammo
					if (actionFailed == false)
					{
						const int curTu = action.actor->getTimeUnits();

						switch (action.type)
						{
							case BA_LAUNCH:
								_currentAction.waypoints.clear();
							case BA_THROW:
								cancelCurrentAction(true);
							break;

							case BA_SNAPSHOT:
								//Log(LOG_INFO) << ". SnapShot, TU percent = " << (float)action.weapon->getRules()->getTUSnap();
								if (curTu < action.actor->getActionTUs(
																	BA_SNAPSHOT,
																	action.weapon)
									|| action.weapon->getAmmoItem() == NULL)
//									|| (action.weapon->selfPowered() == false
//										&& (action.weapon->getAmmoItem() == NULL
//											|| action.weapon->getAmmoItem()->getAmmoQuantity() == 0)))
								{
									cancelCurrentAction(true);
								}
							break;

							case BA_AUTOSHOT:
								//Log(LOG_INFO) << ". AutoShot, TU percent = " << (float)action.weapon->getRules()->getTUAuto();
								if (curTu < action.actor->getActionTUs(
																	BA_AUTOSHOT,
																	action.weapon)
									|| action.weapon->getAmmoItem() == NULL)
//									|| (action.weapon->selfPowered() == false
//										&& (action.weapon->getAmmoItem() == NULL
//											|| action.weapon->getAmmoItem()->getAmmoQuantity() == 0)))
								{
									cancelCurrentAction(true);
								}
							break;

							case BA_AIMEDSHOT:
								//Log(LOG_INFO) << ". AimedShot, TU percent = " << (float)action.weapon->getRules()->getTUAimed();
								if (curTu < action.actor->getActionTUs(
																	BA_AIMEDSHOT,
																	action.weapon)
									|| action.weapon->getAmmoItem() == NULL)
//									|| (action.weapon->selfPowered() == false
//										&& (action.weapon->getAmmoItem() == NULL
//											|| action.weapon->getAmmoItem()->getAmmoQuantity() == 0)))
								{
									cancelCurrentAction(true);
								}
							break;

/*							case BA_PSIPANIC:
								if (curTu < action.actor->getActionTUs(BA_PSIPANIC, action.weapon))
									cancelCurrentAction(true);
							break;
							case BA_PSICONTROL:
								if (curTu < action.actor->getActionTUs(BA_PSICONTROL, action.weapon))
									cancelCurrentAction(true);
							break; */
/*							case BA_USE:
								if (action.weapon->getRules()->getBattleType() == BT_MINDPROBE
									&& curTu < action.actor->getActionTUs(BA_PSICONTROL, action.weapon))
									cancelCurrentAction(true);
							break; */
						}
					}

					setupCursor();
					_parentState->getGame()->getCursor()->setVisible(); // might not be needed here anymore. But safety.
					_parentState->getGame()->getCursor()->setHidden(false);
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
				_parentState->getGame()->getCursor()->setHidden(false);
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
	if (unit->getTimeUnits() == 0) // was <6
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

	// might need this: populate _hostileUnit for a newly-created alien
	//Log(LOG_INFO) << "BattlescapeGame::handleAI(), calculateFOV() call";
	_battleSave->getTileEngine()->calculateFOV(unit->getPosition());
		// it might also help chryssalids realize they've zombified someone and need to move on;
		// it should also hide units when they've killed the guy spotting them;
		// it's also for good luck and prosperity.

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
//		&& unit->getHostileUnits()->size() == 0)
	// TODO: and, if either no innate meleeWeapon, or a visible hostile is not within say 5 tiles.
	{
		//Log(LOG_INFO) << ". . no mainhand weapon or no ammo";
		//Log(LOG_INFO) << ". . . call findItem()";
		findItem(&action);
	}
	//Log(LOG_INFO) << ". findItem DONE";

	if (unit->getChargeTarget() != NULL
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
	//Log(LOG_INFO) << ". getChargeTarget DONE";


//	std::wostringstream ss; // debug.

	if (action.type == BA_MOVE)
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
	//Log(LOG_INFO) << ". BA_MOVE DONE";


	if (action.type == BA_SNAPSHOT
		|| action.type == BA_AUTOSHOT
		|| action.type == BA_AIMEDSHOT
		|| action.type == BA_THROW
		|| action.type == BA_HIT
		|| action.type == BA_PSICONTROL
		|| action.type == BA_PSIPANIC
		|| action.type == BA_LAUNCH)
	{
//		ss.clear();
//		ss << L"Attack type = " << action.type
//				<< ", target = " << action.target
//				<< ", weapon = " << action.weapon->getRules()->getName().c_str();
//		_parentState->debug(ss.str());

		//Log(LOG_INFO) << ". . in action.type";
		if (action.type == BA_PSICONTROL
			|| action.type == BA_PSIPANIC)
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

		if (action.type == BA_PSIPANIC
			|| action.type == BA_PSICONTROL)
		{
			//Log(LOG_INFO) << ". . . in action.type Psi";
//			const bool success = _battleSave->getTileEngine()->psiAttack(&action);
			//Log(LOG_INFO) << ". . . success = " << success;
			if (_battleSave->getTileEngine()->psiAttack(&action) == true)
			{
				const BattleUnit* const unit = _battleSave->getTile(action.target)->getUnit();
				Game* const game = _parentState->getGame();
				std::wstring wst;
				if (action.type == BA_PSICONTROL)
					wst = game->getLanguage()->getString(
													"STR_IS_UNDER_ALIEN_CONTROL",
													unit->getGender())
												.arg(unit->getName(game->getLanguage()))
												.arg(action.value);
				else // Panic Atk
					wst = game->getLanguage()->getString("STR_PSI_PANIC_SUCCESS")
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
 * @note The action is set up in ActionMenuState.
 */
void BattlescapeGame::handleNonTargetAction()
{
	if (_currentAction.targeting == false)
	{
		_currentAction.cameraPosition = Position(0,0,-1);

		int showWarning = 0;

		// NOTE: These actions are done partly in ActionMenuState::btnActionMenuClick() and
		// this subsequently handles a greater or lesser proportion of the resultant niceties.
		//
		switch (_currentAction.type)
		{
			case BA_PRIME:
			case BA_DEFUSE:
				if (_currentAction.actor->spendTimeUnits(_currentAction.TU) == false)
				{
					_currentAction.result = "STR_NOT_ENOUGH_TIME_UNITS";
					showWarning = 1;
				}
				else
				{
					_currentAction.weapon->setFuse(_currentAction.value);

					if (_currentAction.value == -1)
					{
						_currentAction.result = "STR_GRENADE_IS_DEACTIVATED";
						showWarning = 1;
					}
					else if (_currentAction.value == 0)
					{
						_currentAction.result = "STR_GRENADE_IS_ACTIVATED";
						showWarning = 1;
					}
					else
					{
						_currentAction.result = "STR_GRENADE_IS_ACTIVATED_";
						showWarning = 2;
					}
				}
			break;

			case BA_USE:
				if (_currentAction.result.empty() == false)
					showWarning = 1;
				else if (_currentAction.targetUnit != NULL)
				{
					_battleSave->reviveUnit(_currentAction.targetUnit);
					_currentAction.targetUnit = NULL;
				}
			break;

			case BA_LAUNCH:
				if (_currentAction.result.empty() == false)
					showWarning = 1;
			break;

			case BA_HIT:
				if (_currentAction.result.empty() == false)
					showWarning = 1;
				else if (_currentAction.actor->spendTimeUnits(_currentAction.TU) == false)
				{
					_currentAction.result = "STR_NOT_ENOUGH_TIME_UNITS";
					showWarning = 1;
				}
				else
				{
					statePushBack(new ProjectileFlyBState(
														this,
														_currentAction));
					return;
				}
			break;

			case BA_DROP:
				if (_currentAction.result.empty() == false)
					showWarning = 1;
				else
				{
					_battleSave->getTileEngine()->applyGravity(_currentAction.actor->getTile());
					getResourcePack()->getSound(
											"BATTLE.CAT",
											ResourcePack::ITEM_DROP)
										->play(
											-1,
											getMap()->getSoundAngle(_currentAction.actor->getPosition()));
				}
			break;

			case BA_EXECUTE:
				if (_currentAction.result.empty() == false)
					showWarning = 1;
				else if (_currentAction.targetUnit != NULL)
				{
					executeUnit();
					_currentAction.targetUnit = NULL;
				}

			// switch_end.
		}

		if (showWarning != 0)
		{
			if (showWarning == 1)
				_parentState->warning(_currentAction.result);
			else if (showWarning == 2)
				_parentState->warning(
									_currentAction.result,
									true,
									_currentAction.value);

			_currentAction.result.clear();
		}

		_currentAction.type = BA_NONE;
		_parentState->updateSoldierInfo();
	}

	setupCursor();
}

/**
 * Summary execution.
 */
void BattlescapeGame::executeUnit() // private.
{
	_currentAction.actor->aim();
	_currentAction.actor->setCache(NULL);
	getMap()->cacheUnit(_currentAction.actor);

	const RuleItem* const itRule = _currentAction.weapon->getRules();
	BattleItem* const ammo = _currentAction.weapon->getAmmoItem();
	int
		sound = -1,
		start = 0,		// void vc++ linker warning.
		isMelee = 0;	// void vc++ linker warning.

	if (itRule->getBattleType() == BT_MELEE)
	{
		start = itRule->getMeleeAnimation();
		isMelee = 1;

		sound = ammo->getRules()->getMeleeHitSound();
		if (sound == -1)
		{
			sound = itRule->getMeleeHitSound();
			if (sound == -1)
				sound = ResourcePack::ITEM_DROP;
		}
	}
	else if (itRule->getBattleType() == BT_FIREARM)
	{
		start = ammo->getRules()->getHitAnimation();

		sound = ammo->getRules()->getHitSound();
		if (sound == -1)
			sound = itRule->getHitSound();
	}

	if (sound != -1)
		getResourcePack()->getSound(
								"BATTLE.CAT",
								sound)
							->play(
								-1,
								getMap()->getSoundAngle(_currentAction.actor->getPosition()));

/*	if (ammo->spendBullet() == false)
	{
		_battleSave->removeItem(ammo);
		_currentAction.weapon->setAmmoItem(NULL);
	} */
	ammo->spendBullet(
				*_battleSave,
				*_currentAction.weapon);

	Position posOrigin_voxel = _currentAction.target * Position(16,16,24) + Position(8,8,2);
	Explosion* const explosion = new Explosion(
											posOrigin_voxel,
											start,
											0,
											false,
											isMelee);
	getMap()->getExplosions()->push_back(explosion);
	_executeProgress = true;

//	Uint32 interval = BattlescapeState::STATE_INTERVAL_STANDARD * 5 / 7;
/*	Uint32 interval = BattlescapeState::STATE_INTERVAL_STANDARD * 100; // test
	interval -= static_cast<Uint32>(ammo->getRules()->getExplosionSpeed()) * 10;
	if (interval < 1) interval = 1;
	setStateInterval(interval); */

	_currentAction.targetUnit->playDeathSound(); // scream little piggie

	_currentAction.actor->spendTimeUnits(_currentAction.TU);

	_currentAction.targetUnit->setHealth(0);
	checkForCasualties(
					_currentAction.weapon,
					_currentAction.actor,
					false,false,
					true);
}

/**
 * Sets the cursor according to the selected action.
 */
void BattlescapeGame::setupCursor()
{
	getMap()->refreshSelectorPosition();

	CursorType cType;
	int quads = 1;

	if (_currentAction.targeting == true)
	{
		switch (_currentAction.type)
		{
			case BA_THROW:
				cType = CT_THROW;
			break;

			case BA_PSICONTROL:
			case BA_PSIPANIC:
			case BA_PSICONFUSE:
			case BA_PSICOURAGE:
			case BA_USE:
				cType = CT_PSI;
			break;

			case BA_LAUNCH:
				cType = CT_WAYPOINT;
			break;

			default:
				cType = CT_AIM;
		}
	}
	else
	{
		cType = CT_NORMAL;

		_currentAction.actor = _battleSave->getSelectedUnit();
		if (_currentAction.actor != NULL)
			quads = _currentAction.actor->getArmor()->getSize();
	}

	getMap()->setCursorType(
						cType,
						quads);
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
 * TODO: allow Civilian units to kneel when controlled by medikit or by AI.
 * @param unit - pointer to a BattleUnit
 * @return, true if the action succeeded
 */
bool BattlescapeGame::kneel(BattleUnit* const unit)
{
	//Log(LOG_INFO) << "BattlescapeGame::kneel()";
	if (unit->getGeoscapeSoldier() != NULL
		&& unit->getFaction() == unit->getOriginalFaction())
	{
		if (unit->isFloating() == false) // kL_note: This prevents flying soldiers from 'kneeling' .....
		{
			int tu;
			if (unit->isKneeled() == true)
				tu = 10;
			else
				tu = 3;

			if (checkReservedTu(unit, tu) == true
				|| (tu == 3
					&& _battleSave->getKneelReserved() == true))
			{
				if (unit->getTimeUnits() >= tu)
				{
					if (tu == 3
						|| (tu == 10
							&& unit->spendEnergy(std::max(
													0,
													5 - unit->getArmor()->getAgility())) == true))
					{
						unit->spendTimeUnits(tu);
						unit->kneel(!unit->isKneeled());
						// kneeling or standing up can reveal new terrain or units. I guess. -> sure can!
						// But updateSoldierInfo() also does does calculateFOV(), so ...
//						getTileEngine()->calculateFOV(unit);

						getMap()->cacheUnits();

//						_parentState->updateSoldierInfo(false); // <- also does calculateFOV() !
						// wait... shouldn't one of those calcFoV's actually trigger!! ? !
						// Hopefully it's done after returning, in another updateSoldierInfo... or newVis check.
						// So.. I put this in BattlescapeState::btnKneelClick() instead; updates will
						// otherwise be handled by walking or what have you. Doing it this way conforms
						// updates/FoV checks with my newVis routines.

//						getTileEngine()->checkReactionFire(unit);
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
	else if (unit->getGeoscapeSoldier() != NULL) // MC'd xCom agent, trying to stand & walk by AI.
	{
		const int energyCost = std::max(
									0,
									5 - unit->getArmor()->getAgility());

		if (unit->getTimeUnits() > 9
			&& unit->getEnergy() >= energyCost)
		{
			unit->spendTimeUnits(10);
			unit->spendEnergy(energyCost);

			unit->kneel(false);
			getMap()->cacheUnits();

			return true;
		}
	}
	else if (unit->getFaction() == FACTION_PLAYER
		&& unit->getOriginalFaction() == FACTION_HOSTILE)
//		&& unit->getUnitRules()->isMechanical() == false) // MOB has Unit-rules
	{
		_parentState->warning("STR_ACTION_NOT_ALLOWED_ALIEN");
	}

	return false;
}

/**
 * Ends the turn.
 * @note This starts the switchover.
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
				&& (*j)->getFuse() != -1
				&& (*j)->getFuse() < 2) // it's a grenade to explode now
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

	if (_battleSave->getTileEngine()->closeUfoDoors() != 0 // close doors between grenade & terrain explosions
		&& ResourcePack::SLIDING_DOOR_CLOSE != -1)
	{
		getResourcePack()->getSound(
								"BATTLE.CAT",
								ResourcePack::SLIDING_DOOR_CLOSE)
							->play();
	}
//	}

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
				&& (*i)->getFuse() > 1)
			{
				(*i)->setFuse((*i)->getFuse() - 1);
			}
		}
	}

	// THE NEXT 3 BLOCKS could get Quirky. (ie: tiles vs. units, tallyUnits, tiles vs. ground-items)
	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if ((*i)->getOriginalFaction() == _battleSave->getSide()
			&& (*i)->isOut_t(OUT_STAT) == false)
//			&& (*i)->isOut() == false)
		{
			tile = (*i)->getTile();
			if (tile != NULL
				&& (tile->getSmoke() != 0
					|| tile->getFire() != 0))
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
			if (tile->getInventory()->empty() == false
				&& (tile->getSmoke() != 0
					|| tile->getFire() != 0))
			{
				tile->hitStuff(_battleSave); // Damage tile's items w/ Fire at beginning of each full-turn.
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
	// -> SavedBattleGame::tileVolatiles()
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
	const bool hostilesPacified = tallyUnits(
										liveAliens,
										liveSoldiers);

	if (_battleSave->allObjectivesDestroyed() == true // brain death.
		&& _battleSave->getObjectiveType() == MUST_DESTROY)
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

		if (hostilesPacified == true)
			_battleSave->setPacified();
	}

	if (_endTurnRequested == true
		&& (_battleSave->getSide() != FACTION_NEUTRAL
			|| battleComplete == true))
	{
		_parentState->getGame()->pushState(new NextTurnState(
														_battleSave,
														_parentState,
														hostilesPacified));
	}

	_endTurnRequested = false;
}

/**
 * Checks for casualties and adjusts morale accordingly.
 * @note Also checks if Alien Base Control was destroyed in a BaseAssault tactical.
 * @param weapon		- pointer to the weapon responsible (default NULL)
 * @param attackUnit	- pointer to credit the kill (default NULL)
 * @param hiddenExpl	- true for UFO Power Source explosions at the start of battlescape (default false)
 * @param terrainExpl	- true for terrain explosions (default false)
 * @param execution		- true if called by an execution (default false)
 */
void BattlescapeGame::checkForCasualties(
		const BattleItem* const weapon,
		BattleUnit* attackUnit,
		bool hiddenExpl,
		bool terrainExpl,
		bool execution)
{
	//Log(LOG_INFO) << "BattlescapeGame::checkForCasualties()"; if (attackUnit != NULL) Log(LOG_INFO) << ". id-" << attackUnit->getId();

	// If the victim was killed by the attacker's death explosion,
	// fetch who killed the attacker and make THAT the attacker!
	if (attackUnit != NULL
		&& execution == false)
	{
		if (attackUnit->getStatus() == STATUS_DEAD
			&& attackUnit->getMurdererId() != 0
			&& attackUnit->getUnitRules() != NULL
			&& attackUnit->getUnitRules()->getSpecialAbility() == SPECAB_EXPLODE)
		{
			for (std::vector<BattleUnit*>::const_iterator
					i = _battleSave->getUnits()->begin();
					i != _battleSave->getUnits()->end();
					++i)
			{
				if ((*i)->getId() == attackUnit->getMurdererId())
				{
					attackUnit = *i;
					break;
				}
			}
		}

		// attacker gets Exposed if a spotter is still conscious
		// NOTE: Useful only after Melee attacks. Firearms & explosives handle
		// things differently ... see note in TileEngine::checkReactionFire().
		//Log(LOG_INFO) << ". check for spotters Qty = " << (int)attackUnit->getRfSpotters()->size();
		if (attackUnit->getRfSpotters()->empty() == false)
		{
			for (std::list<BattleUnit*>::const_iterator // -> not sure what happens if RF-trigger kills Cyberdisc that kills aLien .....
					i = attackUnit->getRfSpotters()->begin();
					i != attackUnit->getRfSpotters()->end();
					++i)
			{
//				if ((*i)->getHealth() != 0 && (*i)->getHealth() > (*i)->getStun())
				if ((*i)->isOut_t(OUT_DEAD_STUN) == false)
				{
					attackUnit->setExposed(); // defender has been spotted on Player turn.
					break;
				}
			}

			attackUnit->getRfSpotters()->clear();
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


	if (attackUnit != NULL
		&& attackUnit->getGeoscapeSoldier() != NULL)
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

		const BattleItem* item = attackUnit->getItem("STR_RIGHT_HAND");
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

		item = attackUnit->getItem("STR_LEFT_HAND");
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
		if ((*i)->getStatus() != STATUS_LIMBO) // kL_tentative.
		{
//			dead = ((*i)->getHealth() == 0);
//			stunned = ((*i)->getHealth() <= (*i)->getStun());
			dead = (*i)->isOut_t(OUT_DEAD);
			stunned = (*i)->isOut_t(OUT_STUN);

			converted =
			bypass = false;

			if (dead == false) // for converting infected units that aren't dead.
			{
				if ((*i)->getSpawnUnit() == "STR_ZOMBIE") // human->zombie (nobody cares about zombie->chryssalid)
				{
					converted = true;
					convertUnit(*i);
				}
				else if (stunned == false)
					bypass = true;
			}

			if (bypass == false)
			{
				BattleUnit* const defendUnit = *i; // kL

				// Awards: decide victim race and rank
				if (attackUnit != NULL
					&& attackUnit->getGeoscapeSoldier() != NULL)
				{
					killStatPoints = defendUnit->getValue();

					if (defendUnit->getOriginalFaction() == FACTION_PLAYER)	// <- xCom DIED
					{
						killStatPoints = -killStatPoints;

						if (defendUnit->getGeoscapeSoldier() != NULL)	// Soldier
						{
							killStatRace = "STR_HUMAN";
							killStatRank = defendUnit->getGeoscapeSoldier()->getRankString();
						}
						else											// Support unit
						{
							killStatRace = "STR_TANK";
							killStatRank = "STR_HEAVY_WEAPONS_PLATFORM_LC";
						}
					}
					else if (defendUnit->getOriginalFaction() == FACTION_HOSTILE)	// <- aLien DIED
					{
						killStatRace = defendUnit->getUnitRules()->getRace();
						killStatRank = defendUnit->getUnitRules()->getRank();
					}
					else if (defendUnit->getOriginalFaction() == FACTION_NEUTRAL)	// <- Civilian DIED
					{
						killStatPoints = -killStatPoints * 2;
						killStatRace = "STR_HUMAN";
						killStatRank = "STR_CIVILIAN";
					}
				}


				if ((dead == true
						&& defendUnit->getStatus() != STATUS_DEAD
						&& defendUnit->getStatus() != STATUS_COLLAPSING	// kL_note: is this really needed ....
						&& defendUnit->getStatus() != STATUS_TURNING	// kL: may be set by UnitDieBState cTor
						&& defendUnit->getStatus() != STATUS_DISABLED)	// kL
					|| converted == true)
				{
					if (execution == true)
						defendUnit->setStatus(STATUS_DEAD);
					else if (dead == true)
						defendUnit->setStatus(STATUS_DISABLED);

					// attacker's Morale Bonus & diary ->
					if (attackUnit != NULL)
					{
						defendUnit->killedBy(attackUnit->getFaction()); // used in DebriefingState.
						//Log(LOG_INFO) << "BSG::checkForCasualties() " << defendUnit->getId() << " killedBy = " << (int)attackUnit->getFaction();

						if (attackUnit->getGeoscapeSoldier() != NULL)
						{
							defendUnit->setMurdererId(attackUnit->getId());
							attackUnit->getStatistics()->kills.push_back(new BattleUnitKills(
																						killStatRank,
																						killStatRace,
																						killStatWeapon,
																						killStatWeaponAmmo,
																						defendUnit->getFaction(),
																						STATUS_DEAD,
																						killStatMission,
																						killStatTurn,
																						killStatPoints));
						}

						if (attackUnit->isFearable() == true)
						{
							int bonus;
							if (attackUnit->getOriginalFaction() == FACTION_PLAYER)
							{
								bonus = _battleSave->getMoraleModifier();

								if (attackUnit->getFaction() == FACTION_PLAYER		// not MC'd
									&& attackUnit->getGeoscapeSoldier() != NULL)	// is Soldier
								{
									attackUnit->addKillCount();
								}
							}
							else if (attackUnit->getOriginalFaction() == FACTION_HOSTILE)
								bonus = _battleSave->getMoraleModifier(NULL, false);
							else
								bonus = 0;

							// attacker's Valor
							if ((attackUnit->getOriginalFaction() == FACTION_HOSTILE
									&& defendUnit->getOriginalFaction() == FACTION_PLAYER)
								|| (attackUnit->getOriginalFaction() == FACTION_PLAYER
									&& defendUnit->getOriginalFaction() == FACTION_HOSTILE))
							{
								const int courage = 10 * bonus / 100;
								attackUnit->moraleChange(courage); // double what rest of squad gets below
							}
							// attacker (mc'd or not) will get a penalty with friendly fire (mc'd or not)
							// ... except aLiens, who don't care.
							else if (attackUnit->getOriginalFaction() == FACTION_PLAYER
								&& defendUnit->getOriginalFaction() == FACTION_PLAYER)
							{
								int chagrin = 5000 / bonus; // huge chagrin!
								if (defendUnit->getUnitRules() != NULL
									&& defendUnit->getUnitRules()->isMechanical() == true)
								{
									chagrin /= 2;
								}
								attackUnit->moraleChange(-chagrin);
							}
							else if (defendUnit->getOriginalFaction() == FACTION_NEUTRAL) // civilian kills
							{
								if (attackUnit->getOriginalFaction() == FACTION_PLAYER)
								{
									const int dishonor = 2000 / bonus;
									attackUnit->moraleChange(-dishonor);
								}
								else if (attackUnit->getOriginalFaction() == FACTION_HOSTILE)
									attackUnit->moraleChange(20); // no leadership bonus for aLiens executing civies: it's their job.
							}
						}
					}

					// cycle through units and do all faction
//					if (defendUnit->getFaction() != FACTION_NEUTRAL) // civie deaths now affect other Factions.
//					{
					// penalty for the death of a unit; civilians & MC'd aLien units return 100.
					const int loss = _battleSave->getMoraleModifier(defendUnit);
					// These two are factions (aTeam & bTeam leaderships mitigate losses).
					int
						aTeam, // winners
						bTeam; // losers

					if (defendUnit->getOriginalFaction() == FACTION_HOSTILE)
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
//						if ((*j)->isOut(true, true) == false
						if ((*j)->isOut_t() == false
							&& (*j)->isFearable() == true) // not mechanical. Or a ZOMBIE!!
						{
							if ((*j)->getOriginalFaction() == defendUnit->getOriginalFaction()
								|| (defendUnit->getOriginalFaction() == FACTION_NEUTRAL			// for civie-death,
									&& (*j)->getFaction() == FACTION_PLAYER						// non-Mc'd xCom takes hit
									&& (*j)->getOriginalFaction() != FACTION_HOSTILE)			// but not Mc'd aLiens
								|| (defendUnit->getOriginalFaction() == FACTION_PLAYER			// for death of xCom unit,
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
									else if (defendUnit->getUnitRules() != NULL
										&& defendUnit->getUnitRules()->isMechanical() == true)
									{
										moraleLoss /= 2;
									}

									(*j)->moraleChange(-moraleLoss);
								}
/*								if (attackUnit
									&& attackUnit->getFaction() == FACTION_PLAYER
									&& defendUnit->getFaction() == FACTION_HOSTILE)
								{
									attackUnit->setExposed(); // interesting
									//Log(LOG_INFO) << ". . . . attacker Exposed";
								} */
							}
							else if ((((*j)->getOriginalFaction() == FACTION_PLAYER
										|| (*j)->getOriginalFaction() == FACTION_NEUTRAL)
									&& defendUnit->getOriginalFaction() == FACTION_HOSTILE)
								|| ((*j)->getOriginalFaction() == FACTION_HOSTILE
									&& (defendUnit->getOriginalFaction() == FACTION_PLAYER
										|| defendUnit->getOriginalFaction() == FACTION_NEUTRAL)))
							{
								// winning faction(s) all get a morale boost unaffected by rank of the dead unit
								(*j)->moraleChange(aTeam / 10);
							}
						}
					}
//					}

					if (execution == true)
						return;

					if (converted == false)
					{
						ItemDamageType dType;
						bool noSound;

						if (weapon != NULL) // This is where units get sent to DEATH!
						{
							dType = weapon->getRules()->getDamageType();
							noSound = false;
						}
						else // hidden or terrain explosion or death by fatal wounds
						{
							if (hiddenExpl == true) // this is instant death from UFO powersources without screaming sounds
							{
								dType = DT_HE;
								noSound = true;
							}
							else
							{
								if (terrainExpl == true)
								{
									dType = DT_HE;
									noSound = false;
								}
								else // no attacker and no terrain explosion - must be fatal wounds
								{
									dType = DT_NONE; // -> STR_HAS_DIED_FROM_A_FATAL_WOUND
									noSound = false;
								}
							}
						}

						statePushNext(new UnitDieBState(
													this,
													*i,
													dType,
													noSound));
					}
				}
				else if (stunned == true
					&& defendUnit->getStatus() != STATUS_DEAD
					&& defendUnit->getStatus() != STATUS_UNCONSCIOUS
					&& defendUnit->getStatus() != STATUS_COLLAPSING	// kL_note: is this really needed ....
					&& defendUnit->getStatus() != STATUS_TURNING	// kL_note: may be set by UnitDieBState cTor
					&& defendUnit->getStatus() != STATUS_DISABLED)	// kL
				{
					(*i)->setStatus(STATUS_DISABLED); // kL

					if (attackUnit != NULL
						&& attackUnit->getGeoscapeSoldier() != NULL)
					{
						attackUnit->getStatistics()->kills.push_back(new BattleUnitKills(
																					killStatRank,
																					killStatRace,
																					killStatWeapon,
																					killStatWeaponAmmo,
																					defendUnit->getFaction(),
																					STATUS_UNCONSCIOUS,
																					killStatMission,
																					killStatTurn,
																					killStatPoints));
					}

					if (defendUnit != NULL
						&& defendUnit->getGeoscapeSoldier() != NULL)
					{
						defendUnit->getStatistics()->wasUnconscious = true;
					}

					statePushNext(new UnitDieBState( // This is where units get sent to STUNNED.
												this,
												*i,
												DT_STUN,
												true));
				}
			}
		}
	}


	if (hiddenExpl == false)
	{
		if (_battleSave->getSide() == FACTION_PLAYER)
		{
			const BattleUnit* const unit = _battleSave->getSelectedUnit();
			_parentState->showPsiButton(unit != NULL
									 && unit->getOriginalFaction() == FACTION_HOSTILE
									 && unit->getBaseStats()->psiSkill > 0
									 && unit->isOut_t() == false);
//									 && unit->isOut(true, true) == false);
		}

/*		if (_battleSave->getTacticalType() == TCT_BASEASSAULT // do this in SavedBattleGame::addDestroyedObjective()
			&& _battleSave->getControlDestroyed() == false)
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
				_battleSave->setControlDestroyed();

				Game* const game = _parentState->getGame();
				game->pushState(new InfoboxOKState(game->getLanguage()->getString("STR_ALIEN_BASE_CONTROL_DESTROYED")));
			}
		} */
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
 * @param unit	- pointer to a unit
 * @param tu	- # of time units to check against
 * @param test	- true to suppress error messages (default false)
 * @return, true if unit has enough time units - go!
 */
bool BattlescapeGame::checkReservedTu(
		const BattleUnit* const unit,
		int tu,
		bool test)
{
	if (unit->getFaction() != _battleSave->getSide()	// is RF.
		|| _battleSave->getSide() == FACTION_NEUTRAL)	// or Civies
	{
		return tu <= unit->getTimeUnits();
	}


	BattleActionType batReserved = _battleSave->getBatReserved(); // avoid changing _batReserved here.

	if (_battleSave->getSide() == FACTION_HOSTILE
		&& _debugPlay == false)
	{
		const AlienBAIState* const ai = dynamic_cast<AlienBAIState*>(unit->getCurrentAIState());
		if (ai != NULL)
			batReserved = ai->getReservedAIAction();
		else
			batReserved = BA_NONE; // something went ... wrong. Should always be an AI for non-player units.

		const int extraReserve = RNG::generate(0,13); // kL, added in below ->

		// kL_note: This could use some tweaking, for the poor aLiens:
		switch (batReserved) // aLiens reserve TUs as a percentage rather than just enough for a single action.
		{
			case BA_SNAPSHOT:
			return (tu + extraReserve + (unit->getBaseStats()->tu / 3) <= unit->getTimeUnits());		// 33%

			case BA_AUTOSHOT:
			return (tu + extraReserve + (unit->getBaseStats()->tu * 2 / 5) <= unit->getTimeUnits());	// 40%

			case BA_AIMEDSHOT:
			return (tu + extraReserve + (unit->getBaseStats()->tu / 2) <= unit->getTimeUnits());		// 50%

			default:
			return (tu <= unit->getTimeUnits()); // + extraReserve
		}
	}

	// ** Below here is for xCom soldiers exclusively ***
	// ( which i don't care about - except that this is also used for pathPreviews in Pathfinding object )

	// check TUs against slowest weapon if we have two weapons
//	const BattleItem* const weapon = unit->getMainHandWeapon(false);
	// kL: Use getActiveHand() instead, if xCom wants to reserve TU & for pathPreview.
	const BattleItem* const weapon = unit->getItem(unit->getActiveHand());

	// if reserved for Aimed shot drop to Auto shot
	if (batReserved == BA_AIMEDSHOT
		&& unit->getActionTUs(
						BA_AIMEDSHOT,
						weapon) == 0)
	{
		batReserved = BA_AUTOSHOT;
	}

	// if reserved for Auto shot drop to Snap shot
	if (batReserved == BA_AUTOSHOT
		&& unit->getActionTUs(
						BA_AUTOSHOT,
						weapon) == 0)
	{
		batReserved = BA_SNAPSHOT;
	}

	// if reserved for Snap shot try Auto shot
	if (batReserved == BA_SNAPSHOT
		&& unit->getActionTUs(
						BA_SNAPSHOT,
						weapon) == 0)
	{
		batReserved = BA_AUTOSHOT;
	}

	// if reserved for Auto shot try Aimed shot
	if (batReserved == BA_AUTOSHOT
		&& unit->getActionTUs(
						BA_AUTOSHOT,
						weapon) == 0)
	{
		batReserved = BA_AIMEDSHOT;
	}

	int tuKneel;
	if (_battleSave->getKneelReserved() == true
		&& unit->getGeoscapeSoldier() != NULL
		&& unit->isKneeled() == false)
	{
		tuKneel = 3;
	}
	else
		tuKneel = 0;

	// if no Aimed shot is available revert to bat_NONE
	if (batReserved == BA_AIMEDSHOT
		&& unit->getActionTUs(
						BA_AIMEDSHOT,
						weapon) == 0)
	{
		if (tuKneel != 0)
			batReserved = BA_NONE;
		else
			return true;
	}

	if ((batReserved != BA_NONE
			|| _battleSave->getKneelReserved() == true)
		&& tu + tuKneel + unit->getActionTUs(
										batReserved,
										weapon) > unit->getTimeUnits()
		&& (tuKneel + unit->getActionTUs(
									batReserved,
									weapon) <= unit->getTimeUnits()
			|| test == true))
	{
		if (test == false)
		{
			if (tuKneel != 0)
			{
				switch (batReserved)
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
				switch (_battleSave->getBatReserved())
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
 * @note Called from BattlescapeGame::think() at start of player's turn.
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
 * @note Called from BattlescapeGame::think() at start of non-player turn.
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

		if (Options::alienPanicMessages == true
			|| unit->getUnitVisible() == true)
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
		int tu = unit->getTimeUnits();

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

				const std::vector<int> reachable = pf->findReachable(
																	unit,
																	tu);
				const size_t
					pick = static_cast<size_t>(RNG::generate(
														0,
														static_cast<int>(reachable.size()) - 1)),
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
					ba.type = BA_MOVE;
					statePushBack(new UnitWalkBState(
													this,
													ba));
				}
			}
			break;

			case STATUS_BERSERK:
			{
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
							int shots; // tabulate how many shots can be fired before unit runs out of TUs
							if (actionTu != 0)
								shots = tu / actionTu;
							else
								shots = 0;

							for (int
									i = 0;
									i != shots;
									++i)
							{
								statePushBack(new ProjectileFlyBState(
																	this,
																	ba));
							}
						}
					}
					else if (ba.weapon->getRules()->getBattleType() == BT_GRENADE)
					{
						if (ba.weapon->getFuse() == -1)
							ba.weapon->setFuse(0); // yeh set timer even if throw is invalid.

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

				ba.type = BA_NONE;
			}
		}

		statePushBack(new UnitPanicBState(
										this,
										unit));
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

				_currentAction.targeting = false;
				_currentAction.type = BA_NONE;

				if (_battleSave->getSide() == FACTION_PLAYER)
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
 * @param pos - reference a Position on the map
 */
void BattlescapeGame::primaryAction(const Position& pos)
{
	//Log(LOG_INFO) << "BattlescapeGame::primaryAction()";
	//if (_battleSave->getSelectedUnit()) Log(LOG_INFO) << ". ID " << _battleSave->getSelectedUnit()->getId();
	_currentAction.actor = _battleSave->getSelectedUnit();
	BattleUnit* const targetUnit = _battleSave->selectUnit(pos);

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
				_currentAction.waypoints.push_back(pos);
				getMap()->getWaypoints()->push_back(pos);
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
							_currentAction.actor->getHostileUnits()->begin(),
							_currentAction.actor->getHostileUnits()->end(),
							targetUnit) != _currentAction.actor->getHostileUnits()->end())
				{
					if (getTileEngine()->distance( // in Range
											_currentAction.actor->getPosition(),
											_currentAction.target) <= _currentAction.weapon->getRules()->getMaxRange())
					{
						if (_currentAction.actor->spendTimeUnits(_currentAction.TU) == true)
						{
							getResourcePack()->getSound(
													"BATTLE.CAT",
													_currentAction.weapon->getRules()->getHitSound())
												->play(
													-1,
													getMap()->getSoundAngle(pos));

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
		else if (_currentAction.type == BA_PSIPANIC
			|| _currentAction.type == BA_PSICONTROL
			|| _currentAction.type == BA_PSICONFUSE
			|| _currentAction.type == BA_PSICOURAGE)
		{
			//Log(LOG_INFO) << ". . . . BA_PSIPANIC or BA_PSICONTROL or BA_PSICONFUSE or BA_PSICOURAGE";
			if (targetUnit != NULL
				&& targetUnit->getUnitVisible() == true
				&& ((_currentAction.type != BA_PSICOURAGE
						&& targetUnit->getFaction() != FACTION_PLAYER)
					|| (_currentAction.type == BA_PSICOURAGE
						&& targetUnit->getFaction() != FACTION_HOSTILE)))
			{
				bool aLienPsi = (_currentAction.weapon == NULL);
				if (aLienPsi == true)
					_currentAction.weapon = _alienPsi;

				_currentAction.target = pos;
				_currentAction.TU = _currentAction.actor->getActionTUs(
																	_currentAction.type,
																	_currentAction.weapon);

				if (_currentAction.weapon->getRules()->isLOSRequired() == false
					|| std::find(
							_currentAction.actor->getHostileUnits()->begin(),
							_currentAction.actor->getHostileUnits()->end(),
							targetUnit) != _currentAction.actor->getHostileUnits()->end())
				{
					if (getTileEngine()->distance( // in Range
											_currentAction.actor->getPosition(),
											_currentAction.target) <= _currentAction.weapon->getRules()->getMaxRange())
					{
						// get the sound/animation started
//						getMap()->setCursorType(CT_NONE);
//						_parentState->getGame()->getCursor()->setVisible(false);
//						_currentAction.cameraPosition = getMap()->getCamera()->getMapOffset();
						_currentAction.cameraPosition = Position(0,0,-1); // don't adjust the camera when coming out of ProjectileFlyB/ExplosionB states


//						statePushBack(new PsiAttackBState(this, _currentAction));
						//Log(LOG_INFO) << ". . . . . . new ProjectileFlyBState";
						statePushBack(new ProjectileFlyBState(
															this,
															_currentAction));

						if (_currentAction.actor->getTimeUnits() >= _currentAction.TU) // WAIT, check this *before* all the stuff above!!!
						{
							if (getTileEngine()->psiAttack(&_currentAction) == true)
							{
								//Log(LOG_INFO) << ". . . . . . Psi successful";
								Game* const game = _parentState->getGame(); // show an infobox if successful

								std::wstring wst;
								if (_currentAction.type == BA_PSIPANIC)
									wst = game->getLanguage()->getString("STR_PSI_PANIC_SUCCESS")
																	.arg(_currentAction.value);
								else if (_currentAction.type == BA_PSICONTROL)
									wst = game->getLanguage()->getString("STR_PSI_CONTROL_SUCCESS")
																	.arg(_currentAction.value);
								else if (_currentAction.type == BA_PSICONFUSE)
									wst = game->getLanguage()->getString("STR_PSI_CONFUSE_SUCCESS")
																	.arg(_currentAction.value);
								else if (_currentAction.type == BA_PSICOURAGE)
									wst = game->getLanguage()->getString("STR_PSI_COURAGE_SUCCESS")
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
				|| pos != _currentAction.waypoints.front()))
		{
			_currentAction.waypoints.clear();
			_currentAction.waypoints.push_back(pos);

			getMap()->getWaypoints()->clear();
			getMap()->getWaypoints()->push_back(pos);
		}
		else
		{
			//Log(LOG_INFO) << ". . . . FIRING or THROWING";
			if (Options::battleConfirmFireMode == true)
			{
				_currentAction.waypoints.clear();
				getMap()->getWaypoints()->clear();
			}

			getMap()->setCursorType(CT_NONE);
			_parentState->getGame()->getCursor()->setHidden();

			_currentAction.target = pos;
			_currentAction.cameraPosition = getMap()->getCamera()->getMapOffset();

			_states.push_back(new ProjectileFlyBState(		// TODO: should check for valid LoF/LoT *before* invoking this
													this,	// instead of the (flakey) checks in that state. Then conform w/ AI ...
													_currentAction));

			statePushFront(new UnitTurnBState(
											this,
											_currentAction));
			//Log(LOG_INFO) << ". . . . FIRING or THROWING done";
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
				modCtrl = (SDL_GetModState() & KMOD_CTRL) != 0,
				modAlt = (SDL_GetModState() & KMOD_ALT) != 0,
				isMech = _currentAction.actor->getUnitRules() != NULL
					  && _currentAction.actor->getUnitRules()->isMechanical() == true;

//			_currentAction.dash = false;
//			_currentAction.actor->setDashing(false);

			if (targetUnit != NULL // spin 180 degrees
				&& targetUnit == _currentAction.actor
				&& modCtrl == false
				&& modAlt == false
				&& isMech == false
				&& _currentAction.actor->getArmor()->getSize() == 1) // small units only
			{
				if (allowPreview == true)
					pf->removePreview();

				Position screenPixel;
				getMap()->getCamera()->convertMapToScreen(
														pos,
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
				_currentAction.target += pos;

				statePushBack(new UnitTurnBState(
											this,
											_currentAction));
			}
			else // handle pathPreview and MOVE
			{
				if (allowPreview == true
					&& (_currentAction.target != pos
						|| pf->isModCtrl() != modCtrl
						|| pf->isModAlt() != modAlt))
				{
					pf->removePreview();
				}

				_currentAction.target = pos;
				pf->calculate(
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

					if (allowPreview == false)
					{
						getMap()->setCursorType(CT_NONE);
						_parentState->getGame()->getCursor()->setHidden();

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
 * @param pos - reference a Position on the map
 */
void BattlescapeGame::secondaryAction(const Position& pos)
{
	_currentAction.actor = _battleSave->getSelectedUnit();

	if (_currentAction.actor->getPosition() == pos)
	{
		_parentState->btnKneelClick(NULL); // could put just about anything in here Orelly.
		return;
	}

	// -= turn to or open door =-
	_currentAction.target = pos;
	_currentAction.strafe = _currentAction.actor->getTurretType() > -1
						 && (SDL_GetModState() & KMOD_CTRL) != 0
						 && Options::strafe == true;

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
	_parentState->getGame()->getCursor()->setHidden();

//	_currentAction.cameraPosition = getMap()->getCamera()->getMapOffset();

	_states.push_back(new ProjectileFlyBState(
											this,
											_currentAction));
	statePushFront(new UnitTurnBState(
									this,
									_currentAction));
}

/**
 * Handler for the psi button.
 * @note Additional data gets assigned in primaryAction().
 */
void BattlescapeGame::psiButtonAction()
{
	_currentAction.weapon = NULL;
	_currentAction.targeting = true;
	_currentAction.type = BA_PSIPANIC;

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
		++_currentAction.target.z;
	else
		--_currentAction.target.z;

	getMap()->setCursorType(CT_NONE);
	_parentState->getGame()->getCursor()->setHidden();

	Pathfinding* const pf = _battleSave->getPathfinding();
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
 * @param pos		- reference position to place the item
 * @param item		- pointer to the item
 * @param create	- true if this is a new item (default false)
 * @param disown	- true to remove the item from the owner (default false)
 */
void BattlescapeGame::dropItem(
		const Position& pos,
		BattleItem* const item,
		bool created,
		bool disown)
{
	if (_battleSave->getTile(pos) != NULL			// don't spawn anything outside of bounds
		&& item->getRules()->isFixed() == false)	// don't ever drop fixed items
	{
		_battleSave->getTile(pos)->addItem(
										item,
										getRuleset()->getInventory("STR_GROUND"));

		if (item->getUnit() != NULL)
			item->getUnit()->setPosition(pos);

		if (created == true)
			_battleSave->getItems()->push_back(item);

		if (disown == true)
			item->moveToOwner(NULL);
		else if (item->getRules()->isGrenade() == false)
			item->setOwner(NULL);

		getTileEngine()->applyGravity(_battleSave->getTile(pos));

		if (item->getRules()->getBattleType() == BT_FLARE)
		{
			getTileEngine()->calculateTerrainLighting();
			getTileEngine()->recalculateFOV(true);
		}
	}
}

/**
 * Converts a unit into a unit of another type.
 * @param unit		- pointer to a unit to convert
 * @return, pointer to the new unit
 */
BattleUnit* BattlescapeGame::convertUnit(BattleUnit* const unit)
{
	//Log(LOG_INFO) << "BattlescapeGame::convertUnit() " << conType;
	const bool wasVisible = unit->getUnitVisible();

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


	const std::string conType = unit->getSpawnUnit();
	RuleUnit* const unitRule = getRuleset()->getUnit(conType);
	const std::string armorType = unitRule->getArmor();

	BattleUnit* const conUnit = new BattleUnit(
											unitRule,
											FACTION_HOSTILE,
											_battleSave->getUnits()->back()->getId() + 1,
											getRuleset()->getArmor(armorType),
											_parentState->getGame()->getSavedGame()->getDifficulty(),
											_parentState->getGame()->getSavedGame()->getMonthsPassed(),
											this);

	const Position posUnit = unit->getPosition();
	_battleSave->getTile(posUnit)->setUnit(
										conUnit,
										_battleSave->getTile(posUnit + Position(0,0,-1)));
	conUnit->setPosition(posUnit);
	conUnit->setTimeUnits(0);

	int dir;
	if (conType == "STR_ZOMBIE") // TODO: RuleUnit var '_isZombie' -- narrow FoV etc.
		dir = RNG::generate(0,7); // or, (unit->getDirection())
	else
		dir = 3;
	conUnit->setDirection(dir);

	_battleSave->getUnits()->push_back(conUnit);

	conUnit->setAIState(new AlienBAIState(
										_battleSave,
										conUnit,
										NULL));

	const std::string terrorWeapon = unitRule->getRace().substr(4) + "_WEAPON";
	BattleItem* const item = new BattleItem(
										getRuleset()->getItem(terrorWeapon),
										_battleSave->getNextItemId());
	item->moveToOwner(conUnit);
	item->setSlot(getRuleset()->getInventory("STR_RIGHT_HAND"));
	_battleSave->getItems()->push_back(item);

	getMap()->cacheUnit(conUnit);

	conUnit->setUnitVisible(wasVisible);

	getTileEngine()->applyGravity(conUnit->getTile());
//	getTileEngine()->calculateUnitLighting(); // <- done in UnitDieBState. But does pre-Spawned unit always go through UnitDieBState, and if not does it matter ...
	getTileEngine()->calculateFOV(
							conUnit->getPosition(),
							true);

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
SavedBattleGame* BattlescapeGame::getBattleSave() const
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
void BattlescapeGame::findItem(BattleAction* const action) const
{
	//Log(LOG_INFO) << "BattlescapeGame::findItem()";
	if (action->actor->getRankString() != "STR_LIVE_TERRORIST")
	{
		BattleItem* const targetItem = surveyItems(action->actor);
		if (targetItem != NULL)
		{
			if (targetItem->getTile()->getPosition() == action->actor->getPosition())
			{
				if (takeItemFromGround(
									targetItem,
									action->actor) == 0
					&& targetItem->getAmmoItem() == NULL)
				{
					action->actor->checkAmmo();
				}
			}
			else
			{
				action->target = targetItem->getTile()->getPosition();
				action->type = BA_MOVE;
			}
		}
	}
}

/**
 * Searches through items on the map that were dropped on an alien turn and
 * picks the most attractive one.
 * @param unit - pointer to the BattleUnit looking for an item
 * @return, the BattleItem to go for
 */
BattleItem* BattlescapeGame::surveyItems(BattleUnit* const unit) const
{
	//Log(LOG_INFO) << "BattlescapeGame::surveyItems()";
	const Tile* tile;
	std::vector<BattleItem*> groundItems;
	for (std::vector<BattleItem*>::const_iterator
			i = _battleSave->getItems()->begin();
			i != _battleSave->getItems()->end();
			++i)
	{
		if ((*i)->getSlot() != NULL
			&& (*i)->getSlot()->getId() == "STR_GROUND"
			&& (*i)->getRules()->getAttraction() != 0)
		{
			tile = (*i)->getTile();
			if (tile != NULL
				&& (tile->getUnit() == NULL
					|| tile->getUnit() == unit)
				&& tile->getTuCostTile(O_FLOOR, MT_WALK) != 255 // TODO:: pathfind.
				&& tile->getTuCostTile(O_OBJECT, MT_WALK) != 255
				&& worthTaking(
							*i,
							unit) == true)
			{
				groundItems.push_back(*i);
			}
		}
	}

	BattleItem* ret = NULL;

	if (groundItems.empty() == false) // Select the most suitable candidate depending on attraction and distance.
	{
		const Position posUnit = unit->getPosition();
		int
			worth = 0,
			worthTest,
			dist;

		std::vector<BattleItem*> choiceItems;
		for (std::vector<BattleItem*>::const_iterator
				i = groundItems.begin();
				i != groundItems.end();
				++i)
		{
			dist = _battleSave->getTileEngine()->distance(
														posUnit,
														(*i)->getTile()->getPosition());
			worthTest = (*i)->getRules()->getAttraction() / (dist + 1);
			if (worthTest >= worth)
			{
				if (worthTest > worth)
				{
					worth = worthTest;
					choiceItems.clear();
				}

				choiceItems.push_back(*i);
			}
		}

		if (choiceItems.empty() == false)
		{
			size_t pick = static_cast<size_t>(RNG::generate(
														0,
														static_cast<int>(choiceItems.size()) - 1));
			ret = choiceItems.at(pick);
		}
	}

	return ret;
}

/**
 * Assesses whether an item is worth trying to pick up.
 * @param item - pointer to a BattleItem to go for
 * @param unit - pointer to the BattleUnit looking for an item
 * @return, true if the item is worth going for
 */
bool BattlescapeGame::worthTaking( // TODO: rewrite & rework into rest of pickup code !
		BattleItem* const item,
		BattleUnit* const unit) const
{
	//Log(LOG_INFO) << "BattlescapeGame::worthTaking()";
	const int spaceReqd = item->getRules()->getInventoryHeight() * item->getRules()->getInventoryWidth();
	int spaceFree = 25; // note that my compiler throws a hissy-fit every time it sees this chunk of code ...
	for (std::vector<BattleItem*>::const_iterator
			i = unit->getInventory()->begin();
			i != unit->getInventory()->end();
			++i)
	{
		spaceFree -= (*i)->getRules()->getInventoryHeight() * (*i)->getRules()->getInventoryWidth();
		if (spaceFree < spaceReqd)
			return false;
	}


	bool ret = false;

	if (item->getRules()->getBattleType() == BT_AMMO)
	{
		for (std::vector<BattleItem*>::const_iterator
				i = unit->getInventory()->begin();
				i != unit->getInventory()->end()
					&& ret == false;
				++i)
		{
			if ((*i)->getRules()->getBattleType() == BT_FIREARM)
			{
				for (std::vector<std::string>::const_iterator
						j = (*i)->getRules()->getCompatibleAmmo()->begin();
						j != (*i)->getRules()->getCompatibleAmmo()->end()
							&& ret == false;
						++j)
				{
					if (*j == (*i)->getRules()->getName())
						ret = true;
				}
			}
		}
	}
	else if (item->getAmmoItem() == NULL) // not loaded
	{
		for (std::vector<BattleItem*>::const_iterator
				i = unit->getInventory()->begin();
				i != unit->getInventory()->end()
					&& ret == false;
				++i)
		{
			if ((*i)->getRules()->getBattleType() == BT_AMMO)
			{
				for (std::vector<std::string>::const_iterator
						j = item->getRules()->getCompatibleAmmo()->begin();
						j != item->getRules()->getCompatibleAmmo()->end()
							&& ret == false;
						++j)
				{
					if (*j == (*i)->getRules()->getName())
						ret = true;
				}
			}
		}
	}

	// The problem here, in addition to the quirky space-calculation, is that
	// only weapons and ammo are considered worthwhile.
	return ret;
}

/**
 * Picks the item up from the ground.
 * @note At this point the unit has decided it's worthwhile to grab the item
 * so try to do just that. First check to make sure actor has time units then
 * that there is space (using horrifying logic!) attempt to actually recover
 * the item.
 * @param item - pointer to a BattleItem to go for
 * @param unit - pointer to the BattleUnit looking for an item
 * @return,	 0 - successful
 *			 1 - no-TUs
 *			 2 - not-enough-room
 *			 3 - won't-fit
 *			-1 - something-went-horribly-wrong
 */
int BattlescapeGame::takeItemFromGround(
		BattleItem* const item,
		BattleUnit* const unit) const
{
	//Log(LOG_INFO) << "BattlescapeGame::takeItemFromGround()";
	const int
		TAKE_SUCCESS	=  0,
		TAKE_NO_TU		=  1,
		TAKE_NO_SPACE	=  2,
		TAKE_NO_FIT		=  3;

	if (unit->getTimeUnits() < 6) // should replace that w/ Ground-to-Hand TU rule.
		return TAKE_NO_TU;
	else
	{
		const int spaceReqd = item->getRules()->getInventoryHeight() * item->getRules()->getInventoryWidth();
		int spaceFree = 25; // note that my compiler throws a hissy-fit every time it sees this chunk of code ...
		for (std::vector<BattleItem*>::const_iterator
				i = unit->getInventory()->begin();
				i != unit->getInventory()->end();
				++i)
		{
			spaceFree -= (*i)->getRules()->getInventoryHeight() * (*i)->getRules()->getInventoryWidth();
			if (spaceFree < spaceReqd)
				return TAKE_NO_SPACE;
		}

		if (takeItem(
					item,
					unit) == true)
		{
			unit->spendTimeUnits(6);
			item->getTile()->removeItem(item);

			return TAKE_SUCCESS;
		}

		return TAKE_NO_FIT;
	}
}

/**
 * Tries to fit an item into the unit's inventory.
 * @param item - pointer to a BattleItem to go for
 * @param unit - pointer to the BattleUnit looking for an item
 * @return, true if @a item was successfully retrieved
 */
bool BattlescapeGame::takeItem( // TODO: rewrite & rework into rest of pickup code !
		BattleItem* const item,
		BattleUnit* const unit) const
{
	//Log(LOG_INFO) << "BattlescapeGame::takeItem()";
	bool placed = false;

	switch (item->getRules()->getBattleType())
	{
		case BT_AMMO:
			if (unit->getItem("STR_RIGHT_HAND")
				&& unit->getItem("STR_RIGHT_HAND")->getAmmoItem() == NULL)
			{
				if (unit->getItem("STR_RIGHT_HAND")->setAmmoItem(item) == 0)
					placed = true;
			}
			else
			{
				for (int
						i = 0;
						i != 4; // uhh, my Belts have only 3 x-positions
						++i)
				{
					if (unit->getItem("STR_BELT", i) == NULL)
					{
						item->moveToOwner(unit);
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
					i != 4; // uhh, my Belts have only 3 x-positions
					++i)
			{
				if (unit->getItem("STR_BELT", i) == NULL)
				{
					item->moveToOwner(unit);
					item->setSlot(getRuleset()->getInventory("STR_BELT"));
					item->setSlotX(i);

					placed = true;
					break;
				}
			}
		break;

		case BT_FIREARM:
		case BT_MELEE:
			if (unit->getItem("STR_RIGHT_HAND") == NULL)
			{
				item->moveToOwner(unit);
				item->setSlot(getRuleset()->getInventory("STR_RIGHT_HAND"));

				placed = true;
			}
		break;

		case BT_MEDIKIT:
		case BT_SCANNER:
			if (unit->getItem("STR_BACK_PACK") == NULL)
			{
				item->moveToOwner(unit);
				item->setSlot(getRuleset()->getInventory("STR_BACK_PACK"));

				placed = true;
			}
		break;

		case BT_MINDPROBE:
			if (unit->getItem("STR_LEFT_HAND") == NULL)
			{
				item->moveToOwner(unit);
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
	_battleSave->setBatReserved(bat);
}

/**
 * Returns the action type that is reserved.
 * @return, the BattleActionType that is reserved
 */
BattleActionType BattlescapeGame::getReservedAction() const
{
	return _battleSave->getBatReserved();
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
//		if ((*j)->isOut() == false)
		if ((*j)->isOut_t(OUT_STAT) == false)
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
bool BattlescapeGame::checkProxyGrenades(BattleUnit* const unit)
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
								&& (*i)->getFuse() != -1)
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

/**
 * Sets up a mission complete notification.
 */
void BattlescapeGame::objectiveDone()
{
	const Game* const game = _parentState->getGame();
	const AlienDeployment* const deployRule = game->getRuleset()->getDeployment(_battleSave->getMissionType());
	if (deployRule != NULL)
	{
		const std::string messagePop = deployRule->getObjectivePopup();
		if (messagePop.empty() == false)
			_infoboxQueue.push_back(new InfoboxOKState(game->getLanguage()->getString(messagePop)));
	}
}

/**
 * Sets if an execution is underway and needs animation.
 * @param execute - true to execute (default true)
 */
void BattlescapeGame::setExecution(bool execute)
{
	_executeProgress = execute;
}

/**
 * Gets if an execution is underway and needs animation.
 * @return, true if execute
 */
bool BattlescapeGame::getExecution() const
{
	return _executeProgress;
}

}
