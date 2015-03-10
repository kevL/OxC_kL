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

#ifndef OPENXCOM_CITY_H
#define OPENXCOM_CITY_H

//#include <string>
//#include <yaml-cpp/yaml.h>

#include "../Savegame/Target.h"


namespace OpenXcom
{

class Language;


/**
 * Represents a city of the world.
 * Aliens target cities for certain missions.
 */
class City
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
		City();
		/// Creates a new City at coordinates.
		City(
				const std::string& name,
				double lon,
				double lat);
		/// Cleans up the City.
		~City();

		/// Loads the City from YAML.
		void load(const YAML::Node& node);

		/// Gets the city's name.
		std::wstring getName(Language* lang) const;
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
		int getCityTextureInt() const;
};

}

#endif
