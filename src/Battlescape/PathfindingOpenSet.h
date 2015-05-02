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

#ifndef OPENXCOM_PATHFINDINGOPENSET_H
#define OPENXCOM_PATHFINDINGOPENSET_H

//#include <queue>


namespace OpenXcom
{

class PathfindingNode;


struct OpenSetEntry
{
	int _cost;
	PathfindingNode* _node;
};


/**
 * Helper class to compare entries using pointers.
 */
class EntryCompare
{
	public:
		/**
		 * Compares entries @a *a and @a *b.
		 * @param a - pointer to first entry
		 * @param b - pointer to second entry
		 * @return, true if entry @a *b must come before @a *a
		 */
		bool operator() (OpenSetEntry* a, OpenSetEntry* b) const
		{
			return b->_cost < a->_cost;
		}
};


/**
 * A class that holds references to the nodes to be examined in pathfinding.
 */
class PathfindingOpenSet
{

private:
	std::priority_queue<OpenSetEntry*, std::vector<OpenSetEntry*>, EntryCompare> _queue;

	/// Removes reachable discarded entries.
	void removeDiscarded();


	public:
		/// Cleans up the set and frees allocated memory.
		~PathfindingOpenSet();

		/// Adds a node to the set.
		void addNode(PathfindingNode* node);
		/// Gets the next node to check.
		PathfindingNode* getNode();

		/// Gets if the set is empty.
		bool isNodeSetEmpty() const
		{	return (_queue.empty() == true); }
};

}

#endif
