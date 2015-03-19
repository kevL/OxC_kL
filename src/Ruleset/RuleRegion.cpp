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

#include "RuleRegion.h"

//#include <assert.h>
//#define _USE_MATH_DEFINES
//#include <math.h>
//#include "../fmath.h"

#include "RuleCity.h"

//#include "../Engine/RNG.h"


namespace OpenXcom
{

/**
 * Creates a blank ruleset for a certain type of region.
 * @param type - reference the string defining the type
 */
RuleRegion::RuleRegion(const std::string& type)
	:
		_type(type),
		_buildCost(0),
		_regionWeight(0)
{}

/**
 * Deletes the regions from memory.
 */
RuleRegion::~RuleRegion()
{
	for (std::vector<RuleCity*>::const_iterator
			i = _cities.begin();
			i != _cities.end();
			++i)
	{
		delete *i;
	}
}

/**
 * Loads the region type from a YAML file.
 * @param node - reference a YAML node
 */
void RuleRegion::load(const YAML::Node& node)
{
	_type		= node["type"]		.as<std::string>(_type);
	_buildCost	= node["buildCost"]	.as<int>(_buildCost);

	std::vector<std::vector<double> > areas;
	areas = node["areas"].as<std::vector<std::vector<double> > >(areas);
	for (size_t
			i = 0;
			i != areas.size();
			++i)
	{
		_lonMin.push_back(areas[i][0] * M_PI / 180.);
		_lonMax.push_back(areas[i][1] * M_PI / 180.);
		_latMin.push_back(areas[i][2] * M_PI / 180.);
		_latMax.push_back(areas[i][3] * M_PI / 180.);
	}

	_missionZones = node["missionZones"].as<std::vector<MissionZone> >(_missionZones);

	// kL_begin:
	MissionArea area = *_missionZones.at(MZ_CITY).areas.begin();

	// Delete a possible placeholder in the Geography ruleset, by removing
	// its pointlike MissionArea at MissionZone[3] MZ_CITY; ie [0,0,0,0].
	// Note that safeties have been removed on all below_ ...
	if (area.isPoint() == true)
		_missionZones.at(MZ_CITY).areas.erase(_missionZones.at(MZ_CITY).areas.begin());

	if (const YAML::Node& cities = node["cities"])
	{
		for (YAML::const_iterator
				i = cities.begin();
				i != cities.end();
				++i)
		{
			RuleCity* const cityRule = new RuleCity(); // Load all Cities that are in YAML-ruleset
			cityRule->load(*i);
			_cities.push_back(cityRule);

			area.lonMin =
			area.lonMax = cityRule->getLongitude();
			area.latMin =
			area.latMax = cityRule->getLatitude();

			area.site = cityRule->getName();
			area.texture = cityRule->getCityTextureInt();

			_missionZones.at(MZ_CITY).areas.push_back(area);
		}
	} // end_kL.

	if (const YAML::Node& weights = node["missionWeights"])
		_missionWeights.load(weights);

	_regionWeight	= node["regionWeight"]	.as<size_t>(_regionWeight);
	_missionRegion	= node["missionRegion"]	.as<std::string>(_missionRegion);
}

/**
 * Gets the language string that names this region.
 * Each region type has a unique name.
 * @return, the region type
 */
std::string RuleRegion::getType() const
{
	return _type;
}

/**
 * Gets the cost of building a base inside this region.
 * @return, the construction cost
 */
int RuleRegion::getBaseCost() const
{
	return _buildCost;
}

/**
 * Checks if a point is inside this region.
 * @param lon - longitude in radians
 * @param lat - latitude in radians
 * @return, true if point is inside this region
 */
bool RuleRegion::insideRegion(
		double lon,
		double lat) const
{
	for (size_t
			i = 0;
			i < _lonMin.size();
			++i)
	{
		bool
			inLon,
			inLat;

		if (_lonMin[i] <= _lonMax[i])
			inLon = (lon >= _lonMin[i]
				  && lon < _lonMax[i]);
		else
			inLon = ((lon >= _lonMin[i]
						&& lon < 6.283)
					|| (lon >= 0.
						&& lon < _lonMax[i]));

		inLat = (lat >= _latMin[i]
			  && lat < _latMax[i]);

		if (inLon == true
			&& inLat == true)
		{
			return true;
		}
	}

	return false;
}

/**
 * Gets the list of cities contained in this region.
 * @note Build & cache a vector of all MissionAreas that are Cities.
 * @return, pointer to a vector of pointers to Cities
 */
std::vector<RuleCity*>* RuleRegion::getCities()
{
	if (_cities.empty() == true) // kL_note: unused for now. Just return the cities, thanks anyway.
	{
		for (std::vector<MissionZone>::const_iterator
				i = _missionZones.begin();
				i != _missionZones.end();
				++i)
		{
			for (std::vector<MissionArea>::const_iterator
					j = i->areas.begin();
					j != i->areas.end();
					++j)
			{
				if (j->isPoint() == true
					&& j->site.empty() == false)
				{
					_cities.push_back(new RuleCity(
											j->site,
											j->lonMin,
											j->latMin));
				}
			}
		}
	}

	return &_cities;
}

/**
 * Gets the weight of this region for mission selection.
 * This is only used when creating a new game since these weights change in the course of the game.
 * @return, the initial weight of this Region
 */
size_t RuleRegion::getWeight() const
{
	return _regionWeight;
}

/**
 * Gets a random point that is guaranteed to be inside the given zone.
 * @param zone - the target zone
 * @return, a pair of longitude and latitude
 */
std::pair<double, double> RuleRegion::getRandomPoint(size_t zone) const
{
	if (zone < _missionZones.size())
	{
		const size_t area = RNG::generate(
										0,
										static_cast<int>(_missionZones[zone].areas.size()) - 1);

		double
			lonMin = _missionZones[zone].areas[area].lonMin,
			lonMax = _missionZones[zone].areas[area].lonMax,
			latMin = _missionZones[zone].areas[area].latMin,
			latMax = _missionZones[zone].areas[area].latMax;

		if (lonMin > lonMax)
		{
			lonMin = _missionZones[zone].areas[area].lonMax;
			lonMax = _missionZones[zone].areas[area].lonMin;
		}

		if (latMin > latMax)
		{
			latMin = _missionZones[zone].areas[area].latMax;
			latMax = _missionZones[zone].areas[area].latMin;
		}

		const double
			lon = RNG::generate(lonMin, lonMax),
			lat = RNG::generate(latMin, latMax);

		return std::make_pair(
							lon,
							lat);
	}

	assert(0 && "Invalid zone number");
	return std::make_pair(0.,0.);
}

/**
 * Gets the area data for the mission point in the specified zone and coordinates.
 * @param zone		- the target zone
 * @param target	- the target coordinates
 * @return, a pair of longitude and latitude
 */
MissionArea RuleRegion::getMissionPoint(
		size_t zone,
		Target* target) const
{
	if (zone < _missionZones.size())
	{
		for (std::vector<MissionArea>::const_iterator
				i = _missionZones[zone].areas.begin();
				i != _missionZones[zone].areas.end();
				++i)
		{
			if (i->isPoint() == true
				&& AreSame(target->getLongitude(), i->lonMin)
				&& AreSame(target->getLatitude(), i->latMin))
			{
				return *i;
			}
		}
	}

	assert(0 && "Invalid zone number");
	return MissionArea();
}

/**
 * Gets the mission zones.
 * @return, reference to a vector of MissionZones
 */
const std::vector<MissionZone>& RuleRegion::getMissionZones() const
{
	return _missionZones;
}

}
