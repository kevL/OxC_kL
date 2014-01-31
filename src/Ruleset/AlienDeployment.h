/*
 * Copyright 2010-2014 OpenXcom Developers.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_ALIENDEPLOYMENT_H
#define OPENXCOM_ALIENDEPLOYMENT_H

#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

class Ruleset;
class RuleTerrain;


struct ItemSet
{
	std::vector<std::string> items;
};


struct DeploymentData
{
	int alienRank;
	int
		lowQty,
		highQty,
		dQty;
	int percentageOutsideUfo;
	std::vector<ItemSet> itemSets;
};


/**
 * Represents a specific type of Alien Deployment.
 * Contains constant info about an Alien Deployment, like the
 * number of aliens for each alien type and what items they carry
 * (itemset depends on alien technology advancement level 0, 1 or 2).
 * - deployment type can be a craft's name, but also alien base or cydonia.
 * - alienRank is used to check which nodeRanks can be used to deploy this unit
 *   + to match to a specific unit (=race/rank combination) that should be deployed.
 * @sa Node
 */
class AlienDeployment
{

private:

	int
		_civilians,
		_height,
		_length,
		_width,
		_shade;

	std::string
		_type,
		_terrain,
		_nextStage;

	std::vector<int> _roadTypeOdds;
	std::vector<DeploymentData> _data;


	public:

		/// Creates a blank Alien Deployment ruleset.
		AlienDeployment(const std::string& type);
		/// Cleans up the Alien Deployment ruleset.
		~AlienDeployment();

		/// Loads Alien Deployment data from YAML.
		void load(const YAML::Node& node);

		/// Gets the Alien Deployment's type.
		std::string getType() const;
		/// Gets a pointer to the data.
		std::vector<DeploymentData>* getDeploymentData();
		/// Gets dimensions.
		void getDimensions(
				int* width,
				int* length,
				int* height);
		/// Gets civilians.
		int getCivilians() const;
		/// Gets road type odds.
		std::vector<int> getRoadTypeOdds() const;
		/// Gets the terrain for battlescape generation.
		std::string getTerrain() const;
		/// Gets the shade level for battlescape generation.
		int getShade() const;
		/// Gets the next stage of the mission.
		std::string getNextStage() const;
};

}


namespace YAML
{
	template<>
	struct convert<OpenXcom::ItemSet>
	{
		static Node encode(const OpenXcom::ItemSet& rhs)
		{
			Node node;
			node = rhs.items;

			return node;
		}

		static bool decode(const Node& node, OpenXcom::ItemSet& rhs)
		{
			if (!node.IsSequence())
				return false;

			rhs.items = node.as< std::vector<std::string> >(rhs.items);

			return true;
		}
	};
}

#endif
