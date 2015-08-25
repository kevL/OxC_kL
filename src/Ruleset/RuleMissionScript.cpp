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

#include "RuleMissionScript.h"

//#include "../Engine/Exception.h"


namespace OpenXcom
{

/**
 * RuleMissionScript. The rules for Alien Mission propagation.
 * @note Each script element is independent and the saved game will probe the
 * list of these each month to determine what's going to happen.
 * @param type - reference the type ID
 */
RuleMissionScript::RuleMissionScript(const std::string& type)
	:
		_type(type),
		_firstMonth(0),
		_lastMonth(-1),
		_label(0),
		_executionOdds(100),
		_targetBaseOdds(0),
		_minDifficulty(0),
		_maxRuns(-1),
		_avoidRepeats(0),
		_delay(0),
		_useTable(true),
		_siteType(false)
{}

/**
 * Destructor.
 * @note Cleans up the mess Warboy left in RAM. oh yah ->
 */
RuleMissionScript::~RuleMissionScript()
{
	for (std::vector<std::pair<size_t, WeightedOptions*> >::const_iterator
			i = _missionWeights.begin();
			i != _missionWeights.end();
			++i)
	{
		delete i->second;
		i = _missionWeights.erase(i);
	}

	for (std::vector<std::pair<size_t, WeightedOptions*> >::const_iterator
			i = _raceWeights.begin();
			i != _raceWeights.end();
			++i)
	{
		delete i->second;
		i = _raceWeights.erase(i);
	}

	for (std::vector<std::pair<size_t, WeightedOptions*> >::const_iterator
			i = _regionWeights.begin();
			i != _regionWeights.end();
			++i)
	{
		delete i->second;
		i = _regionWeights.erase(i);
	}
}

/**
 * Loads a missionScript from a YAML file.
 * @param node - reference a YAML node
 */
void RuleMissionScript::load(const YAML::Node& node)
{
	_varName		= node["varName"]		.as<std::string>(_varName);
	_firstMonth		= node["firstMonth"]	.as<int>(_firstMonth);
	_lastMonth		= node["lastMonth"]		.as<int>(_lastMonth);
	_label			= node["label"]			.as<unsigned int>(_label);
	_executionOdds	= node["executionOdds"]	.as<int>(_executionOdds);
	_targetBaseOdds	= node["targetBaseOdds"].as<int>(_targetBaseOdds);
	_minDifficulty	= node["minDifficulty"]	.as<int>(_minDifficulty);
	_maxRuns		= node["maxRuns"]		.as<int>(_maxRuns);
	_avoidRepeats	= node["avoidRepeats"]	.as<int>(_avoidRepeats);
	_delay			= node["startDelay"]	.as<int>(_delay);
	_conditionals	= node["conditionals"]	.as<std::vector<int> >(_conditionals);

	if (const YAML::Node& weights = node["missionWeights"])
	{
		for (YAML::const_iterator
				i = weights.begin();
				i != weights.end();
				++i)
		{
			WeightedOptions* j = new WeightedOptions();
			j->load(i->second);
			_missionWeights.push_back(std::make_pair(
												i->first.as<size_t>(0),
												j));
		}
	}

	if (const YAML::Node& weights = node["raceWeights"])
	{
		for (YAML::const_iterator
				i = weights.begin();
				i != weights.end();
				++i)
		{
			WeightedOptions* j = new WeightedOptions();
			j->load(i->second);
			_raceWeights.push_back(std::make_pair(
											i->first.as<size_t>(0),
											j));
		}
	}

	if (const YAML::Node& weights = node["regionWeights"])
	{
		for (YAML::const_iterator
				i = weights.begin();
				i != weights.end();
				++i)
		{
			WeightedOptions* j = new WeightedOptions();
			j->load(i->second);
			_regionWeights.push_back(std::make_pair(
												i->first.as<size_t>(0),
												j));
		}
	}

	_researchTriggers = node["researchTriggers"].as<std::map<std::string, bool> >(_researchTriggers);
	_useTable = node["useTable"].as<bool>(_useTable);

	if (_varName.empty() == true
		&& (_maxRuns > 0
			|| _avoidRepeats > 0))
	{
		throw Exception("Error in mission script: " + _type + ": no varName provided for a script with maxRuns or repeatAvoidance.");
	}
}

/**
 * Gets the type of this command.
 * @return, type
 */
std::string RuleMissionScript::getType() const
{
	return _type;
}

/**
 * Gets the first month this script should run.
 * @return, first month
 */
int RuleMissionScript::getFirstMonth() const
{
	return _firstMonth;
}

/**
 * Gets the last month this script should run.
 * @return, last month
 */
int RuleMissionScript::getLastMonth() const
{
	return _lastMonth;
}

/**
 * Gets the label this command uses for conditional tracking.
 * @return, label
 */
int RuleMissionScript::getLabel() const
{
	return _label;
}

/**
 * Gets the odds of this command's execution.
 * @return, odds
 */
int RuleMissionScript::getExecutionOdds() const
{
	return _executionOdds;
}

/**
 * Gets the odds of this command targetting a base.
 * @return, odds
 */
int RuleMissionScript::getTargetBaseOdds() const
{
	return _targetBaseOdds;
}

/**
 * Gets the minimum difficulty for this script to run.
 * @return, minimum difficulty
 */
int RuleMissionScript::getMinDifficulty() const
{
	return _minDifficulty;
}

/**
 * Gets the maximum runs for scripts tracking our varName.
 * @return, maximum runs
 */
int RuleMissionScript::getMaxRuns() const
{
	return _maxRuns;
}

/**
 * Gets the number of sites to avoid repeating missions.
 * @return, repeat avoidance
 */
int RuleMissionScript::getRepeatAvoidance() const
{
	return _avoidRepeats;
}

/**
 * Gets the fixed delay on spawning the first wave if any to override what's
 * written in the mission definition.
 * @note Overrides the spawn delay defined in the mission waves.
 * @return, delay
 */
int RuleMissionScript::getDelay() const
{
	return _delay;
}

/**
 * Gets the list of conditions that govern execution of this command.
 * @return, reference to a vector of ints
 */
const std::vector<int>& RuleMissionScript::getConditionals() const
{
	return _conditionals;
}

/**
 * Gets if this command uses a weighted distribution to pick a race.
 * @return, true if race-weights
 */
bool RuleMissionScript::hasRaceWeights() const
{
	return (_raceWeights.empty() == false);
}

/**
 * Gets if this command uses a weighted distribution to pick a mission.
 * @return, true if mission-weights
 */
bool RuleMissionScript::hasMissionWeights() const
{
	return (_missionWeights.empty() == false);
}

/**
 * Gets if this command uses a weighted distribution to pick a region.
 * @return, true if region-weights
 */
bool RuleMissionScript::hasRegionWeights() const
{
	return (_regionWeights.empty() == false);
}

/**
 * Gets a list of research topics that govern execution of this script.
 * @return, map of strings to bools
 */
const std::map<std::string, bool>& RuleMissionScript::getResearchTriggers() const
{
	return _researchTriggers;
}

/**
 * Gets if this command should remove the mission it generates from the strategy
 * table.
 * @note Stops it coming up again in random selection but NOT if a missionScript
 * calls it by name.
 * @return, true to use table
 */
bool RuleMissionScript::usesTable() const
{
	return _useTable;
}

/**
 * Gets the name of the variable to use to track in the saved game.
 * @return, string-id
 */
std::string RuleMissionScript::getVarName() const
{
	return _varName;
}

/**
 * Gets a complete unique list of all the mission types this command could
 * possibly generate.
 * @return, set of strings
 */
const std::set<std::string> RuleMissionScript::getAllMissionTypes() const
{
	std::set<std::string> types;

	for (std::vector<std::pair<size_t, WeightedOptions*> >::const_iterator
			i = _missionWeights.begin();
			i != _missionWeights.end();
			++i)
	{
		std::vector<std::string> ids = (*i).second->getNames();
		for (std::vector<std::string>::const_iterator
				j = ids.begin();
				j != ids.end();
				++j)
		{
			types.insert(*j);
		}
	}

	return types;
}

/**
 * Gets a list of the possible missions for the given month.
 * @param month - the month for info
 * @return, vector of strings
 */
const std::vector<std::string> RuleMissionScript::getMissionTypes(const size_t month) const
{
	std::vector<std::string> missions;

	std::vector<std::pair<size_t, WeightedOptions*> >::const_reverse_iterator rweight = _missionWeights.rbegin();
	while (month < rweight->first)
	{
		++rweight;
		if (rweight == _missionWeights.rend())
		{
			--rweight;
			break;
		}
	}

	std::vector<std::string> ids = rweight->second->getNames();
	for (std::vector<std::string>::const_iterator
			i = ids.begin();
			i != ids.end();
			++i)
	{
		missions.push_back(*i);
	}

	return missions;
}

/**
 * Gets the list of regions to pick from this month.
 * @param month - the month for info
 * @return, vector of strings
 */
const std::vector<std::string> RuleMissionScript::getRegions(const size_t month) const
{
	std::vector<std::string> regions;

	std::vector<std::pair<size_t, WeightedOptions*> >::const_reverse_iterator rweight = _regionWeights.rbegin();
	while (month < rweight->first)
	{
		++rweight;
		if (rweight == _regionWeights.rend())
		{
			--rweight;
			break;
		}
	}

	std::vector<std::string> ids = rweight->second->getNames();
	for (std::vector<std::string>::const_iterator
			i = ids.begin();
			i != ids.end();
			++i)
	{
		regions.push_back(*i);
	}

	return regions;
}

/**
 * Chooses one of the available races, regions, or missions for this command.
 * @param monthsPassed	- number of months that have passed in the game world
 * @param type			- type of thing to generate; region, mission or race
 * @return, string-id of the it generated
 */
std::string RuleMissionScript::genMissionDatum(
		const size_t monthsPassed,
		const GenerationType type) const
{
	std::vector<std::pair<size_t, WeightedOptions*> >::const_reverse_iterator rweight;

	switch (type)
	{
		case GEN_RACE:
			rweight = _raceWeights.rbegin();
		break;

		case GEN_REGION:
			rweight = _regionWeights.rbegin();
		break;

		default:
			rweight = _missionWeights.rbegin();
	}

	while (monthsPassed < rweight->first)
		++rweight;

	return rweight->second->choose();
}

/**
 * Sets this command to be a missionSite type or not.
 * @param siteType - true if true
 */
void RuleMissionScript::setSiteType(const bool siteType)
{
	_siteType = siteType;
}

/**
 * Gets if this is a mission site type command or not.
 * @return, true if site
 */
bool RuleMissionScript::getSiteType() const
{
	return _siteType;
}

}
