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

#ifndef OPENXCOM_ALIENSTRATEGY_H
#define OPENXCOM_ALIENSTRATEGY_H

//#include <yaml-cpp/yaml.h>

#include "WeightedOptions.h"


namespace OpenXcom
{

class Ruleset;


/**
 * Stores the information about alien strategy.
 */
class AlienStrategy
{

private:
	WeightedOptions _regionChances; // The chances of each region to be targeted for a mission.
	std::map<std::string, WeightedOptions*> _regionMissions; // The chances of each mission type for each region.

	std::map<std::string, int> _missionRuns;
	std::map<std::string, std::vector<std::pair<std::string, int> > > _missionLocations;

	/// Disable copy and assignments.
	AlienStrategy(const AlienStrategy&);
	///
	AlienStrategy& operator= (const AlienStrategy&);


	public:
		/// Creates an AlienStrategy with no data.
		AlienStrategy();
		/// Frees resources used by the AlienStrategy.
		~AlienStrategy();

		/// Saves the data to YAML.
		YAML::Node save() const;

		/// Initializes values according to the rules.
		void init(const Ruleset* const rules);
		/// Loads the data from YAML.
		void load(const YAML::Node& node);

		/// Chooses a random region for a regular mission.
		const std::string chooseRandomRegion(const Ruleset* const rules);
		/// Chooses a random mission for a region.
		const std::string chooseRandomMission(const std::string& region) const;

		/// Removes a region and mission from the list of possibilities.
		bool removeMission(
				const std::string& region,
				const std::string& mission);

		/// Checks the number of missions labelled 'id' that have run.
		int getMissionsRun(const std::string& id);
		/// Increments the number of missions labelled 'id' that have run.
		void addMissionRun(const std::string& id);
		/// Adds a mission location to the storage array.
		void addMissionLocation(
				const std::string& id,
				const std::string& region,
				int zone,
				size_t track);
		/// Checks if a given mission location has been attacked already.
		bool validMissionLocation(
				const std::string& id,
				const std::string& region,
				int zone);
		/// Checks that a given region appears in the strategy table.
		bool validMissionRegion(const std::string& region) const;
};

}

#endif
