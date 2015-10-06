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
 * Creates a blank ruleset for a certain type of Region.
 * @param type - reference the string defining the type
 */
RuleRegion::RuleRegion(const std::string& type)
	:
		_type(type),
		_buildCost(0),
		_regionWeight(0)
{}

/**
 * Deletes the Region from memory.
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
 * Loads the Region type from a YAML file.
 * @param node - reference a YAML node
 */
void RuleRegion::load(const YAML::Node& node)
{
	_type		= node["type"]		.as<std::string>(_type);
	_buildCost	= node["buildCost"]	.as<int>(_buildCost);

	std::vector<std::vector<double> > areas (node["areas"].as<std::vector<std::vector<double> > >());
	for (size_t
			i = 0;
			i != areas.size();
			++i)
	{
		_lonMin.push_back(areas[i][0] * M_PI / 180.);
		_lonMax.push_back(areas[i][1] * M_PI / 180.);
		_latMin.push_back(areas[i][2] * M_PI / 180.);
		_latMax.push_back(areas[i][3] * M_PI / 180.);

		// safeties ->
//		if (_lonMin.back() > _lonMax.back())
//			std::swap(_lonMin.back(), _lonMax.back());
//		if (_latMin.back() > _latMax.back())
//			std::swap(_latMin.back(), _latMax.back());
	}

	// TODO: if ["delete"] delete previous mission zones.
	// NOTE: the next line replaces previous missionZones:
	_missionZones = node["missionZones"].as<std::vector<MissionZone> >(_missionZones);
	// NOTE: while the following lines add to missionZones:
//	std::vector<MissionZone> misZones = node["missionZones"].as<std::vector<MissionZone> >(misZones);
//	_missionZones.insert(
//					_missionZones.end(),
//					misZones.begin(),
//					misZones.end());
	// revert that until it gets worked out. FalcoOXC says,
	/* So if two mods add missionZones how do the trajectory
	references and negative texture entries remain in sync? */

	// kL_begin:
	MissionArea area (*_missionZones.at(MZ_CITY).areas.begin());

	// Delete a possible placeholder in the Geography ruleset, by removing
	// its pointlike MissionArea at MissionZone[3] MZ_CITY; ie [0,0,0,0].
	// Note that safeties have been removed on all below_ ...
	if (area.isPoint() == true)
		_missionZones.at(MZ_CITY).areas.erase(_missionZones.at(MZ_CITY).areas.begin());

	if (const YAML::Node& cities = node["cities"])
	{
		for (YAML::const_iterator // load all Cities that are in YAML-ruleset
				i = cities.begin();
				i != cities.end();
				++i)
		{
			RuleCity* const city (new RuleCity());
			city->load(*i);
			_cities.push_back(city);

			area.lonMin =
			area.lonMax = city->getLongitude();
			area.latMin =
			area.latMax = city->getLatitude();

			area.site = city->getName();
			area.texture = city->getTextureInt();

			_missionZones.at(MZ_CITY).areas.push_back(area);
		}
	} // end_kL.

	if (const YAML::Node& weights = node["missionWeights"])
		_missionWeights.load(weights);

	_regionWeight	= node["regionWeight"]	.as<size_t>(_regionWeight);
	_missionRegion	= node["missionRegion"]	.as<std::string>(_missionRegion);
}

/**
 * Gets the language string that names this Region.
 * @note Each Region type has a unique name.
 * @return, the region type
 */
const std::string& RuleRegion::getType() const
{
	return _type;
}

/**
 * Gets the cost of building a base inside this Region.
 * @return, the construction cost
 */
int RuleRegion::getBaseCost() const
{
	return _buildCost;
}

/**
 * Checks if a point is inside this Region.
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
			i != _lonMin.size();
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
						&& lon < M_PI * 2.)
				  || (lon >= 0.
						&& lon < _lonMax[i]));

		inLat = (lat >= _latMin[i]
			  && lat < _latMax[i]);

		if (inLon == true && inLat == true)
			return true;
	}

	return false;
}

/**
 * Gets the list of cities contained in this Region.
 * @note Build & cache a vector of all MissionAreas that are Cities.
 * @return, pointer to a vector of pointers to Cities
 */
std::vector<RuleCity*>* RuleRegion::getCities()
{
	if (_cities.empty() == true) // kL_note: unused for now. Just return the cities, thanks anyway.
		for (std::vector<MissionZone>::const_iterator
				i = _missionZones.begin();
				i != _missionZones.end();
				++i)
			for (std::vector<MissionArea>::const_iterator
					j = i->areas.begin();
					j != i->areas.end();
					++j)
				if (j->isPoint() == true && j->site.empty() == false)
					_cities.push_back(new RuleCity(
												j->site,
												j->lonMin,
												j->latMin));
	return &_cities;
}

/**
 * Gets the weight of the Region for mission selection.
 * @note This is only used when creating a new game since these weights change
 * in the course of the game.
 * @return, the initial weight of this Region
 */
size_t RuleRegion::getWeight() const
{
	return _regionWeight;
}

/**
 * Gets a list of all MissionZones in the Region.
 * @return, reference to a vector of MissionZones
 */
const std::vector<MissionZone>& RuleRegion::getMissionZones() const
{
	return _missionZones;
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
/*		double
			lonMin = _missionZones[zone].areas[pick].lonMin,
			lonMax = _missionZones[zone].areas[pick].lonMax,
			latMin = _missionZones[zone].areas[pick].latMin,
			latMax = _missionZones[zone].areas[pick].latMax;
		if (lonMin > lonMax) // safeties.
		{
			lonMin = _missionZones[zone].areas[pick].lonMax;
			lonMax = _missionZones[zone].areas[pick].lonMin;
		}
		if (latMin > latMax)
		{
			latMin = _missionZones[zone].areas[pick].latMax;
			latMax = _missionZones[zone].areas[pick].latMin;
		}
		const double
			lon = RNG::generate(lonMin, lonMax),
			lat = RNG::generate(latMin, latMax); */

		const size_t pick (RNG::pick(_missionZones[zone].areas.size()));
		const double
			lon = RNG::generate(
							_missionZones[zone].areas[pick].lonMin,
							_missionZones[zone].areas[pick].lonMax),
			lat = RNG::generate(
							_missionZones[zone].areas[pick].latMin,
							_missionZones[zone].areas[pick].latMax);

		return std::make_pair(lon,lat);
	}

//	assert(0 && "Invalid zone number");
	return std::make_pair(0.,0.);
}

/**
 * Gets the area data for the mission point in the specified zone and coordinates.
 * @param zone		- the target zone
 * @param target	- the target coordinates
 * @return, a MissionArea from which to extract coordinates, textures, or any other pertinent information
 */
MissionArea RuleRegion::getMissionPoint(
		size_t zone,
		const Target* const target) const
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

//	assert(0 && "Invalid zone number");
	return MissionArea();
}

/**
 * Gets the area data for the random mission point in this Region.
 * @return, a MissionArea from which to extract coordinates, textures, or any other pertinent information
 */
MissionArea RuleRegion::getRandomMissionPoint(size_t zone) const
{
	if (zone < _missionZones.size())
	{
		std::vector<MissionArea> randArea = _missionZones[zone].areas;
		for (std::vector<MissionArea>::const_iterator
				i = randArea.begin();
				i != randArea.end();
				)
		{
			if (i->isPoint() == false)
				i = randArea.erase(i);
			else
				++i;
		}

		if (randArea.empty() == false)
			return randArea.at(static_cast<size_t>(RNG::generate(0,
							   static_cast<int>(randArea.size()) - 1)));

	}

//	assert(0 && "Invalid zone number");
	return MissionArea();
}

}
