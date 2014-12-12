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

//#define _USE_MATH_DEFINES

#include "MovingTarget.h"

//#include <cmath>

//#include "../fmath.h"

#include "../Geoscape/GeoscapeState.h"


namespace OpenXcom
{

/**
 * Initializes a moving target with blank coordinates.
 */
MovingTarget::MovingTarget()
	:
		Target(),
		_dest(NULL),
		_speedLon(0.),
		_speedLat(0.),
		_speedRadian(0.),
		_speed(0)
{
}

/**
 * Make sure to cleanup the target's destination followers.
 */
MovingTarget::~MovingTarget()
{
	if (_dest != NULL
		&& _dest->getFollowers()->empty() == false)
	{
		for (std::vector<Target*>::const_iterator
				i = _dest->getFollowers()->begin();
				i != _dest->getFollowers()->end();
				++i)
		{
			if (*i == this)
			{
				_dest->getFollowers()->erase(i);
				break;
			}
		}
	}
}

/**
 * Loads the moving target from a YAML file.
 * @param node - reference a YAML node
 */
void MovingTarget::load(const YAML::Node& node)
{
	Target::load(node);

	_speedLon		= node["speedLon"]		.as<double>(_speedLon);
	_speedLat		= node["speedLat"]		.as<double>(_speedLat);
	_speedRadian	= node["speedRadian"]	.as<double>(_speedRadian);
	_speed			= node["speed"]			.as<int>(_speed);
}

/**
 * Saves the moving target to a YAML file.
 * @return, YAML node
 */
YAML::Node MovingTarget::save() const
{
	YAML::Node node = Target::save();

	if (_dest != NULL)
		node["dest"]	= _dest->saveId();

	node["speedLon"]	= _speedLon;
	node["speedLat"]	= _speedLat;
	node["speedRadian"]	= _speedRadian;
	node["speed"]		= _speed;

	return node;
}

/**
 * Returns the destination the moving target is heading to.
 * @return, pointer to Target destination
 */
Target* MovingTarget::getDestination() const
{
	return _dest;
}

/**
 * Changes the destination the moving target is heading to.
 * @param dest - pointer to Target destination
 */
void MovingTarget::setDestination(Target* dest)
{
	if (_dest != NULL) // remove moving target from old destination's followers
	{
		for (std::vector<Target*>::const_iterator
				i = _dest->getFollowers()->begin();
				i != _dest->getFollowers()->end();
				++i)
		{
			if (*i == this)
			{
				_dest->getFollowers()->erase(i);
				break;
			}
		}
	}

	_dest = dest;

	if (_dest != NULL) // add moving target to new destination's followers
		_dest->getFollowers()->push_back(this);

	calculateSpeed();
}

/**
 * Returns the speed of the moving target.
 * @return, speed in knots
 */
int MovingTarget::getSpeed() const
{
	return _speed;
}

/**
 * Changes the speed of the moving target and converts it from standard
 * knots (nautical miles per hour) into radians per 5 in-game seconds.
 * @param speed - speed in knots
 */
void MovingTarget::setSpeed(const int speed)
{
	_speed = speed;

	// each nautical mile is 1/60th of a degree; each hour contains 720 5-seconds
//	_speedRadian = ceil(static_cast<double>(_speed) * (1.0 / 60.0) * (M_PI / 180.0) / 720.0);
	_speedRadian = static_cast<double>(_speed) * unitToRads / 720.;

	calculateSpeed();
}

/**
 * Calculates the speed vector based on the great circle
 * distance to destination and current raw speed.
 */
void MovingTarget::calculateSpeed()
{
	if (_dest != NULL)
	{
		const double
			dLon = std::sin(_dest->getLongitude() - _lon)
				 * std::cos(_dest->getLatitude()),
			dLat = std::cos(_lat)
				 * std::sin(_dest->getLatitude()) - std::sin(_lat)
				 * std::cos(_dest->getLatitude())
				 * std::cos(_dest->getLongitude() - _lon),
			length = std::sqrt((dLon * dLon) + (dLat * dLat));

		_speedLat = dLat / length * _speedRadian;
		_speedLon = dLon / length * _speedRadian / std::cos(_lat + _speedLat);

		// Check for invalid speeds when a division by zero occurs due to near-zero values
		if (_speedLon != _speedLon
			|| _speedLat != _speedLat)
		{
			_speedLon = 0.;
			_speedLat = 0.;
		}
	}
	else
	{
		_speedLon = 0.;
		_speedLat = 0.;
	}
}

/**
 * Checks if the moving target has reached its destination.
 * @return, true if it has
 */
bool MovingTarget::reachedDestination() const
{
	if (_dest == NULL)
		return false;

	return AreSame(_dest->getLongitude(), _lon)
		&& AreSame(_dest->getLatitude(), _lat);
}

/**
 * Executes a movement cycle for the moving target.
 */
void MovingTarget::moveTarget()
{
	calculateSpeed();

	if (_dest != NULL)
	{
		if (getDistance(_dest) > _speedRadian)
		{
			setLongitude(_lon + _speedLon);
			setLatitude(_lat + _speedLat);
		}
		else
		{
			setLongitude(_dest->getLongitude());
			setLatitude(_dest->getLatitude());
		}
	}
}

}
