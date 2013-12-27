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

#ifndef OPENXCOM_NODE_H
#define OPENXCOM_NODE_H

#include <yaml-cpp/yaml.h>

#include "../Battlescape/Position.h"


namespace OpenXcom
{

enum NodeRank
{
	NR_SCOUT,		// #0
	NR_XCOM,		// #1
	NR_SOLDIER,		// #2
	NR_NAVIGATOR,	// #3
	NR_LEADER,		// #4
	NR_ENGINEER,	// #5
	NR_MISC1,		// #6
	NR_MEDIC,		// #7
	NR_MISC2		// #8
};


/**
 * Represents a node/spawnpoint in the battlescape, loaded from RMP files.
 * @sa http://www.ufopaedia.org/index.php?title=ROUTES
 */
class Node
{

private:
	bool _allocated;
	int
		_flags,		// might not be used at present.
		_id,		// unique identifier
		_priority,	// "spawn" in .Mcd
		_rank,		// aLien rank that can spawn here
		_reserved,	//
		_segment,	//
		_type;		// usable by small/large/flying units.

	Position _pos;

	std::vector<int> _nodeLinks;


	public:
		static const int CRAFTSEGMENT	= 1000;
		static const int UFOSEGMENT		= 2000;

		static const int TYPE_FLYING	= 0x01;	// non-flying unit cannot spawn here when this bit is set
		static const int TYPE_SMALL		= 0x02;	// large unit cannot spawn here when this bit is set
		static const int TYPE_DANGEROUS	= 0x04;	// an alien was shot here, stop patrolling to it like an idiot with a death wish

		static const int nodeRank[8][7];		// maps alien ranks to node (.RMP) ranks

		/// Creates a Node.
		Node();
		///
		Node(
				int id,
				Position pos,
				int segment,
				int type,
				int rank,
				int flags,
				int reserved,
				int priority);
		/// Cleans up the Node.
		~Node();

		/// Loads the node from YAML.
		void load(const YAML::Node& node);
		/// Saves the node to YAML.
		YAML::Node save() const;

		/// get the node's id
		int getID() const;
		/// get the node's paths
		std::vector<int>* getNodeLinks();
		/// Gets node's rank.
		NodeRank getRank() const;
		/// Gets node's priority.
		int getPriority() const;
		/// Gets the node's position.
		const Position& getPosition() const;
		/// Gets the node's segment.
		int getSegment() const;
		/// Gets the node's type.
		int getType() const;
		/// Sets the node's type, surprisingly
		void setType(int type);
		/// gets "flags" variable, which is really the patrolling desirability value
		int getFlags()
		{
			return _flags;
		}
		/// compares the _flags variables of the nodes (for the purpose of patrol decisions!)
		bool operator<(Node &b)
		{
			return _flags < b.getFlags();
		};
		///
		bool isAllocated() const;
		///
		void allocateNode();
		///
		void freeNode();
		///
		bool isTarget() const;
};

}

#endif
