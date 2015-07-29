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

#ifndef OPENXCOM_ALIEN_MISSION_H
#define OPENXCOM_ALIEN_MISSION_H

//#include <string>
//#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

class AlienBase;
class AlienDeployment;
class Game;
class Globe;
class MissionSite;
class RuleAlienMission;
class RuleRegion;
class Ruleset;
class SavedGame;
class Ufo;
class UfoTrajectory;

struct MissionArea;
struct MissionWave;


/**
 * Represents an ongoing AlienMission.
 * @note Contains variable info about the mission such as spawn counter and
 * target region and current wave.
 * @sa RuleAlienMission
 */
class AlienMission
{

private:
	bool _success;	// Prevents infiltration missions taking multiple success points
					// and creating multiple bases on the final 2xBattleship wave.
	int _uniqueID;
	size_t
		_liveUfos,
		_ufoCount,
		_waveCount,
		_siteZone,
		_spawnTime;

	std::string
		_race,
		_region;

	const AlienBase* _aBase;
	const RuleAlienMission& _missionRule;
	SavedGame& _gameSave;

	/// Calculates time remaining until the next wave spawns.
	void calcSpawnTime(size_t nextWave);
	/// Spawns a UFO based on mission rules.
	Ufo* spawnUfo(
			const Ruleset& rules,
			const Globe& globe,
			const MissionWave& wave,
			const UfoTrajectory& trajectory);
	/// Spawns a MissionSite at a specific location.
	MissionSite* spawnMissionSite(
			const AlienDeployment* const deployment,
			const MissionArea& area);
	/// Spawns an alien base
	void spawnAlienBase(
			const Globe& globe,
			const Ruleset& rules,
			const size_t zone);
	/// Handles Points for mission successes.
	void addScore(
			const double lon,
			const double lat);


	public:
		/// Creates a mission of the specified type.
		AlienMission(
				const RuleAlienMission& missionRule,
				SavedGame& gameSave);
		/// Cleans up the mission info.
		~AlienMission();

		/// Loads the mission from YAML.
		void load(const YAML::Node& node);
		/// Saves the mission to YAML.
		YAML::Node save() const;

		/// Gets the mission's ruleset.
		const RuleAlienMission& getRules() const
		{ return _missionRule; }

		/// Sets the unique ID for this mission.
		void setId(int id);
		/// Gets the unique ID for this mission.
		int getId() const;

		/// Gets the mission's region.
		const std::string& getRegion() const
		{ return _region; }
		/// Sets the mission's region.
		void setRegion(
				const std::string& region,
				const Ruleset& rules);

		/// Gets the mission's race.
		const std::string& getRace() const
		{ return _race; }
		/// Sets the mission's race.
		void setRace(const std::string& race)
		{ _race = race; }

		/// Gets the minutes until next wave spawns.
		size_t getWaveCountdown() const
		{ return _spawnTime; }
		/// Sets the minutes until next wave spawns.
		void setWaveCountdown(size_t minutes);

		/// Increases number of live UFOs.
		void increaseLiveUfos()
		{ ++_liveUfos; }
		/// Decreases number of live UFOs.
		void decreaseLiveUfos()
		{ --_liveUfos; }

		/// Gets if this mission over.
		bool isOver() const;
		/// Initializes with values from rules.
		void start(size_t countdown = 0);

		/// Handles UFO spawning for the mission.
		void think(
				Game& engine,
				const Globe& globe);

		/// Handles UFO reaching a waypoint.
		void ufoReachedWaypoint(
				Ufo& ufo,
				const Ruleset& rules,
				const Globe& globe);

		/// Gets the alien base for this mission.
		const AlienBase* getAlienBase() const;
		/// Sets the alien base for this mission.
		void setAlienBase(const AlienBase* const base);

		/// Handles UFO lifting from the ground.
		void ufoLifting(
				Ufo& ufo,
				Ruleset& rules,
				const Globe& globe);
		/// Handles UFO shot down.
		void ufoShotDown(const Ufo& ufo);

		/// Selects a destination (lon/lat) based on the criteria of our trajectory and desired waypoint.
		std::pair<double, double> getWaypoint(
				const UfoTrajectory& trajectory,
				const size_t nextWaypoint,
				const RuleRegion& region);
		/// Gets a random landing point inside the given region zone.
		std::pair<double, double> getLandPoint(
				const Globe& globe,
				const RuleRegion& region,
				const size_t zone);

		/// Tracks the target site.
		void setMissionSiteZone(size_t zone);
};

}

#endif
