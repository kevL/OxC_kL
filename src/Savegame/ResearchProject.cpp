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

#include "../Interface/Text.h"

#include "../Ruleset/RuleResearch.h"


namespace OpenXcom
{

const float
	PROGRESS_LIMIT_UNKNOWN	= 0.09f,
	PROGRESS_LIMIT_POOR		= 0.23f,
	PROGRESS_LIMIT_AVERAGE	= 0.55f,
	PROGRESS_LIMIT_GOOD		= 0.86f;


/**
 * Constructs a ResearchProject at a Base.
 * @param project	- pointer to RuleResearch
 * @param cost		- the cost to complete the project in man-days (default 0)
 */
ResearchProject::ResearchProject(
		RuleResearch* project,
		int cost)
	:
		_project(project),
		_assigned(0),
		_spent(0),
		_cost(cost),
		_offline(false)
{}

/**
 * dTor.
 */
ResearchProject::~ResearchProject()
{}

/**
 * Called every day to compute time spent on this ResearchProject.
 * @return, true if project finishes on the step
 */
bool ResearchProject::step()
{
	_spent += _assigned;

	if (_spent >= _cost)
		return true;

	return false;
}

/**
 * Gets this ResearchProject's rule.
 * @return, pointer to RuleResearch
 */
const RuleResearch* ResearchProject::getRules() const
{
	return _project;
}

/**
 * Changes the number of scientists to this ResearchProject.
 * @param qty - quantity of scientists assigned
 */
void ResearchProject::setAssigned(const int qty)
{
	_assigned = qty;
}

/**
 * Returns the number of scientists assigned to this ResearchProject.
 * @return, quantity of scientists assigned
 */
int ResearchProject::getAssigned() const
{
	return _assigned;
}

/**
 * Changes the cost of this ResearchProject.
 * @param spent - new project cost in man-days
 */
void ResearchProject::setSpent(const int spent)
{
	_spent = spent;
}

/**
 * Returns the time already spent on this ResearchProject.
 * @return, the time already spent in man-days
 */
int ResearchProject::getSpent() const
{
	return _spent;
}

/**
 * Changes the cost of this ResearchProject.
 * @param cost - new project cost in man-days
 */
void ResearchProject::setCost(const int cost)
{
	_cost = cost;
}

/**
 * Returns the cost of this ResearchProject.
 * @return, the cost in man-days
 */
int ResearchProject::getCost() const
{
	return _cost;
}

/**
 * Sets this ResearchProject offline.
 * Used to remove the project from lists while preserving the cost & spent values.
 * @param offline - true to set project offline
 */
void ResearchProject::setOffline(const bool offline)
{
	_offline = offline;
}

/**
 * Gets whether this ResearchProject is offline or not.
 * @return, true if project is offline
 */
bool ResearchProject::getOffline() const
{
	return _offline;
}

/**
 * Loads the ResearchProject from a YAML file.
 * @param node - reference a YAML node
 */
void ResearchProject::load(const YAML::Node& node)
{
	_assigned	= node["assigned"]	.as<int>(_assigned);
	_spent		= node["spent"]		.as<int>(_spent);
	_cost		= node["cost"]		.as<int>(_cost);
	_offline	= node["offline"]	.as<bool>(_offline);

}

/**
 * Saves this ResearchProject to a YAML file.
 * @return, YAML node
 */
YAML::Node ResearchProject::save() const
{
	YAML::Node node;

	node["project"]		= _project->getName();
	node["assigned"]	= _assigned;
	node["spent"]		= _spent;
	node["cost"]		= _cost;
	node["offline"]		= _offline;

	return node;
}

/**
 * Returns a string describing Research progress.
 * @return, description of research progress
*/
std::string ResearchProject::getResearchProgress() const
{
/*	if (_assigned == 0)
		return "STR_NONE";

	const float progress = static_cast<float>(_spent) / static_cast<float>(_cost);

	if (progress < PROGRESS_LIMIT_UNKNOWN)	// < 0.1
		return "STR_UNKNOWN";

	if (progress < PROGRESS_LIMIT_POOR)		// < 0.2
		return "STR_POOR";

	if (progress < PROGRESS_LIMIT_AVERAGE)	// < 0.5
		return "STR_AVERAGE";

	if (progress < PROGRESS_LIMIT_GOOD)		// < 0.8
		return "STR_GOOD";

	return "STR_EXCELLENT"; */

	if (_assigned == 0)
		return "na.";
//		return "STR_NONE";

	if (static_cast<float>(_spent) / static_cast<float>(_cost) < PROGRESS_LIMIT_UNKNOWN)
		return "STR_UNKNOWN";

	float rating = static_cast<float>(_assigned);
	rating /= getRules()->getCost();

	if (rating < PROGRESS_LIMIT_POOR)
		return "STR_POOR";

	if (rating < PROGRESS_LIMIT_AVERAGE)
		return "STR_AVERAGE";

	if (rating < PROGRESS_LIMIT_GOOD)
		return "STR_GOOD";

	return "STR_EXCELLENT";
}

/**
 * Returns research time completed as a wide string.
 * @return, time completed
 */
std::wstring ResearchProject::getCostCompleted() const
{
	return Text::formatNumber(_spent);
}

}
