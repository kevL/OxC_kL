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

#include "AlienDeployment.h"

#include "../Engine/RNG.h"


namespace YAML
{

template<>
struct convert<OpenXcom::DeploymentData>
{
	///
	static Node encode(const OpenXcom::DeploymentData& rhs)
	{
		Node node;

		node["alienRank"]				= rhs.alienRank;
		node["lowQty"]					= rhs.lowQty;
		node["highQty"]					= rhs.highQty;
		node["dQty"]					= rhs.dQty;
		node["extraQty"]				= rhs.extraQty;
		node["percentageOutsideUfo"]	= rhs.percentageOutsideUfo;
		node["itemSets"]				= rhs.itemSets;

		return node;
	}

	///
	static bool decode(
			const Node& node,
			OpenXcom::DeploymentData& rhs)
	{
		if (node.IsMap() == false)
			return false;

		rhs.alienRank				= node["alienRank"]				.as<int>(rhs.alienRank);
		rhs.lowQty					= node["lowQty"]				.as<int>(rhs.lowQty);
		rhs.highQty					= node["highQty"]				.as<int>(rhs.highQty);
		rhs.dQty					= node["dQty"]					.as<int>(rhs.dQty);
		rhs.extraQty				= node["extraQty"]				.as<int>(0); // give this a default, as it's not 100% needed, unlike the others.
		rhs.percentageOutsideUfo	= node["percentageOutsideUfo"]	.as<int>(rhs.percentageOutsideUfo);
		rhs.itemSets				= node["itemSets"]				.as< std::vector<OpenXcom::ItemSet> >(rhs.itemSets);

		return true;
	}
};

}


namespace OpenXcom
{

/**
 * Creates a blank ruleset for a certain type of deployment data.
 * @param type - reference a string defining the type of AlienMission
 */
AlienDeployment::AlienDeployment(const std::string& type)
	:
		_type(type),
		_width(0),
		_length(0),
		_height(0),
		_civilians(0),
		_shade(-1)
{
}

/**
 * dTor.
 */
AlienDeployment::~AlienDeployment()
{
}

/**
 * Loads the Deployment from a YAML file.
 * @param node - reference a YAML node
 */
void AlienDeployment::load(const YAML::Node& node)
{
	_type			= node["type"]			.as<std::string>(_type);
	_data			= node["data"]			.as< std::vector<DeploymentData> >(_data);
	_width			= node["width"]			.as<int>(_width);
	_length			= node["length"]		.as<int>(_length);
	_height			= node["height"]		.as<int>(_height);
	_civilians		= node["civilians"]		.as<int>(_civilians);
	_terrains		= node["terrains"]		.as<std::vector<std::string> >(_terrains);
	_shade			= node["shade"]			.as<int>(_shade);
	_nextStage		= node["nextStage"]		.as<std::string>(_nextStage);
	_race			= node["race"]			.as<std::string>(_race);
	_script			= node["script"]		.as<std::string>(_script);
}

/**
 * Returns the language string that names this deployment.
 * Each deployment type has a unique name.
 * @return, deployment name
 */
std::string AlienDeployment::getType() const
{
	return _type;
}

/**
 * Gets a pointer to the data.
 * @return, pointer to the data
 */
std::vector<DeploymentData>* AlienDeployment::getDeploymentData()
{
	return &_data;
}

/**
 * Gets dimensions.
 * @param width		- pointer to width
 * @param length 	- pointer to length
 * @param height	- pointer to height
 */
void AlienDeployment::getDimensions(
		int* width,
		int* length,
		int* height)
{
	*width	= _width;
	*length	= _length;
	*height	= _height;
}

/**
 * Gets the number of civilians.
 * @return, the number of civilians
 */
int AlienDeployment::getCivilians() const
{
	return _civilians;
}

/**
 * Gets the terrain for battlescape generation.
 * kL_note: See note in header file.
 * @return, the terrain
 */
/* std::string AlienDeployment::getTerrain() const
{
	if (!_terrains.empty())
	{
		unsigned terrain = RNG::generate(
										0,
										_terrains.size() - 1);
		return _terrains.at(terrain);
	}
	return "";
} */

/**
 * Gets the terrains for battlescape generation.
 * @return, vector of the terrain strings
 */
std::vector<std::string> AlienDeployment::getTerrains() const
{
	return _terrains;
}

/**
 * Gets the shade level for battlescape generation.
 * @return, the shade level
 */
int AlienDeployment::getShade() const
{
	return _shade;
}

/**
 * Gets the next stage of the mission.
 * @return, the next stage of the mission
 */
std::string AlienDeployment::getNextStage() const
{
	return _nextStage;
}

/**
 * Gets the next stage's aLien race.
 * @return, the next stage's alien race
 */
std::string AlienDeployment::getRace() const
{
	return _race;
}

/**
 * Gets the script to use to generate a mission of this type.
 * @return, the script to use to generate a mission of this type
 */
std::string AlienDeployment::getScript() const
{
	return _script;
}

}
