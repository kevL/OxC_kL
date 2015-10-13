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

#include "ItemContainer.h"

#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

/**
 * Initializes an ItemContainer with no contents.
 */
ItemContainer::ItemContainer()
{}

/**
 * dTor.
 */
ItemContainer::~ItemContainer()
{}

/**
 * Loads the ItemContainer from a YAML file.
 * @param node - reference a YAML node
 */
void ItemContainer::load(const YAML::Node& node)
{
	_contents = node.as<std::map<std::string, int> >(_contents);
}

/**
 * Saves the item container to a YAML file.
 * @return, YAML node
 */
YAML::Node ItemContainer::save() const
{
	YAML::Node node;
	node = _contents;

	return node;
}

/**
 * Adds an item amount to the container.
 * @param type	- reference an item type
 * @param qty	- item quantity (default 1)
 */
void ItemContainer::addItem(
		const std::string& type,
		int qty)
{
	if (type.empty() == false)
	{
		if (_contents.find(type) == _contents.end())
			_contents[type] = 0;

		_contents[type] += qty;
	}
}

/**
 * Removes an item amount from the container.
 * @param type	- reference an item type
 * @param qty	- item quantity (default 1)
 */
void ItemContainer::removeItem(
		const std::string& type,
		int qty)
{
	if (type.empty() == false && _contents.find(type) != _contents.end())
	{
		if (qty < _contents[type])
			_contents[type] -= qty;
		else
			_contents.erase(type);
	}
}

/**
 * Returns the quantity of an item in the container.
 * @param type - reference an item type
 * @return, item quantity
 */
int ItemContainer::getItemQty(const std::string& type) const
{
	if (type.empty() == false)
	{
		std::map<std::string, int>::const_iterator i = _contents.find(type);
		if (i != _contents.end())
			return i->second;
	}

	return 0;
}

/**
 * Returns the total quantity of the items in the container.
 * @return, total item quantity
 */
int ItemContainer::getTotalQuantity() const
{
	int total = 0;
	for (std::map<std::string, int>::const_iterator
			i = _contents.begin();
			i != _contents.end();
			++i)
	{
		total += i->second;
	}

	return total;
}

/**
 * Returns the total size (storage units) of the items in the container.
 * @param rules - pointer to Ruleset
 * @return, total item size
 */
double ItemContainer::getTotalSize(const Ruleset* const rules) const
{
	double total = 0;
	for (std::map<std::string, int>::const_iterator
			i = _contents.begin();
			i != _contents.end();
			++i)
	{
		total += rules->getItem(i->first)->getSize() * i->second;
	}

	return total;
}

/**
 * Returns all the items currently contained within.
 * @return, pointer to the map of contents
 */
std::map<std::string, int>* ItemContainer::getContents()
{
	return &_contents;
}

}
