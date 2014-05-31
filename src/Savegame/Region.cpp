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

#include "Region.h"

#include "../Ruleset/RuleRegion.h"


namespace OpenXcom
{

/**
 * Initializes a region of the specified type.
 * @param rules Pointer to ruleset.
 */
Region::Region(RuleRegion* rules)
	:
		_rules(rules),
		_activityRecent(-1) // kL
{
	_activityAlien.push_back(0);
	_activityXcom.push_back(0);
}

/**
 *
 */
Region::~Region()
{
}

/**
 * Loads the region from a YAML file.
 * @param node YAML node.
 */
void Region::load(const YAML::Node& node)
{
	_activityXcom	= node["activityXcom"].as< std::vector<int> >(_activityXcom);
	_activityAlien	= node["activityAlien"].as< std::vector<int> >(_activityAlien);

	_activityRecent	= node["activityRecent"].as<int>(_activityRecent); // kL
}

/**
 * Saves the region to a YAML file.
 * @return YAML node.
 */
YAML::Node Region::save() const
{
	YAML::Node node;

	node["type"]			= _rules->getType();
	node["activityXcom"]	= _activityXcom;
	node["activityAlien"]	= _activityAlien;
	node["activityRecent"]	= _activityRecent; // kL

	return node;
}

/**
 * Returns the ruleset for the region's type.
 * @return, Pointer to ruleset.
 */
RuleRegion* Region::getRules() const
{
	return _rules;
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
 * @return, activity level.
 */
const std::vector<int>& Region::getActivityXcom() const
{
	return _activityXcom;
}

/**
 * Gets the region's alien activity level.
 * @return, activity level.
 */
const std::vector<int>& Region::getActivityAlien() const
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
 * kL. Handles recent alien activity in this region for GraphsState blink.
 */
bool Region::recentActivity( // kL
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

			if (_activityRecent == 24)
				_activityRecent = -1;
		}
	}

	if (_activityRecent == -1)
		return false;

	return true;
}

/**
 * kL. Sets recent alien activity in this region.
 */
/* void Region::setRecentActivity(bool activity) // kL
{
	_activityRecent = activity;
} */

/**
 * kL. Gets recent alien activity in this region.
 */
/* bool Region::getRecentActivity() const // kL
{
	return _activityRecent;
} */

/**
 * kL. Sets last alien activity in this region.
 */
/* void Region::setLastActivity(int activity) // kL
{
	_activityLast = activity;
} */

/**
 * kL. Gets last alien activity in this region.
 */
/* int Region::getLastActivity() const // kL
{
	return _activityLast;
} */

}
