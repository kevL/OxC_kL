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

#include "AlienBase.h"

//#include <sstream>

#include "../Engine/Language.h"

#include "../Geoscape/Globe.h" // Globe::GM_ALIENBASE


namespace OpenXcom
{

/**
 * Initializes an alien base
 */
AlienBase::AlienBase()
	:
		Target(),
		_id(0),
		_inTactical(false),
		_discovered(false)
{}

/**
 * dTor.
 */
AlienBase::~AlienBase()
{}

/**
 * Loads the alien base from a YAML file.
 * @param node - reference a YAML node
 */
void AlienBase::load(const YAML::Node& node)
{
	Target::load(node);

	_id			= node["id"]		.as<int>(_id);
	_race		= node["race"]		.as<std::string>(_race);
	_edit		= node["edit"]		.as<std::string>(_edit);
	_inTactical	= node["inTactical"].as<bool>(_inTactical);
	_discovered	= node["discovered"].as<bool>(_discovered);
}

/**
 * Saves the alien base to a YAML file.
 * @return, YAML node
 */
YAML::Node AlienBase::save() const
{
	YAML::Node node = Target::save();

	node["id"]		= _id;
	node["race"]	= _race;
	node["edit"]	= _edit;

	if (_inTactical == true)
		node["inTactical"] = _inTactical;
	if (_discovered == true)
		node["discovered"] = _discovered;

	return node;
}

/**
 * Saves the alien base's unique identifiers to a YAML file.
 * @return, YAML node
 */
YAML::Node AlienBase::saveId() const
{
	YAML::Node node = Target::saveId();

	node["type"]	= "STR_ALIEN_BASE";
	node["id"]		= _id;

	return node;
}

/**
 * Returns the alien base's unique ID.
 * @return, unique ID
 */
int AlienBase::getId() const
{
	return _id;
}

/**
 * Changes the alien base's unique ID.
 * @param id - unique ID
 */
void AlienBase::setId(int id)
{
	_id = id;
}

/**
 * Returns the alien base's unique identifying name.
 * @param lang - pointer to Language to get strings from
 * @return, full name
 */
std::wstring AlienBase::getName(Language* lang) const
{
	return lang->getString("STR_ALIEN_BASE_").arg(_id);
}

/**
 * Returns the globe marker for the alien base.
 * @return, marker sprite #7 (-1 if none)
 */
int AlienBase::getMarker() const
{
	if (_discovered == false)
		return -1;

	return Globe::GM_ALIENBASE;
}

/**
 * Returns the alien race currently residing in the alien base.
 * @return, alien race string
 */
std::string AlienBase::getAlienRace() const
{
	return _race;
}

/**
 * Changes the alien race currently residing in the alien base.
 * @param race - reference to alien race string
 */
void AlienBase::setAlienRace(const std::string& race)
{
	_race = race;
}

/**
 * Returns textedit that the player has entered.
 * @return, user text
 */
std::string AlienBase::getLabel() const
{
	return _edit;
}

/**
 * Changes textedit that the player has entered.
 * @param edit - user text
 */
void AlienBase::setLabel(const std::string& edit)
{
	_edit = edit;
}

/**
 * Gets an alien base's battlescape status.
 * @return, true if this base is in the battlescape
 */
bool AlienBase::isInBattlescape() const
{
	return _inTactical;
}

/**
 * Sets an alien base's battlescape status.
 * @param inTactical - true if this base is in battle
 */
void AlienBase::setInBattlescape(bool inTactical)
{
	_inTactical = inTactical;
}

/**
 * Gets an alien base's geoscape status.
 * @return, true if this base has been discovered
 */
bool AlienBase::isDiscovered() const
{
	return _discovered;
}

/**
 * Sets an alien base's discovered status.
 * @param discovered - true if this base has been discovered
 */
void AlienBase::setDiscovered(bool discovered)
{
	_discovered = discovered;
}

}
