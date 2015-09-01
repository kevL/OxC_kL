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

#include "Region.h"

#include "../Ruleset/RuleRegion.h"


namespace OpenXcom
{

/**
 * Initializes a region of the specified type.
 * @param regionRule - pointer to RuleRegion
 */
Region::Region(RuleRegion* const regionRule)
	:
		_regionRule(regionRule),
		_recentActA(-1),
		_recentActX(-1)
{
	_actA.push_back(0);
	_actX.push_back(0);
}

/**
 * dTor.
 */
Region::~Region()
{}

/**
 * Loads the region from a YAML file.
 * @param node - reference a YAML node
 */
void Region::load(const YAML::Node& node)
{
	_actA		= node["actA"]		.as<std::vector<int> >(_actA);
	_actX		= node["actX"]		.as<std::vector<int> >(_actX);
	_recentActA	= node["recentActA"].as<int>(_recentActA);
	_recentActX	= node["recentActX"].as<int>(_recentActX);
}

/**
 * Saves the region to a YAML file.
 * @return, YAML node
 */
YAML::Node Region::save() const
{
	YAML::Node node;

	node["type"]		= _regionRule->getType();
	node["actA"]		= _actA;
	node["actX"]		= _actX;
	node["recentActA"]	= _recentActA;
	node["recentActX"]	= _recentActX;

	return node;
}

/**
 * Returns the ruleset for the region's type.
 * @return, pointer to RuleRegion
 */
RuleRegion* Region::getRules() const
{
	return _regionRule;
}

/**
 * Gets the region's name.
 * @return, region name
 */
std::string Region::getType() const
{
	return _regionRule->getType();
}

/**
 * Adds to the region's alien activity level.
 * @param activity - amount to add
 */
void Region::addActivityAlien(int activity)
{
	_actA.back() += activity;
}

/**
 * Adds to the region's xcom activity level.
 * @param activity - amount to add
 */
void Region::addActivityXCom(int activity)
{
	_actX.back() += activity;
}

/**
 * Gets the region's alien activity level.
 * @return, activity level
 */
std::vector<int>& Region::getActivityAlien()
{
	return _actA;
}

/**
 * Gets the region's xcom activity level.
 * @return, activity level
 */
std::vector<int>& Region::getActivityXCom()
{
	return _actX;
}

/**
 * Stores last month's counters, starts new counters.
 */
void Region::newMonth()
{
	_actA.push_back(0);
	_actX.push_back(0);

	if (_actA.size() > 12)
		_actA.erase(_actA.begin());

	if (_actX.size() > 12)
		_actX.erase(_actX.begin());
}

/**
 * Handles recent aLien activity in this region for GraphsState blink.
 * @param activity	- true to reset the startcounter (default true)
 * @param graphs	- not sure lol (default false)
 * @return, true if there is activity
 */
bool Region::recentActivityAlien(
		bool activity,
		bool graphs)
{
	if (activity == true)
		_recentActA = 0;
	else if (_recentActA != -1)
	{
		if (graphs == true)
			return true;
		else
		{
			++_recentActA;

			if (_recentActA == 24) // min: aLien bases show activity every 24 hrs.
				_recentActA = -1;
		}
	}

	if (_recentActA == -1)
		return false;

	return true;
}

/**
 * Handles recent XCOM activity in this region for GraphsState blink.
 * @param activity	- true to reset the startcounter (default true)
 * @param graphs	- not sure lol (default false)
 * @return, true if there is activity
 */
bool Region::recentActivityXCom(
		bool activity,
		bool graphs)
{
	if (activity == true)
		_recentActX = 0;
	else if (_recentActX != -1)
	{
		if (graphs == true)
			return true;
		else
		{
			++_recentActX;

			if (_recentActX == 24) // min: aLien bases show activity every 24 hrs. /shrug
				_recentActX = -1;
		}
	}

	if (_recentActX == -1)
		return false;

	return true;
}

/**
 * Resets activity.
 */
void Region::resetActivity()
{
	_recentActA =
	_recentActX = -1;
}

}
