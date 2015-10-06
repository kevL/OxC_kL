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

//#include "../Engine/RNG.h"


namespace OpenXcom
{

/**
 * Adds the weighted options from a YAML::Node to this WeightedOptions.
 * @note The WeightedOptions list is not replaced; only values in @a node will
 * be added/changed.
 * @param node - reference the YAML node of a map with the new values
 */
void WeightedOptions::load(const YAML::Node& node)
{
	for (YAML::const_iterator
			val = node.begin();
			val != node.end();
			++val)
	{
		const std::string type	(val->first	.as<std::string>());
		const size_t weight		(val->second.as<size_t>());

		setWeight(type, weight);
	}
}

/**
 * Sends this WeightedOptions content to a YAML::Emitter.
 * @return, YAML node
 */
YAML::Node WeightedOptions::save() const
{
	YAML::Node node;
	for (std::map<std::string, size_t>::const_iterator
			i = _options.begin();
			i != _options.end();
			++i)
	{
		node[i->first] = i->second;
	}

	return node;
}

/**
 * Selects a random choice from among the contents.
 * @note This MUST be called on non-empty objects. Each time this is called the
 * returned value can be different.
 * @note kL rewrite
 * @return, the key of the selected choice
 */
std::string WeightedOptions::getOptionResult() const
{
	if (_totalWeight != 0)
	{
		const size_t pick (static_cast<size_t>(RNG::generate(1, // can't use RNG::pick() because 1.
						   static_cast<int>(_totalWeight))));
		size_t tally (0);
		std::map<std::string, size_t>::const_iterator i (_options.begin());
		while (i != _options.end())
		{
			tally += i->second;
			if (pick <= tally)
				return i->first;

			++i;
		}
	}

	return "";
}

/*
 * Selects the most likely option.
 * @note This MUST be called on non-empty objects. Currently used only to
 * determine race of initial alien mission -> GeoscapeState::determineAlienMissions(atGameStart=true)
 * @note kL rewrite
 * @return, the key of the selected choice
 *
std::string WeightedOptions::topChoice() const
{
	if (_totalWeight != 0)
	{
		size_t topWeight = 0;
		std::vector<std::string> rets;
		for (std::map<std::string, size_t>::const_iterator i = _options.begin(); i != _options.end(); ++i)
		{
			if (i->second > topWeight)
			{
				rets.clear();
				rets.push_back(i->first);
				topWeight = i->second;
			}
			else if (i->second != 0 && i->second == topWeight)
				rets.push_back(i->first);
		}
		const size_t pick = RNG::generate(0, rets.size() - 1);
		return rets.at(pick);
	}
	return "";
} */

/**
 * Sets this WeightedOptions' weight.
 * @note If @a weight is set to 0 the option is removed from the list of choices.
 * @note If @a type already exists the new weight replaces the old one; otherwise
 * @a type is added to the list of choices with @a weight as the weight.
 * @param type		- reference the option name
 * @param weight	- the option's new weight
 */
void WeightedOptions::setWeight(
		const std::string& type,
		size_t weight)
{
	std::map<std::string, size_t>::iterator i (_options.find(type));
	if (i != _options.end())
	{
		_totalWeight -= i->second;

		if (weight != 0)
		{
			i->second = weight;
			_totalWeight += weight;
		}
		else
			_options.erase(i);
	}
	else if (weight != 0)
	{
		_options.insert(std::make_pair(type, weight));
		_totalWeight += weight;
	}
}

/**
 * Gets the list of strings associated with these weights.
 * @return, vector of strings in these weights
 */
std::vector<std::string> WeightedOptions::getTypes() const
{
	std::vector<std::string> types;
	for (std::map<std::string, size_t>::const_iterator
			i = _options.begin();
			i != _options.end();
			++i)
	{
		types.push_back((*i).first);
	}

	return types;
}

}
