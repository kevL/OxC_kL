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

#include "Pathfinding.h"

#include <list>

#include "PathfindingNode.h"
#include "PathfindingOpenSet.h"

#include "../Battlescape/BattlescapeGame.h"
#include "../Battlescape/BattlescapeState.h"
#include "../Battlescape/TileEngine.h"

#include "../Engine/Game.h"
//#include "../Engine/Logger.h"
#include "../Engine/Options.h"

#include "../Ruleset/Armor.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

int
	Pathfinding::red	= 3,
	Pathfinding::green	= 4,
	Pathfinding::yellow	= 10;


/**
 * Sets up a Pathfinding object.
 * @param save - pointer to SavedBattleGame.
 */
Pathfinding::Pathfinding(SavedBattleGame* save)
	:
		_save(save),
		_unit(NULL),
		_pathPreviewed(false),
		_strafeMove(false),
		_totalTUCost(0),
		_modCTRL(false),
		_modALT(false),
		_movementType(MT_WALK),
		_openDoor(0) // kL
//		_kneelCheck(true) // kL
{
	_size = _save->getMapSizeXYZ();
	_nodes.reserve(_size); // initialize one node per tile.

	Position pos;
	for (int
			i = 0;
			i < _size;
			++i)
	{
		_save->getTileCoords(
							i,
							&pos.x,
							&pos.y,
							&pos.z);
		_nodes.push_back(PathfindingNode(pos));
	}
}

/**
 * Deletes the Pathfinding.
 * @internal This is required to be here because it requires the PathfindingNode class definition.
 */
Pathfinding::~Pathfinding()
{
	// Nothing more to do here.
}

/**
 * Gets the Node on a given position on the map.
 * @param pos - reference the Position
 * @return, pointer to PathfindingNode
 */
PathfindingNode* Pathfinding::getNode(const Position& pos)
{
	return &_nodes[_save->getTileIndex(pos)];
}

/**
 * Calculates the shortest path; tries bresenham then A* paths.
 * @param unit				- pointer to a BattleUnit
 * @param endPos			- destination Position
 * @param missileTarget		- pointer to a targeted BattleUnit (default NULL)
 * @param maxTUCost			- maximum time units this path can cost (default 1000)
 * @param strafeRejected	- true if path needs to be recalculated w/out strafe (default false)
 */
void Pathfinding::calculate(
		BattleUnit* unit,
		Position endPos,
		BattleUnit* missileTarget,
		int maxTUCost,
		bool strafeRejected) // kL_add.
{
	//Log(LOG_INFO) << "Pathfinding::calculate()";
	_unit = unit;

	_totalTUCost = 0;
	_path.clear();

	_modCTRL = (SDL_GetModState() & KMOD_CTRL) != 0;
	_modALT = (SDL_GetModState() & KMOD_ALT) != 0;

	Position endPos2 = endPos; // kL: for keeping things straight if strafeRejected happens.

	// i'm DONE with these out of bounds errors.
	// kL_note: I really don't care what you're "DONE" with.....
	if (endPos.x < 0
		|| endPos.y < 0
		|| endPos.x > _save->getMapSizeX() - unit->getArmor()->getSize()
		|| endPos.y > _save->getMapSizeY() - unit->getArmor()->getSize())
	{
		return;
	}

	if (missileTarget != NULL
		&& maxTUCost == -1) // pathfinding for missile
	{
		maxTUCost = 10000;
		_movementType = MT_FLY;
	}
	else
	{
		_movementType = unit->getMovementType();

		if (_movementType == MT_FLY
			&& _modALT //(SDL_GetModState() & KMOD_ALT) != 0 // this forces soldiers in flyingsuits to walk on (or fall to) the ground.
			&& (unit->getGeoscapeSoldier() != NULL
//				|| (unit->getUnitRules() &&
				|| unit->getUnitRules()->getMechanical() == false)
//			&& unit->getTurretType() == -1					// hovertanks always hover.
			&& unit->getRaceString() != "STR_FLOATER"		// floaters always float
			&& unit->getRaceString() != "STR_CELATID")		// celatids always .. float.
//			&& unit->getRaceString() != "STR_CYBERDISC")	// cyberdiscs always .. float; done w/ getMechanical()
			// Ethereals *can* walk, but they don't like to. Should turn this into Ruleset param: 'alwaysFloat'
		{
			//Log(LOG_INFO) << ". MT_WALK";
			_movementType = MT_WALK;
		}
	}

	Tile* destTile = _save->getTile(endPos);

	if (isBlocked( // check if destination is blocked
				destTile,
				MapData::O_FLOOR,
				missileTarget)
		|| isBlocked(
				destTile,
				MapData::O_OBJECT,
				missileTarget))
	{
		return;
	}

	// The following check avoids that the unit walks behind the stairs
	// if we click behind the stairs to make it go up the stairs.
	// It only works if the unit is on one of the 2 tiles on the
	// stairs, or on the tile right in front of the stairs.
	// kL_note: I don't want this: ( the function, below, can be removed ).
/*kL
	if (isOnStairs(startPos, endPos))
	{
		endPos.z++;
		destTile = _save->getTile(endPos);
	} */

	while (destTile->getTerrainLevel() == -24
		&& endPos.z != _save->getMapSizeZ())
	{
		endPos.z++;
		destTile = _save->getTile(endPos);
	}

	// check if we have a floor, else lower destination.
	// kL_note: This allows click in the air for non-flyers
	// and they target the ground tile below the clicked tile.
	if (_movementType != MT_FLY)
	{
		while (canFallDown( // for large & small units.
						destTile,
						unit->getArmor()->getSize()))
		{
			endPos.z--;
			destTile = _save->getTile(endPos);
			//Log(LOG_INFO) << ". canFallDown() -1 level, endPos = " << endPos;
		}
	}

	int size = unit->getArmor()->getSize() - 1;
	if (size > 0) // for large units only.
	{
		//Log(LOG_INFO) << ". checking large unit blockage";
		int i = 0;
		const int dir[3] = {4, 2, 3};

		for (int
				x = 0;
				x <= size;
				x += size)
		{
			for (int
					y = 0;
					y <= size;
					y += size)
			{
				if (x || y)
				{
					Tile* testTile = _save->getTile(endPos + Position(x, y, 0));
					if (x && y
						&& ((testTile->getMapData(MapData::O_NORTHWALL)
								&& testTile->getMapData(MapData::O_NORTHWALL)->isDoor())
							||  (testTile->getMapData(MapData::O_WESTWALL)
								&& testTile->getMapData(MapData::O_WESTWALL)->isDoor())))
					{
						return;
					}
					else if (isBlocked(
									destTile,
									testTile,
									dir[i],
									unit)
						&& isBlocked(
									destTile,
									testTile,
									dir[i],
									missileTarget))
					{
						return;
					}
					else if (testTile->getUnit())
					{
						BattleUnit* testUnit = testTile->getUnit();
						if (testUnit != unit
							&& testUnit != missileTarget
							&& testUnit->getVisible())
						{
							return;
						}
					}

					++i;
				}
			}
		}
	}

	// Strafing allowed only to adjacent squares on same z
	// ( z-rule mainly to simplify walking render ).
	// kL_note: This is 'bugged'/featured because it allows soldiers to strafe
	// around a corner (ie, dest is only 1 tile distant but move is actually
	// across 2+ tiles at 90deg. path-angle) -> now accounted for and *allowed*
	Position startPos = unit->getPosition();

	bool isTank = unit->getUnitRules()
				&& unit->getUnitRules()->getMechanical();

	_strafeMove = strafeRejected == false
				&& Options::strafe
				&& ((_modCTRL == true
						&& isTank == false)
//						&& unit->getTurretType() == -1)
					|| (_modALT == true
						&& isTank == true)) // should create Ruleset param 'trackedVehicle' <- DONE.
//						&& unit->getTurretType() > -1))
				&& startPos.z == endPos.z
				&& abs(startPos.x - endPos.x) < 2
				&& abs(startPos.y - endPos.y) < 2;
	//if (_strafeMove) Log(LOG_INFO) << "Pathfinding::calculate() _strafeMove VALID";
	//else Log(LOG_INFO) << "Pathfinding::calculate() _strafeMove INVALID";


	// look for a possible fast and accurate bresenham path and skip A*
	bool sneak = unit->getFaction() == FACTION_HOSTILE
				&& Options::sneakyAI;

	if (startPos.z == endPos.z
		&& bresenhamPath(
					startPos,
					endPos,
					missileTarget,
					sneak))
	{
		//Log(LOG_INFO) << "bresenhamPath";
		std::reverse( // paths are stored in reverse order
				_path.begin(),
				_path.end());

		// kL_begin:
		if (_strafeMove
			&& _path.size() > 2) // no strafe then! recalculate!!!
		{
			//Log(LOG_INFO) << "strafeRejected, bresenhamPath";
			calculate(
					unit,
					endPos2,
					missileTarget,
					maxTUCost,
					true);

			BattleAction* action = _save->getBattleGame()->getCurrentAction();
			action->strafe = false;
			action->dash = true;
			action->actor->setDashing(true);
				// why these things were set up in BattlescapeGame::primaryAction()
				// rather than here I don't know ....
		} // kL_end.

//		_kneelCheck = true; // to ensure this doesn't conflict w/ BattlescapeState::btnUnitUp/DownClick()

		return;
	}
	else
	{
		//Log(LOG_INFO) << "bresenhamPath aborted";
		// if bresenham failed, we shouldn't keep the path
		// it was attempting, in case A* fails too.
		abortPath();
	}

	if (!aStarPath( // now try through A*
				startPos,
				endPos,
				missileTarget,
				sneak,
				maxTUCost))
	{
		//Log(LOG_INFO) << "aStarPath aborted";
		abortPath();
	}
	else
	{
		//Log(LOG_INFO) << "aStarPath";

		// kL_begin:
		if (_strafeMove
			&& _path.size() > 2) // no strafe then! recalculate!!!
		{
			//Log(LOG_INFO) << "strafeRejected, aStarPath";
			calculate(
					unit,
					endPos2,
					missileTarget,
					maxTUCost,
					true);

			BattleAction* action = _save->getBattleGame()->getCurrentAction();
			action->strafe = false;
			action->dash = true;
			action->actor->setDashing(true);
				// why these things were set up in BattlescapeGame::primaryAction()
				// rather than here I don't know ....
		} // kL_end.
	}

//	_kneelCheck = true; // to ensure this doesn't conflict w/ BattlescapeState::btnUnitUp/DownClick()
}

/**
 * Calculates the shortest path using Brensenham path algorithm.
 * @note This only works in the X/Y plane.
 * @param origin		- reference the position to start from
 * @param target		- reference the position to end at
 * @param missileTarget	- pointer to targeted BattleUnit
 * @param sneak			- true if unit is sneaking (default false)
 * @param maxTUCost		- maximum time units the path can cost (default 1000)
 * @return, true if a path is found
 */
bool Pathfinding::bresenhamPath(
		const Position& origin,
		const Position& target,
		BattleUnit* missileTarget,
		bool sneak,
		int maxTUCost)
{
	//Log(LOG_INFO) << "Pathfinding::bresenhamPath()";
	int
		xd[8] = { 0, 1, 1, 1, 0,-1,-1,-1},
		yd[8] = {-1,-1, 0, 1, 1, 1, 0,-1},

		x, x0, x1, delta_x, step_x,
		y, y0, y1, delta_y, step_y,
		z, z0, z1, delta_z, step_z,

		swap_xy, swap_xz,
		drift_xy, drift_xz,

		cx, cy, cz,

		dir,
		lastTUCost = -1;

	Position lastPoint(origin);
	Position nextPoint;

	_totalTUCost = 0;

	// start and end points
	x0 = origin.x; x1 = target.x;
	y0 = origin.y; y1 = target.y;
	z0 = origin.z; z1 = target.z;

	// 'steep' xy Line, make longest delta x plane
	swap_xy = abs(y1 - y0) > abs(x1 - x0);
	if (swap_xy)
	{
		std::swap(x0, y0);
		std::swap(x1, y1);
	}

	// do same for xz
	swap_xz = abs(z1 - z0) > abs(x1 - x0);
	if (swap_xz)
	{
		std::swap(x0, z0);
		std::swap(x1, z1);
	}

	// delta is Length in each plane
	delta_x = abs(x1 - x0);
	delta_y = abs(y1 - y0);
	delta_z = abs(z1 - z0);

	// drift controls when to step in 'shallow' planes
	// starting value keeps Line centred
	drift_xy = (delta_x / 2);
	drift_xz = (delta_x / 2);

	// direction of line
	step_x = 1; if (x0 > x1) step_x = -1;
	step_y = 1; if (y0 > y1) step_y = -1;
	step_z = 1; if (z0 > z1) step_z = -1;

	// starting point
	y = y0;
	z = z0;

	// step through longest delta (which we have swapped to x)
	for (
			x = x0;
			x != (x1 + step_x);
			x += step_x)
	{
		// copy position
		cx = x;
		cy = y;
		cz = z;

		// unswap (in reverse)
		if (swap_xz) std::swap(cx, cz);
		if (swap_xy) std::swap(cx, cy);

		if (x != x0 || y != y0 || z != z0)
		{
			Position realNextPoint = Position(cx, cy, cz);
			nextPoint = realNextPoint;

			// get direction
			for (
					dir = 0;
					dir < 8;
					++dir)
			{
				if (xd[dir] == cx - lastPoint.x
					&& yd[dir] == cy - lastPoint.y)
				{
					break;
				}
			}

			int tuCost = getTUCost(
								lastPoint,
								dir,
								&nextPoint,
								_unit,
								missileTarget,
								(missileTarget && maxTUCost == 10000));
			//Log(LOG_INFO) << ". TU Cost = " << tuCost;

			if (sneak
				&& _save->getTile(nextPoint)->getVisible())
			{
				return false;
			}

			// delete the following
			bool isDiagonal = (dir & 1);
			int
				lastTUCostDiagonal = lastTUCost + lastTUCost / 2,
				tuCostDiagonal = tuCost + tuCost / 2;

			if (nextPoint == realNextPoint
				&& tuCost < 255
				&& (tuCost == lastTUCost
					|| (isDiagonal
						&& tuCost == lastTUCostDiagonal)
					|| (isDiagonal == false
						&& tuCostDiagonal == lastTUCost)
					|| lastTUCost == -1)
				&& isBlocked(
						_save->getTile(lastPoint),
						_save->getTile(nextPoint),
						dir,
						missileTarget) == false)
			{
				_path.push_back(dir);
			}
			else
				return false;

			if (missileTarget == NULL
				&& tuCost != 255)
			{
				lastTUCost = tuCost;
				_totalTUCost += tuCost;
			}

			lastPoint = Position(cx, cy, cz);
		}

		// update progress in other planes
		drift_xy = drift_xy - delta_y;
		drift_xz = drift_xz - delta_z;

		// step in y plane
		if (drift_xy < 0)
		{
			y = y + step_y;
			drift_xy = drift_xy + delta_x;
		}

		// same in z
		if (drift_xz < 0)
		{
			z = z + step_z;
			drift_xz = drift_xz + delta_x;
		}
	}

	return true;
}

/**
 * Calculates the shortest path using a simple A-Star algorithm.
 * The unit information and movement type must have already been set.
 * The path information is set only if a valid path is found.
 * @param startPosition	- reference the position to start from
 * @param endPosition	- reference the position to end at
 * @param missileTarget	- pointer to targeted BattleUnit
 * @param sneak			- true if the unit is sneaking (default false)
 * @param maxTUCost		- maximum time units this path can cost (default 1000)
 * @return, true if a path is found
 */
bool Pathfinding::aStarPath(
		const Position& startPosition,
		const Position& endPosition,
		BattleUnit* missileTarget,
		bool sneak,
		int maxTUCost)
{
	//Log(LOG_INFO) << "Pathfinding::aStarPath()";

	// reset every node, so have to check them all
	for (std::vector<PathfindingNode>::iterator
			i = _nodes.begin();
			i != _nodes.end();
			++i)
	{
		i->reset();
	}

	// start position is the first one in the "open" list
	PathfindingNode* start = getNode(startPosition);
	start->connect(0, 0, 0, endPosition);
	PathfindingOpenSet openList;
	openList.push(start);

	bool missile = missileTarget
				&& maxTUCost == -1;

	while (openList.empty() == false) // if the openList is empty, reached the end
	{
		PathfindingNode* currentNode = openList.pop();
		Position const &currentPos = currentNode->getPosition();
		currentNode->setChecked();

		if (currentPos == endPosition) // found the target.
		{
			_path.clear();

			PathfindingNode* pf = currentNode;
			while (pf->getPrevNode())
			{
				_path.push_back(pf->getPrevDir());
				pf = pf->getPrevNode();
			}

			return true;
		}

		for (int // try all reachable neighbours.
				direction = 0;
				direction < 10; // dir 0 thro 7, up/down
				direction++)
		{
			//Log(LOG_INFO) << ". try direction ... " << direction;

			Position nextPos;

			int tuCost = getTUCost(
								currentPos,
								direction,
								&nextPos,
								_unit,
								missileTarget,
								missile);
			//Log(LOG_INFO) << ". TU Cost = " << tuCost;
			if (tuCost >= 255) // Skip unreachable / blocked
				continue;

			if (sneak
				&& _save->getTile(nextPos)->getVisible())
			{
				tuCost *= 2; // avoid being seen
			}

			PathfindingNode* nextNode = getNode(nextPos);
			if (nextNode->isChecked()) // algorithm means this node is already at minimum cost
				//Log(LOG_INFO) << ". node already Checked ... cont.";
				continue;

			_totalTUCost = currentNode->getTUCost(missile) + tuCost;

			// if this node is unvisited or has only been visited from inferior paths...
			if ((nextNode->inOpenSet() == false
					|| nextNode->getTUCost(missile) > _totalTUCost)
				&& _totalTUCost <= maxTUCost)
			{
				//Log(LOG_INFO) << ". nodeChecked(dir) = " << direction << " totalCost = " << _totalTUCost;
				nextNode->connect(
								_totalTUCost,
								currentNode,
								direction,
								endPosition);

				openList.push(nextNode);
			}
		}
	}

	return false; // unable to reach the target
}

/**
 * Gets the TU cost to move from 1 tile to the other (ONE STEP ONLY).
 * But also updates the endPosition, because it is possible
 * the unit goes upstairs or falls down while walking.
 * @param startPos		- reference to the start position
 * @param dir			- direction of movement
 * @param endPos		- pointer to destination Position
 * @param unit			- pointer to unit
 * @param missileTarget	- pointer to targeted BattleUnit
 * @param missile		- true if a guided missile
 * @return, TU cost or 255 if movement is impossible
 */
int Pathfinding::getTUCost(
		const Position& startPos,
		int dir,
		Position* endPos,
		BattleUnit* unit,
		BattleUnit* missileTarget,
		bool missile)
{
	//Log(LOG_INFO) << "Pathfinding::getTUCost()";
	_unit = unit;

	directionToVector(
					dir,
					endPos);
	*endPos += startPos;

	bool
		fellDown	= false,
		triedStairs	= false;

	int
		partsGoingUp		= 0,
		partsGoingDown		= 0,
		partsFalling		= 0,
		partsChangingHeight	= 0,

		cost		= 0,
		totalCost	= 0;

	_openDoor = 0;

	int size = _unit->getArmor()->getSize() - 1;
	for (int
			x = 0;
			x <= size;
			++x)
	{
		for (int
				y = 0;
				y <= size;
				++y)
		{
			Position offset = Position(x, y, 0);

			Tile* startTile	= _save->getTile(startPos + offset);
			Tile* destTile	= _save->getTile(*endPos + offset);

			// this means the destination is probably outside the map
			if (startTile == NULL
				|| destTile == NULL)
			{
				return 255;
			}

			// don't let tanks phase through doors.
			if (x && y)
			{
				if ((destTile->getMapData(MapData::O_NORTHWALL)
						&& destTile->getMapData(MapData::O_NORTHWALL)->isDoor())
					|| (destTile->getMapData(MapData::O_WESTWALL)
						&& destTile->getMapData(MapData::O_WESTWALL)->isDoor()))
				{
					return 255;
				}
			}

			// 'terrainLevel' starts at 0 (floor) and goes up to -24 *cuckoo**
			if (dir < DIR_UP
				&& startTile->getTerrainLevel() > -16) // lower than half.
			{
				// check if we can go this way
				if (isBlocked(
							startTile,
							destTile,
							dir,
							missileTarget))
				{
					return 255;
				}

				if (startTile->getTerrainLevel() - destTile->getTerrainLevel() > 8) // greater than 1/3 step up.
					return 255;
			}


			// this will later be used to re-init the start tile
			Position vertOffset(0, 0, 0);

			Tile* belowDest = _save->getTile(*endPos + offset + Position(0, 0,-1));
			Tile* aboveDest = _save->getTile(*endPos + offset + Position(0, 0, 1));

			// if we are on a stairs try to go up a level
			if (dir < DIR_UP
				&& startTile->getTerrainLevel() < -15 // less than 1/3 step up.
				&& aboveDest->hasNoFloor(destTile) == false
				&& triedStairs == false)
			{
				partsGoingUp++;

				if (partsGoingUp > size)
				{
					vertOffset.z++;
					endPos->z++;
					destTile = _save->getTile(*endPos + offset);
					belowDest = _save->getTile(*endPos + Position(x, y,-1));

					triedStairs = true;
				}
			}
			else if (fellDown == false // for safely walking down ramps or stairs ...
				&& _movementType != MT_FLY
				&& canFallDown(destTile)
				&& belowDest
				&& belowDest->getTerrainLevel() < -11)	// less than 1/2 step down.
			{
				partsGoingDown++;

				if (partsGoingDown == (size + 1) * (size + 1))
				{
					endPos->z--;
					destTile = _save->getTile(*endPos + offset);
					belowDest = _save->getTile(*endPos + Position(x, y,-1));

					fellDown = true;
				}
			}
			else if (_movementType == MT_FLY
				&& belowDest
				&& belowDest->getUnit()
				&& belowDest->getUnit() != unit)
			{
				// 2 or more voxels poking into this tile -> no go
				if (belowDest->getUnit()->getHeight()
							+ belowDest->getUnit()->getFloatHeight()
							- belowDest->getTerrainLevel()
						> 26)
				{
					return 255;
				}
			}

			// this means the destination is probably outside the map
			if (destTile == NULL)
				return 255;

			if (dir < DIR_UP
				&& endPos->z == startTile->getPosition().z)
			{
				// check if we can go this way
				if (isBlocked(
							startTile,
							destTile,
							dir,
							missileTarget))
				{
					return 255;
				}

				if (startTile->getTerrainLevel() - destTile->getTerrainLevel() > 8)
					return 255;
			}
			else if (dir >= DIR_UP)
			{
				// check if we can go up or down through gravlift or fly
				int vert = validateUpDown(
										unit,
										startPos + offset,
										dir);
				//Log(LOG_INFO) << ". vert = " << vert;
				if (vert < 1)
					return 255;
				else
					cost = 8; // vertical movement by flying suit or grav lift
			}

			// check if we have floor, else fall down;
			// for walking off roofs and finding yerself in mid-air ...
			if (fellDown == false
				&& _movementType != MT_FLY
				&& canFallDown(startTile))
			{
				partsFalling++;

				if (partsFalling == (size + 1) * (size + 1))
				{
					*endPos = startPos + Position(0, 0,-1);
					destTile = _save->getTile(*endPos + offset);
					belowDest = _save->getTile(*endPos + Position(x, y,-1));
					dir = DIR_DOWN;
					fellDown = true;
				}
			}

			startTile = _save->getTile(startTile->getPosition() + vertOffset);

			if (dir < DIR_UP
				&& partsGoingUp != 0)
			{
				// check if we can go this way
				if (isBlocked(
							startTile,
							destTile,
							dir,
							missileTarget))
				{
					return 255;
				}

				if (startTile->getTerrainLevel() - destTile->getTerrainLevel() > 8)
					return 255;
			}

			// check if the destination tile can be walked over
			if (isBlocked(
						destTile,
						MapData::O_FLOOR,
						missileTarget)
				|| isBlocked(
							destTile,
							MapData::O_OBJECT,
							missileTarget))
			{
				return 255;
			}


			if (dir < DIR_UP)
			{
				cost += destTile->getTUCost(
										MapData::O_FLOOR,
										_movementType);

				if (fellDown == false
					&& triedStairs == false
					&& destTile->getMapData(MapData::O_OBJECT))
				{
					cost += destTile->getTUCost(
											MapData::O_OBJECT,
											_movementType);
				}

				// climbing up a level costs one extra
				if (vertOffset.z > 0)
					cost++;

				// if we don't want to fall down and there is no floor,
				// we can't know the TUs so it defaults to 4
				if (fellDown == false
					&& destTile->hasNoFloor(NULL))
				{
					cost = 4;
				}

				int wallCost = 0;	// walking through rubble walls --
									// but don't charge for walking diagonally through doors (which is impossible);
									// they're a special case unto themselves, if we can walk past them diagonally,
									// it means we can go around since there is no wall blocking us.
				int sides = 0;		// how many walls we cross when moving diagonally
				int wallTU = 0;		// used to check if there's a wall that costs +TU.

				if (dir == 7
					|| dir == 0
					|| dir == 1)
				{
					//Log(LOG_INFO) << ". from " << startTile->getPosition() << " to " << destTile->getPosition() << " dir = " << dir;
					wallTU = startTile->getTUCost( // ( 'walkover' bigWalls not incl. -- none exists )
												MapData::O_NORTHWALL,
												_movementType);
					if (wallTU > 0)
					{
//						if (dir &1) // would use this to increase diagonal wall-crossing by +50%
//							wallTU += wallTU / 2;

						wallCost += wallTU;
						sides++;

						if (startTile->getMapData(MapData::O_NORTHWALL)->isDoor()
							|| startTile->getMapData(MapData::O_NORTHWALL)->isUFODoor())
						{
							//Log(LOG_INFO) << ". . . _openDoor[N] = TRUE, wallTU = " << wallTU;
							if (wallTU > _openDoor) // don't let large unit parts reset _openDoor prematurely
								_openDoor = wallTU;
						}
					}
				}

				if (dir == 1
					|| dir == 2
					|| dir == 3)
				{
					//Log(LOG_INFO) << ". from " << startTile->getPosition() << " to " << destTile->getPosition() << " dir = " << dir;

					// Test: i don't know why 'aboveDest' don't work here
					Tile* aboveDestTile = _save->getTile(destTile->getPosition() + Position(0, 0, 1));

					if (startTile->getPosition().z <= destTile->getPosition().z) // don't count wallCost if it's on the floor below.
					{
						wallTU = destTile->getTUCost(
													MapData::O_WESTWALL,
													_movementType);
						//Log(LOG_INFO) << ". . eastish, wallTU = " << wallTU;
						if (wallTU > 0)
						{
							wallCost += wallTU;
							sides++;

							if (destTile->getMapData(MapData::O_WESTWALL)->isDoor()
								|| destTile->getMapData(MapData::O_WESTWALL)->isUFODoor())
							{
								//Log(LOG_INFO) << ". . . _openDoor[E] = TRUE, wallTU = " << wallTU;
								if (wallTU > _openDoor)
									_openDoor = wallTU;
							}
						}
					}
					else if (aboveDestTile != NULL // should be redundant, really
						&& aboveDestTile->getMapData(MapData::O_WESTWALL) != NULL) // going downwards...
					{
						wallTU = aboveDestTile->getTUCost(
													MapData::O_WESTWALL,
													_movementType);
						//Log(LOG_INFO) << ". . (down) eastish, wallTU = " << wallTU;
						if (wallTU > 0)
						{
							wallCost += wallTU;
							sides++;

							if (aboveDestTile->getMapData(MapData::O_WESTWALL)->isDoor()
								|| aboveDestTile->getMapData(MapData::O_WESTWALL)->isUFODoor())
							{
								//Log(LOG_INFO) << ". . . _openDoor[E] = TRUE (down), wallTU = " << wallTU;
								if (wallTU > _openDoor)
									_openDoor = wallTU;
							}
						}
					}
				}

				if (dir == 3
					|| dir == 4
					|| dir == 5)
				{
					//Log(LOG_INFO) << ". from " << startTile->getPosition() << " to " << destTile->getPosition() << " dir = " << dir;

					// Test: i don't know why 'aboveDest' don't work here
					Tile* aboveDestTile = _save->getTile(destTile->getPosition() + Position(0, 0, 1));

					if (startTile->getPosition().z <= destTile->getPosition().z) // don't count wallCost if it's on the floor below.
					{
						wallTU = destTile->getTUCost(
													MapData::O_NORTHWALL,
													_movementType);
						if (wallTU > 0)
						{
							wallCost += wallTU;
							sides++;

							if (destTile->getMapData(MapData::O_NORTHWALL)->isDoor()
								|| destTile->getMapData(MapData::O_NORTHWALL)->isUFODoor())
							{
								//Log(LOG_INFO) << ". . . _openDoor[S] = TRUE, wallTU = " << wallTU;
								if (wallTU > _openDoor)
									_openDoor = wallTU;
							}
						}
					}
					else if (aboveDestTile != NULL // should be redundant, really
						&& aboveDestTile->getMapData(MapData::O_NORTHWALL) != NULL) // going downwards...
					{
						wallTU = aboveDestTile->getTUCost(
													MapData::O_NORTHWALL,
													_movementType);

						if (wallTU > 0)
						{
							wallCost += wallTU;
							sides++;

							if (aboveDestTile->getMapData(MapData::O_NORTHWALL)->isDoor()
								|| aboveDestTile->getMapData(MapData::O_NORTHWALL)->isUFODoor())
							{
								//Log(LOG_INFO) << ". . . _openDoor[S] = TRUE (down), wallTU = " << wallTU;
								if (wallTU > _openDoor)
									_openDoor = wallTU;
							}
						}
					}
				}

				if (dir == 5
					|| dir == 6
					|| dir == 7)
				{
					//Log(LOG_INFO) << ". from " << startTile->getPosition() << " to " << destTile->getPosition() << " dir = " << dir;
					wallTU = startTile->getTUCost(
												MapData::O_WESTWALL,
												_movementType); // ( bigWalls not incl. yet )
					if (wallTU > 0)
					{
						wallCost += wallTU;
						sides++;

						if (startTile->getMapData(MapData::O_WESTWALL)->isDoor()
							|| startTile->getMapData(MapData::O_WESTWALL)->isUFODoor())
						{
							//Log(LOG_INFO) << ". . . _openDoor[W] = TRUE, wallTU = " << wallTU;
							if (wallTU > _openDoor)
								_openDoor = wallTU;
						}
					}
				}

				// diagonal walking (uneven directions) costs 50% more tu's
				// kL_note: this is moved up so that objects don't cost +150% tu;
				// instead, let them keep a flat +100% to step onto
				// -- note that Walls also do not take +150 tu to step over diagonally....
				if (dir & 1)
				{
					cost = static_cast<int>(static_cast<float>(cost) * 1.5f);

					if (wallCost > 0
						&& _openDoor == 0)
					{
						if (sides > 0)
							wallCost /= sides; // average of the wall-sides crossed

						if ((wallCost - sides) %2 == 1) // round wallCost up.
							wallCost += 1;
					}
				}

				//Log(LOG_INFO) << ". wallCost = " << wallCost;
				//Log(LOG_INFO) << ". cost[0] = " << cost;
				cost += wallCost;
				//Log(LOG_INFO) << ". cost[1] = " << cost;
			}


			if (_unit->getFaction() == FACTION_HOSTILE
				&& destTile->getFire() > 0)
			{
				cost += 32; // try to find a better path, but don't exclude this path entirely.
			}

			// TFTD thing: tiles on fire are cost 2 TUs more for whatever reason.
			if (_save->getDepth() > 0
				&& destTile->getFire() > 0)
			{
				cost += 2;
			}

			// Propose: if flying then no extra TU cost
			//Log(LOG_INFO) << ". pathSize = " << (int)_path.size();
			if (_strafeMove)
			{
				// kL_begin: extra TU for strafe-moves ->	1 0 1
				//											2 ^ 2
				//											3 2 3
				int delta = abs((dir + 4) %8 - _unit->getDirection());

				if (_unit->getUnitRules()
					&& _unit->getUnitRules()->getMechanical()
					&& 1 < delta && delta != 7)
				{
					_strafeMove = false;
				}
				else if (_unit->getDirection() != dir)
				{
					int delta = std::min(
										abs(8 + dir - _unit->getDirection()),
										std::min(
												abs(_unit->getDirection() - dir),
												abs(8 + _unit->getDirection() - dir)));
					if (delta == 4)
						delta = 2;

					cost += delta;
				}
/*				else if (_unit->getDirection() == dir // forced dash 1 tile straight forward.
					&& ((SDL_GetModState() & KMOD_RSHIFT) != 0)) // try this one ... uh exploitable.
				// note: Unfortunately, this overrides a 2-tile strafe around corners, and
				// I can't get _path.size() to return a usable value .... to differentiate.
				// eg. if (_path.size() < 1) <- TUCost is ascertained *before*
				// the step is added to _path<vector>
				{
					_strafeMove = false;
					cost = cost * 3 / 4;

					BattleAction* action = _save->getBattleGame()->getCurrentAction();
					action->strafe = false;
					action->dash = true;
					action->actor->setDashing(true);
				} */ // kL_end.
			}

			totalCost += cost;
			cost = 0;
		}
	}

	// for bigger sized units, check the path between part 1,1 and part 0,0 at end position
	if (size > 0)
	{
		double tCost = ceil(static_cast<double>(totalCost) / static_cast<double>((size + 1) * (size + 1))); // kL
		totalCost = static_cast<int>(Round(tCost)); // kL: round those tanks up!

		Tile* startTile = _save->getTile(*endPos + Position(1, 1, 0));
		Tile* destTile = _save->getTile(*endPos);

		int dirTest = 7;
		if (isBlocked(
					startTile,
					destTile,
					dirTest,
					missileTarget))
		{
			return 255;
		}
		else if (fellDown == false
			&& abs(startTile->getTerrainLevel() - destTile->getTerrainLevel()) > 10)
		{
			return 255;
		}
		// also check if we change level, that there are two parts changing level,
		// so a big sized unit can not go up a small sized stairs
		else if (partsChangingHeight == 1)
			return 255;
	}

	if (missile)
		return 0;
	else
	{
		//Log(LOG_INFO) << "Pathfinding::getTUCost() ret = " << totalCost;
		return totalCost;
	}
}

/**
 * Converts direction to a vector. Direction starts north = 0 and goes clockwise.
 * @param direction	- source direction
 * @param vector	- pointer to a Position (which acts as a vector)
 */
void Pathfinding::directionToVector(
		int const direction,
		Position* vector)
{
	int
		x[10] = { 0, 1, 1, 1, 0,-1,-1,-1, 0, 0},
		y[10] = {-1,-1, 0, 1, 1, 1, 0,-1, 0, 0},
		z[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 1,-1};

	vector->x = x[direction];
	vector->y = y[direction];
	vector->z = z[direction];
}

/**
 * Converts vector to a direction. Direction starts north = 0 and goes clockwise.
 * @param vector	- reference a Position (which acts as a vector)
 * @param dir		- reference the resulting direction (up/down & same-tile sets dir = -1)
 */
void Pathfinding::vectorToDirection(
		const Position& vector,
		int& dir)
{
	dir = -1;

	int
		x[8] = { 0, 1, 1, 1, 0,-1,-1,-1},
		y[8] = {-1,-1, 0, 1, 1, 1, 0,-1};

	for (int
			i = 0;
			i < 8;
			++i)
	{
		if (x[i] == vector.x
			&& y[i] == vector.y)
		{
			dir = i;
			return;
		}
	}
}

/**
 * Checks whether a path is ready and gives the first direction.
 * @return, direction where the unit needs to go next, -1 if it's the end of the path
 */
int Pathfinding::getStartDirection()
{
	if (_path.empty())
		return -1;

	return _path.back();
}

/**
 * Dequeues the next path direction. Ie, returns the direction and removes it from queue.
 * @return, direction where the unit needs to go next, -1 if it's the end of the path
 */
int Pathfinding::dequeuePath()
{
	if (_path.empty())
		return -1;

	int last_element = _path.back();
	_path.pop_back();

	return last_element;
}

/**
 * Aborts the current path. Clears the path vector.
 */
void Pathfinding::abortPath()
{
	_modCTRL = (SDL_GetModState() & KMOD_CTRL) != 0;	// kL
	_modALT = (SDL_GetModState() & KMOD_ALT) != 0;		// kL

	_totalTUCost = 0;
	_path.clear();
}

/**
 * Determines whether a certain part of a tile blocks movement.
 * @param tile				- pointer to a specified Tile, can be NULL
 * @param part				- part of the tile
 * @param missileTarget		- pointer to targeted BattleUnit (default NULL)
 * @param bigWallExclusion	- for diagonal bigwalls (default -1)
 * @return, true if path is blocked
 */
bool Pathfinding::isBlocked( // private
		Tile* tile,
		const int part,
		BattleUnit* missileTarget,
		int bigWallExclusion)
{
	//Log(LOG_INFO) << "Pathfinding::isBlocked() #2";
	if (tile == NULL) // probably outside the map here
		return true;
/*
BIGWALL_NONE,	// 0
BIGWALL_BLOCK,	// 1
BIGWALL_NESW,	// 2
BIGWALL_NWSE,	// 3
BIGWALL_WEST,	// 4
BIGWALL_NORTH,	// 5
BIGWALL_EAST,	// 6
BIGWALL_SOUTH,	// 7
BIGWALL_E_S		// 8
*/
	// kL_begin:
	if ((tile->getMapData(MapData::O_WESTWALL)
			&& tile->getMapData(MapData::O_WESTWALL)->getBigWall() == BIGWALL_BLOCK)
		|| (tile->getMapData(MapData::O_NORTHWALL)
			&& tile->getMapData(MapData::O_NORTHWALL)->getBigWall() == BIGWALL_BLOCK))
//		|| (tile->getMapData(MapData::O_OBJECT)
//			&& tile->getMapData(MapData::O_OBJECT)->getBigWall() == BIGWALL_BLOCK))
	{
		return true;
	} // kL_end.

	if (part == O_BIGWALL)
	{
		//Log(LOG_INFO) << ". part is Bigwall";
		if (tile->getMapData(MapData::O_OBJECT)
			&& tile->getMapData(MapData::O_OBJECT)->getBigWall() != 0
			&& tile->getMapData(MapData::O_OBJECT)->getBigWall() < BIGWALL_WEST
			&& tile->getMapData(MapData::O_OBJECT)->getBigWall() != bigWallExclusion)
		{
			return true; // blocking part
		}
		else
			return false;
	}

	if (part == MapData::O_WESTWALL)
	{
		//Log(LOG_INFO) << ". part is Westwall";
		if (tile->getMapData(MapData::O_OBJECT)
			&& (tile->getMapData(MapData::O_OBJECT)->getBigWall() == BIGWALL_WEST
				|| tile->getMapData(MapData::O_OBJECT)->getBigWall() == BIGWALL_W_N
				|| tile->getMapData(MapData::O_OBJECT)->getBigWall() == BIGWALL_BLOCK)) // kL
		{
			return true; // blocking part
		}

		Tile* tileWest = _save->getTile(tile->getPosition() + Position(-1, 0, 0));
		if (tileWest == NULL)
			return true; // do not look outside of map

		if (tileWest->getMapData(MapData::O_OBJECT)
			&& (tileWest->getMapData(MapData::O_OBJECT)->getBigWall() == BIGWALL_EAST
				|| tileWest->getMapData(MapData::O_OBJECT)->getBigWall() == BIGWALL_E_S
				|| tileWest->getMapData(MapData::O_OBJECT)->getBigWall() == BIGWALL_BLOCK)) // kL
		{
			return true; // blocking part
		}
	}

	if (part == MapData::O_NORTHWALL)
	{
		//Log(LOG_INFO) << ". part is Northwall";
		if (tile->getMapData(MapData::O_OBJECT)
			&& (tile->getMapData(MapData::O_OBJECT)->getBigWall() == BIGWALL_NORTH
				|| tile->getMapData(MapData::O_OBJECT)->getBigWall() == BIGWALL_W_N
				|| tile->getMapData(MapData::O_OBJECT)->getBigWall() == BIGWALL_BLOCK)) // kL
		{
			return true; // blocking part
		}

		Tile* tileNorth = _save->getTile(tile->getPosition() + Position(0,-1, 0));
		if (tileNorth == NULL)
			return true; // do not look outside of map

		if (tileNorth->getMapData(MapData::O_OBJECT)
			&& (tileNorth->getMapData(MapData::O_OBJECT)->getBigWall() == BIGWALL_SOUTH
				|| tileNorth->getMapData(MapData::O_OBJECT)->getBigWall() == BIGWALL_E_S
				|| tileNorth->getMapData(MapData::O_OBJECT)->getBigWall() == BIGWALL_BLOCK)) // kL
		{
			return true; // blocking part
		}
	}

	if (part == MapData::O_FLOOR)
	{
		//Log(LOG_INFO) << ". part is Floor";
		if (tile->getUnit())
		{
			BattleUnit* tileUnit = tile->getUnit();

			if (tileUnit == _unit
				|| tileUnit == missileTarget
				|| tileUnit->isOut(true, true))
			{
				return false;
			}

			if (_unit
				&& _unit->getFaction() == FACTION_PLAYER
				&& tileUnit->getVisible())
			{
				return true; // player knows about visible units only
			}

			if (_unit
				&& _unit->getFaction() == tileUnit->getFaction())
			{
				return true; // AI knows all allied units
			}

			if (_unit
				&& _unit->getFaction() == FACTION_HOSTILE
				&& std::find(
						_unit->getUnitsSpottedThisTurn().begin(),
						_unit->getUnitsSpottedThisTurn().end(),
						tileUnit)
					!= _unit->getUnitsSpottedThisTurn().end())
			{
				return true; // AI knows only spotted xCom units.
			}
		}
		else if (tile->hasNoFloor(NULL) // this whole section is devoted to making large units
			&& _movementType != MT_FLY) // not take part in any kind of falling behaviour
		{
			Position pos = tile->getPosition();
			while (pos.z >= 0)
			{
				Tile* testTile = _save->getTile(pos);
				BattleUnit* tileUnit = testTile->getUnit();

				if (tileUnit
					&& tileUnit != _unit)
				{
					// don't let large units fall on other units
					if (_unit
						&& _unit->getArmor()->getSize() > 1)
					{
						return true;
					}

					// don't let any units fall on large units
					if (//kL tileUnit != _unit &&
						tileUnit != missileTarget
						&& tileUnit->isOut() == false
						&& tileUnit->getArmor()->getSize() > 1)
					{
						return true;
					}
				}

				// not gonna fall any further, so stop checking.
				if (testTile->hasNoFloor(NULL) == false)
					break;

				pos.z--;
			}
		}
	}

	// missiles can't pathfind through closed doors. kL: neither can proxy mines.
	if (missileTarget != NULL
		&& tile->getMapData(part)
		&& (tile->getMapData(part)->isDoor()
			|| (tile->getMapData(part)->isUFODoor()
				&& tile->isUfoDoorOpen(part) == false)))
	{
		return true;
	}

	if (tile->getTUCost( // blocking part
					part,
					_movementType)
				== 255)
	{
		//Log(LOG_INFO) << "isBlocked() EXIT true, part = " << part << " MT = " << (int)_movementType;
		return true;
	}

	//Log(LOG_INFO) << "isBlocked() EXIT false, part = " << part << " MT = " << (int)_movementType;
	return false;
}

/**
 * Determines whether going from one tile to another blocks movement.
 * @param startTile		- pointer to start Tile
 * @param endTile		- pointer to destination Tile
 * @param dir			- direction of movement
 * @param missileTarget	- pointer to targeted BattleUnit (default NULL)
 * @return, true if path is blocked
 */
bool Pathfinding::isBlocked( // public
		Tile* startTile,
		Tile* /* endTile */,
		const int dir,
		BattleUnit* missileTarget)
{
	//Log(LOG_INFO) << "Pathfinding::isBlocked() #1";

	// check if the difference in height between start and destination is not too high
	// so we can not jump to the highest part of the stairs from the floor
	// stairs terrainlevel goes typically -8 -16 (2 steps) or -4 -12 -20 (3 steps)
	// this "maximum jump height" is therefore set to 8

	const Position pos = startTile->getPosition();

	static const Position tileNorth	= Position( 0,-1, 0);
	static const Position tileEast	= Position( 1, 0, 0);
	static const Position tileSouth	= Position( 0, 1, 0);
	static const Position tileWest	= Position(-1, 0, 0);

	// kL_begin:
	switch (dir)
	{
		case 0:	// north
			//Log(LOG_INFO) << ". try North";
			if (isBlocked(
						startTile,
						MapData::O_NORTHWALL,
						missileTarget))
			{
				return true;
			}
		break;
		case 1: // north-east
			//Log(LOG_INFO) << ". try NorthEast";
			if (isBlocked(
						startTile,
						MapData::O_NORTHWALL,
						missileTarget)
				|| isBlocked(
						_save->getTile(pos + tileEast),
						MapData::O_WESTWALL,
						missileTarget)
				|| isBlocked(
						_save->getTile(pos + tileEast),
						MapData::O_NORTHWALL,
						missileTarget)
				|| isBlocked(
						_save->getTile(pos + tileEast),
						O_BIGWALL,
						missileTarget,
						BIGWALL_NESW)
				|| isBlocked(
						_save->getTile(pos + tileEast + tileNorth),
						MapData::O_WESTWALL,
						missileTarget)
				|| isBlocked(
						_save->getTile(pos + tileNorth),
						O_BIGWALL,
						missileTarget,
						BIGWALL_NESW))
			{
				return true;
			}
		break;
		case 2: // east
			//Log(LOG_INFO) << ". try East";
			if (isBlocked(
						_save->getTile(pos + tileEast),
						MapData::O_WESTWALL,
						missileTarget))
			{
				return true;
			}
		break;
		case 3: // south-east
			//Log(LOG_INFO) << ". try SouthEast";
			if (isBlocked(
						_save->getTile(pos + tileEast),
						MapData::O_WESTWALL,
						missileTarget)
				|| isBlocked(
						_save->getTile(pos + tileEast),
						O_BIGWALL,
						missileTarget,
						BIGWALL_NWSE)
				|| isBlocked(
						_save->getTile(pos + tileEast + tileSouth),
						MapData::O_NORTHWALL,
						missileTarget)
				|| isBlocked(
						_save->getTile(pos + tileEast + tileSouth),
						MapData::O_WESTWALL,
						missileTarget)
				|| isBlocked(
						_save->getTile(pos + tileSouth),
						MapData::O_NORTHWALL,
						missileTarget)
				|| isBlocked(
						_save->getTile(pos + tileSouth),
						O_BIGWALL,
						missileTarget,
						BIGWALL_NWSE))
			{
				return true;
			}
		break;
		case 4: // south
			//Log(LOG_INFO) << ". try South";
			if (isBlocked(
						_save->getTile(pos + tileSouth),
						MapData::O_NORTHWALL,
						missileTarget))
			{
				return true;
			}
		break;
		case 5: // south-west
			//Log(LOG_INFO) << ". try SouthWest";
			if (isBlocked(
						startTile,
						MapData::O_WESTWALL,
						missileTarget)
				|| isBlocked(
						_save->getTile(pos + tileSouth),
						MapData::O_WESTWALL,
						missileTarget)
				|| isBlocked(
						_save->getTile(pos + tileSouth),
						MapData::O_NORTHWALL,
						missileTarget)
				|| isBlocked(
						_save->getTile(pos + tileSouth),
						O_BIGWALL,
						missileTarget,
						BIGWALL_NESW)
				|| isBlocked(
						_save->getTile(pos + tileSouth + tileWest),
						MapData::O_NORTHWALL,
						missileTarget)
				|| isBlocked(
						_save->getTile(pos + tileWest),
						O_BIGWALL,
						missileTarget,
						BIGWALL_NESW))
			{
				return true;
			}
		break;
		case 6: // west
			//Log(LOG_INFO) << ". try West";
			if (isBlocked(
						startTile,
						MapData::O_WESTWALL,
						missileTarget))
			{
				return true;
			}
		break;
		case 7: // north-west
			//Log(LOG_INFO) << ". try NorthWest";
			if (isBlocked(
						startTile,
						MapData::O_WESTWALL,
						missileTarget)
				|| isBlocked(
						startTile,
						MapData::O_NORTHWALL,
						missileTarget)
				|| isBlocked(
						_save->getTile(pos + tileWest),
						MapData::O_NORTHWALL,
						missileTarget)
				|| isBlocked(
						_save->getTile(pos + tileWest),
						O_BIGWALL,
						missileTarget,
						BIGWALL_NWSE)
				|| isBlocked(
						_save->getTile(pos + tileNorth),
						MapData::O_WESTWALL,
						missileTarget)
				|| isBlocked(
						_save->getTile(pos + tileNorth),
						O_BIGWALL,
						missileTarget,
						BIGWALL_NWSE))
			{
				return true;
			}
		break;
	} // kL_end.

	return false;
}

/**
 * Determines whether a unit can fall down from this tile.
 * Can fall down here
 * if the current position is higher than 0 (the tileBelow does not exist)
 * or if the tile has no floor (and there is no unit standing below).
 * @param tile - the current tile
 * @return, true if a unit on tile can fall down
 */
// aha! (kL) This is why that sectoid stood in the air: it walked off
// the top of a building but there was a cyberdisc below!!! NOT.
bool Pathfinding::canFallDown(Tile* tile)
{
	if (tile->getPosition().z == 0) // already on lowest maplevel
		return false;

	Tile* tileBelow = _save->getTile(tile->getPosition() - Position(0, 0, 1));

	return tile->hasNoFloor(tileBelow);
}

/**
 * Wrapper for canFallDown() above.
 * @param tile - pointer to the current Tile
 * @param size - the size of the unit
 * @return, true if a unit on tile can fall down
 */
bool Pathfinding::canFallDown(
		Tile* tile,
		int size)
{
	for (int
			x = 0;
			x != size;
			++x)
	{
		for (int
				y = 0;
				y != size;
				++y)
		{
			Position checkPos = tile->getPosition() + Position(x, y, 0);
			Tile* checkTile = _save->getTile(checkPos);
			if (!canFallDown(checkTile))
			{
				//Log(LOG_INFO) << "Pathfinding::canFallDown() ret FALSE";
				return false;
			}
		}
	}

	//Log(LOG_INFO) << "Pathfinding::canFallDown() ret TRUE";
	return true;
}

/**
 * Determines whether the unit is going up a stairs.
 * @param startPosition The position to start from.
 * @param endPosition The position we wanna reach.
 * @return True if the unit is going up a stairs.
 */
/*kL bool Pathfinding::isOnStairs(const Position& startPosition, const Position& endPosition)
{
	// condition 1 : endposition has to the south a terrainlevel -16 object (upper part of the stairs)
	if (_save->getTile(endPosition + Position(0, 1, 0))
		&& _save->getTile(endPosition + Position(0, 1, 0))->getTerrainLevel() == -16)
	{
		// condition 2 : one position further to the south there has to be a terrainlevel -8 object (lower part of the stairs)
		if (_save->getTile(endPosition + Position(0, 2, 0))
			&& _save->getTile(endPosition + Position(0, 2, 0))->getTerrainLevel() != -8)
		{
			return false;
		}

		// condition 3 : the start position has to be on either of the 3 tiles to the south of the endposition
		if (startPosition == endPosition + Position(0, 1, 0)
			|| startPosition == endPosition + Position(0, 2, 0)
			|| startPosition == endPosition + Position(0, 3, 0))
		{
			return true;
		}
	}

	// same for the east-west oriented stairs.
	if (_save->getTile(endPosition + Position(1, 0, 0))
		&& _save->getTile(endPosition + Position(1, 0, 0))->getTerrainLevel() == -16)
	{
		if (_save->getTile(endPosition + Position(2, 0, 0))
			&& _save->getTile(endPosition + Position(2, 0, 0))->getTerrainLevel() != -8)
		{
			return false;
		}

		if (startPosition == endPosition + Position(1, 0, 0)
			|| startPosition == endPosition + Position(2, 0, 0)
			|| startPosition == endPosition + Position(3, 0, 0))
		{
			return true;
		}
	}

	//TFTD stairs 1 : endposition has to the south a terrainlevel -18 object (upper part of the stairs)
	if (_save->getTile(endPosition + Position(0, 1, 0)) && _save->getTile(endPosition + Position(0, 1, 0))->getTerrainLevel() == -18)
	{
		// condition 2 : one position further to the south there has to be a terrainlevel -8 object (lower part of the stairs)
		if (_save->getTile(endPosition + Position(0, 2, 0)) && _save->getTile(endPosition + Position(0, 2, 0))->getTerrainLevel() != -12)
		{
			return false;
		}

		// condition 3 : the start position has to be on either of the 3 tiles to the south of the endposition
		if (startPosition == endPosition + Position(0, 1, 0) || startPosition == endPosition + Position(0, 2, 0) || startPosition == endPosition + Position(0, 3, 0))
		{
			return true;
		}
	}

	// same for the east-west oriented stairs.
	if (_save->getTile(endPosition + Position(1, 0, 0)) && _save->getTile(endPosition + Position(1, 0, 0))->getTerrainLevel() == -18)
	{
		if (_save->getTile(endPosition + Position(2, 0, 0)) && _save->getTile(endPosition + Position(2, 0, 0))->getTerrainLevel() != -12)
		{
			return false;
		}
		if (startPosition == endPosition + Position(1, 0, 0) || startPosition == endPosition + Position(2, 0, 0) || startPosition == endPosition + Position(3, 0, 0))
		{
			return true;
		}
	}
	return false;
} */

/**
 * Checks if vertical movement is valid.
 * Either there is a grav lift or the unit can fly, and there are no obstructions.
 * @param bu		- pointer to a BattleUnit
 * @param startPos	- start Position
 * @param dir		- up or down
 * @return,	-1 kneeling (stop unless on gravLift)
			 0 blocked (stop)
			 1 gravLift (go)
			 2 flying (go unless blocked)
 */
int Pathfinding::validateUpDown(
		BattleUnit* bu,
		Position startPos,
		int const dir)
{
	//Log(LOG_INFO) << "validateUpDown()";
	Position destPos;
	directionToVector(
					dir,
					&destPos);
	destPos += startPos;

	Tile* startTile = _save->getTile(startPos);
	Tile* destTile = _save->getTile(destPos);

	if (destTile == NULL)
		return 0;

	bool gravLift = startTile->getMapData(MapData::O_FLOOR)
					&& startTile->getMapData(MapData::O_FLOOR)->isGravLift()
					&& destTile->getMapData(MapData::O_FLOOR)
					&& destTile->getMapData(MapData::O_FLOOR)->isGravLift();

	if (gravLift)
		return 1;
	else if (bu->getMovementType() == MT_FLY)
	{
		if (dir == DIR_UP
			&& (SDL_GetModState() & KMOD_ALT) != 0) // for, BattlescapeState::btnUnitUpClick()
		{
			return -2;
		}

		Tile* belowStart = _save->getTile(startPos + Position(0, 0,-1));
		if ((dir == DIR_UP
				&& destTile->hasNoFloor(NULL)) // flying up only possible when there is no roof
			|| (dir == DIR_DOWN
				&& startTile->hasNoFloor(belowStart))) // flying down only possible when there is no floor
		{
			return 2; // flying.
		}
		else
			return 0; // blocked.
	}

	return -2; // no flying suit.
}

/**
 * kL. Sets pathfinding to check whether a unit is kneeled.
 * Used when vertical movement is initiated by BattlescapeState::btnUnitUpClick()
 * because otherwise the kneelCheck boolean has not been reset to false before
 * validateUpDown() is called, and it has to be false at that time. It of course
 * is set false in calculate(), but that's not called by Battlescape btns
 * till a tad later.
 */
/* void Pathfinding::setKneelCheck(bool checkKneel) // kL
{
	_kneelCheck = checkKneel;
} */

/**
 * Checks if going one step from start to destination in the
 * given direction requires going through a closed UFO door.
 * @param direction The direction of travel.
 * @param start The starting position of the travel.
 * @param destination Where the travel ends.
 * @return The TU cost of opening the door. 0 if no UFO door opened.
 */
/* int Pathfinding::getOpeningUfoDoorCost(int direction, Position start, Position destination)
{
	Tile* s = _save->getTile(start);
	Tile* d = _save->getTile(destination);

	switch (direction)
	{
		case 0:
			if (s->getMapData(MapData::O_NORTHWALL)
				&& s->getMapData(MapData::O_NORTHWALL)->isUFODoor()
				&& !s->isUfoDoorOpen(MapData::O_NORTHWALL))
			return s->getMapData(MapData::O_NORTHWALL)->getTUCost(_movementType);
		break;
		case 1:
			if (s->getMapData(MapData::O_NORTHWALL)
				&& s->getMapData(MapData::O_NORTHWALL)->isUFODoor()
				&& !s->isUfoDoorOpen(MapData::O_NORTHWALL))
			return s->getMapData(MapData::O_NORTHWALL)->getTUCost(_movementType);
			if (d->getMapData(MapData::O_WESTWALL)
				&& d->getMapData(MapData::O_WESTWALL)->isUFODoor()
				&& !d->isUfoDoorOpen(MapData::O_WESTWALL))
			return d->getMapData(MapData::O_WESTWALL)->getTUCost(_movementType);
		break;
		case 2:
			if (d->getMapData(MapData::O_WESTWALL)
				&& d->getMapData(MapData::O_WESTWALL)->isUFODoor()
				&& !d->isUfoDoorOpen(MapData::O_WESTWALL))
			return d->getMapData(MapData::O_WESTWALL)->getTUCost(_movementType);
		break;
		case 3:
			if (d->getMapData(MapData::O_NORTHWALL)
				&& d->getMapData(MapData::O_NORTHWALL)->isUFODoor()
				&& !d->isUfoDoorOpen(MapData::O_NORTHWALL))
			return d->getMapData(MapData::O_NORTHWALL)->getTUCost(_movementType);
			if (d->getMapData(MapData::O_WESTWALL)
				&& d->getMapData(MapData::O_WESTWALL)->isUFODoor()
				&& !d->isUfoDoorOpen(MapData::O_WESTWALL))
			return d->getMapData(MapData::O_WESTWALL)->getTUCost(_movementType);
		break;
		case 4:
			if (d->getMapData(MapData::O_NORTHWALL)
				&& d->getMapData(MapData::O_NORTHWALL)->isUFODoor()
				&& !d->isUfoDoorOpen(MapData::O_NORTHWALL))
			return d->getMapData(MapData::O_NORTHWALL)->getTUCost(_movementType);
		break;
		case 5:
			if (d->getMapData(MapData::O_NORTHWALL)
				&& d->getMapData(MapData::O_NORTHWALL)->isUFODoor()
				&& !d->isUfoDoorOpen(MapData::O_NORTHWALL))
			return d->getMapData(MapData::O_NORTHWALL)->getTUCost(_movementType);
			if (s->getMapData(MapData::O_WESTWALL)
				&& s->getMapData(MapData::O_WESTWALL)->isUFODoor()
				&& !s->isUfoDoorOpen(MapData::O_WESTWALL))
			return s->getMapData(MapData::O_WESTWALL)->getTUCost(_movementType);
		break;
		case 6:
			if (s->getMapData(MapData::O_WESTWALL)
				&& s->getMapData(MapData::O_WESTWALL)->isUFODoor()
				&& !s->isUfoDoorOpen(MapData::O_WESTWALL))
			return s->getMapData(MapData::O_WESTWALL)->getTUCost(_movementType);
		break;
		case 7:
			if (s->getMapData(MapData::O_NORTHWALL)
				&& s->getMapData(MapData::O_NORTHWALL)->isUFODoor()
				&& !s->isUfoDoorOpen(MapData::O_NORTHWALL))
			return s->getMapData(MapData::O_NORTHWALL)->getTUCost(_movementType);
			if (s->getMapData(MapData::O_WESTWALL)
				&& s->getMapData(MapData::O_WESTWALL)->isUFODoor()
				&& !s->isUfoDoorOpen(MapData::O_WESTWALL))
			return s->getMapData(MapData::O_WESTWALL)->getTUCost(_movementType);
		break;

		default:
			return 0;
	}

	return 0;
} */

/**
 * Marks tiles for the path preview.
 * @param bRemove - true removes preview
 * @return, true if a path is previewed
 */
bool Pathfinding::previewPath(bool bRemove)
{
	if (_path.empty())
		return false;

	if (bRemove == false
		&& _pathPreviewed)
	{
		return false;
	}

	_pathPreviewed = !bRemove;

	int
		curTU		= _unit->getTimeUnits(),
		usedTU		= 0, // only for soldiers reserving TUs
		tu			= 0, // cost per tile
		energy		= _unit->getEnergy(),
		energyLim	= energy,
		size		= _unit->getArmor()->getSize() - 1,
		dir			= -1,
		color		= 0;

	std::string armorType = _unit->getArmor()->getType();

	bool
		hathStood	= false,
		dash		= Options::strafe
						&& _modCTRL
						&& _unit->getGeoscapeSoldier() != NULL
//						&& size == 0
						&& (_strafeMove == false
							|| (_strafeMove == true
								&& _path.size() == 1
								&& _unit->getDirection() == _path.front())),
		bodySuit	= armorType == "STR_PERSONAL_ARMOR_UC",
		powerSuit	= _unit->hasPowerSuit()
						|| (_unit->hasFlightSuit()
							&& _movementType == MT_WALK),
		flightSuit	= _unit->hasFlightSuit()
						&& _movementType == MT_FLY,
		gravLift	= false,
		reserveOk	= false;

	//Log(LOG_INFO) << ". bodySuit = " << bodySuit;
	//Log(LOG_INFO) << ". powerSuit = " << powerSuit;
	//Log(LOG_INFO) << ". flightSuit = " << flightSuit;
// kL_note: Ought to create a factor for those in ruleArmor class & RuleSets ( _burden ).
// Or 'enum' those, as in
/* enum ArmorBurthen
{
	AB_LOW,		// -1
	AB_NORMAL,	//  0
	AB_MEDIUM,	//  1
	AB_HIGH		//  2
}; */

	Position
		start = _unit->getPosition(),
		dest;

	Tile* tile;

	bool switchBack = false;
	if (_save->getBattleGame()->getReservedAction() == BA_NONE)
	{
		switchBack = true;
		_save->getBattleGame()->setTUReserved(
											BA_SNAPSHOT);
	}

	for (std::vector<int>::reverse_iterator
			i = _path.rbegin();
			i != _path.rend();
			++i)
	{
		dir = *i;
		//Log(LOG_INFO) << "dir = " << dir;

		tu = getTUCost( // gets tu cost, but also gets the destination position.
					start,
					dir,
					&dest,
					_unit,
					0,
					false);
		//Log(LOG_INFO) << ". . tu[0] = " << tu;
/*		if (tu == 255)		// kL: Conflicts w/ the switchback for marker colors @ BA_SNAP.
		{
			abortPath();	// kL
			return false;	// kL
		} */

		energyLim = energy;

		gravLift = dir >= DIR_UP
					&& _save->getTile(start)->getMapData(MapData::O_FLOOR)
					&& _save->getTile(start)->getMapData(MapData::O_FLOOR)->isGravLift();
		if (gravLift == false)
		{
			if (hathStood == false
				&& _unit->isKneeled())
			{
				hathStood = true;

				curTU -= 8;
				usedTU += 8;
			}

			if (dash)
			{
				tu -= _openDoor;
				energy -= tu * 3 / 2;

				tu = (tu * 3 / 4) + _openDoor;
				//Log(LOG_INFO) << ". . tu [dash] = " << tu;
				//Log(LOG_INFO) << ". . energy [dash] = " << energy;
			}
			else
				energy -= tu;

			if (bodySuit)
				energy -= 1;
			else if (powerSuit)
				energy += 1;
			else if (flightSuit)
				energy += 2;
			//Log(LOG_INFO) << ". . energy left = " << energy;

			if (energy > energyLim)
			{
				//Log(LOG_INFO) << ". hit energyLim of " << energyLim;
				energy = energyLim;
			}
		}
		//Log(LOG_INFO) << ". . tu[1] = " << tu;

		curTU -= tu;
		//Log(LOG_INFO) << ". . curTU = " << curTU;

		usedTU += tu;
		reserveOk = _save->getBattleGame()->checkReservedTU(
														_unit,
														usedTU,
														true);
		start = dest;

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
				tile = _save->getTile(start + Position(x, y, 0));
				Tile* tileAbove = _save->getTile(start + Position(x, y, 1));

				if (bRemove == false)
				{
					if (i == _path.rend() - 1)
						tile->setPreview(10);
					else
					{
						int nextDir = *(i + 1);
						tile->setPreview(nextDir);
					}

					if ((x && y)
						|| size == 0)
					{
						tile->setTUMarker(curTU);
					}

					if (tileAbove // unit fell down, retroactively make the tileAbove's direction marker to DOWN
						&& tileAbove->getPreview() == 0
						&& tu == 0
						&& _movementType != MT_FLY)
					{
						tileAbove->setPreview(DIR_DOWN);
					}
				}
				else
				{
					tile->setPreview(-1);
					tile->setTUMarker(-1);
				}

				color = 0;
				if (bRemove == false)
				{
					if (curTU > -1
						&& energy > -1)
					{
						if (reserveOk)
							color = Pathfinding::green;
						else
							color = Pathfinding::yellow;
					}
					else
						color = Pathfinding::red;
				}

				tile->setMarkerColor(color);
			}
		}
	}

	if (switchBack)
		_save->getBattleGame()->setTUReserved(
											BA_NONE);

	//Log(LOG_INFO) << ". . tu @ return = " << tu;
	return true;
}

/**
 * Unmarks the tiles used for the path preview.
 * @return, true if the previewed path was removed
 */
bool Pathfinding::removePreview()
{
	if (_pathPreviewed == false)
		return false;

	previewPath(true);

	return true;
}

/**
 * Gets the path preview setting.
 * @return, true if paths are previewed
 */
bool Pathfinding::isPathPreviewed() const
{
	return _pathPreviewed;
}

/**
 * Locates all tiles reachable to @a *unit with a TU cost no more than @a tuMax.
 * Uses Dijkstra's algorithm.
 * @param unit	- pointer to a BattleUnit
 * @param tuMax	- the maximum cost of the path to each tile
 * @return, an array of reachable tiles, sorted in ascending order of cost; the first tile is the start location
 */
std::vector<int> Pathfinding::findReachable(
		BattleUnit* unit,
		int tuMax)
{
	//Log(LOG_INFO) << "Pathfinding::findReachable()";
	const Position& start = unit->getPosition();
	int energyMax = unit->getEnergy();

	for (std::vector<PathfindingNode>::iterator
			it = _nodes.begin();
			it != _nodes.end();
			++it)
	{
		it->reset();
	}

	PathfindingNode* startNode = getNode(start);
	startNode->connect(0, 0, 0);
	PathfindingOpenSet unvisited;
	unvisited.push(startNode);
	std::vector<PathfindingNode*> reachable;

	while (unvisited.empty() == false)
	{
		PathfindingNode* currentNode = unvisited.pop();
		Position const &currentPos = currentNode->getPosition();

		for (int // Try all reachable neighbours.
				direction = 0;
				direction < 10;
				direction++)
		{
			Position nextPos;

			int tuCost = getTUCost(
								currentPos,
								direction,
								&nextPos,
								unit,
								0,
								false);
			if (tuCost == 255) // Skip unreachable / blocked
				continue;

			if (currentNode->getTUCost(false) + tuCost > tuMax
//kL			|| (currentNode->getTUCost(false) + tuCost) / 2 > energyMax) // Run out of TUs/Energy
				|| (currentNode->getTUCost(false) + tuCost) > energyMax) // kL
			{
				continue;
			}

			PathfindingNode* nextNode = getNode(nextPos);
			if (nextNode->isChecked()) // Our algorithm means this node is already at minimum cost.
				continue;

			int totalTuCost = currentNode->getTUCost(false) + tuCost;
			if (nextNode->inOpenSet() == false
				|| nextNode->getTUCost(false) > totalTuCost) // If this node is unvisited or visited from a better path.
			{
				nextNode->connect(
								totalTuCost,
								currentNode,
								direction);

				unvisited.push(nextNode);
			}
		}

		currentNode->setChecked();
		reachable.push_back(currentNode);
	}

	std::sort(
			reachable.begin(),
			reachable.end(),
			MinNodeCosts());

	std::vector<int> tiles;
	tiles.reserve(reachable.size());

	for (std::vector<PathfindingNode*>::const_iterator
			it = reachable.begin();
			it != reachable.end();
			++it)
	{
		tiles.push_back(_save->getTileIndex((*it)->getPosition()));
	}

	return tiles;
}

/**
 * Gets the strafe move setting.
 * @return, true if strafe move
 */
bool Pathfinding::getStrafeMove() const
{
	return _strafeMove;
}

/**
 * Sets _unit in order to abuse low-level pathfinding functions from outside the class.
 * @param unit - unit taking the path
 */
void Pathfinding::setUnit(BattleUnit* unit)
{
	_unit = unit;

	if (unit != NULL)
	{
		_movementType = unit->getMovementType();

/*		if (_movementType == MT_FLY
			&& (SDL_GetModState() & KMOD_ALT) != 0)
		{
			_movementType = MT_WALK;	// kL. I put this in but not sure where it gets used (if..).
										// SavedBattleGame::reviveUnconsciousUnits() ...
		} */
	}
	else
		_movementType = MT_WALK;
}

/**
 * Checks whether a modifier key was used to enable strafing or running.
 * @return, true if modifier was used
 */
bool Pathfinding::isModCTRL() const
{
	return _modCTRL;
}

/**
 * Checks whether a modifier key was used to enable forced walking.
 * @return, true if modifier was used
 */
bool Pathfinding::isModALT() const
{
	return _modALT;
}

/**
 * kL. Gets the current movementType.
 * @return, the currently selected unit's movementType
 */
MovementType Pathfinding::getMovementType() const // kL
{
	return _movementType;
}

/**
 * kL. Gets TU cost for opening a door. Used to conform TU costs in UnitWalkBState.
 * @return, TU cost for opening a specific door
 */
int Pathfinding::getOpenDoor() const // kL
{
	return _openDoor;
}

/**
 * Gets a reference to the current path.
 * @return, the actual path
 */
const std::vector<int>& Pathfinding::getPath()
{
	return _path;
}

/**
 * Makes a copy of the current path.
 * @return, a copy of the path
 */
std::vector<int> Pathfinding::copyPath() const
{
	return _path;
}

}
