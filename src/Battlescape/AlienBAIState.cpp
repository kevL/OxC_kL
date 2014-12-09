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

//#define _USE_MATH_DEFINES

#include "AlienBAIState.h"

//#include <algorithm>
//#include <climits>
//#include <cmath>

#include "ProjectileFlyBState.h"

#include "../Battlescape/BattlescapeState.h"
#include "../Battlescape/Map.h"
#include "../Battlescape/Pathfinding.h"
#include "../Battlescape/TileEngine.h"

#include "../Engine/Game.h"
//#include "../Engine/Logger.h"
//#include "../Engine/RNG.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/Armor.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/RuleInventory.h" // kL, grenade from Belt to Hand
#include "../Ruleset/Ruleset.h"

#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/Node.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

//bool const kL_bDebug = true;

/**
 * Sets up an AlienBAIState w/ BattleAIState.
 * @param save - pointer to SavedBattleGame
 * @param unit - pointer to the BattleUnit
 * @param node - pointer to the Node the unit originates from
 */
AlienBAIState::AlienBAIState(
		SavedBattleGame* save,
		BattleUnit* unit,
		Node* node)
	:
		BattleAIState(
			save,
			unit),
		_aggroTarget(NULL),
		_knownEnemies(0),
		_visibleEnemies(0),
		_spottingEnemies(0),
		_escapeTUs(0),
		_ambushTUs(0),
//kL	_reserveTUs(0),
		_rifle(false),
		_melee(false),
		_blaster(false),
		_grenade(false), // kL
		_didPsi(false),
		_AIMode(AI_PATROL),
		_closestDist(100),
		_fromNode(node),
		_toNode(NULL)
{
	//Log(LOG_INFO) << "Create AlienBAIState";
	_traceAI		= Options::traceAI;

	_reserve		= BA_NONE;
	_intelligence	= _unit->getIntelligence();

	_escapeAction	= new BattleAction();
	_ambushAction	= new BattleAction();
	_attackAction	= new BattleAction();
	_patrolAction	= new BattleAction();
	_psiAction		= new BattleAction();
}

/**
 * Deletes the BattleAIState.
 */
AlienBAIState::~AlienBAIState()
{
	//Log(LOG_INFO) << "Delete AlienBAIState";

	delete _escapeAction;
	delete _ambushAction;
	delete _attackAction;
	delete _patrolAction;
	delete _psiAction;
}

/**
 * Loads the AI state from a YAML file.
 * @param node - reference a YAML node
 */
void AlienBAIState::load(const YAML::Node& node)
{
	int
		fromNodeID,
		toNodeID;

	fromNodeID	= node["fromNode"]	.as<int>(-1);
	toNodeID	= node["toNode"]	.as<int>(-1);
	_AIMode		= node["AIMode"]	.as<int>(0);
//	_wasHitBy	= node["wasHitBy"].as<std::vector<int> >(_wasHitBy);

	if (fromNodeID != -1)
		_fromNode = _save->getNodes()->at(fromNodeID);

	if (toNodeID != -1)
		_toNode = _save->getNodes()->at(toNodeID);
}

/**
 * Saves the AI state to a YAML file.
 * @return, YAML node
 */
YAML::Node AlienBAIState::save() const
{
	int
		fromNodeID	= -1,
		toNodeID	= -1;

	if (_fromNode != NULL)	fromNodeID	= _fromNode->getID();
	if (_toNode != NULL)	toNodeID	= _toNode->getID();

	YAML::Node node;

	node["fromNode"]	= fromNodeID;
	node["toNode"]		= toNodeID;
	node["AIMode"]		= _AIMode;
//kL	node["wasHitBy"]		= _wasHitBy;

	return node;
}

/**
 * Enters the current AI state.
 */
void AlienBAIState::enter()
{
	//Log(LOG_INFO) << "AlienBAIState::enter()";
	// ROOOAARR !
}


/**
 * Exits the current AI state.
 */
void AlienBAIState::exit()
{
	//Log(LOG_INFO) << "AlienBAIState::exit()";
}

/**
 * Runs any code the state needs to keep updating every AI cycle.
 * @param action - pointer to AI BattleAction to execute after thinking is done
 */
void AlienBAIState::think(BattleAction* action)
{
	//Log(LOG_INFO) << "AlienBAIState::think(), unitID = " << _unit->getId();
 	action->type	= BA_RETHINK;
	action->actor	= _unit;
//kL	action->weapon	= _unit->getMainHandWeapon();	// kL_note: does not return grenades.
														// *cough* Does return grenades. -> DID return grenades but i fixed it so it won't.
	action->weapon	= _unit->getMainHandWeapon(false);	// this switches BL/Pistol aLiens to use BL (when not reaction firing)
														// It needs to be refined to account TU usage, also.

	if (action->weapon == NULL // kL ->
		&& _unit->getUnitRules() != NULL
		&& _unit->getUnitRules()->getMeleeWeapon() == "STR_FIST")
	{
		action->weapon = _save->getBattleGame()->getFist();
	}

	_attackAction->diff		= static_cast<int>(_save->getBattleState()->getGame()->getSavedGame()->getDifficulty());
	_attackAction->actor	= _unit;
	_attackAction->weapon	= action->weapon;
	_attackAction->number	= action->number;

	_escapeAction->number	= action->number;

	_knownEnemies		= countKnownTargets();
	_visibleEnemies		= selectNearestTarget();
	_spottingEnemies	= getSpottingUnits(_unit->getPosition());

	_melee		= (_unit->getMeleeWeapon().empty() == false); // kL_note: or STR_FIST
	_rifle		= false;
	_blaster	= false;
	_grenade	= false; // kL

	_reachable	= _save->getPathfinding()->findReachable(
													_unit,
													_unit->getTimeUnits());
//	_wasHitBy.clear();

	if (_unit->getCharging() != NULL
		&& _unit->getCharging()->isOut(true, true) == true)
	{
		_unit->setCharging(NULL);
	}

/*	if (_traceAI)
	{
		Log(LOG_INFO) << "Unit has " << _visibleEnemies
				<< "/" << _knownEnemies << " known enemies visible, "
				<< _spottingEnemies << " of whom are spotting him. ";

		std::string AIMode;
		switch (_AIMode)
		{
			case 0: AIMode = "Patrol"; break;
			case 1: AIMode = "Ambush"; break;
			case 2: AIMode = "Combat"; break;
			case 3: AIMode = "Escape"; break;
		}
		Log(LOG_INFO) << "Currently using " << AIMode << " behaviour";
	} */

	//Log(LOG_INFO) << ". . pos 1";
	if (action->weapon != NULL)
	{
		const RuleItem* const rule = action->weapon->getRules();
		if (rule->isWaterOnly() == false
			|| _save->getDepth() != 0)
		{
			if (rule->getBattleType() == BT_FIREARM)
			{
				if (rule->isWaypoint() == false)
				{
					_rifle = true;
					const int preShotTU = _unit->getTimeUnits() - _unit->getActionTUs(
																					BA_SNAPSHOT,
																					action->weapon);
					_reachableWithAttack = _save->getPathfinding()->findReachable(
																			_unit,
																			preShotTU);
				}
				else
				{
					_blaster = true;
					const int preShotTU = _unit->getTimeUnits() - _unit->getActionTUs(
																					BA_LAUNCH,
																					action->weapon);
					_reachableWithAttack = _save->getPathfinding()->findReachable(
																			_unit,
																			preShotTU);
				}
			}
			else if (rule->getBattleType() == BT_MELEE)
			{
				_melee = true;
				const int preShotTU = _unit->getTimeUnits() - _unit->getActionTUs(
																				BA_HIT,
																				action->weapon);
				_reachableWithAttack = _save->getPathfinding()->findReachable(
																		_unit,
																		preShotTU);
			}
			else if (rule->getBattleType() == BT_GRENADE)	// kL
				_grenade = true;							// kL, this is no longer useful since I fixed
															// getMainHandWeapon() to not return grenades.
		}
		else
			action->weapon = NULL;
	}
//	else if () // kL_add -> Give the invisible 'meleeWeapon' param a try ....
//	{
//	}


	//Log(LOG_INFO) << ". . pos 2";
	if (_spottingEnemies > 0
		&& _escapeTUs == 0)
	{
		setupEscape();
	}

	//Log(LOG_INFO) << ". . pos 3";
	if (_knownEnemies != 0
		&& _melee == false
		&& _ambushTUs == 0)
	{
		//Log(LOG_INFO) << ". . . . setupAmbush()";
		setupAmbush();
		//Log(LOG_INFO) << ". . . . setupAmbush() DONE";
	}

	//Log(LOG_INFO) << ". . . . setupAttack()";
	setupAttack();
	//Log(LOG_INFO) << ". . . . setupAttack() DONE, setupPatrol()";
	setupPatrol();
	//Log(LOG_INFO) << ". . . . setupPatrol() DONE";

	//Log(LOG_INFO) << ". . pos 4";
	if (_psiAction->type != BA_NONE
		&& _didPsi == false)
	{
		//Log(LOG_INFO) << ". . . inside Psi";
		_didPsi = true;

		action->type	= _psiAction->type;
		action->target	= _psiAction->target;
		action->number -= 1;

		//Log(LOG_INFO) << "AlienBAIState::think() EXIT, Psi";
		return;
	}
	else
		_didPsi = false;

	bool evaluate = false;

	//Log(LOG_INFO) << ". . pos 5";
	if (_AIMode == AI_ESCAPE)
	{
		if (_spottingEnemies == 0
			|| _knownEnemies == 0)
		{
			evaluate = true;
		}
	}
	else if (_AIMode == AI_PATROL)
	{
		if (_spottingEnemies > 0
			|| _visibleEnemies > 0
			|| _knownEnemies > 0
			|| RNG::percent(10) == true)
		{
			evaluate = true;
		}
	}
	else if (_AIMode == AI_AMBUSH)
	{
		if (_rifle == false
			|| _ambushTUs == 0
			|| _visibleEnemies != 0)
		{
			evaluate = true;
		}
	}

	//Log(LOG_INFO) << ". . pos 6";
	if (_AIMode == AI_COMBAT)
	{
		if (_attackAction->type == BA_RETHINK)
			evaluate = true;
	}

	if (_spottingEnemies > 2
		|| _unit->getHealth() < _unit->getBaseStats()->health * 2 / 3
		|| (_aggroTarget != NULL
			&& _aggroTarget->getTurnsExposed() > _intelligence))
	{
		evaluate = true;
	}

	if (_save->isCheating() == true
		&& _AIMode != AI_COMBAT)
	{
		evaluate = true;
	}

	//Log(LOG_INFO) << ". . pos 7";
	if (evaluate == true)
	{
		evaluateAIMode();

/*		if (_traceAI)
		{
			std::string AIMode;
			switch (_AIMode)
			{
				case 0: AIMode = "Patrol"; break;
				case 1: AIMode = "Ambush"; break;
				case 2: AIMode = "Combat"; break;
				case 3: AIMode = "Escape"; break;
			}

			Log(LOG_INFO) << "Re-Evaluated, now using " << AIMode << " behaviour";
		} */
	}

	//Log(LOG_INFO) << ". . pos 8";
	_reserve = BA_NONE;

	switch (_AIMode)
	{
		case AI_ESCAPE:
			//Log(LOG_INFO) << ". . . . AI_ESCAPE";
			_unit->setCharging(NULL);

			action->type		= _escapeAction->type;
			action->target		= _escapeAction->target;
			action->finalAction	= true;		// end this unit's turn.
			action->desperate	= true;		// ignore new targets.

			_unit->setHiding(true);			// spin 180 at the end of your route.

			// forget about reserving TUs, we need to get out of here.
//			_save->getBattleGame()->setTUReserved(BA_NONE, false); // kL
		break;

		case AI_PATROL:
			//Log(LOG_INFO) << ". . . . AI_PATROL";
			_unit->setCharging(NULL);

			if (action->weapon != NULL
				&& action->weapon->getRules()->getBattleType() == BT_FIREARM)
			{
				switch (_unit->getAggression())
				{
					case 0: _reserve = BA_AIMEDSHOT;	break;
					case 1: _reserve = BA_AUTOSHOT;		break;
					case 2: _reserve = BA_SNAPSHOT;
				}
			}

			action->type	= _patrolAction->type;
			action->target	= _patrolAction->target;
		break;

		case AI_COMBAT:
			//Log(LOG_INFO) << ". . . . AI_COMBAT";
			action->type	= _attackAction->type;
			action->target	= _attackAction->target;
			action->weapon	= _attackAction->weapon; // this may have changed to a grenade. Or an innate meleeWeapon ...

			if (action->weapon != NULL
				&& action->weapon->getRules()->getBattleType() == BT_GRENADE
				&& action->type == BA_THROW)
			{
				const RuleInventory* const rule = action->weapon->getSlot();
				int costTU = rule->getCost(_save->getBattleState()->getGame()->getRuleset()->getInventory("STR_RIGHT_HAND"));

				if (action->weapon->getFuseTimer() == -1)
				{
					costTU += _unit->getActionTUs(
												BA_PRIME,
												action->weapon);

//					action->weapon->setFuseTimer(0); // kL: but this is going to be done in ProjectileFlyBState.
				}

				_unit->spendTimeUnits(costTU); // cf. grenadeAction()
				//Log(LOG_INFO) << "AlienBAIState::think() Move & Prime GRENADE, costTU = " << costTU;
			}

			action->finalFacing	= _attackAction->finalFacing;					// if this is a firepoint action, set our facing.
			action->TU			= _unit->getActionTUs(
													_attackAction->type,
													_attackAction->weapon);

//			_save->getBattleGame()->setTUReserved(BA_NONE, false);				// don't worry about reserving TUs, we've factored that in already.

			if (action->type == BA_WALK											// if this is a "find fire point" action, don't increment the AI counter.
				&& _rifle == true
				&& _unit->getTimeUnits() > _unit->getActionTUs(
															BA_SNAPSHOT,
															action->weapon))	// so long as we can take a shot afterwards.
			{
				action->number -= 1;
			}
			else if (action->type == BA_LAUNCH)
				action->waypoints = _attackAction->waypoints;
		break;

		case AI_AMBUSH:
			//Log(LOG_INFO) << ". . . . AI_AMBUSH";
			_unit->setCharging(NULL);

			action->type		= _ambushAction->type;
			action->target		= _ambushAction->target;
			action->finalFacing	= _ambushAction->finalFacing;	// face where we think our target will appear.
			action->finalAction	= true;							// end this unit's turn.
																// we've factored in the reserved TUs already, so don't worry.
	}

	//Log(LOG_INFO) << ". . pos 9";
	if (action->type == BA_WALK)
	{
		if (action->target != _unit->getPosition()) // if we're moving, we'll have to re-evaluate our escape/ambush position.
		{
			_escapeTUs = 0;
			_ambushTUs = 0;
		}
		else
			action->type = BA_NONE;
	}
	//Log(LOG_INFO) << "AlienBAIState::think() EXIT";
}

/**
 * Sets the "was hit" flag to true.
 */
/* void AlienBAIState::setWasHitBy(BattleUnit* attacker)
{
	if (attacker->getFaction() != _unit->getFaction() && !getWasHitBy(attacker->getId()))
		_wasHitBy.push_back(attacker->getId());
} */

/*
 * Gets whether the unit was hit.
 * @return if it was hit.
 */
/* bool AlienBAIState::getWasHitBy(int attacker) const
{
	return std::find(_wasHitBy.begin(), _wasHitBy.end(), attacker) != _wasHitBy.end();
} */

/**
 * Sets up a patrol action.
 * This is mainly going from node to node, moving about the map.
 * Handles node selection, and fills out the _patrolAction with useful data.
 */
void AlienBAIState::setupPatrol()
{
	//Log(LOG_INFO) << "AlienBAIState::setupPatrol()";

	Node* node = NULL;
	_patrolAction->TU = 0;

	if (_toNode != NULL
		&& _unit->getPosition() == _toNode->getPosition())
	{
		//if (_traceAI) Log(LOG_INFO) << "Patrol destination reached!";

		// destination reached; head off to next patrol node
		_fromNode = _toNode;
		_toNode->freeNode();
		_toNode = NULL;

		// take a peek through window before walking to the next node
		const int dir = _save->getTileEngine()->faceWindow(_unit->getPosition());
		if (dir != -1
			&& dir != _unit->getDirection())
		{
			_unit->lookAt(dir);

			while (_unit->getStatus() == STATUS_TURNING)
				_unit->turn();
		}
	}

	if (_fromNode == NULL)
	{
		// assume closest node as "from node"
		// on same level to avoid strange things, and the node has to match unit size or it will freeze
		int closest = 1000000;

		for (std::vector<Node*>::const_iterator
				i = _save->getNodes()->begin();
				i != _save->getNodes()->end();
				++i)
		{
			node = *i;
			const int dist = _save->getTileEngine()->distanceSq(
															_unit->getPosition(),
															node->getPosition());

			if (_unit->getPosition().z == node->getPosition().z
				&& dist < closest
				&& (!
					(node->getType() & Node::TYPE_SMALL)
						|| _unit->getArmor()->getSize() == 1))
			{
				_fromNode = node;
				closest = dist;
			}
		}
	}

	int triesLeft = 5;
	while (_toNode == NULL
		&& triesLeft != 0)
	{
		--triesLeft;

		// look for a new node to walk towards
		bool scout = true;

		if (_save->getMissionType() != "STR_BASE_DEFENSE")
		{
			// after turn 20 or if the morale is low, everyone moves out the UFO and scout

			// kL_note: That, above is wrong. Orig behavior depends on "aggression" setting;
			// determines whether aliens come out of UFO to scout/search.

			// also anyone standing in fire should also probably move
			if (_save->isCheating() == true
				|| _fromNode == NULL
				|| _fromNode->getRank() == 0
				|| (_save->getTile(_unit->getPosition()) != NULL
					&& _save->getTile(_unit->getPosition())->getFire() != 0))
			{
				scout = true;
			}
			else
				scout = false;
		}

		// In base defense missions the non-large aliens walk towards
		// target nodes -- or once there shoot objects thereabouts.
		else if (_unit->getArmor()->getSize() == 1)
		{
			// scan this room for objects to destroy
			if (_fromNode->isTarget() == true
				&& _unit->getMainHandWeapon() != NULL
				&& _unit->getMainHandWeapon()->getAmmoItem()->getRules()->getDamageType() != DT_HE
				&& _save->getModuleMap()[_fromNode->getPosition().x / 10][_fromNode->getPosition().y / 10].second > 0)
			{
				const int
					x = (_unit->getPosition().x / 10) * 10,
					y = (_unit->getPosition().y / 10) * 10;

				for (int
						i = x;
						i < x + 9;
						++i)
				{
					for (int
							j = y;
							j < y + 9;
							++j)
					{
						const MapData* const data = _save->getTile(Position(i, j, 1))->getMapData(MapData::O_OBJECT);
						if (data != NULL
							&& data->isBaseModule() == true)
//							&& data->getDieMCD()
//							&& data->getArmor() < 60)
						{
							_patrolAction->actor	= _unit;
							_patrolAction->target	= Position(i, j, 1);
							_patrolAction->weapon	= _patrolAction->actor->getMainHandWeapon();
							_patrolAction->type		= BA_SNAPSHOT;
							_patrolAction->TU		= _patrolAction->actor->getActionTUs(
																					_patrolAction->type,
																					_patrolAction->weapon);
							return;
						}
					}
				}
			}
			else
			{
				// find closest high value target which is not already allocated
				int closest = 1000000;

				for (std::vector<Node*>::const_iterator
						i = _save->getNodes()->begin();
						i != _save->getNodes()->end();
						++i)
				{
					if ((*i)->isTarget() == true
						&& (*i)->isAllocated() == false)
					{
						node = *i;
						const int dist = _save->getTileEngine()->distanceSq(
																		_unit->getPosition(),
																		node->getPosition());
						if (_toNode == NULL
							|| (dist < closest
								&& node != _fromNode))
						{
							_toNode = node;
							closest = dist;
						}
					}
				}
			}
		}

		if (_toNode == NULL)
		{
			//Log(LOG_INFO) << ". _toNode == 0 a -> getPatrolNode(scout)";
			_toNode = _save->getPatrolNode(
										scout,
										_unit,
										_fromNode);

			if (_toNode == NULL)
			{
				//Log(LOG_INFO) << ". _toNode == 0 b -> getPatrolNode(!scout)";
				_toNode = _save->getPatrolNode(
											!scout,
											_unit,
											_fromNode);
			}
		}

		if (_toNode != NULL)
		{
			_save->getPathfinding()->calculate(
											_unit,
											_toNode->getPosition());

//			if (std::find(
//						_reachable.begin(),
//						_reachable.end(),
//						_save->getTileIndex(_toNode->getPosition())) == _reachable.end()) // kL
			if (_save->getPathfinding()->getStartDirection() == -1)
				_toNode = NULL;

			_save->getPathfinding()->abortPath(); // kL_note: should this be with {_toNode=0} ?
		}
	}

	if (_toNode != NULL)
	{
		_toNode->allocateNode();

		_patrolAction->actor = _unit;
		_patrolAction->target = _toNode->getPosition();
		_patrolAction->type = BA_WALK;
	}
	else
		_patrolAction->type = BA_RETHINK;
	//Log(LOG_INFO) << "AlienBAIState::setupPatrol() EXIT";
}

/**
 * Try to set up an ambush action.
 * The idea is to check within a 11x11 tile square for a tile which is not seen by
 * our aggroTarget, but that can be reached by him. We then intuit where we will
 * see the target first from our covered position, and set that as our final facing.
 * Fills out the _ambushAction with useful data.
 */
void AlienBAIState::setupAmbush()
{
	_ambushAction->type = BA_RETHINK;
	_ambushTUs = 0;

	int bestScore = 0;

	std::vector<int> path;

	if (selectClosestKnownEnemy() == true)
	{
		const int
			BASE_SYSTEMATIC_SUCCESS	= 100,
			COVER_BONUS				= 25,
			FAST_PASS_THRESHOLD		= 80;

		Position origin = _save->getTileEngine()->getSightOriginVoxel(_aggroTarget);
		Position target;

		// we'll use node positions for this, since it gives map makers a
		// good degree of control over how the units will use the environment.
		for (std::vector<Node*>::const_iterator
				i = _save->getNodes()->begin();
				i != _save->getNodes()->end();
				++i)
		{
			const Position pos = (*i)->getPosition();
			Tile* const tile = _save->getTile(pos);
			if (tile == NULL
				|| _save->getTileEngine()->distance(pos, _unit->getPosition()) > 10
				|| pos.z != _unit->getPosition().z
				|| tile->getDangerous() == true
				|| std::find(
						_reachableWithAttack.begin(),
						_reachableWithAttack.end(),
						_save->getTileIndex(pos)) == _reachableWithAttack.end())
			{
				continue; // just ignore unreachable tiles
			}

/*			if (_traceAI) // colour all the nodes in range purple.
			{
				tile->setPreview(10);
				tile->setMarkerColor(13);
			} */

			if (_save->getTileEngine()->canTargetUnit(
													&origin,
													tile,
													&target,
													_aggroTarget,
													_unit) == false
				&& getSpottingUnits(pos) == 0)											// make sure we can't be seen here.
			{
				_save->getPathfinding()->calculate(_unit, pos);
				const int ambushTUs = _save->getPathfinding()->getTotalTUCost();

				if (_save->getPathfinding()->getStartDirection() != -1)					// make sure we can move here
//					&& ambushTUs <= _unit->getTimeUnits()
//							- _unit->getActionTUs(BA_SNAPSHOT, _attackAction->weapon))	// make sure we can still shoot
				{
					int score = BASE_SYSTEMATIC_SUCCESS;
					score -= ambushTUs;

					_save->getPathfinding()->calculate(_aggroTarget, pos);				// make sure our enemy can reach here too.

					if (_save->getPathfinding()->getStartDirection() != -1)
					{
						if (_save->getTileEngine()->faceWindow(pos) != -1)				// ideally we'd like to be behind some cover,
						{																// like say a window or a low wall.
							score += COVER_BONUS;
						}

						if (score > bestScore)
						{
							path = _save->getPathfinding()->copyPath();

							_ambushAction->target = pos;
							if (pos == _unit->getPosition())
								_ambushTUs = 1;
							else
								_ambushTUs = ambushTUs;

							bestScore = score;
							if (bestScore > FAST_PASS_THRESHOLD)
								break;
						}
					}
				}
			}
		}

		if (bestScore > 0)
		{
			_ambushAction->type = BA_WALK;

			// i should really make a function for this
			origin = _ambushAction->target * Position(16, 16, 24)
					+ Position(
							8,
							8,
							_unit->getHeight() + _unit->getFloatHeight()
								- _save->getTile(_ambushAction->target)->getTerrainLevel()
								- 4); // -4, because -2 is eyes and -2 that is the rifle ( perhaps )

			Position currentPos = _aggroTarget->getPosition();
			_save->getPathfinding()->setUnit(_aggroTarget);
			Position nextPos;

			size_t tries = path.size();
			while (tries > 0) // hypothetically walk the target through the path.
			{
				_save->getPathfinding()->getTUCost(
												currentPos,
												path.back(),
												&nextPos,
												_aggroTarget,
												NULL,
												false);
				path.pop_back();
				currentPos = nextPos;

				const Tile* const tile = _save->getTile(currentPos);
				Position target; // done above?
				if (_save->getTileEngine()->canTargetUnit( // do a virtual fire calculation
														&origin,
														tile,
														&target,
														_unit,
														_aggroTarget) == true)
				{
					// if we can virtually fire at the hypothetical target, we know which way to face.
					_ambushAction->finalFacing = _save->getTileEngine()->getDirectionTo(
																					_ambushAction->target,
																					currentPos);
					break;
				}

				--tries;
			}
			//if (_traceAI) Log(LOG_INFO) << "Ambush estimation will move to " << _ambushAction->target;
			return;
		}
	}
	//if (_traceAI) Log(LOG_INFO) << "Ambush estimation failed";
}

/**
 * Try to set up a combat action.
 * This will either be a psionic, grenade, or weapon attack,
 * or potentially just moving to get a line of sight to a target.
 * Fills out the _attackAction with useful data.
 */
void AlienBAIState::setupAttack()
{
	//Log(LOG_INFO) << "AlienBAIState::setupAttack()";
	_attackAction->type = BA_RETHINK;
	_psiAction->type = BA_NONE;

	// if enemies are known to us but not necessarily visible, we can attack them with a blaster launcher or psi.
	if (_knownEnemies != 0)
	{
		if (_unit->getBaseStats()->psiSkill
			&& psiAction() == true
			&& _didPsi == false) // kL, also checked in AlienBAIState::psiAction().
		{
			//Log(LOG_INFO) << "do Psi attack";
			// at this point we can save some time with other calculations - the unit WILL make a psionic attack this turn.
			return;
		}

		if (_blaster == true)
		{
			//Log(LOG_INFO) << "do Blaster launch";
			wayPointAction();
		}
	}

	// if we CAN see someone, that makes them a viable target for "regular" attacks.
	//Log(LOG_INFO) << ". . selectNearestTarget()";
	if (selectNearestTarget() != 0)
	{
		//Log(LOG_INFO) << ". . . . selectNearestTarget()";
		// if there are both types of weapon, make a determination on which to use.
		if (_melee == true
			&& _rifle == true)
		{
			selectMeleeOrRanged();
		}

		if (_unit->getGrenade() != NULL)
		{
			//Log(LOG_INFO) << ". . . . . . grenadeAction()";
			grenadeAction();
			//Log(LOG_INFO) << ". . . . . . grenadeAction() DONE";
		}

		if (_melee == true)
		{
			//Log(LOG_INFO) << ". . . . . . meleeAction()";
			meleeAction();
			//Log(LOG_INFO) << ". . . . . . meleeAction() DONE";
		}

		if (_rifle == true)
		{
			//Log(LOG_INFO) << ". . . . . . projectileAction()";
			projectileAction();
			//Log(LOG_INFO) << ". . . . . . projectileAction() DONE";
		}
	}
	//Log(LOG_INFO) << ". . selectNearestTarget() DONE";

	if (_attackAction->type != BA_RETHINK)
	{
/*		if (_traceAI)
		{
			if (_attackAction->type != BA_WALK) Log(LOG_INFO) << "Attack estimation desires to shoot at " << _attackAction->target;
			else Log(LOG_INFO) << "Attack estimation desires to move to " << _attackAction->target;
		} */

		return;
	}
	else if (_spottingEnemies != 0
		|| _unit->getAggression() < RNG::generate(0, 3))
	{
		if (findFirePoint() == true) // if enemies can see us, or if we're feeling lucky, we can try to spot the enemy.
		{
			//if (_traceAI) Log(LOG_INFO) << "Attack estimation desires to move to " << _attackAction->target;
			return;
		}
	}
	//if (_traceAI) Log(LOG_INFO) << "Attack estimation failed";
	//Log(LOG_INFO) << "AlienBAIState::setupAttack() EXIT";
}

/**
 * Attempts to find cover, and move toward it.
 * The idea is to check within a 11x11 tile square for a tile which is not seen by our aggroTarget.
 * If there is no such tile, we run away from the target.
 * Fills out the _escapeAction with useful data.
 */
void AlienBAIState::setupEscape()
{
	bool coverFound = false;
	const int
		unitsSpottingMe = getSpottingUnits(_unit->getPosition()),
		currentTilePreference = 15;
	int tries = -1;

	selectNearestTarget();
	_escapeTUs = 0;

	int dist = 0;
	if (_aggroTarget != NULL)
		dist = _save->getTileEngine()->distance(
											_unit->getPosition(),
											_aggroTarget->getPosition());

	int
		bestTileScore = -100000,
		score = -100000;

	Position bestTile(0, 0, 0);
	const Tile* tile = NULL;

	// weights of various factors in choosing a tile to which to withdraw
	const int
		EXPOSURE_PENALTY		= 10,
		FIRE_PENALTY			= 40,
		BASE_SYSTEMATIC_SUCCESS	= 100,
		BASE_DESPERATE_SUCCESS	= 110,
		FAST_PASS_THRESHOLD		= 100;	// a score that's good enough to quit the while loop early;
										// it's subjective, hand-tuned and may need tweaking

	std::vector<Position> randomTileSearch = _save->getTileSearch();
	RNG::shuffle(randomTileSearch);

	while (tries < 150
		&& coverFound == false)
	{
		_escapeAction->target = _unit->getPosition();		// start looking in a direction away from the enemy
		if (_save->getTile(_escapeAction->target) == NULL)
			_escapeAction->target = _unit->getPosition();	// cornered at the edge of the map perhaps?

		score = 0;

		if (tries == -1)
		{
			// you know, maybe we should just stay where we are and not risk reaction fire...
			// or maybe continue to wherever we were running to and not risk looking stupid
			if (_save->getTile(_unit->lastCover) != 0)
				_escapeAction->target = _unit->lastCover;
		}
		else if (tries < 121) // looking for cover
		{
			_escapeAction->target.x += randomTileSearch[tries].x;
			_escapeAction->target.y += randomTileSearch[tries].y;

			score = BASE_SYSTEMATIC_SUCCESS;

			if (_escapeAction->target == _unit->getPosition())
			{
				if (unitsSpottingMe > 0)
				{
					// maybe don't stay in the same spot? move or something if there's any point to it?
					_escapeAction->target.x += RNG::generate(-20, 20);
					_escapeAction->target.y += RNG::generate(-20, 20);
				}
				else
					score += currentTilePreference;
			}
		}
		else
		{
			//if (tries == 121 && _traceAI) Log(LOG_INFO) << "best score after systematic search was: " << bestTileScore;

			score = BASE_DESPERATE_SUCCESS; // ruuuuuuun!!1

			_escapeAction->target = _unit->getPosition();
			_escapeAction->target.x += RNG::generate(-10, 10);
			_escapeAction->target.y += RNG::generate(-10, 10);
			_escapeAction->target.z = _unit->getPosition().z + RNG::generate(-1, 1);

			if (_escapeAction->target.z < 0)
				_escapeAction->target.z = 0;
			else if (_escapeAction->target.z >= _save->getMapSizeZ())
				_escapeAction->target.z = _unit->getPosition().z;
		}

		++tries;

		// THINK, DAMN YOU!!!
		tile = _save->getTile(_escapeAction->target);
		int distTarget = 0;
		if (_aggroTarget != NULL)
			distTarget = _save->getTileEngine()->distance(
													_aggroTarget->getPosition(),
													_escapeAction->target);
		if (dist >= distTarget)
			score -= (distTarget - dist) * 10;
		else
			score += (distTarget - dist) * 10;

		int spotters = 0;

		if (tile == NULL)
			score = -100001; // no you can't quit the battlefield by running off the map.
		else
		{
			if (std::find(
						_reachable.begin(),
						_reachable.end(),
						_save->getTileIndex(_escapeAction->target)) == _reachable.end())
			{
				continue; // just ignore unreachable tiles
			}

			spotters = getSpottingUnits(_escapeAction->target);
			if (_spottingEnemies != 0
				|| spotters != 0)
			{
				if (_spottingEnemies <= spotters)
					score -= (1 + spotters - _spottingEnemies) * EXPOSURE_PENALTY; // that's for giving away our position
				else
					score += (_spottingEnemies - spotters) * EXPOSURE_PENALTY;
			}

			if (tile->getFire() != 0)
				score -= FIRE_PENALTY;

			if (tile->getDangerous() == true)
				score -= BASE_SYSTEMATIC_SUCCESS;

/*			if (_traceAI)
			{
				tile->setMarkerColor(score < 0? 3: (score < FAST_PASS_THRESHOLD / 2? 8: (score < FAST_PASS_THRESHOLD? 9: 5)));
				tile->setPreview(10);
				tile->setTUMarker(score);
			} */
		}

		if (tile != NULL
			&& score > bestTileScore)
		{
			// calculate TUs to tile; we could be getting this from findReachable() somehow but that would break something for sure...
			_save->getPathfinding()->calculate(
											_unit,
											_escapeAction->target);

			if (_escapeAction->target == _unit->getPosition()
				|| _save->getPathfinding()->getStartDirection() != -1)
			{
				bestTileScore = score;
				bestTile = _escapeAction->target;

				_escapeTUs = _save->getPathfinding()->getTotalTUCost();
				if (_escapeAction->target == _unit->getPosition())
					_escapeTUs = 1;

/*				if (_traceAI)
				{
					tile->setMarkerColor(score < 0? 7:(score < FAST_PASS_THRESHOLD / 2? 10:(score < FAST_PASS_THRESHOLD? 4:5)));
					tile->setPreview(10);
					tile->setTUMarker(score);
				} */
			}

			_save->getPathfinding()->abortPath();

			if (bestTileScore > FAST_PASS_THRESHOLD)
				coverFound = true; // good enough, gogogo!!
		}
	}

	_escapeAction->target = bestTile;

	//if (_traceAI) _save->getTile(_escapeAction->target)->setMarkerColor(13);

	if (bestTileScore <= -100000)
	{
		//if (_traceAI) Log(LOG_INFO) << "Escape estimation failed.";
		_escapeAction->type = BA_RETHINK; // do something, just don't look dumbstruck :P
		return;
	}
	else
	{
		//if (_traceAI) Log(LOG_INFO) << "Escape estimation completed after " << tries << " tries, "
		//		<< _save->getTileEngine()->distance(_unit->getPosition(), bestTile) << " squares or so away.";
		_escapeAction->type = BA_WALK;
	}
}

/**
 * Counts how many targets, both xcom and civilian are known to the unit.
 * @return, how many targets are known to it
 */
int AlienBAIState::countKnownTargets() const
{
	int knownEnemies = 0;

	for (std::vector<BattleUnit*>::const_iterator
			i = _save->getUnits()->begin();
			i != _save->getUnits()->end();
			++i)
	{
		if (validTarget(
					*i,
					true,
					true) == true)
		{
			++knownEnemies;
		}
	}

	return knownEnemies;
}

/**
 * Counts how many enemies (xcom only) are spotting any given position.
 * @param pos - the Position to check for spotters
 * @return, spotters
 */
int AlienBAIState::getSpottingUnits(Position pos) const
{
	// if we don't actually occupy the position being checked, we need to do a virtual LOF check.
	const bool checking = (pos != _unit->getPosition());
	int tally = 0;

	for (std::vector<BattleUnit*>::const_iterator
			i = _save->getUnits()->begin();
			i != _save->getUnits()->end();
			++i)
	{
		if (validTarget(
					*i,
					false,
					false) == true)
		{
			const int dist = _save->getTileEngine()->distance(
															pos,
															(*i)->getPosition());
			if (dist > 20)
				continue;

			Position originVoxel = _save->getTileEngine()->getSightOriginVoxel(*i);
			originVoxel.z -= 2;
			Position targetVoxel;

			const BattleUnit* potentialUnit = NULL;
			if (checking == true)
				potentialUnit = _unit;

			if (_save->getTileEngine()->canTargetUnit(
													&originVoxel,
													_save->getTile(pos),
													&targetVoxel,
													*i,
													potentialUnit) == true)
			{
				++tally;
			}
		}
	}

	return tally;
}

/**
 * Selects the nearest known living target we can see/reach
 * and returns the number of visible enemies.
 * This function includes civilians as viable targets.
 * @return, viable targets
 */
int AlienBAIState::selectNearestTarget()
{
	int tally = 0;

	_aggroTarget = NULL;
	_closestDist = 100;

	Position
		target,
		origin = _save->getTileEngine()->getSightOriginVoxel(_unit);

	origin.z -= 2;


	for (std::vector<BattleUnit*>::const_iterator
			i = _save->getUnits()->begin();
			i != _save->getUnits()->end();
			++i)
	{
		if (validTarget(
					*i,
					true,
					true) == true
			&& _save->getTileEngine()->visible(
											_unit,
											(*i)->getTile()) == true)
		{
			++tally;

			const int dist = _save->getTileEngine()->distance(
														_unit->getPosition(),
														(*i)->getPosition());
			if (dist < _closestDist)
			{
				bool valid = false;
				if (_rifle == true
					|| _melee == false)
				{
					valid = _save->getTileEngine()->canTargetUnit(
																&origin,
																(*i)->getTile(),
																&target,
																_unit);
				}
				else if (selectPointNearTarget(
											*i,
											_unit->getTimeUnits()) == true)
				{
					const int dir = _save->getTileEngine()->getDirectionTo(
																_attackAction->target,
																(*i)->getPosition());
					valid = _save->getTileEngine()->validMeleeRange(
																_attackAction->target,
																dir,
																_unit,
																*i,
																NULL);
				}

				if (valid == true)
				{
					_closestDist = dist;
					_aggroTarget = *i;
				}
			}
		}
	}

	if (_aggroTarget != NULL)
		return tally;

	return 0;
}

/**
 * Selects the nearest known living Xcom unit - used for ambush calculations.
 * @return, true if it found one
 */
bool AlienBAIState::selectClosestKnownEnemy()
{
	_aggroTarget = NULL;
	int minDist = 255;

	for (std::vector<BattleUnit*>::const_iterator
			i = _save->getUnits()->begin();
			i != _save->getUnits()->end();
			++i)
	{
		if (validTarget(
					*i,
					true,
					false) == true)
		{
			const int dist = _save->getTileEngine()->distance(
														(*i)->getPosition(),
														_unit->getPosition());
			if (dist < minDist)
			{
				minDist = dist;
				_aggroTarget = *i;
			}
		}
	}

	return (_aggroTarget != NULL);
}

/**
 * Selects a random known living Xcom or civilian unit.
 * @return, true if it found one
 */
bool AlienBAIState::selectRandomTarget()
{
	_aggroTarget = NULL;
	int farthest = -100;

	for (std::vector<BattleUnit*>::const_iterator
			i = _save->getUnits()->begin();
			i != _save->getUnits()->end();
			++i)
	{
		if (validTarget(
					*i,
					true,
					true) == true)
		{
			const int dist = RNG::generate(0, 20)
						   - _save->getTileEngine()->distance(
														_unit->getPosition(),
														(*i)->getPosition());
			if (dist > farthest)
			{
				farthest = dist;
				_aggroTarget = *i;
			}
		}
	}

	return (_aggroTarget != NULL);
}

/**
 * Selects a point near enough to our target to perform a melee attack.
 * @param target - pointer to a target BattleUnit
 * @param maxTUs - maximum time units that the path to the target can cost
 * @return, true if a point was found
 */
bool AlienBAIState::selectPointNearTarget(
		BattleUnit* target,
		int maxTUs) const
{
	bool ret = false;

	const int
		unitSize = _unit->getArmor()->getSize(),
		targetSize = target->getArmor()->getSize();
	size_t distance = 1000;

	for (int
			z = -1;
			z <= 1;
			++z)
	{
		for (int
				x = -unitSize;
				x <= targetSize;
				++x)
		{
			for (int
					y = -unitSize;
					y <= targetSize;
					++y)
			{
				if (x || y) // skip the unit itself
				{
					const Position checkPath = target->getPosition() + Position (x, y, z);

					if (_save->getTile(checkPath) == NULL
						|| std::find(
									_reachable.begin(),
									_reachable.end(),
									_save->getTileIndex(checkPath)) == _reachable.end())
					{
						continue;
					}

					const int dir = _save->getTileEngine()->getDirectionTo(
																		checkPath,
																		target->getPosition());
					const bool
						valid = _save->getTileEngine()->validMeleeRange(
																	checkPath,
																	dir,
																	_unit,
																	target,
																	NULL),
						fitHere = _save->setUnitPosition(
														_unit,
														checkPath,
														true);
					if (valid == true
						&& fitHere == true
						&& _save->getTile(checkPath)->getDangerous() == false)
					{
						_save->getPathfinding()->calculate(
														_unit,
														checkPath,
														NULL,
														maxTUs);

						if (_save->getPathfinding()->getStartDirection() != -1
							&& _save->getPathfinding()->getPath().size() < distance)
//							&& _save->getTileEngine()->distance(
//															checkPath,
//															_unit->getPosition())
//														< distance)
						{
							ret = true;

							_attackAction->target = checkPath;
							distance = _save->getPathfinding()->getPath().size();
//							distance = _save->getTileEngine()->distance(
//																	checkPath,
//																	_unit->getPosition());
						}

						_save->getPathfinding()->abortPath();
					}
				}
			}
		}
	}

	return ret;
}

/**
 * Selects an AI mode based on a number of factors:
 * some RNG and the results of the rest of the determinations.
 */
void AlienBAIState::evaluateAIMode()
{
	if (_unit->getCharging() != NULL
		&& _attackAction->type != BA_RETHINK)
	{
		_AIMode = AI_COMBAT;
		return;
	}

	float escapeOdds = 13.f; // was 15
	if (_unit->getTimeUnits() > _unit->getBaseStats()->tu / 2
		|| _unit->getCharging() != NULL)
	{
		escapeOdds = 5.f;
	}
	else if (_melee == true)
		escapeOdds = 10.f; // was 12

	float ambushOdds = 12.f;
	float combatOdds = 22.f; // was 20

	// we're less likely to patrol if we see enemies.
	float patrolOdds = 30.f;
	if (_visibleEnemies != 0)
		patrolOdds = 10.f; // was 15

	// the enemy sees us, we should take retreat into consideration, and forget about patrolling for now.
	if (_spottingEnemies != 0)
	{
		patrolOdds = 0.f;

		if (_escapeTUs == 0)
			setupEscape();
	}

	// melee/blaster units shouldn't consider ambush
	if (_rifle == false
		|| _ambushTUs == 0)
	{
		ambushOdds = 0.f;

		if (_melee == true)
		{
//			escapeOdds = 10.f;	// kL
			combatOdds *= 1.2f;	// kL
		}
	}

	// if we KNOW there are enemies around...
	if (_knownEnemies != 0)
	{
		if (_knownEnemies == 1)
			combatOdds *= 1.8f; // was 1.2

		if (_escapeTUs == 0)
		{
			if (selectClosestKnownEnemy() == true)
				setupEscape();
			else
				escapeOdds = 0.f;
		}
	}
	else
	{
		combatOdds = 0.f;
		escapeOdds = 0.f;
	}

	// take our current mode into consideration
	switch (_AIMode)
	{
		case AI_PATROL:
			patrolOdds *= 1.2f; // was 1.1
		break;
		case AI_AMBUSH:
			ambushOdds *= 1.2f; // was 1.1
		break;
		case AI_COMBAT:
			combatOdds *= 1.2f; // was 1.1
		break;
		case AI_ESCAPE:
			escapeOdds *= 1.2f; // was 1.1
	}

	// take our overall health into consideration
	if (_unit->getHealth() < _unit->getBaseStats()->health / 3)
	{
		escapeOdds *= 1.8f; // was 1.7
		combatOdds *= 0.5f; // was 0.6
		ambushOdds *= 0.7f; // was 0.75
	}
	else if (_unit->getHealth() < _unit->getBaseStats()->health * 2 / 3)
	{
		escapeOdds *= 1.5f; // was 1.4
		combatOdds *= 0.9f; // was 0.8
		ambushOdds *= 0.8f; // was 0.8
	}
	else if (_unit->getHealth() < _unit->getBaseStats()->health)
		escapeOdds *= 1.3f; // was 1.1

	switch (_unit->getAggression()) // take our aggression into consideration
	{
		case 0:
			escapeOdds *= 1.5f; // was 1.4
			combatOdds *= 0.5f; // was 0.7
		break;
		case 1:
			ambushOdds *= 1.2f; // was 1.1
		break;
		case 2:
			combatOdds *= 1.5f; // was 1.4
			escapeOdds *= 0.5f; // was 0.7
		break;

		default:
			combatOdds *= std::max(
								0.1f,
								std::min(
										2.f,
										1.2f + (static_cast<float>(_unit->getAggression()) / 10.f)));
			escapeOdds *= std::min(
								2.f,
								std::max(
										0.1f,
										0.9f - (static_cast<float>(_unit->getAggression()) / 10.f)));
		break;
	}

	if (_AIMode == AI_COMBAT)
		ambushOdds *= 1.5f;

	if (_spottingEnemies) // factor in the spotters.
	{
		escapeOdds *= (10.f * static_cast<float>(_spottingEnemies + 10) / 100.f);
		combatOdds *= (5.f * static_cast<float>(_spottingEnemies + 20) / 100.f);
	}
	else
		escapeOdds /= 2.f;

	if (_visibleEnemies) // factor in visible enemies.
	{
		combatOdds *= (10.f * static_cast<float>(_visibleEnemies + 10) / 100.f);

		if (_closestDist < 6) // was <5
			ambushOdds = 0.f;
	}

	if (_ambushTUs) // make sure we have an ambush lined up, or don't even consider it.
		ambushOdds *= 2.f; // was 1.7
	else
		ambushOdds = 0.f;

	// factor in mission type
	if (_save->getMissionType() == "STR_BASE_DEFENSE")
	{
		escapeOdds *= 0.8f; // was 0.75
		ambushOdds *= 0.5f; // was 0.6
	}

	// GENERATE A RANDOM NUMBER TO PRESENT OUR SITUATION:
	int decision = RNG::generate(
								1,
								std::max(
										1,
										static_cast<int>(patrolOdds + ambushOdds + escapeOdds + combatOdds)));

	// if the aliens are cheating, or the unit is charging, enforce combat as a priority.
	if (_save->isCheating() == true // <- hmm, do i want this - kL_note
		|| _unit->getCharging() != NULL)
	{
		_AIMode = AI_COMBAT;
	}
	else if (static_cast<float>(decision) > escapeOdds)
	{
		if (static_cast<float>(decision) > escapeOdds + ambushOdds)
		{
			if (static_cast<float>(decision) > escapeOdds + ambushOdds + combatOdds)
				_AIMode = AI_PATROL;
			else
				_AIMode = AI_COMBAT;
		}
		else
			_AIMode = AI_AMBUSH;
	}
	else
		_AIMode = AI_ESCAPE;

	// enforce the validity of our decision, and try fallback behaviour according to priority.
	if (_AIMode == AI_COMBAT)
	{
//		if (_aggroTarget) // kL
		if (_save->getTile(_attackAction->target) != NULL
			&& _save->getTile(_attackAction->target)->getUnit() != NULL)
		{
			if (_attackAction->type != BA_RETHINK)
				return;

			if (findFirePoint() == true)
				return;
		}
		else if (selectRandomTarget() == true
			&& findFirePoint() == true)
		{
			return;
		}

		_AIMode = AI_PATROL;
	}

	if (_AIMode == AI_PATROL)
	{
		if (_toNode != NULL)
			return;

		_AIMode = AI_AMBUSH;
	}

	if (_AIMode == AI_AMBUSH)
	{
		if (_ambushTUs != 0)
			return;

		_AIMode = AI_ESCAPE;
	}
}

/**
 * Find a position where we can see our target, and move there.
 * Check the 11x11 grid for a position nearby where we can potentially target him.
 * @return, true if a possible position was found
 */
bool AlienBAIState::findFirePoint()
{
	if (selectClosestKnownEnemy() == false)
		return false;

	std::vector<Position> randomTileSearch = _save->getTileSearch();
	RNG::shuffle(randomTileSearch);

	const int
		BASE_SYSTEMATIC_SUCCESS	= 100,
		FAST_PASS_THRESHOLD		= 125;

	_attackAction->type = BA_RETHINK;

	Position target;

	int bestScore = 0;

	for (std::vector<Position>::const_iterator
			i = randomTileSearch.begin();
			i != randomTileSearch.end();
			++i)
	{
		const Position pos = _unit->getPosition() + *i;

		const Tile* const tile = _save->getTile(pos);
		if (tile == 0
			|| std::find(
						_reachableWithAttack.begin(),
						_reachableWithAttack.end(),
						_save->getTileIndex(pos)) == _reachableWithAttack.end())
		{
			continue;
		}

		int score = 0;
		// i should really make a function for this
		Position origin = (pos * Position(16, 16, 24))
							+ Position(
									8,
									8,
									_unit->getHeight()
										+ _unit->getFloatHeight()
										- tile->getTerrainLevel()
										- 4);
			// 4 because -2 is eyes and 2 below that is the rifle (or at least that's my understanding)

		if (_save->getTileEngine()->canTargetUnit(
												&origin,
												_aggroTarget->getTile(),
												&target,
												_unit) == true)
		{
			_save->getPathfinding()->calculate(
											_unit,
											pos);

			if (_save->getPathfinding()->getStartDirection() != -1)						// can move here
//				&& _save->getPathfinding()->getTotalTUCost() <= _unit->getTimeUnits())	// can still shoot
			{
				score = BASE_SYSTEMATIC_SUCCESS - getSpottingUnits(pos) * 10;
				score += _unit->getTimeUnits() - _save->getPathfinding()->getTotalTUCost();

				if (_aggroTarget->checkViewSector(pos) == false)
					score += 10;

				if (score > bestScore)
				{
					bestScore = score;

					_attackAction->target = pos;
					_attackAction->finalFacing = _save->getTileEngine()->getDirectionTo(
																					pos,
																					_aggroTarget->getPosition());

					if (score > FAST_PASS_THRESHOLD)
						break;
				}
			}
		}
	}

	if (bestScore > 70)
	{
		//if (_traceAI) Log(LOG_INFO) << "Firepoint found at " << _attackAction->target << ", with a score of: " << bestScore;

		_attackAction->type = BA_WALK;
		return true;
	}

	//if (_traceAI) Log(LOG_INFO) << "Firepoint failed, best estimation was: " << _attackAction->target << ", with a score of: " << bestScore;
	return false;
}

/**
 * Decides if it's worthwhile to create an explosion.
 * @param targetPos	- target's position
 * @param attacker	- pointer to the attacking unit
 * @param radius	- radius of explosion in tile space
 * @param diff		- game difficulty (default -1)
 * @param grenade	- true if explosion will be from a grenade
 * @return, true if it's worthwile creating an explosion at the target position
 */
bool AlienBAIState::explosiveEfficacy(
		Position targetPos,
		BattleUnit* attacker,
		int radius,
		int diff) const
//		bool grenade) const
{
	//Log(LOG_INFO) << "AlienBAIState::explosiveEfficacy()";
	if (diff == -1)
		diff = static_cast<int>(_save->getBattleState()->getGame()->getSavedGame()->getDifficulty());

	// i hate the player and i want him dead, but i don't want to piss him off
	if (_save->getTurn() < 5 - diff)
		return false;


	// if attacker is hurt, assume things are dire and increase desperation
	const int
		desperation = (100 - attacker->getMorale()) / 10,
		hurt = 10 - static_cast<int>(
					static_cast<double>(attacker->getHealth()) / static_cast<double>(attacker->getBaseStats()->health) * 10.),
		// but don't go kamikaze unless we're already doomed
		distance = _save->getTileEngine()->distance(
												attacker->getPosition(),
												targetPos);
	int effect = (desperation + hurt) * 3;

	if (distance <= radius) // attacker inside blast zone
	{
//		effect -= 35;
		effect -= (radius - distance + 1) * 5;

		if (std::abs(attacker->getPosition().z - targetPos.z) <= Options::battleExplosionHeight)
			effect -= 15;
	}

	if (_save->getMissionType() == "STR_ALIEN_BASE_ASSAULT")
		effect -= 25;
	else if (_save->getMissionType() == "STR_BASE_DEFENSE"
		|| _save->getMissionType() == "STR_TERROR_MISSION")
	{
		effect += 50;
	}

	// allow difficulty to have an influence
	effect += diff;

	// account for the targeted unit -> excludeUnit when calculatingLine below.
	const BattleUnit* const target = _save->getTile(targetPos)->getUnit();

	for (std::vector<BattleUnit*>::const_iterator
			i = _save->getUnits()->begin();
			i != _save->getUnits()->end();
			++i)
	{
		if ((*i)->isOut(true) == false
			&& *i != attacker
			&& std::abs((*i)->getPosition().z - targetPos.z) <= Options::battleExplosionHeight
			&& _save->getTileEngine()->distance(
											targetPos,
											(*i)->getPosition()) <= radius)
		{
			if (((*i)->getTile()
					&& (*i)->getTile()->getDangerous())		// don't count people who were already grenaded this turn
				|| ((*i)->getFaction() == FACTION_PLAYER	// don't count units we don't know about
					&& (*i)->getTurnsExposed() > _intelligence))
			{
				continue;
			}

			// trace a line from the grenade origin to targetPos
			const Position
				voxelPosA = Position(
								(targetPos.x * 16) + 8,
								(targetPos.y * 16) + 8,
								(targetPos.z * 24) + 12),
				voxelPosB = Position(
								((*i)->getPosition().x * 16) + 8,
								((*i)->getPosition().y * 16) + 8,
								((*i)->getPosition().z * 24) + 12);

			std::vector<Position> traj;
			const int collision = _save->getTileEngine()->calculateLine(
																	voxelPosA,
																	voxelPosB,
																	false,
																	&traj,
																	target,
																	true,
																	false,
																	*i);
			if (collision == VOXEL_UNIT
				&& traj.front() / Position(16, 16, 24) == (*i)->getPosition())
			{
//				if ((*i)->getFaction() == FACTION_PLAYER)
				if ((*i)->getFaction() != attacker->getFaction()) // do xCom, Mc'd aLiens, & civies.
					effect += 10;

				if ((*i)->getOriginalFaction() == attacker->getFaction()) // but true friendlies count negative-half
					effect -= 5;
			}
		}
	}

	if (RNG::percent(effect) == true)
	{
		//Log(LOG_INFO) << "AlienBAIState::explosiveEfficacy() EXIT true, effect = " << effect;
		return true;
	}

	//Log(LOG_INFO) << "AlienBAIState::explosiveEfficacy() EXIT false, effect = " << effect;
	return false;
}

/**
 * Attempts to take a melee attack/charge an enemy we can see.
 * Melee targetting: we can see an enemy, we can move to it so we're charging blindly toward an enemy.
 */
void AlienBAIState::meleeAction()
{
	if (_aggroTarget != NULL
		&& _aggroTarget->isOut() == false) // (true, true)
	{
		if (_save->getTileEngine()->validMeleeRange(
												_unit,
												_aggroTarget,
												_save->getTileEngine()->getDirectionTo(
																					_unit->getPosition(),
																					_aggroTarget->getPosition())) == true)
		{
			meleeAttack();
			return;
		}
	}

	const int
		attackCost = _unit->getActionTUs(
										BA_HIT,
										_unit->getMainHandWeapon()),
		chargeReserve = _unit->getTimeUnits() - attackCost;
	int distance = (chargeReserve / 4) + 1;

	_aggroTarget = NULL;

	for (std::vector<BattleUnit*>::const_iterator
			i = _save->getUnits()->begin();
			i != _save->getUnits()->end();
			++i)
	{
		const int newDistance = _save->getTileEngine()->distance(
															_unit->getPosition(),
															(*i)->getPosition());
		if (newDistance > 20
			|| validTarget(
						*i,
						true,
						true) == false)
		{
			continue;
		}

		// pick closest living unit that we can move to
		if ((newDistance < distance
				|| newDistance == 1)
			&& (*i)->isOut() == false)
		{
			if (newDistance == 1
				|| selectPointNearTarget(
									*i,
									chargeReserve) == true)
			{
				_aggroTarget = (*i);
				_attackAction->type = BA_WALK;
				_unit->setCharging(_aggroTarget);

				distance = newDistance;
			}
		}
	}

	if (_aggroTarget != NULL)
	{
		if (_save->getTileEngine()->validMeleeRange(
												_unit,
												_aggroTarget,
												_save->getTileEngine()->getDirectionTo(_unit->getPosition(),
												_aggroTarget->getPosition())) == true)
		{
			meleeAttack();
		}
	}

/*	if (_traceAI && _aggroTarget)
	{
		Log(LOG_INFO) << "AlienBAIState::meleeAction:"
				<< " [target]: " << (_aggroTarget->getId())
				<< " at: "  << _attackAction->target;
		Log(LOG_INFO) << "CHARGE!";
	} */
}

/**
 * Attempts to fire a waypoint projectile at an enemy that is seen (by any aLien).
 * Waypoint targeting: pick from any units currently spotted by our aLiens.
 */
void AlienBAIState::wayPointAction()
{
	_aggroTarget = NULL;

	for (std::vector<BattleUnit*>::const_iterator
			i = _save->getUnits()->begin();
			i != _save->getUnits()->end()
				&& _aggroTarget == NULL;
			++i)
	{
		if (validTarget(
						*i,
						true,
						true) == false)
		{
			continue;
		}

		_save->getPathfinding()->calculate(
										_unit,
										(*i)->getPosition(),
										*i,
										-1);

		const int radius = _unit->getMainHandWeapon()->getAmmoItem()->getRules()->getExplosionRadius(); // kL
		if (_save->getPathfinding()->getStartDirection() != -1
			&& explosiveEfficacy(
							(*i)->getPosition(),
							_unit,
//							(_unit->getMainHandWeapon()->getAmmoItem()->getRules()->getPower() / 20) + 1,
							radius, // kL
							_attackAction->diff) == true)
		{
			_aggroTarget = *i;
		}

		_save->getPathfinding()->abortPath();
	}


	if (_aggroTarget != NULL)
	{
		_attackAction->type = BA_LAUNCH;
		_attackAction->TU = _unit->getActionTUs(
											BA_LAUNCH,
											_attackAction->weapon);
		if (_attackAction->TU > _unit->getTimeUnits())
		{
			_attackAction->type = BA_RETHINK;
			return;
		}

		_attackAction->waypoints.clear();

		_save->getPathfinding()->calculate(
										_unit,
										_aggroTarget->getPosition(),
										_aggroTarget,
										-1);

		Position
			posEnd = _unit->getPosition(),
			posLast = _unit->getPosition(),
			posCur = _unit->getPosition(),
			dirVect;

		int
			collision,
			dirPath = _save->getPathfinding()->dequeuePath();

		while (dirPath != -1)
		{
			posLast = posCur;

			_save->getPathfinding()->directionToVector(
													dirPath,
													&dirVect);

			posCur += dirVect;

			const Position
				voxelPosA(
						(posCur.x * 16) + 8,
						(posCur.y * 16) + 8,
						(posCur.z * 24) + 16),
				voxelPosB(
						(posEnd.x * 16) + 8,
						(posEnd.y * 16) + 8,
						(posEnd.z * 24) + 16);

			collision = _save->getTileEngine()->calculateLine(
															voxelPosA,
															voxelPosB,
															false,
															NULL,
															_unit);

			if (VOXEL_EMPTY < collision && collision < VOXEL_UNIT)
			{
				_attackAction->waypoints.push_back(posLast);
				posEnd = posLast;
			}
			else if (collision == VOXEL_UNIT)
			{
				if (_save->getTile(posCur)->getUnit() == _aggroTarget)
				{
					_attackAction->waypoints.push_back(posCur);
					posEnd = posCur;
				}
			}

			dirPath = _save->getPathfinding()->dequeuePath();
		}

		_attackAction->target = _attackAction->waypoints.front();

		if (static_cast<int>(_attackAction->waypoints.size()) > (_attackAction->diff * 2) + 6
			|| posEnd != _aggroTarget->getPosition())
		{
			_attackAction->type = BA_RETHINK;
		}
	}
}

/**
 * Attempts to fire at an enemy we can see.
 * Regular targeting: we can see an enemy, we have a gun, let's try to shoot.
 */
void AlienBAIState::projectileAction()
{
	_attackAction->target = _aggroTarget->getPosition();

	if (_attackAction->weapon->getAmmoItem()->getRules()->getExplosionRadius() < 0
		|| explosiveEfficacy(
						_aggroTarget->getPosition(),
						_unit,
						_attackAction->weapon->getAmmoItem()->getRules()->getExplosionRadius(),
						_attackAction->diff) == true)
	{
		selectFireMethod();
	}
}

/**
 * Selects a fire method based on range, time units, and time units reserved for cover.
 * kL_note: Question: what should this set for BA_* if no FireMethod condition checks TRUE...?
 *			I'll leave it at BA_RETHINK for now......
 */
void AlienBAIState::selectFireMethod()
{
	_attackAction->type = BA_RETHINK;

	int usableTU = _unit->getTimeUnits();// - _escapeTUs;
	if (_unit->getAggression() < RNG::generate(0, 3)) // kL
		usableTU -= _escapeTUs;

	const int distance = _save->getTileEngine()->distance(
													_unit->getPosition(),
													_attackAction->target);
	if (distance <= _attackAction->weapon->getRules()->getAutoRange())
	{
		if (_attackAction->weapon->getRules()->getTUAuto() != 0
			&& usableTU >= _unit->getActionTUs(
											BA_AUTOSHOT,
											_attackAction->weapon))
		{
			_attackAction->type = BA_AUTOSHOT;
		}
		else if (_attackAction->weapon->getRules()->getTUSnap() != 0
			&& usableTU >= _unit->getActionTUs(
											BA_SNAPSHOT,
											_attackAction->weapon))
		{
			_attackAction->type = BA_SNAPSHOT;
		}
		else if (_attackAction->weapon->getRules()->getTUAimed() != 0
			&& usableTU >= _unit->getActionTUs(
											BA_AIMEDSHOT,
											_attackAction->weapon))
		{
			_attackAction->type = BA_AIMEDSHOT;
		}
	}
	else if (distance <= _attackAction->weapon->getRules()->getSnapRange())
	{
		if (_attackAction->weapon->getRules()->getTUSnap() != 0
			&& usableTU >= _unit->getActionTUs(
											BA_SNAPSHOT,
											_attackAction->weapon))
		{
			_attackAction->type = BA_SNAPSHOT;
		}
		else if (_attackAction->weapon->getRules()->getTUAimed() != 0
			&& usableTU >= _unit->getActionTUs(
											BA_AIMEDSHOT,
											_attackAction->weapon))
		{
			_attackAction->type = BA_AIMEDSHOT;
		}
		else if (_attackAction->weapon->getRules()->getTUAuto() != 0
			&& usableTU >= _unit->getActionTUs(
											BA_AUTOSHOT,
											_attackAction->weapon))
		{
			_attackAction->type = BA_AUTOSHOT;
		}
	}
	else if (distance <= _attackAction->weapon->getRules()->getAimRange())
	{
		if (_attackAction->weapon->getRules()->getTUAimed() != 0
			&& usableTU >= _unit->getActionTUs(
											BA_AIMEDSHOT,
											_attackAction->weapon))
		{
			_attackAction->type = BA_AIMEDSHOT;
		}
		else if (_attackAction->weapon->getRules()->getTUSnap() != 0
			&& usableTU >= _unit->getActionTUs(
											BA_SNAPSHOT,
											_attackAction->weapon))
		{
			_attackAction->type = BA_SNAPSHOT;
		}
		else if (_attackAction->weapon->getRules()->getTUAuto() != 0
			&& usableTU >= _unit->getActionTUs(
											BA_AUTOSHOT,
											_attackAction->weapon))
		{
			_attackAction->type = BA_AUTOSHOT;
		}
	}
}

/**
 * Evaluates whether to throw a grenade at a seen enemy or group of enemies.
 */
void AlienBAIState::grenadeAction()
{
	// do we have a grenade on our belt?
	// kL_note: this is already checked in setupAttack()
	// Could use it to determine if grenade is already inHand though! ( see _grenade var.)
	const BattleItem* const grenade = _unit->getGrenade();

	if (grenade == NULL)
		return;

	// distance must be more than X tiles, otherwise it's too dangerous to explode
	if (explosiveEfficacy(
						_aggroTarget->getPosition(),
						_unit,
						grenade->getRules()->getExplosionRadius(),
						_attackAction->diff) == true)
//						true))
	{
//		if (_unit->getFaction() == FACTION_HOSTILE)
//		{
		const RuleInventory* const rule = grenade->getSlot();
		int costTU = rule->getCost(_save->getBattleState()->getGame()->getRuleset()->getInventory("STR_RIGHT_HAND"));

		if (grenade->getFuseTimer() == -1)
			costTU += _unit->getActionTUs(
										BA_PRIME,
										grenade);

		costTU += _unit->getActionTUs(
									BA_THROW,
									grenade);

		if (costTU <= _unit->getTimeUnits())
		{
			BattleAction action;
			action.actor = _unit;
			action.target = _aggroTarget->getPosition();
			action.weapon = const_cast<BattleItem*>(grenade);
			action.type = BA_THROW;

			const Position
				originVoxel = _save->getTileEngine()->getOriginVoxel(
																action,
																NULL),
				targetVoxel = action.target * Position(16, 16, 24)
							+ Position(
									8,
									8,
									2 - _save->getTile(action.target)->getTerrainLevel());

			if (_save->getTileEngine()->validateThrow(
													action,
													originVoxel,
													targetVoxel) == true)
			{
				_attackAction->target = action.target;
				_attackAction->weapon = const_cast<BattleItem*>(grenade);
				_attackAction->type = BA_THROW;

				_rifle = false;
				_melee = false;
			}
		}
//		}
	}
}

/**
 * Evaluates a psionic attack on an 'exposed' enemy.
 * Psionic targetting: pick from any of the exposed units.
 * Exposed means they have been previously spotted, and are therefore known to
 * the AI, regardless of whether we can see them or not, because we're psycho.
 * @return, true if a psionic attack should be performed
 */
bool AlienBAIState::psiAction()
{
	//Log(LOG_INFO) << "AlienBAIState::psiAction()";
	if (_didPsi == true										// didn't already do a psi action this round
		|| _unit->getOriginalFaction() != FACTION_HOSTILE)	// don't let any faction but HOSTILE mind-control others.
	{
		return false;
	}

	const RuleItem* const itemRule = _save->getBattleGame()->getRuleset()->getItem("ALIEN_PSI_WEAPON");

	int tuCost = itemRule->getTUUse();
	if (itemRule->getFlatRate() == false)
		tuCost = static_cast<int>(floor(static_cast<float>(_unit->getBaseStats()->tu * tuCost) / 100.f));
	//Log(LOG_INFO) << "AlienBAIState::psiAction() tuCost = " << tuCost;

	if (_unit->getTimeUnits() < tuCost + _escapeTUs) // check if aLien has the required TUs and can still make it to cover
	{
		//Log(LOG_INFO) << ". not enough Tu, EXIT";
		return false;
	}
	else // do it -> further evaluation req'd.
	{
		const bool LOSRequired = itemRule->isLOSRequired();
		const int
			LoS_factor = 50, // increase chance of attack against a unit that is currently in LoS.
			attackStr = static_cast<int>(std::floor(
						static_cast<double>(_unit->getBaseStats()->psiStrength * _unit->getBaseStats()->psiSkill) / 50.));
		//Log(LOG_INFO) << ". . attackStr = " << attackStr << " ID = " << _unit->getId();

		bool hasLoS = false;
		int
			chance = 0,
			chance2 = 0;

		_aggroTarget = NULL;

		for (std::vector<BattleUnit*>::const_iterator
				i = _save->getUnits()->begin();
				i != _save->getUnits()->end();
				++i)
		{
//			if ((*i)->getUnitRules() != NULL)
			// NOTE: Should use isFearable() for doggies ....
//			if ((*i)->getGeoscapeSoldier() == NULL)
//				continue;

			if ((*i)->getGeoscapeSoldier() != NULL // what about doggies ....
				&& validTarget( // will check for Mc, Exposed, etc.
						*i,
						true,
						false) == true
				&& (LOSRequired == false
					|| std::find(
							_unit->getVisibleUnits()->begin(),
							_unit->getVisibleUnits()->end(),
							*i) != _unit->getVisibleUnits()->end()))
			{
				hasLoS = false;

				// is this gonna crash..................
				Position
					target,
					origin = _save->getTileEngine()->getSightOriginVoxel(_unit);
				const int
					LoS = static_cast<int>(_save->getTileEngine()->canTargetUnit(
																			&origin,
																			(*i)->getTile(),
																			&target,
																			_unit)) * LoS_factor,
					// stupid aLiens don't know soldier's psiSkill tho..
					// psiSkill would typically factor in at only a fifth of psiStrength.
					defense = (*i)->getBaseStats()->psiStrength,
					dist = _save->getTileEngine()->distance(
														(*i)->getPosition(),
														_unit->getPosition()) * 2,
					rand = RNG::generate(1, 30);

				//Log(LOG_INFO) << ". . . ";
				//Log(LOG_INFO) << ". . . targetID = " << (*i)->getId();
				//Log(LOG_INFO) << ". . . defense = " << defense;
				//Log(LOG_INFO) << ". . . dist = " << dist;
				//Log(LOG_INFO) << ". . . rand = " << rand;
				//Log(LOG_INFO) << ". . . LoS = " << LoS;

				chance2 = attackStr // NOTE that this is NOT a true calculation of Success chance!
						- defense
						- dist
						+ rand
						+ LoS;
				//Log(LOG_INFO) << ". . . chance2 = " << chance2;

				if (chance2 == chance
					&& (RNG::percent(50) == true
						|| _aggroTarget == NULL))
				{
					hasLoS = (LoS > 0);
					_aggroTarget = *i;
				}
				else if (chance2 > chance)
				{
					chance = chance2;

					hasLoS = (LoS > 0);
					_aggroTarget = *i;
				}
			}
		}

		if (hasLoS == true)
			chance -= LoS_factor;

		if (_aggroTarget == NULL			// if no target
			|| chance < 26					// or chance of success too low
			|| RNG::percent(13) == true)	// or aLien just don't feel like it... do FALSE.
		{
			//Log(LOG_INFO) << "AlienBAIState::psiAction() EXIT, False : not good.";
			return false;
		}

/*kL
		if (_visibleEnemies
			&& _attackAction->weapon
			&& _attackAction->weapon->getAmmoItem())
		{
			if (_attackAction->weapon->getAmmoItem()->getRules()->getPower() > chance)
			{
				//Log(LOG_INFO) << "AlienBAIState::psiAction() EXIT 2, False";
				return false;
			}
		}
		else if (RNG::generate(35, 155) > chance)
		{
			//Log(LOG_INFO) << "AlienBAIState::psiAction() EXIT 3, False";
			return false;
		} */

		//if (_traceAI) Log(LOG_INFO) << "making a psionic attack this turn";

		const int morale = _aggroTarget->getMorale();
		if (morale > 0)		// panicAtk is valid since target has morale to chew away
//			&& chance < 30)	// esp. when aLien atkStr is low
		{
			//Log(LOG_INFO) << ". . test if MC or Panic";
			const int bravery = _aggroTarget->getBaseStats()->bravery;
			int panicOdds = 110 - bravery; // ie, moraleHit
			const int moraleResult = morale - panicOdds;
			//Log(LOG_INFO) << ". . panicOdds_1 = " << panicOdds;

			if (moraleResult < 0)
				panicOdds -= bravery / 2;
			else if (moraleResult < 50)
				panicOdds -= bravery;
			else
				panicOdds -= bravery * 2;

			//Log(LOG_INFO) << ". . panicOdds_2 = " << panicOdds;
			panicOdds += (RNG::generate(51, 100) - (attackStr / 5));
			//Log(LOG_INFO) << ". . panicOdds_3 = " << panicOdds;
			if (RNG::percent(panicOdds) == true)
			{
				_psiAction->target	= _aggroTarget->getPosition();
				_psiAction->type	= BA_PANIC;

				//Log(LOG_INFO) << "AlienBAIState::psiAction() EXIT . do Panic vs " << _aggroTarget->getId();
				return true;
			}
		}

		_psiAction->target	= _aggroTarget->getPosition();
		_psiAction->type	= BA_MINDCONTROL;

		//Log(LOG_INFO) << "AlienBAIState::psiAction() EXIT . do MindControl vs " << _aggroTarget->getId();
		return true;
	}

	//Log(LOG_INFO) << "AlienBAIState::psiAction() EXIT, False";
	return false;
}

/**
 * Performs a melee attack action.
 */
void AlienBAIState::meleeAttack()
{
	_unit->lookAt(
			_aggroTarget->getPosition() + Position(
												_unit->getArmor()->getSize() - 1,
												_unit->getArmor()->getSize() - 1,
												0));

	while (_unit->getStatus() == STATUS_TURNING)
		_unit->turn();

	//if (_traceAI) Log(LOG_INFO) << "Attack unit: " << _aggroTarget->getId();

	_attackAction->target	= _aggroTarget->getPosition();
	_attackAction->type		= BA_HIT;
}

/**
 * Validates a target.
 * @param unit			- pointer to a target to validate
 * @param assessDanger	- true to care if target has already been grenaded (default false)
 * @param includeCivs	- true to include civilians in the threat assessment (default false)
 * @return, true if the target is something to be killed
 */
bool AlienBAIState::validTarget(
		const BattleUnit* const unit,
		bool assessDanger,
		bool includeCivs) const
{
	if (unit->getFaction() == FACTION_HOSTILE				// they mustn't be on our side
		|| unit->isOut(true, true) == true					// ignore units that are dead/unconscious
		|| _intelligence < unit->getTurnsExposed()			// they must be units that we "know" about
		|| (assessDanger == true
			&& unit->getTile()->getDangerous() == true))	// they haven't been grenaded
	{
		return false;
	}

	if (includeCivs == true)
		return true;

	return (unit->getFaction() == FACTION_PLAYER);
}

/**
 * Checks the alien's reservation setting.
 * @return the reserve setting.
 */
BattleActionType AlienBAIState::getReserveMode()
{
	return _reserve;
}

/**
 * He has a dichotomy on his hands: it has a ranged weapon as well as melee ability
 * ... so, make a determination on which one to use this round.
 */
void AlienBAIState::selectMeleeOrRanged()
{
	const BattleItem* const mainWeapon = _unit->getMainHandWeapon();
	if (mainWeapon == NULL) // kL safety.
	{
		_rifle = false;
		return;
	}

	const RuleItem* const rifleWeapon = mainWeapon->getRules();
	if (rifleWeapon->getBattleType() != BT_FIREARM) // kL_add.
//		|| rifleWeapon == NULL)
//		|| _unit->getMainHandWeapon()->getAmmoItem() == NULL) // done in getMainHandWeapon()
	{
		_rifle = false;
		return;
	}

	const RuleItem* const meleeWeapon = _save->getBattleGame()->getRuleset()->getItem(_unit->getMeleeWeapon());
	if (meleeWeapon == NULL)
	{
		// no idea how we got here, but melee is definitely out of the question.
		_melee = false;
		return;
	}

	int meleeOdds = 10;

	int power = meleeWeapon->getPower();
	if (meleeWeapon->isStrengthApplied() == true)
		power += static_cast<int>(Round(
				 static_cast<double>(_unit->getBaseStats()->strength) * (_unit->getAccuracyModifier() / 2. + 0.5)));

	power = static_cast<int>(Round(
			static_cast<float>(power) * _aggroTarget->getArmor()->getDamageModifier(meleeWeapon->getDamageType())));

	if (power > 50)
		meleeOdds += (power - 50) / 2;

	if (_visibleEnemies > 1)
		meleeOdds -= 15 * (_visibleEnemies - 1);

	if (meleeOdds > 0
		&& _unit->getHealth() > _unit->getBaseStats()->health * 2 / 3)
	{
/*		if (_unit->getAggression() == 0)
			meleeOdds -= 20;
		else if (_unit->getAggression() > 1)
			meleeOdds += 10 * _unit->getAggression(); */
		meleeOdds += 10 * _unit->getAggression(); // kL

		if (RNG::percent(meleeOdds) == true)
		{
			_rifle = false;
			const int preShotTU = _unit->getTimeUnits() - _unit->getActionTUs(
																		BA_HIT,
																		meleeWeapon);
			_reachableWithAttack = _save->getPathfinding()->findReachable(
																	_unit,
																	preShotTU);
			return;
		}
	}

	_melee = false;
}

}
