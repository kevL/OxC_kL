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

#ifndef OPENXCOM_RULEMISSIONSCRIPT_H
#define OPENXCOM_RULEMISSIONSCRIPT_H

//#include <map>
//#include <string>
//#include <vector>
//#include <yaml-cpp/yaml.h>

#include "../Savegame/WeightedOptions.h"


namespace OpenXcom
{

enum GenerationType
{
	GT_REGION,	// 0
	GT_MISSION,	// 1
	GT_RACE		// 2
};

class WeightedOptions;


/**
 * For scripted missions.
 */
class RuleMissionScript
{

private:
	bool
		_siteType,
		_useTable;
	int
		_avoidRepeats,
		_delay,
		_executionOdds,
		_firstMonth,
		_label,
		_lastMonth,
		_maxRuns,
		_minDifficulty,
		_targetBaseOdds;

	std::string
		_type,
		_varName;

	std::vector<int> _conditionals;

	std::vector<std::pair<size_t, WeightedOptions*> >
		_missionWeights,
		_raceWeights,
		_regionWeights;

	std::map<std::string, bool> _researchTriggers;


	public:
		/// Creates a new mission script.
		RuleMissionScript(const std::string& type);
		/// Deletes a mission script.
		~RuleMissionScript();

		/// Loads a mission script from yaml.
		void load(const YAML::Node& node);

		/// Gets the name of the script command.
		std::string getType() const;
		/// Gets the name of the variable to use for keeping track of ... things.
		std::string getVarName() const;

		/// Gets a complete and unique list of all the mission types contained within this command.
		const std::set<std::string> getAllMissionTypes() const;
		/// Gets a list of the mission types in this command's mission weights for a given month.
		const std::vector<std::string> getMissionTypes(const size_t month) const;
		/// Gets a list of the regions in this command's region weights for a given month.
		const std::vector<std::string> getRegions(const size_t month) const;

		/// Gets the first month this command will run.
		int getFirstMonth() const;
		/// Gets the last month this command will run.
		int getLastMonth() const;

		/// Gets the label of this command, used for conditionals.
		int getLabel() const;

		/// Gets the odds of this command executing.
		int getExecutionOdds() const;
		/// Gets the odds of this mission targetting an xcom base.
		int getTargetBaseOdds() const;

		/// Gets the minimum difficulty for this command to run
		int getMinDifficulty() const;
		/// Gets the maximum number of times to run a command with this varName
		int getMaxRuns() const;
		/// Gets how many previous mission sites to track.
		int getRepeatAvoidance() const;
		/// Gets the number of minutes to delay spawning the first wave.
		int getDelay() const;

		/// Gets the list of conditions this command requires in order to run.
		const std::vector<int>& getConditionals() const;

		/// Checks if this command has race weights.
		bool hasRaceWeights() const;
		/// Checks if this command has mission weights.
		bool hasMissionWeights() const;
		/// Checks if this command has region weights.
		bool hasRegionWeights() const;

		/// Gets the research triggers that may apply to this command.
		const std::map<std::string, bool>& getResearchTriggers() const;

		/// Checks if this mission uses the table.
		bool usesTable() const;

		/// Generates either a region, a mission, or a race based on the month.
		std::string genMissionDatum(
				const size_t monthsPassed,
				const GenerationType type) const;

		/// Sets this script to a terror mission type command or not.
		void setSiteType(const bool siteType);
		/// Checks if this is a terror-type mission or not.
		bool getSiteType() const;
};

}

#endif
