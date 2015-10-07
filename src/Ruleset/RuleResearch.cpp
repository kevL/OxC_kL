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

#include "RuleResearch.h"


namespace OpenXcom
{

/**
 * Creates a new RuleResearch.
 * @param type - reference to the type string of this RuleResearch
 */
RuleResearch::RuleResearch(const std::string& type)
	:
		_type(type),
		_cost(0),
		_points(0),
		_needsItem(false),
		_listOrder(0)
{}

/**
 * dTor.
 */
RuleResearch::~RuleResearch()
{}

/**
 * Loads the RuleResearch from a YAML file.
 * @param node		- reference a YAML node
 * @param listOrder	- list weight
 */
void RuleResearch::load(
		const YAML::Node& node,
		int listOrder)
{
	_type			= node["type"]			.as<std::string>(_type);
	_lookup			= node["lookup"]		.as<std::string>(_lookup);
	_cost			= node["cost"]			.as<int>(_cost);
	_points			= node["points"]		.as<int>(_points);
	_dependencies	= node["dependencies"]	.as<std::vector<std::string> >(_dependencies);
	_unlocks		= node["unlocks"]		.as<std::vector<std::string> >(_unlocks);
	_getOneFree		= node["getOneFree"]	.as<std::vector<std::string> >(_getOneFree);
	_requires		= node["requires"]		.as<std::vector<std::string> >(_requires);
	_needsItem		= node["needsItem"]		.as<bool>(_needsItem);
	_listOrder		= node["listOrder"]		.as<int>(_listOrder);

	if (_listOrder == 0)
		_listOrder = listOrder;
}

/**
 * Gets the cost of this RuleResearch.
 * @return, cost in man/days
 */
int RuleResearch::getCost() const
{
	return _cost;
}

/**
 * Gets the type of this RuleResearch.
 * @return, reference to the type string
 */
const std::string& RuleResearch::getType() const
{
	return _type;
}

/**
 * Gets the list of dependencies - ie. research types - that must be
 * discovered before this one.
 * @return, reference to a vector of type strings
 */
const std::vector<std::string>& RuleResearch::getDependencies() const
{
	return _dependencies;
}

/**
 * Checks if this RuleResearch needs a corresponding Item to be researched.
 * @return, true if item is required
 */
bool RuleResearch::needsItem() const
{
	return _needsItem;
}

/**
 * Gets the list of research types unlocked by this RuleResearch.
 * @return, reference to a vector of type strings
 */
const std::vector<std::string>& RuleResearch::getUnlocks() const
{
	return _unlocks;
}

/**
 * Get the points earned for completing this RuleResearch.
 * @return, points granted
 */
int RuleResearch::getPoints() const
{
	return _points;
}

/**
 * Gets the list of research types granted at random for free by this RuleResearch.
 * @return, reference to a vector of type strings
 */
const std::vector<std::string>& RuleResearch::getGetOneFree() const
{
	return _getOneFree;
}

/**
 * Gets what article to look up in the Ufopedia.
 * @return, article to look up
 */
const std::string& RuleResearch::getLookup() const
{
	return _lookup;
}

/**
 * Gets the requirements for this RuleResearch.
 * @return, reference to a vector of type strings
 */
const std::vector<std::string>& RuleResearch::getPrerequisites() const
{
	return _requires;
}

/**
 * Gets the list priority for this RuleResearch.
 * @return, list priority
 */
int RuleResearch::getListOrder() const
{
	return _listOrder;
}

}
