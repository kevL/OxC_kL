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

#include "RuleAward.h"


namespace OpenXcom
{

/**
 * Creates a blank set of Award data.
 */
RuleAward::RuleAward()
	:
		_sprite(-1)
{}

/**
 * Cleans up the Award.
 */
RuleAward::~RuleAward()
{}

/**
 * Loads the Award from YAML.
 * @param node - reference a YAML node
 */
void RuleAward::load(const YAML::Node& node)
{
	_description	= node["description"]	.as<std::string>(_description);
	_descGeneral	= node["descGeneral"]	.as<std::string>(_descGeneral);
	_sprite			= node["sprite"]		.as<int>(_sprite);
	_criteria		= node["criteria"]		.as<std::map<std::string, std::vector<int> > >(_criteria);
	_killCriteria	= node["killCriteria"]	.as<std::vector<std::map<int, std::vector<std::string> > > >(_killCriteria);
}

/**
 * Gets the Award's description.
 * @return, award description
 */
std::string RuleAward::getDescription() const
{
	return _description;
}

/**
 * Gets the Award's non-specific description.
 * @return, generic description
 */
std::string RuleAward::getDescriptionGeneral() const
{
	return _descGeneral;
}

/**
 * Gets the Award's sprite.
 * @return, sprite number
 */
int RuleAward::getSprite() const
{
	return _sprite;
}

/**
 * Gets the Award's criteria.
 * @return, pointer to a map of (strings & vectors of ints) that denote award criteria
 */
std::map<std::string, std::vector<int> >* RuleAward::getCriteria()
{
	return &_criteria;
}

/**
 * Gets the Award's kill criteria.
 * @return, pointer to a vector of maps of (ints & vectors of strings) that denote award kill criteria
 */
std::vector<std::map<int, std::vector<std::string> > >* RuleAward::getKillCriteria()
{
	return &_killCriteria;
}

}
