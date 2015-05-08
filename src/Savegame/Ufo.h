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

#ifndef OPENXCOM_UFO_H
#define OPENXCOM_UFO_H

//#include <string>
//#include <yaml-cpp/yaml.h>

#include "CraftId.h"
#include "MovingTarget.h"


namespace OpenXcom
{

class AlienMission;
class Ruleset;
class RuleUfo;
class SavedGame;
class UfoTrajectory;


/**
 * Represents an alien UFO on the map.
 * Contains variable info about a UFO like position, damage, speed, etc.
 * @sa RuleUfo
 */
class Ufo
	:
		public MovingTarget
{

	public:
		enum UfoStatus
		{
			FLYING,		// 0
			LANDED,		// 1
			CRASHED,	// 2
			DESTROYED	// 3
		};


private:
	bool
		_detected,
		_hyperDetected,
		_inTactical,
		_processedIntercept;
	int
		_damage,
		_id,
		_crashId,
		_crashPower,
		_escapeCountdown,
		_fireCountdown,
		_hitFrame,
		_landId,
		_secondsLeft,
		_shootingAt;
	size_t
		_radius,
		_trajectoryPoint;

	AlienMission* _mission;
	const RuleUfo* _rules;
	const UfoTrajectory* _trajectory;

	std::string
		_altitude,
		_direction,
		_terrain;

	CraftId _shotDownByCraftId;

	enum UfoStatus _status;

	/// Calculates a new speed vector to the destination.
	void calculateSpeed();


	public:
		/// Creates a UFO of the specified type.
		Ufo(const RuleUfo* const rules);
		/// Cleans up the UFO.
		~Ufo();

		/// Loads the UFO from YAML.
		void load(
				const YAML::Node& node,
				const Ruleset& ruleset,
				SavedGame& game);
		/// Saves the UFO to YAML.
		YAML::Node save(bool newBattle) const;
		/// Saves the UFO's ID to YAML.
		YAML::Node saveId() const;

		/// Gets the UFO's ruleset.
		const RuleUfo* const getRules() const;
		/// Sets the UFO's ruleset.
		void changeRules(const RuleUfo* const rules);

		/// Handles UFO logic.
		void think();

		/// Sets the UFO's ID.
		void setId(int id);
		/// Gets the UFO's ID.
		int getId() const;
		/// Gets the UFO's name.
		std::wstring getName(Language* lang) const;

		/// Gets the UFO's marker.
		int getMarker() const;

		/// Sets the UFO's amount of damage.
		void setDamage(int damage);
		/// Gets the UFO's amount of damage.
		int getDamage() const;
		/// Gets the UFO's percentage of damage.
		int getDamagePercent() const;

		/// Sets the UFO's detection status.
		void setDetected(bool detected = true);
		/// Gets the UFO's detection status.
		bool getDetected() const;
		/// Sets the UFO's hyper detection status.
		void setHyperDetected(bool hyperdetected = true);
		/// Gets the UFO's hyper detection status.
		bool getHyperDetected() const;

		/// Sets the UFO's seconds left on the ground.
		void setSecondsLeft(int sec);
		/// Gets the UFO's seconds left on the ground.
		int getSecondsLeft() const;

		/// Sets the UFO's altitude.
		void setAltitude(const std::string& altitude);
		/// Gets the UFO's altitude.
		std::string getAltitude() const;
		/// Gets the UFO's direction.
		std::string getDirection() const;

		/// Gets the UFO status.
		enum UfoStatus getStatus() const
		{ return _status; }
		/// Set the UFO's status.
		void setStatus(enum UfoStatus status)
		{ _status = status; }

		/// Gets if the UFO has crashed.
		bool isCrashed() const;
		/// Gets if the UFO has been destroyed.
		bool isDestroyed() const;

		/// Sets the UFO's battlescape status.
		void setInBattlescape(const bool inTactical);
		/// Gets if the UFO is in battlescape.
		bool isInBattlescape() const;

		/// Gets the UFO's alien race.
		const std::string& getAlienRace() const;

		/// Sets the ID of the Craft that shot down the UFO.
		void setShotDownByCraftId(const CraftId& craftId);
		/// Gets the ID of the Craft that shot down the UFO.
		CraftId getShotDownByCraftId() const;

		/// Gets the scare-factor of UFOs for activity on the Graphs.
		int getVictoryPoints() const;

		/// Gets the UFO's visibility.
		int getVisibility() const;
		/// Gets a UFO's detect-xCom-base ability.
		int getDetectors() const;

		/// Sets the UFO's mission information.
		void setUfoMissionInfo(
				AlienMission* mission,
				const UfoTrajectory* trajectory);
		/// Gets the UFO's Mission type.
		const std::string& getUfoMissionType() const;
		/// Gets the UFO's mission object.
		AlienMission* getAlienMission() const
		{ return _mission; }

		/// Gets the UFO's progress on the trajectory track.
		size_t getTrajectoryPoint() const
		{ return _trajectoryPoint; }
		/// Sets the UFO's progress on the trajectory track.
		void setTrajectoryPoint(size_t pt)
		{ _trajectoryPoint = pt; }
		/// Gets the UFO's trajectory.
		const UfoTrajectory& getTrajectory() const
		{ return *_trajectory; }

		/// Sets the UFO's destination.
		void setDestination(Target* dest);

		/// Sets the interceptor engaging this Ufo.
		void setShootingAt(const size_t target);
		/// Gets the interceptor engaging this Ufo.
		size_t getShootingAt() const;

		/// Sets the UFO's landing site ID.
		void setLandId(int id);
		/// Gets the UFO's landing site ID.
		int getLandId() const;
		/// Sets the UFO's crash site ID.
		void setCrashId(int id);
		/// Gets the UFO's crash site ID.
		int getCrashId() const;

		/// Sets the UFO's hit frame.
		void setHitFrame(int frame);
		/// Gets the UFO's hit frame.
		int getHitFrame() const;

		/// Sets the time left before this Ufo can fire in a Dogfight.
		void setFireCountdown(int timeLeft);
		/// Gets the time left before this Ufo can fire in a Dogfight.
		int getFireCountdown() const;
		/// Sets the time left before this Ufo attempts to escape a Dogfight.
		void setEscapeCountdown(int timeLeft);
		/// Gets the time left before this Ufo attempts to escape a Dogfight.
		int getEscapeCountdown() const;
		/// Sets whether or not this Ufo has had Dogfight info processed.
		void setEngaged(bool processed = true);
		/// Gets whether or not this Ufo has had Dogfight info processed.
		bool getEngaged() const;

		/// Sets a crashed or landed UFO's terrainType.
		void setUfoTerrainType(const std::string& terrain);
		/// Gets a crashed or landed UFO's terrainType.
		std::string getUfoTerrainType() const;

		/// Gets the size of this Ufo.
		size_t getRadius() const;
};

}

#endif
