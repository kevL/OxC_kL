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

#include "Texture.h"

//#include "../Engine/RNG.h"

#include "../Savegame/Target.h"


namespace OpenXcom
{

/**
 * Initializes a globe Texture.
 * @param id - Texture identifier
 */
Texture::Texture(int id)
	:
		_id(id)
{}

/**
 * dTor.
 */
Texture::~Texture()
{}

/**
 * Loads a Texture type from a YAML file.
 * @param node - reference a YAML node
 */
void Texture::load(const YAML::Node& node)
{
	_id			= node["id"]		.as<int>(_id);
	_deployment	= node["deployment"].as<std::string>(_deployment);
	_terrain	= node["terrain"]	.as< std::vector<TerrainCriteria> >(_terrain);
}

/**
 * Returns the list of TerrainCriteria associated with this Texture.
 * @return, pointer to a vector of TerrainCriteria's
 */
std::vector<TerrainCriteria>* Texture::getTerrain()
{
	return &_terrain;
}

/**
 * Calculates a random terrain for a mission-target based
 * on this texture's available TerrainCriteria.
 * @param target - pointer to the mission Target
 * @return, terrain string
 */
std::string Texture::getRandomTerrain(const Target* const target) const
{
	int totalWeight = 0;

	std::map<int, std::string> eligibleTerrains;
	for (std::vector<TerrainCriteria>::const_iterator
			i = _terrain.begin();
			i != _terrain.end();
			++i)
	{
		if (i->weight > 0
			&& target->getLongitude() >= i->lonMin
			&& target->getLongitude() < i->lonMax
			&& target->getLatitude() >= i->latMin
			&& target->getLatitude() < i->latMax)
		{
			totalWeight += i->weight;
			eligibleTerrains[totalWeight] = i->name;
		}
	}

	const int pick = RNG::generate(
								1,
								totalWeight);
	for (std::map<int, std::string>::const_iterator
			i = eligibleTerrains.begin();
			i != eligibleTerrains.end();
			++i)
	{
		if (pick <= i->first)
			return i->second;
	}

	return "";
}

/**
 * Gets this Texture's alien deployment.
 * @return, deployment-type string
 */
std::string Texture::getDeployment() const
{
	return _deployment;
}

}
