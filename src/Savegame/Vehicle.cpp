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

#include "Vehicle.h"

#include "../Ruleset/RuleItem.h"


namespace OpenXcom
{

/**
 * Initializes a vehicle of the specified type.
 * @note This describes a vehicle that has been loaded onto a Craft only.
 * @param itRule	- pointer to RuleItem
 * @param ammo		- initial ammo
 * @param vhclSize	- size in tiles
 */
Vehicle::Vehicle(
		RuleItem* itRule,
		int ammo,
		int vhclSize)
	:
		_itRule(itRule),
		_ammo(ammo),
		_size(vhclSize)
{}

/**
 * dTor.
 */
Vehicle::~Vehicle()
{}

/**
 * Loads the vehicle from a YAML file.
 * @param node - reference a YAML node
 */
void Vehicle::load(const YAML::Node& node)
{
	_ammo = node["ammo"].as<int>(_ammo);
	_size = node["size"].as<int>(_size);
}

/**
 * Saves the vehicle to a YAML file.
 * @return, YAML node
 */
YAML::Node Vehicle::save() const
{
	YAML::Node node;

	node["type"] = _itRule->getType();
	node["ammo"] = _ammo;
	node["size"] = _size;

	return node;
}

/**
 * Returns the ruleset for the vehicle's type.
 * @return, pointer to RuleItem
 */
RuleItem* Vehicle::getRules() const
{
	return _itRule;
}

/**
 * Returns the ammo contained in this vehicle.
 * @return, quantity of weapon ammo
 */
int Vehicle::getAmmo() const
{
	if (_ammo == -1)
		return 255;

	return _ammo;
}

/**
 * Changes the ammo contained in this vehicle.
 * @param ammo - quantity of weapon ammo
 */
void Vehicle::setAmmo(int ammo)
{
	if (_ammo != -1)
		_ammo = ammo;
}

/**
 * Returns the size occupied by this vehicle in a transport craft.
 * @return, size in tiles
 */
int Vehicle::getSize() const
{
	return _size;
}

}
