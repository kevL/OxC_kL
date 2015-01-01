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

#include "MapDataSet.h"
#include "Ruleset.h"
#include "RuleTerrain.h"

#include "../Engine/RNG.h"


namespace OpenXcom
{

/**
 * RuleTerrain construction.
 */
RuleTerrain::RuleTerrain(const std::string& name)
	:
		_name(name),
		_script("DEFAULT"),
		_hemisphere(0),
		_minDepth(0),
		_maxDepth(0),
		_ambience(-1)
{
}

/**
 * dTor.
 * @note Ruleterrain only holds mapblocks. Map datafiles are referenced.
 */
RuleTerrain::~RuleTerrain()
{
	for (std::vector<MapBlock*>::const_iterator
			i = _mapBlocks.begin();
			i != _mapBlocks.end();
			++i)
	{
		delete *i;
	}
}

/**
 * Loads the terrain from a YAML file.
 * @param node		- reference a YAML node
 * @param ruleset	- game's Ruleset
 */
void RuleTerrain::load(
		const YAML::Node& node,
		Ruleset* ruleset)
{
	if (const YAML::Node& map = node["mapDataSets"])
	{
		_mapDataSets.clear();

		for (YAML::const_iterator
				i = map.begin();
				i != map.end();
				++i)
		{
			_mapDataSets.push_back(ruleset->getMapDataSet(i->as<std::string>()));
		}
	}

	if (const YAML::Node& map = node["mapBlocks"])
	{
		_mapBlocks.clear();

		for (YAML::const_iterator
				i = map.begin();
				i != map.end();
				++i)
		{
			MapBlock* const mapBlock = new MapBlock((*i)["name"].as<std::string>());
			mapBlock->load(*i);
			_mapBlocks.push_back(mapBlock);
		}
	}

	_name				= node["name"]		.as<std::string>(_name);
	_textures			= node["textures"]	.as<std::vector<int> >(_textures);
	_hemisphere			= node["hemisphere"].as<int>(_hemisphere);

	if (const YAML::Node& civs = node["civilianTypes"])
		_civilianTypes = civs.as<std::vector<std::string> >(_civilianTypes);
	else
	{
		_civilianTypes.push_back("MALE_CIVILIAN");
		_civilianTypes.push_back("FEMALE_CIVILIAN");
	}

	_minDepth	= node["minDepth"]	.as<int>(_minDepth);
	_maxDepth	= node["maxDepth"]	.as<int>(_maxDepth);
	_ambience	= node["ambience"]	.as<int>(_ambience);
	_script		= node["script"]	.as<std::string>(_script);
}

/**
 * Gets the array of mapblocks.
 * @return, pointer to a vector of pointers as an array of MapBlocks
 */
std::vector<MapBlock*>* RuleTerrain::getMapBlocks()
{
	return &_mapBlocks;
}

/**
 * Gets the array of mapdatafiles.
 * @return, pointer to a vector of pointers as an array of MapDataSets
 */
std::vector<MapDataSet*>* RuleTerrain::getMapDataSets()
{
	return &_mapDataSets;
}

/**
 * Gets the terrain name.
 * @return, the terrain name
 */
std::string RuleTerrain::getName() const
{
	return _name;
}

/**
 * Gets a random mapblock within the given constraints.
 * @param maxSizeX	- the maximum X size of the mapblock
 * @param maxSizeY	- the maximum Y size of the mapblock
 * @param group		- the group Type
 * @param force		- true to enforce the max size (default false)
 * @return, pointer to a MapBlock or NULL if not found
 */
MapBlock* RuleTerrain::getRandomMapBlock(
		int maxSizeX,
		int maxSizeY,
		int group,
		bool force)
{
	std::vector<MapBlock*> compliantBlocks;

	for (std::vector<MapBlock*>::const_iterator
			i = _mapBlocks.begin();
			i != _mapBlocks.end();
			++i)
	{
		if (((*i)->getSizeX() == maxSizeX
				|| (force == false
					&& (*i)->getSizeX() < maxSizeX))
			&& ((*i)->getSizeY() == maxSizeY
				|| (force == false
					&& (*i)->getSizeY() < maxSizeY))
			&& (*i)->isInGroup(group) == true)
		{
			compliantBlocks.push_back((*i));
		}
	}

	if (compliantBlocks.empty() == true)
		return NULL;

	const size_t mapBlock = static_cast<size_t>(RNG::generate(
															0,
															static_cast<int>(compliantBlocks.size()) - 1));
	return compliantBlocks[mapBlock];
}

/**
 * Gets a MapBlock with a given name.
 * @param name - reference the name of a MapBlock
 * @return, pointer to a MapBlock or NULL if not found
 */
MapBlock* RuleTerrain::getMapBlock(const std::string& name)
{
	for (std::vector<MapBlock*>::const_iterator
			i = _mapBlocks.begin();
			i != _mapBlocks.end();
			++i)
	{
		if ((*i)->getName() == name)
			return (*i);
	}

	return NULL;
}

/**
 * Gets a mapdata object.
 * @param id			- pointer to the ID of the terrain
 * @param mapDataSetID	- pointer to the ID of the MapDataSet
 * @return, pointer to MapData object
 */
MapData* RuleTerrain::getMapData(
		unsigned int* id,
		int* mapDataSetID) const
{
	MapDataSet* dataSet = NULL;

	std::vector<MapDataSet*>::const_iterator i = _mapDataSets.begin();
	for (
			;
			i != _mapDataSets.end();
			++i)
	{
		dataSet = *i;

		if (*id < static_cast<int>(dataSet->getSize()))
			break;

		*id -= dataSet->getSize();
		++(*mapDataSetID);
	}

	if (i == _mapDataSets.end())
	{
		// oops! someone at microprose made an error in the map!
		// set this broken tile reference to BLANKS 0.
		dataSet = _mapDataSets.front();
		*id = 0;
		*mapDataSetID = 0;
	}

	return dataSet->getObjects()->at(*id);
}

/**
 * Gets the array of globe texture IDs this terrain is loaded for.
 * @return, pointer to the array of texture IDs
 */
std::vector<int>* RuleTerrain::getTextures()
{
	return &_textures;
}

/**
 * Gets the hemishpere this terrain occurs in:
 *	-1 = northern
 *	 0 = either
 *	 1 = southern.
 * @return, hemisphere
 */
int RuleTerrain::getHemisphere() const
{
	return _hemisphere;
}

/**
 * Gets the list of civilian types to use on this terrain.
 * @return, list of civilian types to use (default MALE_CIVILIAN and FEMALE_CIVILIAN)
 */
std::vector<std::string> RuleTerrain::getCivilianTypes() const
{
	return _civilianTypes;
}

/**
 * Gets the minimum depth.
 * @return, min depth
 */
const int RuleTerrain::getMinDepth() const
{
	return _minDepth;
}

/**
 * Gets the maximum depth.
 * @return, max depth
 */
const int RuleTerrain::getMaxDepth() const
{
	return _maxDepth;
}

/**
 * Gets The ambient sound effect.
 * @return, the ambient sound effect
 */
const int RuleTerrain::getAmbience() const
{
	return _ambience;
}

/**
 * Gets the generation script name.
 * @return, the name of the script to use
 */
const std::string& RuleTerrain::getScript() const
{
	return _script;
}

}
