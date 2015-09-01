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

#ifndef OPENXCOM_REGION_H
#define OPENXCOM_REGION_H

//#include <vector>
//#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

class RuleRegion;

/**
 * Represents a region of the world.
 * @note Contains variable info about a region like X-Com and alien activity.
 */
class Region
{

private:
	int
		_activityRecent,
		_activityRecentXCom;

	RuleRegion* _rules;

	std::vector<int>
		_activityAlien,
		_activityXcom;


	public:
		/// Creates a new region of the specified type.
		explicit Region(RuleRegion* rules);
		/// Cleans up the region.
		~Region();

		/// Loads the region from YAML.
		void load(const YAML::Node& node);
		/// Saves the region to YAML.
		YAML::Node save() const;

		/// Gets the region's ruleset.
		RuleRegion* getRules() const;
		/// Get the region's name.
		std::string getType() const;

		/// Adds xcom activity in this region.
		void addActivityXCom(int activity);
		/// Adds alien activity in this region.
		void addActivityAlien(int activity);
		/// Gets xcom activity for this region.
		std::vector<int>& getActivityXCom();
		/// Gets xcom activity for this region.
		std::vector<int>& getActivityAlien();

		/// Stores last month's counters, starts new counters.
		void newMonth();

		/// Handles recent alien activity in this region for GraphsState blink.
		bool recentActivity(
				bool activity = true,
				bool graphs = false);
		/// Handles recent XCOM activity in this region for GraphsState blink.
		bool recentActivityXCom(
				bool activity = true,
				bool graphs = false);
		/// Resets activity.
		void resetActivity();
};

}

#endif
