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
#include "../Engine/Options.h"

#include "../Ruleset/Armor.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

/**
 * Sets up a Pathfinding.
 * @param save pointer to SavedBattleGame object.
 */
Pathfinding::Pathfinding(SavedBattleGame* save)
	:
		_save(save),
		_nodes(),
		_unit(0),
		_pathPreviewed(false),
		_strafeMove(false),
		_totalTUCost(0),
		_modifierUsed(false),
		_movementType(MT_WALK)
{
	_size = _save->getMapSizeXYZ();
	_nodes.reserve(_size); // initialize one node per tile.

	Position p;
	for (int
			i = 0;
			i < _size;
			++i)
	{
		_save->getTileCoords(
							i,
							&p.x,
							&p.y,
							&p.z);
		_nodes.push_back(PathfindingNode(p));
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
 * @param pos Position.
 * @return Pointer to node.
 */
PathfindingNode* Pathfinding::getNode(const Position& pos)
{
	return &_nodes[_save->getTileIndex(pos)];
}

/**
 * Calculates the shortest path; tries bresenham then A* paths.
 * @param unit, Unit taking the path.
 * @param endPosition, The position we want to reach.
 * @param target, Target of the path.
 * @param maxTUCost, Maximum time units the path can cost.
 */
void Pathfinding::calculate(
		BattleUnit* unit,
		Position endPosition,
		BattleUnit* target,
		int maxTUCost)
{
	//Log(LOG_INFO) << "Pathfinding::calculate()";
	_totalTUCost = 0;
	_path.clear();

	_unit = unit;

	// i'm DONE with these out of bounds errors.
	// kL_note: I really don't care what you're "DONE" with.....
	if (endPosition.x > _save->getMapSizeX() - _unit->getArmor()->getSize()
		|| endPosition.y > _save->getMapSizeY() - _unit->getArmor()->getSize()
		|| endPosition.x < 0
		|| endPosition.y < 0)
	{
		return;
	}

	// check if destination is not blocked
	Tile* destTile = _save->getTile(endPosition);
	if (isBlocked(destTile, MapData::O_FLOOR, target)
		|| isBlocked(destTile, MapData::O_OBJECT, target))
	{
		return;
	}


	if (target != 0
		&& maxTUCost == -1) // pathfinding for missile
	{
		maxTUCost = 10000;
		_movementType = MT_FLY;
	}
	else
		_movementType = _unit->getArmor()->getMovementType();


	// The following check avoids that the unit walks behind the stairs
	// if we click behind the stairs to make it go up the stairs.
	// It only works if the unit is on one of the 2 tiles on the
	// stairs, or on the tile right in front of the stairs.
	// kL_note: I don't want this: ( the function, below, can be removed ).
/*kL	if (isOnStairs(startPosition, endPosition))
	{
		endPosition.z++;
		destTile = _save->getTile(endPosition);
	} */

	while (destTile->getTerrainLevel() == -24
		&& endPosition.z != _save->getMapSizeZ())
	{
		endPosition.z++;
		destTile = _save->getTile(endPosition);
	}

	// check if we have a floor, else lower destination
	if (_movementType != MT_FLY)
	{
		while (canFallDown( // for large & small units.
						destTile,
						_unit->getArmor()->getSize()))
		{
			endPosition.z--;
			destTile = _save->getTile(endPosition);
			//Log(LOG_INFO) << ". canFallDown() -1 level, endPosition = " << endPosition;
		}
	}

	int size = _unit->getArmor()->getSize() - 1;
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
					Tile* checkTile = _save->getTile(endPosition + Position(x, y, 0));
					if (x && y
						&& ((checkTile->getMapData(MapData::O_NORTHWALL)
								&& checkTile->getMapData(MapData::O_NORTHWALL)->isDoor())
							||  (checkTile->getMapData(MapData::O_WESTWALL)
								&& checkTile->getMapData(MapData::O_WESTWALL)->isDoor())))
					{
						return;
					}
					else if (isBlocked(
									destTile,
									checkTile,
									dir[i],
									_unit)
						&& isBlocked(
									destTile,
									checkTile,
									dir[i],
									target))
					{
						return;
					}
					else if (checkTile->getUnit())
					{
						BattleUnit* checkUnit = checkTile->getUnit();
						if (checkUnit != _unit
							&& checkUnit != target
							&& checkUnit->getVisible())
						{
							return;
						}
					}

					++i;
				}
			}
		}
	}

	/* kL_begin_TEST:
		if (_save->getStrafeSetting()) Log(LOG_INFO)					<< "strafe Option true";
		if ((SDL_GetModState()&KMOD_CTRL) != 0) Log(LOG_INFO)			<< "strafe Ctrl true";
		if (startPosition.z == endPosition.z) Log(LOG_INFO)				<< "strafe z-level true";
		if (abs(startPosition.x - endPosition.x) <= 1) Log(LOG_INFO)	<< "strafe Pos.x true";
		if (abs(startPosition.y - endPosition.y) <= 1) Log(LOG_INFO)	<< "strafe Pos.y true";
	*/ // kL_end_TEST.

	// Strafing move allowed only to adjacent squares on same z;
	// "same z" rule mainly to simplify walking render.
	Position startPosition = _unit->getPosition();
	_strafeMove = Options::strafe
				&& (SDL_GetModState() & KMOD_CTRL) != 0
				&& startPosition.z == endPosition.z
				&& abs(startPosition.x - endPosition.x) < 2
				&& abs(startPosition.y - endPosition.y) < 2;
//	if (_strafeMove) Log(LOG_INFO) << "Pathfinding::calculate() _strafeMove VALID";
//	else Log(LOG_INFO) << "Pathfinding::calculate() _strafeMove INVALID";

	// look for a possible fast and accurate bresenham path and skip A*
	bool sneak = _unit->getFaction() == FACTION_HOSTILE
				&& Options::sneakyAI;

	if (startPosition.z == endPosition.z
		&& bresenhamPath(
					startPosition,
					endPosition,
					target,
					sneak))
	{
		std::reverse( // paths are stored in reverse order
				_path.begin(),
				_path.end());

		return;
	}
	else
	{
		// if bresenham failed, we shouldn't keep the path
		// it was attempting, in case A* fails too.
		abortPath();
	}

	if (!aStarPath( // now try through A*
				startPosition,
				endPosition,
				target,
				sneak,
				maxTUCost))
	{
		abortPath();
	}
}

/**
 * Calculates the shortest path using Brensenham path algorithm.
 * @note This only works in the X/Y plane.
 * @param origin The position to start from.
 * @param target The position we want to reach.
 * @param targetUnit Target of the path.
 * @param sneak Is the unit sneaking?
 * @param maxTUCost Maximum time units the path can cost.
 * @return True if a path exists, false otherwise.
 */
bool Pathfinding::bresenhamPath(
		const Position& origin,
		const Position& target,
		BattleUnit* targetUnit,
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
								targetUnit,
								(targetUnit && maxTUCost == 10000));

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
					|| (!isDiagonal
						&& tuCostDiagonal == lastTUCost)
					|| lastTUCost == -1)
				&& !isBlocked(
						_save->getTile(lastPoint),
						_save->getTile(nextPoint),
						dir,
						targetUnit))
			{
				_path.push_back(dir);
			}
			else
				return false;

			if (targetUnit == 0
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
 * @param startPosition, The position to start from.
 * @param endPosition, The position we want to reach.
 * @param target, Target of the path.
 * @param sneak, Is the unit sneaking?
 * @param maxTUCost, Maximum time units the path can cost.
 * @return, True if a path exists, false otherwise.
 */
bool Pathfinding::aStarPath(
		const Position& startPosition,
		const Position& endPosition,
		BattleUnit* target,
		bool sneak,
		int maxTUCost)
{
	//Log(LOG_INFO) << "Pathfinding::aStarPath()";

	// reset every node, so we have to check them all
	for (std::vector<PathfindingNode>::iterator
			i = _nodes.begin();
			i != _nodes.end();
			++i)
	{
		i->reset();
	}

	// start position is the first one in our "open" list
	PathfindingNode* start = getNode(startPosition);
	start->connect(0, 0, 0, endPosition);
	PathfindingOpenSet openList;
	openList.push(start);

	bool missile = target
				&& maxTUCost == -1;

	// if the openList is empty, we've reached the end
	while (!openList.empty())
	{
		PathfindingNode* currentNode = openList.pop();
		Position const &currentPos = currentNode->getPosition();
		currentNode->setChecked();

		if (currentPos == endPosition) // We found our target.
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

		for (int // Try all reachable neighbours.
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
								target,
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
			if (nextNode->isChecked()) // Our algorithm means this node is already at minimum cost.
				//Log(LOG_INFO) << ". node already Checked ... cont.";
				continue;

			_totalTUCost = currentNode->getTUCost(missile) + tuCost;

			// If this node is unvisited or has only been visited from inferior paths...
			if ((!nextNode->inOpenSet()
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
 * @param startPosition, The position to start from.
 * @param direction, The direction we are facing.
 * @param endPosition, The position we want to reach.
 * @param unit, The unit moving.
 * @param target, The target unit.
 * @param missile, Is this a guided missile?
 * @return, TU cost or 255 if movement is impossible.
 */
int Pathfinding::getTUCost(
		const Position& startPosition,
		int direction,
		Position* endPosition,
		BattleUnit* unit,
		BattleUnit* target,
		bool missile)
{
	//Log(LOG_INFO) << "Pathfinding::getTUCost()";
	_unit = unit;

	directionToVector(
				direction,
				endPosition);
	*endPosition += startPosition;

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

			Tile* startTile			= _save->getTile(startPosition + offset);
			Tile* destTile			= _save->getTile(*endPosition + offset);
			Tile* belowDestination	= _save->getTile(*endPosition + offset + Position(0, 0,-1));
			Tile* aboveDestination	= _save->getTile(*endPosition + offset + Position(0, 0, 1));

			// this means the destination is probably outside the map
			if (startTile == 0
				|| destTile == 0)
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

			if (direction < DIR_UP
				&& startTile->getTerrainLevel() > -16)
			{
				// check if we can go this way
				if (isBlocked(
							startTile,
							destTile,
							direction,
							target))
				{
					return 255;
				}

				if (startTile->getTerrainLevel() - destTile->getTerrainLevel() > 8)
					return 255;
			}

			// this will later be used to re-init the start tile again.
			Position verticalOffset(0, 0, 0);

			// if we are on a stairs try to go up a level
			if (direction < DIR_UP
				&& startTile->getTerrainLevel() < -15
				&& !aboveDestination->hasNoFloor(destTile)
				&& !triedStairs)
			{
				partsGoingUp++;
				if (partsGoingUp > size)	// kL_note: seems that partsGoingUp can never
				{							// be greater than size... if large unit.
					verticalOffset.z++;
					endPosition->z++;
					destTile = _save->getTile(*endPosition + offset);
					belowDestination = _save->getTile(*endPosition + Position(x, y,-1));

					triedStairs = true;
				}
			}
			else if (!fellDown
				&& _movementType != MT_FLY
				&& belowDestination
				&& canFallDown(destTile)
				&& belowDestination->getTerrainLevel() < -11)	// kL_note: why not fall more than half tile.Z
																// Because then you're doing stairs or ramp.
			{
				partsGoingDown++;
				if (partsGoingDown == (size + 1) * (size + 1))
				{
					endPosition->z--;
					destTile = _save->getTile(*endPosition + offset);
					belowDestination = _save->getTile(*endPosition + Position(x, y,-1));

					fellDown = true;
				}
			}
			else if (_movementType == MT_FLY
				&& belowDestination
				&& belowDestination->getUnit()
				&& belowDestination->getUnit() != unit)
			{
				// 2 or more voxels poking into this tile = no go
				if (belowDestination->getUnit()->getHeight()
						+ belowDestination->getUnit()->getFloatHeight()
						- belowDestination->getTerrainLevel() > 26)
				{
					return 255;
				}
			}

			// this means the destination is probably outside the map
			if (!destTile)
				return 255;

			if (direction < DIR_UP
				&& endPosition->z == startTile->getPosition().z)
			{
				// check if we can go this way
				if (isBlocked(
							startTile,
							destTile,
							direction,
							target))
				{
					return 255;
				}

				if (startTile->getTerrainLevel() - destTile->getTerrainLevel() > 8)
					return 255;
			}
			else if (direction >= DIR_UP)
			{
				// check if we can go up or down through gravlift or fly
				if (validateUpDown(
								unit,
								startPosition + offset,
								direction)
							> 0)
				{
					cost = 8; // vertical movement by flying suit or grav lift
				}
				else
					return 255;
			}

			// check if we have floor, else fall down
			if (_movementType != MT_FLY
				&& !fellDown
				&& canFallDown(startTile))
			{
				partsFalling++;
				if (partsFalling == (size + 1) * (size + 1))
				{
					*endPosition = startPosition + Position(0, 0,-1);
					destTile = _save->getTile(*endPosition + offset);
					belowDestination = _save->getTile(*endPosition + Position(x, y,-1));
					fellDown = true;
					direction = DIR_DOWN;
				}
			}

			startTile = _save->getTile(startTile->getPosition() + verticalOffset);

			if (direction < DIR_UP
				&& partsGoingUp != 0)
			{
				// check if we can go this way
				if (isBlocked(
							startTile,
							destTile,
							direction,
							target))
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
						target)
				|| isBlocked(
							destTile,
							MapData::O_OBJECT,
							target))
			{
				return 255;
			}


			if (direction < DIR_UP)
			{
				cost += destTile->getTUCost(MapData::O_FLOOR, _movementType);

				if (!fellDown
					&& !triedStairs
					&& destTile->getMapData(MapData::O_OBJECT))
				{
					cost += destTile->getTUCost(MapData::O_OBJECT, _movementType);
				}

				// climbing up a level costs one extra
				if (verticalOffset.z > 0)
					cost++;

				// if we don't want to fall down and there is no floor,
				// we can't know the TUs so it defaults to 4
				if (!fellDown
					&& destTile->hasNoFloor(0))
				{
					cost = 4;
				}

				int wallcost = 0;	// walking through rubble walls --
									// but don't charge for walking diagonally through doors (which is impossible);
									// they're a special case unto themselves, if we can walk past them diagonally,
									// it means we can go around since there is no wall blocking us.
				int sides = 0;		// how many walls we cross when moving diagonally
				int wallTU = 0;		// used to check if there's a wall that costs +TU.

				if (direction == 0
					|| direction == 7
					|| direction == 1)
				{
					wallTU = startTile->getTUCost(MapData::O_NORTHWALL, _movementType);
					if (wallTU > 0)
					{
//						if (direction &1) // would use this to increase diagonal wall-crossing by +50%
//						{
//							wallTU += wallTU / 2;
//						}

						wallcost += wallTU;
						sides ++;
					}
				}

				if (direction == 2
					|| direction == 1
					|| direction == 3)
				{
					if (startTile->getPosition().z == destTile->getPosition().z) // don't count wallcost if it's on the floor below.
					{
						wallTU = destTile->getTUCost(MapData::O_WESTWALL, _movementType);
						if (wallTU > 0)
						{
							wallcost += wallTU;
							sides ++;
						}
					}
				}

				if (direction == 4
					|| direction == 3
					|| direction == 5)
				{
					if (startTile->getPosition().z == destTile->getPosition().z) // don't count wallcost if it's on the floor below.
					{
						wallTU = destTile->getTUCost(MapData::O_NORTHWALL, _movementType);
						if (wallTU > 0)
						{
							wallcost += wallTU;
							sides ++;
						}
					}
				}

				if (direction == 6
					|| direction == 5
					|| direction == 7)
				{
					wallTU = startTile->getTUCost(MapData::O_WESTWALL, _movementType);
					if (wallTU > 0)
					{
						wallcost += wallTU;
						sides ++;
					}
				}

				// diagonal walking (uneven directions) costs 50% more tu's
				// kL_note: this is moved up so that objects don't cost +150% tu;
				// instead, let them keep a flat +100% to step onto
				// -- note that Walls also do not take +150 tu to step over diagonally....
				if (direction & 1)
				{
					cost = static_cast<int>(static_cast<float>(cost) * 1.5f);

					if (sides == 2)
						wallcost /= 2; // average of the wall-sides crossed

					if (wallcost)
						wallcost += 1; // kL. <- arbitrary inflation.
				}

				cost += wallcost;
			}


			if (_unit->getFaction() == FACTION_HOSTILE
				&& destTile->getFire() > 0)
			{
				cost += 32; // try to find a better path, but don't exclude this path entirely.
			}

			// Strafing costs +1 for forwards-ish or sidewards, propose +2 for backwards-ish directions
			// Maybe if flying then it makes no difference?
			if (Options::strafe
				&& _strafeMove)
			{
				if (size)
					// 4-tile units not supported, Turn off strafe move and continue
					_strafeMove = false;
				// kL_begin:
				else if (_unit->getDirection() != direction)
				{
					int delta = std::min(
										abs(8 + direction - _unit->getDirection()),
										std::min(
												abs(_unit->getDirection() - direction),
												abs(8 + _unit->getDirection() - direction)));
					if (delta == 4)
						delta = 2;

					cost += delta;
				} // kL_end.
			}

			totalCost += cost;
			cost = 0;
		}
	}

	// for bigger sized units, check the path between part 1,1 and part 0,0 at end position
	if (size)
	{
		double fTCost = ceil(static_cast<double>(totalCost) / static_cast<double>((size + 1) * (size + 1))); // kL
		totalCost = static_cast<int>(fTCost); // kL: round those tanks up!

		Tile* startTile = _save->getTile(*endPosition + Position(1, 1, 0));
		Tile* destTile = _save->getTile(*endPosition);

		int tmpDirection = 7;
		if (isBlocked(
					startTile,
					destTile,
					tmpDirection,
					target))
		{
			return 255;
		}
		else if (!fellDown
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
		//Log(LOG_INFO) << "Pathfinding::getTUCost() ret = " << totalCost;
		return totalCost;
}

/**
 * Converts direction to a vector. Direction starts north = 0 and goes clockwise.
 * @param direction, Source direction
 * @param vector, Pointer to a position (which acts as a vector)
 */
void Pathfinding::directionToVector(
		int const direction,
		Position* vector)
{
	int x[10] = { 0, 1, 1, 1, 0,-1,-1,-1, 0, 0};
	int y[10] = {-1,-1, 0, 1, 1, 1, 0,-1, 0, 0};
	int z[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 1,-1};

	vector->x = x[direction];
	vector->y = y[direction];
	vector->z = z[direction];
}

/**
 * Converts direction to a vector. Direction starts north = 0 and goes clockwise.
 * @param vector, Reference to a position (which acts as a vector)
 * @param dir, Reference to the resulting direction (up/down & same-tile returns -1)
 */
void Pathfinding::vectorToDirection(
		const Position& vector,
		int& dir)
{
	dir = -1;

	int x[8] = { 0, 1, 1, 1, 0,-1,-1,-1};
	int y[8] = {-1,-1, 0, 1, 1, 1, 0,-1};

	for (int
			i = 0;
			i < 8;
			++i)
	{
		if (   x[i] == vector.x
			&& y[i] == vector.y)
		{
			dir = i;

			return;
		}
	}
}

/**
 * Checks whether a path is ready and gives the first direction.
 * @return, Direction where the unit needs to go next, -1 if it's the end of the path.
 */
int Pathfinding::getStartDirection()
{
	if (_path.empty())
		return -1;

	return _path.back();
}

/**
 * Dequeues the next path direction. Ie, returns the direction and removes it from queue.
 * @return, Direction where the unit needs to go next, -1 if it's the end of the path.
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
	_totalTUCost = 0;
	_path.clear();
}

/**
 * Determines whether a certain part of a tile blocks movement.
 * @param tile, Specified tile, can be a null pointer.
 * @param part, Part of the tile.
 * @param missileTarget, Target for a missile.
 * @return, True if the movement is blocked.
 */
// private
bool Pathfinding::isBlocked(
		Tile* tile,
		const int part,
		BattleUnit* missileTarget,
		int bigWallExclusion)
{
	//Log(LOG_INFO) << "Pathfinding::isBlocked() #2";
	if (tile == 0) // probably outside the map here
		return true;

	if (part == O_BIGWALL)
	{
		//Log(LOG_INFO) << ". part is Bigwall";
		if (tile->getMapData(MapData::O_OBJECT)
			&& tile->getMapData(MapData::O_OBJECT)->getBigWall() != 0
			&& tile->getMapData(MapData::O_OBJECT)->getBigWall() <= BIGWALL_NWSE
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
			&& tile->getMapData(MapData::O_OBJECT)->getBigWall() == BIGWALL_WEST)
		{
			return true; // blocking part
		}

		Tile* tileWest = _save->getTile(tile->getPosition() + Position(-1, 0, 0));
		if (!tileWest)
			return true; // do not look outside of map

		if (tileWest->getMapData(MapData::O_OBJECT)
			&& (tileWest->getMapData(MapData::O_OBJECT)->getBigWall() == BIGWALL_EAST
				|| tileWest->getMapData(MapData::O_OBJECT)->getBigWall() == BIGWALL_E_S))
		{
			return true; // blocking part
		}
	}

	if (part == MapData::O_NORTHWALL)
	{
		//Log(LOG_INFO) << ". part is Northwall";
		if (tile->getMapData(MapData::O_OBJECT)
			&& tile->getMapData(MapData::O_OBJECT)->getBigWall() == BIGWALL_NORTH)
		{
			return true; // blocking part
		}

		Tile* tileNorth = _save->getTile(tile->getPosition() + Position(0,-1, 0));
		if (!tileNorth)
			return true; // do not look outside of map

		if (tileNorth->getMapData(MapData::O_OBJECT)
			&& (tileNorth->getMapData(MapData::O_OBJECT)->getBigWall() == BIGWALL_SOUTH
				|| tileNorth->getMapData(MapData::O_OBJECT)->getBigWall() == BIGWALL_E_S))
		{
			return true; // blocking part
		}
	}

	if (part == MapData::O_FLOOR)
	{
		//Log(LOG_INFO) << ". part is Floor";
		if (tile->getUnit())
		{
			BattleUnit* unit = tile->getUnit();

			if (unit == _unit
				|| unit == missileTarget
				|| unit->isOut())
			{
				return false;
			}

			if (_unit
				&& _unit->getFaction() == FACTION_PLAYER
				&& unit->getVisible())
			{
				return true; // player know all visible units
			}

			if (_unit
				&& _unit->getFaction() == unit->getFaction())
			{
				return true;
			}

			if (_unit
				&& _unit->getFaction() == FACTION_HOSTILE
				&& std::find(
						_unit->getUnitsSpottedThisTurn().begin(),
						_unit->getUnitsSpottedThisTurn().end(),
						unit)
					!= _unit->getUnitsSpottedThisTurn().end())
			{
				return true;
			}
		}
		else if (tile->hasNoFloor(0) // this whole section is devoted to making large units not take part in any kind of falling behaviour
			&& _movementType != MT_FLY)
		{
			Position pos = tile->getPosition();
			while (pos.z >= 0)
			{
				Tile* t = _save->getTile(pos);
				if (t->getUnit()
					&& t->getUnit() != _unit)
				{
					BattleUnit* unit = t->getUnit();

					// don't let large units fall on other units
					if (_unit
						&& _unit->getArmor()->getSize() > 1)
					{
						return true;
					}

					// don't let any units fall on large units
					if (//kL unit != _unit &&
						unit != missileTarget
						&& !unit->isOut()
						&& unit->getArmor()->getSize() > 1)
					{
						return true;
					}
				}

				// not gonna fall any further, so we can stop checking.
				if (!t->hasNoFloor(0))
					break;

				pos.z--;
			}
		}
	}

	// missiles can't pathfind through closed doors.
	if (missileTarget != 0
		&& tile->getMapData(part)
		&& (tile->getMapData(part)->isDoor()
			|| (tile->getMapData(part)->isUFODoor()
				&& !tile->isUfoDoorOpen(part))))
	{
		return true;
	}

	if (tile->getTUCost( // blocking part
					part,
					_movementType)
				== 255)
	{
		return true;
	}

	return false;
}

/**
 * Determines whether going from one tile to another blocks movement.
 * @param startTile, The tile to start from.
 * @param endTile, The tile we want to reach.
 * @param direction, The direction we are facing.
 * @param missileTarget, Target for a missile.
 * @return, True if the movement is blocked.
 */
// public:
bool Pathfinding::isBlocked(
		Tile* startTile,
		Tile* /* endTile */,
		const int direction,
		BattleUnit* missileTarget)
{
	//Log(LOG_INFO) << "Pathfinding::isBlocked() #1";

	// check if the difference in height between start and destination is not too high
	// so we can not jump to the highest part of the stairs from the floor
	// stairs terrainlevel goes typically -8 -16 (2 steps) or -4 -12 -20 (3 steps)
	// this "maximum jump height" is therefore set to 8

	const Position currentPosition = startTile->getPosition();

	static const Position oneTileNorth	= Position( 0,-1, 0);
	static const Position oneTileEast	= Position( 1, 0, 0);
	static const Position oneTileSouth	= Position( 0, 1, 0);
	static const Position oneTileWest	= Position(-1, 0, 0);

	// kL_begin:
	switch (direction)
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
						_save->getTile(currentPosition + oneTileEast),
						MapData::O_WESTWALL,
						missileTarget)
				|| isBlocked(
						_save->getTile(currentPosition + oneTileEast),
						MapData::O_NORTHWALL,
						missileTarget)
				|| isBlocked(
						_save->getTile(currentPosition + oneTileEast),
						O_BIGWALL,
						missileTarget,
						BIGWALL_NESW)
				|| isBlocked(
						_save->getTile(currentPosition + oneTileEast + oneTileNorth),
						MapData::O_WESTWALL,
						missileTarget)
				|| isBlocked(
						_save->getTile(currentPosition + oneTileNorth),
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
						_save->getTile(currentPosition + oneTileEast),
						MapData::O_WESTWALL,
						missileTarget))
			{
				return true;
			}
		break;
		case 3: // south-east
			//Log(LOG_INFO) << ". try SouthEast";
			if (isBlocked(
						_save->getTile(currentPosition + oneTileEast),
						MapData::O_WESTWALL,
						missileTarget)
				|| isBlocked(
						_save->getTile(currentPosition + oneTileEast),
						O_BIGWALL,
						missileTarget,
						BIGWALL_NWSE)
				|| isBlocked(
						_save->getTile(currentPosition + oneTileEast + oneTileSouth),
						MapData::O_NORTHWALL,
						missileTarget)
				|| isBlocked(
						_save->getTile(currentPosition + oneTileEast + oneTileSouth),
						MapData::O_WESTWALL,
						missileTarget)
				|| isBlocked(
						_save->getTile(currentPosition + oneTileSouth),
						MapData::O_NORTHWALL,
						missileTarget)
				|| isBlocked(
						_save->getTile(currentPosition + oneTileSouth),
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
						_save->getTile(currentPosition + oneTileSouth),
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
						_save->getTile(currentPosition + oneTileSouth),
						MapData::O_WESTWALL,
						missileTarget)
				|| isBlocked(
						_save->getTile(currentPosition + oneTileSouth),
						MapData::O_NORTHWALL,
						missileTarget)
				|| isBlocked(
						_save->getTile(currentPosition + oneTileSouth),
						O_BIGWALL,
						missileTarget,
						BIGWALL_NESW)
				|| isBlocked(
						_save->getTile(currentPosition + oneTileSouth + oneTileWest),
						MapData::O_NORTHWALL,
						missileTarget)
				|| isBlocked(
						_save->getTile(currentPosition + oneTileWest),
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
						_save->getTile(currentPosition + oneTileWest),
						MapData::O_NORTHWALL,
						missileTarget)
				|| isBlocked(
						_save->getTile(currentPosition + oneTileWest),
						O_BIGWALL,
						missileTarget,
						BIGWALL_NWSE)
				|| isBlocked(
						_save->getTile(currentPosition + oneTileNorth),
						MapData::O_WESTWALL,
						missileTarget)
				|| isBlocked(
						_save->getTile(currentPosition + oneTileNorth),
						O_BIGWALL,
						missileTarget,
						BIGWALL_NWSE))
			{
				return true;
			}
		break;
	} // kL_end.
/*kL	switch (direction)
	{
		case 0:	// north
			//Log(LOG_INFO) << ". try North";
			if (isBlocked(startTile, MapData::O_NORTHWALL, missileTarget))
				return true;
		break;
		case 1: // north-east
			//Log(LOG_INFO) << ". try NorthEast";
			if (isBlocked(startTile, MapData::O_NORTHWALL, missileTarget))
				return true;
			if (isBlocked(_save->getTile(currentPosition + oneTileNorth + oneTileEast), MapData::O_WESTWALL, missileTarget))
				return true;
			if (isBlocked(_save->getTile(currentPosition + oneTileEast), MapData::O_WESTWALL, missileTarget))
				return true;
			if (isBlocked(_save->getTile(currentPosition + oneTileEast), MapData::O_NORTHWALL, missileTarget))
				return true;
			if (isBlocked(_save->getTile(currentPosition + oneTileEast), O_BIGWALL, missileTarget, BIGWALL_NESW))
				return true;
			if (isBlocked(_save->getTile(currentPosition + oneTileNorth), O_BIGWALL, missileTarget, BIGWALL_NESW))
				return true;
		break;
		case 2: // east
			//Log(LOG_INFO) << ". try East";
			if (isBlocked(_save->getTile(currentPosition + oneTileEast), MapData::O_WESTWALL, missileTarget))
				//Log(LOG_INFO) << "Pathfinding::isBlocked -> east is Blocked";
				return true;
		break;
		case 3: // south-east
			//Log(LOG_INFO) << ". try SouthEast";
			if (isBlocked(_save->getTile(currentPosition + oneTileEast), MapData::O_WESTWALL, missileTarget))
				return true;
			if (isBlocked(_save->getTile(currentPosition + oneTileSouth), MapData::O_NORTHWALL, missileTarget))
				return true;
			if (isBlocked(_save->getTile(currentPosition + oneTileSouth + oneTileEast), MapData::O_NORTHWALL, missileTarget))
				return true;
			if (isBlocked(_save->getTile(currentPosition + oneTileSouth + oneTileEast), MapData::O_WESTWALL, missileTarget))
				return true;
			if (isBlocked(_save->getTile(currentPosition + oneTileEast), O_BIGWALL, missileTarget, BIGWALL_NWSE))
				return true;
			if (isBlocked(_save->getTile(currentPosition + oneTileSouth), O_BIGWALL, missileTarget, BIGWALL_NWSE))
				return true;
		break;
		case 4: // south
			//Log(LOG_INFO) << ". try South";
			if (isBlocked(_save->getTile(currentPosition + oneTileSouth), MapData::O_NORTHWALL, missileTarget))
				return true;
		break;
		case 5: // south-west
			//Log(LOG_INFO) << ". try SouthWest";
			if (isBlocked(startTile, MapData::O_WESTWALL, missileTarget))
				return true;
			if (isBlocked(_save->getTile(currentPosition + oneTileSouth), MapData::O_WESTWALL, missileTarget))
				return true;
			if (isBlocked(_save->getTile(currentPosition + oneTileSouth), MapData::O_NORTHWALL, missileTarget))
				return true;
			if (isBlocked(_save->getTile(currentPosition + oneTileSouth), O_BIGWALL, missileTarget, BIGWALL_NESW))
				return true;
			if (isBlocked(_save->getTile(currentPosition + oneTileWest), O_BIGWALL, missileTarget, BIGWALL_NESW))
				return true;
			if (isBlocked(_save->getTile(currentPosition + oneTileSouth + oneTileWest), MapData::O_NORTHWALL, missileTarget))
				return true;
		break;
		case 6: // west
			//Log(LOG_INFO) << ". try West";
			if (isBlocked(startTile, MapData::O_WESTWALL, missileTarget))
				return true;
		break;
		case 7: // north-west
			//Log(LOG_INFO) << ". try NorthWest";
			if (isBlocked(startTile, MapData::O_WESTWALL, missileTarget))
				return true;
			if (isBlocked(startTile, MapData::O_NORTHWALL, missileTarget))
				return true;
			if (isBlocked(_save->getTile(currentPosition + oneTileWest), MapData::O_NORTHWALL, missileTarget))
				return true;
			if (isBlocked(_save->getTile(currentPosition + oneTileNorth), MapData::O_WESTWALL, missileTarget))
				return true;
			if (isBlocked(_save->getTile(currentPosition + oneTileNorth), O_BIGWALL, missileTarget, BIGWALL_NWSE))
				return true;
			if (isBlocked(_save->getTile(currentPosition + oneTileWest), O_BIGWALL, missileTarget, BIGWALL_NWSE))
				return true;
		break;
	} */

	return false;
}

/**
 * Determines whether a unit can fall down from this tile.
 * We can fall down here,
 * if the current position is higher than 0 (the tileBelow does not exist)
 * or if the tile has no floor, [or if there is no unit standing below us].
 * @param here, The current tile.
 * @return, True if a unit can fall down.
 */
// aha! (kL) This is why that sectoid stood in the air: it walked off
// the top of a building but there was a cyberdisc below!!! NOT.
bool Pathfinding::canFallDown(Tile* here)
{
	if (here->getPosition().z == 0) // already on lowest maplevel
		return false;

	Tile* tileBelow = _save->getTile(here->getPosition() - Position(0, 0, 1));

	return here->hasNoFloor(tileBelow);
}

/**
 * Wrapper for canFallDown() above.
 * @param here, The current tile.
 * @param size, The size of the unit.
 * @return, True if a unit can fall down.
 */
bool Pathfinding::canFallDown(
		Tile* here,
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
			Position checkPos = here->getPosition() + Position(x, y, 0);
			Tile* checkTile = _save->getTile(checkPos);
			if (!canFallDown(checkTile))
				//Log(LOG_INFO) << "Pathfinding::canFallDown() ret FALSE";
				return false;
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

	return false;
} */

/**
 * Checks if a movement is valid via the Up/Down buttons.
 * Either there is a grav lift or the unit can fly, and there are no obstructions.
 * @param bu			- oointer to a unit
 * @param startPosition	- starting position
 * @param direction		- up or down
 * @return,	-1 kneeling (stop unless on gravLift)
			 0 blocked (stop)
			 1 gravLift (go)
			 2 flying (go unless blocked)
 */
int Pathfinding::validateUpDown(
		BattleUnit* bu,
		Position startPosition,
		int const direction)
{
	Position endPosition;
	directionToVector(
					direction,
					&endPosition);
	endPosition += startPosition;

	Tile* startTile = _save->getTile(startPosition);
	Tile* destTile = _save->getTile(endPosition);

	if (startTile->getMapData(MapData::O_FLOOR)
		&& startTile->getMapData(MapData::O_FLOOR)->isGravLift()
		&& destTile
		&& destTile->getMapData(MapData::O_FLOOR)
		&& destTile->getMapData(MapData::O_FLOOR)->isGravLift())
	{
		return 1; // gravLift.
	}
	else if (bu->isKneeled())
		return -1; // kneeled.
	else if (bu->getArmor()->getMovementType() == MT_FLY)
	{
		Tile* belowStart = _save->getTile(startPosition + Position(0, 0,-1));
		if ((direction == DIR_UP
				&& destTile
				&& !destTile->getMapData(MapData::O_FLOOR)) // flying up only possible when there is no roof
			|| (direction == DIR_DOWN
				&& destTile
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
 * Checks if going one step from start to destination in the given direction requires
 * going through a closed UFO door.
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
 * @param bRemove, To remove preview or not
 * @return, True if a path is previewed
 */
bool Pathfinding::previewPath(bool bRemove)
{
	if (_path.empty())
		return false;

	if (!bRemove
		&& _pathPreviewed)
	{
		return false;
	}

	_pathPreviewed = !bRemove;

	Position
		pos = _unit->getPosition(),
		destination;

	int tus = _unit->getTimeUnits();
	if (_unit->isKneeled()) // -> and not on gravLift, and not moving an x/y direction.
							// ie. up/down or gravLift:floor to gravLift:floor
		tus -= 8;
		// That is not right when going up/down GravLifts kneeled ...!
		// note: Neither is energy, below.

	int
		energy		= _unit->getEnergy(),
		energyStop	= energy,
		size		= _unit->getArmor()->getSize() - 1,
		dir			= -1,
		total		= 0,
		tu			= 0,
		color		= 0;

	bool switchBack = false;
	if (_save->getBattleGame()->getReservedAction() == BA_NONE)
	{
		switchBack = true;
		_save->getBattleGame()->setTUReserved(
//kL											BA_AUTOSHOT,
											BA_SNAPSHOT, // kL
											false);
	}

	std::string armorType = _unit->getArmor()->getType();

	_modifierUsed = (SDL_GetModState() & KMOD_CTRL) != 0;
	bool
		dash = Options::strafe
				&& _modifierUsed
				&& !size
				&& _path.size() > 1,	// <- not exactly true. If moving around a corner +2 tiles, it
										// will strafe (potential conflict). See WalkBState also or ...
		bLaden		= armorType == "STR_PERSONAL_ARMOR_UC",
		bPowered	= armorType == "STR_POWER_SUIT_UC",
		bPowered2	= armorType == "STR_FLYING_SUIT_UC",
		gravLift	= false,
		reserve		= false;
		// kL_note: Ought to create a factor for those in ruleArmor class & RuleSets ( _burden ). Or 'enum' those,
		// as in
/* enum ArmorBurden
{
	AB_LOW,		// -1
	AB_NORMAL,	// 0
	AB_MEDIUM,	// 1
	AB_HIGH		// 2
}; */

	Tile
		* tileLift,
		* tile;

	for (std::vector<int>::reverse_iterator
			i = _path.rbegin();
			i != _path.rend();
			++i)
	{
		dir = *i;

		// gets tu cost, but also gets the destination position.
		tu = getTUCost(
					pos,
					dir,
					&destination,
					_unit,
					0,
					false);
		energyStop = energy;

		tileLift = _save->getTile(pos);
		gravLift = dir >= DIR_UP
				&& tileLift->getMapData(MapData::O_FLOOR)
				&& tileLift->getMapData(MapData::O_FLOOR)->isGravLift();
		if (!gravLift)
		{
			if (dash)
			{
				energy -= tu * 3 / 2;
				tu = tu * 3 / 4;
			}
			else
				energy -= tu;

			if (bLaden)
				energy -= 1;
			else if (bPowered)
			{
				energy += 1;
			}
			else if (bPowered2)
			{
				energy += 2;
			}

			if (energy > energyStop)
				energy = energyStop;
		}
//		else // gravLift
//			energy = 0;


		tus -= tu;
		total += tu;
		reserve = _save->getBattleGame()->checkReservedTU(
														_unit,
														total,
														true);

		pos = destination;
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
				tile = _save->getTile(pos + Position(x, y, 0));
				if (!bRemove)
				{
					if (i == _path.rend() - 1)
						tile->setPreview(10);
					else
					{
						int nextDir = *(i + 1);
						tile->setPreview(nextDir);
					}

					tile->setTUMarker(tus);
				}
				else
				{
					tile->setPreview(-1);
					tile->setTUMarker(0);
				}

				color = 0;
				if (!bRemove)
				{
					if (tus > -1
						&& energy > -1)
					{
						if (reserve)
							color = 4; // green
						else
							color = 10; // yellow
					}
					else
						color = 3; // red
				}

				tile->setMarkerColor(color);
			}
		}
	}

	if (switchBack)
		_save->getBattleGame()->setTUReserved(
											BA_NONE,
											false);

	return true;
}

/**
 * Unmarks the tiles used for the path preview.
 * @return True, if the previewed path was removed.
 */
bool Pathfinding::removePreview()
{
	if (!_pathPreviewed)
		return false;

	previewPath(true);

	return true;
}

/**
 * Gets the path preview setting.
 * @return True, if paths are previewed.
 */
bool Pathfinding::isPathPreviewed() const
{
	return _pathPreviewed;
}

/**
 * Locates all tiles reachable to @a *unit with a TU cost no more than @a tuMax.
 * Uses Dijkstra's algorithm.
 * @param unit Pointer to the unit.
 * @param tuMax The maximum cost of the path to each tile.
 * @return An array of reachable tiles, sorted in ascending order of cost. The first tile is the start location.
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

	while (!unvisited.empty())
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
//kL				|| (currentNode->getTUCost(false) + tuCost) / 2 > energyMax) // Run out of TUs/Energy
				|| (currentNode->getTUCost(false) + tuCost) > energyMax) // kL
			{
				continue;
			}

			PathfindingNode* nextNode = getNode(nextPos);
			if (nextNode->isChecked()) // Our algorithm means this node is already at minimum cost.
				continue;

			int totalTuCost = currentNode->getTUCost(false) + tuCost;
			if (!nextNode->inOpenSet()
				|| nextNode->getTUCost(false) > totalTuCost) // If this node is unvisited or visited from a better path.
			{
				nextNode->connect(totalTuCost, currentNode, direction);

				unvisited.push(nextNode);
			}
		}

		currentNode->setChecked();
		reachable.push_back(currentNode);
	}

	std::sort(reachable.begin(), reachable.end(), MinNodeCosts());

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
 * @return Strafe move.
 */
bool Pathfinding::getStrafeMove() const
{
	return _strafeMove;
}

/**
 * Sets _unit in order to abuse low-level pathfinding functions from outside the class.
 * @param unit Unit taking the path.
 */
void Pathfinding::setUnit(BattleUnit* unit)
{
	_unit = unit;

	if (unit != 0)
		_movementType = unit->getArmor()->getMovementType();
	else
		_movementType = MT_WALK;
}

/**
 * Checks whether a modifier key was used to enable strafing or running.
 * @return, True if a modifier was used.
 */
bool Pathfinding::isModifierUsed() const
{
	return _modifierUsed;
}

/**
 * Gets a reference to the current path.
 * @return, the actual path.
 */
const std::vector<int>& Pathfinding::getPath()
{
	return _path;
}

/**
 * Makes a copy of the current path.
 * @return a copy of the path.
 */
std::vector<int> Pathfinding::copyPath() const
{
	return _path;
}

}
