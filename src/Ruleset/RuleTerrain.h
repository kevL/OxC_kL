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

#ifndef OPENXCOM_RULETERRAIN_H
#define OPENXCOM_RULETERRAIN_H

//#include <string>
//#include <vector>
//#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

class MapBlock;
class MapData;
class MapDataSet;
class Ruleset;


/**
 * Represents a specific type of Battlescape Terrain.
 * - the names of the objectsets needed in this specific terrain.
 * - the mapblocks that can be used to build this terrain.
 * @sa http://www.ufopaedia.org/index.php?title=TERRAIN
 */
class RuleTerrain
{

private:
	std::string
		_pyjamaType,
		_type,
		_script;

	std::vector<std::string>
		_civilianTypes,
		_musics;

	std::vector<MapBlock*> _mapBlocks;
	std::vector<MapDataSet*> _mapDataSets;


	public:
		/// Constructs a RuleTerrain object.
		explicit RuleTerrain(const std::string& type);
		/// Destructs this RuleTerrain object.
		~RuleTerrain();

		/// Loads the terrain from YAML.
		void load(
				const YAML::Node& node,
				Ruleset* const rules);

		/// Gets the terrain's type (used for MAP generation).
		const std::string& getType() const;

		/// Gets the terrain's mapblocks.
		std::vector<MapBlock*>* getMapBlocks();
		/// Gets the terrain's mapdatafiles.
		std::vector<MapDataSet*>* getMapDataSets();

		/// Gets a random mapblock.
		MapBlock* getRandomMapBlock(
				int maxSizeX,
				int maxSizeY,
				int group,
				bool force = true) const;
		/// Gets a mapblock given its type.
		MapBlock* getMapBlock(const std::string& type) const;
		/// Gets the mapdata object.
		MapData* getMapData(
				unsigned int* id,
				int* mapDataSetId) const;

		/// Gets the civilian types to use.
		const std::vector<std::string>& getCivilianTypes() const;

		/// Gets the generation script.
		const std::string& getScript() const;

		/// Gets the list of music to pick from.
		const std::vector<std::string>& getTerrainMusics() const;

		/// Gets the pyjama type.
		const std::string& getPyjamaType() const;
};

}

#endif
