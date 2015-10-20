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

#include "RuleTexture.h"

//#include "../Engine/RNG.h"

#include "../Savegame/Target.h"


namespace OpenXcom
{

/**
 * Initializes a globe Texture.
 * @param id - Texture identifier
 */
RuleTexture::RuleTexture(int id)
	:
		_id(id)
{}

/**
 * dTor.
 */
RuleTexture::~RuleTexture()
{}

/**
 * Loads a RuleTexture type from a YAML file.
 * @param node - reference a YAML node
 */
void RuleTexture::load(const YAML::Node& node)
{
	_id				= node["id"]			.as<int>(_id);
	_deployTypes	= node["deployTypes"]	.as<std::map<std::string, int> >(_deployTypes);
	_terrains		= node["terrains"]		.as<std::vector<TerrainCriteria> >(_terrains);
}

/**
 * Returns the list of TerrainCriteria associated with this RuleTexture.
 * @return, pointer to a vector of TerrainCriteria's
 */
std::vector<TerrainCriteria>* RuleTexture::getTerrainCriteria()
{
	return &_terrains;
}

/**
 * Returns the list of deployments associated with this Texture.
 * @return, reference to a map of deployments
 */
const std::map<std::string, int>& RuleTexture::getTextureDeployments()
{
	return _deployTypes;
}

/**
 * Calculates a random deployment for a mission target based on the texture's
 * available deployments.
 * @return, type of deployment
 */
std::string RuleTexture::getRandomDeployment() const
{
	if (_deployTypes.empty() == true)
		return "";

	std::map<std::string, int>::const_iterator i (_deployTypes.begin());
	if (_deployTypes.size() == 1)
		return i->first;

	int totalWeight (0);
	for (
			;
			i != _deployTypes.end();
			++i)
	{
		totalWeight += i->second;
	}

	if (totalWeight != 0)
	{
		int pick = RNG::generate(1, totalWeight);
		for (
				i = _deployTypes.begin();
				i != _deployTypes.end();
				++i)
		{
			if (pick <= i->second)
				return i->first;
			else
				pick -= i->second;
		}
	}
	return "";
}

/**
 * Calculates a random terrain for a mission-target based on this texture's
 * available TerrainCriteria.
 * @param target - pointer to the mission Target (default NULL to exclude geographical bounds)
 * @return, terrain string
 */
std::string RuleTexture::getRandomTerrain(const Target* const target) const
{
	Log(LOG_INFO) << "RuleTexture::getRandomTerrain(TARGET)";
	double
		lon,lat;
	std::map<int, std::string> eligibleTerrains;

	int totalWeight (0);
	for (std::vector<TerrainCriteria>::const_iterator
			i = _terrains.begin();
			i != _terrains.end();
			++i)
	{
		Log(LOG_INFO) << ". terrainType = " << i->type;
		if (i->weight != 0)
		{
			bool inBounds = false;
			if (target != NULL)
			{
				lon = target->getLongitude();
				lat = target->getLatitude();

				if (   lon >= i->lonMin
					&& lon <  i->lonMax
					&& lat >= i->latMin
					&& lat <  i->latMax)
				{
					inBounds = true;
				}
			}

			if (target == NULL || inBounds == true)
			{
				Log(LOG_INFO) << ". . weight = " << i->weight;
				totalWeight += i->weight;
				eligibleTerrains[totalWeight] = i->type;
			}
			else Log(LOG_INFO) << ". . target outside texture-detail's assigned geographical area";
		}
	}

	if (totalWeight != 0)
	{
		const int pick = RNG::generate(1, totalWeight);
		for (std::map<int, std::string>::const_iterator
				i = eligibleTerrains.begin();
				i != eligibleTerrains.end();
				++i)
		{
			if (pick <= i->first)
				return i->second;
		}
	}
	return ""; // this means nothing. Literally. That is, if the code runs here -> CTD.
}

}
