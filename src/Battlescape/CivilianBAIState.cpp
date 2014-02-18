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

#include "CivilianBAIState.h"

#include <cmath>

#include "BattlescapeState.h"
#include "Pathfinding.h"
#include "TileEngine.h"

#include "../Engine/Logger.h"
#include "../Engine/Options.h"
#include "../Engine/RNG.h"

#include "../Ruleset/Armor.h"

#include "../Savegame/BattleUnit.h"
#include "../Savegame/Node.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

/**
 * Sets up a CivilianBAIState.
 */
CivilianBAIState::CivilianBAIState(
		SavedBattleGame* game,
		BattleUnit* unit,
		Node* node)
	:
		BattleAIState(game, unit),
		_aggroTarget(0),
		_escapeTUs(0),
		_AIMode(0),
		_visibleEnemies(0),
		_spottingEnemies(0),
		_fromNode(node),
		_toNode(0)
{
	//Log(LOG_INFO) << "Create CivilianBAIState";

	_traceAI = _save->getTraceSetting();

	_escapeAction = new BattleAction();
	_patrolAction = new BattleAction();
}

/**
 * Deletes the CivilianBAIState.
 */
CivilianBAIState::~CivilianBAIState()
{
	//Log(LOG_INFO) << "Delete CivilianBAIState";

	delete _escapeAction;
	delete _patrolAction;
}

/**
 * Loads the AI state from a YAML file.
 * @param node YAML node.
 */
void CivilianBAIState::load(const YAML::Node& node)
{
	_AIMode = node["AIMode"].as<int>(0);

	int fromNodeID = node["fromNode"].as<int>(-1);
	if (fromNodeID != -1)
	{
		_fromNode = _save->getNodes()->at(fromNodeID);
	}

	int toNodeID = node["toNode"].as<int>(-1);
	if (toNodeID != -1)
	{
		_toNode = _save->getNodes()->at(toNodeID);
	}
}

/**
 * Saves the AI state to a YAML file.
 * @param out YAML emitter.
 */
YAML::Node CivilianBAIState::save() const
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

	return node;
}

/**
 * Enters the current AI state.
 */
void CivilianBAIState::enter()
{
	//Log(LOG_INFO) << "CivilianBAIState::enter()";
}

/**
 * Exits the current AI state.
 */
void CivilianBAIState::exit()
{
//kL	if (_toNode) _toNode->freeNode(); // kL_note: This class never allocates nodes....

	//Log(LOG_INFO) << "CivilianBAIState::exit()";
}

/**
 * Runs any code the state needs to keep updating every
 * AI cycle.
 */
void CivilianBAIState::think(BattleAction* action)
{
	//Log(LOG_INFO) << "CivilianBAIState::think()";

 	action->type = BA_RETHINK;
	action->actor = _unit;

	_escapeAction->number = action->number;

	_visibleEnemies = selectNearestTarget();
	_spottingEnemies = getSpottingUnits(_unit->getPosition());

	if (_traceAI)
	{
		Log(LOG_INFO) << "Civilian Unit has " << _visibleEnemies << " enemies visible, "
				<< _spottingEnemies << " of whom are spotting him. ";

		std::string AIMode;
		switch (_AIMode)
		{
			case 0:
				AIMode = "Patrol";
			break;
			case 3:
				AIMode = "Escape";
			break;
		}

		Log(LOG_INFO) << "Currently using " << AIMode << " behaviour";
	}

	if (_spottingEnemies && !_escapeTUs)
	{
		setupEscape();
	}

	setupPatrol();

	bool evaluate = false;
	if (_AIMode == AI_ESCAPE)
	{
		if (!_spottingEnemies)
		{
			evaluate = true;
		}
	}
	else if (_AIMode == AI_PATROL)
	{
		if (_spottingEnemies || _visibleEnemies || RNG::percent(10))
		{
			evaluate = true;
		}
	}

	if (_spottingEnemies > 2
		|| _unit->getHealth() < 2 * _unit->getStats()->health / 3)
	{
		evaluate = true;
	}

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
				case 3:
					AIMode = "Escape";
				break;
			}

			if (_traceAI) Log(LOG_INFO) << "Re-Evaluated, now using " << AIMode << " behaviour";
		}
	}

	switch (_AIMode)
	{
		case AI_ESCAPE:
			action->type = _escapeAction->type;
			action->target = _escapeAction->target;
			action->number = 3;
			_unit->dontReselect();
			action->desperate = true;
			_save->getBattleGame()->setTUReserved(BA_NONE, false);
		break;
		case AI_PATROL:
			action->type = _patrolAction->type;
			action->target = _patrolAction->target;
		break;

		default:
		break;
	}

	if (action->type == BA_WALK
		&& action->target != _unit->getPosition())
	{
		_escapeTUs = 0;
	}

	//Log(LOG_INFO) << "CivilianBAIState::think() EXIT";
}

/**
 *
 */
int CivilianBAIState::selectNearestTarget()
{
	int tally = 0;
	int closest = 100;

	_aggroTarget = 0;

	Position origin = _save->getTileEngine()->getSightOriginVoxel(_unit);
	origin.z -= 4;

	Position target;
	for (std::vector<BattleUnit*>::const_iterator
			i = _save->getUnits()->begin();
			i != _save->getUnits()->end();
			++i)
	{
		if (!(*i)->isOut()
			&& (*i)->getFaction() == FACTION_HOSTILE)
		{
			if (_save->getTileEngine()->visible(_unit, (*i)->getTile()))
			{
				tally++;

				int dist = _save->getTileEngine()->distance(
														_unit->getPosition(),
														(*i)->getPosition());
				if (dist < closest)
				{
					bool validTarget = _save->getTileEngine()->canTargetUnit(
																		&origin,
																		(*i)->getTile(),
																		&target,
																		_unit);
					if (validTarget)
					{
						closest = dist;
						_aggroTarget = *i;
					}
				}
			}
		}
	}

	if (_aggroTarget)
		return tally;

	return 0;
}

/**
 *
 */
int CivilianBAIState::getSpottingUnits(Position pos) const
{
	bool checking = pos != _unit->getPosition();

	int tally = 0;
	for (std::vector<BattleUnit*>::const_iterator
			i = _save->getUnits()->begin();
			i != _save->getUnits()->end();
			++i)
	{
		if (!(*i)->isOut()
			&& (*i)->getFaction() == FACTION_HOSTILE)
		{
			int dist = _save->getTileEngine()->distance(
													pos,
													(*i)->getPosition());
			if (dist > 20)
				continue;

			Position originVoxel = _save->getTileEngine()->getSightOriginVoxel(*i);
			originVoxel.z -= 4;

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
 *
 */
void CivilianBAIState::setupEscape()
{
	selectNearestTarget(); // gets an _aggroTarget

//kL	int dist = _aggroTarget? _save->getTileEngine()->distance(_unit->getPosition(), _aggroTarget->getPosition()): 0;
	int dist = 0;
	if (_aggroTarget)
	{
		_save->getTileEngine()->distance(
									_unit->getPosition(),
									_aggroTarget->getPosition());
	}


	int bestTileScore = -100000;
	int score = -100000;

	Position bestTile(0, 0, 0);
	Tile* tile = 0;

	// weights for various factors when choosing a tile to withdraw to;
	// they're subjective, should be hand-tuned, may need tweaking.
	const int EXPOSURE_PENALTY			= 10;
	const int FIRE_PENALTY				= 40;
	const int BASE_SYSTEMATIC_SUCCESS	= 100;
	const int BASE_DESPERATE_SUCCESS	= 110;
	// a score that's good enough to quit the while loop early;
	const int FAST_PASS_THRESHOLD		= 100;

	int tu = _unit->getTimeUnits() / 2;

	std::vector<int> reachable = _save->getPathfinding()->findReachable(_unit, tu);
	std::vector<Position> randomTileSearch = _save->getTileSearch();
	RNG::shuffle(randomTileSearch);
//	std::random_shuffle(
//					randomTileSearch.begin(),
//					randomTileSearch.end());

	int unitsSpotting = getSpottingUnits(_unit->getPosition());
	int currentTilePref = 15;

	bool coverFound = false;
	int tries = 0;
	while (tries < 150
		&& !coverFound)
	{
		// start looking in a direction away from the enemy
		_escapeAction->target = _unit->getPosition();

		// cornered at the edge of the map perhaps?
/*kL		if (!_save->getTile(_escapeAction->target))
		{
			_escapeAction->target = _unit->getPosition(); // <- kL_note: looks redundant to me.
		} */

		score = 0;

		if (tries < 121)
		{
			// looking for cover
			_escapeAction->target.x += randomTileSearch[tries].x;
			_escapeAction->target.y += randomTileSearch[tries].y;

			score = BASE_SYSTEMATIC_SUCCESS;

			if (_escapeAction->target == _unit->getPosition())
			{
				if (unitsSpotting > 0)
				{
					// maybe don't stay in the same spot? move or something if there's any point to it?
					_escapeAction->target.x += RNG::generate(-20, 20);
					_escapeAction->target.y += RNG::generate(-20, 20);
				}
				else
				{
					score += currentTilePref;
				}
			}
		}
		else
		{
			if (tries == 121
				&& _traceAI)
			{
				Log(LOG_INFO) << "best score after systematic search was: " << bestTileScore;
			}

			score = BASE_DESPERATE_SUCCESS; // ruuuuuuun

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

		// civilians shouldn't have any tactical sense anyway so save some CPU cycles here
		tries += 10;

		// THINK, DAMN YOU1
		tile = _save->getTile(_escapeAction->target);

		int distanceFromTarget = 0;
		if (_aggroTarget)
		{
			_save->getTileEngine()->distance(
										_aggroTarget->getPosition(),
										_escapeAction->target);
		}

		if (dist >= distanceFromTarget)
		{
			score -= (distanceFromTarget - dist) * 10;
		}
		else
		{
			score += (distanceFromTarget - dist) * 10;
		}

		int spotters = 0;

		if (!tile)
		{
			// no you can't quit the battlefield by running off the map.
			score = -100001;
		}
		else
		{
			spotters = getSpottingUnits(_escapeAction->target);

			// just ignore unreachable tiles
			if (std::find(
					reachable.begin(),
					reachable.end(),
					_save->getTileIndex(tile->getPosition())) == reachable.end())
			{
				continue;
			}

			if (_spottingEnemies || spotters)
			{
				if (_spottingEnemies <= spotters)
				{
					// that's for giving away our position
					score -= (1 + spotters - _spottingEnemies) * EXPOSURE_PENALTY;
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

			if (_traceAI)
			{
				tile->setMarkerColor(score < 0 ? 3 : (score < FAST_PASS_THRESHOLD/2 ? 8 : (score < FAST_PASS_THRESHOLD ? 9 : 5)));
				tile->setPreview(10);
				tile->setTUMarker(score);
			}
		}

		if (tile
			&& score > bestTileScore)
		{
			// calculate TUs to tile;
			// we could be getting this from findReachable() somehow but that would break something for sure...
			_save->getPathfinding()->calculate(_unit, _escapeAction->target, 0, tu);

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
					tile->setMarkerColor(score < 0 ? 7 : (score < FAST_PASS_THRESHOLD / 2 ? 10 : (score < FAST_PASS_THRESHOLD ? 4 : 5)));
					tile->setPreview(10);
					tile->setTUMarker(score);
				}
			}

			_save->getPathfinding()->abortPath();

			if (bestTileScore > FAST_PASS_THRESHOLD)
				coverFound = true; // good enough, gogogo!!
		}
	}

	_escapeAction->target = bestTile;

	if (_traceAI)
	{
		_save->getTile(_escapeAction->target)->setMarkerColor(13);
	}

	if (bestTileScore <= -100000)
	{
		if (_traceAI) Log(LOG_INFO) << "Escape estimation failed.";

		// do something, just don't look dumbstruck :P
		_escapeAction->type = BA_RETHINK;

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
 *
 */
void CivilianBAIState::setupPatrol()
{
	Node* node;

	if (_toNode != 0
		&& _unit->getPosition() == _toNode->getPosition())
	{
		if (_traceAI) Log(LOG_INFO) << "Patrol destination reached!";

		// destination reached
		// head off to next patrol node
		_fromNode = _toNode;
		_toNode = 0;
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

			int dist = _save->getTileEngine()->distanceSq(
													_unit->getPosition(),
													node->getPosition());
			if (_unit->getPosition().z == node->getPosition().z
				&& dist < closest
				&& node->getType() & Node::TYPE_SMALL)
			{
				_fromNode = node;
				closest = dist;
			}
		}
	}

	// look for a new node to walk towards
	int triesLeft = 5;

	while (_toNode == 0
		&& triesLeft)
	{
		triesLeft--;

		_toNode = _save->getPatrolNode(
									true,
									_unit,
									_fromNode);
		if (_toNode == 0)
		{
			_toNode = _save->getPatrolNode(
										false,
										_unit,
										_fromNode);
		}

		if (_toNode != 0)
		{
			_save->getPathfinding()->calculate(
											_unit,
											_toNode->getPosition());

			if (_save->getPathfinding()->getStartDirection() == -1)
			{
				_toNode = 0;
			}

			_save->getPathfinding()->abortPath();
		}
	}

	if (_toNode != 0)
	{
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
 *
 */
void CivilianBAIState::evaluateAIMode()
{
	double escapeOdds = _visibleEnemies? 15.0: 0.0;
	double patrolOdds = _visibleEnemies? 15.0: 30.0;

	if (_spottingEnemies)
	{
		patrolOdds = 0.0;

		if (_escapeTUs == 0)
		{
			setupEscape();
		}
	}

	switch (_AIMode)
	{
		case 0:
			patrolOdds *= 1.1;
		break;
		case 3:
			escapeOdds *= 1.1;
		break;
	}

	if (_unit->getHealth() < _unit->getStats()->health / 3)
	{
		escapeOdds *= 1.7;
	}
	else if (_unit->getHealth() < 2 * (_unit->getStats()->health / 3))
	{
		escapeOdds *= 1.4;
	}
	else if (_unit->getHealth() < _unit->getStats()->health)
	{
		escapeOdds *= 1.1;
	}

	switch (_unit->getAggression())
	{
		case 0:
			escapeOdds *= 1.4;
		break;
		case 2:
			escapeOdds *= 0.7;
		break;
	}

	if (_spottingEnemies)
	{
		escapeOdds = 10.0 * escapeOdds * static_cast<double>((_spottingEnemies + 10)) / 100.0;
	}
	else
	{
		escapeOdds /= 2.0;
	}

	double decision = 1.0 + RNG::generate(0.0, patrolOdds + escapeOdds);
	if (decision > escapeOdds)
	{
		_AIMode = AI_PATROL;
	}
	else
	{
		_AIMode = AI_ESCAPE;
	}

	if (_AIMode == AI_PATROL)
	{
		if (_toNode)
		{
			return;
		}
	}

	_AIMode = AI_ESCAPE;
}

}
