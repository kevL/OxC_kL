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

#include "RuleCountry.h"

//#define _USE_MATH_DEFINES
//#include <math.h>

//#include "../Engine/RNG.h"


namespace OpenXcom
{

/**
 * Creates a blank ruleset for a certain type of country.
 * @param type - string defining the type
 */
RuleCountry::RuleCountry(const std::string& type)
	:
		_type(type),
		_fundingBase(0),
		_fundingCap(0),
		_labelLon(0.),
		_labelLat(0.)
{}

/**
 * dTor.
 */
RuleCountry::~RuleCountry()
{}

/**
 * Loads the country type from a YAML file.
 * @param node - reference a YAML node
 */
void RuleCountry::load(const YAML::Node& node)
{
	_type			= node["type"]			.as<std::string>(_type);
	_fundingBase	= node["fundingBase"]	.as<int>(_fundingBase);
	_fundingCap		= node["fundingCap"]	.as<int>(_fundingCap);
	_labelLon		= node["labelLon"]		.as<double>(_labelLon) * M_PI / 180.;
	_labelLat		= node["labelLat"]		.as<double>(_labelLat) * M_PI / 180.;

	std::vector<std::vector<double> > areas;
	areas = node["areas"].as<std::vector<std::vector<double> > >(areas);

	for (size_t
			i = 0;
			i != areas.size();
			++i)
	{
		_lonMin.push_back(areas[i][0] * M_PI / 180.);
		_lonMax.push_back(areas[i][1] * M_PI / 180.);
		_latMin.push_back(areas[i][2] * M_PI / 180.);
		_latMax.push_back(areas[i][3] * M_PI / 180.);

		// safeties ->
//		if (_lonMin.back() > _lonMax.back())
//			std::swap(_lonMin.back(), _lonMax.back());
//		if (_latMin.back() > _latMax.back())
//			std::swap(_latMin.back(), _latMax.back());
	}
}

/**
 * Gets the language string that names this country.
 * @note Each country type has a unique name.
 * @return, the country's name
 */
const std::string& RuleCountry::getType() const
{
	return _type;
}

/**
 * Generates the random starting funding for the country.
 * @return, the monthly funding
 */
int RuleCountry::generateFunding() const
{
	return RNG::generate(
					_fundingBase / 2,
					_fundingBase * 2) * 1000;
}

/**
 * Gets the country's funding cap.
 * @note Country funding can never exceed this.
 * @return, the funding cap in thousands
 */
int RuleCountry::getFundingCap() const
{
	return _fundingCap;
}

/**
 * Gets the longitude of the country's label on the globe.
 * @return, the longitude in radians
 */
double RuleCountry::getLabelLongitude() const
{
	return _labelLon;
}

/**
 * Gets the latitude of the country's label on the globe.
 * @return, the latitude in radians
 */
double RuleCountry::getLabelLatitude() const
{
	return _labelLat;
}

/**
 * Checks if a point is inside this country.
 * @param lon - longitude in radians
 * @param lat - latitude in radians
 * @return, true if inside
 */
bool RuleCountry::insideCountry(
		double lon,
		double lat) const
{
	for (size_t
			i = 0;
			i != _lonMin.size();
			++i)
	{
		bool
			inLon,
			inLat;

		if (_lonMin[i] <= _lonMax[i])
			inLon = (lon >= _lonMin[i] && lon < _lonMax[i]);
		else
			inLon = (lon >= _lonMin[i] && lon < M_PI * 2.)
					|| (lon >= 0. && lon < _lonMax[i]);

		inLat = (lat >= _latMin[i] && lat < _latMax[i]);

		if (inLon && inLat)
			return true;
	}

	return false;
}

}
