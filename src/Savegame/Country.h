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

#ifndef OPENXCOM_COUNTRY_H
#define OPENXCOM_COUNTRY_H

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

class RuleCountry;

/**
 * Represents a country that funds the player.
 * Contains variable info about a country like
 * monthly funding and various activities.
 */
class Country
{

private:
	bool
		_newPact,
		_pact;
	int
		_activityRecent,
		_activityRecentXCOM,
		_satisfaction;

	RuleCountry* _rules;

	std::vector<int>
		_activityAlien,
		_activityXcom,
		_funding;


	public:
		/// Creates a new country of the specified type.
		Country(
				RuleCountry* rules,
				bool gen = true);
		/// Cleans up the country.
		~Country();

		/// Loads the country from YAML.
		void load(const YAML::Node& node);
		/// Saves the country to YAML.
		YAML::Node save() const;

		/// Gets the country's ruleset.
		RuleCountry* getRules() const;

		/// kL. Get the country's name.
		std::string getType() const; // kL

		/// Gets the country's funding.
		const std::vector<int>& getFunding() const;
		/// Sets the country's funding.
		void setFunding(int funding);

		/// get the country's satisfaction level
		int getSatisfaction() const;

		/// add xcom activity in this country
		void addActivityXcom(int activity);
		/// add alien activity in this country
		void addActivityAlien(int activity);
		/// get xcom activity in this country
		const std::vector<int>& getActivityXcom() const;
		/// get alien activity in this country
		const std::vector<int>& getActivityAlien() const;

		/// store last month's counters, start new counters, set this month's change.
		void newMonth(
				const int xcomTotal,
				const int alienTotal,
				const int diff);

		/// are we signing a new pact?
		bool getNewPact() const;
		/// sign a pact at the end of this month.
		void setNewPact();
		/// have we signed a pact?
		bool getPact() const;

		/// kL. Handles recent alien activity in this country for GraphsState blink.
		bool recentActivity( // kL
				bool activity = true,
				bool graphs = false);
		/// kL. Handles recent XCOM activity in this country for GraphsState blink.
		bool recentActivityXCOM( // kL
				bool activity = true,
				bool graphs = false);
};

}

#endif
