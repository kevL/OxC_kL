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

#include "AlienDeployment.h"


namespace YAML
{

template<>
struct convert<OpenXcom::ItemSet>
{
	///
	static Node encode(const OpenXcom::ItemSet& rhs)
	{
		Node node;
		node = rhs.items;

		return node;
	}

	///
	static bool decode(const Node& node, OpenXcom::ItemSet& rhs)
	{
		if (node.IsSequence() == false)
			return false;

		rhs.items = node.as<std::vector<std::string> >(rhs.items);

		return true;
	}
};

template<>
struct convert<OpenXcom::DeploymentData>
{
	///
	static Node encode(const OpenXcom::DeploymentData& rhs)
	{
		Node node;

		node["alienRank"]		= rhs.alienRank;
		node["lowQty"]			= rhs.lowQty;
		node["highQty"]			= rhs.highQty;
		node["dQty"]			= rhs.dQty;
		node["extraQty"]		= rhs.extraQty;
		node["pctOutsideUfo"]	= rhs.pctOutsideUfo;
		node["itemSets"]		= rhs.itemSets;

		return node;
	}

	///
	static bool decode(
			const Node& node,
			OpenXcom::DeploymentData& rhs)
	{
		if (node.IsMap() == false)
			return false;

		rhs.alienRank		= node["alienRank"]		.as<int>(rhs.alienRank);
		rhs.lowQty			= node["lowQty"]		.as<int>(rhs.lowQty);
		rhs.highQty			= node["highQty"]		.as<int>(rhs.highQty);
		rhs.dQty			= node["dQty"]			.as<int>(rhs.dQty);
		rhs.extraQty		= node["extraQty"]		.as<int>(0); // give this a default, as it's not 100% needed, unlike the others.
		rhs.pctOutsideUfo	= node["pctOutsideUfo"]	.as<int>(rhs.pctOutsideUfo);
		rhs.itemSets		= node["itemSets"]		.as<std::vector<OpenXcom::ItemSet> >(rhs.itemSets);

		return true;
	}
};

template<>
struct convert<OpenXcom::BriefingData>
{
	///
	static Node encode(const OpenXcom::BriefingData& rhs)
	{
		Node node;

		node["palette"]			= rhs.palette;
		node["textOffset"]		= rhs.textOffset;
		node["title"]			= rhs.title;
		node["desc"]			= rhs.desc;
		node["music"]			= rhs.music;
		node["background"]		= rhs.background;
		node["showCraftText"]	= rhs.showCraftText;
		node["showTargetText"]	= rhs.showTargetText;

		return node;
	}

	///
	static bool decode(const Node& node, OpenXcom::BriefingData& rhs)
	{
		if (node.IsMap() == false)
			return false;

		rhs.palette			= node["palette"]		.as<int>(rhs.palette);
		rhs.textOffset		= node["textOffset"]	.as<int>(rhs.textOffset);
		rhs.title			= node["title"]			.as<std::string>(rhs.title);
		rhs.desc			= node["desc"]			.as<std::string>(rhs.desc);
		rhs.music			= node["music"]			.as<std::string>(rhs.music);
		rhs.background		= node["background"]	.as<std::string>(rhs.background);
		rhs.showCraftText	= node["showCraftText"]	.as<bool>(rhs.showCraftText);
		rhs.showTargetText	= node["showTargetText"].as<bool>(rhs.showTargetText);

		return true;
	}
};

}


namespace OpenXcom
{

/**
 * Creates a blank ruleset for a certain type of deployment data.
 * @param type - reference a string defining the type of AlienMission
 */
AlienDeployment::AlienDeployment(const std::string& type)
	:
		_type(type),
		_width(0),
		_length(0),
		_height(0),
		_civilians(0),
		_shade(-1),
		_noRetreat(false),
		_finalDestination(false),
		_finalMission(false),
		_markerIcon(-1),
		_durationMin(0),
		_durationMax(0),
		_objectiveType(STT_NONE),
		_objectivesReqd(0),
		_objectiveCompleteScore(0),
		_objectiveFailedScore(0),
		_despawnPenalty(0),
		_pointsPer30(0),
		_markerType("STR_TERROR_SITE"),
		_alert("STR_ALIENS_TERRORISE"),
		_alertBg("BACK03.SCR")
{}

/**
 * dTor.
 */
AlienDeployment::~AlienDeployment()
{}

/**
 * Loads the Deployment from a YAML file.
 * @param node - reference a YAML node
 */
void AlienDeployment::load(const YAML::Node& node)
{
	_type				= node["type"]				.as<std::string>(_type);
	_data				= node["data"]				.as<std::vector<DeploymentData> >(_data);
	_width				= node["width"]				.as<int>(_width);
	_length				= node["length"]			.as<int>(_length);
	_height				= node["height"]			.as<int>(_height);
	_civilians			= node["civilians"]			.as<int>(_civilians);
	_terrains			= node["terrains"]			.as<std::vector<std::string> >(_terrains);
	_shade				= node["shade"]				.as<int>(_shade);
	_nextStage			= node["nextStage"]			.as<std::string>(_nextStage);
	_race				= node["race"]				.as<std::string>(_race);
	_noRetreat			= node["noRetreat"]			.as<bool>(_noRetreat);
	_finalDestination	= node["finalDestination"]	.as<bool>(_finalDestination);
	_finalMission		= node["finalMission"]		.as<bool>(_finalMission);
	_script				= node["script"]			.as<std::string>(_script);
	_briefingData		= node["briefing"]			.as<BriefingData>(_briefingData);
	_markerType			= node["markerType"]		.as<std::string>(_markerType);
	_markerIcon			= node["markerIcon"]		.as<int>(_markerIcon);
	_alert				= node["alert"]				.as<std::string>(_alert);
	_alertBg			= node["alertBg"]			.as<std::string>(_alertBg);

	if (node["duration"])
	{
		_durationMin = node["duration"][0].as<int>(_durationMin);
		_durationMax = node["duration"][1].as<int>(_durationMax);
	}

	for (YAML::const_iterator
			i = node["music"].begin();
			i != node["music"].end();
			++i)
	{
		_musics.push_back((*i).as<std::string>(""));
	}

	_objectiveType = static_cast<SpecialTileType>(node["objectiveType"].as<int>(_objectiveType));
	_objectivesReqd	= node["objectivesReqd"].as<int>(_objectivesReqd);
	_objectivePopup	= node["objectivePopup"].as<std::string>(_objectivePopup);

	_despawnPenalty	= node["despawnPenalty"].as<int>(_despawnPenalty);
	_pointsPer30	= node["pointsPer30"]	.as<int>(_pointsPer30);

	if (node["objectiveComplete"])
	{
		_objectiveCompleteText	= node["objectiveComplete"][0].as<std::string>(_objectiveCompleteText);
		_objectiveCompleteScore	= node["objectiveComplete"][1].as<int>(_objectiveCompleteScore);
	}

	if (node["objectiveFailed"])
	{
		_objectiveFailedText	= node["objectiveFailed"][0].as<std::string>(_objectiveFailedText);
		_objectiveFailedScore	= node["objectiveFailed"][1].as<int>(_objectiveFailedScore);
	}
}

/**
 * Returns the language string that names this deployment.
 * @note Each deployment type has a unique name.
 * @return, deployment name
 */
const std::string& AlienDeployment::getType() const
{
	return _type;
}

/**
 * Gets a pointer to the data.
 * @return, pointer to a vector holding the DeploymentData
 */
std::vector<DeploymentData>* AlienDeployment::getDeploymentData()
{
	return &_data;
}

/**
 * Gets the dimensions of this deployment's battlefield.
 * @param width		- pointer to width
 * @param lenght	- pointer to length
 * @param heigth	- pointer to height
 */
void AlienDeployment::getDimensions(
		int* width,
		int* lenght,
		int* heigth) const
{
	*width = _width;
	*lenght = _length;
	*heigth = _height;
}

/**
 * Gets the number of civilians.
 * @return, the number of civilians
 */
int AlienDeployment::getCivilians() const
{
	return _civilians;
}

/**
 * Gets eligible terrains for battlescape generation.
 * @return, vector of terrain-type strings
 */
const std::vector<std::string>& AlienDeployment::getDeployTerrains() const
{
	return _terrains;
}

/**
 * Gets the shade level for battlescape generation.
 * @return, the shade level
 */
int AlienDeployment::getShade() const
{
	return _shade;
}

/**
 * Gets the next stage of the mission.
 * @return, the next stage of the mission
 */
const std::string& AlienDeployment::getNextStage() const
{
	return _nextStage;
}

/**
 * Gets the next stage's aLien race.
 * @return, the next stage's alien race
 */
const std::string& AlienDeployment::getRace() const
{
	return _race;
}

/**
 * Gets the script to use to generate a mission of this type.
 * @return, the script to use to generate a mission of this type
 */
const std::string& AlienDeployment::getScript() const
{
	return _script;
}

/**
 * Gets if aborting this mission will fail the game.
 * @return, true if aborting this mission will fail the game
 */
bool AlienDeployment::isNoRetreat() const
{
	return _noRetreat;
}

/**
 * Gets if winning this mission completes the game.
 * @return, true if winning this mission completes the game
 */
bool AlienDeployment::isFinalDestination() const
{
	return _finalDestination;
}

/**
 * Gets if winning this mission completes the game.
 * @return, true if winning this mission completes the game
 */
bool AlienDeployment::isFinalMission() const
{
	return _finalMission;
}

/**
 * Gets the alert message displayed when this mission spawns.
 * @return, ID for the message
 */
const std::string& AlienDeployment::getAlertMessage() const
{
	return _alert;
}

/**
 * Gets the alert background displayed when this mission spawns.
 * @return, ID for the background
 */
const std::string& AlienDeployment::getAlertBackground() const
{
	return _alertBg;
}

/**
 * Gets the briefing data for this mission type.
 * @return, data for the briefing window to use
 */
BriefingData AlienDeployment::getBriefingData() const
{
	return _briefingData;
}

/**
 * Returns the globe marker type for this mission.
 * @return, ID for marker type
 */
const std::string& AlienDeployment::getMarkerType() const
{
	return _markerType;
}

/**
 * Returns the globe marker icon for this mission.
 * @return, marker sprite (-1 if not set)
 */
int AlienDeployment::getMarkerIcon() const
{
	return _markerIcon;
}

/**
 * Returns the minimum duration for this mission type.
 * @return, minimum duration in hours
 */
int AlienDeployment::getDurationMin() const
{
	return _durationMin;
}

/**
 * Returns the maximum duration for this mission type.
 * @return, maximum duration in hours
 */
int AlienDeployment::getDurationMax() const
{
	return _durationMax;
}

/**
 * Gets the list of musics this deployment has to choose from.
 * @return, list of track names
 */
const std::vector<std::string>& AlienDeployment::getDeploymentMusics()
{
	return _musics;
}

/**
 * Gets the objective type for this mission (eg alien control consoles).
 * @return, objective type (RuleItem.h)
 */
SpecialTileType AlienDeployment::getObjectiveType() const
{
	return _objectiveType;
}

/**
 * Gets the number of objectives required by this mission.
 * @return, number of objectives
 */
int AlienDeployment::getObjectivesRequired() const
{
	return _objectivesReqd;
}

/**
 * Gets the string name for the popup to splash when the objective conditions
 * are met.
 * @return, string to pop
 */
const std::string& AlienDeployment::getObjectivePopup() const
{
	return _objectivePopup;
}

/**
 * Fills out the variables associated with mission success and returns if those
 * variables actually contain anything.
 * @param text	- reference to the text to alter
 * @param score	- reference to the score to alter
 * @return, true if anything worthwhile happened
 */
bool AlienDeployment::getObjectiveCompleteInfo(
		std::string& text,
		int& score) const
{
	text = _objectiveCompleteText;
	score = _objectiveCompleteScore;

	return text.empty() == false;
}

/**
 * Fills out the variables associated with mission failure and returns if those
 * variables actually contain anything.
 * @param text	- reference to the text to alter
 * @param score	- reference to the score to alter
 * @return, true if anything worthwhile happened
 */
bool AlienDeployment::getObjectiveFailedInfo(
		std::string& text,
		int& score) const
{
	text = _objectiveFailedText;
	score = _objectiveFailedScore;

	return text.empty() == false;
}

/**
 * Gets the score penalty XCom receives for letting a mission despawn.
 * @return, penalty
 */
int AlienDeployment::getDespawnPenalty() const
{
	return _despawnPenalty;
}

/**
 * Gets the half-hourly score penalty against XCom for this site existing.
 * @return, points for aLiens per 30 mins
 */
int AlienDeployment::getPointsPer30() const
{
	return _pointsPer30;
}

}
