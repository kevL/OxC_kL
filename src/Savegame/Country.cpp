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

#include "Country.h"

//#include "../Engine/Logger.h"
//#include "../Engine/RNG.h"

#include "../Ruleset/RuleCountry.h"


namespace OpenXcom
{

/**
 * Initializes a country of the specified type.
 * @param rules		- pointer to RuleCountry
 * @param genFunds	- true to generate new funding (default true)
 */
Country::Country(
		RuleCountry* rules,
		bool genFunds)
	:
		_rules(rules),
		_pact(false),
		_newPact(false),
		_funding(0),
		_satisfaction(2),
		_activityRecent(-1),
		_activityRecentXCOM(-1)
{
	if (genFunds == true)
		_funding.push_back(_rules->generateFunding());

	_activityXcom.push_back(0);
	_activityAlien.push_back(0);
}

/**
 * dTor.
 */
Country::~Country()
{}

/**
 * Loads the country from a YAML file.
 * @param node - reference a YAML node
 */
void Country::load(const YAML::Node& node)
{
	_funding			= node["funding"]			.as<std::vector<int> >(_funding);
	_activityXcom		= node["activityXcom"]		.as<std::vector<int> >(_activityXcom);
	_activityAlien		= node["activityAlien"]		.as<std::vector<int> >(_activityAlien);
	_activityRecent		= node["activityRecent"]	.as<int>(_activityRecent);
	_activityRecentXCOM	= node["activityRecentXCOM"].as<int>(_activityRecentXCOM);
	_pact				= node["pact"]				.as<bool>(_pact);
	_newPact			= node["newPact"]			.as<bool>(_newPact);
}

/**
 * Saves the country to a YAML file.
 * @return, YAML node
 */
YAML::Node Country::save() const
{
	YAML::Node node;

	node["type"]				= _rules->getType();
	node["funding"]				= _funding;
	node["activityXcom"]		= _activityXcom;
	node["activityAlien"]		= _activityAlien;
	node["activityRecent"]		= _activityRecent;
	node["activityRecentXCOM"]	= _activityRecentXCOM;

	if (_pact == true)
		node["pact"]	= _pact;
	else if (_newPact == true)
		node["newPact"]	= _newPact;

	return node;
}

/**
 * Returns the ruleset for the country's type.
 * @return, pointer to RuleCountry
 */
RuleCountry* Country::getRules() const
{
	return _rules;
}

/**
 * Gets the country's name.
 * @return, country name
 */
std::string Country::getType() const
{
	return _rules->getType();
}

/**
 * Returns the country's current monthly funding.
 * @return, reference to a vector of monthly funds
 */
std::vector<int>& Country::getFunding()
{
	return _funding;
}

/**
 * Changes the country's current monthly funding.
 * @param funding - monthly funds
 */
void Country::setFunding(int funding)
{
	_funding.back() = funding;
}

/**
 * Keith Richards would be so proud.
 * @return, satisfaction level
 *			0 = alien pact
 *			1 = unhappy
 *			2 = satisfied
 *			3 = happy
 */
int Country::getSatisfaction() const
{
	if (_pact == true)
		return 0;

	return _satisfaction;
}

/**
 * Adds to the country's xcom activity level.
 * @param activity - how many points to add
 */
void Country::addActivityXcom(int activity)
{
	_activityXcom.back() += activity;
}

/**
 * Adds to the country's alien activity level.
 * @param activity - how many points to add
 */
void Country::addActivityAlien(int activity)
{
	_activityAlien.back() += activity;
}

/**
 * Gets the country's xcom activity level.
 * @return, reference to a vector of activity levels
 */
std::vector<int>& Country::getActivityXcom()
{
	return _activityXcom;
}

/**
 * Gets the country's alien activity level.
 * @return, reference to a vector of activity levels
 */
std::vector<int>& Country::getActivityAlien()
{
	return _activityAlien;
}

/**
 * Resets all the counters, calculates monthly funding, and sets the delta value
 * for the month.
 * @param xcomTotal		- the council's xcom score
 * @param alienTotal	- the council's alien score
 * @param diff			- game difficulty
 */
void Country::newMonth(
		const int xcomTotal,
		const int alienTotal,
		const int diff)
{
	//Log(LOG_INFO) << "Country::newMonth()";
	_satisfaction = 2;

	const int
		xcomPts = (xcomTotal / 10) + _activityXcom.back(),
		alienPts = (alienTotal / 20) + _activityAlien.back(),
		funding = getFunding().back(),
		oldFunding = _funding.back() / 1000;
	int newFunding = (oldFunding * RNG::generate(5,20) / 100) * 1000;

	if (xcomPts > alienPts + ((diff + 1) * 20)) // country auto. increases funding
	{
		//Log(LOG_INFO) << ". auto funding increase";
		const int cap = getRules()->getFundingCap() * 1000;

		if (funding + newFunding > cap)
			newFunding = cap - funding;

		if (newFunding != 0)
			_satisfaction = 3;
	}
	else if (xcomPts - (diff * 20) > alienPts) // 50-50 increase/decrease funding
	{
		//Log(LOG_INFO) << ". possible funding increase/decrease";
		if (RNG::generate(0, xcomPts) > alienPts)
		{
			//Log(LOG_INFO) << ". . funding increase";
			const int cap = getRules()->getFundingCap() * 1000;

			if (funding + newFunding > cap)
				newFunding = cap - funding;

			if (newFunding != 0)
				_satisfaction = 3;
		}
		else if (RNG::generate(0, alienPts) > xcomPts
			&& newFunding != 0)
		{
			//Log(LOG_INFO) << ". . funding decrease";
			newFunding = -newFunding;
			_satisfaction = 1;
		}
	}
	else if (newFunding != 0) // auto. funding decrease
	{
		//Log(LOG_INFO) << ". auto funding decrease";
		newFunding = -newFunding;
		_satisfaction = 1;
	}

	if (_newPact == true // about to be in cahoots
		&& _pact == false)
	{
		_newPact = false;
		_pact = true;
//		addActivityAlien(100 + diff * 50);	// should this be added to Region also.
	}										// Yes it should be .... or taken out here.
											// Let AlienMission or something else take care of it.

	// set the new funding and reset the activity meters
	if (_pact == true)
		_funding.push_back(0);
	else if (_satisfaction != 2)
		_funding.push_back(funding + newFunding);
	else
		_funding.push_back(funding);

	_activityAlien.push_back(0);
	_activityXcom.push_back(0);

	if (_activityAlien.size() > 12)
		_activityAlien.erase(_activityAlien.begin());

	if (_activityXcom.size() > 12)
		_activityXcom.erase(_activityXcom.begin());

	if (_funding.size() > 12)
		_funding.erase(_funding.begin());
}

/**
 * Gets if this country has signed a brand new pact with aLiens.
 * @return, true if this country will sign a new pact with aLiens
 */
bool Country::getNewPact() const
{
	return _newPact;
}

/**
 * Signs a pact with aLiens at month's end.
 */
void Country::setNewPact()
{
	 _newPact = true;
}

/**
 * Gets if this country already has a pact with aLiens.
 * @note There is no setter for this one since it gets set automatically at the
 * end of the month if @a _newPact is true.
 * @return, true if country has a pact with aLiens
 */
bool Country::getPact() const
{
	return _pact;
}

/**
 * Sign a new pact.
 */
void Country::setPact()
{
	 _pact = true;
}

/**
 * Handles recent aLien activity in this country for GraphsState blink.
 * @param activity	- true to reset the startcounter (default true)
 * @param graphs	- not sure lol (default false)
 * @return, true if there is activity
 */
bool Country::recentActivity(
		bool activity,
		bool graphs)
{
	if (activity == true)
		_activityRecent = 0;
	else if (_activityRecent != -1)
	{
		if (graphs == true)
			return true;


		++_activityRecent;

		if (_activityRecent == 24) // aLien bases show activity every 24 hrs.
			_activityRecent = -1;
	}

	if (_activityRecent == -1)
		return false;

	return true;
}

/**
 * Handles recent XCOM activity in this country for GraphsState blink.
 * @param activity	- true to reset the startcounter (default true)
 * @param graphs	- not sure lol (default false)
 * @return, true if there is activity
 */
bool Country::recentActivityXCOM(
		bool activity,
		bool graphs)
{
	if (activity == true)
		_activityRecentXCOM = 0;
	else if (_activityRecentXCOM != -1)
	{
		if (graphs == true)
			return true;


		++_activityRecentXCOM;

		if (_activityRecentXCOM == 24) // aLien bases show activity every 24 hrs.
			_activityRecentXCOM = -1;
	}

	if (_activityRecentXCOM == -1)
		return false;

	return true;
}

/**
 * Resets activity.
 */
void Country::resetActivity()
{
	_activityRecent =
	_activityRecentXCOM = -1;
}

}
