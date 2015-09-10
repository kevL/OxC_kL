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

#include "AlienRace.h"


namespace OpenXcom
{

/**
 * Creates a blank alien race.
 * @param type - reference the defining type
 */
AlienRace::AlienRace(const std::string& type)
	:
		_type(type),
		_retaliation(true)
{}

/**
 * dTor.
 */
AlienRace::~AlienRace()
{}

/**
 * Loads the alien race from a YAML file.
 * @param node - reference a YAML node
 */
void AlienRace::load(const YAML::Node& node)
{
	_type			= node["type"]			.as<std::string>(_type);
	_members		= node["members"]		.as<std::vector<std::string> >(_members);
	_retaliation	= node["retaliation"]	.as<bool>(_retaliation);
}

/**
 * Returns the language string that names this AlienRace.
 * @return, race type
 */
/* std::string AlienRace::getAlienType() const
{
	return _type;
} */

/**
 * Gets a certain member of this AlienRace family.
 * @param rankId - AlienRank (AlienRace.h)
 * @return, the member's race & rank
 */
std::string AlienRace::getMember(int rankId) const
{
	return _members[static_cast<size_t>(rankId)];
}

/**
 * Returns if this AlienRace can participate in retaliation missions.
 * @return, true if race can retaliate
 */
bool AlienRace::canRetaliate() const
{
	return _retaliation;
}

}
