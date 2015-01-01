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

#include "ResearchProject.h"

#include <algorithm>

#include "../Interface/Text.h"

#include "../Ruleset/RuleResearch.h"
#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

const double
	PROGRESS_LIMIT_UNKNOWN	= 0.09,
	PROGRESS_LIMIT_POOR		= 0.23,
	PROGRESS_LIMIT_AVERAGE	= 0.55,
	PROGRESS_LIMIT_GOOD		= 0.86;


ResearchProject::ResearchProject(
		RuleResearch* project,
		int cost)
	:
		_project(project),
		_assigned(0),
		_spent(0),
		_cost(cost),
		_offline(false)
{
}

// kL_note: DESTRUCTOR?

/**
 * Called every day to compute time spent on this ResearchProject
 * @return, True if the ResearchProject is finished
 */
bool ResearchProject::step()
{
	_spent += _assigned;

	if (_spent >= getCost())
		return true;

	return false;
}

/**
 *
 */
const RuleResearch* ResearchProject::getRules() const
{
	return _project;
}

/**
 * Changes the number of scientists to the ResearchProject
 * @param qty - quantity of scientists assigned to this ResearchProject
 */
void ResearchProject::setAssigned(const int qty)
{
	_assigned = qty;
}

/**
 * Returns the number of scientists assigned to this project
 * @return, Number of assigned scientists.
 */
int ResearchProject::getAssigned() const
{
	return _assigned;
}

/**
 * Changes the cost of the ResearchProject
 * @param spent, New project cost(in man/day)
 */
void ResearchProject::setSpent(const int spent)
{
	_spent = spent;
}

/**
 * Returns the time already spent on this project
 * @return, The time already spent on this ResearchProject(in man/day)
 */
int ResearchProject::getSpent() const
{
	return _spent;
}

/**
 * Changes the cost of the ResearchProject
 * @param cost, New project cost(in man/day)
 */
void ResearchProject::setCost(const int cost)
{
	_cost = cost;
}

/**
 * Returns the cost of the ResearchProject
 * @return, The cost of the ResearchProject(in man/day)
 */
int ResearchProject::getCost() const
{
	return _cost;
}

/**
 * kL. Sets the project offline.
 * Used to remove the project from lists, while preserving its cost & spent values.
 * @param cost, New project cost(in man/day)
 */
void ResearchProject::setOffline(const bool offline) // kL
{
	_offline = offline;
}

/**
 * kL. Gets whether the project is offline or not.
 * @return, The cost of the ResearchProject(in man/day)
 */
bool ResearchProject::getOffline() const // kL
{
	return _offline;
}

/**
 * Loads the research project from a YAML file.
 * @param node, YAML node.
 */
void ResearchProject::load(const YAML::Node& node)
{
	setAssigned(node["assigned"].as<int>(getAssigned()));
	setSpent(node["spent"].as<int>(getSpent()));
	setCost(node["cost"].as<int>(getCost()));

	setOffline(node["offline"].as<bool>(getOffline()));
}

/**
 * Saves the research project to a YAML file.
 * @return, YAML node.
 */
YAML::Node ResearchProject::save() const
{
	YAML::Node node;

	node["project"]		= getRules()->getName();
	node["assigned"]	= getAssigned();
	node["spent"]		= getSpent();
	node["cost"]		= getCost();

	node["offline"]		= getOffline();

	return node;
}

/**
 * kL. Returns a string describing Research progress.
 * @return, description of research progress
*/
std::string ResearchProject::getResearchProgress() const
{
	double progress = static_cast<double>(getSpent()) / static_cast<double>(getCost());

	if (getAssigned() == 0)
		return "STR_NONE";
	else if (progress < PROGRESS_LIMIT_UNKNOWN)		// < 0.1
		return "STR_UNKNOWN";
	else if (progress < PROGRESS_LIMIT_POOR)		// < 0.2
		return "STR_POOR";
	else if (progress < PROGRESS_LIMIT_AVERAGE)		// < 0.5
		return "STR_AVERAGE";
	else if (progress < PROGRESS_LIMIT_GOOD)		// < 0.8
		return "STR_GOOD";
	else											// > 0.8
		return "STR_EXCELLENT";
/*kL
	float progress = (float)getSpent() / getRules()->getCost();

	if (getAssigned() == 0)
		return "STR_NONE";
	else if (progress < PROGRESS_LIMIT_UNKNOWN) // 0.2f
		return "STR_UNKNOWN";
	else
	{
		float rating = (float)getAssigned();
		rating /= getRules()->getCost();

		if (rating < PROGRESS_LIMIT_POOR) // 0.1f
			return "STR_POOR";
		else if (rating < PROGRESS_LIMIT_AVERAGE) // 0.25f
			return "STR_AVERAGE";
		else if (rating < PROGRESS_LIMIT_GOOD) // 0.5f
			return "STR_GOOD";

		return "STR_EXCELLENT";
	} */
}

/**
 * kL. Returns research time completed as a wide string.
 * @return, time completed
 */
std::wstring ResearchProject::getCostCompleted() const // kL
{
	return Text::formatNumber(getSpent(), L"", false);
}

}
