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

#include "RuleCity.h"

//#define _USE_MATH_DEFINES
//#include <math.h>

#include "../Engine/Language.h"

#include "../Geoscape/Globe.h" // Globe::GM_CITY


namespace OpenXcom
{

/**
 * Instantiates a City.
 * @note A City is a 1-pixel MissionArea within a MissionZone as defined in RuleRegion.
 */
RuleCity::RuleCity()
	:
		Target(),
		_zoomLevel(4),
		_labelTop(false),
		_texture(-1)
{
	_lon =
	_lat = 0.;
}

/**
 * Alternate cTor for Cities. Used by TFTD-style rulesets.
 * @param name	- name of the city
 * @param lon	- longitude of the city
 * @param lat	- latitude of the city
 */
RuleCity::RuleCity(
		const std::string& name,
		double lon,
		double lat)
	:
		Target(),
		_name(name),
		_zoomLevel(4),
		_labelTop(false),
		_texture(-1)
{
	_lon = lon;
	_lat = lat;
}

/**
 * dTor.
 */
RuleCity::~RuleCity()
{}

/**
 * Loads the region type from a YAML file.
 * @param node - reference a YAML node
 */
void RuleCity::load(const YAML::Node& node)
{
	_lon		= node["lon"]		.as<double>(_lon) * M_PI / 180.; // radians
	_lat		= node["lat"]		.as<double>(_lat) * M_PI / 180.;
	_name		= node["name"]		.as<std::string>(_name);
	_zoomLevel	= node["zoomLevel"]	.as<size_t>(_zoomLevel);
	_labelTop	= node["labelTop"]	.as<bool>(_labelTop);
	_texture	= node["texture"]	.as<int>(_texture);

	// iDea: _hidden, marker -1 etc.
	// add _zoneType (to specify the missionZone category 0..5+ that City is part of)
}

/**
 * Returns this City's name as seen on the Globe.
 * @param lang - Language to get strings from
 * @return, the city's IG name
 */
std::wstring RuleCity::getName(Language* lang) const
{
	return lang->getString(_name);
}

/**
 * Returns this City's name as a raw string.
 */
std::string RuleCity::getName() const
{
	return _name;
}

/**
 * Returns the latitude coordinate of this City.
 * @return, the city's latitude in radians
 */
double RuleCity::getLatitude() const
{
	return _lat;
}

/**
 * Returns the longitude coordinate of this City.
 * @return, the city's longitude in radians
 */
double RuleCity::getLongitude() const
{
	return _lon;
}

/**
 * Returns the globe marker for this City.
 * @return, marker sprite #8
 */
int RuleCity::getMarker() const
{
	return Globe::GM_CITY;
}

/**
 * Returns the the minimal zoom level that is required to show name of this City on geoscape.
 * @return, required zoom level
 */
size_t RuleCity::getZoomLevel() const
{
	return _zoomLevel;
}

/**
 * Gets if this City's label is to be positioned above or below its marker.
 * @return, true if label goes on top
 */
bool RuleCity::getLabelTop() const
{
	return _labelTop;
}

/**
 * Gets the texture of this City for the battlescape.
 * @return, texture ID
 */
int RuleCity::getTextureInt() const
{
	return _texture;
}

}
