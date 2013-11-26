/*
 * Copyright 2010-2013 OpenXcom Developers.
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

#include "ResearchProject.h"

#include <algorithm>

#include "../Ruleset/RuleResearch.h"
#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

/*kL const float PROGRESS_LIMIT_UNKNOWN	= 0.333f;
const float PROGRESS_LIMIT_POOR		= 0.008f;
const float PROGRESS_LIMIT_AVERAGE	= 0.14f;
const float PROGRESS_LIMIT_GOOD		= 0.26f; */
/*
const float PROGRESS_LIMIT_UNKNOWN	= 0.2f;		// kL
const float PROGRESS_LIMIT_POOR		= 0.1f;		// kL
const float PROGRESS_LIMIT_AVERAGE	= 0.25f;	// kL
const float PROGRESS_LIMIT_GOOD		= 0.5f;		// kL
*/
const float PROGRESS_LIMIT_UNKNOWN	= 0.09f;		// kL
const float PROGRESS_LIMIT_POOR		= 0.23f;		// kL
const float PROGRESS_LIMIT_AVERAGE	= 0.55f;		// kL
const float PROGRESS_LIMIT_GOOD		= 0.86f;		// kL


ResearchProject::ResearchProject(RuleResearch* project, int cost)
	:
		_project(project),
		_assigned(0),
		_spent(0),
		_cost(cost)
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
	{
		return true;
	}

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
 * @param nb, Number of scientists assigned to this ResearchProject
 */
void ResearchProject::setAssigned(int nb)
{
	_assigned = nb;
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
void ResearchProject::setSpent(int spent)
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
void ResearchProject::setCost(int cost)
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
 * Loads the research project from a YAML file.
 * @param node, YAML node.
 */
void ResearchProject::load(const YAML::Node& node)
{
	setAssigned(node["assigned"].as<int>());
	setSpent(node["spent"].as<int>());
	setCost(node["cost"].as<int>());
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

	return node;
}

/**
 * Return a string describing Research progress.
 * @return, A string describing Research progress.
*/
/*kL std::string ResearchProject::getResearchProgress() const
{
//kL	float progress = (float)getSpent() / getRules()->getCost();
	float progress = (float)getSpent() / (float)getRules()->getCost();	// kL
	Log(LOG_INFO) << "ResearchProject::getResearchProgress(), progress = " << progress;

	if (getAssigned() == 0)
	{
		Log(LOG_INFO) << ". . none assigned.";
		return "STR_NONE";
	}
	else if (progress < PROGRESS_LIMIT_UNKNOWN) // 0.2f
	{
		Log(LOG_INFO) << ". . progress unknown.";
		return "STR_UNKNOWN";
	}
	else
	{
//kL		float rating = (float)getAssigned();
//kL		rating /= getRules()->getCost();
		float rating = (float)getAssigned() / (float)getRules()->getCost();		// kL
		Log(LOG_INFO) << ". . . . rating = " << rating;

		if (rating < PROGRESS_LIMIT_POOR) // 0.1f
		{
			Log(LOG_INFO) << ". . . . . . progress < POOR";
			return "STR_POOR";
		}
		else if (rating < PROGRESS_LIMIT_AVERAGE) // 0.25f
		{
			Log(LOG_INFO) << ". . . . . . progress < AVERAGE";
			return "STR_AVERAGE";
		}
		else if (rating < PROGRESS_LIMIT_GOOD) // 0.5f
		{
			Log(LOG_INFO) << ". . . . . . progress < GOOD";
			return "STR_GOOD";
		}

		Log(LOG_INFO) << ". . . . . . progress = EXCELLENT"; // > 0.5f
		return "STR_EXCELLENT";
	}
} */

// kL_begin: ResearchProject::getResearchProgress(), rewrite so it makes sense.
std::string ResearchProject::getResearchProgress() const
{
//	float progress = static_cast<float>(getSpent()) / static_cast<float>(getRules()->getCost());
	float progress = static_cast<float>(getSpent()) / static_cast<float>(getCost());

//	Log(LOG_INFO) << "ResearchProject::getResearchProgress(), progress = " << progress;
//	Log(LOG_INFO) << "ResearchProject::getResearchProgress(), getSpent() = " << getSpent();
//	Log(LOG_INFO) << "ResearchProject::getResearchProgress(), getRules()->getCost() = " << getRules()->getCost();

	if (getAssigned() == 0)
	{
//		Log(LOG_INFO) << ". . none assigned.";
		return "STR_NONE";
	}
	else if (progress < PROGRESS_LIMIT_UNKNOWN)		// 0.1f
	{
//		Log(LOG_INFO) << ". . progress < unknown.";
		return "STR_UNKNOWN";
	}
	else if (progress < PROGRESS_LIMIT_POOR)		// 0.2f
	{
//		Log(LOG_INFO) << ". . . . . . progress < POOR";
		return "STR_POOR";
	}
	else if (progress < PROGRESS_LIMIT_AVERAGE)		// 0.5f
	{
//		Log(LOG_INFO) << ". . . . . . progress < AVERAGE";
		return "STR_AVERAGE";
	}
	else if (progress < PROGRESS_LIMIT_GOOD)		// 0.8f
	{
//		Log(LOG_INFO) << ". . . . . . progress < GOOD";
		return "STR_GOOD";
	}
	else											// > 0.8f
	{
//		Log(LOG_INFO) << ". . . . . . progress = EXCELLENT";
		return "STR_EXCELLENT";
	}
} // kL_end.

}
