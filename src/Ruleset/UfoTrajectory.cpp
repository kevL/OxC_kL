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

#include "UfoTrajectory.h"


namespace
{

const char* stAltitude[] =
{
	"STR_GROUND",
	"STR_VERY_LOW",
	"STR_LOW_UC",
	"STR_HIGH_UC",
	"STR_VERY_HIGH"
};

}


namespace YAML
{

template<>
struct convert<OpenXcom::TrajectoryWaypoint>
{
	///
	static Node encode(const OpenXcom::TrajectoryWaypoint& rhs)
	{
		Node node;

		node.push_back(rhs.zone);
		node.push_back(rhs.altitude);
		node.push_back(rhs.speed);

		return node;
	}

	///
	static bool decode(
			const Node& node,
			OpenXcom::TrajectoryWaypoint& rhs)
	{
		if (node.IsSequence() == false
			|| node.size() != 3)
		{
			return false;
		}

		rhs.zone		= node[0].as<int>();
		rhs.altitude	= node[1].as<int>();
		rhs.speed		= node[2].as<int>();

		return true;
	}
};

}


namespace OpenXcom
{

const std::string UfoTrajectory::RETALIATION_ASSAULT_RUN = "__RETALIATION_ASSAULT_RUN";


/**
 * Creates a UfoTrajectory.
 * @param id - reference the ID string (eg. 'P0')
 */
UfoTrajectory::UfoTrajectory(const std::string& id)
	:
		_id(id),
		_groundTimer(5)
{}

/**
 * Overwrites trajectory data with the data stored in @a node.
 * @note Only the fields contained in the node will be overwritten.
 * @param node - reference the node containing the new values
 */
void UfoTrajectory::load(const YAML::Node& node)
{
	_id				= node["id"]			.as<std::string>(_id);
	_groundTimer	= node["groundTimer"]	.as<size_t>(_groundTimer);
	_waypoints		= node["waypoints"]		.as<std::vector<TrajectoryWaypoint> >(_waypoints);
}

/**
 * Gets the altitude at a waypoint.
 * @param wpId - the waypoint ID
 * @return, altitude string
 */
const std::string UfoTrajectory::getAltitude(size_t wpId) const // does not like return &ref
{
	return stAltitude[_waypoints[wpId].altitude];
}

}
