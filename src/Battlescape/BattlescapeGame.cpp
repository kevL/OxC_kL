/**
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

#define _USE_MATH_DEFINES

#include <cmath>
#include <sstream>
#include <typeinfo>
#include "BattlescapeGame.h"
#include "BattlescapeState.h"
#include "Map.h"
#include "Camera.h"
#include "NextTurnState.h"
#include "AbortMissionState.h"
#include "BattleState.h"
#include "UnitTurnBState.h"
#include "UnitWalkBState.h"
#include "ProjectileFlyBState.h"
#include "ExplosionBState.h"
#include "TileEngine.h"
#include "UnitInfoState.h"
#include "UnitDieBState.h"
#include "UnitPanicBState.h"
#include "AlienBAIState.h"
#include "CivilianBAIState.h"
#include "Pathfinding.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Sound.h"
#include "../Resource/ResourcePack.h"
#include "../Interface/Cursor.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/BattleItem.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/RuleInventory.h"
#include "../Ruleset/Armor.h"
#include "../Engine/Options.h"
#include "../Engine/RNG.h"
#include "InfoboxState.h"
#include "InfoboxOKState.h"
#include "UnitFallBState.h"
#include "../Engine/Logger.h"


namespace OpenXcom
{

bool BattlescapeGame::_debugPlay = false;

/**
 * Initializes all the elements in the Battlescape screen.
 * @param save Pointer to the save game.
 * @param parentState Pointer to the parent battlescape state.
 */
BattlescapeGame::BattlescapeGame(SavedBattleGame* save, BattlescapeState* parentState)
	:
		_save(save),
		_parentState(parentState),
		_playedAggroSound(false),
		_endTurnRequested(false),
		_kneelReserved(false)
{
	//Log(LOG_INFO) << "Create BattlescapeGame";

	_tuReserved = BA_NONE;
	_playerTUReserved = BA_NONE;
	_debugPlay = false;
	_playerPanicHandled = true;
	_AIActionCounter = 0;
	_AISecondMove = false;
	_currentAction.actor = 0;

	checkForCasualties(0, 0, true);
	cancelCurrentAction();
	_currentAction.targeting = false;
	_currentAction.type = BA_NONE;
}

/**
 * Delete BattlescapeGame.
 */
BattlescapeGame::~BattlescapeGame()
{
	//Log(LOG_INFO) << "Delete BattlescapeGame";
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
						handleAI(_save->getSelectedUnit());
				}
				else
				{
					if (_save->selectNextPlayerUnit(true, _AISecondMove) == 0)
					{
						if (!_save->getDebugMode())
						{
							_endTurnRequested = true;
							statePushBack(0); // end AI turn
						}
						else
						{
							_save->selectNextPlayerUnit();
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
			Log(LOG_INFO) << "BattlescapeGame::think(), get/setUnitsFalling() ID " << _save->getSelectedUnit()->getId();

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
	if (_save->getSide() == FACTION_PLAYER)
	{
		_playerPanicHandled = false;
	}
}

/**
 * Handles the processing of the AI states of a unit.
 * @param unit, Pointer to a unit
 */
void BattlescapeGame::handleAI(BattleUnit* unit)
{
	Log(LOG_INFO) << "BattlescapeGame::handleAI()";

	std::wstringstream ss;

	_tuReserved = BA_NONE;
	if (unit->getTimeUnits() < 6)
	{
		unit->dontReselect();
	}

	if (unit->getTimeUnits() < 6
		|| _AIActionCounter > 1
		|| !unit->reselectAllowed())
	{
		if (_save->selectNextPlayerUnit(true, _AISecondMove) == 0)
		{
			if (!_save->getDebugMode())
			{
				_endTurnRequested = true;
				//Log(LOG_INFO) << "BattlescapeGame::handleAI() statePushBack(end AI turn)";
				statePushBack(0); // end AI turn
			}
			else
			{
				_save->selectNextPlayerUnit();
				_debugPlay = true;
			}
		}

		if (_save->getSelectedUnit())
		{
			getMap()->getCamera()->centerOnPosition(_save->getSelectedUnit()->getPosition());
			if (_save->getSelectedUnit()->getId() <= unit->getId())
			{
				_AISecondMove = true;
			}
		}

		_AIActionCounter = 0;

		//Log(LOG_INFO) << "BattlescapeGame::handleAI() EXIT 1";
		return;
	}

	if (unit->getMainHandWeapon()
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
	}
	//Log(LOG_INFO) << ". aggressionReserved DONE";


	unit->setVisible(false);

	// might need this populate _visibleUnit for a newly-created alien
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
			unit->setAIState(new AlienBAIState(_save, unit, 0));
		else
			unit->setAIState(new CivilianBAIState(_save, unit, 0));

		ai = unit->getCurrentAIState();
	}

	_AIActionCounter++;
	if (_AIActionCounter == 1)
	{
		_playedAggroSound = false;
		unit->_hidingForTurn = false;

		if (_save->getTraceSetting()) { Log(LOG_INFO) << "#" << unit->getId() << "--" << unit->getType(); }
	}
	//Log(LOG_INFO) << ". _AIActionCounter DONE";


	// this cast only works when ai was already AlienBAIState at heart
//	AlienBAIState* aggro = dynamic_cast<AlienBAIState*>(ai);

	BattleAction action;
	action.actor = unit;
    action.number = _AIActionCounter;
	unit->think(&action);

	if (action.type == BA_RETHINK)
	{
		_parentState->debug(L"Rethink");
		unit->think(&action);
	}
	//Log(LOG_INFO) << ". BA_RETHINK DONE";


	_AIActionCounter = action.number;

	if (!unit->getMainHandWeapon()
		|| !unit->getMainHandWeapon()->getAmmoItem())
	{
		if (unit->getOriginalFaction() == FACTION_HOSTILE
			&& unit->getVisibleUnits()->size() == 0)
		{
			findItem(&action);
		}
	}
	//Log(LOG_INFO) << ". findItem DONE";


	if (unit->getCharging() != 0)
	{
		if (unit->getAggroSound() != -1
			&& !_playedAggroSound)
		{
			getResourcePack()->getSound("BATTLE.CAT", unit->getAggroSound())->play();
			_playedAggroSound = true;
		}
	}
	//Log(LOG_INFO) << ". getCharging DONE";


	if (action.type == BA_WALK)
	{
		ss << L"Walking to " << action.target;
		_parentState->debug(ss.str());

		if (_save->getTile(action.target))
		{
			_save->getPathfinding()->calculate(action.actor, action.target);//, _save->getTile(action.target)->getUnit());
		}

		if (_save->getPathfinding()->getStartDirection() != -1)
		{
			statePushBack(new UnitWalkBState(this, action));
		}
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
		}
		else
		{
			statePushBack(new UnitTurnBState(this, action));
		}
		//Log(LOG_INFO) << ". . create Psi weapon DONE";


		ss.clear();
		ss << L"Attack type=" << action.type << " target="<< action.target << " weapon=" << action.weapon->getRules()->getName().c_str();
		_parentState->debug(ss.str());

		//Log(LOG_INFO) << ". . ProjectileFlyBState";
		statePushBack(new ProjectileFlyBState(this, action));
		//Log(LOG_INFO) << ". . ProjectileFlyBState DONE";

		if (action.type == BA_MINDCONTROL
			|| action.type == BA_PANIC)
		{
			//Log(LOG_INFO) << ". . . in action.type Psi";

			bool success = _save->getTileEngine()->psiAttack(&action); // crash ???? ... sometimes!!
			//Log(LOG_INFO) << ". . . success = " << success;
			if (success
				&& action.type == BA_MINDCONTROL)
			{
				//Log(LOG_INFO) << ". . . inside success MC";
				// show a little infobox with the name of the unit and "... is under alien control"
				Game* game = _parentState->getGame();
				BattleUnit* unit = _save->getTile(action.target)->getUnit();

				game->pushState(new InfoboxState(game,
						game->getLanguage()->getString("STR_IS_UNDER_ALIEN_CONTROL", unit->getGender())
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

		if (_save->selectNextPlayerUnit(true, _AISecondMove) == 0)
		{
			if (!_save->getDebugMode())
			{
				_endTurnRequested = true;
				//Log(LOG_INFO) << "BattlescapeGame::handleAI() statePushBack(end AI turn) 2";
				statePushBack(0); // end AI turn
			}
			else
			{
				_save->selectNextPlayerUnit();
				_debugPlay = true;
			}
		}

		if (_save->getSelectedUnit())
		{
			getMap()->getCamera()->centerOnPosition(_save->getSelectedUnit()->getPosition());

			if (_save->getSelectedUnit()->getId() <= unit->getId())
			{
				_AISecondMove = true;
			}
		}
	}

	//Log(LOG_INFO) << "BattlescapeGame::handleAI() EXIT";
}

/**
 * Toggles the Kneel/Standup status of the unit.
 * @param bu Pointer to a unit.
 * @return If the action succeeded.
 */
bool BattlescapeGame::kneel(BattleUnit* bu)
{
	//Log(LOG_INFO) << "BattlescapeGame::kneel()";

	int tu = bu->isKneeled() ? 8 : 4;

	if (bu->getType() == "SOLDIER"
		&& !bu->isFloating()		// kL_note: This prevents flying soldiers from 'kneeling' .....
		&& checkReservedTU(bu, tu))
	{
		if (bu->spendTimeUnits(tu))
		{
			bu->kneel(!bu->isKneeled());
			// kneeling or standing up can reveal new terrain or units. I guess.
			getTileEngine()->calculateFOV(bu);
			getMap()->cacheUnits();
			_parentState->updateSoldierInfo();
			getTileEngine()->checkReactionFire(bu);

			return true;
		}
		else
		{
			_parentState->warning("STR_NOT_ENOUGH_TIME_UNITS");
		}
	}

	return false;
}

/**
 * Ends the turn.
 */
void BattlescapeGame::endTurn()
{
	Log(LOG_INFO) << "BattlescapeGame::endTurn()";

	Position p;

	_tuReserved = _playerTUReserved;
	_debugPlay = false;
	_currentAction.type = BA_NONE;
	getMap()->getWaypoints()->clear();
	_currentAction.waypoints.clear();
	_parentState->showLaunchButton(false);
	_currentAction.targeting = false;
	_AISecondMove = false;

	if (_save->getTileEngine()->closeUfoDoors())
	{
		getResourcePack()->getSound("BATTLE.CAT", 21)->play(); // ufo door closed
	}

	for (int i = 0; i < _save->getMapSizeXYZ(); ++i) // check for hot grenades on the ground
	{
		for (std::vector<BattleItem*>::iterator it = _save->getTiles()[i]->getInventory()->begin(); it != _save->getTiles()[i]->getInventory()->end(); )
		{
			if ((*it)->getRules()->getBattleType() == BT_GRENADE
				&& (*it)->getExplodeTurn() == 0) // it's a grenade to explode now
			{
				p.x = _save->getTiles()[i]->getPosition().x * 16 + 8;
				p.y = _save->getTiles()[i]->getPosition().y * 16 + 8;
				p.z = _save->getTiles()[i]->getPosition().z * 24 - _save->getTiles()[i]->getTerrainLevel();

				statePushNext(new ExplosionBState(this, p, *it, (*it)->getPreviousOwner()));
				_save->removeItem(*it);

				statePushBack(0);

				//Log(LOG_INFO) << "BattlescapeGame::endTurn(), hot grenades EXIT";
				return;
			}

			++it;
		}
	}
	//Log(LOG_INFO) << ". done grenades";

	// check for terrain explosions
	Tile* t = _save->getTileEngine()->checkForTerrainExplosions();
	if (t)
	{
		Position p = Position(t->getPosition().x * 16, t->getPosition().y * 16, t->getPosition().z * 24);
		statePushNext(new ExplosionBState(this, p, 0, 0, t));

		t = _save->getTileEngine()->checkForTerrainExplosions();

		statePushBack(0);

		//Log(LOG_INFO) << "BattlescapeGame::endTurn(), terrain explosions EXIT";
		return;
	}
	//Log(LOG_INFO) << ". done explosions";

	if (_save->getSide() != FACTION_NEUTRAL)
	{
		for (std::vector<BattleItem*>::iterator it = _save->getItems()->begin(); it != _save->getItems()->end(); ++it)
		{
			if (((*it)->getRules()->getBattleType() == BT_GRENADE
					|| (*it)->getRules()->getBattleType() == BT_PROXIMITYGRENADE)
				&& (*it)->getExplodeTurn() > 0)
			{
				(*it)->setExplodeTurn((*it)->getExplodeTurn() - 1);
			}
		}
	}
	//Log(LOG_INFO) << ". done !neutral";

	// kL_begin:
	/* kL. from Savegame/BattleUnit.cpp, void BattleUnit::prepareNewTurn()
	// kL_note: fire damage is also caused by TileEngine::explode()
	if (!_hitByFire && _fire > 0) // suffer from fire
	{
		_health -= _armor->getDamageModifier(DT_IN) * RNG::generate(5, 10);
		_fire--;
	} */
	for (std::vector<BattleUnit*>::iterator j = _save->getUnits()->begin(); j != _save->getUnits()->end(); ++j)
	{
		if (_save->getSide() == (*j)->getFaction())
		{
			if (!(*j)->tookFireDamage()
				&& (*j)->getFire() > 0)
			{
				//Log(LOG_INFO) << ". do Turn Fire : " << (*j)->getId();

				(*j)->setFire(-1);
				(*j)->setHealth((*j)->getHealth() - (int)(*j)->getArmor()->getDamageModifier(DT_IN) * RNG::generate(4, 11));

				if ((*j)->getHealth() < 0)
				{
					(*j)->setHealth(0);
				}
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
	tallyUnits(liveAliens, liveSoldiers, true);
	//Log(LOG_INFO) << ". done tallyUnits";

	_save->endTurn();
	//Log(LOG_INFO) << ". done save->endTurn";

	if (_save->getSide() == FACTION_PLAYER)
	{
		setupCursor();
	}
	else
	{
		getMap()->setCursorType(CT_NONE);
	}

	checkForCasualties(0, 0, false, false);
	//Log(LOG_INFO) << ". done checkForCasualties";

	// turn off MCed alien lighting.
	_save->getTileEngine()->calculateUnitLighting();

	if (_save->isObjectiveDestroyed())
	{
		_parentState->finishBattle(false, liveSoldiers);

		//Log(LOG_INFO) << "BattlescapeGame::endTurn(), objectiveDestroyed EXIT";
		return;
	}

	if (liveAliens > 0 && liveSoldiers > 0)
	{
		showInfoBoxQueue();

		_parentState->updateSoldierInfo();

		if (playableUnitSelected())
		{
			getMap()->getCamera()->centerOnPosition(_save->getSelectedUnit()->getPosition());
			setupCursor();
		}
	}
	//Log(LOG_INFO) << ". done updates";

	if (_save->getSide() != FACTION_NEUTRAL
		&& _endTurnRequested)
	{
		//Log(LOG_INFO) << ". . pushState(nextTurnState)";
		_parentState->getGame()->pushState(new NextTurnState(_parentState->getGame(), _save, _parentState));
	}

	_endTurnRequested = false;
	Log(LOG_INFO) << "BattlescapeGame::endTurn() EXIT";
}

/**
 * Checks for casualties and adjusts morale accordingly.
 * @param murderweapon, Need to know this, for a HE explosion there is an instant death.
 * @param killer, This is needed for credits for the kill.
 * @param hiddenExplosion, Set to true for the explosions of UFO Power sources at start of battlescape.
 * @param terrainExplosion, Set to true for the explosions of terrain.
 */
void BattlescapeGame::checkForCasualties(BattleItem* murderweapon, BattleUnit* killer, bool hiddenExplosion, bool terrainExplosion)
{
	Log(LOG_INFO) << "BattlescapeGame::checkForCasualties()";

	for (std::vector<BattleUnit*>::iterator
			x = _save->getUnits()->begin();
			x != _save->getUnits()->end();
			++x)
	{
		if ((*x)->getHealth() == 0
			&& (*x)->getStatus() != STATUS_DEAD
			&& (*x)->getStatus() != STATUS_COLLAPSING)
		{
			BattleUnit* victim = *x;
			Log(LOG_INFO) << ". victim = " << (*x)->getId();

			if (killer)
			{
				Log(LOG_INFO) << ". killer = " << killer->getId();

				killer->addKillCount();
				victim->killedBy(killer->getFaction());

				int bonus = 100;
				if (killer->getFaction() == FACTION_PLAYER)
				{
					bonus = _save->getMoraleModifier();
				}
				else if (killer->getFaction() == FACTION_HOSTILE)
				{
					bonus = _save->getMoraleModifier(0, false);
				}

				// killer's boost
				if ((victim->getOriginalFaction() == FACTION_PLAYER
						&& killer->getOriginalFaction() == FACTION_HOSTILE)
					|| (victim->getOriginalFaction() == FACTION_HOSTILE
						&& killer->getOriginalFaction() == FACTION_PLAYER))
				{
					int boost = 10 * bonus / 100;
					killer->moraleChange(boost); // double what rest of squad gets below

					Log(LOG_INFO) << ". . killer boost + " << boost;
				}

				// killer (mc'd or not) will get a penalty with friendly fire (mc'd or not)
				// ... except aLiens, who don't care.
				if (victim->getOriginalFaction() == FACTION_PLAYER
					&& killer->getOriginalFaction() == FACTION_PLAYER)
				{
					int hit = 5000 / bonus;
					killer->moraleChange(-hit); // huge hit!

					Log(LOG_INFO) << ". . FF hit, killer - " << hit;
				}

				if (victim->getOriginalFaction() == FACTION_NEUTRAL) // civilian kills
				{
					if (killer->getOriginalFaction() == FACTION_PLAYER)
					{
						int civdeath = 2000 / bonus;
						killer->moraleChange(-civdeath);

						Log(LOG_INFO) << ". . civdeath by xCom, soldier - " << civdeath;
					}
					else if (killer->getOriginalFaction() == FACTION_HOSTILE)
					{
						killer->moraleChange(20); // no leadership bonus for aLiens executing civies: it's their job.

						Log(LOG_INFO) << ". . civdeath by aLien, killer + " << 20;
					}
				}
			}

			// cycle through units and do all faction
			if (victim->getFaction() != FACTION_NEUTRAL)
			{
				int solo = _save->getMoraleModifier(victim); // penalty for losing a unit on your side

				// these two are faction bonuses ('losers' mitigates the loss of solo, 'winners' boosts solo)
				int losers = 100, winners = 100;
				if (victim->getFaction() == FACTION_HOSTILE)
				{
					losers = _save->getMoraleModifier(0, false);
					winners = _save->getMoraleModifier();
				}
				else
				{
					losers = _save->getMoraleModifier();
					winners = _save->getMoraleModifier(0, false);
				}

				for (std::vector<BattleUnit*>::iterator
						i = _save->getUnits()->begin();
						i != _save->getUnits()->end();
						++i)
				{
					if (!(*i)->isOut(true)
						&& (*i)->getArmor()->getSize() == 1) // not a large unit
					{
						if ((*i)->getOriginalFaction() == victim->getOriginalFaction())
						{
							// losing faction get a morale loss
							int bravery = (110 - (*i)->getStats()->bravery) / 10;
							bravery = solo * 200 * bravery / losers / 100;
							(*i)->moraleChange(-bravery);

							Log(LOG_INFO) << ". . . loser - " << bravery;

							if (killer
								&& killer->getFaction() == FACTION_PLAYER
								&& victim->getFaction() == FACTION_HOSTILE)
							{
								killer->setTurnsExposed(0); // interesting

								Log(LOG_INFO) << ". . . . killer Exposed";
							}
						}
						// prevent mind-controlled units from receiving benefits.
						else if ((*i)->getOriginalFaction() != victim->getOriginalFaction())
						{
							// the winning squad all get a morale increase
							int boost = 10 * winners / 100;
							(*i)->moraleChange(boost);

							Log(LOG_INFO) << ". . . winner + " << boost;
						}
					}
				}
			}

			if (murderweapon) // kL_note: This is where units get sent to DEATH!
			{
				statePushNext(new UnitDieBState(this, *x, murderweapon->getRules()->getDamageType(), false));
			}
			else
			{
				if (hiddenExplosion) // this is instant death from UFO powersources, without screaming sounds
				{
					statePushNext(new UnitDieBState(this, *x, DT_HE, true));
				}
				else
				{
					if (terrainExplosion)
					{
						statePushNext(new UnitDieBState(this, *x, DT_HE, false));
					}
					else // no killer, and no terrain explosion, must be fatal wounds
					{
						statePushNext(new UnitDieBState(this, *x, DT_NONE, false));  // DT_NONE = STR_HAS_DIED_FROM_A_FATAL_WOUND
					}
				}
			}
		}
		else if ((*x)->getStunlevel() >= (*x)->getHealth()
			&& (*x)->getStatus() != STATUS_DEAD
			&& (*x)->getStatus() != STATUS_UNCONSCIOUS
			&& (*x)->getStatus() != STATUS_COLLAPSING
			&& (*x)->getStatus() != STATUS_TURNING)
		{
			statePushNext(new UnitDieBState(this, *x, DT_STUN, true)); // kL_note: This is where units get set to STUNNED
		}
	}

	BattleUnit* bu = _save->getSelectedUnit();

	if (_save->getSide() == FACTION_PLAYER)
	{
		_parentState->showPsiButton(bu && bu->getOriginalFaction() == FACTION_HOSTILE && bu->getStats()->psiSkill > 0 && !bu->isOut());
	}

	Log(LOG_INFO) << "BattlescapeGame::checkForCasualties() EXIT";
}

/**
 * Shows the infoboxes in the queue (if any).
 */
void BattlescapeGame::showInfoBoxQueue()
{
	for (std::vector<InfoboxOKState*>::iterator i = _infoboxQueue.begin(); i != _infoboxQueue.end(); ++i)
	{
		_parentState->getGame()->pushState(*i);
	}

	_infoboxQueue.clear();
}

/**
 * Handles the result of non target actions, like priming a grenade.
 */
void BattlescapeGame::handleNonTargetAction()
{
	if (!_currentAction.targeting)
	{
		if (_currentAction.type == BA_PRIME
			&& _currentAction.value > -1)
		{
			if (_currentAction.actor->spendTimeUnits(_currentAction.TU))
			{
				_currentAction.weapon->setExplodeTurn(_currentAction.value);
				_parentState->warning("STR_GRENADE_IS_ACTIVATED");
			}
			else
			{
				_parentState->warning("STR_NOT_ENOUGH_TIME_UNITS");
			}
		}

		if (_currentAction.type == BA_USE
			|| _currentAction.type == BA_LAUNCH)
		{
			if (_currentAction.result.length() > 0)
			{
				_parentState->warning(_currentAction.result);
				_currentAction.result = "";
			}

			_save->reviveUnconsciousUnits();
		}

		if (_currentAction.type == BA_HIT)
		{
			if (_currentAction.result.length() > 0)
			{
				_parentState->warning(_currentAction.result);
				_currentAction.result = "";
			}
			else
			{
				if (_currentAction.actor->spendTimeUnits(_currentAction.TU))
				{
					Position p;
					Pathfinding::directionToVector(_currentAction.actor->getDirection(), &p);
					Tile* tile (_save->getTile(_currentAction.actor->getPosition() + p));

					for (int x = 0; x != _currentAction.actor->getArmor()->getSize(); ++x)
					{
						for (int y = 0; y != _currentAction.actor->getArmor()->getSize(); ++y)
						{
							tile = _save->getTile(Position(_currentAction.actor->getPosition().x + x,
									_currentAction.actor->getPosition().y + y,
									_currentAction.actor->getPosition().z) + p);
							if (tile->getUnit()
								&& tile->getUnit() != _currentAction.actor)
							{
								Position voxel = Position(tile->getPosition().x * 16,tile->getPosition().y * 16,tile->getPosition().z * 24);
								voxel.x += 8;voxel.y += 8;voxel.z += 8;

								statePushNext(new ExplosionBState(this, voxel, _currentAction.weapon, _currentAction.actor));

								break;
							}
						}

						if (tile->getUnit()
							&& tile->getUnit() != _currentAction.actor)
						{
							break;
						}
					}
				}
				else
				{
					_parentState->warning("STR_NOT_ENOUGH_TIME_UNITS");
				}
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
	if (_currentAction.targeting)
	{
		if (_currentAction.type == BA_THROW)
		{
			getMap()->setCursorType(CT_THROW);
		}
		else if (_currentAction.type == BA_MINDCONTROL
			|| _currentAction.type == BA_PANIC
			|| _currentAction.type == BA_USE)
		{
			getMap()->setCursorType(CT_PSI);
		}
		else if (_currentAction.type == BA_LAUNCH)
		{
			getMap()->setCursorType(CT_WAYPOINT);
		}
		else
		{
			getMap()->setCursorType(CT_AIM);
		}
	}
	else
	{
		_currentAction.actor = _save->getSelectedUnit();
		if (_currentAction.actor)
		{
			getMap()->setCursorType(CT_NORMAL, _currentAction.actor->getArmor()->getSize());
		}
	}
}

/**
 * Determines whether a playable unit is selected. Normally only player side
 * units can be selected, but in debug mode one can play with aliens too :)
 * Is used to see if stats can be displayed and action buttons will work.
 * @return Whether a playable unit is selected.
 */
bool BattlescapeGame::playableUnitSelected()
{
	return _save->getSelectedUnit() != 0 && (_save->getSide() == FACTION_PLAYER || _save->getDebugMode());
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
		{
			_states.front()->think();
		}

		getMap()->draw();			// kL, old code!! -> Map::draw()
//kL		getMap()->invalidate(); // redraw map
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
	{
		_states.insert(++_states.begin(), bs);
	}
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
		{
			bs->init();
		}
	}
	else
	{
		_states.push_back(bs);
	}
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
	Log(LOG_INFO) << "BattlescapeGame::popState()";

	if (_save->getTraceSetting())
	{
		Log(LOG_INFO) << "BattlescapeGame::popState() #" << _AIActionCounter << " with "
			<< (_save->getSelectedUnit() ? _save->getSelectedUnit()->getTimeUnits() : -9999) << " TU";
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
					{
						cancelCurrentAction(true);
					}
				break;
			}
		} // kL_end.
	}

	//Log(LOG_INFO) << ". states.Popfront";
	_states.pop_front();
	//Log(LOG_INFO) << ". states.Popfront DONE";

	// handle the end of this unit's actions
	if (action.actor
		&& noActionsPending(action.actor))
	{
		//Log(LOG_INFO) << ". noActionsPending";

		if (action.actor->getFaction() == FACTION_PLAYER)
		{
			//Log(LOG_INFO) << ". actor -> Faction_Player";

			// spend TUs of "target triggered actions" (shooting, throwing) only
			// the other actions' TUs (healing,scanning,..) are already take care of
			if (action.targeting
				&& _save->getSelectedUnit()
				&& !actionFailed)
			{
				action.actor->spendTimeUnits(action.TU);
			}

			if (_save->getSide() == FACTION_PLAYER)
			{
				//Log(LOG_INFO) << ". side -> Faction_Player";

				// after throwing, the cursor returns to default cursor, after shooting it stays in
				// targeting mode and the player can shoot again in the same mode (autoshot/snap/aimed)
				// kL_note: unless he/she is out of tu's

/*kL				if ((action.type == BA_THROW || action.type == BA_LAUNCH) && !actionFailed)
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

							if (curTU < action.actor->getActionTUs(BA_SNAPSHOT, action.weapon))
							{
								cancelCurrentAction(true);
							}
						break;
						case BA_AIMEDSHOT:
							//Log(LOG_INFO) << ". AimedShot, TU percent = " << (float)action.weapon->getRules()->getTUAimed();

							if (curTU < action.actor->getActionTUs(BA_AIMEDSHOT, action.weapon))
							{
								cancelCurrentAction(true);
							}
						break;
						case BA_AUTOSHOT:
							//Log(LOG_INFO) << ". AutoShot, TU percent = " << (float)action.weapon->getRules()->getTUAuto();

							if (curTU < action.actor->getActionTUs(BA_AUTOSHOT, action.weapon))
							{
								cancelCurrentAction(true);
							}
						break;
						case BA_PANIC:
							if (curTU < action.actor->getActionTUs(BA_PANIC, action.weapon))
							{
								cancelCurrentAction(true);
							}
						break;
						case BA_MINDCONTROL:
							if (curTU < action.actor->getActionTUs(BA_MINDCONTROL, action.weapon))
							{
								cancelCurrentAction(true);
							}
						break;
/*						case BA_USE:
							if (action.weapon->getRules()->getBattleType() == BT_MINDPROBE
								&& curTU < action.actor->getActionTUs(BA_MINDCONTROL, action.weapon))
							{
								cancelCurrentAction(true);
							}
						break; */
					}
				} // kL_end.

				_parentState->getGame()->getCursor()->setVisible(true);
				setupCursor();

				//Log(LOG_INFO) << ". end NOT actionFailed";
			}
		}
		else
		{
			//Log(LOG_INFO) << ". action -> NOT Faction_Player";

			action.actor->spendTimeUnits(action.TU); // spend TUs

			if (_save->getSide() != FACTION_PLAYER && !_debugPlay)
			{
				 // AI does three things per unit, before switching to the next, or it got killed before doing the second thing
				if (_AIActionCounter > 2
					|| _save->getSelectedUnit() == 0
					|| _save->getSelectedUnit()->isOut())
				{
					if (_save->getSelectedUnit())
					{
						_save->getSelectedUnit()->setCache(0);
						getMap()->cacheUnit(_save->getSelectedUnit());
					}

					_AIActionCounter = 0;

					if (_states.empty()
						&& _save->selectNextPlayerUnit(true) == 0)
					{
						if (!_save->getDebugMode())
						{
							_endTurnRequested = true;
							statePushBack(0); // end AI turn
						}
						else
						{
							_save->selectNextPlayerUnit();
							_debugPlay = true;
						}
					}

					if (_save->getSelectedUnit())
					{
						getMap()->getCamera()->centerOnPosition(_save->getSelectedUnit()->getPosition());
					}
				}
			}
			else if (_debugPlay)
			{
				_parentState->getGame()->getCursor()->setVisible(true);
				setupCursor();
			}
		}
	}

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
				Log(LOG_INFO) << ". endTurn()";
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
	if (_save->getSelectedUnit() == 0
		|| _save->getSelectedUnit()->isOut())
	{
		//Log(LOG_INFO) << ". huh hey wot)";

		cancelCurrentAction();

		_save->setSelectedUnit(0);

		getMap()->setCursorType(CT_NORMAL, 1);
		_parentState->getGame()->getCursor()->setVisible(true);
	}

	Log(LOG_INFO) << ". updateSoldierInfo()";
	_parentState->updateSoldierInfo();

	//Log(LOG_INFO) << "BattlescapeGame::popState() EXIT";
}

/**
 * Determines whether there are any actions pending for the given unit.
 * @param bu BattleUnit.
 * @return True if there are no actions pending.
 */
bool BattlescapeGame::noActionsPending(BattleUnit* bu)
{
	if (_states.empty()) return true;

	for (std::list<BattleState*>::iterator i = _states.begin(); i != _states.end(); ++i)
	{
		if (*i != 0
			&& (*i)->getAction().actor == bu)
		{
			return false;
		}
	}

	return true;
}

/**
 * Sets the timer interval for think() calls of the state.
 * @param interval An interval in ms.
 */
void BattlescapeGame::setStateInterval(Uint32 interval)
{
	_parentState->setStateInterval(interval);
}

/**
 * Checks against reserved time units.
 * @param bu, Pointer to the unit.
 * @param tu, Number of time units to check.
 * @return bool, Whether or not *bu has enough time units.
 */
bool BattlescapeGame::checkReservedTU(BattleUnit* bu, int tu, bool justChecking)
{
    BattleActionType effectiveTuReserved = _tuReserved; // avoid changing _tuReserved in this method

	if (_save->getSide() != FACTION_PLAYER) // aliens reserve TUs as a percentage rather than just enough for a single action.
	{
		if (_save->getSide() == FACTION_NEUTRAL)
		{
			return tu < bu->getTimeUnits();
		}

		switch (effectiveTuReserved)
		{
			case BA_SNAPSHOT:
				return tu + (bu->getStats()->tu / 3) < bu->getTimeUnits();			// 33%
			break;
			case BA_AUTOSHOT:
				return tu + ((bu->getStats()->tu / 5) * 2) < bu->getTimeUnits();	// 40%
			break;
			case BA_AIMEDSHOT:
				return tu + (bu->getStats()->tu / 2) < bu->getTimeUnits();			// 50%
			break;

			default:
				return tu < bu->getTimeUnits();
			break;
		}
	}

	// check TUs against slowest weapon if we have two weapons
	BattleItem* slowestWeapon = bu->getMainHandWeapon(false);
	// kL_note: Use getActiveHand() instead, if xCom wants to reserve TU.
	// kL_note: make sure this doesn't work on aLiens, because getMainHandWeapon()
	// returns grenades and that can easily cause problems. Probably could cause
	// problems for xCom too, if xCom wants to reserve TU's in this manner.
	// note: won't return grenades anymore.

	// if the weapon has no autoshot, reserve TUs for snapshot
	if (bu->getActionTUs(_tuReserved, slowestWeapon) == 0
		&& _tuReserved == BA_AUTOSHOT)
	{
		effectiveTuReserved = BA_SNAPSHOT;
	}

	const int tuKneel = _kneelReserved? 4:0;
	if ((effectiveTuReserved != BA_NONE || _kneelReserved)
		&& tu + tuKneel + bu->getActionTUs(effectiveTuReserved, slowestWeapon) > bu->getTimeUnits()
		&& tuKneel + bu->getActionTUs(effectiveTuReserved, slowestWeapon) <= bu->getTimeUnits())
	{
		if (!justChecking)
		{
			if (_kneelReserved)
			{
				switch (effectiveTuReserved)
				{
					case BA_NONE:
						_parentState->warning("STR_TIME_UNITS_RESERVED_FOR_KNEELING");
					break;

					default:
						_parentState->warning("STR_TUS_RESERVED_FOR_KNEELING_AND_FIRING");
					break;
				}
			}
			else
			{
				switch (effectiveTuReserved)
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
	for (std::vector<BattleUnit*>::iterator j = _save->getUnits()->begin(); j != _save->getUnits()->end(); ++j)
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
 * @return False when unit not in panicking mode.
 */
bool BattlescapeGame::handlePanickingUnit(BattleUnit* unit)
{
	UnitStatus status = unit->getStatus();

	if (status != STATUS_PANICKING && status != STATUS_BERSERK)
		return false;

	//Log(LOG_INFO) << "unit Panic/Berserk : " << unit->getId() << " / " << unit->getMorale();		// kL

	unit->setVisible(true);
	getMap()->getCamera()->centerOnPosition(unit->getPosition());
	_save->setSelectedUnit(unit);

	// show a little infobox with the name of the unit and "... is panicking"
	Game* game = _parentState->getGame();
	if (status == STATUS_PANICKING)
	{
		game->pushState(new InfoboxState(game,
				game->getLanguage()->getString("STR_HAS_PANICKED", unit->getGender())
				.arg(unit->getName(game->getLanguage()))));
	}
	else
	{
		game->pushState(new InfoboxState(game,
				game->getLanguage()->getString("STR_HAS_GONE_BERSERK", unit->getGender())
				.arg(unit->getName(game->getLanguage()))));
	}

	unit->abortTurn(); // makes the unit go to status STANDING :p

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
				{
					dropItem(unit->getPosition(), item, false, true);
				}

				item = unit->getItem("STR_LEFT_HAND");
				if (item)
				{
					dropItem(unit->getPosition(), item, false, true);
				}

				unit->setCache(0);

				ba.target = Position(unit->getPosition().x + RNG::generate(-5, 5), unit->getPosition().y + RNG::generate(-5, 5), unit->getPosition().z);
				if (_save->getTile(ba.target)) // only walk towards it when the place exists
				{
					_save->getPathfinding()->calculate(ba.actor, ba.target);

					statePushBack(new UnitWalkBState(this, ba));
				}
			}
		break;
		case STATUS_BERSERK: // berserk - do some weird turning around and then aggro towards an enemy unit or shoot towards random place
			ba.type = BA_TURN;
			for (int i= 0; i < 4; i++)
			{
				ba.target = Position(unit->getPosition().x + RNG::generate(-5,5), unit->getPosition().y + RNG::generate(-5,5), unit->getPosition().z);
				statePushBack(new UnitTurnBState(this, ba));
			}

			for (std::vector<BattleUnit*>::iterator j = unit->getVisibleUnits()->begin(); j != unit->getVisibleUnits()->end(); ++j)
			{
				ba.target = (*j)->getPosition();
				statePushBack(new UnitTurnBState(this, ba));
			}

			if (_save->getTile(ba.target) != 0)
			{
				ba.weapon = unit->getMainHandWeapon();
				if (ba.weapon)
				{
					if (ba.weapon->getRules()->getBattleType() == BT_FIREARM)
					{
						ba.type = BA_SNAPSHOT;
						int tu = ba.actor->getActionTUs(ba.type, ba.weapon);

						for (int i = 0; i < 10; i++) // fire shots until unit runs out of TUs
						{
							if (!ba.actor->spendTimeUnits(tu))
								break;

							statePushBack(new ProjectileFlyBState(this, ba));
						}
					}
					else if (ba.weapon->getRules()->getBattleType() == BT_GRENADE)
					{
						if (ba.weapon->getExplodeTurn() == -1)
						{
							ba.weapon->setExplodeTurn(0);
						}

						ba.type = BA_THROW;
						statePushBack(new ProjectileFlyBState(this, ba));
					}
				}
			}

			unit->setTimeUnits(unit->getStats()->tu); // replace the TUs from shooting
			ba.type = BA_NONE;
		break;

		default:
		break;
	}

	// Time units can only be reset after everything else occurs
	statePushBack(new UnitPanicBState(this, ba.actor));

	unit->moraleChange(+15);

	//Log(LOG_INFO) << "unit Panic/Berserk : " << unit->getId() << " / " << unit->getMorale();		// kL
	return true;
}

/**
  * Cancels the current action the user had selected (firing, throwing,..)
  * @param bForce, Force the action to be cancelled.
  * @return, Whether an action was cancelled or not.
  */
bool BattlescapeGame::cancelCurrentAction(bool bForce)
{
	//Log(LOG_INFO) << "BattlescapeGame::cancelCurrentAction()";

	bool bPreviewed = Options::getInt("battleNewPreviewPath") > 0;

	if (_save->getPathfinding()->removePreview() && bPreviewed)
		return true;

	if (_states.empty() || bForce)
	{
		if (_currentAction.targeting)
		{
			if (_currentAction.type == BA_LAUNCH
				&& !_currentAction.waypoints.empty())
			{
				_currentAction.waypoints.pop_back();
				if (!getMap()->getWaypoints()->empty())
				{
					getMap()->getWaypoints()->pop_back();
				}

				if (_currentAction.waypoints.empty())
				{
					_parentState->showLaunchButton(false);
				}

				return true;
			}
			else
			{
				_currentAction.targeting = false;
				_currentAction.type = BA_NONE;

				setupCursor();
				_parentState->getGame()->getCursor()->setVisible(true);

				return true;
			}
		}
	}
	else if (!_states.empty() && _states.front() != 0)
	{
		_states.front()->cancel();

		return true;
	}

	return false;
}

/**
 * Gets a pointer to access action members directly.
 * @return Pointer to action.
 */
BattleAction* BattlescapeGame::getCurrentAction()
{
	return &_currentAction;
}

/**
 * Determines whether an action is currently going on?
 * @return true or false.
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
/*	std::string sUnit = "none selected";			// kL
	if (_save->getSelectedUnit())					// kL
	{
		sUnit = _save->getSelectedUnit()->getId();	// kL
	} */

	Log(LOG_INFO) << "BattlescapeGame::primaryAction()"; // unitID = " << _currentAction.actor->getId();

	bool bPreviewed = Options::getInt("battleNewPreviewPath") > 0;

	if (_currentAction.targeting
		&& _save->getSelectedUnit())
	{
		//Log(LOG_INFO) << ". . _currentAction.targeting";
		_currentAction.strafe = false;		// kL

		if (_currentAction.type == BA_LAUNCH)
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
				&& _save->selectUnit(pos)->getFaction() != _save->getSelectedUnit()->getFaction())
			{
				if (_currentAction.actor->spendTimeUnits(_currentAction.TU))
				{
					_parentState->getGame()->getResourcePack()->getSound("BATTLE.CAT", _currentAction.weapon->getRules()->getHitSound())->play();
					_parentState->getGame()->pushState(new UnitInfoState(_parentState->getGame(), _save->selectUnit(pos), _parentState));

					cancelCurrentAction();
				}
				else
				{
					cancelCurrentAction();		// kL
					_parentState->warning("STR_NOT_ENOUGH_TIME_UNITS");
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

				_currentAction.TU = _currentAction.weapon->getRules()->getTUUse();
				_currentAction.target = pos;

				// get the sound/animation started
				_currentAction.cameraPosition = getMap()->getCamera()->getMapOffset();
				statePushBack(new ProjectileFlyBState(this, _currentAction));

				if (_currentAction.TU <= _currentAction.actor->getTimeUnits())
				{
					if (getTileEngine()->psiAttack(&_currentAction))
					{
						//Log(LOG_INFO) << ". . . . . . Psi successful";

						// show a little infobox if it's successful
						Game* game = _parentState->getGame();
						if (_currentAction.type == BA_PANIC)
						{
							//Log(LOG_INFO) << ". . . . . . . . BA_Panic";

							BattleUnit* unit = _save->getTile(_currentAction.target)->getUnit();
							game->pushState(new InfoboxState(game,
									game->getLanguage()->getString("STR_HAS_PANICKED", unit->getGender())
									.arg(unit->getName(game->getLanguage()))));
						}
						else if (_currentAction.type == BA_MINDCONTROL)
						{
							//Log(LOG_INFO) << ". . . . . . . . BA_MindControl";

							game->pushState(new InfoboxState(game, game->getLanguage()->getString("STR_MIND_CONTROL_SUCCESSFUL")));
						}

						//Log(LOG_INFO) << ". . . . . . updateSoldierInfo()";
						_parentState->updateSoldierInfo();
						//Log(LOG_INFO) << ". . . . . . updateSoldierInfo() DONE";


						// kL_begin: BattlescapeGame::primaryAction(), not sure where this bit came from.....
						// it doesn't seem to be in the official oXc but it might
						// stop some (psi-related) crashes i'm getting;
						// but then it probably never runs because I doubt that selectedUnit can be other than xCom.
						if (_save->getSelectedUnit()->getFaction() != FACTION_PLAYER)
						{
							_currentAction.targeting = false;
							_currentAction.type = BA_NONE;
						}
//						setupCursor();



						// kL_note: might need to put a refresh (redraw/blit) cursor here;
						// else it 'sticks' for a moment at its previous position.
//						_parentState->getMap()->refreshSelectorPosition();			// kL

//						getMap()->setCursorType(CT_NONE);							// kL
						_parentState->getGame()->getCursor()->setVisible(false);	// kL
						// kL_end.

						//Log(LOG_INFO) << ". . . . . . inVisible cursor, DONE";
					}

					if (builtinpsi)
					{
						_save->removeItem(_currentAction.weapon);
						_currentAction.weapon = 0;
					}
				}
			}
		}
		else
		{
			//Log(LOG_INFO) << ". . . . Firing or Throwing";

			getMap()->setCursorType(CT_NONE);
			_parentState->getGame()->getCursor()->setVisible(false);

			_currentAction.target = pos;
			_currentAction.cameraPosition = getMap()->getCamera()->getMapOffset();

			_states.push_back(new ProjectileFlyBState(this, _currentAction));

			statePushFront(new UnitTurnBState(this, _currentAction)); // first of all turn towards the target
		}
	}
	else
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

			if (_currentAction.target != pos && bPreviewed)
				_save->getPathfinding()->removePreview();

			_currentAction.run = false;
			_currentAction.strafe = _save->getStrafeSetting()
					&& (SDL_GetModState() & KMOD_CTRL) != 0
					&& _save->getSelectedUnit()->getTurretType() == -1;

			if (_currentAction.strafe
				&& _save->getTileEngine()->distance(_currentAction.actor->getPosition(), pos) > 1)
			{
				_currentAction.run = true;
				_currentAction.strafe = false;
			}

			_currentAction.target = pos;
			_save->getPathfinding()->calculate(_currentAction.actor, _currentAction.target);

			if (bPreviewed
				&& !_save->getPathfinding()->previewPath()
				&& _save->getPathfinding()->getStartDirection() != -1)
			{
				_save->getPathfinding()->removePreview();

				bPreviewed = false;
			}

			if (!bPreviewed && _save->getPathfinding()->getStartDirection() != -1)
			{
				//  -= start walking =-
				getMap()->setCursorType(CT_NONE);
				_parentState->getGame()->getCursor()->setVisible(false);

				statePushBack(new UnitWalkBState(this, _currentAction));
			}
		}
	}

	Log(LOG_INFO) << "BattlescapeGame::primaryAction() EXIT";
}

/**
 * Activates secondary action (right click).
 * @param pos Position on the map.
 */
void BattlescapeGame::secondaryAction(const Position &pos)
{
	//Log(LOG_INFO) << "BattlescapeGame::secondaryAction()";

	//  -= turn to or open door =-
	_currentAction.target = pos;
	_currentAction.actor = _save->getSelectedUnit();
	_currentAction.strafe = _save->getStrafeSetting()
			&& (SDL_GetModState() & KMOD_CTRL) != 0
			&& _save->getSelectedUnit()->getTurretType() > -1;

	statePushBack(new UnitTurnBState(this, _currentAction));
}

/**
 * Handler for the blaster launcher button.
 */
void BattlescapeGame::launchAction()
{
	Log(LOG_INFO) << "BattlescapeGame::launchAction()";

	_parentState->showLaunchButton(false);

	getMap()->getWaypoints()->clear();
	_currentAction.target = _currentAction.waypoints.front();

	getMap()->setCursorType(CT_NONE);
	_parentState->getGame()->getCursor()->setVisible(false);

	_currentAction.cameraPosition = getMap()->getCamera()->getMapOffset();

	_states.push_back(new ProjectileFlyBState(this, _currentAction));

	statePushFront(new UnitTurnBState(this, _currentAction)); // first of all turn towards the target
}

/**
 * Handler for the psi button.
 */
void BattlescapeGame::psiButtonAction()
{
	Log(LOG_INFO) << "BattlescapeGame::psiButtonAction()";

	_currentAction.weapon = 0;
	_currentAction.targeting = true;
	_currentAction.type = BA_PANIC;
	_currentAction.TU = 25;

	setupCursor();
}


/**
 * Moves a unit up or down.
 * @param unit The unit.
 * @param dir Direction DIR_UP or DIR_DOWN.
 */
void BattlescapeGame::moveUpDown(BattleUnit* unit, int dir)
{
	Log(LOG_INFO) << "BattlescapeGame::moveUpDown()";

	_currentAction.target = unit->getPosition();
	if (dir == Pathfinding::DIR_UP)
		_currentAction.target.z++;
	else
		_currentAction.target.z--;

	getMap()->setCursorType(CT_NONE);
	_parentState->getGame()->getCursor()->setVisible(false);

	// kL_note: taking this out so I can go up/down *kneeled* on GravLifts. <- bork! not what i was looking for
	// might be a problem with soldiers in flying suits, later...
/*kL	if (_save->getSelectedUnit()->isKneeled())
//		&& not on GravLift)		// kL
	{
		Log(LOG_INFO) << "BattlescapeGame::moveUpDown()" ;	// kL

		kneel(_save->getSelectedUnit());
	} */

	_save->getPathfinding()->calculate(_currentAction.actor, _currentAction.target);

	statePushBack(new UnitWalkBState(this, _currentAction));
}

/**
 * Requests the end of the turn (waits for explosions etc to really end the turn).
 */
void BattlescapeGame::requestEndTurn()
{
	Log(LOG_INFO) << "BattlescapeGame::requestEndTurn()";

	cancelCurrentAction();

	if (!_endTurnRequested)
	{
		_endTurnRequested = true;

		statePushBack(0);
	}
}

/**
 * Sets the TU reserved type.
 * @param tur A battleactiontype.
 * @param player is this requested by the player?
 */
void BattlescapeGame::setTUReserved(BattleActionType tur, bool player)
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
void BattlescapeGame::dropItem(const Position& position, BattleItem* item, bool newItem, bool removeItem)
{
	Position p = position;

	// don't spawn anything outside of bounds
	if (_save->getTile(p) == 0)
		return;

	// don't ever drop fixed items
	if (item->getRules()->isFixed())
		return;

	_save->getTile(p)->addItem(item, getRuleset()->getInventory("STR_GROUND"));

	if (item->getUnit())
	{
		item->getUnit()->setPosition(p);
	}

	if (newItem)
	{
		_save->getItems()->push_back(item);
	}
	else if (_save->getSide() != FACTION_PLAYER)
	{
		item->setTurnFlag(true);
	}

	if (removeItem)
	{
		item->moveToOwner(0);
	}
	else if (item->getRules()->getBattleType() != BT_GRENADE
		&& item->getRules()->getBattleType() != BT_PROXIMITYGRENADE)
	{
		item->setOwner(0);
	}

	getTileEngine()->applyGravity(_save->getTile(p));

	if (item->getRules()->getBattleType() == BT_FLARE)
	{
		getTileEngine()->calculateTerrainLighting();
		getTileEngine()->calculateFOV(position);
	}
}

/**
 * Converts a unit into a unit of another type.
 * @param unit The unit to convert.
 * @param newType The type of unit to convert to.
 * @param Pointer to the new unit.
 */
BattleUnit* BattlescapeGame::convertUnit(BattleUnit* unit, std::string newType)
{
	getSave()->getBattleState()->showPsiButton(false);
	// in case the unit was unconscious
	getSave()->removeUnconsciousBodyItem(unit);

//	int origDir = unit->getDirection();		// kL
	unit->instaKill();

	if (Options::getBool("battleNotifyDeath")
		&& unit->getFaction() == FACTION_PLAYER
		&& unit->getOriginalFaction() == FACTION_PLAYER)
	{
		_parentState->getGame()->pushState(new InfoboxState(_parentState->getGame(),
				_parentState->getGame()->getLanguage()->getString("STR_HAS_BEEN_KILLED", unit->getGender())
				.arg(unit->getName(_parentState->getGame()->getLanguage()))));
	}

	for (std::vector<BattleItem*>::iterator i = unit->getInventory()->begin(); i != unit->getInventory()->end(); ++i)
	{
		dropItem(unit->getPosition(), (*i));
		(*i)->setOwner(0);
	}

	unit->getInventory()->clear();

	// remove unit-tile link
	unit->setTile(0);
	getSave()->getTile(unit->getPosition())->setUnit(0);

	std::ostringstream newArmor;
	newArmor << getRuleset()->getUnit(newType)->getArmor();
	std::string terroristWeapon = getRuleset()->getUnit(newType)->getRace().substr(4);
	terroristWeapon += "_WEAPON";
	RuleItem* newItem = getRuleset()->getItem(terroristWeapon);

	int difficulty = (int)(_parentState->getGame()->getSavedGame()->getDifficulty());
	BattleUnit* newUnit = new BattleUnit(getRuleset()->getUnit(newType),
			FACTION_HOSTILE,
			_save->getUnits()->back()->getId() + 1,
			getRuleset()->getArmor(newArmor.str()),
			difficulty);
	// kL_note: what about setting _zombieUnit=true ? It's not generic but it's the only case, afaict

	if (!difficulty)
	{
		newUnit->halveArmor();
	}

	getSave()->getTile(unit->getPosition())->setUnit(newUnit, _save->getTile(unit->getPosition() + Position(0, 0, -1)));
	newUnit->setPosition(unit->getPosition());
	newUnit->setDirection(3);
//	newUnit->setDirection(origDir);		// kL
//	this->getMap()->cacheUnit(newUnit);	// kL, try this. ... damn!
	newUnit->setCache(0);
	newUnit->setTimeUnits(0);

	getSave()->getUnits()->push_back(newUnit);
	getMap()->cacheUnit(newUnit);
	newUnit->setAIState(new AlienBAIState(getSave(), newUnit, 0));

	BattleItem* bi = new BattleItem(newItem, getSave()->getCurrentItemId());
	bi->moveToOwner(newUnit);
	bi->setSlot(getRuleset()->getInventory("STR_RIGHT_HAND"));
	getSave()->getItems()->push_back(bi);

	getTileEngine()->calculateFOV(newUnit->getPosition());
	getTileEngine()->applyGravity(newUnit->getTile());
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
	Log(LOG_INFO) << "BattlescapeGame::findItem()";

	if (action->actor->getRankString() != "STR_TERRORIST")								// terrorists don't have hands.
	{
		BattleItem *targetItem = surveyItems(action);									// pick the best available item
		if (targetItem && worthTaking(targetItem, action))								// make sure it's worth taking
		{
			if (targetItem->getTile()->getPosition() == action->actor->getPosition())	// if we're already standing on it...
			{
				if (takeItemFromGround(targetItem, action) == 0)						// try to pick it up
				{
					if (!targetItem->getAmmoItem())										// if it isn't loaded or it is ammo
					{
						action->actor->checkAmmo();										// try to load our weapon
					}
				}
			}
			else if (!targetItem->getTile()->getUnit()									// if we're not standing on it, we should try to get to it.
				|| targetItem->getTile()->getUnit()->isOut())
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
	Log(LOG_INFO) << "BattlescapeGame::surveyItems()";

	std::vector<BattleItem*> droppedItems;

	// first fill a vector with items on the ground that were dropped on the alien turn, and have an attraction value.
	for (std::vector<BattleItem*>::iterator i = _save->getItems()->begin(); i != _save->getItems()->end(); ++i)
	{
		if ((*i)->getSlot()
			&& (*i)->getSlot()->getId() == "STR_GROUND"
			&& (*i)->getTile()
			&& (*i)->getTurnFlag()
			&& (*i)->getRules()->getAttraction())
		{
			droppedItems.push_back(*i);
		}
	}

	BattleItem* targetItem = 0;
	int maxWorth = 0;

	// now select the most suitable candidate depending on attractiveness and distance
	// (are we still talking about items?)
	for (std::vector<BattleItem*>::iterator i = droppedItems.begin(); i != droppedItems.end(); ++i)
	{
		int currentWorth = (*i)->getRules()->getAttraction() / ((_save->getTileEngine()->distance(action->actor->getPosition(), (*i)->getTile()->getPosition()) * 2) + 1);
		if (currentWorth > maxWorth)
		{
			maxWorth = currentWorth;
			targetItem = *i;
		}
	}

	return targetItem;
}

/**
 * Assesses whether this item is worth trying to pick up, taking into account how many units we see,
 * whether or not the Weapon has ammo, and if we have ammo FOR it,
 * or, if it's ammo, checks if we have the weapon to go with it,
 * assesses the attraction value of the item and compares it with the distance to the object,
 * then returns false anyway.
 * @param item The item to attempt to take.
 * @param action A pointer to the action being performed.
 * @return false.
 */
bool BattlescapeGame::worthTaking(BattleItem* item, BattleAction *action)
{
	Log(LOG_INFO) << "BattlescapeGame::worthTaking()";

	int worthToTake = 0;

	// don't even think about making a move for that gun if you can see a target, for some reason
	// (maybe this should check for enemies spotting the tile the item is on?)
	if (action->actor->getVisibleUnits()->empty())
	{
		// retrieve an insignificantly low value from the ruleset.
		worthToTake = item->getRules()->getAttraction();

		// it's always going to be worth while to try and take a blaster launcher, apparently
		if (!item->getRules()->isWaypoint() && item->getRules()->getBattleType() != BT_AMMO)
		{
			// we only want weapons that HAVE ammo, or weapons that we have ammo FOR
			bool ammoFound = true;
			if (!item->getAmmoItem())
			{
				ammoFound = false;
				for (std::vector<BattleItem*>::iterator i = action->actor->getInventory()->begin(); i != action->actor->getInventory()->end() && !ammoFound; ++i)
				{
					if ((*i)->getRules()->getBattleType() == BT_AMMO)
					{
						for (std::vector<std::string>::iterator j = item->getRules()->getCompatibleAmmo()->begin(); j != item->getRules()->getCompatibleAmmo()->end() && !ammoFound; ++j)
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
			{
				return false;
			}
		}

		if (item->getRules()->getBattleType() == BT_AMMO)
		{
			// similar to the above, but this time we're checking if the ammo is suitable for a weapon we have.
			bool weaponFound = false;
			for (std::vector<BattleItem*>::iterator i = action->actor->getInventory()->begin(); i != action->actor->getInventory()->end() && !weaponFound; ++i)
			{
				if ((*i)->getRules()->getBattleType() == BT_FIREARM)
				{
					for (std::vector<std::string>::iterator j = (*i)->getRules()->getCompatibleAmmo()->begin(); j != (*i)->getRules()->getCompatibleAmmo()->end() && !weaponFound; ++j)
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
			{
				return false;
			}
		}
	}

    if (worthToTake)
    {
		// use bad logic to determine if we'll have room for the item
		int freeSlots = 25;

		for (std::vector<BattleItem*>::iterator i = action->actor->getInventory()->begin(); i != action->actor->getInventory()->end(); ++i)
		{
			freeSlots -= (*i)->getRules()->getInventoryHeight() * (*i)->getRules()->getInventoryWidth();
		}

		int size = item->getRules()->getInventoryHeight() * item->getRules()->getInventoryWidth();
		if (freeSlots < size)
		{
			return false;
		}
	}

	// return false for any item that we aren't standing directly on top of with an attraction value less than 6 (aka always)
	return (worthToTake - (_save->getTileEngine()->distance(action->actor->getPosition(), item->getTile()->getPosition())*2)) > 5;
}

/**
 * Picks the item up from the ground.
 *
 * At this point we've decided it's worth our while to grab this item, so we try to do just that.
 * First we check to make sure we have time units, then that we have space (using horrifying logic)
 * then we attempt to actually recover the item.
 * @param item The item to attempt to take.
 * @param action A pointer to the action being performed.
 * @return 0 if successful, 1 for no TUs, 2 for not enough room, 3 for "won't fit" and -1 for "something went horribly wrong".
 */
int BattlescapeGame::takeItemFromGround(BattleItem* item, BattleAction *action)
{
	Log(LOG_INFO) << "BattlescapeGame::takeItemFromGround()";

	const int unhandledError = -1;
	const int success = 0;
	const int notEnoughTimeUnits = 1;
	const int notEnoughSpace = 2;
	const int couldNotFit = 3;
	int freeSlots = 25;

	// make sure we have time units
	if (action->actor->getTimeUnits() < 6)
	{
		return notEnoughTimeUnits;
	}
	else
	{
		// check to make sure we have enough space by checking all the sizes of items in our inventory
		for (std::vector<BattleItem*>::iterator i = action->actor->getInventory()->begin(); i != action->actor->getInventory()->end(); ++i)
		{
			freeSlots -= (*i)->getRules()->getInventoryHeight() * (*i)->getRules()->getInventoryWidth();
		}

		if (freeSlots < item->getRules()->getInventoryHeight() * item->getRules()->getInventoryWidth())
		{
			return notEnoughSpace;
		}
		else
		{
			// check that the item will fit in our inventory, and if so, take it
			if (takeItem(item, action))
			{
				action->actor->spendTimeUnits(6);
				item->getTile()->removeItem(item);

				return success;
			}
			else
			{
				return couldNotFit;
			}
		}
	}

	// shouldn't ever end up here
	return unhandledError;
}

/**
 * Tries to fit an item into the unit's inventory, return false if you can't.
 * @param item The item to attempt to take.
 * @param action A pointer to the action being performed.
 * @return Whether or not the item was successfully retrieved.
 */
bool BattlescapeGame::takeItem(BattleItem* item, BattleAction* action)
{
	Log(LOG_INFO) << "BattlescapeGame::takeItem()";

	bool placed = false;
	Ruleset* rules = _parentState->getGame()->getRuleset();

	switch (item->getRules()->getBattleType())
	{
		case BT_AMMO:
			// find equipped weapons that can be loaded with this ammo
			if (action->actor->getItem("STR_RIGHT_HAND") && action->actor->getItem("STR_RIGHT_HAND")->getAmmoItem() == 0)
			{
				if (action->actor->getItem("STR_RIGHT_HAND")->setAmmoItem(item) == 0)
				{
					placed = true;
				}
			}
			else
			{
				for (int i = 0; i != 4; ++i)
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
			for (int i = 0; i != 4; ++i)
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
 * @return The type of action that is reserved.
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
void BattlescapeGame::tallyUnits(int& liveAliens, int& liveSoldiers, bool convert)
{
	Log(LOG_INFO) << "BattlescapeGame::tallyUnits()";

	liveSoldiers = 0;
	liveAliens = 0;
	bool psiCapture = Options::getBool("allowPsionicCapture");

	if (convert)
	{
		for (std::vector<BattleUnit*>::iterator
				j = _save->getUnits()->begin();
				j != _save->getUnits()->end();
				++j)
		{
//kL			if ((*j)->getHealth() > 0
			if ((*j)->getSpecialAbility() == SPECAB_RESPAWN)
			{
				//Log(LOG_INFO) << "BattlescapeGame::tallyUnits() " << (*j)->getId() << " : health > 0, SPECAB_RESPAWN -> converting unit!";

				(*j)->setSpecialAbility(SPECAB_NONE);
				convertUnit(*j, (*j)->getSpawnUnit());

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
				if (!psiCapture
					|| (*j)->getFaction() != FACTION_PLAYER)
				{
					liveAliens++;
				}
			}
			else if ((*j)->getOriginalFaction() == FACTION_PLAYER)
			{
				if ((*j)->getFaction() == FACTION_PLAYER)
				{
					liveSoldiers++;
				}
				else
				{
					liveAliens++;
				}
			}
		}
	}

	Log(LOG_INFO) << "BattlescapeGame::tallyUnits() EXIT";
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
 * @param unit Pointer to a unit.
 * @return True if a proximity grenade was triggered.
 */
bool BattlescapeGame::checkForProximityGrenades(BattleUnit* unit)
{
	int size = unit->getArmor()->getSize() - 1;
	for (int x = size; x >= 0; x--)
	{
		for (int y = size; y >= 0; y--)
		{
			for (int tx = -1; tx < 2; tx++)
			{
				for (int ty = -1; ty < 2; ty++)
				{
					Tile* t = _save->getTile(unit->getPosition() + Position(x, y, 0) + Position(tx, ty, 0));
					if (t)
					{
						for (std::vector<BattleItem*>::iterator i = t->getInventory()->begin(); i != t->getInventory()->end(); ++i)
						{
							if ((*i)->getRules()->getBattleType() == BT_PROXIMITYGRENADE
								&& (*i)->getExplodeTurn() == 0)
							{
								Position p;
								p.x = t->getPosition().x * 16 + 8;
								p.y = t->getPosition().y * 16 + 8;
								p.z = t->getPosition().z * 24 + t->getTerrainLevel();

								statePushNext(new ExplosionBState(this, p, *i, (*i)->getPreviousOwner()));

								getSave()->removeItem(*i);

								unit->setCache(0);
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

}
