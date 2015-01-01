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

#ifndef OPENXCOM_COUNTRY_H
#define OPENXCOM_COUNTRY_H

//#include <yaml-cpp/yaml.h>


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
		/// Get the country's name.
		std::string getType() const;

		/// Gets the country's funding.
		std::vector<int>& getFunding();
		/// Sets the country's funding.
		void setFunding(int funding);

		/// Gets the country's satisfaction level.
		int getSatisfaction() const;

		/// Adds xcom activity in this country.
		void addActivityXcom(int activity);
		/// Adds alien activity in this country.
		void addActivityAlien(int activity);
		/// Gets xcom activity for this country.
		std::vector<int>& getActivityXcom();
		/// Gets alien activity for this country.
		std::vector<int>& getActivityAlien();

		/// Stores last month's counters, starts new counters, sets this month's change.
		void newMonth(
				const int xcomTotal,
				const int alienTotal,
				const int diff);

		/// Gets if they're signing a new pact w/ aLiens.
		bool getNewPact() const;
		/// Signs a pact at the end of this month.
		void setNewPact();
		/// Gets if they already signed a pact w/ aLiens.
		bool getPact() const;
		/// Signs a pact w/ aLiens immediately!1
		void setPact();

		/// kL. Handles recent alien activity in this country for GraphsState blink.
		bool recentActivity( // kL
				bool activity = true,
				bool graphs = false);
		/// kL. Handles recent XCOM activity in this country for GraphsState blink.
		bool recentActivityXCOM( // kL
				bool activity = true,
				bool graphs = false);
		/// kL. Resets activity.
		void resetActivity(); // kL
};

}

#endif
