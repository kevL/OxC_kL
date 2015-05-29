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

#ifndef OPENXCOM_RULECITY_H
#define OPENXCOM_RULECITY_H

//#include <string>
//#include <yaml-cpp/yaml.h>

#include "../Savegame/Target.h"


namespace OpenXcom
{

class Language;


/**
 * Represents a city of the world.
 * @note Aliens target cities for certain missions.
 */
class RuleCity
	:
		public Target
{

private:
	bool _labelTop;
	int _texture;
	size_t _zoomLevel;

	std::string _name;


	public:
		/// Creates a new City.
		RuleCity();
		/// Creates a new City at coordinates.
		RuleCity(
				const std::string& name,
				double lon,
				double lat);
		/// Cleans up the City.
		~RuleCity();

		/// Loads the City from YAML.
		void load(const YAML::Node& node);

		/// Gets the city's name.
		std::wstring getName(const Language* const lang) const;
		/// Gets the city's name as a raw string.
		std::string getName() const;

		/// Gets the City's latitude.
		double getLatitude() const;
		/// Gets the City's longitude.
		double getLongitude() const;

		/// Gets the city's marker.
		int getMarker() const;

		/// Gets the level of zoom that shows City's name.
		size_t getZoomLevel() const;

		/// Gets if City's label is above or below its marker.
		bool getLabelTop() const;

		/// Gets the texture of this City for the battlescape.
		int getTextureInt() const;
};

}

#endif
