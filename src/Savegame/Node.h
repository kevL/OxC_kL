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

#ifndef OPENXCOM_NODE_H
#define OPENXCOM_NODE_H

//#include <yaml-cpp/yaml.h>

#include "../Battlescape/Position.h"


namespace OpenXcom
{

enum NodeRank
{
	NR_SCOUT,		// 0
	NR_XCOM,		// 1
	NR_SOLDIER,		// 2
	NR_NAVIGATOR,	// 3
	NR_LEADER,		// 4
	NR_ENGINEER,	// 5
	NR_MISC1,		// 6
	NR_MEDIC,		// 7
	NR_MISC2		// 8
};


/**
 * Represents a node/spawnpoint in the battlescape loaded from RMP files.
 * @sa http://www.ufopaedia.org/index.php?title=ROUTES
 */
class Node
{

private:
	bool _allocated;
	int
		_patrol,	// desirability of patrolling to - "flags" in .RMP
		_id,		// unique identifier
		_priority,	// desirability of spawning at - "spawn" in .RMP
		_rank,		// aLien rank that can spawn or path here
		_destruct,	// something to do with shooting objectives in BaseDefense missions
		_segment,	// something to do with nodeLinks; see BattlescapeGenerator::attachNodeLinks()
		_type;		// usable by small/large/flying units.

	Position _pos;

	std::vector<int> _nodeLinks;


	public:
		static const int
			SEG_CRAFT	= 1000,
			SEG_UFO		= 2000,

		// -> Any=0; Flying=1; Small=2; FlyingLarge=3; Large=4 <- loaded in BattlescapeGenerator::loadRMP()
			TYPE_FLYING			= 0x01,	// non-flying unit cannot spawn here when this bit is set; see SavedBattleGame::getSpawnNode()
			TYPE_SMALL			= 0x02,	// large unit cannot spawn here when this bit is set; see SavedBattleGame::getSpawnNode()
										// NOTE: getNodeType() is also used in SavedBattleGame::getPatrolNode() <- ie, it's not about spawning only; it affects patrolling also.
			TYPE_LARGEFLYING	= 0x04,	// kL_add
			TYPE_LARGE			= 0x08,	// kL_add (not used... equivalent to Any)
			TYPE_DANGEROUS		= 0x10,	// kL_changed from 0x04[ie.large] -> an alien was shot here, stop patrolling to it like an idiot with a death wish

			nodeRank[8][8]; // maps node-ranks (.RMP) to aliens' ranks

		/// Creates a Node.
		Node();
		///
		Node(
				size_t id,
				Position pos,
				int segment,
				int type,
				int nodeRank,
				int patrol,
				int reserved,
				int priority);
		/// Cleans up the Node.
		~Node();

		/// Loads the node from YAML.
		void load(const YAML::Node& node);
		/// Saves the node to YAML.
		YAML::Node save() const;

		/// Gets the node's id.
		int getID() const;

		/// Gets the node's paths.
		std::vector<int>* getNodeLinks();

		/// Gets node's rank.
		NodeRank getNodeRank() const;

		/// Gets node's priority.
		int getPriority() const;

		/// Gets the node's position.
		const Position& getPosition() const;

		/// Gets the node's segment.
		int getSegment() const;

		/// Sets the node's type. SURPRISE!! (not)
		void setNodeType(int type);
		/// Gets the node's type.
		int getNodeType() const;

		/// Gets the 'flags' variable which is really the patrol-desirability value.
		int getPatrol()
		{	return _patrol; }

		// kL_note: in SavedBattleGame::getPatrolNodes() I changed less-than to greater-than ...
		// wonder if that matters here. So: REVERTED.
		// correction: REMOVED. Just use getPatrol() directly. Tks, anyway
		/// Compares the 'flags' variables of the nodes for the purpose of patrol decisions.
//		bool operator< (Node& b)
//		{	return _patrol < b.getPatrol(); };
		/// Compares the 'flags' variables of the nodes for the purpose of patrol decisions.
//		bool operator> (Node& b)
//		{	return _patrol > b.getPatrol(); };


		/// Gets if this Node is allocated.
		bool isAllocated() const;
		/// Sets this Node as allocated.
		void allocateNode();
		/// Sets this Node as NOT allocated.
		void freeNode();

		/// Gets if this Node is suitable for an aLien to target XCOM base targets/objectives.
		bool isTarget() const;
};

}

#endif
