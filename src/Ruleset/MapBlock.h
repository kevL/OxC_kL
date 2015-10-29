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

#ifndef OPENXCOM_MAPBLOCK_H
#define OPENXCOM_MAPBLOCK_H

//#include <string>
//#include <vector>
//#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

enum MapBlockType // These define 'groups'
{
	MBT_UNDEFINED = -1,	// -1
	MBT_DEFAULT,		//  0
	MBT_LANDPAD,		//  1
	MBT_EWROAD,			//  2
	MBT_NSROAD,			//  3
	MBT_CROSSROAD,		//  4
	MBT_START,			//  5 kL_add. For AlienBase starting equipment spawn
	MBT_CONTROL,		//  6
	MBT_BRAIN			//  7
};


class Position;


/**
 * Represents a Terrain Map Block.
 * It contains constant info about this mapblock, like its type, dimensions, attributes...
 * Map blocks are stored in RuleTerrain objects.
 * @sa http://www.ufopaedia.org/index.php?title=MAPS_Terrain
 */
class MapBlock
{

private:
	std::string _type;
	int
		_size_x,
		_size_y,
		_size_z;

	std::vector<int>
		_groups,
		_revealedFloors;

	std::map<std::string, std::vector<Position> > _items;


	public:
		/// Constructs a MapBlock object.
		explicit MapBlock(const std::string& type);
		/// Destructs this MapBlock object.
		~MapBlock();

		/// Loads the map block from YAML.
		void load(const YAML::Node& node);

		/// Gets the mapblock's type - used for MAP generation.
		const std::string& getType() const;

		/// Gets the mapblock's x size.
		int getSizeX() const;
		/// Gets the mapblock's y size.
		int getSizeY() const;
		/// Gets the mapblock's z size.
		int getSizeZ() const;
		/// Sets the mapblock's z size.
		void setSizeZ(int size_z);

		/// Gets if this mapblock is from the group specified.
		bool isInGroup(int group) const;
		/// Gets if this floor should be revealed or not.
		bool isFloorRevealed(int reveal) const;

		/// Gets the layout for any items that belong in this map block.
		std::map<std::string, std::vector<Position> >* getItems();
};

}

#endif
