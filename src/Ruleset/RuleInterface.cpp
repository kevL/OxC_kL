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

#include "RuleInterface.h"

//#include <climits>


namespace OpenXcom
{

/**
 * Creates a blank ruleset for a certain type of interface,
 * containing an index of elements that make it up.
 * @param type - reference the string defining the type
 */
RuleInterface::RuleInterface(const std::string& type)
	:
		_type(type)
{}

/**
 * dTor.
 */
RuleInterface::~RuleInterface()
{}

/**
 * Loads the elements from a YAML file.
 * @param node - reference a YAML node
 */
void RuleInterface::load(const YAML::Node& node)
{
	_palette	= node["palette"]	.as<std::string>(_palette);
	_parent		= node["parent"]	.as<std::string>(_parent);

	for (YAML::const_iterator
			i = node["elements"].begin();
			i != node["elements"].end();
			++i)
	{
		Element element;

		if ((*i)["pos"])
		{
			const std::pair<int, int> pos = (*i)["pos"].as<std::pair<int, int> >();
			element.x = pos.first;
			element.y = pos.second;
		}
		else
			element.x =
			element.y = std::numeric_limits<int>::max(); // note, these could conceivably be "-1"

		if ((*i)["size"])
		{
			const std::pair<int, int> pos = (*i)["size"].as<std::pair<int, int> >();
			element.w = pos.first;
			element.h = pos.second;
		}
		else
			element.w =
			element.h = -1;

		element.color		= (*i)["color"]		.as<int>(-1);
		element.color2		= (*i)["color2"]	.as<int>(-1);
		element.border		= (*i)["border"]	.as<int>(-1);
		element.TFTDMode	= (*i)["TFTDMode"]	.as<bool>(false);

		std::string id = (*i)["id"].as<std::string>("");
		_elements[id] = element;
	}
}

/**
 * Retrieves info on an element
 * @param id - a string defining the element
 * @return, pointer to Element
 */
const Element* const RuleInterface::getElement(const std::string id) const // <- why i hate const. There is likely NO optimization done despite this.
{
	const std::map<std::string, Element>::const_iterator i = _elements.find(id);
	if (i != _elements.end())
		return &i->second;

	return NULL;
}

/**
 * Gets this Interface's palette.
 * @return, reference to palette
 */
const std::string& RuleInterface::getPalette() const
{
	return _palette;
}

/**
 * Gets this Interface's parent state.
 * return, reference to parent state
 */
const std::string& RuleInterface::getParent() const
{
	return _parent;
}

}
