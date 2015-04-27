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

#include "RuleGlobe.h"

//#define _USE_MATH_DEFINES
//#include <cmath>
//#include <fstream>
//#include <SDL_endian.h>

#include "Polygon.h"
#include "Polyline.h"
#include "RuleTexture.h"

//#include "../Engine/CrossPlatform.h"
//#include "../Engine/Exception.h"
//#include "../Engine/Palette.h"

#include "../Geoscape/Globe.h"


namespace OpenXcom
{

/**
 * Creates a blank ruleset for globe contents.
 */
RuleGlobe::RuleGlobe()
{}

/**
 * dTor.
 */
RuleGlobe::~RuleGlobe()
{
	for (std::list<Polygon*>::const_iterator
			i = _polygons.begin();
			i != _polygons.end();
			++i)
	{
		delete *i;
	}

	for (std::list<Polyline*>::const_iterator
			i = _polylines.begin();
			i != _polylines.end();
			++i)
	{
		delete *i;
	}

	for (std::map<int, RuleTexture*>::const_iterator
			i = _textures.begin();
			i != _textures.end();
			++i)
	{
		delete i->second;
	}
}

/**
 * Loads the globe from a YAML file.
 * @param node - reference a YAML node
 */
void RuleGlobe::load(const YAML::Node& node)
{
	if (node["data"])
	{
		for (std::list<Polygon*>::const_iterator
				i = _polygons.begin();
				i != _polygons.end();
				++i)
		{
			delete *i;
		}
		_polygons.clear();

		loadDat(CrossPlatform::getDataFile(node["data"].as<std::string>()));
	}

	if (node["polygons"])
	{
		for (std::list<Polygon*>::const_iterator
				i = _polygons.begin();
				i != _polygons.end();
				++i)
		{
			delete *i;
		}
		_polygons.clear();

		for (YAML::const_iterator
				i = node["polygons"].begin();
				i != node["polygons"].end();
				++i)
		{
			Polygon* polygon = new Polygon(3);
			polygon->load(*i);
			_polygons.push_back(polygon);
		}
	}

	if (node["polylines"])
	{
		for (std::list<Polyline*>::const_iterator
				i = _polylines.begin();
				i != _polylines.end();
				++i)
		{
			delete *i;
		}
		_polylines.clear();

		for (YAML::const_iterator
				i = node["polylines"].begin();
				i != node["polylines"].end();
				++i)
		{
			Polyline* polyline = new Polyline(3);
			polyline->load(*i);
			_polylines.push_back(polyline);
		}
	}

	if (node["textures"])
	{
		for (std::map<int, RuleTexture*>::const_iterator
				i = _textures.begin();
				i != _textures.end();
				++i)
		{
			delete i->second;
		}

		_textures.clear();

		for (YAML::const_iterator
				i = node["textures"].begin();
				i != node["textures"].end();
				++i)
		{
			int id = (*i)["id"].as<int>();
			RuleTexture* texture = new RuleTexture(id);
			texture->load(*i);
			_textures[id] = texture;
		}
	}

	Globe::CLO_LABELBASE	= static_cast<Uint8>(node["baseColor"]		.as<int>(Globe::CLO_LABELBASE));
	Globe::CLO_LABELCITY	= static_cast<Uint8>(node["cityColor"]		.as<int>(Globe::CLO_LABELCITY));
	Globe::CLO_LABELCOUNTRY	= static_cast<Uint8>(node["countryColor"]	.as<int>(Globe::CLO_LABELCOUNTRY));
	Globe::CLO_LINE			= static_cast<Uint8>(node["lineColor"]		.as<int>(Globe::CLO_LINE));			// country lines
//	Globe::CLO_RADAR1		= static_cast<Uint8>(node["radar1Color"]	.as<int>(Globe::CLO_RADAR1));		// base radars
	Globe::CLO_RADAR2		= static_cast<Uint8>(node["radar2Color"]	.as<int>(Globe::CLO_RADAR2));		// craft radars
	Globe::CLO_FLIGHT		= static_cast<Uint8>(node["flightColor"]	.as<int>(Globe::CLO_FLIGHT));		// flight paths
	Globe::CLO_OCEAN		= static_cast<Uint8>(node["oceanPalette"]	.as<int>(Globe::CLO_OCEAN));
//	if (node["oceanPalette"]) Globe::CLO_OCEAN = Palette::blockOffset(static_cast<Uint8>(node["oceanPalette"].as<int>(12)));
}

/**
 * Returns the list of polygons in the globe.
 * @return, pointer to a list of pointers to Polygons
 */
std::list<Polygon*>* RuleGlobe::getPolygons()
{
	return &_polygons;
}

/**
 * Returns the list of polylines in the globe.
 * @return, pointer to a list of pointers to Polylines
 */
std::list<Polyline*>* RuleGlobe::getPolylines()
{
	return &_polylines;
}

/**
 * Loads a series of map polar coordinates in X-Com format,
 * converts them and stores them in a set of polygons.
 * @param filename - filename of DAT file
 * @sa http://www.ufopaedia.org/index.php?title=WORLD.DAT
 */
void RuleGlobe::loadDat(const std::string& filename)
{
	// Load file
	std::ifstream mapFile (filename.c_str(), std::ios::in | std::ios::binary);
	if (mapFile.fail() == true)
	{
		throw Exception(filename + " not found");
	}

	short value[10];

	while (mapFile.read((char*)&value, sizeof(value)))
	{
		Polygon* poly;
		size_t points;

		for (size_t
				i = 0;
				i != 10;
				++i)
		{
			value[i] = SDL_SwapLE16(value[i]);
		}

		if (value[6] != -1)
			points = 4;
		else
			points = 3;

		poly = new Polygon(points);

		size_t j = 0;
		for (size_t
				i = 0;
				i != points;
				++i)
		{
			double // correct X-Com degrees and convert to radians ( 7.25 arc-min = 1 xcomDegree ~or so~ /shrug )
				lonRad = value[j++] * 0.125 * M_PI / 180., // should j double-increment like this
				latRad = value[j++] * 0.125 * M_PI / 180.; // should j double-increment like this

			poly->setLongitude(i, lonRad);
			poly->setLatitude(i, latRad);
		}

		poly->setPolyTexture(value[8]);
		_polygons.push_back(poly);
	}

	if (mapFile.eof() == false)
	{
		throw Exception("Invalid globe map");
	}

	mapFile.close();
}

/**
 * Returns the rule for a Texture.
 * @param id - texture ID
 * @return, rule for a Texture
 */
RuleTexture* RuleGlobe::getTextureRule(int id) const
{
	std::map<int, RuleTexture*>::const_iterator i = _textures.find(id);
	if (i != _textures.end())
		return i->second;

	return NULL;
}

/**
 * Returns a list of all globe terrains associated with a specific AlienDeployment.
 * @note If a blank string is passed in then terrains that are not associated with any AlienDeployment are returned.
 * @param deployType - reference the deployment name (eg. "STR_TERROR_MISSION") (default "" for now ...)
 * @return, vector of terrain-types as strings
 */
std::vector<std::string> RuleGlobe::getGlobeTerrains(const std::string& deployType) const
{
	std::vector<std::string> terrains;

	for (std::map<int, RuleTexture*>::const_iterator
			i = _textures.begin();
			i != _textures.end();
			++i)
	{
		if ((deployType.empty() == true
				&& i->second->getTextureDeployments().empty() == true)
			|| i->second->getTextureDeployments().find(deployType) != i->second->getTextureDeployments().end())
		{
			for (std::vector<TerrainCriteria>::const_iterator
					j = i->second->getTerrainCriteria()->begin();
					j != i->second->getTerrainCriteria()->end();
					++j)
			{
				terrains.push_back(j->type);
			}
		}
	}

	return terrains;
}

}
