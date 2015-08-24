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

#include "PathfindingNode.h"

//#include <math.h>


namespace OpenXcom
{

/**
 * Sets up a PathfindingNode.
 * @param pos - Position on the battlefield
 */
PathfindingNode::PathfindingNode(Position pos)
	:
		_pos(pos),
		_checked(false),
		_tuCost(0),
		_tuGuess(0),
		_prevNode(NULL),
		_prevDir(0),
		_openSetEntry(NULL)
{}

/**
 * Deletes the PathfindingNode.
 */
PathfindingNode::~PathfindingNode()
{}

/**
 * Gets this Node's Position.
 * @return - reference to position
 */
const Position& PathfindingNode::getPosition() const
{
	return _pos;
}

/**
 * Resets this Node.
 */
void PathfindingNode::reset()
{
	_checked = false;
	_openSetEntry = NULL;
}

/**
 * Gets the checked status of this Node.
 * @return, true if node has been checked
 */
bool PathfindingNode::getChecked() const
{
	return _checked;
}

/**
 * Gets this Node's TU cost.
 * @param missile - true if this is a missile (default false)
 * @return, TU cost
 */
int PathfindingNode::getTuCostNode(bool missile) const
{
	if (missile == true)
		return 0;

	return _tuCost;
}

/**
 * Gets this Node's previous Node.
 * @return, pointer to the previous node
 */
PathfindingNode* PathfindingNode::getPrevNode() const
{
	return _prevNode;
}

/**
 * Gets the previous walking direction for how a unit got on this Node.
 * @return, previous dir
 */
int PathfindingNode::getPrevDir() const
{
	return _prevDir;
}

/**
 * Connects the node.
 * @note This will connect the node to the previous node along the path to its
 * @a target and update Pathfinding information.
 * @param tuCost	- the total cost of the path so far
 * @param prevNode	- pointer to the previous node along the path
 * @param prevDir	- the direction FROM the previous node
 * @param target	- reference of the target position (used to update the '_tuGuess' cost)
*/
void PathfindingNode::linkNode(
		int tuCost,
		PathfindingNode* prevNode,
		int prevDir,
		const Position& target)
{
	_tuCost = tuCost;
	_prevNode = prevNode;
	_prevDir = prevDir;

	if (inOpenSet() == false) // otherwise this has been done already
	{
		Position pos = target - _pos;
		pos *= pos;
		_tuGuess = 4 * static_cast<int>(std::ceil(std::sqrt(
					   static_cast<double>(pos.x + pos.y + pos.z))));
	}
}

/**
 * Connects the node.
 @note This will connect the node to the previous node along the path.
 * @param tuCost	- the total cost of the path so far
 * @param prevNode	- pointer to the previous node along the path
 * @param prevDir	- the direction FROM the previous node
*/
void PathfindingNode::linkNode(
		int tuCost,
		PathfindingNode* prevNode,
		int prevDir)
{
	_tuCost = tuCost;
	_prevNode = prevNode;
	_prevDir = prevDir;
	_tuGuess = 0;
}

}
