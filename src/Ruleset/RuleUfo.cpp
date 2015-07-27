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

#include "RuleUfo.h"

#include "RuleTerrain.h"


namespace OpenXcom
{

/**
 * Creates a blank ruleset for a certain type of UFO.
 * @param type - reference string defining the type
 */
RuleUfo::RuleUfo(const std::string& type)
	:
		_type(type),
		_size("STR_VERY_SMALL"),
		_sprite(-1),
		_marker(-1),
		_damageMax(0),
		_speedMax(0),
		_accel(0),
		_power(0),
		_range(0),
		_score(0),
		_reload(0),
		_escape(0),
		_reconRange(600),
		_tacticalTerrainData(NULL)
{}

/**
 * dTor.
 */
RuleUfo::~RuleUfo()
{
	delete _tacticalTerrainData;
}

/**
 * Loads the UFO from a YAML file.
 * @param node	- reference a YAML node
 * @param rules	- pointer to Ruleset
 */
void RuleUfo::load(
		const YAML::Node& node,
		Ruleset* const rules)
{
	_type			= node["type"]		.as<std::string>(_type);
	_size			= node["size"]		.as<std::string>(_size);
	_sprite			= node["sprite"]	.as<int>(_sprite);
	_marker			= node["marker"]	.as<int>(_marker);
	_damageMax		= node["damageMax"]	.as<int>(_damageMax);
	_speedMax		= node["speedMax"]	.as<int>(_speedMax);
	_accel			= node["accel"]		.as<int>(_accel);
	_power			= node["power"]		.as<int>(_power);
	_range			= node["range"]		.as<int>(_range);
	_score			= node["score"]		.as<int>(_score);
	_reload			= node["reload"]	.as<int>(_reload);
	_escape			= node["escape"]	.as<int>(_escape);
	_reconRange		= node["reconRange"].as<int>(_reconRange);
	_modSprite		= node["modSprite"]	.as<std::string>(_modSprite);

	if (const YAML::Node& terrain = node["battlescapeTerrainData"])
	{
		RuleTerrain* const terrainRule = new RuleTerrain(terrain["name"].as<std::string>());
		terrainRule->load(
						terrain,
						rules);
		_tacticalTerrainData = terrainRule;
	}
}

/**
 * Gets the language string that names this UFO.
 * Each UFO type has a unique name.
 * @return, the Ufo's type
 */
std::string RuleUfo::getType() const
{
	return _type;
}

/**
 * Gets the size of this type of UFO.
 * @return, the Ufo's size
 */
std::string RuleUfo::getSize() const
{
	return _size;
}

/**
 * Gets the radius of this type of UFO on the dogfighting window.
 * @return, the radius in pixels
 */
int RuleUfo::getRadius() const
{
	if (_size == "STR_VERY_LARGE")
		return 4;

	if (_size == "STR_LARGE")
		return 3;

	if (_size == "STR_MEDIUM_UC")
		return 2;

	if (_size == "STR_SMALL")
		return 1;

//	if (_size == "STR_VERY_SMALL")
	return 0;
}

/**
 * Gets the ID of the sprite used to draw the UFO in the Dogfight window.
 * @return, the sprite ID
 */
int RuleUfo::getSprite() const
{
	return _sprite;
}

/**
 * Returns the globe marker for the UFO type.
 * @return, marker sprite -1 if none
 */
int RuleUfo::getMarker() const
{
	return _marker;
}

/**
 * Gets the maximum damage (damage the UFO can take) of the UFO.
 * @return, the maximum damage
 */
int RuleUfo::getMaxDamage() const
{
	return _damageMax;
}

/**
 * Gets the maximum speed of the UFO flying around the Geoscape.
 * @return, the maximum speed
 */
int RuleUfo::getMaxSpeed() const
{
	return _speedMax;
}

/**
 * Gets the acceleration of the UFO for taking off or stopping.
 * @return, the acceleration
 */
int RuleUfo::getAcceleration() const
{
	return _accel;
}

/**
 * Gets the maximum damage done by the UFO's weapons per shot.
 * @return, the weapon power
 */
int RuleUfo::getWeaponPower() const
{
	return _power;
}

/**
 * Gets the maximum range for the UFO's weapons.
 * @return, the weapon range
 */
int RuleUfo::getWeaponRange() const
{
	return _range;
}

/**
 * Gets the amount of points the player gets for shooting down the UFO.
 * @return, the score
 */
int RuleUfo::getScore() const
{
	return _score;
}

/**
 * Gets the terrain data needed to draw the UFO in the battlescape.
 * @return, pointer to the RuleTerrain
 */
RuleTerrain* RuleUfo::getBattlescapeTerrainData() const
{
	return _tacticalTerrainData;
}

/**
 * Gets the UFO's weapon reload time.
 * @return, ticks
 */
int RuleUfo::getWeaponReload() const
{
	return _reload;
}

/**
 * Gets the UFO's break off time.
 * @return, ticks
 */
int RuleUfo::getEscapeTime() const
{
	return _escape;
}

/**
 * For user-defined UFOs use a surface for the preview image.
 * @return, the string-ID of the surface that represents the UFO
 */
std::string RuleUfo::getModSprite() const
{
	return _modSprite;
}

/**
 * Gets the UFO's radar range for detecting bases.
 * @return, the range in nautical miles
 */
int RuleUfo::getReconRange() const
{
	return _reconRange;
}

}
