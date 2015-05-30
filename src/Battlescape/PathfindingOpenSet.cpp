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

#include "PathfindingOpenSet.h"

//#include <assert.h>

#include "PathfindingNode.h"


namespace OpenXcom
{

/**
 * Cleans up all the entries still in set.
 */
PathfindingOpenSet::~PathfindingOpenSet()
{
	while (_queue.empty() == false)
	{
		const OpenSetEntry* const entry = _queue.top();
		_queue.pop();

		delete entry;
	}
}

/**
 * Keeps removing all discarded entries that have come to the top of the queue.
 */
void PathfindingOpenSet::removeDiscarded()
{
	while (_queue.empty() == false
		&& _queue.top()->_node == NULL)
	{
		const OpenSetEntry* const entry = _queue.top();
		_queue.pop();

		delete entry;
	}
}

/**
 * Gets the node with the least cost.
 * @note After this call the node is no longer in the set.
 * @note It is an error to call this when the set is empty.
 * @return, pointer to the node which had the least cost
 */
PathfindingNode* PathfindingOpenSet::getNode()
{
	assert(isNodeSetEmpty() == false);

	const OpenSetEntry* const entry = _queue.top();
	PathfindingNode* const node = entry->_node;
	_queue.pop();

	delete entry;
	node->_openSetEntry = NULL;

	removeDiscarded(); // Discarded entries might be visible now.

	return node;
}

/**
 * Places the node in the set.
 * @note If the node was already in the set the previous entry is discarded.
 * @note It is the caller's responsibility to never re-add a node with a worse cost.
 * @param node - pointer to the node to add
 */
void PathfindingOpenSet::addNode(PathfindingNode* node)
{
	OpenSetEntry* const entry = new OpenSetEntry;
	entry->_node = node;
	entry->_cost = node->getTUCostNode() + node->getTUGuess();

	if (node->_openSetEntry != NULL)
		node->_openSetEntry->_node = NULL;

	node->_openSetEntry = entry;
	_queue.push(entry);
}

}
