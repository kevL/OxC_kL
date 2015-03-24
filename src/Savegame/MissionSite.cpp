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

#include "MissionSite.h"

//#include <sstream>

#include "../Engine/Language.h"

#include "../Geoscape/Globe.h" // Globe::GM_MISSIONSITE

#include "../Ruleset/AlienDeployment.h"
#include "../Ruleset/RuleAlienMission.h"


namespace OpenXcom
{

/**
 * Initializes a mission site.
 */
MissionSite::MissionSite(
		const RuleAlienMission* rules,
		const AlienDeployment* deployment)
	:
		Target(),
		_missionRule(rules),
		_deployment(deployment),
		_id(0),
		_texture(-1),
		_secondsLeft(0),
		_inTactical(false)
{}

/**
 * dTor.
 */
MissionSite::~MissionSite()
{}

/**
 * Loads the mission site from a YAML file.
 * @param node - reference a YAML node
 */
void MissionSite::load(const YAML::Node& node)
{
	Target::load(node);

	// NOTE: "type" & "deployment" loaded by SavedGame and passed into cTor.
	_id				= node["id"]			.as<int>(_id);
	_texture		= node["texture"]		.as<int>(_texture);
	_secondsLeft	= node["secondsLeft"]	.as<int>(_secondsLeft);
	_race			= node["race"]			.as<std::string>(_race);
	_inTactical		= node["inTactical"]	.as<bool>(_inTactical);
}

/**
 * Saves the mission site to a YAML file.
 * @return, YAML node
 */
YAML::Node MissionSite::save() const
{
	YAML::Node node = Target::save();

	node["type"]			= _missionRule->getType();
	node["deployment"]		= _deployment->getType();

	node["id"]				= _id;
	node["texture"]			= _texture;
	node["race"]			= _race;

	if (_secondsLeft != 0)
		node["secondsLeft"]	= _secondsLeft;
	if (_inTactical == true)
		node["inTactical"]	= _inTactical;

	return node;
}

/**
 * Saves the mission site's unique identifiers to a YAML file.
 * @return, YAML node
 */
YAML::Node MissionSite::saveId() const
{
	YAML::Node node = Target::saveId();

	node["type"]	= _deployment->getMarkerName();
	node["id"]		= _id;

	return node;
}

/**
 * Returns the ruleset for the mission's type.
 * @return, pointer to RuleAlienMission
 */
const RuleAlienMission* MissionSite::getRules() const
{
	return _missionRule;
}

/**
 * Returns the ruleset for the mission's deployment.
 * @return, pointer to AlienDeployment rules
 */
const AlienDeployment* MissionSite::getDeployment() const
{
	return _deployment;
}

/**
 * Returns the mission site's unique ID.
 * @return, unique ID
 */
int MissionSite::getId() const
{
	return _id;
}

/**
 * Changes the mission site's unique ID.
 * @param id - unique ID
 */
void MissionSite::setId(const int id)
{
	_id = id;
}

/**
 * Returns the mission site's unique identifying name.
 * @param lang - pointer to Language to get strings from
 * @return, full name
 */
std::wstring MissionSite::getName(Language* lang) const
{
	return lang->getString(_deployment->getMarkerName()).arg(_id);
}

/**
 * Returns the globe marker for the mission site (default 5 if no marker is specified).
 * @return, marker sprite #5 (or special Deployment icon)
 */
int MissionSite::getMarker() const
{
	if (_deployment->getMarkerIcon() == -1)
		return Globe::GM_MISSIONSITE;

	return _deployment->getMarkerIcon();
}

/**
 * Returns the number of seconds remaining before the mission site expires.
 * @return, seconds remaining
 */
int MissionSite::getSecondsLeft() const
{
	return _secondsLeft;
}

/**
 * Changes the number of seconds before the mission site expires.
 * @param sec - time in seconds
 */
void MissionSite::setSecondsLeft(int sec)
{
	_secondsLeft = std::max(0, sec);
}

/**
 * Gets this MissionSite's battlescape status.
 * @return, true if in the battlescape
 */
bool MissionSite::isInBattlescape() const
{
	return _inTactical;
}

/**
 * Sets this MissionSite's battlescape status.
 * @param inTactical - true if in the battlescape
 */
void MissionSite::setInBattlescape(bool inTactical)
{
	_inTactical = inTactical;
}

/**
 * Returns the alien race currently residing in the mission site.
 * @return, alien race
 */
std::string MissionSite::getAlienRace() const
{
	return _race;
}

/**
 * Changes the alien race currently residing in the mission site.
 * @param race - reference the alien race string
 */
void MissionSite::setAlienRace(const std::string& race)
{
	_race = race;
}

/**
 * Gets this MissionSite's terrainType.
 * @return, terrain type string
 */
std::string MissionSite::getSiteTerrainType() const
{
	return _terrain;
}

/**
 * Sets this MissionSite's terrainType.
 * @param terrain - reference the terrain string
 */
void MissionSite::setSiteTerrainType(const std::string& terrain)
{
	_terrain = terrain;
}

/**
 * Gets the mission site's associated texture.
 * @return, the texture ID
 */
int MissionSite::getSiteTextureInt() const
{
	return _texture;
}

/**
 * Sets the mission site's associated texture.
 * @param texture - the texture ID
 */
void MissionSite::setSiteTextureInt(int texture)
{
	_texture = texture;
}

/**
 * Gets the mission site's associated City if any.
 * @return, string ID for the city; "" if none
 */
std::string MissionSite::getCity() const
{
	return _city;
}

/**
 * Sets the mission site's associated City if any.
 * @param city - reference the string ID for a city; "" if none
 */
void MissionSite::setCity(const std::string& city)
{
	_city = city;
}

}
