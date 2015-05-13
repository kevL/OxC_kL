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

#ifndef OPENXCOM_UFOTRAJECTORY_H
#define OPENXCOM_UFOTRAJECTORY_H

//#include <vector>
//#include <string>
//#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

/**
 * Information for points on a UFO trajectory.
 */
struct TrajectoryWaypoint
{
	size_t
		zone,		// The mission zone.
		altitude,	// The altitude to reach.
		speed;		// The speed percentage [0..100]
};


YAML::Emitter& operator<< (
		YAML::Emitter& emitter,
		const TrajectoryWaypoint& wp);

bool operator>> (
		const YAML::Node& node,
		TrajectoryWaypoint& wp);


/**
 * Holds information about a specific trajectory.
 * Ufo trajectories are a sequence of mission zones, altitudes and speed percentages.
 */
class UfoTrajectory
{

private:
	size_t _groundTimer;
	std::string _id;

	std::vector<TrajectoryWaypoint> _waypoints;


	public:
		///
		explicit UfoTrajectory(const std::string& id);

		/// Loads trajectory data from YAML.
		void load(const YAML::Node &node);

		/**
		 * Gets the trajectory's ID.
		 * @return, the trajectory's ID
		 */
		const std::string& getID() const
		{ return _id; }

		/**
		 * Gets the number of waypoints in this trajectory.
		 * @return, the number of waypoints
		 */
		size_t getWaypointTotal() const
		{ return _waypoints.size(); }

		/**
		 * Gets the zone index at a waypoint.
		 * @param wp - the waypoint
		 * @return, the zone index
		 */
		size_t getZone(size_t wp) const
		{ return _waypoints[wp].zone; }

		/// Gets the altitude at a waypoint.
		std::string getAltitude(size_t wp) const;

		/**
		 * Gets the speed percentage at a waypoint.
		 * @param wp - the waypoint
		 * @return, the speed as a percentage
		 */
		float getSpeedPercentage(size_t wp) const
		{ return _waypoints[wp].speed / 100.f; }

		/**
		 * Gets the number of seconds UFOs should spend on the ground.
		 * @return, the number of seconds
		 */
		size_t groundTimer() const
		{ return _groundTimer; }
};

}

#endif
