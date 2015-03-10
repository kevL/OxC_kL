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

#ifndef OPENXCOM_TEXTURE_H
#define OPENXCOM_TEXTURE_H

//#define _USE_MATH_DEFINES
//#include <math.h>
//#include <string>
//#include <vector>
//#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

struct TerrainCriteria
{
	std::string type;
	int weight;
	double
		lonMin,
		lonMax,
		latMin,
		latMax;

	TerrainCriteria()
		:
			weight(10),
			lonMin(0.),
			lonMax(360.),
			latMin(-90.),
			latMax(90.)
	{}
};

class Target;


/**
 * Represents the relations between a Geoscape texture
 * and the corresponding Battlescape mission attributes.
 */
class Texture
{

private:
	int _id;
	std::string _deployType;
	std::vector<TerrainCriteria> _terrains;


	public:
		/// Creates a new texture with mission data.
		Texture(int id);
		/// Cleans up the texture.
		~Texture();

		/// Loads the texture from YAML.
		void load(const YAML::Node& node);

		/// Gets the list of terrain criteria.
		std::vector<TerrainCriteria>* getTerrainCriteria();

		/// Gets a randomly textured terrain-type for a given target.
		std::string getRandomTerrain(const Target* const target = NULL) const;

		/// Gets the alien deployment for this Texture.
		std::string getTextureDeployment() const;
};

}


namespace YAML
{

template<>
struct convert<OpenXcom::TerrainCriteria>
{
	///
	static Node encode(const OpenXcom::TerrainCriteria& rhs)
	{
		Node node;

		node["type"]	= rhs.type;
		node["weight"]	= rhs.weight;

		std::vector<double> area;
		area.push_back(rhs.lonMin);
		area.push_back(rhs.lonMax);
		area.push_back(rhs.latMin);
		area.push_back(rhs.latMax);

		node["area"] = area;

		return node;
	}

	///
	static bool decode(
			const Node& node,
			OpenXcom::TerrainCriteria& rhs)
	{
		if (node.IsMap() == false)
			return false;

		rhs.type	= node["type"]	.as<std::string>(rhs.type);
		rhs.weight	= node["weight"].as<int>(rhs.weight);

		if (node["area"])
		{
			std::vector<double> area = node["area"].as<std::vector<double> >();
			rhs.lonMin = area[0] * M_PI / 180.;
			rhs.lonMax = area[1] * M_PI / 180.;
			rhs.latMin = area[2] * M_PI / 180.;
			rhs.latMax = area[3] * M_PI / 180.;
		}

		return true;
	}
};

}

#endif
