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

#include "RuleCraftWeapon.h"


namespace OpenXcom
{

/**
 * Creates a blank ruleset for a certain type of CraftWeapon.
 * @param type - string defining type
 */
RuleCraftWeapon::RuleCraftWeapon(const std::string& type)
	:
		_type(type),
		_sprite(-1),
		_sound(-1),
		_damage(0),
		_range(0),
		_accuracy(0),
		_reloadCautious(0),
		_reloadStandard(0),
		_reloadAggressive(0),
		_ammoMax(0),
		_rearmRate(1),
		_prjSpeed(0),
		_prjType(PT_CANNON_ROUND)
{}

/**
 * dTor.
 */
RuleCraftWeapon::~RuleCraftWeapon()
{}

/**
 * Loads the CraftWeapon from a YAML file.
 * @param node		- reference a YAML node
 * @param modIndex	- a value that offsets the sounds and sprite values to avoid conflicts
 */
void RuleCraftWeapon::load(
		const YAML::Node& node,
		int modIndex)
{
	_type = node["type"].as<std::string>(_type);

	if (node["sprite"])
	{
		_sprite = node["sprite"].as<int>(_sprite);
		if (_sprite > 5) // this one is an offset within INTICONS.PCK
			_sprite += modIndex;
	}

	if (node["sound"])
	{
		_sound = node["sound"].as<int>(_sound);
		if (_sound > 13) // 14 entries in GEO.CAT
			_sound += modIndex;
	}

	_damage				= node["damage"]			.as<int>(_damage);
	_range				= node["range"]				.as<int>(_range);
	_accuracy			= node["accuracy"]			.as<int>(_accuracy);
	_reloadCautious		= node["reloadCautious"]	.as<int>(_reloadCautious);
	_reloadStandard		= node["reloadStandard"]	.as<int>(_reloadStandard);
	_reloadAggressive	= node["reloadAggressive"]	.as<int>(_reloadAggressive);
	_ammoMax			= node["ammoMax"]			.as<int>(_ammoMax);
	_rearmRate			= node["rearmRate"]			.as<int>(_rearmRate);
	_prjSpeed			= node["prjSpeed"]			.as<int>(_prjSpeed);
	_launcher			= node["launcher"]			.as<std::string>(_launcher);
	_clip				= node["clip"]				.as<std::string>(_clip);

	_prjType = static_cast<CwpType>(node["prjType"].as<int>(_prjType));
}

/**
 * Gets the language string that names the CraftWeapon.
 * Each craft weapon type has a unique name.
 * @return, the CraftWeapon's name
 */
std::string RuleCraftWeapon::getType() const
{
	return _type;
}

/**
 * Gets the ID of the sprite used to draw the CraftWeapon in the
 * CraftInfoState (BASEBITS) and DogfightState (INTICON) screens.
 * @return, the sprite ID
 */
int RuleCraftWeapon::getSprite() const
{
	return _sprite;
}

/**
 * Gets the ID of the sound used when firing the CraftWeapon in the Dogfight screen.
 * @return, the sound ID
 */
int RuleCraftWeapon::getSound() const
{
	return _sound;
}

/**
 * Gets the amount of damage this craft weapon inflicts on enemy crafts.
 * @return, the damage amount
 */
int RuleCraftWeapon::getDamage() const
{
	return _damage;
}

/**
 * Gets the maximum range of the CraftWeapon.
 * @return, the range in km
 */
int RuleCraftWeapon::getRange() const
{
	return _range;
}

/**
 * Gets the percentage chance of each shot of the CraftWeapon hitting an enemy craft.
 * @return, the accuracy as a percentage
 */
int RuleCraftWeapon::getAccuracy() const
{
	return _accuracy;
}

/**
 * Gets the amount of time the CraftWeapon takes to reload in cautious mode.
 * @return, the time in game seconds
 */
int RuleCraftWeapon::getCautiousReload() const
{
	return _reloadCautious;
}

/**
 * Gets the amount of time the CraftWeapon takes to reload in standard mode.
 * @return, the time in game seconds
 */
int RuleCraftWeapon::getStandardReload() const
{
	return _reloadStandard;
}

/**
 * Gets the amount of time the CraftWeapon takes to reload in aggressive mode.
 * @return, the time in game seconds
 */
int RuleCraftWeapon::getAggressiveReload() const
{
	return _reloadAggressive;
}

/**
 * Gets the maximum amount of ammo the CraftWeapon can carry.
 * @return, the amount of ammo
 */
int RuleCraftWeapon::getAmmoMax() const
{
	return _ammoMax;
}

/**
 * Gets how much ammo is added to the CraftWeapon while rearming.
 * @return, the amount of ammo
 */
int RuleCraftWeapon::getRearmRate() const
{
	return _rearmRate;
}

/**
 * Gets the language string of the item that is the CraftWeapon.
 * @return, the item name
 */
std::string RuleCraftWeapon::getLauncherItem() const
{
	return _launcher;
}

/**
 * Gets the language string of the item used to load the CraftWeapon with ammo.
 * @return, the item name
 */
std::string RuleCraftWeapon::getClipItem() const
{
	return _clip;
}

/**
 * Gets the CwpType the CraftWeapon will fire.
 * @return, the projectile type
 */
CwpType RuleCraftWeapon::getProjectileType() const
{
	return _prjType;
}

/**
 * Gets the speed of the projectile fired by the CraftWeapon.
 * @return, the projectile speed
 */
int RuleCraftWeapon::getProjectileSpeed() const
{
	return _prjSpeed;
}

}
