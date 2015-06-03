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

#ifndef OPENXCOM_PATHFINDINGNODE_H
#define OPENXCOM_PATHFINDINGNODE_H

#include "Position.h"


namespace OpenXcom
{

class PathfindingOpenSet;

struct OpenSetEntry;


/**
 * A class that holds pathfinding info for a node on the map.
 */
class PathfindingNode
{

private:
	friend class PathfindingOpenSet;

	bool _checked;
	int
		_prevDir,
		_tuCost,
		_tuGuess; // approximate cost to reach goal position

	OpenSetEntry* _openSetEntry; // invasive field needed by PathfindingOpenSet
	PathfindingNode* _prevNode;

	Position _pos;


	public:
		/// Creates a new PathfindingNode class.
		explicit PathfindingNode(Position pos);
		/// Cleans up the PathfindingNode.
		~PathfindingNode();

		/// Gets this Node's position.
		const Position& getPosition() const;

		/// Resets this Node.
		void reset();

		/// Gets if this Node has been checked.
		bool getChecked() const;
		/// Marks this Node as checked.
		void setChecked()
		{ _checked = true; }

		/// Gets this Node's TU cost.
		int getTUCostNode(bool missile = false) const;
		/// Gets the approximate cost to reach the target position.
		int getTUGuess() const
		{ return _tuGuess; }

		/// Gets this Node's previous Node.
		PathfindingNode* getPrevNode() const;
		/// Gets the previous walking direction.
		int getPrevDir() const;

		/// Gets if this Node is already in a PathfindingOpenSet.
		bool inOpenSet() const
		{ return (_openSetEntry != NULL); }

//#ifdef __MORPHOS__
//	#undef connect
//#endif

		/// Connects to previous Node along the path.
		void linkNode(
				int tuCost,
				PathfindingNode* prevNode,
				int prevDir,
				const Position& target);
		/// Connects to previous Node along a visit.
		void linkNode(
				int tuCost,
				PathfindingNode* prevNode,
				int prevDir);
};


/**
 * Compares PathfindingNode pointers based on TU cost.
 */
class MinNodeCosts
{

public:
	/**
	 * Compares nodes @a *a and @a *b.
	 * @param a - pointer to first node
	 * @param b - pointer to second node
	 * @return, true if node @a *a must come before @a *b
	 */
	bool operator() (const PathfindingNode* a, const PathfindingNode* b) const
	{
		return (a->getTUCostNode() < b->getTUCostNode());
	}
};

}

#endif
