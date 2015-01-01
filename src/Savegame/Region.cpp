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
 * @param rules - pointer to RuleRegion
 */
Region::Region(RuleRegion* rules)
	:
		_rules(rules),
		_activityRecent(-1),
		_activityRecentXCOM(-1)
{
	_activityAlien.push_back(0);
	_activityXcom.push_back(0);
}

/**
 * dTor.
 */
Region::~Region()
{
}

/**
 * Loads the region from a YAML file.
 * @param node - reference a YAML node
 */
void Region::load(const YAML::Node& node)
{
	_activityXcom		= node["activityXcom"].as< std::vector<int> >(_activityXcom);
	_activityAlien		= node["activityAlien"].as< std::vector<int> >(_activityAlien);
	_activityRecent		= node["activityRecent"].as<int>(_activityRecent);
	_activityRecentXCOM	= node["activityRecentXCOM"].as<int>(_activityRecentXCOM);
}

/**
 * Saves the region to a YAML file.
 * @return, YAML node
 */
YAML::Node Region::save() const
{
	YAML::Node node;

	node["type"]				= _rules->getType();
	node["activityXcom"]		= _activityXcom;
	node["activityAlien"]		= _activityAlien;
	node["activityRecent"]		= _activityRecent;
	node["activityRecentXCOM"]	= _activityRecentXCOM;

	return node;
}

/**
 * Returns the ruleset for the region's type.
 * @return, pointer to RuleRegion
 */
RuleRegion* Region::getRules() const
{
	return _rules;
}

/**
 * Gets the region's name.
 * @return, region name
 */
std::string Region::getType() const
{
	return _rules->getType();
}

/**
 * Adds to the region's xcom activity level.
 * @param activity - amount to add
 */
void Region::addActivityXcom(int activity)
{
	_activityXcom.back() += activity;
}

/**
 * Adds to the region's alien activity level.
 * @param activity - amount to add
 */
void Region::addActivityAlien(int activity)
{
	_activityAlien.back() += activity;
}

/**
 * Gets the region's xcom activity level.
 * @return, activity level
 */
std::vector<int>& Region::getActivityXcom()
{
	return _activityXcom;
}

/**
 * Gets the region's alien activity level.
 * @return, activity level
 */
std::vector<int>& Region::getActivityAlien()
{
	return _activityAlien;
}

/**
 * Stores last month's counters, starts new counters.
 */
void Region::newMonth()
{
	_activityAlien.push_back(0);
	_activityXcom.push_back(0);

	if (_activityAlien.size() > 12)
		_activityAlien.erase(_activityAlien.begin());

	if (_activityXcom.size() > 12)
		_activityXcom.erase(_activityXcom.begin());
}

/**
 * Handles recent aLien activity in this region for GraphsState blink.
 * @param activity	- true to reset the startcounter (default true)
 * @param graphs	- not sure lol (default false)
 * @return, true if there is activity
 */
bool Region::recentActivity(
		bool activity,
		bool graphs)
{
	if (activity)
		_activityRecent = 0;
	else if (_activityRecent != -1)
	{
		if (graphs)
			return true;
		else
		{
			++_activityRecent;

			if (_activityRecent == 24) // min: aLien bases show activity every 24 hrs.
				_activityRecent = -1;
		}
	}

	if (_activityRecent == -1)
		return false;

	return true;
}

/**
 * Handles recent XCOM activity in this region for GraphsState blink.
 * @param activity	- true to reset the startcounter (default true)
 * @param graphs	- not sure lol (default false)
 * @return, true if there is activity
 */
bool Region::recentActivityXCOM(
		bool activity,
		bool graphs)
{
	if (activity)
		_activityRecentXCOM = 0;
	else if (_activityRecentXCOM != -1)
	{
		if (graphs)
			return true;
		else
		{
			++_activityRecentXCOM;

			if (_activityRecentXCOM == 24) // min: aLien bases show activity every 24 hrs. /shrug
				_activityRecentXCOM = -1;
		}
	}

	if (_activityRecentXCOM == -1)
		return false;

	return true;
}

/**
 * Resets activity.
 */
void Region::resetActivity()
{
	_activityRecent = -1;
	_activityRecentXCOM = -1;
}

}
