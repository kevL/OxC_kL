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

#ifndef OPENXCOM_RULEINTERFACE_H
#define OPENXCOM_RULEINTERFACE_H

//#include <string>
//#include <map>
//#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

struct Element
{
	int
		x,y,
		w,h;
//	Uint8 // <- fucko-fuclo'd
	int
		color,
		color2,
		border;
};


class RuleInterface
{

private:
	std::string
		_palette,
		_parent,
		_type;
	std::map <std::string, Element> _elements;


	public:
		/// Consructor.
		explicit RuleInterface(const std::string& type);
		/// Destructor.
		~RuleInterface();

		/// Loads from YAML.
		void load(const YAML::Node& node);

		/// Gets an element.
		const Element* const getElement(const std::string& id) const;

		/// Gets palette.
		const std::string& getPalette() const;
		/// Gets parent interface rule.
		const std::string& getParent() const;
};

}

#endif
