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

#include "CivilianBAIState.h"

//#define _USE_MATH_DEFINES
//#include <cmath>

#include "Pathfinding.h"
#include "TileEngine.h"

//#include "../Engine/Options.h"
//#include "../Engine/RNG.h"

#include "../Savegame/BattleUnit.h"
#include "../Savegame/Node.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

/**
 * Sets up a CivilianBAIState.
 * @param battleSave	- pointer to the SavedBattleGame
 * @param unit			- pointer to the BattleUnit
 * @param node			- pointer to the node the unit originates from
 */
CivilianBAIState::CivilianBAIState(
		SavedBattleGame* const battleSave,
		BattleUnit* const unit,
		Node* const node)
	:
		BattleAIState(
			battleSave,
			unit),
		_aggroTarget(NULL),
		_escapeTUs(0),
		_AIMode(0),
		_visibleEnemies(0),
		_spottingEnemies(0),
		_fromNode(node),
		_toNode(NULL)
//		_traceAI(false)
{
//	_traceAI = Options::traceAI;
	_escapeAction = new BattleAction();
	_patrolAction = new BattleAction();
}

/**
 * Deletes the CivilianBAIState.
 */
CivilianBAIState::~CivilianBAIState()
{
	delete _escapeAction;
	delete _patrolAction;
}

/**
 * Loads the AI state from a YAML file.
 * @param node - reference a YAML node
 */
void CivilianBAIState::load(const YAML::Node& node)
{
	_AIMode = node["AIMode"].as<int>(0);

	const int
		fromNodeId	= node["fromNode"]	.as<int>(-1),
		toNodeId	= node["toNode"]	.as<int>(-1);

	if (fromNodeId != -1)
		_fromNode = _battleSave->getNodes()->at(static_cast<size_t>(fromNodeId));

	if (toNodeId != -1)
		_toNode = _battleSave->getNodes()->at(static_cast<size_t>(toNodeId));
}

/**
 * Saves the AI state to a YAML file.
 * @return, YAML node
 */
YAML::Node CivilianBAIState::save() const
{
	int
		fromNodeId = -1,
		toNodeId = -1;

	if (_fromNode != NULL)	fromNodeId	= _fromNode->getId();
	if (_toNode != NULL)	toNodeId	= _toNode->getId();

	YAML::Node node;

	node["fromNode"]	= fromNodeId;
	node["toNode"]		= toNodeId;
	node["AIMode"]		= _AIMode;

	return node;
}

/**
 * Enters the current AI state.
 */
//void CivilianBAIState::enter(){}

/**
 * Exits the current AI state.
 */
//void CivilianBAIState::exit(){}

/**
 * Runs any code the state needs to keep updating every AI cycle.
 * @param action (possible) AI action to execute after thinking is done.
 */
void CivilianBAIState::think(BattleAction* action)
{
	//Log(LOG_INFO) << "CivilianBAIState::think()";
 	action->type = BA_RETHINK;
	action->actor = _unit;

	_escapeAction->number = action->number;

	_visibleEnemies = countHostiles();
	_spottingEnemies = countSpottingUnits(_unit->getPosition());

//	if (_traceAI)
//	{
//		Log(LOG_INFO) << "Civilian Unit has " << _visibleEnemies << " enemies visible, " << _spottingEnemies << " of whom are spotting him. ";
//		std::string AIMode;
//		switch (_AIMode)
//		{
//			case 0: AIMode = "Patrol"; break;
//			case 3: AIMode = "Escape";
//		}
//		Log(LOG_INFO) << "Currently using " << AIMode << " behaviour";
//	}

	if (_spottingEnemies != 0
		&& _escapeTUs == 0)
	{
		setupEscape();
	}

	setupPatrol();

	bool evaluate = false;
	if (_AIMode == AI_ESCAPE)
	{
		if (_spottingEnemies == 0)
			evaluate = true;
	}
	else if (_AIMode == AI_PATROL)
	{
		if (_spottingEnemies != 0
			|| _visibleEnemies != 0
			|| RNG::percent(10) == true)
		{
			evaluate = true;
		}
	}

	if (_spottingEnemies > 2
		|| _unit->getHealth() < 2 * _unit->getBaseStats()->health / 3)
	{
		evaluate = true;
	}

	if (evaluate == true)
	{
		evaluateAIMode();
//		if (_traceAI)
//		{
//			std::string AIMode;
//			switch (_AIMode)
//			{
//				case 0: AIMode = "Patrol"; break;
//				case 3: AIMode = "Escape";
//			}
//			Log(LOG_INFO) << "Re-Evaluated, now using " << AIMode << " behaviour";
//		}
	}

	switch (_AIMode)
	{
		case AI_ESCAPE:
			action->type		= _escapeAction->type;
			action->target		= _escapeAction->target;
			action->number		= 3;
			action->desperate	= true;

			_unit->dontReselect();
//			_battleSave->getBattleGame()->setReservedAction(BA_NONE, false);
		break;
		case AI_PATROL:
			action->type	= _patrolAction->type;
			action->target	= _patrolAction->target;
	}

	if (action->type == BA_WALK
		&& action->target != _unit->getPosition())
	{
		_escapeTUs = 0;
	}
}

/**
 * Counts the quantity of Hostiles that the civilian sees and sets the closest
 * one as the '_aggroTarget'.
 * @note If none of the hostiles can target the civilian this returns 0.
 * @return, quantity of potential perps
 */
int CivilianBAIState::countHostiles()
{
	int
		tally = 0,
		dist = 1000,
		distTest;

	_aggroTarget = NULL;

	const Position origin = _battleSave->getTileEngine()->getSightOriginVoxel(_unit)
						  + Position(0,0,-4);

	Position target;
	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if ((*i)->isOut() == false
			&& (*i)->getFaction() == FACTION_HOSTILE)
		{
			if (_battleSave->getTileEngine()->visible(
													_unit,
													(*i)->getTile()) == true)
			{
				++tally;
				distTest = _battleSave->getTileEngine()->distance(
																_unit->getPosition(),
																(*i)->getPosition());
				if (distTest < dist
					&& _battleSave->getTileEngine()->canTargetUnit(
																&origin,
																(*i)->getTile(),
																&target,
																_unit) == true)
				{
					dist = distTest;
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
 *
 */
int CivilianBAIState::countSpottingUnits(Position pos) const
{
	bool checking = (pos != _unit->getPosition());
	int tally = 0;

	Position
		originVoxel,
		targetVoxel;

	const BattleUnit* unit;

	for (std::vector<BattleUnit*>::const_iterator
			i = _battleSave->getUnits()->begin();
			i != _battleSave->getUnits()->end();
			++i)
	{
		if ((*i)->isOut() == false
			&& (*i)->getFaction() == FACTION_HOSTILE
			&& _battleSave->getTileEngine()->distance(
													pos,
													(*i)->getPosition()) < 25)
		{
			originVoxel = _battleSave->getTileEngine()->getSightOriginVoxel(*i);
			originVoxel.z -= 4;

			if (checking == true)
				unit = _unit;
			else
				unit = NULL;

			if (_battleSave->getTileEngine()->canTargetUnit(
														&originVoxel,
														_battleSave->getTile(pos),
														&targetVoxel,
														*i,
														unit) == true)
			{
				++tally;
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
	// weights for various factors when choosing a tile to withdraw to;
	// they're subjective, should be hand-tuned, may need tweaking.
	const int
		EXPOSURE_PENALTY		= 10,
		FIRE_PENALTY			= 40,
		BASE_SUCCESS_SYSTEMATIC	= 100,
		BASE_SUCCESS_DESPERATE	= 110,
		FAST_PASS_THRESHOLD		= 100, // a score that's good enough to quit the while-loop early
		CUR_TILE_PREF			= 15;

	int
		bestTileScore = -100000,
		score,
		tu = _unit->getTimeUnits() / 2,
		unitsSpotting = countSpottingUnits(_unit->getPosition()),
		dist = 0;

	countHostiles(); // sets an _aggroTarget
	if (_aggroTarget != NULL)
		dist = _battleSave->getTileEngine()->distance(
												_unit->getPosition(),
												_aggroTarget->getPosition());

	const Tile* tile;
	Position bestTilePos(0,0,0);

	Pathfinding* const pf = _battleSave->getPathfinding();
	pf->setPathingUnit(_unit);
	std::vector<int> reachable = pf->findReachable(
												_unit,
												tu);

	std::vector<Position> randTileSearch = _battleSave->getTileSearch();
	RNG::shuffle(randTileSearch);

	bool coverFound = false;
	int tries = 0;
	while (tries < 150
		&& coverFound == false)
	{
		_escapeAction->target = _unit->getPosition();

		score = 0;

		if (tries < _battleSave->SEARCH_SIZE) //121 // looking for cover
		{
			// looking for cover
			_escapeAction->target.x += randTileSearch[tries].x;
			_escapeAction->target.y += randTileSearch[tries].y;

			score = BASE_SUCCESS_SYSTEMATIC;

			if (_escapeAction->target == _unit->getPosition())
			{
				if (unitsSpotting > 0)
				{
					// maybe don't stay in the same spot? move or something if there's any point to it?
					_escapeAction->target.x += RNG::generate(-20,20);
					_escapeAction->target.y += RNG::generate(-20,20);
				}
				else
					score += CUR_TILE_PREF;
			}
		}
		else
		{
			//if (_traceAI && tries == 121) Log(LOG_INFO) << "best score after systematic search was: " << bestTileScore;

			score = BASE_SUCCESS_DESPERATE; // ruuuuuuun

			_escapeAction->target = _unit->getPosition();
			_escapeAction->target.x += RNG::generate(-10,10);
			_escapeAction->target.y += RNG::generate(-10,10);
			_escapeAction->target.z = _unit->getPosition().z + RNG::generate(-1,1);

			if (_escapeAction->target.z < 0)
				_escapeAction->target.z = 0;
			else if (_escapeAction->target.z >= _battleSave->getMapSizeZ())
				_escapeAction->target.z = _unit->getPosition().z;
		}

		// civilians shouldn't have any tactical sense anyway so save some CPU cycles here
		tries += 10;

		// THINK, DAMN YOU1
		tile = _battleSave->getTile(_escapeAction->target);

		int distTarget = 0;
		if (_aggroTarget != NULL)
			distTarget = _battleSave->getTileEngine()->distance(
															_aggroTarget->getPosition(),
															_escapeAction->target);

		if (dist >= distTarget)
			score -= (distTarget - dist) * 10;
		else
			score += (distTarget - dist) * 10;

		if (tile == NULL) // no you can't quit the battlefield by running off the map.
			score = -100001;
		else
		{
			const int spotters = countSpottingUnits(_escapeAction->target);

			// just ignore unreachable tiles
			if (std::find(
						reachable.begin(),
						reachable.end(),
						_battleSave->getTileIndex(tile->getPosition())) == reachable.end())
			{
				continue;
			}

			if (_spottingEnemies != 0
				|| spotters != 0)
			{
				if (_spottingEnemies <= spotters) // that's for giving away our position
					score -= (1 + spotters - _spottingEnemies) * EXPOSURE_PENALTY;
				else
					score += (_spottingEnemies - spotters) * EXPOSURE_PENALTY;
			}

			if (tile->getFire() != 0)
				score -= FIRE_PENALTY;

//			if (_traceAI)
//			{
//				tile->setPreviewColor(score < 0? 3: (score < FAST_PASS_THRESHOLD/2? 8: (score < FAST_PASS_THRESHOLD ?9: 5)));
//				tile->setPreviewDir(10);
//				tile->setPreviewTU(score);
//			}
		}

		if (tile != NULL
			&& score > bestTileScore)
		{
			// calculate TUs to tile;
			// could be getting this from findReachable() somehow but that would break something for sure...
			pf->calculate(
						_unit,
						_escapeAction->target,
						NULL,
						tu);

			if (_escapeAction->target == _unit->getPosition()
				|| pf->getStartDirection() != -1)
			{
				bestTileScore = score;
				bestTilePos = _escapeAction->target;
				_escapeTUs = pf->getTotalTUCost();

				if (_escapeAction->target == _unit->getPosition())
					_escapeTUs = 1;

//				if (_traceAI)
//				{
//					tile->setPreviewColor(score < 0? 7: (score < FAST_PASS_THRESHOLD / 2? 10: (score < FAST_PASS_THRESHOLD ?4: 5)));
//					tile->setPreviewDir(10);
//					tile->setPreviewTU(score);
//				}
			}

			pf->abortPath();

			if (bestTileScore > FAST_PASS_THRESHOLD)
				coverFound = true; // good enough, gogogo!!
		}
	}

	_escapeAction->target = bestTilePos;

	//if (_traceAI) _battleSave->getTile(_escapeAction->target)->setPreviewColor(13);

	if (bestTileScore <= -100000)
	{
		//if (_traceAI) Log(LOG_INFO) << "Escape estimation failed.";
		// do something, just don't look dumbstruck :P
		_escapeAction->type = BA_RETHINK;
		return;
	}
	else
	{
		//if (_traceAI) Log(LOG_INFO) << "Escape estimation completed after " << tries << " tries, " << _battleSave->getTileEngine()->distance(_unit->getPosition(), bestTilePos) << " squares or so away.";
		_escapeAction->type = BA_WALK;
	}
}

/**
 *
 */
void CivilianBAIState::setupPatrol()
{
	if (_toNode != NULL
		&& _unit->getPosition() == _toNode->getPosition())
	{
		//if (_traceAI) Log(LOG_INFO) << "Patrol destination reached!";
		// destination reached
		// head off to next patrol node
		_fromNode = _toNode;
		_toNode = NULL;
	}

	if (_fromNode == NULL)
		_fromNode = _battleSave->getNearestNode(_unit);
/*{
		// assume closest node as "from node"
		// on same level to avoid strange things, and the node has to match unit size or it will freeze
		int dist = 1000000;
		for (std::vector<Node*>::const_iterator
				i = _battleSave->getNodes()->begin();
				i != _battleSave->getNodes()->end();
				++i)
		{
			Node* node = *i;
			const int distTest = _battleSave->getTileEngine()->distanceSq(
																		_unit->getPosition(),
																		node->getPosition());
			if (_unit->getPosition().z == node->getPosition().z
				&& distTest < dist
				&& (node->getNodeType() & Node::TYPE_SMALL))
			{
				_fromNode = node;
				dist = distTest;
			}
		}
	} */

	Pathfinding* const pf = _battleSave->getPathfinding();
	pf->setPathingUnit(_unit);

	int triesLeft = 5; // look for a new node to walk towards
	while (_toNode == NULL
		&& triesLeft != 0)
	{
		--triesLeft;

		_toNode = _battleSave->getPatrolNode(
										true,
										_unit,
										_fromNode);
		if (_toNode == NULL)
			_toNode = _battleSave->getPatrolNode(
											false,
											_unit,
											_fromNode);

		if (_toNode != NULL)
		{
			pf->calculate(
						_unit,
						_toNode->getPosition());

			if (pf->getStartDirection() == -1)
				_toNode = NULL;

			pf->abortPath();
		}
	}

	if (_toNode != NULL)
	{
		_patrolAction->actor = _unit;
		_patrolAction->type = BA_WALK;
		_patrolAction->target = _toNode->getPosition();
	}
	else
		_patrolAction->type = BA_RETHINK;
}

/**
 *
 */
void CivilianBAIState::evaluateAIMode()
{
	double
		escapeOdds = 0.,
		patrolOdds = 30.;

	if (_visibleEnemies > 0)
	{
		escapeOdds = 15.;
		patrolOdds = 15.;
	}

	if (_spottingEnemies)
	{
		patrolOdds = 0.;

		if (_escapeTUs == 0)
			setupEscape();
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

	if (_unit->getHealth() < _unit->getBaseStats()->health / 3)
		escapeOdds *= 1.7;
	else if (_unit->getHealth() < 2 * (_unit->getBaseStats()->health / 3))
		escapeOdds *= 1.4;
	else if (_unit->getHealth() < _unit->getBaseStats()->health)
		escapeOdds *= 1.1;

	switch (_unit->getAggression())
	{
		case 0:
			escapeOdds *= 1.4;
		break;
		case 2:
			escapeOdds *= 0.7;
		break;
	}

	if (_spottingEnemies != 0)
		escapeOdds = 10. * escapeOdds * static_cast<double>(_spottingEnemies + 10) / 100.;
	else
		escapeOdds /= 2.;

	const double decision = 1. + RNG::generate(0., patrolOdds + escapeOdds);
	if (decision > escapeOdds)
		_AIMode = AI_PATROL;
	else
		_AIMode = AI_ESCAPE;

	if (_AIMode == AI_PATROL)
	{
		if (_toNode != NULL)
			return;
	}

	_AIMode = AI_ESCAPE;
}

}
