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

#include "AlienStrategy.h"

//#include <assert.h>

#include "WeightedOptions.h"

#include "../Ruleset/RuleRegion.h"
#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

typedef std::map<std::string, WeightedOptions*> MissionsByRegion;


/**
 * Creates an AlienStrategy with no values.
 * @note Running a game like this will most likely crash.
 */
AlienStrategy::AlienStrategy()
{}

/**
 * Frees all resources used by this AlienStrategy.
 */
AlienStrategy::~AlienStrategy()
{
	for (MissionsByRegion::iterator
			i = _regionMissions.begin();
			i != _regionMissions.end();
			++i)
	{
		delete i->second;
	}
}

/**
 * Gets starting values from the rules.
 * @param rules - pointer to the game Ruleset
 */
void AlienStrategy::init(const Ruleset* const rules)
{
	const std::vector<std::string> regions = rules->getRegionsList();
	for (std::vector<std::string>::const_iterator
			i = regions.begin();
			i != regions.end();
			++i)
	{
		const RuleRegion* const region = rules->getRegion(*i);
		_regionChances.setWeight(
							*i,
							region->getWeight());

		WeightedOptions* const missions = new WeightedOptions(region->getAvailableMissions());
		_regionMissions.insert(std::make_pair(
											*i,
											missions));
	}
}

/**
 * Loads the data from a YAML file.
 * @param node - reference a YAML node
 */
void AlienStrategy::load(const YAML::Node& node)
{
	for (MissionsByRegion::iterator
			i = _regionMissions.begin();
			i != _regionMissions.end();
			++i)
	{
		delete i->second;
	}

	_regionMissions.clear();
	_regionChances.clearWeights();
	_regionChances.load(node["regions"]);

	const YAML::Node& strat = node["possibleMissions"];

	for (YAML::const_iterator
			i = strat.begin();
			i != strat.end();
			++i)
	{
		const std::string region	= (*i)["region"].as<std::string>();
		const YAML::Node& missions	= (*i)["missions"];

		std::auto_ptr<WeightedOptions> options(new WeightedOptions());
		options->load(missions);
		_regionMissions.insert(std::make_pair(
											region,
											options.release()));
	}

	_missionLocations	= node["missionLocations"]	.as<std::map<std::string, std::vector<std::pair<std::string, size_t> > > >(_missionLocations);
	_missionRuns		= node["missionsRun"]		.as<std::map<std::string, int> >(_missionRuns);
}

/**
 * Saves the alien data to a YAML file.
 * @return, YAML node
 */
YAML::Node AlienStrategy::save() const
{
	YAML::Node node;

	node["regions"] = _regionChances.save();

	for (MissionsByRegion::const_iterator
			i = _regionMissions.begin();
			i != _regionMissions.end();
			++i)
	{
		YAML::Node subnode;

		subnode["region"]	= i->first;
		subnode["missions"]	= i->second->save();

		node["possibleMissions"].push_back(subnode);
	}

	node["missionLocations"]	= _missionLocations;
	node["missionsRun"]			= _missionRuns;

	return node;
}

/**
 * Chooses one of the regions for a mission.
 * @param rules - pointer to the Ruleset
 * @return, the region id
 */
std::string AlienStrategy::chooseRandomRegion(const Ruleset* const rules)
{
	std::string chosen (_regionChances.getOptionResult());
	if (chosen.empty() == true)
	{
		// no more missions to choose from, refresh
		// First free allocated memory.
		for (MissionsByRegion::const_iterator
				i = _regionMissions.begin();
				i != _regionMissions.end();
				++i)
		{
			delete i->second;
		}

		_regionMissions.clear();

		// re-initialize the list
 		init(rules);
		// now try that again:
 		chosen = _regionChances.getOptionResult();
	}

	assert(chosen != "");

	return chosen;
}

/**
 * Chooses one of the missions available for @a region.
 * @param region - reference the region id
 * @return, the mission id
 */
std::string AlienStrategy::chooseRandomMission(const std::string& region) const
{
	MissionsByRegion::const_iterator found (_regionMissions.find(region));
	assert(found != _regionMissions.end());

	return found->second->getOptionResult();
}

/**
 * Removes @a mission from the list of possible missions for @a region.
 * @param region	- the region id
 * @param mission	- the mission id
 * @return, true if there are no more regions with missions available
 */
bool AlienStrategy::removeMission(
		const std::string& region,
		const std::string& mission)
{
	MissionsByRegion::const_iterator found (_regionMissions.find(region));
	if (found != _regionMissions.end())
	{
		found->second->setWeight(mission, 0);

//		if (found->second->empty() == true)
		if (found->second->hasNoWeight() == true)
		{
			_regionMissions.erase(found);
			_regionChances.setWeight(region, 0);
		}
	}

	return (_regionMissions.empty() == true);
}

/**
 * Checks the number of missions labelled 'id' that have run.
 * @return, the number of missions already run
 */
int AlienStrategy::getMissionsRun(const std::string& id)
{
	if (_missionRuns.find(id) != _missionRuns.end())
		return _missionRuns[id];

	return 0;
}

/**
 * Increments the number of missions labelled 'id' that have run.
 * @param id - the variable name to use to keep track of runs
 */
void AlienStrategy::addMissionRun(const std::string& id)
{
	if (id.empty() == true)
		return;

	++_missionRuns[id];
}

/**
 * Adds a mission location to the storage array.
 * @param id		- name of the variable under which to store this info
 * @param region	- name of the region
 * @param zone		- number of the zone within the region
 * @param track 	- maximum size of the list to maintain
 */
void AlienStrategy::addMissionLocation(
		const std::string& id,
		const std::string& region,
		size_t zone,
		size_t track)
{
	if (track == 0)
		return;

	_missionLocations[id].push_back(std::make_pair(region, zone));
	if (_missionLocations[id].size() > track)
		_missionLocations.erase(_missionLocations.begin());
}

/**
 * Checks that a given mission location (city or whatever) isn't stored in the
 * list of previously attacked locations.
 * @param id		- name of the variable under which this info is stored
 * @param region	- name of the region
 * @param zone		- number of the region to check
 * @return, true if the region is valid (it's not in the table)
 */
bool AlienStrategy::validMissionLocation(
		const std::string& id,
		const std::string& region,
		size_t zone)
{
	if (_missionLocations.find(id) != _missionLocations.end())
	{
		for (std::vector<std::pair<std::string, size_t> >::const_iterator
				i = _missionLocations[id].begin();
				i != _missionLocations[id].end();
				++i)
		{
			if ((*i).first == region
				&& (*i).second == zone)
			{
				return false;
			}
		}
	}

	return true;
}
/**
 * Checks that a given region appears in the strategy table.
 * @param region - region to check for validity
 * @return, true if the region appears in the table
 */
bool AlienStrategy::validMissionRegion(const std::string& region) const
{
	return (_regionMissions.find(region) != _regionMissions.end());
}

}
