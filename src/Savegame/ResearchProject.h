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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_RESEARCHPROJECT_H
#define OPENXCOM_RESEARCHPROJECT_H

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

class RuleResearch;
class Ruleset;


/**
 * Represents a ResearchProject.
 * Contains information about assigned scientists, time already spent and cost of the project.
*/
class ResearchProject
{

private:
	bool _offline; // kL
	int
		_assigned,
		_cost,
		_spent;

	RuleResearch* _project;


	public:
		ResearchProject(
				RuleResearch* project,
				int cost = 0);
		// kL_note: DESTRUCTOR?

		/// Game logic. Called every new day to compute time spent.
		bool step();

		/// get the ResearchProject Ruleset
		const RuleResearch* getRules() const;

		/// set the number of scientist assigned to this ResearchProject
		void setAssigned(int nb);
		/// get the number of scientist assigned to this ResearchProject
		int getAssigned() const;
		/// set time already spent on this ResearchProject
		void setSpent(int spent);
		/// get time already spent on this ResearchProject
		int getSpent() const;
		/// set time cost of this ResearchProject
		void setCost(int cost);
		/// get time cost of this ResearchProject
		int getCost() const;
		/// kL. Sets the project offline.
		void setOffline(bool offline = true); // kL
		/// kL. Gets whether the project is offline or not.
		bool getOffline() const; // kL

		/// load the ResearchProject from YAML
		void load(const YAML::Node& node);
		/// save the ResearchProject to YAML
		YAML::Node save() const;

		/// Get a string describing current progress.
		std::string getResearchProgress() const;
		/// Get a string of cost completed as a per cent value.
		std::wstring getCostCompleted() const; // kL
};

}

#endif
