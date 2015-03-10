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

#include "MapBlock.h"

//#include <sstream>

#include "../Battlescape/Position.h"

//#include "../Engine/Exception.h"


namespace OpenXcom
{

/**
 * This is MapBlock construction.
 * @param type - reference the type of the MapBlock
 */
MapBlock::MapBlock(const std::string& type)
	:
		_type(type),
		_size_x(10),
		_size_y(10),
		_size_z(4)
{
	_groups.push_back(0);
}

/**
 * MapBlock destruction.
 */
MapBlock::~MapBlock()
{}

/**
 * Loads the map block from a YAML file.
 * @param node - reference a YAML node
 */
void MapBlock::load(const YAML::Node& node)
{
	_type	= node["type"]	.as<std::string>(_type);
	_size_x	= node["width"]	.as<int>(_size_x);
	_size_y	= node["length"].as<int>(_size_y);
	_size_z	= node["height"].as<int>(_size_z);

	if (_size_x %10 != 0
		|| _size_y %10 != 0)
	{
		std::stringstream ss;
		ss << "Error: MapBlock " << _type << ": Size must be divisible by ten";
		throw Exception(ss.str());
	}

	if (const YAML::Node& block = node["groups"])
	{
		_groups.clear();

		if (block.Type() == YAML::NodeType::Sequence)
			_groups = block.as<std::vector<int> >(_groups);
		else
			_groups.push_back(block.as<int>(0));
	}

	if (const YAML::Node& block = node["revealedFloors"])
	{
		_revealedFloors.clear();

		if (block.Type() == YAML::NodeType::Sequence)
			_revealedFloors = block.as<std::vector<int> >(_revealedFloors);
		else
			_revealedFloors.push_back(block.as<int>(0));
	}

	_items = node["items"].as<std::map<std::string, std::vector<Position> > >(_items);
}

/**
 * Gets the MapBlock type.
 * @return, the type of this block
 */
std::string MapBlock::getType() const
{
	return _type;
}

/**
 * Gets the MapBlock size x.
 * @return, x in tiles
 */
int MapBlock::getSizeX() const
{
	return _size_x;
}

/**
 * Gets the MapBlock size y.
 * @return, y in tiles
 */
int MapBlock::getSizeY() const
{
	return _size_y;
}

/**
 * Sets the MapBlock size z.
 * @param size_z - z in tiles
 */
void MapBlock::setSizeZ(int size_z)
{
	_size_z = size_z;
}

/**
 * Gets the MapBlock size z.
 * @return, z in tiles
 */
int MapBlock::getSizeZ() const
{
	return _size_z;
}

/**
 * Gets whether this MapBlock is from a particular group.
 * @param group - the group to check for
 * @return, true if block is defined in the specified group
 */
bool MapBlock::isInGroup(int group) const
{
	return std::find(
				_groups.begin(),
				_groups.end(),
				group) != _groups.end();
}

/**
 * Gets if this floor should be revealed or not.
 * @param reveal - the floor to check reveal value of
 * @return, true if floor will be revealed
 */
bool MapBlock::isFloorRevealed(int reveal) const
{
	return std::find(
				_revealedFloors.begin(),
				_revealedFloors.end(),
				reveal) != _revealedFloors.end();
}

/**
 * Gets any items associated with this block and their Positions.
 * @return, pointer to a map of items and their positions
 */
std::map<std::string, std::vector<Position> >* MapBlock::getItems()
{
	return &_items;
}

}
