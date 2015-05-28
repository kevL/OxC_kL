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

#include "RuleManufacture.h"


namespace OpenXcom
{

/**
 * Creates a new Manufacture.
 * @param name - reference a unique manufacture name
 */
RuleManufacture::RuleManufacture(const std::string& name)
	:
		_name(name),
		_space(0),
		_time(0),
		_cost(0),
		_listOrder(0)
{
	_producedItems[name] = 1;
}

/**
 * Loads a RuleManufacture from a YAML file.
 * @param node		- reference a YAML node
 * @param listOrder	- the list weight for this manufacture
 */
void RuleManufacture::load(
		const YAML::Node& node,
		int listOrder)
{
	// why ->
	bool same = _producedItems.size() == 1
			 && _name == _producedItems.begin()->first;

	_name = node["name"].as<std::string>(_name);

	if (same == true)
	{
		const int qty = _producedItems.begin()->second;
		_producedItems.clear();
		_producedItems[_name] = qty;
	} // end_why. Perhaps to overwrite a previous entry with a subsequently loaded ID-string, perhaps.

	_category		= node["category"]		.as<std::string>(_category);
	_requires		= node["requires"]		.as< std::vector<std::string> >(_requires);
	_space			= node["space"]			.as<int>(_space);
	_time			= node["time"]			.as<int>(_time);
	_cost			= node["cost"]			.as<int>(_cost);
	_requiredItems	= node["requiredItems"]	.as<std::map<std::string, int> >(_requiredItems);
	_producedItems	= node["producedItems"]	.as<std::map<std::string, int> >(_producedItems);

	_listOrder		= node["listOrder"]		.as<int>(_listOrder);
	if (_listOrder == 0)
		_listOrder = listOrder;
}

/**
 * Gets the unique name of the manufacture.
 * @return, the name
 */
std::string RuleManufacture::getName() const
{
	return _name;
}

/**
 * Gets the category shown in the manufacture list.
 * @return, the category
 */
std::string RuleManufacture::getCategory() const
{
	return _category;
}

/**
 * Gets the list of research required to manufacture this object.
 * @return, reference to a list of research IDs
 */
const std::vector<std::string>& RuleManufacture::getRequirements() const
{
	return _requires;
}

/**
 * Gets the required workspace to start production.
 * @return, the required workspace
*/
int RuleManufacture::getRequiredSpace() const
{
	return _space;
}

/**
 * Gets the time needed to manufacture one object.
 * @return, the time needed to manufacture one object (in man/hour)
*/
int RuleManufacture::getManufactureTime() const
{
	return _time;
}

/**
 * Gets the cost of manufacturing one object.
 * @return, the cost of manufacturing one object
*/
int RuleManufacture::getManufactureCost() const
{
	return _cost;
}

/**
 * Gets the list of items required to manufacture one object.
 * @return, reference to the list of items required to manufacture one object
*/
const std::map<std::string, int>& RuleManufacture::getRequiredItems() const
{
	return _requiredItems;
}

/**
 * Gets the list of items produced by completing one object of this project.
 * @note By default it contains only the item's ID-string with a value of 1.
 * @return, reference to the list of items produced by completing one object of this project
*/
const std::map<std::string, int>& RuleManufacture::getProducedItems() const
{
	return _producedItems;
}

/**
 * Gets the list weight for this manufacture item.
 * @return, the list weight for this manufacture item
 */
int RuleManufacture::getListOrder() const
{
	return _listOrder;
}

}
