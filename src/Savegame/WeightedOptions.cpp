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

#include "WeightedOptions.h"

//#include <assert.h>

//#include "../Engine/RNG.h"


namespace OpenXcom
{

/**
 * Selects a random choice from among the contents.
 * @note This MUST be called on non-empty objects.
 * @note Each time this is called, the returned value can be different.
 * @return, the key of the selected choice
 */
const std::string WeightedOptions::choose() const
{
	if (_totalWeight == 0)
		return "";

	size_t var = RNG::generate(
							0,
							_totalWeight);

	std::map<std::string, size_t>::const_iterator i = _choices.begin();
	for (
			;
			i != _choices.end();
			++i)
	{
		if (var <= i->second)
			break;

		var -= i->second;
	}

	// This is always a valid iterator here.
	return i->first;
}

/**
 * Selects the most likely option.
 * @note This MUST be called on non-empty objects.
 * @return, the key of the selected choice
 */
const std::string WeightedOptions::topChoice() const
{
	if (_totalWeight == 0)
		return "";

	size_t choice = 0;

	std::map<std::string, size_t>::const_iterator i = _choices.begin();
	for (std::map<std::string, size_t>::const_iterator
			j = _choices.begin();
			j != _choices.end();
			++j)
	{
		if (j->second >= choice)
		{
			choice = j->second;
			i = j;
		}
	}

	return i->first; // This is always a valid iterator here.
}

/**
 * Sets an option's weight.
 * @note If @a weight is set to 0, the option is removed from the list of choices.
 * @note If @a id already exists, the new weight replaces the old one, otherwise
 *		@a id is added to the list of choices, with @a weight as the weight.
 * @param id		- reference the option name
 * @param weight	- the option's new weight
 */
void WeightedOptions::setWeight(
		const std::string& id,
		size_t weight)
{
	std::map<std::string, size_t>::iterator option = _choices.find(id);
	if (option != _choices.end())
	{
		_totalWeight -= option->second;

		if (weight != 0)
		{
			option->second = weight;
			_totalWeight += weight;
		}
		else
			_choices.erase(option);
	}
	else if (weight != 0)
	{
		_choices.insert(std::make_pair(id, weight));
		_totalWeight += weight;
	}
}

/**
 * Adds the weighted options from a YAML::Node to a WeightedOptions.
 * @note The weight option list is not replaced; only values in @a node will be added / changed.
 * @param node - a YAML node (containing a map) with the new values
 */
void WeightedOptions::load(const YAML::Node& node)
{
	for (YAML::const_iterator
			val = node.begin();
			val != node.end();
			++val)
	{
		const std::string id = val->first.as<std::string>();
		const size_t weight = val->second.as<size_t>();

		setWeight(id, weight);
	}
}

/**
 * Sends the WeightedOption contents to a YAML::Emitter.
 * @return, YAML node
 */
YAML::Node WeightedOptions::save() const
{
	YAML::Node node;

	for (std::map<std::string, size_t>::const_iterator
			i = _choices.begin();
			i != _choices.end();
			++i)
	{
		node[i->first] = i->second;
	}

	return node;
}

}
