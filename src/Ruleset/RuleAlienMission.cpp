/*
 * Copyright 2012 OpenXcom Developers.
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

#include "RuleAlienMission.h"

#include "../Savegame/WeightedOptions.h"


namespace YAML
{

template<>
struct convert<OpenXcom::MissionWave>
{
	///
	static Node encode(const OpenXcom::MissionWave& rhs)
	{
		Node node;

		node["ufo"]			= rhs.ufoType;
		node["count"]		= rhs.ufoCount;
		node["trajectory"]	= rhs.trajectory;
		node["timer"]		= rhs.spawnTimer;

		return node;
	}

	///
	static bool decode(
			const Node& node,
			OpenXcom::MissionWave& rhs)
	{
		if (!node.IsMap())
			return false;

		rhs.ufoType		= node["ufo"].as<std::string>();
		rhs.ufoCount	= node["count"].as<size_t>();
		rhs.trajectory	= node["trajectory"].as<std::string>();
		rhs.spawnTimer	= node["timer"].as<size_t>();

		return true;
	}
};

}


namespace OpenXcom
{

/**
 * Creates an AlienMission rule.
 * @param type - reference the mission type
 */
RuleAlienMission::RuleAlienMission(const std::string& type)
	:
		_type(type),
		_points(0),
		_markerIcon(-1)
{}

/**
 * Ensures the allocated memory is released.
 */
RuleAlienMission::~RuleAlienMission()
{
	for (std::vector<std::pair<size_t, WeightedOptions*> >::const_iterator
			i = _raceDistribution.begin();
			i != _raceDistribution.end();
			++i)
	{
		delete i->second;
	}
}

/**
 * Loads the mission data from a YAML node.
 * @param node - YAML node
 */
void RuleAlienMission::load(const YAML::Node& node)
{
	_type		= node["type"]		.as<std::string>(_type);
	_points		= node["points"]	.as<int>(_points);
	_waves		= node["waves"]		.as<std::vector<MissionWave> >(_waves);
	_specialUfo	= node["specialUfo"].as<std::string>(_specialUfo);
	_deployment	= node["deployment"].as<std::string>(_deployment);
	_markerName	= node["markerName"].as<std::string>(_markerName);
	_markerIcon	= node["markerIcon"].as<int>(_markerIcon);

	// Only allow full replacement of mission racial distribution.
	if (const YAML::Node& weights = node["raceWeights"])
	{
		typedef std::map<size_t, WeightedOptions*> Associative;
		typedef std::vector<std::pair<size_t, WeightedOptions*> > Linear;

		Associative assoc;
		// Place in the associative container so we can index by month and keep entries sorted.
		for (Linear::const_iterator
				i = _raceDistribution.begin();
				i != _raceDistribution.end();
				++i)
		{
			assoc.insert(*i);
		}

		// Now go through the node contents and merge with existing data.
		for (YAML::const_iterator
				i = weights.begin();
				i != weights.end();
				++i)
		{
			const size_t month = i->first.as<size_t>();

			Associative::const_iterator existing = assoc.find(month);
			if (assoc.end() == existing) // New entry, load and add it.
			{
				std::auto_ptr<WeightedOptions> weight (new WeightedOptions);
				weight->load(i->second);

				assoc.insert(std::make_pair(month, weight.release()));
			}
			else
				existing->second->load(i->second); // Existing entry, update it.
		}

		// Now replace values in our actual member variable!
		_raceDistribution.clear();
		_raceDistribution.reserve(assoc.size());

		for (Associative::const_iterator
				i = assoc.begin();
				i != assoc.end();
				++i)
		{
			if (i->second->hasNoWeight()) // Don't keep empty lists.
				delete i->second;
			else
				_raceDistribution.push_back(*i); // Place it
		}
	}
}

/**
 * Chooses one of the available races for this mission.
 * @note The racial distribution may vary based on the current game date.
 * @param monthsPassed - the number of months that have passed in the game world
 * @return, the string id of the race
 */
const std::string RuleAlienMission::generateRace(size_t const monthsPassed) const
{
	std::vector<std::pair<size_t, WeightedOptions*> >::const_reverse_iterator race = _raceDistribution.rbegin();
	while (monthsPassed < race->first)
		++race;

	return race->second->choose();
}

/**
 * Chooses the best candidate.
 * @param monthsPassed - the number of months that have passed in the game world
 */
const std::string RuleAlienMission::getTopRace(const size_t monthsPassed) const
{
	std::vector<std::pair<size_t, WeightedOptions*> >::const_iterator race = _raceDistribution.begin();
	return race->second->topChoice();
}

/**
 * Returns the Alien score for this mission.
 * @return, amount of points
 */
int RuleAlienMission::getPoints() const
{
	return _points;
}

/**
 * Returns the alien deployment for this mission.
 * @return, string ID for deployment
 */
std::string RuleAlienMission::getDeployment() const
{
	return _deployment;
}

/**
 * Returns the globe marker name for this mission.
 * @return, string ID for marker name
 */
std::string RuleAlienMission::getMarkerName() const
{
	return _markerName;
}

/**
 * Returns the globe marker icon for this mission.
 * @return, marker sprite -1 if none
 */
int RuleAlienMission::getMarkerIcon() const
{
	return _markerIcon;
}

}
