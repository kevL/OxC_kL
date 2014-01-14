/*
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

#include "AlienBAIState.h"

#include <algorithm>
#include <climits>
#include <cmath>

#include "ProjectileFlyBState.h"

#include "../Battlescape/BattlescapeState.h"
#include "../Battlescape/Map.h"
#include "../Battlescape/Pathfinding.h"
#include "../Battlescape/TileEngine.h"

#include "../Engine/Game.h"
#include "../Engine/Logger.h"
#include "../Engine/RNG.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/Armor.h"

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
 * Sets up a BattleAIState.
 * @param game Pointer to the game.
 * @param unit Pointer to the unit.
 * @param node Pointer to the node the unit originates from.
 */
AlienBAIState::AlienBAIState(
		SavedBattleGame* save,
		BattleUnit* unit,
		Node* node)
	:
		BattleAIState(
			save,
			unit),
		_aggroTarget(0),
		_knownEnemies(0),
		_visibleEnemies(0),
		_spottingEnemies(0),
		_escapeTUs(0),
		_ambushTUs(0),
		_reserveTUs(0),
		_rifle(false),
		_melee(false),
		_blaster(false),
		_grenade(false), // kL
		_wasHit(false),
		_didPsi(false),
		_AIMode(AI_PATROL),
		_closestDist(100),
		_fromNode(node),
		_toNode(0)
{
	//Log(LOG_INFO) << "Create AlienBAIState";

	_traceAI		= _save->getTraceSetting();

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
 * @param node YAML node.
 */
void AlienBAIState::load(const YAML::Node& node)
{
	int fromNodeID, toNodeID;

	fromNodeID	= node["fromNode"].as<int>(-1);
	toNodeID	= node["toNode"].as<int>(-1);
	_AIMode		= node["AIMode"].as<int>(0);
	_wasHit		= node["wasHit"].as<bool>(false);

	if (fromNodeID != -1)
	{
		_fromNode = _save->getNodes()->at(fromNodeID);
	}

	if (toNodeID != -1)
	{
		_toNode = _save->getNodes()->at(toNodeID);
	}
}

/**
 * Saves the AI state to a YAML file.
 * @param out YAML emitter.
 */
YAML::Node AlienBAIState::save() const
{
	int fromNodeID = -1, toNodeID = -1;
	if (_fromNode)
		fromNodeID = _fromNode->getID();
	if (_toNode)
		toNodeID = _toNode->getID();

	YAML::Node node;

	node["fromNode"]	= fromNodeID;
	node["toNode"]		= toNodeID;
	node["AIMode"]		= _AIMode;
	node["wasHit"]		= _wasHit;

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
 * @param action (possible) AI action to execute after thinking is done.
 */
void AlienBAIState::think(BattleAction* action)
{
	//Log(LOG_INFO) << "AlienBAIState::think(), unitID = " << _unit->getId();

 	action->type = BA_RETHINK;
	action->actor = _unit;
	action->weapon = _unit->getMainHandWeapon();	// kL_note: does not return grenades.
													// *cough* Does return grenades. -> DID return grenades but i fixed it so it won't.

	_attackAction->diff = static_cast<int>(_save->getBattleState()->getGame()->getSavedGame()->getDifficulty());
	_attackAction->actor = _unit;
	_attackAction->weapon = action->weapon;
	_attackAction->number = action->number;
	_escapeAction->number = action->number;

	_knownEnemies = countKnownTargets();
	_visibleEnemies = selectNearestTarget();
	_spottingEnemies = getSpottingUnits(_unit->getPosition());

	_melee = false;
	_rifle = false;
	_blaster = false;
	_grenade = false; // kL

	if (_traceAI)
	{
		Log(LOG_INFO) << "Unit has " << _visibleEnemies
				<< "/" << _knownEnemies << " known enemies visible, "
				<< _spottingEnemies << " of whom are spotting him. ";

		std::string AIMode;
		switch (_AIMode)
		{
			case 0:
				AIMode = "Patrol";
			break;
			case 1:
				AIMode = "Ambush";
			break;
			case 2:
				AIMode = "Combat";
			break;
			case 3:
				AIMode = "Escape";
			break;
		}

		Log(LOG_INFO) << "Currently using " << AIMode << " behaviour";
	}

	//Log(LOG_INFO) << ". . pos 1";
	if (action->weapon)
	{
		RuleItem* rule = action->weapon->getRules();
		if (rule->getBattleType() == BT_FIREARM)
		{
			if (!rule->isWaypoint())
				_rifle = true;
			else
				_blaster = true;
		}
		else if (rule->getBattleType() == BT_MELEE)
		{
			_melee = true;
		}
		else if (rule->getBattleType() == BT_GRENADE)		// kL
			_grenade = true;								// kL, this is no longer useful since I fixed
															// getMainHandWeapon() to not return grenades.
	}

	//Log(LOG_INFO) << ". . pos 2";
	if (_spottingEnemies
		&& !_escapeTUs)
	{
		setupEscape();
	}

	//Log(LOG_INFO) << ". . pos 3";
	if (_knownEnemies
		&& !_melee
		&& !_ambushTUs)
	{
		//Log(LOG_INFO) << ". . . . setupAmbush()";
		setupAmbush();
		//Log(LOG_INFO) << ". . . . setupAmbush() DONE";
	}

	//Log(LOG_INFO) << ". . . . setupAttack()";
	setupAttack(); // <- crash *was* here.
	//Log(LOG_INFO) << ". . . . setupAttack() DONE, setupPatrol()";
	setupPatrol();
	//Log(LOG_INFO) << ". . . . setupPatrol() DONE";

	//Log(LOG_INFO) << ". . pos 4";
	if (_psiAction->type != BA_NONE
		&& !_didPsi) // <- new crash in here.
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
	{
		_didPsi = false;
	}

	bool evaluate = false;

	//Log(LOG_INFO) << ". . pos 5";
	if (_AIMode == AI_ESCAPE)
	{
		if (!_spottingEnemies
			|| !_knownEnemies)
		{
			evaluate = true;
		}
	}
	else if (_AIMode == AI_PATROL)
	{
		if (_spottingEnemies
			|| _visibleEnemies
			|| _knownEnemies
			|| RNG::percent(10))
		{
			evaluate = true;
		}
	}
	else if (_AIMode == AI_AMBUSH)
	{
		if (!_rifle
			|| !_ambushTUs
			|| _visibleEnemies)
		{
			evaluate = true;
		}
	}

	//Log(LOG_INFO) << ". . pos 6";
	if (_AIMode == AI_COMBAT)
	{
		if (_attackAction->type == BA_RETHINK)
		{
			evaluate = true;
		}
	}

	if (_spottingEnemies > 2
		|| _unit->getHealth() < 2 * _unit->getStats()->health / 3
		|| (_aggroTarget && _aggroTarget->getTurnsExposed() > _intelligence))
	{
		evaluate = true;
	}

	if (_save->isCheating()
		&& _AIMode != AI_COMBAT)
	{
		evaluate = true;
	}

	//Log(LOG_INFO) << ". . pos 7";
	if (evaluate)
	{
		evaluateAIMode();

		if (_traceAI)
		{
			std::string AIMode;
			switch (_AIMode)
			{
				case 0:
					AIMode = "Patrol";
				break;
				case 1:
					AIMode = "Ambush";
				break;
				case 2:
					AIMode = "Combat";
				break;
				case 3:
					AIMode = "Escape";
				break;
			}

			Log(LOG_INFO) << "Re-Evaluated, now using " << AIMode << " behaviour";
		}
	}

	//Log(LOG_INFO) << ". . pos 8";
	switch (_AIMode)
	{
		case AI_ESCAPE:
			//Log(LOG_INFO) << ". . . . AI_ESCAPE";
			action->type = _escapeAction->type;
			action->target = _escapeAction->target;
			action->finalAction = true;			// end this unit's turn.
			action->desperate = true;			// ignore new targets.
			_unit->_hidingForTurn = true;		// spin 180 at the end of your route.
			// forget about reserving TUs, we need to get out of here.
			_save->getBattleGame()->setTUReserved(BA_NONE, false);
		break;
		case AI_PATROL:
			//Log(LOG_INFO) << ". . . . AI_PATROL";
			action->type = _patrolAction->type;
			action->target = _patrolAction->target;
		break;
		case AI_COMBAT:
			//Log(LOG_INFO) << ". . . . AI_COMBAT";
			action->type = _attackAction->type;
			action->target = _attackAction->target;
			action->weapon = _attackAction->weapon; // this may have changed to a grenade.

			if (action->weapon
				&& action->type == BA_THROW
				&& action->weapon->getRules()->getBattleType() == BT_GRENADE)
			{
				_unit->spendTimeUnits(_unit->getActionTUs(BA_PRIME, action->weapon));
			}

			action->finalFacing = _attackAction->finalFacing;									// if this is a firepoint action, set our facing.
			action->TU = _unit->getActionTUs(_attackAction->type, _attackAction->weapon);
			_save->getBattleGame()->setTUReserved(BA_NONE, false);								// don't worry about reserving TUs, we've factored that in already.

			if (action->type == BA_WALK															// if this is a "find fire point" action, don't increment the AI counter.
				&& _rifle
				&& _unit->getTimeUnits() > _unit->getActionTUs(BA_SNAPSHOT, action->weapon))	// so long as we can take a shot afterwards.
			{
				action->number -= 1;
			}
			else if (action->type == BA_LAUNCH)
			{
				action->waypoints = _attackAction->waypoints;
			}
		break;
		case AI_AMBUSH:
			//Log(LOG_INFO) << ". . . . AI_AMBUSH";
			action->type = _ambushAction->type;
			action->target = _ambushAction->target;
			action->finalFacing = _ambushAction->finalFacing;			// face where we think our target will appear.
			action->finalAction = true;									// end this unit's turn.
			_save->getBattleGame()->setTUReserved(BA_NONE, false);		// we've factored in the reserved TUs already, so don't worry.
		break;

		default:
		break;
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
		{
			action->type = BA_NONE;
		}
	}

	//Log(LOG_INFO) << "AlienBAIState::think() EXIT";
}

/**
 * sets the "was hit" flag to true.
 */
void AlienBAIState::setWasHit()
{
	_wasHit = true; 
}

/*
 * Gets whether the unit was hit.
 * @return if it was hit.
 */
bool AlienBAIState::getWasHit() const
{
	return _wasHit;
}

/**
 * Sets up a patrol action.
 * this is mainly going from node to node, moving about the map.
 * handles node selection, and fills out the _patrolAction with useful data.
 */
void AlienBAIState::setupPatrol()
{
	Node* node;
	_patrolAction->TU = 0;

	if (_toNode != 0
		&& _unit->getPosition() == _toNode->getPosition())
	{
		if (_traceAI)
		{
			Log(LOG_INFO) << "Patrol destination reached!";
		}

		// destination reached; head off to next patrol node
		_fromNode = _toNode;
		_toNode->freeNode();
		_toNode = 0;

		// take a peek through window before walking to the next node
		int dir = _save->getTileEngine()->faceWindow(_unit->getPosition());
		if (dir != -1
			&& dir != _unit->getDirection())
		{
			_unit->lookAt(dir);

			while (_unit->getStatus() == STATUS_TURNING)
			{
				_unit->turn();
			}
		}
	}

	if (_fromNode == 0)
	{
		// assume closest node as "from node"
		// on same level to avoid strange things, and the node has to match unit size or it will freeze
		int closest = 1000000;
		for (std::vector<Node*>::iterator
				i = _save->getNodes()->begin();
				i != _save->getNodes()->end();
				++i)
		{
			node = *i;
			int d = _save->getTileEngine()->distanceSq(
													_unit->getPosition(),
													node->getPosition());

			if (_unit->getPosition().z == node->getPosition().z 
				&& d < closest 
				&& (!(node->getType() & Node::TYPE_SMALL)
					|| _unit->getArmor()->getSize() == 1))
			{
				_fromNode = node;
				closest = d;
			}
		}
	}

	int triesLeft = 5;
	while (_toNode == 0
		&& triesLeft)
	{
		triesLeft--;

		// look for a new node to walk towards
		bool scout = true;

		if (_save->getMissionType() != "STR_BASE_DEFENSE")
		{
			// after turn 20 or if the morale is low, everyone moves out the UFO and scout

			// kL_note: That, above is wrong. Orig behavior depends on "aggression" setting;
			// determines whether aliens come out of UFO to scout/search.

			// also anyone standing in fire should also probably move
			if (_save->isCheating()
				|| !_fromNode
				|| _fromNode->getRank() == 0
				||  (_save->getTile(_unit->getPosition())
					&& _save->getTile(_unit->getPosition())->getFire()))
			{
				scout = true;
			}
			else
			{
				scout = false;
			}
		}

		// in base defense missions, the non-large aliens walk towards
		// target nodes - or once there, shoot objects thereabouts
		else if (_unit->getArmor()->getSize() == 1)
		{
			// can i shoot an object?
			if (_fromNode->isTarget()
				&& _unit->getMainHandWeapon()
				&& _unit->getMainHandWeapon()->getAmmoItem()->getRules()->getDamageType() != DT_HE
				&& _save->getModuleMap()[_fromNode->getPosition().x / 10][_fromNode->getPosition().y / 10].second > 0)
			{
				// scan this room for objects to destroy
				int x = (_unit->getPosition().x / 10) * 10;
				int y = (_unit->getPosition().y / 10) * 10;
				for (int
						i = x;
						i < x + 9;
						i++)
				{
					for (int
							j = y;
							j < y + 9;
							j++)
					{
						MapData* md = _save->getTile(Position(i, j, 1))->getMapData(MapData::O_OBJECT);
						if (md
							&& md->isBaseModule())
//							&& md->getDieMCD()
//							&& md->getArmor() < 60)
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
				for (std::vector<Node*>::iterator
						i = _save->getNodes()->begin();
						i != _save->getNodes()->end();
						++i)
				{
					if ((*i)->isTarget()
						&& !(*i)->isAllocated())
					{
						node = *i;
						int d = _save->getTileEngine()->distanceSq(_unit->getPosition(), node->getPosition());
						if (!_toNode
							|| (d < closest
								&& node != _fromNode))
						{
							_toNode = node;
							closest = d;
						}
					}
				}
			}
		}

		if (_toNode == 0)
		{
			_toNode = _save->getPatrolNode(
										scout,
										_unit,
										_fromNode);
			if (_toNode == 0)
			{
				_toNode = _save->getPatrolNode(
											!scout,
											_unit,
											_fromNode);
			}
		}

		if (_toNode != 0)
		{
			_save->getPathfinding()->calculate(_unit, _toNode->getPosition());

			if (_save->getPathfinding()->getStartDirection() == -1)
			{
				_toNode = 0;
			}

			_save->getPathfinding()->abortPath();
		}
	}

	if (_toNode != 0)
	{
		_toNode->allocateNode();

		_patrolAction->actor = _unit;
		_patrolAction->type = BA_WALK;
		_patrolAction->target = _toNode->getPosition();
	}
	else
	{
		_patrolAction->type = BA_RETHINK;
	}
}

/**
 * Try to set up an ambush action
 * The idea is to check within a 11x11 tile square for a tile which is not seen by our aggroTarget,
 * but that can be reached by him. we then intuit where we will see the target first from our covered
 * position, and set that as our final facing.
 * Fills out the _ambushAction with useful data.
 */
void AlienBAIState::setupAmbush()
{
	_ambushAction->type = BA_RETHINK;
	int bestScore = 0;
	_ambushTUs = 0;
	std::vector<int> path;

	if (selectClosestKnownEnemy())
	{
		Position target;
		const int BASE_SYSTEMATIC_SUCCESS = 100;
		const int COVER_BONUS = 25;
		const int FAST_PASS_THRESHOLD = 80;
		Position origin = _save->getTileEngine()->getSightOriginVoxel(_aggroTarget);

		// we'll use node positions for this, since it gives map makers a
		// good degree of control over how the units will use the environment.
		for (std::vector<Node*>::const_iterator
				i = _save->getNodes()->begin();
				i != _save->getNodes()->end();
				++i)
		{
			Position pos = (*i)->getPosition();
			Tile* tile = _save->getTile(pos);
			if (tile == 0
				|| _save->getTileEngine()->distance(pos, _unit->getPosition()) > 10
				|| pos.z != _unit->getPosition().z
				|| tile->getDangerous())
			{
				continue;
			}
			if (_traceAI) // colour all the nodes in range purple.
			{
				tile->setPreview(10);
				tile->setMarkerColor(13);
			}

			if (!_save->getTileEngine()->canTargetUnit(&origin, tile, &target, _aggroTarget, _unit)
				&& !getSpottingUnits(pos))												// make sure we can't be seen here.
			{
				_save->getPathfinding()->calculate(_unit, pos);
				int ambushTUs = _save->getPathfinding()->getTotalTUCost();

				if (_save->getPathfinding()->getStartDirection() != -1					// make sure we can move here
					&& ambushTUs <= _unit->getTimeUnits()
							- _unit->getActionTUs(BA_SNAPSHOT, _attackAction->weapon))	// make sure we can still shoot
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
							path = _save->getPathfinding()->_path;
							bestScore = score;
							_ambushTUs = (pos == _unit->getPosition()) ? 1 : ambushTUs;
							_ambushAction->target = pos;

							if (bestScore > FAST_PASS_THRESHOLD)
							{
								break;
							}
						}
					}
				}
			}
		}

		if (bestScore > 0)
		{
			_ambushAction->type = BA_WALK;

			// i should really make a function for this
			origin = (_ambushAction->target * Position(16,16,24)) + 
					// 4 because -2 is eyes and 2 below that is the rifle (or at least that's my understanding)
					Position(8, 8, _unit->getHeight() + _unit->getFloatHeight()
							- _save->getTile(_ambushAction->target)->getTerrainLevel() - 4);

			Position currentPos = _aggroTarget->getPosition();
			_save->getPathfinding()->setUnit(_aggroTarget);
			Position nextPos;

			size_t tries = path.size();
			while (tries > 0)  // hypothetically walk the target through the path.
			{
				_save->getPathfinding()->getTUCost(currentPos, path.back(), &nextPos, _aggroTarget, 0, false);
				path.pop_back();
				currentPos = nextPos;

				Tile* tile = _save->getTile(currentPos);

				Position target;
				// do a virtual fire calculation
				if (_save->getTileEngine()->canTargetUnit(&origin, tile, &target, _unit, _aggroTarget))
				{
					// if we can virtually fire at the hypothetical target, we know which way to face.
					_ambushAction->finalFacing = _save->getTileEngine()->getDirectionTo(_ambushAction->target, currentPos);

					break;
				}

				--tries;
			}

			if (_traceAI) Log(LOG_INFO) << "Ambush estimation will move to " << _ambushAction->target;

			return;
		}
	}

	if (_traceAI) Log(LOG_INFO) << "Ambush estimation failed";
}

/**
 * Try to set up a combat action
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
	if (_knownEnemies)
	{
		if (_unit->getStats()->psiSkill
			&& psiAction()
			&& !_didPsi) // kL, also checked in AlienBAIState::psiAction().
		{
			//Log(LOG_INFO) << "do Psi attack";
			// at this point we can save some time with other calculations - the unit WILL make a psionic attack this turn.
			return;
		}

		if (_blaster)
		{
			//Log(LOG_INFO) << "do Blaster launch";
			wayPointAction();
		}
	}

	// if we CAN see someone, that makes them a viable target for "regular" attacks.
	//Log(LOG_INFO) << ". . selectNearestTarget()";
	if (selectNearestTarget())
	{
		//Log(LOG_INFO) << ". . . . selectNearestTarget()";

		if (_unit->getGrenadeFromBelt())
		{
			//Log(LOG_INFO) << ". . . . . . grenadeAction()";
			grenadeAction();
			//Log(LOG_INFO) << ". . . . . . grenadeAction() DONE";
		}

		if (_melee)
		{
			//Log(LOG_INFO) << ". . . . . . meleeAction()";
			meleeAction();
			//Log(LOG_INFO) << ". . . . . . meleeAction() DONE";
		}

		if (_rifle)
		{
			//Log(LOG_INFO) << ". . . . . . projectileAction()";
			projectileAction();
			//Log(LOG_INFO) << ". . . . . . projectileAction() DONE";
		}
	}
	//Log(LOG_INFO) << ". . selectNearestTarget() DONE";

	if (_attackAction->type != BA_RETHINK)
	{
		if (_traceAI)
		{
			if (_attackAction->type != BA_WALK) Log(LOG_INFO) << "Attack estimation desires to shoot at " << _attackAction->target;
			else Log(LOG_INFO) << "Attack estimation desires to move to " << _attackAction->target;
		}

		return;
	}
	else if (_spottingEnemies
		|| _unit->getAggression() < RNG::generate(0, 3))
	{
		if (findFirePoint()) // if enemies can see us, or if we're feeling lucky, we can try to spot the enemy.
		{
			if (_traceAI) Log(LOG_INFO) << "Attack estimation desires to move to " << _attackAction->target;

			return;
		}
	}

	if (_traceAI) Log(LOG_INFO) << "Attack estimation failed";
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
	int
		unitsSpottingMe = getSpottingUnits(_unit->getPosition()),
		currentTilePreference = 15,
		tries = -1;

	selectNearestTarget();

	int dist = 0;
	if (_aggroTarget)
		dist = _save->getTileEngine()->distance(
											_unit->getPosition(),
											_aggroTarget->getPosition());

	int
		bestTileScore = -100000,
		score = -100000;

	Position bestTile(0, 0, 0);
	Tile* tile = 0;

	// weights of various factors in choosing a tile to which to withdraw
	const int
		EXPOSURE_PENALTY			= 10,
		FIRE_PENALTY				= 40,
		BASE_SYSTEMATIC_SUCCESS		= 100,
		BASE_DESPERATE_SUCCESS		= 110,
		FAST_PASS_THRESHOLD			= 100;
		// a score that's good enough to quit the while loop early;
		// it's subjective, hand-tuned and may need tweaking

	int tu = _unit->getTimeUnits() / 2;
	std::vector<int> reachable = _save->getPathfinding()->findReachable(_unit, tu);
	std::vector<Position> randomTileSearch = _save->getTileSearch();
	std::random_shuffle(
					randomTileSearch.begin(),
					randomTileSearch.end());

	while (tries < 150 && !coverFound)
	{
		_escapeAction->target = _unit->getPosition(); // start looking in a direction away from the enemy
		if (!_save->getTile(_escapeAction->target))
		{
			_escapeAction->target = _unit->getPosition(); // cornered at the edge of the map perhaps?
		}

		score = 0;

		if (tries == -1)
		{
			// you know, maybe we should just stay where we are and not risk reaction fire...
			// or maybe continue to wherever we were running to and not risk looking stupid
			if (_save->getTile(_unit->lastCover) != 0)
			{
				_escapeAction->target = _unit->lastCover;
			}
		}
		else if (tries < 121) 
		{
			// looking for cover
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
				{
					score += currentTilePreference;
				}
			}
		}
		else
		{
			if (tries == 121)
			{
				if (_traceAI)
				{
					Log(LOG_INFO) << "best score after systematic search was: " << bestTileScore;
				}
			}

			score = BASE_DESPERATE_SUCCESS; // ruuuuuuun!!1

			_escapeAction->target = _unit->getPosition();
			_escapeAction->target.x += RNG::generate(-10, 10);
			_escapeAction->target.y += RNG::generate(-10, 10);
			_escapeAction->target.z = _unit->getPosition().z + RNG::generate(-1, 1);

			if (_escapeAction->target.z < 0)
			{
				_escapeAction->target.z = 0;
			}
			else if (_escapeAction->target.z >= _save->getMapSizeZ())
			{
				_escapeAction->target.z = _unit->getPosition().z;
			}
		}

		tries++;

		// THINK, DAMN YOU
		tile = _save->getTile(_escapeAction->target);
		int distTarget = 0;
		if (_aggroTarget)
			distTarget = _save->getTileEngine()->distance(
													_aggroTarget->getPosition(),
													_escapeAction->target);
		if (dist >= distTarget)
		{
			score -= (distTarget - dist) * 10;
		}
		else
		{
			score += (distTarget - dist) * 10;
		}

		int spotters = 0;

		if (!tile)
		{
			score = -100001; // no you can't quit the battlefield by running off the map.
		}
		else
		{
			spotters = getSpottingUnits(_escapeAction->target);

			if (std::find(reachable.begin(),
					reachable.end(),
					_save->getTileIndex(tile->getPosition())) == reachable.end())
			{
				continue; // just ignore unreachable tiles
			}

			if (_spottingEnemies || spotters)
			{
				if (_spottingEnemies <= spotters)
				{
					score -= (1 + spotters - _spottingEnemies) * EXPOSURE_PENALTY; // that's for giving away our position
				}
				else
				{
					score += (_spottingEnemies - spotters) * EXPOSURE_PENALTY;
				}
			}

			if (tile->getFire())
			{
				score -= FIRE_PENALTY;
			}

			if (tile->getDangerous())
			{
				score -= BASE_SYSTEMATIC_SUCCESS;
			}

			if (_traceAI)
			{
				tile->setMarkerColor(score < 0? 3:(score < FAST_PASS_THRESHOLD / 2? 8:(score < FAST_PASS_THRESHOLD? 9:5)));
				tile->setPreview(10);
				tile->setTUMarker(score);
			}

		}

		if (tile
			&& score > bestTileScore)
		{
			// calculate TUs to tile; we could be getting this from findReachable() somehow but that would break something for sure...
			_save->getPathfinding()->calculate(
											_unit,
											_escapeAction->target,
											0,
											tu);

			if (_escapeAction->target == _unit->getPosition()
				|| _save->getPathfinding()->getStartDirection() != -1)
			{
				bestTileScore = score;
				bestTile = _escapeAction->target;

				_escapeTUs = _save->getPathfinding()->getTotalTUCost();
				if (_escapeAction->target == _unit->getPosition())
				{
					_escapeTUs = 1;
				}

				if (_traceAI)
				{
					tile->setMarkerColor(score < 0? 7:(score < FAST_PASS_THRESHOLD / 2? 10:(score < FAST_PASS_THRESHOLD? 4:5)));
					tile->setPreview(10);
					tile->setTUMarker(score);
				}
			}

			_save->getPathfinding()->abortPath();

			if (bestTileScore > FAST_PASS_THRESHOLD)
				coverFound = true; // good enough, gogogo
		}
	}

	_escapeAction->target = bestTile;

	if (_traceAI) _save->getTile(_escapeAction->target)->setMarkerColor(13);

	if (bestTileScore <= -100000)
	{
		if (_traceAI) Log(LOG_INFO) << "Escape estimation failed.";

		_escapeAction->type = BA_RETHINK; // do something, just don't look dumbstruck :P

		return;
	}
	else
	{
		if (_traceAI) Log(LOG_INFO) << "Escape estimation completed after " << tries << " tries, "
				<< _save->getTileEngine()->distance(_unit->getPosition(), bestTile) << " squares or so away.";

		_escapeAction->type = BA_WALK;
	}
}

/**
 * Counts how many targets, both xcom and civilian are known to this unit
 * @return how many targets are known to us.
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
					true))
		{
			++knownEnemies;
		}
	}

	return knownEnemies;
}

/**
 * counts how many enemies (xcom only) are spotting any given position.
 * @param pos the Position to check for spotters.
 * @return spotters.
 */
int AlienBAIState::getSpottingUnits(Position pos) const
{
	// if we don't actually occupy the position being checked, we need to do a virtual LOF check.
	bool checking = pos != _unit->getPosition();
	int tally = 0;

	for (std::vector<BattleUnit*>::const_iterator
			i = _save->getUnits()->begin();
			i != _save->getUnits()->end();
			++i)
	{
		if (validTarget(*i, false, false))
		{
			int dist = _save->getTileEngine()->distance(
													pos,
													(*i)->getPosition());
			if (dist > 20) continue;

			Position originVoxel = _save->getTileEngine()->getSightOriginVoxel(*i);
			originVoxel.z -= 2;
			Position targetVoxel;

			if (checking)
			{
				if (_save->getTileEngine()->canTargetUnit(
														&originVoxel,
														_save->getTile(pos),
														&targetVoxel,
														*i,
														_unit))
				{
					tally++;
				}
			}
			else
			{
				if (_save->getTileEngine()->canTargetUnit(
														&originVoxel,
														_save->getTile(pos),
														&targetVoxel,
														*i))
				{
					tally++;
				}
			}
		}
	}

	return tally;
}

/**
 * Selects the nearest known living target we can see/reach
 * and returns the number of visible enemies.
 * This function includes civilians as viable targets.
 * @return int, Viable targets.
 */
int AlienBAIState::selectNearestTarget()
{
	int tally = 0;

	_aggroTarget = 0;
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
					true)
			&& _save->getTileEngine()->visible(
											_unit,
											(*i)->getTile()))
		{
			tally++;

			int dist = _save->getTileEngine()->distance(
													_unit->getPosition(),
													(*i)->getPosition());
			if (dist < _closestDist)
			{
				bool validTarget = false;
				if (_rifle
					|| !_melee)
				{
					validTarget = _save->getTileEngine()->canTargetUnit(
																	&origin,
																	(*i)->getTile(),
																	&target,
																	_unit);
				}
				else
				{
					if (selectPointNearTarget(*i, _unit->getTimeUnits()))
					{
						int dir = _save->getTileEngine()->getDirectionTo(
																	_attackAction->target,
																	(*i)->getPosition());
						validTarget = _save->getTileEngine()->validMeleeRange(
																		_attackAction->target,
																		dir,
																		_unit,
																		*i,
																		0);
					}
				}

				if (validTarget)
				{
					_closestDist = dist;
					_aggroTarget = *i;
				}
			}
		}
	}

	if (_aggroTarget)
	{
		return tally;
	}

	return 0;
}

/**
 * Selects the nearest known living Xcom unit.
 * used for ambush calculations
 * @return if we found one.
 */
bool AlienBAIState::selectClosestKnownEnemy()
{
	_aggroTarget = 0;
	int minDist = 255;

	for (std::vector<BattleUnit*>::iterator
			i = _save->getUnits()->begin();
			i != _save->getUnits()->end();
			++i)
	{
		if (validTarget(*i, true, false))
		{
			int dist = _save->getTileEngine()->distance(
													(*i)->getPosition(),
													_unit->getPosition());
			if (dist < minDist)
			{
				minDist = dist;
				_aggroTarget = *i;
			}
		}
	}

	return _aggroTarget != 0;
}

/**
 * Selects a random known living Xcom or civilian unit.
 * @return if we found one.
 */
bool AlienBAIState::selectRandomTarget()
{
	int farthest = -100;
	_aggroTarget = 0;

	for (std::vector<BattleUnit*>::const_iterator
			i = _save->getUnits()->begin();
			i != _save->getUnits()->end();
			++i)
	{
		if (validTarget(
					*i,
					true,
					true))
		{
			int dist = RNG::generate(0, 20)
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

	return _aggroTarget != 0;
}

/**
 * Selects a point near enough to our target to perform a melee attack.
 * @param target Pointer to a target.
 * @param maxTUs Maximum time units the path to the target can cost.
 * @return True if a point was found.
 */
bool AlienBAIState::selectPointNearTarget(
		BattleUnit* target,
		int maxTUs) const
{
	bool returnValue = false;

	int size = _unit->getArmor()->getSize();
	int targetsize = target->getArmor()->getSize();
	int distance = 1000;

	for (int
			x = -size;
			x <= targetsize;
			++x)
	{
		for (int
				y = -size;
				y <= targetsize;
				++y)
		{
			if (x || y) // skip the unit itself
			{
				Position checkPath = target->getPosition() + Position (x, y, 0);

				int dir = _save->getTileEngine()->getDirectionTo(checkPath, target->getPosition());
				bool valid = _save->getTileEngine()->validMeleeRange(checkPath, dir, _unit, target, 0);
				bool fitHere = _save->setUnitPosition(_unit, checkPath, true);
				if (valid
					&& fitHere
					&& !_save->getTile(checkPath)->getDangerous())
				{
					_save->getPathfinding()->calculate(_unit, checkPath, 0, maxTUs);

					if (_save->getPathfinding()->getStartDirection() != -1
						&& _save->getTileEngine()->distance(checkPath, _unit->getPosition()) < distance)
					{
						returnValue = true;

						_attackAction->target = checkPath;
						distance = _save->getTileEngine()->distance(checkPath, _unit->getPosition());
					}

					_save->getPathfinding()->abortPath();
				}
			}
		}
	}

	return returnValue;
}

/**
 * Selects an AI mode based on a number of factors,
 * some RNG and the results of the rest of the determinations.
 */
void AlienBAIState::evaluateAIMode()
{
	// we don't run and hide on our first action.
	float escapeOdds = _escapeAction->number == 1? 0.f: 15.f;
	float ambushOdds = 12.f;
	float combatOdds = 20.f;

	// we're less likely to patrol if we see enemies.
	float patrolOdds = _visibleEnemies? 15.f: 30.f;

	// the enemy sees us, we should take retreat into consideration, and forget about patrolling for now.
	if (_spottingEnemies)
	{
		patrolOdds = 0.f;
		if (_escapeTUs == 0)
		{
			setupEscape();
		}
	}

	// melee/blaster units shouldn't consider ambush
	if (!_rifle
		|| _ambushTUs == 0)
	{
		ambushOdds = 0.f;

		if (_melee)
		{
//kL			escapeOdds = 12.f;
//kL			combatOdds *= 1.3f;
			escapeOdds = 10.f; // kL
			combatOdds *= 1.2f; // kL
		}
	}

	// if we KNOW there are enemies around...
	if (_knownEnemies)
	{
		if (_knownEnemies == 1)
		{
//kL			combatOdds *= 1.2f;
			combatOdds *= 1.8f; // kL
		}

		if (_escapeTUs == 0)
		{
			if (selectClosestKnownEnemy())
			{
				setupEscape();
			}
			else
			{
				escapeOdds = 0.f;
			}
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
//kL			patrolOdds *= 1.1f;
			patrolOdds *= 1.2f; // kL
		break;
		case AI_AMBUSH:
//kL			ambushOdds *= 1.1f;
			ambushOdds *= 1.2f; // kL
		break;
		case AI_COMBAT:
//kL			combatOdds *= 1.1f;
			combatOdds *= 1.2f; // kL
		break;
		case AI_ESCAPE:
//kL			escapeOdds *= 1.1f;
			escapeOdds *= 1.2f; // kL
		break;
	}

	// take our overall health into consideration
	if (_unit->getHealth() < _unit->getStats()->health / 3)
	{
//kL		escapeOdds *= 1.7f;
//kL		combatOdds *= 0.6f;
//kL		ambushOdds *= 0.75f;
		escapeOdds *= 1.8f; // kL
		combatOdds *= 0.5f; // kL
		ambushOdds *= 0.7f; // kL
	}
	else if (_unit->getHealth() < _unit->getStats()->health * 2 / 3)
	{
//kL		escapeOdds *= 1.4f;
//kL		combatOdds *= 0.8f;
//kL		ambushOdds *= 0.8f;
		escapeOdds *= 1.5f; // kL
		combatOdds *= 0.9f; // kL
		ambushOdds *= 0.8f; // kL
	}
	else if (_unit->getHealth() < _unit->getStats()->health)
	{
//kL		escapeOdds *= 1.1f;
		escapeOdds *= 1.3f; // kL
	}

	switch (_unit->getAggression()) // take our aggression into consideration
	{
		case 0:
//kL			escapeOdds *= 1.4f;
//kL			combatOdds *= 0.7f;
			escapeOdds *= 1.5f; // kL
			combatOdds *= 0.5f; // kL
		break;
		case 1:
//kL			ambushOdds *= 1.1f;
			ambushOdds *= 1.2f; // kL
		break;
		case 2:
//kL			combatOdds *= 1.4f;
//kL			escapeOdds *= 0.7f;
			combatOdds *= 1.5f; // kL
			escapeOdds *= 0.5f; // kL
		break;
	}

	if (_AIMode == AI_COMBAT)
	{
		ambushOdds *= 1.5f;
	}

	if (_spottingEnemies) // factor in the spotters.
	{
		escapeOdds *= (10.f * static_cast<float>(_spottingEnemies + 10) / 100.f);
		combatOdds *= (5.f * static_cast<float>(_spottingEnemies + 20) / 100.f);
	}
	else
	{
		escapeOdds /= 2.f;
	}

	if (_visibleEnemies) // factor in visible enemies.
	{
		combatOdds *= (10.f * static_cast<float>(_visibleEnemies + 10) / 100.f);

//kL		if (_closestDist < 5)
		if (_closestDist < 6) // kL
		{
			ambushOdds = 0.f;
		}
	}

	if (_ambushTUs) // make sure we have an ambush lined up, or don't even consider it.
	{
//kL		ambushOdds *= 1.7f;
		ambushOdds *= 2.f; // kL
	}
	else
	{
		ambushOdds = 0.f;
	}

	// factor in mission type
	if (_save->getMissionType() == "STR_BASE_DEFENSE")
	{
//kL		escapeOdds *= 0.75f;
//kL		ambushOdds *= 0.6f;
		escapeOdds *= 0.8f; // kL
		ambushOdds *= 0.5f; // kL
	}

	// generate a random number to represent our decision.
	int decision = RNG::generate(
								1,
								static_cast<int>(patrolOdds + ambushOdds + escapeOdds + combatOdds) + 1);

	// if the aliens are cheating, or the unit is charging, enforce combat as a priority.
	if (_save->isCheating() || _unit->getCharging() != 0)
	{
		_AIMode = AI_COMBAT;
	}
	else if (static_cast<float>(decision) > escapeOdds)
	{
		if (static_cast<float>(decision) > escapeOdds + ambushOdds)
		{
			if (static_cast<float>(decision) > escapeOdds + ambushOdds + combatOdds)
			{
				_AIMode = AI_PATROL;
			}
			else
			{
				_AIMode = AI_COMBAT;
			}
		}
		else
		{
			_AIMode = AI_AMBUSH;
		}
	}
	else
	{
		_AIMode = AI_ESCAPE;
	}

	// enforce the validity of our decision, and try fallback behaviour according to priority.
	if (_AIMode == AI_COMBAT)
	{
		if (_aggroTarget)
		{
			if (_attackAction->type != BA_RETHINK)
			{
				return;
			}

			if (findFirePoint())
			{
				return;
			}
		}
		else if (selectRandomTarget()
			&& findFirePoint())
		{
			return;
		}

		_AIMode = AI_PATROL;
	}

	if (_AIMode == AI_PATROL)
	{
		if (_toNode)
		{
			return;
		}

		_AIMode = AI_AMBUSH;
	}

	if (_AIMode == AI_AMBUSH)
	{
		if (_ambushTUs != 0)
		{
			return;
		}

		_AIMode = AI_ESCAPE;
	}
}

/**
 * Find a position where we can see our target, and move there.
 * check the 11x11 grid for a position nearby where we can potentially target him.
 */
bool AlienBAIState::findFirePoint()
{
	if (!selectClosestKnownEnemy())
		return false;

	std::vector<Position> randomTileSearch = _save->getTileSearch();
	std::random_shuffle(randomTileSearch.begin(), randomTileSearch.end());

	const int BASE_SYSTEMATIC_SUCCESS	= 100;
	const int FAST_PASS_THRESHOLD		= 125;

	Position target;
	_attackAction->type = BA_RETHINK;

	int bestScore = 0;

	for (std::vector<Position>::const_iterator
			i = randomTileSearch.begin();
			i != randomTileSearch.end();
			++i)
	{
		Position pos = _unit->getPosition() + *i;

		Tile* tile = _save->getTile(pos);
		if (tile == 0) continue;

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

		if (_save->getTileEngine()->canTargetUnit(&origin, _aggroTarget->getTile(), &target, _unit))
		{
			_save->getPathfinding()->calculate(_unit, pos);

			if (_save->getPathfinding()->getStartDirection() != -1						// can move here
				&& _save->getPathfinding()->getTotalTUCost() <= _unit->getTimeUnits())	// can still shoot
			{
				score = BASE_SYSTEMATIC_SUCCESS - getSpottingUnits(pos) * 10;
				score += _unit->getTimeUnits() - _save->getPathfinding()->getTotalTUCost();

				if (!_aggroTarget->checkViewSector(pos))
				{
					score += 10;
				}

				if (score > bestScore)
				{
					bestScore = score;

					_attackAction->target = pos;
					_attackAction->finalFacing = _save->getTileEngine()->getDirectionTo(pos, _aggroTarget->getPosition());

					if (score > FAST_PASS_THRESHOLD)
					{
						break;
					}
				}
			}
		}
	}

	if (bestScore > 70)
	{
		_attackAction->type = BA_WALK;
		if (_traceAI)
		{
			Log(LOG_INFO) << "Firepoint found at " << _attackAction->target << ", with a score of: " << bestScore;
		}

		return true;
	}

	if (_traceAI)
	{
		Log(LOG_INFO) << "Firepoint failed, best estimation was: " << _attackAction->target << ", with a score of: " << bestScore;
	}

	return false;
}

/**
 * Decides if it's worth our while to create an explosion here.
 * @param targetPos, The target's position.
 * @param attackingUnit, The attacking unit.
 * @param radius, How big the explosion will be.
 * @param diff, Game difficulty.
 * @return, True if it is worthwile creating an explosion in the target position.
 */
bool AlienBAIState::explosiveEfficacy(
		Position targetPos,
		BattleUnit* attackingUnit,
		int radius,
		int diff) const
//		bool grenade) const
{
	Log(LOG_INFO) << "AlienBAIState::explosiveEfficacy()";

	if (diff == -1)
	{
		diff = static_cast<int>(_save->getBattleState()->getGame()->getSavedGame()->getDifficulty());
	}
	// i hate the player and i want him dead, but i don't want to piss him off:
//kL	if (_save->getTurn() < 3) return false;
	if (_save->getTurn() < 5 - diff) return false;	// kL


	// if we're below 1/3 health, let's assume things are dire, and increase desperation.
	int desperation = (100 - attackingUnit->getMorale()) / 10;
//kL	int hurt = attackingUnit->getStats()->health - attackingUnit->getHealth();
	int hurt = 10 -
			static_cast<int>(
					static_cast<float>(attackingUnit->getHealth()) / static_cast<float>(attackingUnit->getStats()->health) * 10.f);
//kL	if (hurt > attackingUnit->getStats()->health * 2 / 3)
//kL		desperation += 3;

//kL	int eff = desperation + affected; // kL_note: no affected yet...
	int eff = (desperation + hurt) * 2;

	int distance = _save->getTileEngine()->distance(attackingUnit->getPosition(), targetPos);
	if (distance < radius + 1) // attacker inside blast zone
	{
//kL		eff -= 4;
//		eff -= 50;		// kL
		eff -= 35;		// kL
		if (attackingUnit->getPosition().z == targetPos.z)
		{
			eff -= 15;		// kL
		}
	}

	// we don't want to ruin our own base, but we do want to ruin XCom's day.
	if (_save->getMissionType() == "STR_ALIEN_BASE_ASSAULT")
	{
//kL		eff -= 3;
		eff -= 25;
	}
	else if (_save->getMissionType() == "STR_BASE_DEFENSE"
		|| _save->getMissionType() == "STR_TERROR_MISSION")
	{
//kL		eff += 3;
		eff += 50;
	}


//kL	int affected = 0;

	BattleUnit* target = _save->getTile(targetPos)->getUnit();
	for (std::vector<BattleUnit*>::iterator
			i = _save->getUnits()->begin();
			i != _save->getUnits()->end();
			++i)
	{
		if (!(*i)->isOut(true)
			&& *i != attackingUnit
			&& (*i)->getPosition().z == targetPos.z
			&& _save->getTileEngine()->distance((*i)->getPosition(), targetPos) < radius + 1)
		{
			if ((*i)->getFaction() == FACTION_PLAYER
				&& (*i)->getTurnsExposed() > _intelligence)
			{
				continue;
			}

			Position voxelPosA = Position(
									(targetPos.x * 16) + 8,
									(targetPos.y * 16) + 8,
									(targetPos.z * 24) + 12);
			Position voxelPosB = Position(
									((*i)->getPosition().x * 16) + 8,
									((*i)->getPosition().y * 16) + 8,
									((*i)->getPosition().z * 24) + 12);

			int collision = _save->getTileEngine()->calculateLine(
														voxelPosA,
														voxelPosB,
														false,
														0,
														target,
														true,
														false,
														*i);
			if (collision == VOXEL_UNIT)
			{
				if ((*i)->getFaction() == FACTION_PLAYER)
				{
					eff += 10;
//kL					++eff;
//kL					++affected;
				}
//kL				else if ((*i)->getFaction() == attackingUnit->getFaction())
				else if ((*i)->getOriginalFaction() == attackingUnit->getFaction())		// kL
				{
//kL					eff -= 2;	// friendlies count double
					eff -= 5;		// true friendlies count half
				}
			}
		}
	}

	// spice things up a bit by adding a random number based on difficulty level
//	eff += RNG::generate(0, diff + 1) - RNG::generate(0, 5);
//	if (eff > 0 || affected >= 10)
//	if (eff > 0)
//		|| affected >= 3)	// kL

	// don't throw grenades at single targets, unless morale is in the danger zone,
	// or we're halfway towards panicking while bleeding to death.
//kL	if (grenade
//kL		&& desperation < 6
//kL		&& enemiesAffected < 2)
//kL	{
//kL		return false;
//kL	}

	if (eff > 0					// kL
		&& RNG::percent(eff))	// kL
	{
		Log(LOG_INFO) << "AlienBAIState::explosiveEfficacy() EXIT true, eff = " << eff;
		return true;
	}

	Log(LOG_INFO) << "AlienBAIState::explosiveEfficacy() EXIT false, eff = " << eff;
	return false;
}

/**
 * Attempts to take a melee attack/charge an enemy we can see.
 * Melee targetting: we can see an enemy, we can move to it so we're charging blindly toward an enemy.
 */
void AlienBAIState::meleeAction()
{
	if (_aggroTarget != 0
		&& !_aggroTarget->isOut(true, true))
	{
		if (_save->getTileEngine()->validMeleeRange(_unit, _aggroTarget, _unit->getDirection()))
		{
			meleeAttack();

			return;
		}
	}

	int attackCost		= _unit->getActionTUs(BA_HIT, _unit->getMainHandWeapon());
	int chargeReserve	= _unit->getTimeUnits() - attackCost;
	int distance		= (chargeReserve / 4) + 1;

	_aggroTarget = 0;

	for (std::vector<BattleUnit*>::const_iterator
			i = _save->getUnits()->begin();
			i != _save->getUnits()->end();
			++i)
	{
		int newDistance = _save->getTileEngine()->distance(_unit->getPosition(), (*i)->getPosition());
		if (newDistance > 20
			|| !validTarget(*i, true, true))
		{
			continue;
		}

		// pick closest living unit that we can move to
		if ((newDistance < distance
				|| newDistance == 1)
			&& !(*i)->isOut(true, true))
		{
			if (newDistance == 1 || selectPointNearTarget(*i, chargeReserve))
			{
				_aggroTarget = (*i);
				_attackAction->type = BA_WALK;
				_unit->setCharging(_aggroTarget);

				distance = newDistance;
			}
		}
	}

	if (_aggroTarget != 0)
	{
		if (_save->getTileEngine()->validMeleeRange(
											_unit,
											_aggroTarget,
											_save->getTileEngine()->getDirectionTo(_unit->getPosition(),
											_aggroTarget->getPosition())))
		{
			meleeAttack();
		}
	}

	if (_traceAI && _aggroTarget)
	{
		Log(LOG_INFO) << "AlienBAIState::meleeAction:"
				<< " [target]: " << (_aggroTarget->getId())
				<< " at: "  << _attackAction->target;
		Log(LOG_INFO) << "CHARGE!";
	}
}

/**
 * Attempts to fire a waypoint projectile at an enemy that is seen (by any aLien).
 * Waypoint targetting: pick from any units currently spotted by our allies.
 */
void AlienBAIState::wayPointAction()
{
	_aggroTarget = 0;

	for (std::vector<BattleUnit*>::const_iterator
			i = _save->getUnits()->begin();
			i != _save->getUnits()->end()
				&& _aggroTarget == 0;
			++i)
	{
		if (!validTarget(
						*i,
						true,
						true))
		{
			continue;
		}

		_save->getPathfinding()->calculate(
										_unit,
										(*i)->getPosition(),
										*i,
										-1);

		if (_save->getPathfinding()->getStartDirection() != -1
			&& explosiveEfficacy((*i)->getPosition(),
												_unit,
												(_unit->getMainHandWeapon()->getAmmoItem()->getRules()->getPower() / 20) + 1,
												_attackAction->diff))
		{
			_aggroTarget = *i;
		}

		_save->getPathfinding()->abortPath();
	}


	if (_aggroTarget != 0)
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
			lastPt = _unit->getPosition(),
			lastPos = _unit->getPosition(),
			curPos = _unit->getPosition(),
			dirVector;

		int
			collision,
			pathDir = _save->getPathfinding()->dequeuePath();

		while (pathDir != -1)
		{
			lastPos = curPos;

			_save->getPathfinding()->directionToVector(
													pathDir,
													&dirVector);

			curPos = curPos + dirVector;

			Position
				voxelPosA(
						(curPos.x * 16) + 8,
						(curPos.y * 16) + 8,
						(curPos.z * 24) + 16),
				voxelPosB(
						(lastPt.x * 16) + 8,
						(lastPt.y * 16) + 8,
						(lastPt.z * 24) + 16);

			collision = _save->getTileEngine()->calculateLine(
															voxelPosA,
															voxelPosB,
															false,
															0,
															_unit,
															true);

			if (collision > VOXEL_EMPTY
				&& collision < VOXEL_UNIT)
			{
				_attackAction->waypoints.push_back(lastPos);

				lastPt = lastPos;
			}
			else if (collision == VOXEL_UNIT)
			{
				BattleUnit* target = _save->getTile(curPos)->getUnit();
				if (target == _aggroTarget)
				{
					_attackAction->waypoints.push_back(curPos);

					lastPt = curPos;
				}
			}

			pathDir = _save->getPathfinding()->dequeuePath();
		}

		_attackAction->target = _attackAction->waypoints.front();

		if (static_cast<int>(_attackAction->waypoints.size()) > (_attackAction->diff * 2) + 6
			|| lastPt != _aggroTarget->getPosition())
		{
			_attackAction->type = BA_RETHINK;
		}
	}
}

/**
 * Attempts to fire at an enemy we can see.
 * Regular targetting: we can see an enemy, we have a gun, let's try to shoot.
 */
void AlienBAIState::projectileAction()
{
	_attackAction->target = _aggroTarget->getPosition();

	if (!_attackAction->weapon->getAmmoItem()->getRules()->getExplosionRadius()
		|| explosiveEfficacy(
						_aggroTarget->getPosition(),
						_unit,
						_attackAction->weapon->getAmmoItem()->getRules()->getExplosionRadius(),
						_attackAction->diff))
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

	int tuSnap	= _attackAction->weapon->getRules()->getTUSnap();	// least TU
	int tuAuto	= _attackAction->weapon->getRules()->getTUAuto();	// middle TU
	int tuAimed	= _attackAction->weapon->getRules()->getTUAimed();	// most TU

	int usableTU = _unit->getTimeUnits() - _escapeTUs;

	int distance = _save->getTileEngine()->distance(
												_unit->getPosition(),
												_attackAction->target);
	if (distance < 5)
	{
		if (tuAuto
			&& usableTU >= _unit->getActionTUs(BA_AUTOSHOT, _attackAction->weapon))
		{
			_attackAction->type = BA_AUTOSHOT;
		}
		else if (tuSnap
			&& usableTU >= _unit->getActionTUs(BA_SNAPSHOT, _attackAction->weapon))
		{
			_attackAction->type = BA_SNAPSHOT;
		}
		else if (tuAimed
			&& usableTU >= _unit->getActionTUs(BA_AIMEDSHOT, _attackAction->weapon))
		{
			_attackAction->type = BA_AIMEDSHOT;
		}
	}
	else if (distance < 13)
	{
		if (tuSnap
			&& usableTU >= _unit->getActionTUs(BA_SNAPSHOT, _attackAction->weapon))
		{
			_attackAction->type = BA_SNAPSHOT;
		}
		else if (tuAimed
			&& usableTU >= _unit->getActionTUs(BA_AIMEDSHOT, _attackAction->weapon))
		{
			_attackAction->type = BA_AIMEDSHOT;
		}
		else if (tuAuto
			&& usableTU >= _unit->getActionTUs(BA_AUTOSHOT, _attackAction->weapon))
		{
			_attackAction->type = BA_AUTOSHOT;
		}
	}
	else // distance > 12
	{
		if (tuAimed
			&& usableTU >= _unit->getActionTUs(BA_AIMEDSHOT, _attackAction->weapon))
		{
			_attackAction->type = BA_AIMEDSHOT;
		}
		else if (tuSnap
			&& usableTU >= _unit->getActionTUs(BA_SNAPSHOT, _attackAction->weapon))
		{
			_attackAction->type = BA_SNAPSHOT;
		}
		else if (tuAuto
			&& usableTU >= _unit->getActionTUs(BA_AUTOSHOT, _attackAction->weapon))
		{
			_attackAction->type = BA_AUTOSHOT;
		}
	}
}
	// else, just take yer best (most TU) shot!
/*	if (tuAimed
		&& usableTU >= _unit->getActionTUs(BA_AIMEDSHOT, _attackAction->weapon))
	{
		_attackAction->type = BA_AIMEDSHOT;
	}
	else if (tuAuto
		&& usableTU >= _unit->getActionTUs(BA_AUTOSHOT, _attackAction->weapon))
	{
		_attackAction->type = BA_AUTOSHOT;
	}
	else if (tuSnap
		&& usableTU >= _unit->getActionTUs(BA_SNAPSHOT, _attackAction->weapon))
	{
		_attackAction->type = BA_SNAPSHOT;
	} */

/**
 * Evaluates whether to throw a grenade at an enemy (or group of enemies) we can see.
 * @param action Pointer to an action.
 */
void AlienBAIState::grenadeAction()
{
	// do we have a grenade on our belt?
	// kL_note: this is already checked in setupAttack()
	// Could use it to determine if grenade is already inHand though! ( see _grenade var.)
	BattleItem* grenade = _unit->getGrenadeFromBelt();

	// distance must be more than X tiles, otherwise it's too dangerous to play with explosives
	if (explosiveEfficacy(
						_aggroTarget->getPosition(),
						_unit,
						grenade->getRules()->getExplosionRadius(),
						_attackAction->diff))
//						true))
	{
//		if (_unit->getFaction() == FACTION_HOSTILE)
		{
			int tu = 0;

			if (!_grenade)
			{
				tu += 4; // 4TUs for picking up the grenade ( kL_note: unless it's already in-hand..... )
			}
			tu += _unit->getActionTUs(BA_PRIME, grenade);
			tu += _unit->getActionTUs(BA_THROW, grenade);

			if (tu <= _unit->getStats()->tu) // do we have enough TUs to prime and throw the grenade?
			{
				BattleAction action;
				action.actor	= _unit;
				action.target	= _aggroTarget->getPosition();
				action.weapon	= grenade;
				action.type		= BA_THROW;

/* Wb.131129
			Position originVoxel = _save->getTileEngine()->getOriginVoxel(action, 0);
			Position targetVoxel = action.target * Position (16,16,24) + Position (8,8, (2 + -_save->getTile(action.target)->getTerrainLevel()));
			if (_save->getTileEngine()->validateThrow(action, originVoxel, targetVoxel)) // are we within range
*/
				if (_save->getTileEngine()->validateThrow(&action)) // are we within range
				{
					_attackAction->target	= action.target;
					_attackAction->weapon	= grenade;
					_attackAction->type		= BA_THROW;

					_rifle = false;
					_melee = false;
				}
			}
		}
	}
}

/**
 * Attempts a psionic attack on an enemy we know of.
 *
 * Psionic targetting: pick from any of the exposed units.
 * Exposed means they have been previously spotted, and are therefore known to the AI,
 * regardless of whether we can see them or not, because we're psycho.
 * @return bool, True if a psionic attack is performed.
 */
bool AlienBAIState::psiAction()
{
	//Log(LOG_INFO) << "AlienBAIState::psiAction()";

	if (_didPsi												// didn't already do a psi action this round
		|| _unit->getOriginalFaction() != FACTION_HOSTILE)	// don't let any faction but HOSTILE mind-control others.
	{
		return false;
	}

	RuleItem* itemRule = _save->getBattleState()->getGame()->getRuleset()->getItem("ALIEN_PSI_WEAPON");	// kL
	int tuCost = itemRule->getTUUse();																	// kL
	if (!itemRule->getFlatRate())																		// kL
		tuCost = static_cast<int>(floor(static_cast<float>(_unit->getStats()->tu * tuCost) / 100.f));	// kL
	Log(LOG_INFO) << "AlienBAIState::psiAction() tuCost = " << tuCost;

	if (_unit->getTimeUnits() < _escapeTUs + tuCost) // has the required TUs and can still make it to cover
	{
		return false;
	}
	else // do it.
	{
		_aggroTarget = 0;
		int
			chance = 0,
			chance2 = 0;

		int attackStr = static_cast<int>(
									static_cast<float>(_unit->getStats()->psiStrength)
								* static_cast<float>(_unit->getStats()->psiSkill) / 50.f);
		Log(LOG_INFO) << ". . attackStr = " << attackStr;

		for (std::vector<BattleUnit*>::const_iterator
				i = _save->getUnits()->begin();
				i != _save->getUnits()->end();
				++i)
		{
			if ((*i)->getOriginalFaction() == FACTION_PLAYER	// they must be player units
				&& (*i)->getArmor()->getSize() == 1				// don't target tanks
				&& validTarget(									// will check for Mc, etc
							*i,
							true,
							false))
			{
				// is this gonna crash..................
				Position
					target,
					origin = _save->getTileEngine()->getSightOriginVoxel(_unit);
				bool LoS = _save->getTileEngine()->canTargetUnit(
																&origin,
																(*i)->getTile(),
																&target,
																_unit);
				// stupid aLiens don't know soldier's psiSkill tho..
				// psiSkill would typically factor in at only a fifth of psiStrength.
				int
					defense = (*i)->getStats()->psiStrength,
					dist = _save->getTileEngine()->distance(
															(*i)->getPosition(),
															_unit->getPosition()),
					rand = RNG::generate(0, 100);

				Log(LOG_INFO) << ". . . defense = " << defense;
				Log(LOG_INFO) << ". . . dist = " << dist;
				Log(LOG_INFO) << ". . . rand = " << rand;
				Log(LOG_INFO) << ". . . LoS = " << static_cast<int>(LoS) * 50;


				chance2 = attackStr
							- defense
							- dist
							+ rand
							+ static_cast<int>(LoS) * 50;
				Log(LOG_INFO) << ". . . chance2 = " << chance2;

				if (chance2 > chance)
				{
					chance = chance2;
					_aggroTarget = *i;
				}
			}
		}

		if (!_aggroTarget
			|| chance < -10
			|| RNG::percent(10))
		{
			//Log(LOG_INFO) << "AlienBAIState::psiAction() EXIT 1, false";
			return false;
		}


/*		if (_visibleEnemies
			&& _attackAction->weapon
			&& _attackAction->weapon->getAmmoItem())
		{
			if (_attackAction->weapon->getAmmoItem()->getRules()->getPower() > chance)
			{
				//Log(LOG_INFO) << "AlienBAIState::psiAction() EXIT 2, false";
				return false;
			}
		}
		else if (RNG::generate(35, 155) > chance)
		{
			//Log(LOG_INFO) << "AlienBAIState::psiAction() EXIT 3, false";
			return false;
		} */

		if (_traceAI) Log(LOG_INFO) << "making a psionic attack this turn";

		int morale = _aggroTarget->getMorale();
		if (morale > 0)		// panicAtk is valid since target has morale to chew away
//			&& chance < 30)	// esp. when aLien atkStr is low
		{
			Log(LOG_INFO) << ". . test if MC or Panic";

			int
				bravery = _aggroTarget->getStats()->bravery,
				panicOdds = 110 - bravery, // ie, moraleHit
				moraleResult = morale - panicOdds;
			Log(LOG_INFO) << ". . panicOdds1 = " << panicOdds;

			if (moraleResult < 0)
				panicOdds -= bravery / 2;
			else if (moraleResult < 50)
				panicOdds -= bravery;

			Log(LOG_INFO) << ". . panicOdds2 = " << panicOdds;
			if (RNG::percent(panicOdds))
			{
				Log(LOG_INFO) << ". do Panic";
				_psiAction->target = _aggroTarget->getPosition();
				_psiAction->type = BA_PANIC;

				//Log(LOG_INFO) << "AlienBAIState::psiAction() EXIT 2, true";
				return true;
			}
		}

		Log(LOG_INFO) << ". do MindControl";
		_psiAction->target = _aggroTarget->getPosition();
		_psiAction->type = BA_MINDCONTROL;

		//Log(LOG_INFO) << "AlienBAIState::psiAction() EXIT 3, true";
		return true;
	}

	//Log(LOG_INFO) << "AlienBAIState::psiAction() EXIT 4, false";
	return false;
}

/**
 * Performs a melee attack action.
 * @param action Pointer to an action.
 */
void AlienBAIState::meleeAttack()
{
	_unit->lookAt(
			_aggroTarget->getPosition() + Position(
											_unit->getArmor()->getSize() - 1,
											_unit->getArmor()->getSize() - 1,
											0),
			false);

	while (_unit->getStatus() == STATUS_TURNING)
	{
		_unit->turn();
	}

	if (_traceAI)
	{
		Log(LOG_INFO) << "Attack unit: " << _aggroTarget->getId();
	}

	_attackAction->target = _aggroTarget->getPosition();
	_attackAction->type = BA_HIT;
}

/**
 *
 */
bool AlienBAIState::validTarget(
		BattleUnit* unit,
		bool assessDanger,
		bool includeCivs) const
{
	if (unit->isOut(true, true)									// ignore units that are dead/unconscious
		|| _intelligence < unit->getTurnsExposed()				// they must be units that we "know" about
		|| (assessDanger && unit->getTile()->getDangerous())	// they haven't been grenaded
		|| unit->getFaction() == FACTION_HOSTILE)				// and they mustn't be on our side
	{
		return false;
	}

	if (includeCivs)
	{
		return true;
	}

	return unit->getFaction() == FACTION_PLAYER;
}

}
