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

#include "Country.h"

#include "../Engine/Logger.h"
#include "../Engine/RNG.h"

#include "../Ruleset/RuleCountry.h"


namespace OpenXcom
{

/**
 * Initializes a country of the specified type.
 * @param rules Pointer to ruleset.
 * @param gen Generate new funding.
 */
Country::Country(
		RuleCountry* rules,
		bool gen)
	:
		_rules(rules),
		_pact(false),
		_newPact(false),
		_funding(0),
		_satisfaction(2)
{
	if (gen)
		_funding.push_back(_rules->generateFunding());

	_activityAlien.push_back(0);
	_activityXcom.push_back(0);
}

/**
 *
 */
Country::~Country()
{
}

/**
 * Loads the country from a YAML file.
 * @param node YAML node.
 */
void Country::load(const YAML::Node& node)
{
	_funding		= node["funding"].as< std::vector<int> >(_funding);

	_activityXcom	= node["activityXcom"].as< std::vector<int> >(_activityXcom);
	_activityAlien	= node["activityAlien"].as< std::vector<int> >(_activityAlien);

	_pact			= node["pact"].as<bool>(_pact);
	_newPact		= node["newPact"].as<bool>(_newPact);
}

/**
 * Saves the country to a YAML file.
 * @return YAML node.
 */
YAML::Node Country::save() const
{
	YAML::Node node;

	node["type"]			= _rules->getType();
	node["funding"]			= _funding;
	node["activityXcom"]	= _activityXcom;
	node["activityAlien"]	= _activityAlien;

	if (_pact)
		node["pact"]		= _pact;
	else if (_newPact)
		node["newPact"]		= _newPact;

	return node;
}

/**
 * Returns the ruleset for the country's type.
 * @return Pointer to ruleset.
 */
RuleCountry* Country::getRules() const
{
	return _rules;
}

/**
 * kL. Get the country's name.
 */
std::string Country::getType() const // kL
{
	return _rules->getType();
}

/**
 * Returns the country's current monthly funding.
 * @return Monthly funding.
 */
const std::vector<int>& Country::getFunding() const
{
	return _funding;
}

/**
 * Changes the country's current monthly funding.
 * @param funding, Monthly funding.
 */
void Country::setFunding(int funding)
{
	_funding.back() = funding;
}

/*
 * Keith Richards would be so proud
 * @return satisfaction level, 0 = alien pact, 1 = unhappy, 2 = satisfied, 3 = happy.
 */
int Country::getSatisfaction() const
{
	if (_pact)
		return 0;

	return _satisfaction;
}

/**
 * Adds to the country's xcom activity level.
 * @param activity how many points to add.
 */
void Country::addActivityXcom(int activity)
{
	_activityXcom.back() += activity;
}

/**
 * Adds to the country's alien activity level.
 * @param activity how many points to add.
 */
void Country::addActivityAlien(int activity)
{
	_activityAlien.back() += activity;
}

/**
 * Gets the country's xcom activity level.
 * @return activity level.
 */
const std::vector<int>& Country::getActivityXcom() const
{
	return _activityXcom;
}

/**
 * Gets the country's alien activity level.
 * @return activity level.
 */
const std::vector<int>& Country::getActivityAlien() const
{
	return _activityAlien;
}

/**
 * reset all the counters, calculate this month's funding,
 * set the change value for the month.
 * @param xcomTotal, the council's xcom score
 * @param alienTotal, the council's alien score
 */

void Country::newMonth(
		int xcomTotal,
		int alienTotal,
		int diff) // kL
{
	Log(LOG_INFO) << "Country::newMonth()";
	_satisfaction = 2;

	int
		funding = getFunding().back(),

		xCom = (xcomTotal / 10) + _activityXcom.back(),
		aLien = (alienTotal / 20) + _activityAlien.back(),

		oldFunding = _funding.back() / 1000,
		newFunding = (oldFunding * RNG::generate(5, 20) / 100) * 1000;
//		newFunding = _funding.back() * RNG::generate(5, 20) / 100; // kL
			// but earlier formula maintains a nice $1000 multiple.

	// kL_begin:
	if (xCom > aLien + ((diff + 1) * 20)) // country auto. increases funding
	{
		Log(LOG_INFO) << ". auto funding increase";
		int cap = getRules()->getFundingCap() * 1000;

		if (funding + newFunding > cap)
			newFunding = cap - funding;

		if (newFunding)
			_satisfaction = 3;
	}
	else if (xCom - (diff * 20) > aLien) // 50-50 increase/decrease funding
	{
		Log(LOG_INFO) << ". possible funding increase/decrease";
		if (RNG::generate(0, xCom) > aLien)
		{
			Log(LOG_INFO) << ". . funding increase";
			int cap = getRules()->getFundingCap() * 1000;

			if (funding + newFunding > cap)
				newFunding = cap - funding;

			if (newFunding)
				_satisfaction = 3;
		}
		else if (RNG::generate(0, aLien) > xCom
			&& newFunding)
		{
			Log(LOG_INFO) << ". . funding decrease";
			newFunding = -newFunding;
			_satisfaction = 1;
		}
	}
	else if (newFunding) // auto. funding decrease
	{
		Log(LOG_INFO) << ". auto funding decrease";
		newFunding = -newFunding;
		_satisfaction = 1;
	} // kL_end.

/*kL
if (aLien < xCom + 30) // if aLien is above xCom by 30 or less
	{
		if (aLien + 30 < xCom // if aLien is beneath xCom by 30 or more
			&& RNG::generate(0, xCom) > aLien)
		{
			int cap = getRules()->getFundingCap() * 1000;

			if (funding + newFunding > cap)
				newFunding = cap - funding;

			if (newFunding)
				_satisfaction = 3;
		}
	}
	else if (RNG::generate(0, aLien) > xCom
		&& newFunding)
	{
		newFunding = -newFunding;
		_satisfaction = 1;
	} */

	if (_newPact // about to be in cahoots
		&& !_pact)
	{
		_newPact = false;
		_pact = true;

//kL		addActivityAlien(150);
		addActivityAlien(250); // kL
	}

	// set the new funding and reset the activity meters
	if (_pact)
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
 * @return if we will sign a new pact.
 */
bool Country::getNewPact() const
{
	return _newPact;
}

/**
 * sign a new pact at month's end.
 */
void Country::setNewPact()
{
	 _newPact = true;
}

/**
 * no setter for this one, as it gets set automatically
 * at month's end if _newPact is set.
 * @return if we have signed a pact.
 */
bool Country::getPact() const
{
	return _pact;
}

}
