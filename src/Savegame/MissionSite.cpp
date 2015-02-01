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

#include "../Ruleset/RuleAlienMission.h"


namespace OpenXcom
{

/**
 * Initializes a mission site.
 */
MissionSite::MissionSite(const RuleAlienMission* rules)
	:
		Target(),
		_rules(rules),
		_id(0),
		_secondsRemaining(0),
		_inBattlescape(false)
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

	_id					= node["id"].as<int>(_id);
	_secondsRemaining	= node["secondsRemaining"].as<int>(_secondsRemaining);
	_race				= node["race"].as<std::string>(_race);
	_inBattlescape		= node["inBattlescape"].as<bool>(_inBattlescape);
}

/**
 * Saves the mission site to a YAML file.
 * @return, YAML node
 */
YAML::Node MissionSite::save() const
{
	YAML::Node node = Target::save();

	node["type"]					= _rules->getType();
	node["id"]						= _id;
	if (_secondsRemaining)
		node["secondsRemaining"]	= _secondsRemaining;
	node["race"]					= _race;
	if (_inBattlescape)
		node["inBattlescape"]		= _inBattlescape;

	return node;
}

/**
 * Saves the mission site's unique identifiers to a YAML file.
 * @return, YAML node
 */
YAML::Node MissionSite::saveId() const
{
	YAML::Node node = Target::saveId();

	node["type"]	= _rules->getType();
	node["id"]		= _id;

	return node;
}

/**
 * Returns the ruleset for the mission's type.
 * @return, pointer to RuleAlienMission
 */
const RuleAlienMission* MissionSite::getRules() const
{
	return _rules;
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
	return lang->getString(_rules->getMarkerName()).arg(_id);
}

/**
 * Returns the globe marker for the mission site.
 * @return, marker sprite (-1 none)
 */
int MissionSite::getMarker() const
{
	if (_rules->getMarkerIcon() == -1)
		return 5;

	return _rules->getMarkerIcon();
}

/**
 * Returns the number of seconds remaining before the mission site expires.
 * @return, seconds remaining
 */
int MissionSite::getSecondsRemaining() const
{
	return _secondsRemaining;
}

/**
 * Changes the number of seconds before the mission site expires.
 * @param seconds - seconds remaining
 */
void MissionSite::setSecondsRemaining(int seconds)
{
	_secondsRemaining = std::max(0, seconds);
}

/**
 * Gets this MissionSite's battlescape status.
 * @return, true if in the battlescape
 */
bool MissionSite::isInBattlescape() const
{
	return _inBattlescape;
}

/**
 * Sets this MissionSite's battlescape status.
 * @param inbattle - true if in the battlescape
 */
void MissionSite::setInBattlescape(bool inbattle)
{
	_inBattlescape = inbattle;
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
 */
std::string MissionSite::getTerrain() const
{
	return _terrain;
}

/**
 * Sets this MissionSite's terrainType.
 * @param terrain - reference the terrain string
 */
void MissionSite::setTerrain(const std::string& terrain)
{
	_terrain = terrain;
}

}
