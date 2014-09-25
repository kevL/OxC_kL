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

#define _USE_MATH_DEFINES

#include "Target.h"

#include <cmath>

#include "Craft.h"

#include "../Engine/Language.h"


namespace OpenXcom
{

/**
 * Initializes a target with blank coordinates.
 */
Target::Target()
	:
		_lon(0.0),
		_lat(0.0)
{
}

/**
 * Make sure no crafts are chasing this target.
 */
Target::~Target()
{
	for (size_t
			i = 0;
			i < _followers.size();
			++i)
	{
		Craft* craft = dynamic_cast<Craft*>(_followers[i]);
		if (craft)
			craft->returnToBase();
	}
}

/**
 * Loads the target from a YAML file.
 * @param node - YAML node
 */
void Target::load(const YAML::Node& node)
{
	_lon = node["lon"].as<double>(_lon);
	_lat = node["lat"].as<double>(_lat);
}

/**
 * Saves the target to a YAML file.
 * @return, YAML node
 */
YAML::Node Target::save() const
{
	YAML::Node node;

	node["lon"] = _lon;
	node["lat"] = _lat;

	return node;
}

/**
 * Saves the target's unique identifiers to a YAML file.
 * @return, YAML node
 */
YAML::Node Target::saveId() const
{
	YAML::Node node;

	node["lon"] = _lon;
	node["lat"] = _lat;

	return node;
}

/**
 * Returns the longitude coordinate of the target.
 * @return, longitude in radians
 */
double Target::getLongitude() const
{
	return _lon;
}

/**
 * Changes the longitude coordinate of the target.
 * @param lon - longitude in radians
 */
void Target::setLongitude(double lon)
{
	_lon = lon;

	// Keep between 0 and 2xPI
	while (_lon < 0.0)
		_lon += 2.0 * M_PI;

	while (_lon >= 2.0 * M_PI)
		_lon -= 2.0 * M_PI;
}

/**
 * Returns the latitude coordinate of the target.
 * @return, latitude in radians
 */
double Target::getLatitude() const
{
	return _lat;
}

/**
 * Changes the latitude coordinate of the target.
 * @param lat - latitude in radians
 */
void Target::setLatitude(double lat)
{
	_lat = lat;

	if (_lat < -(M_PI / 2.0)) // keep it between -pi/2 and pi/2
	{
		_lat = -M_PI - _lat; // kL
		setLongitude(_lon + M_PI);
	}
	else if (_lat > (M_PI / 2.0))
	{
		_lat = M_PI - _lat; // Doug. right!
		setLongitude(_lon - M_PI);
	}
}

/**
 * Returns the list of crafts currently following this target.
 * @return, pointer to a list of pointers to crafts
 */
std::vector<Target*>* Target::getFollowers()
{
	return &_followers;
}

/**
 * Returns the great circle distance to another target on the globe.
 * @param target - pointer to other target
 * @return, distance in radians
 */
double Target::getDistance(const Target* target) const
{
	return acos(
			cos(_lat)
				* cos(target->getLatitude())
				* cos(target->getLongitude() - _lon)
			+ sin(_lat)
				* sin(target->getLatitude()));
}

}
