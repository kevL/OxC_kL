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

#include "RuleCommendations.h"


namespace OpenXcom
{

/**
 * Creates a blank set of commendation data.
 */
RuleCommendations::RuleCommendations()
	:
		_sprite(-1)
{}

/**
 * Cleans up the commendation.
 */
RuleCommendations::~RuleCommendations()
{}

/**
 * Loads the commendations from YAML.
 * @param node - reference a YAML node
 */
void RuleCommendations::load(const YAML::Node& node)
{
	_description	= node["description"]	.as<std::string>(_description);
	_sprite			= node["sprite"]		.as<int>(_sprite);
	_criteria		= node["criteria"]		.as<std::map<std::string, std::vector<int> > >(_criteria);
	_killCriteria	= node["killCriteria"]	.as<std::vector<std::map<int, std::vector<std::string> > > >(_killCriteria);
}

/**
 * Get the commendation's description.
 * @return, commendation description
 */
std::string RuleCommendations::getDescription() const
{
	return _description;
}

/**
 * Get the commendation's sprite.
 * @return, sprite number
 */
int RuleCommendations::getSprite() const
{
	return _sprite;
}

/**
 * Get the commendation's award criteria.
 * @return, pointer to a map of (strings & vectors of ints) that denote commendation criteria
 */
std::map<std::string, std::vector<int> >* RuleCommendations::getCriteria()
{
	return &_criteria;
}

/**
 * Get the commendation's award kill criteria.
 * @return, pointer to a vector of maps of (ints & vectors of strings) that denote commendation kill criteria
 */
std::vector<std::map<int, std::vector<std::string> > >* RuleCommendations::getKillCriteria()
{
	return &_killCriteria;
}

}
