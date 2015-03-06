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

#ifndef OPENXCOM_RULEALIENMISSION_H
#define OPENXCOM_RULEALIENMISSION_H

//#include <map>
//#include <string>
//#include <vector>
//#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

class WeightedOptions;


/**
 * @brief Information about a mission wave.
 * Mission waves control the UFOs that will be generated during an alien mission.
 */
struct MissionWave
{
	/// The type of the spawned UFOs.
	std::string ufoType;

	/// The number of UFOs that will be generated.
	/// The UFOs are generated sequentially, one every @a spawnTimer minutes.
	size_t ufoCount;

	/// The trajectory ID for this wave's UFOs.
	/// Trajectories control the way UFOs fly around the Geoscape.
	std::string trajectory;

	/// Number of minutes between UFOs in the wave.
	/// The actual value used is spawnTimer*1/4 or spawnTimer*3/4.
	size_t spawnTimer;

	/// This wave performs the mission objective.
	/// The UFO executes a special action based on the mission objective.
	bool objective;
};


enum MissionObjective
{
	OBJECTIVE_SCORE,		// 0
	OBJECTIVE_INFILTRATION,	// 1
	OBJECTIVE_BASE,			// 2
	OBJECTIVE_SITE,			// 3
	OBJECTIVE_RETALIATION,	// 4
	OBJECTIVE_SUPPLY		// 5
};


/**
 * Stores fixed information about a mission type.
 * It stores the mission waves and the distribution of the races
 * that can undertake the mission based on game date.
 */
class RuleAlienMission
{

private:
	int
		_points,		// The mission's points.
		_specialZone;	// The mission zone to use for spawning.
	std::string
		_specialUfo,	// The UFO to use for spawning.
		_type;			// The mission's type ID.

	std::vector<MissionWave> _waves;										// The mission's waves.
	std::vector<std::pair<size_t, WeightedOptions*> > _raceDistribution;	// The race distribution over game time.
	std::map<size_t, int> _weights;											// The mission's weights.

	MissionObjective _objective; // The mission's objective type (enum).


	public:
		/// Creates an Alien Mission rule.
		RuleAlienMission(const std::string& type);
		/// Releases all resources held by the mission.
		~RuleAlienMission();

		/// Loads alien mission data from YAML.
		void load(const YAML::Node& node);

		/// Gets a race based on the game time and the racial distribution.
		const std::string generateRace(const size_t monthsPassed) const;
		/// Gets the most likely race based on the game time and the racial distribution.
		const std::string getTopRace(const size_t monthsPassed) const;

		/// Gets the mission's type.
		const std::string& getType() const
		{ return _type; }
		/// Gets the number of waves.
		size_t getWaveCount() const
		{ return _waves.size(); }
		/// Gets the full wave information.
		const MissionWave& getWave(size_t index) const
		{ return _waves[index]; }

		/// Gets the score for this mission.
		int getPoints() const;

		/// Gets the objective for this mission.
		MissionObjective getObjective() const
		{ return _objective; }

		/// Gets the UFO type for special spawns.
		const std::string& getSpawnUfo() const
		{ return _specialUfo; }
		/// Gets the zone for spawning an alien site or base.
		int getSpawnZone() const
		{ return _specialZone; }

		/// Gets the chances of this mission based on the game time.
		int getWeight(const size_t monthsPassed) const;
};

}

#endif
