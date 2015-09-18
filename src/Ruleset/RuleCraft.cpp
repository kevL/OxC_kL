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

#include "RuleCraft.h"

#include "RuleTerrain.h"


namespace OpenXcom
{

/**
 * Creates a blank ruleset for a certain type of craft.
 * @param type - reference a string defining the type
 */
RuleCraft::RuleCraft(const std::string& type)
	:
		_type(type),
		_sprite(-1),
		_marker(-1),
		_fuelMax(0),
		_damageMax(0),
		_speedMax(0),
		_accel(0),
		_weapons(0),
		_soldiers(0),
		_vehicles(0),
		_costBuy(0),
		_costRent(0),
		_costSell(0),
		_repairRate(1),
		_refuelRate(1),
		_radarRange(600),
		_reconRange(900),
		_transferTime(0),
		_score(0),
		_tacticalTerrainData(NULL),
		_spacecraft(false),
		_listOrder(0),
		_maxItems(0)
{}

/**
 * dTor.
 */
RuleCraft::~RuleCraft()
{
	delete _tacticalTerrainData;
}

/**
 * Loads the craft from a YAML file.
 * @param node		- reference a YAML node
 * @param rules		- pointer to Ruleset
 * @param modIndex	- a value that offsets the sounds and sprite values to avoid conflicts
 * @param listOrder	- the list weight for this craft
 */
void RuleCraft::load(
		const YAML::Node& node,
		Ruleset* const rules,
		int modIndex,
		int listOrder)
{
	_type		= node["type"]		.as<std::string>(_type);
	_requires	= node["requires"]	.as<std::vector<std::string> >(_requires);

	if (node["sprite"])
	{
		_sprite = node["sprite"].as<int>(_sprite);
		if (_sprite > 4) // this is an offset in BASEBITS.PCK, and two in INTICONS.PCK
			_sprite += modIndex;
	}

	_marker			= node["marker"]		.as<int>(_marker);
	_fuelMax		= node["fuelMax"]		.as<int>(_fuelMax);
	_damageMax		= node["damageMax"]		.as<int>(_damageMax);
	_speedMax		= node["speedMax"]		.as<int>(_speedMax);
	_accel			= node["accel"]			.as<int>(_accel);
	_weapons		= node["weapons"]		.as<int>(_weapons);
	_soldiers		= node["soldiers"]		.as<int>(_soldiers);
	_vehicles		= node["vehicles"]		.as<int>(_vehicles);
	_costBuy		= node["costBuy"]		.as<int>(_costBuy);
	_costRent		= node["costRent"]		.as<int>(_costRent);
	_costSell		= node["costSell"]		.as<int>(_costSell);
	_refuelItem		= node["refuelItem"]	.as<std::string>(_refuelItem);
	_repairRate		= node["repairRate"]	.as<int>(_repairRate);
	_refuelRate		= node["refuelRate"]	.as<int>(_refuelRate);
	_radarRange		= node["radarRange"]	.as<int>(_radarRange);
	_reconRange		= node["reconRange"]	.as<int>(_reconRange);
	_transferTime	= node["transferTime"]	.as<int>(_transferTime);
	_score			= node["score"]			.as<int>(_score);
	_spacecraft		= node["spacecraft"]	.as<bool>(_spacecraft);
	_maxItems		= node["maxItems"]		.as<int>(_maxItems);


	if (const YAML::Node& terrain = node["battlescapeTerrainData"])
	{
		RuleTerrain* const terrainRule = new RuleTerrain(terrain["name"].as<std::string>());
		terrainRule->load(
						terrain,
						rules);
		_tacticalTerrainData = terrainRule;

		if (const YAML::Node& deployment = node["deployment"])
			_deployment = deployment.as<std::vector<std::vector<int> > >(_deployment);
	}

	_listOrder = node["listOrder"].as<int>(_listOrder);
	if (_listOrder == 0)
		_listOrder = listOrder;
}

/**
 * Gets the language string that names the craft.
 * Each craft type has a unique name.
 * @return, the craft's name
 */
std::string RuleCraft::getType() const
{
	return _type;
}

/**
 * Gets the list of research required to acquire the craft.
 * @return, reference to a vector of strings that lists the needed research IDs
 */
const std::vector<std::string>& RuleCraft::getRequirements() const
{
	return _requires;
}

/**
 * Gets the ID of the sprite used to draw the craft in the
 * CraftInfoState (BASEBITS) and DogfightState (INTICON) screens.
 * @return, the sprite ID
 */
int RuleCraft::getSprite() const
{
	return _sprite;
}

/**
 * Returns the globe marker for the craft type.
 * @return, marker sprite (-1 if not in rule)
 */
int RuleCraft::getMarker() const
{
	return _marker;
}

/**
 * Gets the maximum fuel the craft can contain.
 * @return, the fuel amount
 */
int RuleCraft::getMaxFuel() const
{
	return _fuelMax;
}

/**
 * Gets the maximum damage the craft can take before it's destroyed.
 * @return, the maximum damage
 */
int RuleCraft::getMaxDamage() const
{
	return _damageMax;
}

/**
 * Gets the maximum speed of the craft flying around the Geoscape.
 * @return, the speed in knots
 */
int RuleCraft::getMaxSpeed() const
{
	return _speedMax;
}

/**
 * Gets the acceleration of the craft for taking off / stopping.
 * @return, the acceleration
 */
int RuleCraft::getAcceleration() const
{
	return _accel;
}

/**
 * Gets the maximum number of weapons that can be equipped onto the craft.
 * @return, the weapon capacity
 */
int RuleCraft::getWeapons() const
{
	return _weapons;
}

/**
 * Gets the maximum number of soldiers that the craft can carry.
 * @return, the soldier capacity
 */
int RuleCraft::getSoldiers() const
{
	return _soldiers;
}

/**
 * Gets the maximum number of vehicles that the craft can carry.
 * @return, the vehicle capacity
 */
int RuleCraft::getVehicles() const
{
	return _vehicles;
}

/**
 * Gets the cost of this craft for purchase/rent (0 if not purchasable).
 * @return, the cost
 */
int RuleCraft::getBuyCost() const
{
	return _costBuy;
}

/**
 * Gets the cost of rent for a month.
 * @return, the cost
 */
int RuleCraft::getRentCost() const
{
	return _costRent;
}

/**
 * Gets the sell value of this craft - rented craft should use 0.
 * @return, the sell value
 */
int RuleCraft::getSellCost() const
{
	return _costSell;
}

/**
 * Gets what item is required to refuel the craft.
 * @return, the item ID string or "" if none
 */
std::string RuleCraft::getRefuelItem() const
{
	return _refuelItem;
}

/**
 * Gets how much damage is removed from the craft while repairing.
 * @return, the amount of damage
 */
int RuleCraft::getRepairRate() const
{
	return _repairRate;
}

/**
 * Gets how much fuel is added to the craft while refuelling.
 * @return, the amount of fuel
 */
int RuleCraft::getRefuelRate() const
{
	return _refuelRate;
}

/**
 * Gets the craft's radar range for detecting UFOs.
 * @return, the range in nautical miles
 */
int RuleCraft::getRadarRange() const
{
	return _radarRange;
}

/**
 * Gets the craft's sight range for detecting bases.
 * @return, the range in nautical miles
 */
int RuleCraft::getReconRange() const
{
	return _reconRange;
}

/**
 * Gets the amount of time a new craft takes to arrive at Base.
 * @return, the time in hours
 */
int RuleCraft::getTransferTime() const
{
	return _transferTime;
}

/**
 * Gets the number of points you lose when this craft is destroyed.
 * @return, the score in points
 */
int RuleCraft::getScore() const
{
	return _score;
}

/**
 * Gets the terrain data needed to draw the Craft in the battlescape.
 * @return, pointer to RuleTerrain data
 */
RuleTerrain* RuleCraft::getBattlescapeTerrainData()
{
	return _tacticalTerrainData;
}

/**
 * Checks if this ship is capable of going to mars.
 * @return, true if this ship is capable of going to Mars
 */
bool RuleCraft::getSpacecraft() const
{
	return _spacecraft;
}

/**
 * Gets the list weight for the craft as a research item.
 * @return, the list weight
 */
int RuleCraft::getListOrder() const
{
	 return _listOrder;
}

/**
 * Gets the deployment layout for the craft.
 * @return, reference to a vector of vectors representing the deployment layout
 */
std::vector<std::vector<int> >& RuleCraft::getDeployment()
{
	return _deployment;
}

/**
 * Gets the maximum amount of items this craft can carry/store.
 * @return, quantity of items
 */
int RuleCraft::getMaxItems() const
{
	return _maxItems;
}

}
