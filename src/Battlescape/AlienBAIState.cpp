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

#include "AlienBAIState.h"

//#include <algorithm>
//#define _USE_MATH_DEFINES
//#include <climits>
//#include <cmath>

#include "../Battlescape/BattlescapeState.h"
#include "../Battlescape/Map.h"
#include "../Battlescape/Pathfinding.h"
#include "../Battlescape/TileEngine.h"

#include "../Engine/Game.h"

#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/RuleInventory.h" // grenade from Belt to Hand
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
 * @param battleSave	- pointer to SavedBattleGame
 * @param unit			- pointer to the BattleUnit
 * @param node			- pointer to the Node the unit originates from
 */
AlienBAIState::AlienBAIState(
		SavedBattleGame* battleSave,
		BattleUnit* unit,
		Node* node)
	:
		BattleAIState(
			battleSave,
			unit),
		_aggroTarget(NULL),
		_targetsKnown(0),
		_targetsVisible(0),
		_xcomSpotters(0),
		_tuEscape(0),
		_tuAmbush(0),
//kL	_reserveTUs(0),
		_rifle(false),
		_melee(false),
		_blaster(false),
		_grenade(false), // kL
		_didPsi(false),
		_AIMode(AI_PATROL),
		_closestDist(100),
		_fromNode(node),
		_toNode(NULL),
		_reserve(BA_NONE),
		_intelligence(unit->getIntelligence())
{
	//Log(LOG_INFO) << "Create AlienBAIState";
//	_traceAI = Options::traceAI; // kL_note: Can take this out of options & configs.

	_escapeAction	= new BattleAction();
	_ambushAction	= new BattleAction();
	_attackAction	= new BattleAction();
	_patrolAction	= new BattleAction();
	_psiAction		= new BattleAction();
	//Log(LOG_INFO) << "Create AlienBAIState EXIT";
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
		_fromNode = _battleSave->getNodes()->at(fromNodeID);

	if (toNodeID != -1)
		_toNode = _battleSave->getNodes()->at(toNodeID);
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

	if (_fromNode != NULL)	fromNodeID	= _fromNode->getId();
	if (_toNode != NULL)	toNodeID	= _toNode->getId();

	YAML::Node node;

	node["fromNode"]	= fromNodeID;
	node["toNode"]		= toNodeID;
	node["AIMode"]		= _AIMode;
//	node["wasHitBy"]	= _wasHitBy;

	return node;
}

/**
 * Enters the current AI state.
 */
/* void AlienBAIState::enter()
{
	Log(LOG_INFO) << "AlienBAIState::enter() ROOOAARR !";
} */

/**
 * Exits the current AI state.
 */
/* void AlienBAIState::exit()
{
	Log(LOG_INFO) << "AlienBAIState::exit()";
} */

/**
 * Runs any code the state needs to keep updating every AI cycle.
 * @param action - pointer to AI BattleAction to execute
 */
void AlienBAIState::think(BattleAction* action)
{
	//Log(LOG_INFO) << "\n";
	//Log(LOG_INFO) << "AlienBAIState::think(), unitID = " << _unit->getId() << " pos " << _unit->getPosition();
 	action->type = BA_RETHINK;
	action->actor = _unit;
//	action->weapon = _unit->getMainHandWeapon();		// kL_note: does not return grenades.
														// *cough* Does return grenades. -> DID return grenades but i fixed it so it won't.
	action->weapon = _unit->getMainHandWeapon(false);	// <- this switches BL/Pistol aLiens to use BL (when not reaction firing)
														// It needs to be refined to account TU usage, also.

	if (action->weapon == NULL // kL ->
		&& _unit->getUnitRules() != NULL
		&& _unit->getUnitRules()->getMeleeWeapon() == "STR_FIST")
	{
		action->weapon = _battleSave->getBattleGame()->getFist();
	}

	_attackAction->diff = static_cast<int>(_battleSave->getBattleState()->getGame()->getSavedGame()->getDifficulty());
	_attackAction->actor = _unit;
	_attackAction->weapon = action->weapon;
	_attackAction->number = action->number;

	_escapeAction->number = action->number;

	_xcomSpotters = countSpottingUnits(_unit->getPosition());
	_targetsKnown = countKnownTargets();
	_targetsVisible = selectNearestTarget();

//	_melee = _unit->getMeleeWeapon() != NULL;
	_melee = (_unit->getMeleeWeapon().empty() == false); // kL_note: or STR_FIST
	_rifle = false;
	_blaster = false;
	_grenade = false; // kL

	Pathfinding* const pf = _battleSave->getPathfinding();
	pf->setPathingUnit(_unit);
	_reachable = pf->findReachable(
								_unit,
								_unit->getTimeUnits());
//	_wasHitBy.clear();

	if (_unit->getChargeTarget() != NULL
		&& _unit->getChargeTarget()->isOut_t(OUT_STAT) == true)
//		&& _unit->getChargeTarget()->isOut(true, true) == true)
	{
		_unit->setChargeTarget(NULL);
	}

	// debug:
	//Log(LOG_INFO) << "_xcomSpotters = " << _xcomSpotters;
	//Log(LOG_INFO) << "_targetsKnown = " << _targetsKnown;
	//Log(LOG_INFO) << "_targetsVisible = " << _targetsVisible;
/*	std::string AIMode;
	switch (_AIMode)
	{
		case 0: AIMode = "Patrol"; break;
		case 1: AIMode = "Ambush"; break;
		case 2: AIMode = "Combat"; break;
		case 3: AIMode = "Escape";
	} */
	//Log(LOG_INFO) << "AIMode = " << AIMode;
	// debug_End.

	//Log(LOG_INFO) << ". . pos 1";
	if (action->weapon != NULL)
	{
		const RuleItem* const itRule = action->weapon->getRules();
		//Log(LOG_INFO) << ". weapon " << itRule->getType();
//		if (itRule->isWaterOnly() == false
//			|| _battleSave->getDepth() != 0)
//		{
		if (itRule->getBattleType() == BT_FIREARM)
		{
			//Log(LOG_INFO) << ". . weapon is Firearm";
			if (itRule->isWaypoints() != 0
				&& _targetsKnown > _targetsVisible) // else let BL fallback to aimed shot
			{
				//Log(LOG_INFO) << ". . . blaster TRUE";
				_blaster = true;
				const int tuPreshot = _unit->getTimeUnits() - _unit->getActionTu(
																			BA_LAUNCH,
																			action->weapon);
				_reachableAttack = pf->findReachable(
												_unit,
												tuPreshot);
			}
			else
			{
				//Log(LOG_INFO) << ". . . rifle TRUE";
				_rifle = true;
				const int tuPreshot = _unit->getTimeUnits() - _unit->getActionTu( // kL_note: this needs selectFireMethod() ...
																			itRule->getDefaultAction(), // BA_SNAPSHOT
																			action->weapon);
				_reachableAttack = pf->findReachable(
												_unit,
												tuPreshot);
			}
		}
		else if (itRule->getBattleType() == BT_MELEE)
		{
			//Log(LOG_INFO) << ". . weapon is Melee";
			_melee = true;
			const int tuPreshot = _unit->getTimeUnits() - _unit->getActionTu(
																		BA_HIT,
																		action->weapon);
			_reachableAttack = pf->findReachable(
											_unit,
											tuPreshot);
		}
		else if (itRule->getBattleType() == BT_GRENADE)	// kL
		{
			//Log(LOG_INFO) << ". . weapon is Grenade";
			_grenade = true;							// kL, this is no longer useful since I fixed
														// getMainHandWeapon() to not return grenades.
		}
//		}
//		else
//		{
			//Log(LOG_INFO) << ". . weapon is NULL [1]";
//			action->weapon = NULL;
//		}
	}
	//else Log(LOG_INFO) << ". . weapon is NULL [2]";
//	else if () // kL_add -> Give the invisible 'meleeWeapon' param a try ....
//	{}


	//Log(LOG_INFO) << ". . pos 2";
	if (_xcomSpotters > 0
		&& _tuEscape == 0)
	{
		setupEscape();
	}

	//Log(LOG_INFO) << ". . pos 3";
	if (_targetsKnown != 0
		&& _melee == false
		&& _tuAmbush == 0)
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

		action->type = _psiAction->type;
		action->target = _psiAction->target;

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
		if (_xcomSpotters == 0
			|| _targetsKnown == 0)
		{
			evaluate = true;
		}
	}
	else if (_AIMode == AI_PATROL)
	{
		if (_xcomSpotters > 0
			|| _targetsVisible > 0
			|| _targetsKnown > 0
			|| RNG::percent(10) == true)
		{
			evaluate = true;
		}
	}
	else if (_AIMode == AI_AMBUSH)
	{
		if (_rifle == false
			|| _tuAmbush == 0
			|| _targetsVisible != 0)
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

	if (_xcomSpotters > 2
		|| _unit->getHealth() < _unit->getBaseStats()->health * 2 / 3
		|| (_aggroTarget != NULL
			&& _aggroTarget->getExposed() != -1
			&& _aggroTarget->getExposed() > _intelligence))
	{
		evaluate = true;
	}

	if (_battleSave->isCheating() == true
		&& _AIMode != AI_COMBAT)
	{
		evaluate = true;
	}

	//Log(LOG_INFO) << ". . pos 7";
	if (evaluate == true)
	{
		// debug:
//		std::string AIMode;
/*		switch (_AIMode)
		{
			case 0: AIMode = "Patrol"; break;
			case 1: AIMode = "Ambush"; break;
			case 2: AIMode = "Combat"; break;
			case 3: AIMode = "Escape"; break;
		} */
		//Log(LOG_INFO) << ". AIMode pre-reEvaluate = " << AIMode;
		// debug_End.

		evaluateAIMode();

		// debug:
/*		switch (_AIMode)
		{
			case 0: AIMode = "Patrol"; break;
			case 1: AIMode = "Ambush"; break;
			case 2: AIMode = "Combat"; break;
			case 3: AIMode = "Escape"; break;
		} */
		//Log(LOG_INFO) << ". AIMode post-reEvaluate = " << AIMode;
		// debug_End.
	}

	//Log(LOG_INFO) << ". . pos 8";
	_reserve = BA_NONE;

	switch (_AIMode)
	{
		case AI_ESCAPE:
			//Log(LOG_INFO) << ". . . . AI_ESCAPE";
			_unit->setChargeTarget(NULL);

			action->type = _escapeAction->type;
			action->target = _escapeAction->target;
			action->finalAction = true;		// end this unit's turn.
			action->desperate = true;		// ignore new targets.

			_unit->setHiding(true);			// spin 180 at the end of route.

			// forget about reserving TUs, we need to get out of here.
//			_battleSave->getBattleGame()->setReservedAction(BA_NONE, false); // kL
		break;

		case AI_PATROL:
			//Log(LOG_INFO) << ". . . . AI_PATROL";
			_unit->setChargeTarget(NULL);

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
			action->type = _attackAction->type;
			action->target = _attackAction->target;
			action->weapon = _attackAction->weapon; // this may have changed to a grenade. Or an innate meleeWeapon ...

			if (action->weapon != NULL
				&& action->weapon->getRules()->getBattleType() == BT_GRENADE
				&& action->type == BA_THROW)
			{
				const RuleInventory* const rule = action->weapon->getSlot();
				int costTU = rule->getCost(_battleSave->getBattleGame()->getRuleset()->getInventory("STR_RIGHT_HAND"));

				if (action->weapon->getFuse() == -1)
				{
					costTU += _unit->getActionTu(
												BA_PRIME,
												action->weapon);
				}

				_unit->spendTimeUnits(costTU); // cf. grenadeAction() -- actually priming the fuse is done in ProjectileFlyBState.
				//Log(LOG_INFO) << "AlienBAIState::think() Move & Prime GRENADE, costTU = " << costTU;
			}

			action->finalFacing = _attackAction->finalFacing;					// if this is a firepoint action, set our facing.
			action->TU = _unit->getActionTu(
										_attackAction->type,
										_attackAction->weapon);

//			_battleSave->getBattleGame()->setReservedAction(BA_NONE, false);	// don't worry about reserving TUs, we've factored that in already.

			if (action->type == BA_MOVE											// if this is a "find fire point" action, don't increment the AI counter.
				&& _rifle == true
				&& _unit->getTimeUnits() > _unit->getActionTu(
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
			_unit->setChargeTarget(NULL);

			action->type = _ambushAction->type;
			action->target = _ambushAction->target;
			action->finalFacing = _ambushAction->finalFacing;	// face where we think our target will appear.
			action->finalAction = true;							// end this unit's turn.
																// factored in the reserved TUs already, so don't worry. Be happy.
	}

	//Log(LOG_INFO) << ". . pos 9";
	if (action->type == BA_MOVE)
	{
		if (action->target != _unit->getPosition()) // if we're moving, we'll have to re-evaluate our escape/ambush position.
		{
			_tuEscape =
			_tuAmbush = 0;
		}
		else
			action->type = BA_NONE;
	}
	//Log(LOG_INFO) << "AlienBAIState::think() EXIT";
}

/**
 * Sets the "was hit" flag to true.
 * @param attacker - pointer to a BattleUnit
 */
/* void AlienBAIState::setWasHitBy(BattleUnit* attacker)
{
	if (attacker->getFaction() != _unit->getFaction() && !getWasHitBy(attacker->getId()))
		_wasHitBy.push_back(attacker->getId());
} */

/**
 * Gets whether the unit was hit.
 * @return, true if unit was hit
 */
/* bool AlienBAIState::getWasHitBy(int attacker) const
{
	return std::find(_wasHitBy.begin(), _wasHitBy.end(), attacker) != _wasHitBy.end();
} */

/**
 * Sets up a patrol action.
 * @note This is mainly going from node to node & moving about the map -
 * handles node selection.
 * @note Fills out the '_patrolAction' with useful data.
 */
void AlienBAIState::setupPatrol() // private.
{
	//Log(LOG_INFO) << "AlienBAIState::setupPatrol()";
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
		const int dir = _battleSave->getTileEngine()->faceWindow(_unit->getPosition());
		if (dir != -1
			&& dir != _unit->getUnitDirection())
		{
			_unit->setDirectionTo(dir);
			while (_unit->getUnitStatus() == STATUS_TURNING)
				_unit->turn();
		}
	}

	if (_fromNode == NULL)
		_fromNode = _battleSave->getNearestNode(_unit);

	Node* node;
	const MapData* data;
	bool scout;

	Pathfinding* const pf = _battleSave->getPathfinding();
	pf->setPathingUnit(_unit);

	int triesLeft = 5;
	while (_toNode == NULL && triesLeft != 0)
	{
		--triesLeft;
		scout = true; // look for a new node to walk towards

		// note: aLiens attacking XCOM Base are always on scout.
		if (_battleSave->getTacType() != TCT_BASEDEFENSE)
		{
			// after turn 20 or if the morale is low, everyone moves out the UFO and scout
			// kL_note: That, above is wrong. Orig behavior depends on "aggression" setting;
			// determines whether aliens come out of UFO to scout/search (attack, actually).
			// also anyone standing in fire should also probably move
			if (_fromNode != NULL
				&& _fromNode->getNodeRank() != NR_SCOUT
				&& (_battleSave->getTile(_unit->getPosition()) == NULL // <- shouldn't be necessary.
					|| _battleSave->getTile(_unit->getPosition())->getFire() == 0)
				&& (_battleSave->isCheating() == false
					|| RNG::percent(_unit->getAggression() * 25) == false))
			{
				scout = false;
			}
		}
		else if (_unit->getArmor()->getSize() == 1)	// in base defense missions the non-large aliens walk towards target nodes - or
		{											// once there shoot objects thereabouts so scan this room for objects to destroy
			if (_fromNode->isTarget() == true
				&& _unit->getMainHandWeapon() != NULL
				&& (_unit->getMainHandWeapon()->getRules()->getAccuracySnap() != 0 // TODO: this ought be expanded to include melee.
					|| _unit->getMainHandWeapon()->getRules()->getAccuracyAuto() != 0
					|| _unit->getMainHandWeapon()->getRules()->getAccuracyAimed() != 0)
				&& _unit->getMainHandWeapon()->getAmmoItem() != NULL
				&& _unit->getMainHandWeapon()->getAmmoItem()->getRules()->getDamageType() != DT_HE
				&& _battleSave->getModuleMap()[_fromNode->getPosition().x / 10][_fromNode->getPosition().y / 10].second > 0)
			{
				const int
					x = (_unit->getPosition().x / 10) * 10,
					y = (_unit->getPosition().y / 10) * 10;

				for (int
						i = x;
						i != x + 9;
						++i)
				{
					for (int
							j = y;
							j != y + 9;
							++j)
					{
						data = _battleSave->getTile(Position(i,j,1))->getMapData(O_OBJECT);
						if (data != NULL && data->isBaseModule() == true)
//							&& data->getDieMCD() && data->getArmor() < 60)
						{
							_patrolAction->actor = _unit;
							_patrolAction->target = Position(i,j,1);
							_patrolAction->weapon = _patrolAction->actor->getMainHandWeapon();
							_patrolAction->type = BA_SNAPSHOT;
							_patrolAction->TU = _patrolAction->actor->getActionTu(
																			_patrolAction->type,
																			_patrolAction->weapon);
							return;
						}
					}
				}
			}
			else
			{
				int // find closest high value target which is not already allocated
					distSqr = 100000,
					distTest;

				for (std::vector<Node*>::const_iterator
						i = _battleSave->getNodes()->begin();
						i != _battleSave->getNodes()->end();
						++i)
				{
					if ((*i)->isTarget() == true && (*i)->isAllocated() == false)
					{
						node = *i;
						distTest = TileEngine::distanceSqr(
														_unit->getPosition(),
														node->getPosition());
						if (_toNode == NULL
							|| (distTest < distSqr && node != _fromNode))
						{
							distSqr = distTest;
							_toNode = node;
						}
					}
				}
			}
		}

		if (_toNode == NULL)
		{
			//Log(LOG_INFO) << ". _toNode == 0 a -> getPatrolNode(scout)";
			_toNode = _battleSave->getPatrolNode(scout, _unit, _fromNode);

			if (_toNode == NULL)
			{
				//Log(LOG_INFO) << ". _toNode == 0 b -> getPatrolNode(!scout)";
				_toNode = _battleSave->getPatrolNode(scout == false, _unit, _fromNode);
			}
		}

		if (_toNode != NULL)
		{
			pf->calculate(_unit, _toNode->getPosition());

//			if (std::find(
//						_reachable.begin(),
//						_reachable.end(),
//						_battleSave->getTileIndex(_toNode->getPosition())) == _reachable.end()) // kL
			if (pf->getStartDirection() == -1)
				_toNode = NULL;

			pf->abortPath(); // kL_note: should this be with {_toNode=0} ?
		}
	}

	if (_toNode != NULL)
	{
		_toNode->allocateNode();

		_patrolAction->actor = _unit;
		_patrolAction->target = _toNode->getPosition();
		_patrolAction->type = BA_MOVE;
	}
	else
		_patrolAction->type = BA_RETHINK;
	//Log(LOG_INFO) << "AlienBAIState::setupPatrol() EXIT";
}

/**
 * Try to set up an ambush action.
 * @note The idea is to check within a 11x11 tile square for a tile which is not
 * seen by the aggroTarget but that can be reached by him/her. Then intuit where
 * AI will see that target first from a covered position and set that as the
 * final facing.
 * @note Fills out the '_ambushAction' with useful data.
 */
void AlienBAIState::setupAmbush() // private.
{
	_ambushAction->type = BA_RETHINK;
	_tuAmbush = 0;

	std::vector<int> targetPath;

	if (selectClosestKnownEnemy() == true)
	{
		const int
			BASE_SUCCESS_SYSTEMATIC	= 100,
			COVER_BONUS				= 25,
			FAST_PASS_THRESHOLD		= 80;

		Position
			originVoxel = _battleSave->getTileEngine()->getSightOriginVoxel(_aggroTarget),
			scanVoxel, // placeholder.
			pos;
		const Tile* tile;
		int bestScore = 0;

		Pathfinding* const pf = _battleSave->getPathfinding();

		for (std::vector<Node*>::const_iterator			// use node positions for this since it gives map makers a good
				i = _battleSave->getNodes()->begin();	// degree of control over how the units will use the environment.
				i != _battleSave->getNodes()->end();	// kL_ TODO: I'd rather not, unless this uses so-called 'OpenNodes' ....
				++i)
		{
			pos = (*i)->getPosition();
			tile = _battleSave->getTile(pos);
			if (tile != NULL
				&& tile->getDangerous() == false
				&& pos.z == _unit->getPosition().z
				&& TileEngine::distance(pos, _unit->getPosition()) < 11
				&& std::find( // ignore unreachable tiles
						_reachableAttack.begin(),
						_reachableAttack.end(),
						_battleSave->getTileIndex(pos)) != _reachableAttack.end())
			{
/*				if (_traceAI) // color all the nodes in range purple.
				{
					tile->setPreviewDir(10);
					tile->setPreviewColor(13);
				} */

				if (countSpottingUnits(pos) == 0
					&& _battleSave->getTileEngine()->canTargetUnit( // make sure Unit can't be seen here.
																&originVoxel,
																tile,
																&scanVoxel,
																_aggroTarget,
																_unit) == false)
				{
					pf->setPathingUnit(_unit);
					pf->calculate(_unit, pos); // make sure Unit can move here

					const int tuAmbush = pf->getTotalTUCost();

					if (pf->getStartDirection() != -1)
//						&& tuAmbush <= _unit->getTimeUnits() - _unit->getActionTu(BA_SNAPSHOT, _attackAction->weapon)) // make sure Unit can still shoot
					{
						int score = BASE_SUCCESS_SYSTEMATIC;
						score -= tuAmbush;

						pf->setPathingUnit(_aggroTarget);
						pf->calculate( // make sure Unit's target can reach here too.
									_aggroTarget,
									pos);

						if (pf->getStartDirection() != -1)
						{
							if (_battleSave->getTileEngine()->faceWindow(pos) != -1)	// ideally get behind some cover,
								score += COVER_BONUS;									// like say a window or low wall.

							if (score > bestScore)
							{
								targetPath = pf->copyPath();

								_ambushAction->target = pos;
								if (pos == _unit->getPosition())
									_tuAmbush = 1;
								else
									_tuAmbush = tuAmbush;

								bestScore = score;
								if (bestScore > FAST_PASS_THRESHOLD)
									break;
							}
						}
					}
				}
			}
		}

		if (bestScore > 0)
		{
			_ambushAction->type = BA_MOVE;

			originVoxel = Position::toVoxelSpaceCentered(
													_ambushAction->target,
													_unit->getHeight(true)
														- _battleSave->getTile(_ambushAction->target)->getTerrainLevel()
														- 4);
			Position posNext;

			pf->setPathingUnit(_aggroTarget);
			pos = _aggroTarget->getPosition();

			size_t tries = targetPath.size();
			while (tries > 0) // hypothetically walk the aggroTarget through the path.
			{
				--tries;

				pf->getTuCostPf(pos, targetPath.back(), &posNext);
				targetPath.pop_back();
				pos = posNext;

				tile = _battleSave->getTile(pos);
				if (_battleSave->getTileEngine()->canTargetUnit( // do a virtual fire calculation
															&originVoxel,
															tile,
															&scanVoxel,
															_unit,
															_aggroTarget) == true)
				{
					// if unit can virtually fire at the hypothetical target it knows which way to face.
					_ambushAction->finalFacing = _battleSave->getTileEngine()->getDirectionTo(
																						_ambushAction->target,
																						pos);
					break;
				}
			}

			//if (_traceAI) Log(LOG_INFO) << "Ambush estimation will move to " << _ambushAction->target;
			return;
		}
	}
	//if (_traceAI) Log(LOG_INFO) << "Ambush estimation failed";
}

/**
 * Try to set up a combat action.
 * @note This will either be a psionic, grenade, or weapon attack or potentially
 * just moving to get a line of sight to a target.
 * @note Fills out the '_attackAction' with useful data.
 */
void AlienBAIState::setupAttack() // private.
{
	//Log(LOG_INFO) << "AlienBAIState::setupAttack()";
	_attackAction->type = BA_RETHINK;
	_psiAction->type = BA_NONE;

	// if enemies are known to us but not necessarily visible, we can attack them with a blaster launcher or psi.
	if (_targetsKnown != 0)
	{
		//Log(LOG_INFO) << ". enemies known";
		if (psiAction() == true)
		{
			//Log(LOG_INFO) << ". . do Psi attack EXIT";
			// at this point we can save some time with other calculations - the unit WILL make a psionic attack this turn.
			return;
		}

		if (_blaster == true)
		{
			//Log(LOG_INFO) << ". . do Blaster launch";
			wayPointAction();
		}
	}

	// if we CAN see someone, that makes them a viable target for "regular" attacks.
	//Log(LOG_INFO) << ". . selectNearestTarget()";
	if (selectNearestTarget() != 0)
	{
		//Log(LOG_INFO) << ". enemies visible = " << selectNearestTarget();
		// if there are both types of weapon, make a determination on which to use.
//		if (_unit->getGrenade() != NULL)
//		{
			//Log(LOG_INFO) << ". . getGrenade TRUE, grenadeAction()";
		grenadeAction();
			//Log(LOG_INFO) << ". . . . . . grenadeAction() DONE";
//		}

		if (_melee == true
			&& _rifle == true)
		{
			//Log(LOG_INFO) << ". . Melee & Rifle are TRUE, selectMeleeOrRanged()";
			selectMeleeOrRanged();
		}

		if (_melee == true)
		{
			//Log(LOG_INFO) << ". . Melee TRUE, meleeAction()";
			meleeAction();
			//Log(LOG_INFO) << ". . . . . . meleeAction() DONE";
		}

		if (_rifle == true)
		{
			//Log(LOG_INFO) << ". . Rifle TRUE, projectileAction()";
			projectileAction();
			//Log(LOG_INFO) << ". . . . . . projectileAction() DONE";
		}
	}
	//Log(LOG_INFO) << ". . selectNearestTarget() DONE";

/*	std::string st;
	switch (_attackAction->type)
	{
		case  0: BA_NONE;		st = "none";		break;
		case  1: BA_TURN,		st = "turn";		break;
		case  2: BA_MOVE,		st = "walk";		break;
		case  3: BA_PRIME,		st = "prime";		break;
		case  4: BA_THROW,		st = "throw";		break;
		case  5: BA_AUTOSHOT,	st = "autoshot";	break;
		case  6: BA_SNAPSHOT,	st = "snapshot";	break;
		case  7: BA_AIMEDSHOT,	st = "aimedshot";	break;
		case  8: BA_HIT,		st = "hit";			break;
		case  9: BA_USE,		st = "use";			break;
		case 10: BA_LAUNCH,		st = "launch";		break;
		case 11: BA_PSICONTROL,	st = "mindcontrol";	break;
		case 12: BA_PSIPANIC,	st = "panic";		break;
		case 13: BA_RETHINK,	st = "rethink";		break;
		case 14: BA_DEFUSE,		st = "defuse";		break;
		case 15: BA_DROP,		st = "drop";		break;
		case 16: BA_PSICONFUSE,	st = "confuse";		break;
		case 17: BA_PSICOURAGE, st = "courage";
	} */
	//Log(LOG_INFO) << ". bat = " << st;

	if (_attackAction->type != BA_RETHINK)
	{
		//if (_attackAction->type == BA_MOVE) Log(LOG_INFO) << ". . walk to " << _attackAction->target;
		//else Log(LOG_INFO) << ". . shoot at " << _attackAction->target;

		return;
	}
	else if (_xcomSpotters != 0
//		|| _unit->getAggression() < RNG::generate(0,3))
		|| _unit->getAggression() > RNG::generate( // kL, opposite to previously.
												0,
												_unit->getAggression()))
	{
		if (findFirePoint() == true) // if enemies can see us, or if we're feeling lucky, we can try to spot the enemy.
		{
			//Log(LOG_INFO) << ". . findFirePoint TRUE " << _attackAction->target;
			return;
		}
	}

	//Log(LOG_INFO) << "AlienBAIState::setupAttack() EXIT failed";
}

/**
 * Attempts to find cover and move toward it.
 * @note The idea is to check within a 11x11 tile square for a tile that is not
 * seen by '_aggroTarget'. If there is no such tile run away from the target.
 * @note Fills out the '_escapeAction' with useful data.
 */
void AlienBAIState::setupEscape() // private.
{
	bool coverFound = false;

	selectNearestTarget();
	_tuEscape = 0;

	int
		bestTileScore = -100000,
		tileScore,
		tries = -1,
		dist;

	if (_aggroTarget != NULL)
		dist = TileEngine::distance(
								_unit->getPosition(),
								_aggroTarget->getPosition());
	else
		dist = 0;

	// weights of various factors in choosing a tile to which to withdraw
	const int
		EXPOSURE_PENALTY		= 10,
		FIRE_PENALTY			= 40,
		BASE_SUCCESS_SYSTEMATIC	= 100,
		BASE_SUCCESS_DESPERATE	= 110,
		FAST_PASS_THRESHOLD		= 100,	// a tileScore that's good enough to quit the while loop early;
										// it's subjective, hand-tuned and may need tweaking
		CUR_TILE_PREF			= 15,
		unitsSpotting = countSpottingUnits(_unit->getPosition());

	Position bestTile (0,0,0); // init.
	const Tile* tile;

	std::vector<Position> tileSearch = _battleSave->getTileSearch();
	RNG::shuffle(tileSearch);

	Pathfinding* const pf = _battleSave->getPathfinding();
	pf->setPathingUnit(_unit);

	while (tries < 150
		&& coverFound == false)
	{
		_escapeAction->target = _unit->getPosition();		// start looking in a direction away from the enemy
		if (_battleSave->getTile(_escapeAction->target) == NULL)
			_escapeAction->target = _unit->getPosition();	// cornered at the edge of the map perhaps?

		tileScore = 0;

		if (tries == -1)
		{
			// you know, maybe we should just stay where we are and not risk reaction fire...
			// or maybe continue to wherever we were running to and not risk looking stupid
			if (_battleSave->getTile(_unit->_lastCover) != NULL)
				_escapeAction->target = _unit->_lastCover;
		}
		else if (tries < _battleSave->SEARCH_SIZE) //121 // looking for cover
		{
			_escapeAction->target.x += tileSearch[tries].x;
			_escapeAction->target.y += tileSearch[tries].y;

			tileScore = BASE_SUCCESS_SYSTEMATIC;

			if (_escapeAction->target == _unit->getPosition())
			{
				if (unitsSpotting > 0)
				{
					// maybe don't stay in the same spot? move or something if there's any point to it?
					_escapeAction->target.x += RNG::generate(-20,20);
					_escapeAction->target.y += RNG::generate(-20,20);
				}
				else
					tileScore += CUR_TILE_PREF;
			}
		}
		else
		{
			//if (tries == 121 && _traceAI) Log(LOG_INFO) << "best tileScore after systematic search was: " << bestTileScore;
			tileScore = BASE_SUCCESS_DESPERATE; // ruuuuuuun!!1

			_escapeAction->target = _unit->getPosition();
			_escapeAction->target.x += RNG::generate(-10,10);
			_escapeAction->target.y += RNG::generate(-10,10);
			_escapeAction->target.z = _unit->getPosition().z + RNG::generate(-1,1);

			if (_escapeAction->target.z < 0)
				_escapeAction->target.z = 0;
			else if (_escapeAction->target.z >= _battleSave->getMapSizeZ())
				_escapeAction->target.z = _unit->getPosition().z;
		}

		++tries;

		// think, Dang NABBIT!!!
		tile = _battleSave->getTile(_escapeAction->target);
		int distTarget;
		if (_aggroTarget != NULL)
			distTarget = TileEngine::distance(
											_aggroTarget->getPosition(),
											_escapeAction->target);
		else
			distTarget = 0;

		if (dist >= distTarget)
			tileScore -= (distTarget - dist) * 10;
		else
			tileScore += (distTarget - dist) * 10;

		if (tile == NULL)
			tileScore = -100001; // no you can't quit the battlefield by running off the map.
		else
		{
			if (std::find(
					_reachable.begin(),
					_reachable.end(),
					_battleSave->getTileIndex(_escapeAction->target)) == _reachable.end())
			{
				continue; // just ignore unreachable tiles
			}

			const int spotters = countSpottingUnits(_escapeAction->target);
			if (spotters != 0
				|| _xcomSpotters != 0)
			{
				if (_xcomSpotters <= spotters)
					tileScore -= (1 + spotters - _xcomSpotters) * EXPOSURE_PENALTY; // that's for giving away our position, schmuckhead.
				else
					tileScore += (_xcomSpotters - spotters) * EXPOSURE_PENALTY;
			}

			if (tile->getFire() != 0)
				tileScore -= FIRE_PENALTY;
			else
				tileScore += tile->getSmoke() * 5; // kL

			if (tile->getDangerous() == true)
				tileScore -= BASE_SUCCESS_SYSTEMATIC;

/*			if (_traceAI)
			{
				tile->setPreviewColor(tileScore < 0? 3: (tileScore < FAST_PASS_THRESHOLD / 2? 8: (tileScore < FAST_PASS_THRESHOLD? 9: 5)));
				tile->setPreviewDir(10);
				tile->setPreviewTu(tileScore);
			} */
		}

		if (tile != NULL
			&& tileScore > bestTileScore)
		{
			// calculate TUs to tile
			// this could be gotten w/ findReachable() somehow but that would break something for sure
			// kL_note: But maybe not after my adulterations to Pathfinding/this/etc.
			pf->calculate(
						_unit,
						_escapeAction->target);

			if (_escapeAction->target == _unit->getPosition()
				|| pf->getStartDirection() != -1)
			{
				bestTileScore = tileScore;
				bestTile = _escapeAction->target;

				_tuEscape = pf->getTotalTUCost();
				if (_escapeAction->target == _unit->getPosition())
					_tuEscape = 1;

/*				if (_traceAI)
				{
					tile->setPreviewColor(tileScore < 0? 7:(tileScore < FAST_PASS_THRESHOLD / 2? 10:(tileScore < FAST_PASS_THRESHOLD? 4:5)));
					tile->setPreviewDir(10);
					tile->setPreviewTu(tileScore);
				} */
			}

			pf->abortPath();

			if (bestTileScore > FAST_PASS_THRESHOLD)
				coverFound = true; // good enough, gogo-agogo!!
		}
	}

	_escapeAction->target = bestTile;
	//if (_traceAI) _battleSave->getTile(_escapeAction->target)->setPreviewColor(13);

	if (bestTileScore < -99999)
	{
		//if (_traceAI) Log(LOG_INFO) << "Escape estimation failed.";
		_escapeAction->type = BA_RETHINK; // do something, just don't look dumbstruck :P
	}
	else
	{
		//if (_traceAI) Log(LOG_INFO) << "Escape estimation completed after " << tries << " tries, "
		//		<< _battleSave->getTileEngine()->distance(_unit->getPosition(), bestTile) << " squares or so away.";
		_escapeAction->type = BA_MOVE;
	}
}

/**
 * Counts how many targets, both xcom and civilian are known to the unit.
 * @return, how many targets are known to it
 */
int AlienBAIState::countKnownTargets() const // private.
{
	int ret = 0;
	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if (validTarget(
					*i,
					true,true) == true)
		{
			++ret;
		}
	}

	return ret;
}

/**
 * Counts how many enemies (ie xCom) are spotting any given position.
 * @param pos - reference the Position to check for spotters
 * @return, spotters
 */
int AlienBAIState::countSpottingUnits(const Position& pos) const // private.
{
	int ret = 0;

	// if target doesn't actually occupy the position being checked do a virtual LOF check
	const bool checking = (pos != _unit->getPosition());
	const TileEngine* const te = _battleSave->getTileEngine();

	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if (validTarget(*i) == true
			&& TileEngine::distance(pos, (*i)->getPosition()) < 25) // note: does not account for xCom's reduced nightvision
		{
			Position
				targetVoxel,
				originVoxel = te->getSightOriginVoxel(*i);
			originVoxel.z -= 2;

			const BattleUnit* potentialUnit;
			if (checking == true)
				potentialUnit = _unit;
			else
				potentialUnit = NULL;

			if (te->canTargetUnit(
								&originVoxel,
								_battleSave->getTile(pos),
								&targetVoxel,
								*i,
								potentialUnit) == true)
			{
				++ret;
			}
		}
	}

	return ret;
}

/**
 * Selects the nearest known living target we can see/reach and returns the
 * number of visible enemies.
 * @note This function includes civilians as viable targets.
 * @return, viable targets
 */
int AlienBAIState::selectNearestTarget() // private.
{
	_aggroTarget = NULL;
	_closestDist = 1000;

	Position
		origin,
		target;
	int
		ret = 0,
		distTest,
		dir;
	const TileEngine* const te = _battleSave->getTileEngine();

	BattleAction action;

	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if (validTarget(
					*i,
					true,true) == true
			&& te->visible(
						_unit,
						(*i)->getTile()) == true)
		{
			++ret;

			distTest = TileEngine::distance(
										_unit->getPosition(),
										(*i)->getPosition());
			if (distTest < _closestDist)
			{
				bool valid = false;

				if (_rifle == true || _melee == false) // -> is ambiguity like that required.
				{
					action.actor = _unit;

					origin = te->getOriginVoxel(action);
					valid = te->canTargetUnit(
											&origin,
											(*i)->getTile(),
											&target,
											_unit);
				}
				else if (selectPositionNearTarget(
											*i,
											_unit->getTimeUnits()) == true)
				{
					dir = te->getDirectionTo(
										_attackAction->target,
										(*i)->getPosition());
					valid = te->validMeleeRange(
											_attackAction->target,
											dir,
											_unit,
											*i);
				}

				if (valid == true)
				{
					_closestDist = distTest;
					_aggroTarget = *i;
				}
			}
		}
	}

	if (_aggroTarget != NULL)
		return ret;

	return 0;
}

/**
 * Selects the nearest known living Xcom unit - used for ambush calculations.
 * @return, true if it found one
 */
bool AlienBAIState::selectClosestKnownEnemy() // private.
{
	_aggroTarget = NULL;
	int
		distClosest = 255,
		distTest;

	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if (validTarget(*i, true) == true)
		{
			distTest = TileEngine::distance(
										_unit->getPosition(),
										(*i)->getPosition());
			if (distTest < distClosest)
			{
				distClosest = distTest;
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
bool AlienBAIState::selectRandomTarget() // private.
{
	_aggroTarget = NULL;
	int
		distFarthest = -1000,
		distTest;

	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if (validTarget(*i, true, true) == true)
		{
			distTest = RNG::generate(0,20);
			distTest -= TileEngine::distance(
										_unit->getPosition(),
										(*i)->getPosition());
			if (distTest > distFarthest)
			{
				distFarthest = distTest;
				_aggroTarget = *i;
			}
		}
	}

	return (_aggroTarget != NULL);
}

/**
 * Selects a point near enough to a BattleUnit to perform a melee attack.
 * @param targetUnit	- pointer to a target BattleUnit
 * @param tuMax		- maximum time units that the path can cost
 * @return, true if a point was found
 */
bool AlienBAIState::selectPositionNearTarget( // private.
		const BattleUnit* const targetUnit,
		int tuMax) const
{
	bool ret = false;

	const int
		actorSize = _unit->getArmor()->getSize(),
		targetSize = targetUnit->getArmor()->getSize();
	size_t dist = 1000;

	Pathfinding* const pf = _battleSave->getPathfinding();
	pf->setPathingUnit(_unit);

	const TileEngine* const te = _battleSave->getTileEngine();

	Position pos;
	int dir;
	bool
		valid,
		fit;

	for (int
			z = -1;
			z != 2;
			++z)
	{
		for (int
				x = -actorSize;
				x <= targetSize;
				++x)
		{
			for (int
					y = -actorSize;
					y <= targetSize;
					++y)
			{
				if (x != 0 || y != 0) // skip the unit itself
				{
					pos = targetUnit->getPosition() + Position(x,y,z);
					if (_battleSave->getTile(pos) != NULL
						&& std::find(
								_reachable.begin(),
								_reachable.end(),
								_battleSave->getTileIndex(pos)) != _reachable.end())
					{
						dir = te->getDirectionTo(pos, targetUnit->getPosition());
						valid = te->validMeleeRange(pos, dir, _unit, targetUnit);
						fit = _battleSave->setUnitPosition(_unit, pos, true);
						if (valid == true && fit == true
							&& _battleSave->getTile(pos)->getDangerous() == false)
						{
							pf->calculate(_unit, pos, NULL, tuMax);
							if (pf->getStartDirection() != -1 && pf->getPath().size() < dist)
							{
								ret = true;
								dist = pf->getPath().size();
								_attackAction->target = pos;
							}
							pf->abortPath();
						}
					}
				}
			}
		}
	}

	return ret;
}

/**
 * Selects an AI mode based on a number of factors: RNG and the results of these
 * determinations.
 */
void AlienBAIState::evaluateAIMode() // private.
{
	if (_unit->getChargeTarget() != NULL
		&& _attackAction->type != BA_RETHINK)
	{
		_AIMode = AI_COMBAT;
		return;
	}

	// if the aliens are cheating or the unit is charging enforce combat as a priority
	if (_battleSave->isCheating() == true // <- hmm, do i want this - kL_note
		|| _unit->getChargeTarget() != NULL
		|| _blaster == true)	// The two (_blaster==true) checks in this function ought obviate the entire re-evaluate thing!
								// Note, there is a valid targetPosition but targetUnit is NOT at that Pos if blaster=TRUE ....
	{
		_AIMode = AI_COMBAT;
	}
	else
	{
		float
			patrolOdds = 28.f, // was 30
			ambushOdds = 13.f, // was 12
			combatOdds = 23.f, // was 20
			escapeOdds = 13.f; // was 15

		if (_unit->getTimeUnits() > _unit->getBaseStats()->tu / 2
			|| _unit->getChargeTarget() != NULL)
		{
			escapeOdds = 5.f;
		}
		else if (_melee == true)
			escapeOdds = 10.5f; // was 12

		// we're less likely to patrol if we see targets
		if (_targetsVisible != 0)
			patrolOdds = 8.f; // was 15

		// the enemy sees us, we should take retreat into consideration and forget about patrolling for now
		if (_xcomSpotters != 0)
		{
			patrolOdds = 0.f;

			if (_tuEscape == 0)
				setupEscape();
		}

		// melee/blaster units shouldn't consider ambush
		if (_rifle == false
			|| _tuAmbush == 0)
		{
			ambushOdds = 0.f;

			if (_melee == true)
			{
//				escapeOdds = 10.f;	// kL
				combatOdds *= 1.2f;	// kL
			}
		}

		// if we KNOW there are targets around...
		if (_targetsKnown != 0)
		{
			if (_targetsKnown == 1)
				combatOdds *= 1.9f; // was 1.2

			if (_tuEscape == 0)
			{
				if (selectClosestKnownEnemy() == true)
					setupEscape();
				else
					escapeOdds = 0.f;
			}
		}
		else
		{
			combatOdds =
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
			combatOdds *= 0.6f; // was 0.6
			ambushOdds *= 0.7f; // was 0.75
		}
		else if (_unit->getHealth() < _unit->getBaseStats()->health * 2 / 3)
		{
			escapeOdds *= 1.5f; // was 1.4
			combatOdds *= 0.8f; // was 0.8
			ambushOdds *= 0.9f; // was 0.8
		}
		else if (_unit->getHealth() < _unit->getBaseStats()->health)
			escapeOdds *= 1.2f; // was 1.1

		switch (_unit->getAggression()) // take aggression into consideration
		{
			case 0:
				escapeOdds *= 1.5f;		// was 1.4
				combatOdds *= 0.55f;	// was 0.7
			break;

			case 1:
				ambushOdds *= 1.2f;		// was 1.1
			break;

			case 2:
				combatOdds *= 1.65f;	// was 1.4
				escapeOdds *= 0.5f;		// was 0.7
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
		}

		if (_AIMode == AI_COMBAT)
			ambushOdds *= 1.5f;

		if (_xcomSpotters) // factor in the spotters.
		{
			escapeOdds *= (10.f * static_cast<float>(_xcomSpotters + 10) / 100.f);
			combatOdds *= (5.f * static_cast<float>(_xcomSpotters + 20) / 100.f);
		}
		else
			escapeOdds /= 2.f;

		if (_targetsVisible) // factor in visible enemies.
		{
			combatOdds *= (10.f * static_cast<float>(_targetsVisible + 10) / 100.f);

			if (_closestDist < 6) // was <5
				ambushOdds = 0.f;
		}

		if (_tuAmbush) // make sure we have an ambush lined up, or don't even consider it.
			ambushOdds *= 2.f; // was 1.7
		else
			ambushOdds = 0.f;

		// factor in mission type
//		if (_battleSave->getTacticalType() == "STR_BASE_DEFENSE")
		if (_battleSave->getTacType() == TCT_BASEDEFENSE)
		{
			escapeOdds *= 0.8f; // was 0.75
			ambushOdds *= 0.5f; // was 0.6
		}

		//Log(LOG_INFO) << "patrolOdds = " << patrolOdds;
		//Log(LOG_INFO) << "ambushOdds = " << ambushOdds;
		//Log(LOG_INFO) << "combatOdds = " << combatOdds;
		//Log(LOG_INFO) << "escapeOdds = " << escapeOdds;

		// GENERATE A RANDOM NUMBER TO REPRESENT THE SITUATION:
		// AI_PATROL,	// 0
		// AI_AMBUSH,	// 1
		// AI_COMBAT,	// 2
		// AI_ESCAPE	// 3
		const float decision = static_cast<float>(RNG::generate(
															1,
															std::max(
																1,
																static_cast<int>(std::floor(patrolOdds + ambushOdds + combatOdds + escapeOdds)))));
		//Log(LOG_INFO) << "decision = " << decision;
		if (decision <= patrolOdds)
		{
			//Log(LOG_INFO) << ". do Patrol";
			_AIMode = AI_PATROL;
		}
		else if (decision <= patrolOdds + ambushOdds)
		{
			//Log(LOG_INFO) << ". do Ambush";
			_AIMode = AI_AMBUSH;
		}
		else if (decision <= patrolOdds + ambushOdds + combatOdds)
		{
			//Log(LOG_INFO) << ". do Combat";
			_AIMode = AI_COMBAT;
		}
		else //if (decision <= patrolOdds + ambushOdds + combatOdds + escapeOdds)
		{
			//Log(LOG_INFO) << ". do Escape";
			_AIMode = AI_ESCAPE;
		}
	/*	else if (static_cast<float>(decision) > escapeOdds)
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
			_AIMode = AI_ESCAPE; */
	}

	// Check validity of the decision and if that fails try a fallback behaviour according to priority.
	// 1) Combat
	// 2) Patrol
	// 3) Ambush
	// 4) Escape
	if (_AIMode == AI_COMBAT)
	{
		//Log(LOG_INFO) << ". AI_COMBAT";
//		if (_aggroTarget) // kL
		if (_blaster == true	// kL, && (if aggroTarget=TRUE) perhaps.
								// Or simply (if bat=BA_LAUNCH) because '_attackAction->target'
								// is the first wp, which will likely (now certainly) be NULL for a unit.
								// ( Blaster-wielding units should go for an AimedShot ... costs less TU.)
								// Likely the check should be for BA_LAUNCH 'cause I'm doing funny things w/ _blaster var at present ...
								// WOW that work'd !!1!1
								// TODO: find a way stop my squads getting utterly obliterated, fairly.
			|| (_battleSave->getTile(_attackAction->target) != NULL
				&& _battleSave->getTile(_attackAction->target)->getUnit() != NULL))
		{
			//Log(LOG_INFO) << ". . targetUnit Valid";
			if (_attackAction->type != BA_RETHINK
				|| findFirePoint() == true)
			{
				//Log(LOG_INFO) << ". . . findFirePoint ok OR using Blaster, ret w/ AI_COMBAT";
				return;
			}
		}
		else if (selectRandomTarget() == true
			&& findFirePoint() == true)
		{
			//Log(LOG_INFO) << ". . targetUnit NOT Valid; randomTarget Valid + findFirePoint ok";
			return;
		}

		//Log(LOG_INFO) << ". targetUnit NOT Valid & can't find FirePoint: falling back to AI_PATROL";
		_AIMode = AI_PATROL;
	}

	if (_AIMode == AI_PATROL
		&& _toNode == NULL)
	{
		//Log(LOG_INFO) << ". toNode invalid: falling back to AI_AMBUSH";
		_AIMode = AI_AMBUSH;
	}

	if (_AIMode == AI_AMBUSH
		&& _tuAmbush == 0)
	{
		//Log(LOG_INFO) << ". no ambush TU; falling back to AI_ESCAPE";
		_AIMode = AI_ESCAPE;
	}
}

/**
 * Find a position where the target can be seen and move there.
 * @note Checks an 11x11 grid for a position nearby from which actor can target.
 * @return, true if a possible position is found
 */
bool AlienBAIState::findFirePoint() // private.
{
	if (selectClosestKnownEnemy() == false)
		return false;

	std::vector<Position> tileSearch = _battleSave->getTileSearch();
	RNG::shuffle(tileSearch);

	const int
		BASE_SUCCESS_SYSTEMATIC	= 100,
		FAST_PASS_THRESHOLD		= 125;

	_attackAction->type = BA_RETHINK;

	Position
		origin,
		target,
		pos;
	const Tile* tile;

	int
		bestTileScore = 0,
		tileScore;
	Pathfinding* const pf = _battleSave->getPathfinding();
	pf->setPathingUnit(_unit);

	for (std::vector<Position>::const_iterator
			i = tileSearch.begin();
			i != tileSearch.end();
			++i)
	{
		pos = _unit->getPosition() + *i;

		tile = _battleSave->getTile(pos);
		if (tile == NULL
			|| std::find(
					_reachableAttack.begin(),
					_reachableAttack.end(),
					_battleSave->getTileIndex(pos)) == _reachableAttack.end())
		{
			continue;
		}

		tileScore = 0;
		// i should really make a function for this
		origin = pos * Position(16,16,24)
					 + Position(
							8,8,
							_unit->getHeight()
								+ _unit->getFloatHeight()
								- tile->getTerrainLevel()
								- 4);
			// 4 because -2 is eyes and 2 below that is the rifle (or at least that's my understanding)

		if (_battleSave->getTileEngine()->canTargetUnit(
													&origin,
													_aggroTarget->getTile(),
													&target,
													_unit) == true)
		{
			pf->calculate(
						_unit,
						pos);

			if (pf->getStartDirection() != -1)						// can move here
//				&& pf->getTotalTUCost() <= _unit->getTimeUnits())	// can still shoot
			{
				tileScore = BASE_SUCCESS_SYSTEMATIC - countSpottingUnits(pos) * 10;
				tileScore += _unit->getTimeUnits() - pf->getTotalTUCost();

				if (_aggroTarget->checkViewSector(pos) == false)
					tileScore += 10;

				if (tileScore > bestTileScore)
				{
					bestTileScore = tileScore;

					_attackAction->target = pos;
					_attackAction->finalFacing = _battleSave->getTileEngine()->getDirectionTo(
																							pos,
																							_aggroTarget->getPosition());

					if (tileScore > FAST_PASS_THRESHOLD)
						break;
				}
			}
		}
	}

	if (bestTileScore > 70)
	{
		//if (_traceAI) Log(LOG_INFO) << "Firepoint found at " << _attackAction->target << ", with a tileScore of: " << bestTileScore;
		_attackAction->type = BA_MOVE;
		return true;
	}

	//if (_traceAI) Log(LOG_INFO) << "Firepoint failed, best estimation was: " << _attackAction->target << ", with a tileScore of: " << bestTileScore;
	return false;
}

/**
 * Decides if it's worthwhile to create an explosion.
 * @note Also called from TileEngine::reactionShot().
 * @param posTarget		- reference the target's position
 * @param attacker		- pointer to the attacking unit
 * @param explRadius	- radius of explosion in tile space
 * @param diff			- game difficulty
// * @param grenade		- true if explosion will be from a grenade
 * @return, true if it's worthwile creating an explosion at the target position
 */
bool AlienBAIState::explosiveEfficacy(
		const Position& posTarget,
		const BattleUnit* const attacker,
		const int explRadius,
		const int diff) const
//		bool grenade) const
{
	//Log(LOG_INFO) << "\n";
	//Log(LOG_INFO) << "explosiveEfficacy() rad = " << explRadius;
	int pct = 0;

	const int firstGrenade = _battleSave->getBattleState()->getGame()->getRuleset()->getFirstGrenade();
	if (firstGrenade == -1
		|| (firstGrenade == 0
			&& _battleSave->getTurn() > 4 - diff)
		|| (firstGrenade > 0
			&& _battleSave->getTurn() > firstGrenade - 1))
	{
		pct = (100 - attacker->getMorale()) / 3;
		pct += 2 * (10 - static_cast<int>(
						 static_cast<float>(attacker->getHealth()) / static_cast<float>(attacker->getBaseStats()->health)
						 * 10.f));
		pct += attacker->getAggression() * 10;

		const int dist = TileEngine::distance(
											attacker->getPosition(),
											posTarget);
		if (dist < explRadius + 1)
		{
			pct -= (explRadius - dist + 1) * 5;
			if (std::abs(attacker->getPosition().z - posTarget.z) < Options::battleExplosionHeight + 1)
				pct -= 15;
		}

		switch (_battleSave->getTacType())
		{
			case TCT_BASEASSAULT:
				pct -= 23;
			break;

			case TCT_BASEDEFENSE:
			case TCT_MISSIONSITE:
				pct += 56;
		}

		pct += diff * 2;

		const BattleUnit* const targetUnit = _battleSave->getTile(posTarget)->getUnit();

		//Log(LOG_INFO) << "attacker = " << attacker->getId();
		//Log(LOG_INFO) << "posTarget = " << posTarget;
		for (std::vector<BattleUnit*>::const_iterator
				i = _battleSave->getUnits()->begin();
				i != _battleSave->getUnits()->end();
				++i)
		{
			//Log(LOG_INFO) << "\n";
			//Log(LOG_INFO) << ". id = " << (*i)->getId();
			//Log(LOG_INFO) << ". isNOTOut = " << ((*i)->isOut(true) == false);
			//Log(LOG_INFO) << ". isNOTAttacker = " << (*i != attacker);
			//Log(LOG_INFO) << ". isNOTtargetUnit = " << (*i != targetUnit);
			//Log(LOG_INFO) << ". vertCheck = " << (std::abs((*i)->getPosition().z - posTarget.z) < Options::battleExplosionHeight + 1);
			//Log(LOG_INFO) << ". inDist = " << (_battleSave->getTileEngine()->distance(posTarget, (*i)->getPosition()) < explRadius + 1);
			if ((*i)->isOut(true) == false
				&& *i != attacker
				&& *i != targetUnit
				&& std::abs((*i)->getPosition().z - posTarget.z) < Options::battleExplosionHeight + 1
				&& TileEngine::distance(
									posTarget,
									(*i)->getPosition()) < explRadius + 1)
			{
				//Log(LOG_INFO) << ". . dangerousFALSE = " << ((*i)->getTile() != NULL && (*i)->getTile()->getDangerous() == false);
				//Log(LOG_INFO) << ". . exposed = " << ((*i)->getFaction() == FACTION_HOSTILE || (*i)->getExposed() < _intelligence + 1);
				if ((*i)->getTile() != NULL
					&& (*i)->getTile()->getDangerous() == false
					&& ((*i)->getFaction() == FACTION_HOSTILE
						|| ((*i)->getExposed() != -1
							&& (*i)->getExposed() <= _intelligence)))
				{
					const Position
						voxelPosA = Position::toVoxelSpaceCentered(posTarget, 12),
						voxelPosB = Position::toVoxelSpaceCentered((*i)->getPosition(), 12);

					std::vector<Position> trajectory;
					const VoxelType impact = _battleSave->getTileEngine()->plotLine(
																				voxelPosA,
																				voxelPosB,
																				false,
																				&trajectory,
																				targetUnit,
																				true,
																				false,
																				*i);
					//Log(LOG_INFO) << "trajSize = " << (int)trajectory.size() << "; impact = " << impact;
					if (impact == VOXEL_UNIT
						&& (*i)->getPosition() == trajectory.front() / Position(16,16,24))
					{
						//Log(LOG_INFO) << "trajFront " << (trajectory.front() / Position(16,16,24));
						if ((*i)->getFaction() != FACTION_HOSTILE)
							pct += 12;

						if ((*i)->getOriginalFaction() == FACTION_HOSTILE)
						{
							pct -= 6;
							if ((*i)->getFaction() == FACTION_HOSTILE)
								pct -= 12;
						}
					}
				}
			}
		}
	}

	return (RNG::percent(pct) == true);
}

/**
 * Attempts to take a melee attack/charge a seen enemy.
 * @note Melee targeting: can see an enemy, can move to it, so let's charge
 * blindly at it.
 */
void AlienBAIState::meleeAction() // private.
{
	const TileEngine* const te = _battleSave->getTileEngine();
	int dir;

	if (_aggroTarget != NULL
		&& _aggroTarget->isOut_t(OUT_STAT) == false)
//		&& _aggroTarget->isOut() == false) // (true, true)
	{
		dir = te->getDirectionTo(
							_unit->getPosition(),
							_aggroTarget->getPosition());
		if (te->validMeleeRange(_unit, dir, _aggroTarget) == true)
		{
			meleeAttack();
			return;
		}
	}

	const int tuPreMelee = _unit->getTimeUnits()
						 - _unit->getActionTu(
											BA_HIT,
											_attackAction->weapon);
//											_unit->getMainHandWeapon());
	int
		dist = (tuPreMelee / 4) + 1,
		distTest;

	_aggroTarget = NULL;

	// TODO: set up a vector of BattleUnits to pick from
	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if (validTarget(*i, true, true) == true) // pick closest living unit that can be moved to
		{
			distTest = TileEngine::distance(
										_unit->getPosition(),
										(*i)->getPosition());
			if (distTest == 1 // TODO: should do a validMeleeRange() for this. Or even in validTarget(isMelee) ...
				|| (distTest < dist
					&& selectPositionNearTarget(*i, tuPreMelee) == true))
			{
				_aggroTarget = *i;
				_attackAction->type = BA_MOVE;
				_unit->setChargeTarget(_aggroTarget);

				dist = distTest;
			}
		}
	}

	if (_aggroTarget != NULL)
	{
		dir = te->getDirectionTo(
							_unit->getPosition(),
							_aggroTarget->getPosition());
		if (te->validMeleeRange(_unit, dir, _aggroTarget) == true)
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

/**
 * Performs a melee attack action.
 */
void AlienBAIState::meleeAttack() // private.
{
	_unit->setDirectionTo(
			_aggroTarget->getPosition() + Position(
												_unit->getArmor()->getSize() - 1,
												_unit->getArmor()->getSize() - 1,
												0));

	while (_unit->getUnitStatus() == STATUS_TURNING)
		_unit->turn();

	//if (_traceAI) Log(LOG_INFO) << "Attack unit: " << _aggroTarget->getId();
	_attackAction->target = _aggroTarget->getPosition();
	_attackAction->type = BA_HIT;
}

/**
 * Attempts to trace a waypoint projectile to an enemy that is known by any aLien.
 */
void AlienBAIState::wayPointAction() // private.
{
	//Log(LOG_INFO) << "AlienBAIState::wayPointAction() w/ " << _attackAction->weapon->getRules()->getType();
	_attackAction->TU = _unit->getActionTu(
										BA_LAUNCH,
										_attackAction->weapon);
	//Log(LOG_INFO) << ". actionTU = " << _attackAction->TU;
	//Log(LOG_INFO) << ". unit TU = " << _unit->getTimeUnits();

	if (_unit->getTimeUnits() >= _attackAction->TU)
	{
		Pathfinding* const pf = _battleSave->getPathfinding();
		pf->setPathingUnit(_unit); // jic.

		std::vector<BattleUnit*> targets;
//		const int explRadius = _unit->getMainHandWeapon()->getAmmoItem()->getRules()->getExplosionRadius();
		const int explRadius = _attackAction->weapon->getAmmoItem()->getRules()->getExplosionRadius();

		for (std::vector<BattleUnit*>::const_iterator
				i = _battleSave->getUnits()->begin();
				i != _battleSave->getUnits()->end();
				++i)
		{
			//Log(LOG_INFO) << ". . test Vs unit ID " << (*i)->getId() << " pos " << (*i)->getPosition();
			if (validTarget(
						*i,
						true,
						true) == true)
			{
				//Log(LOG_INFO) << ". . . unit VALID";
				if (explosiveEfficacy(
								(*i)->getPosition(),
								_unit,
								explRadius,
								_attackAction->diff) == true)
				{
					//Log(LOG_INFO) << ". . . . explEff VALID";
					_aggroTarget = *i;

					if (pathWaypoints() == true)
						targets.push_back(*i);
				}
				//else Log(LOG_INFO) << ". . . . explEff invalid";

				pf->abortPath();
			}
		}

		if (targets.empty() == false)
		{
			//Log(LOG_INFO) << ". targets available";
			size_t target = RNG::generate(
										0,
										targets.size() - 1);
			_aggroTarget = targets.at(target);
			//Log(LOG_INFO) << ". . total = " << targets.size() << " pick = " << target << "; Target ID " << _aggroTarget->getId();
			if (pathWaypoints() == true) // vs. _aggroTarget, should be true
			{
				//Log(LOG_INFO) << ". . . Return, do LAUNCH";
				_attackAction->type = BA_LAUNCH;
				return; // success.
			}
		}
	}

	// fail:
	_blaster = false;
	_aggroTarget = NULL;
	_attackAction->type = BA_RETHINK;
	//Log(LOG_INFO) << ". . NULL target, reThink !";
}

/**
 * Constructs a waypoint path for weapons like Blaster Launcher.
 * @note helper for wayPointAction()
 * @return, true if waypoints get positioned
 */
bool AlienBAIState::pathWaypoints() // private.
{
	//Log(LOG_INFO) << "AlienBAIState::pathWaypoints() vs ID " << _aggroTarget->getId() << " pos " << _aggroTarget->getPosition();
	//Log(LOG_INFO) << ". actor ID " << _unit->getId() << " pos " << _unit->getPosition();

	Pathfinding* const pf = _battleSave->getPathfinding();
	pf->setPathingUnit(_unit);
	pf->calculate(
				_unit,
				_aggroTarget->getPosition(),
				_aggroTarget,
				-1);
	int dir = pf->dequeuePath();

	if (dir != -1)
	{
		_attackAction->waypoints.clear();

		Position
			pos = _unit->getPosition(),
			vect;
		int dir2;

		while (dir != -1)
		{
			dir2 = dir;

			while (dir != -1
				&& dir2 == dir)
			{
				pf->directionToVector(
									dir,
									&vect);
				pos += vect; // step along path one tile

				dir = pf->dequeuePath();
			}
			// dir changed:
			_attackAction->waypoints.push_back(pos); // place wp. Auto-explodes at last wp. Or when it hits anything, lulz.
			//Log(LOG_INFO) << ". . place WP " << pos;
		}

		// pathing done & wp's have been positioned:
		//Log(LOG_INFO) << ". . qty WP's = " << _attackAction->waypoints.size() << " / max WP's = " << _attackAction->weapon->getRules()->isWaypoints();
		if (static_cast<int>(_attackAction->waypoints.size()) <= _attackAction->weapon->getRules()->isWaypoints())
		{
			//Log(LOG_INFO) << ". path valid, ret TRUE";
			_attackAction->target = _attackAction->waypoints.front();
			return true;
		}
		//Log(LOG_INFO) << ". . too many WP's !!";
	}

	//Log(LOG_INFO) << ". path or WP's invalid, ret FALSE";
	return false;
}
/*		_attackAction->waypoints.clear();

		pf->calculate(
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
			dirPath = pf->dequeuePath();

		while (dirPath != -1)
		{
			posLast = posCur;

			pf->directionToVector(
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

			collision = _battleSave->getTileEngine()->plotLine(
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
				if (_battleSave->getTile(posCur)->getUnit() == _aggroTarget)
				{
					_attackAction->waypoints.push_back(posCur);
					posEnd = posCur;
				}
			}

			dirPath = pf->dequeuePath();
		}

		_attackAction->target = _attackAction->waypoints.front();

		if (static_cast<int>(_attackAction->waypoints.size()) > (_attackAction->diff * 2) + 6
			|| posEnd != _aggroTarget->getPosition())
		{
			_attackAction->type = BA_RETHINK;
		} */

/**
 * Attempts to fire at an enemy we can see.
 * @note Regular targeting: can see an enemy, have a gun, let's try to shoot it.
 */
void AlienBAIState::projectileAction() // private.
{
	_attackAction->target = _aggroTarget->getPosition();

	if (_attackAction->weapon->getAmmoItem()->getRules()->getExplosionRadius() == -1
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
 */
void AlienBAIState::selectFireMethod() // private.
{
	_attackAction->type = BA_RETHINK;

	int usableTU = _unit->getTimeUnits();
	if (_unit->getAggression() < RNG::generate(0,3))
		usableTU -= _tuEscape;

	const int dist = TileEngine::distance(
										_unit->getPosition(),
										_attackAction->target);
	if (dist <= _attackAction->weapon->getRules()->getAutoRange())
	{
		if (_attackAction->weapon->getRules()->getAutoTu() != 0
			&& usableTU >= _unit->getActionTu(
											BA_AUTOSHOT,
											_attackAction->weapon))
		{
			_attackAction->type = BA_AUTOSHOT;
		}
		else if (_attackAction->weapon->getRules()->getSnapTu() != 0
			&& usableTU >= _unit->getActionTu(
											BA_SNAPSHOT,
											_attackAction->weapon))
		{
			_attackAction->type = BA_SNAPSHOT;
		}
		else if (_attackAction->weapon->getRules()->getAimedTu() != 0
			&& usableTU >= _unit->getActionTu(
											BA_AIMEDSHOT,
											_attackAction->weapon))
		{
			_attackAction->type = BA_AIMEDSHOT;
		}
	}
	else if (dist <= _attackAction->weapon->getRules()->getSnapRange())
	{
		if (_attackAction->weapon->getRules()->getSnapTu() != 0
			&& usableTU >= _unit->getActionTu(
											BA_SNAPSHOT,
											_attackAction->weapon))
		{
			_attackAction->type = BA_SNAPSHOT;
		}
		else if (_attackAction->weapon->getRules()->getAimedTu() != 0
			&& usableTU >= _unit->getActionTu(
											BA_AIMEDSHOT,
											_attackAction->weapon))
		{
			_attackAction->type = BA_AIMEDSHOT;
		}
		else if (_attackAction->weapon->getRules()->getAutoTu() != 0
			&& usableTU >= _unit->getActionTu(
											BA_AUTOSHOT,
											_attackAction->weapon))
		{
			_attackAction->type = BA_AUTOSHOT;
		}
	}
	else if (dist <= _attackAction->weapon->getRules()->getAimRange())
	{
		if (_attackAction->weapon->getRules()->getAimedTu() != 0
			&& usableTU >= _unit->getActionTu(
											BA_AIMEDSHOT,
											_attackAction->weapon))
		{
			_attackAction->type = BA_AIMEDSHOT;
		}
		else if (_attackAction->weapon->getRules()->getSnapTu() != 0
			&& usableTU >= _unit->getActionTu(
											BA_SNAPSHOT,
											_attackAction->weapon))
		{
			_attackAction->type = BA_SNAPSHOT;
		}
		else if (_attackAction->weapon->getRules()->getAutoTu() != 0
			&& usableTU >= _unit->getActionTu(
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
void AlienBAIState::grenadeAction() // private.
{
	// do we have a grenade on our belt?
	// kL_note: this is already checked in setupAttack()
	// Could use it to determine if grenade is already inHand though! ( see _grenade var.)
	BattleItem* const grenade = _unit->getGrenade();
	if (grenade != NULL)
	{
		// distance must be more than X tiles, otherwise it's too dangerous to explode
		if (explosiveEfficacy(
						_aggroTarget->getPosition(),
						_unit,
						grenade->getRules()->getExplosionRadius(),
						_attackAction->diff) == true)
		{
			const RuleInventory* const invRule = grenade->getSlot();
			int tuCost = invRule->getCost(_battleSave->getBattleGame()->getRuleset()->getInventory("STR_RIGHT_HAND"));

			if (grenade->getFuse() == -1)
				tuCost += _unit->getActionTu(BA_PRIME, grenade);
			tuCost += _unit->getActionTu(BA_THROW, grenade); // the Prime itself is done 'auto' in ProjectileFlyBState.

			if (tuCost <= _unit->getTimeUnits())
			{
				BattleAction action;
				action.actor = _unit;
				action.target = _aggroTarget->getPosition();
				action.weapon = grenade;
				action.type = BA_THROW;

				const Position
					originVoxel = _battleSave->getTileEngine()->getOriginVoxel(action),
					targetVoxel = Position::toVoxelSpaceCentered(
															action.target,
															2 - _battleSave->getTile(action.target)->getTerrainLevel()); // LoFT of floor is typically 2 voxels thick.

				if (_battleSave->getTileEngine()->validateThrow(action, originVoxel, targetVoxel) == true)
				{
					_attackAction->target = action.target;
					_attackAction->weapon = grenade;
					_attackAction->type = BA_THROW;

					_rifle =
					_melee = false;
				}
			}
		}
	}
}
/*	// do we have a grenade on our belt?
	BattleItem *grenade = _unit->getGrenadeFromBelt();
	int tu = 4; // 4TUs for picking up the grenade
	tu += _unit->getActionTu(BA_PRIME, grenade);
	tu += _unit->getActionTu(BA_THROW, grenade);
	// do we have enough TUs to prime and throw the grenade?
	if (tu <= _unit->getTimeUnits())
	{
		BattleAction action;
		action.weapon = grenade;
		action.type = BA_THROW;
		action.actor = _unit;
		if (explosiveEfficacy(_aggroTarget->getPosition(), _unit, grenade->getRules()->getExplosionRadius(), _attackAction->diff, true))
		{
			action.target = _aggroTarget->getPosition();
		}
		else if (!getNodeOfBestEfficacy(&action))
		{
			return;
		}
		Position originVoxel = _battleSave->getTileEngine()->getOriginVoxel(action, 0);
		Position targetVoxel = action.target * Position(16,16,24) + Position(8,8, (2 + -_battleSave->getTile(action.target)->getTerrainLevel()));
		// are we within range?
		if (_battleSave->getTileEngine()->validateThrow(action, originVoxel, targetVoxel))
		{
			_attackAction->weapon = grenade;
			_attackAction->target = action.target;
			_attackAction->type = BA_THROW;
			_attackAction->TU = tu;
			_rifle = false;
			_melee = false;
		}
	} */

/**
 * Evaluates a psionic attack on an 'exposed' enemy.
 * @note Psionic targetting: pick from any of the exposed units. Exposed means
 * they have been previously spotted and are therefore known to the AI
 * regardless of whether they can be seen or not because they're psycho.
 * @return, true if a psionic attack should be performed
 */
bool AlienBAIState::psiAction() // private.
{
	//Log(LOG_INFO) << "AlienBAIState::psiAction() ID = " << _unit->getId();
	if (_didPsi == false									// didn't already do a psi action this round
		&& _unit->getBaseStats()->psiSkill != 0				// has psiSkill
		&& _unit->getOriginalFaction() == FACTION_HOSTILE)	// don't let any faction but HOSTILE mind-control others.
	{
		const RuleItem* const itRule = _battleSave->getBattleGame()->getRuleset()->getItem("ALIEN_PSI_WEAPON");

		int tuCost = itRule->getUseTu();
		if (itRule->getFlatRate() == false)
			tuCost = static_cast<int>(std::floor(
					 static_cast<float>(_unit->getBaseStats()->tu * tuCost) / 100.f));
		//Log(LOG_INFO) << "AlienBAIState::psiAction() tuCost = " << tuCost;

		if (_unit->getTimeUnits() < tuCost + _tuEscape) // check if aLien has the required TUs and can still make it to cover
		{
			//Log(LOG_INFO) << ". not enough Tu, EXIT";
			return false;
		}
		else // do it -> further evaluation req'd.
		{
			const int
				losFactor = 50, // increase chance of attack against a unit that is currently in LoS.
				attackStr = static_cast<int>(std::floor(
							static_cast<double>(_unit->getBaseStats()->psiStrength * _unit->getBaseStats()->psiSkill) / 50.));
			//Log(LOG_INFO) << ". . attackStr = " << attackStr;

			bool losTrue = false;
			int
				chance = 0,
				chance2 = 0;

			_aggroTarget = NULL;

			for (std::vector<BattleUnit*>::const_iterator
					i = _battleSave->getUnits()->begin();
					i != _battleSave->getUnits()->end();
					++i)
			{
				if ((*i)->getGeoscapeSoldier() != NULL // what about doggies .... Should use isFearable() for doggies ....
					&& validTarget( // will check for Mc, Exposed, etc.
								*i,
								true) == true
					&& (itRule->isLosRequired() == false
						|| std::find(
								_unit->getHostileUnits()->begin(),
								_unit->getHostileUnits()->end(),
								*i) != _unit->getHostileUnits()->end()))
				{
					losTrue = false;

					// is this gonna crash..................
					Position
						target,
						origin = _battleSave->getTileEngine()->getSightOriginVoxel(_unit);
					const int
						hasSight = static_cast<int>(_battleSave->getTileEngine()->canTargetUnit(
																							&origin,
																							(*i)->getTile(),
																							&target,
																							_unit)) * losFactor,
						// stupid aLiens don't know soldier's psiSkill tho..
						// psiSkill would typically factor in at only a fifth of psiStrength.
						defense = (*i)->getBaseStats()->psiStrength,
						dist = TileEngine::distance(
												(*i)->getPosition(),
												_unit->getPosition()) * 2,
						currentMood = RNG::generate(1,30);

					//Log(LOG_INFO) << ". . . ";
					//Log(LOG_INFO) << ". . . targetID = " << (*i)->getId();
					//Log(LOG_INFO) << ". . . defense = " << defense;
					//Log(LOG_INFO) << ". . . dist = " << dist;
					//Log(LOG_INFO) << ". . . currentMood = " << currentMood;
					//Log(LOG_INFO) << ". . . hasSight = " << hasSight;

					chance2 = attackStr // NOTE that this is NOT a true calculation of Success chance!
							- defense
							- dist
							+ currentMood
							+ hasSight;
					//Log(LOG_INFO) << ". . . chance2 = " << chance2;

					if (chance2 == chance
						&& (RNG::percent(50) == true
							|| _aggroTarget == NULL))
					{
						losTrue = (hasSight > 0);
						_aggroTarget = *i;
					}
					else if (chance2 > chance)
					{
						chance = chance2;

						losTrue = (hasSight > 0);
						_aggroTarget = *i;
					}
				}
			}

			if (losTrue == true)
				chance -= losFactor;

			if (_aggroTarget == NULL			// if no target
				|| chance < 26					// or chance of success too low
				|| RNG::percent(13) == true)	// or aLien just don't feel like it... do FALSE.
			{
				//Log(LOG_INFO) << "AlienBAIState::psiAction() EXIT, False : not good.";
				return false;
			}
/*kL
			if (_targetsVisible
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
//				&& chance < 30)	// esp. when aLien atkStr is low
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
				panicOdds += (RNG::generate(51,100) - (attackStr / 5));
				//Log(LOG_INFO) << ". . panicOdds_3 = " << panicOdds;
				if (RNG::percent(panicOdds) == true)
				{
					_psiAction->target = _aggroTarget->getPosition();
					_psiAction->type = BA_PSIPANIC;

					//Log(LOG_INFO) << "AlienBAIState::psiAction() EXIT . do Panic vs " << _aggroTarget->getId();
					return true;
				}
			}

			_psiAction->target = _aggroTarget->getPosition();
			_psiAction->type = BA_PSICONTROL;

			//Log(LOG_INFO) << "AlienBAIState::psiAction() EXIT . do MindControl vs " << _aggroTarget->getId();
			return true;
		}
	}

	//Log(LOG_INFO) << "AlienBAIState::psiAction() EXIT, False";
	return false;
}

/**
 * Validates a target.
 * @param unit			- pointer to a target to validate
 * @param assessDanger	- true to care if target has already been grenaded (default false)
 * @param includeCivs	- true to include civilians in the threat assessment (default false)
 * @return, true if the target is something to be killed
 */
bool AlienBAIState::validTarget( // private.
		const BattleUnit* const unit,
		bool assessDanger,
		bool includeCivs) const
{
	//Log(LOG_INFO) << "AlienBAIState::validTarget() ID " << unit->getId();
	if (unit->getFaction() == FACTION_HOSTILE				// target must not be on aLien side
		|| unit->isOut_t(OUT_STAT) == true					// ignore targets that are dead/unconscious
//		|| unit->isOut(true, true) == true
		|| unit->getExposed() == -1
		|| unit->getExposed() > _intelligence				// target must be a unit that this aLien 'knows about'
		|| (assessDanger == true
//			&& unit->getTile() != NULL						// safety. Should not be needed if isOut()=true
			&& unit->getTile()->getDangerous() == true))	// target has not been grenaded
	{
		return false;
	}

	//Log(LOG_INFO) << ". . ret = " << (unit->getFaction() == FACTION_PLAYER);
	return unit->getFaction() == FACTION_PLAYER
		|| includeCivs == true;
}

/**
 * Checks the alien's TU reservation setting.
 * @return, the reserve setting
 */
BattleActionType AlienBAIState::getReservedAIAction() const
{
	return _reserve;
}

/**
 * aLien has a dichotomy on its hands: has a ranged weapon as well as melee
 * ability ... so make a determination on which to use this round.
 */
void AlienBAIState::selectMeleeOrRanged() // private.
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

	const RuleItem* const meleeWeapon = _battleSave->getBattleGame()->getRuleset()->getItem(_unit->getMeleeWeapon());
	if (meleeWeapon == NULL)
	{
		// no idea how we got here, but melee is definitely out of the question.
		_melee = false;
		return;
	}

	int meleeOdds = 10;

	int power = meleeWeapon->getPower();
	if (meleeWeapon->isStrengthApplied() == true)
		power += _unit->getStrength();

	power = static_cast<int>(Round(
			static_cast<float>(power) * _aggroTarget->getArmor()->getDamageModifier(meleeWeapon->getDamageType())));

	if (power > 50)
		meleeOdds += (power - 50) / 2;

	if (_targetsVisible > 1)
		meleeOdds -= 15 * (_targetsVisible - 1);

	if (meleeOdds > 0
		&& _unit->getHealth() > _unit->getBaseStats()->health * 2 / 3)
	{
		if (_unit->getAggression() == 0)
			meleeOdds -= 20;
		else if (_unit->getAggression() > 1)
			meleeOdds += 10 * _unit->getAggression();

		if (RNG::percent(meleeOdds) == true)
		{
			_rifle = false;
			const int tuPreshot = _unit->getTimeUnits() - _unit->getActionTu(
																		BA_HIT,
																		meleeWeapon);
			Pathfinding* const pf = _battleSave->getPathfinding();
			pf->setPathingUnit(_unit);
			_reachableAttack = pf->findReachable(
											_unit,
											tuPreshot);
			return;
		}
	}

	_melee = false;
}

/**
 * Checks nearby nodes to see if they'd make good grenade targets.
 * @param action - pointer to BattleAction struct;
 * contents details one weapon and user and sets the target
 * @return, true if a viable node was found
 */
/*
bool AlienBAIState::getNodeOfBestEfficacy(BattleAction* action)
{
	if (_battleSave->getTurn() < 3) // <- note.
		return false;

	int bestScore = 2;

	Position
		originVoxel = _battleSave->getTileEngine()->getSightOriginVoxel(_unit),
		targetVoxel;

	for (std::vector<Node*>::const_iterator
			i = _battleSave->getNodes()->begin();
			i != _battleSave->getNodes()->end();
			++i)
	{
		int dist = _battleSave->getTileEngine()->distance(
													(*i)->getPosition(),
													_unit->getPosition());
		if (dist < 21
			&& dist > action->weapon->getRules()->getExplosionRadius()
			&& _battleSave->getTileEngine()->canTargetTile(
														&originVoxel,
														_battleSave->getTile((*i)->getPosition()),
														O_FLOOR,
														&targetVoxel,
														_unit))
		{
			int nodePoints = 0;

			for (std::vector<BattleUnit*>::const_iterator
					j = _battleSave->getUnits()->begin();
					j != _battleSave->getUnits()->end();
					++j)
			{
				dist = _battleSave->getTileEngine()->distance(
														(*i)->getPosition(),
														(*j)->getPosition());
				if ((*j)->isOut() == false
					&& dist < action->weapon->getRules()->getExplosionRadius())
				{
					Position targetOriginVoxel = _battleSave->getTileEngine()->getSightOriginVoxel(*j);
					if (_battleSave->getTileEngine()->canTargetTile(
																&targetOriginVoxel,
																_battleSave->getTile((*i)->getPosition()),
																O_FLOOR,
																&targetVoxel,
																*j))
					{
						if ((*j)->getFaction() != FACTION_HOSTILE)
						{
							if ((*j)->getExposed() <= _intelligence)
								++nodePoints;
						}
						else
							nodePoints -= 2;
					}
				}
			}

			if (nodePoints > bestScore)
			{
				bestScore = nodePoints;
				action->target = (*i)->getPosition();
			}
		}
	}

	return (bestScore > 2);
} */
/**
 * Gets the currently targeted unit.
 * @return, pointer to a BattleUnit
 */
/*BattleUnit* AlienBAIState::getTarget()
{
	return _aggroTarget;
}*/

}
