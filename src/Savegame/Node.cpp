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

#include "Node.h"


namespace OpenXcom
{

/* const int Node::nodeRank[8][7] =
{
	{4, 3, 5, 8, 7, 2, 0},	// commander
	{4, 3, 5, 8, 7, 2, 0},	// leader
	{5, 4, 3, 2, 7, 8, 0},	// engineer
	{7, 6, 2, 8, 3, 4, 0},	// medic
	{3, 4, 5, 2, 7, 8, 0},	// navigator
	{2, 5, 3, 4, 6, 8, 0},	// soldier
	{2, 5, 3, 4, 6, 8, 0},	// terrorist1
	{2, 5, 3, 4, 6, 8, 0}	// terrorist2
}; */
const int Node::nodeRank[8][8] = // kL_begin:
{
	{4, 3, 5, 2, 7, 8, 6, 0},	// commander
	{4, 3, 5, 2, 7, 8, 6, 0},	// leader
	{5, 4, 3, 2, 7, 8, 6, 0},	// engineer
	{7, 3, 5, 4, 2, 8, 6, 0},	// medic
	{3, 4, 5, 2, 7, 8, 6, 0},	// navigator
	{2, 5, 3, 7, 4, 8, 6, 0},	// soldier
	{6, 8, 2, 5, 3, 4, 7, 0},	// terrorist1
	{8, 6, 2, 5, 3, 4, 7, 0}	// terrorist2
}; // kL_end.
// kL_note: The 2nd dimension holds fallbacks for spawning:

	// 0:Civ-Scout
	// 1:XCom
	// 2:Soldier
	// 3:Navigator
	// 4:Leader/Commander
	// 5:Engineer
	// 6:Misc1
	// 7:Medic
	// 8:Misc2

// see Node.h, enum NodeRank ->


Node::Node()
	:
		_id(0),
		_segment(0),
		_type(0),
		_rank(0),
		_flags(0),
		_reserved(0),
		_priority(0),
		_allocated(false)
{
}

/**
 * Initializes a Node.
 * @param id
 * @param pos
 * @param segment
 * @param type
 * @param rank
 * @param flags
 * @param reserved
 * @param priority
 */
Node::Node(
		size_t id,
		Position pos,
		int segment,
		int type,
		int rank,
		int flags,
		int reserved,
		int priority)
	:
		_id(id),
		_pos(pos),
		_segment(segment),
		_type(type),
		_rank(rank),
		_flags(flags),
		_reserved(reserved),
		_priority(priority),
		_allocated(false)
{
}

/**
 * clean up node.
 */
Node::~Node()
{
}

/**
 * Loads the UFO from a YAML file.
 * @param node - reference a YAML node
 */
void Node::load(const YAML::Node& node)
{
	_id			= node["id"].as<int>(_id);
	_pos		= node["position"].as<Position>(_pos);
//	_segment	= node["segment"].as<int>(_segment);
	_type		= node["type"].as<int>(_type);
	_rank		= node["rank"].as<int>(_rank);
	_flags		= node["flags"].as<int>(_flags);
	_reserved	= node["reserved"].as<int>(_reserved);
	_priority	= node["priority"].as<int>(_priority);
	_allocated	= node["allocated"].as<bool>(_allocated);
	_nodeLinks	= node["links"].as<std::vector<int> >(_nodeLinks);
}

/**
 * Saves the UFO to a YAML file.
 * @return, YAML node
 */
YAML::Node Node::save() const
{
	YAML::Node node;

	node["id"]			= _id;
	node["position"]	= _pos;
//	node["segment"]		= _segment;
	node["type"]		= _type;
	node["rank"]		= _rank;
	node["flags"]		= _flags;
	node["reserved"]	= _reserved;
	node["priority"]	= _priority;
	node["allocated"]	= _allocated;
	node["links"]		= _nodeLinks;

	return node;
}

/**
 * Get the node's id
 * @return unique id
 */
int Node::getID() const
{
	return _id;
}

/**
 * Get the rank of units that can spawn on this node.
 * @return noderank
 */
NodeRank Node::getRank() const
{
	return (NodeRank)_rank;
}

/**
 * Get the priority of this spawnpoint.
 * @return priority
 */
int Node::getPriority() const
{
	return _priority;
}

/**
 * Gets the Node's position.
 * @return position
 */
const Position& Node::getPosition() const
{
	return _pos;
}

/**
 * Gets the Node's segment.
 * @return segment
 */
int Node::getSegment() const
{
	return _segment;
}

/**
 * get the node's paths
 */
std::vector<int>* Node::getNodeLinks()
{
	return &_nodeLinks;
}

/**
 * Gets the Node's type.
 * @return type
 */
int Node::getType() const
{
	return _type;
}

/**
 *
 */
bool Node::isAllocated() const
{
	return _allocated;
}

/**
 *
 */
void Node::allocateNode()
{
	_allocated = true;
}

/**
 *
 */
void Node::freeNode()
{
	_allocated = false;
}

/**
 *
 */
bool Node::isTarget() const
{
	return _reserved == 5;
}

/**
 *
 */
void Node::setType(int type)
{
	_type = type;
}

}
