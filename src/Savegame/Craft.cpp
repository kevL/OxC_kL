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

#include "Craft.h"

//#define _USE_MATH_DEFINES
//#include <cmath>

#include "AlienBase.h"
#include "Base.h"
#include "CraftWeapon.h"
#include "ItemContainer.h"
#include "MissionSite.h"
#include "Soldier.h"
#include "Ufo.h"
#include "Vehicle.h"
#include "Waypoint.h"

#include "../Engine/Language.h"

#include "../Geoscape/GeoscapeState.h"
#include "../Geoscape/Globe.h" // Globe::GLM_CRAFT

#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/AlienDeployment.h"
#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleCraftWeapon.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleUnit.h"

#include "../Savegame/SavedGame.h"
#include "../Savegame/Transfer.h"


namespace OpenXcom
{

/**
 * Creates a Craft of the specified type and assigns it the latest craft ID available.
 * @param crRule	- pointer to RuleCraft
 * @param base		- pointer to Base of origin
 * @param id		- ID to assign to the craft; 0 for no id (default 0)
 */
Craft::Craft(
		RuleCraft* const crRule,
		Base* const base,
		int id)
	:
		MovingTarget(),
		_crRule(crRule),
		_base(base),
		_id(id),
		_fuel(0),
		_damage(0),
		_takeOff(0),
		_status("STR_READY"),
		_lowFuel(false),
		_tacticalDone(false),
		_tactical(false),
		_inDogfight(false),
		_loadCur(0),
		_warning(CW_NONE),
		_warned(false), // do not save-to-file; ie, re-warn player if reloading
		_kills(0)
{
	_items = new ItemContainer();

	for (int
			i = 0;
			i != _crRule->getWeapons();
			++i)
	{
		_weapons.push_back(NULL);
	}

	if (_base != NULL)
		setBase(_base);

	_loadCap = _crRule->getMaxItems() + _crRule->getSoldiers() * 10;
}

/**
 * Deletes this Craft.
 */
Craft::~Craft()
{
	for (std::vector<CraftWeapon*>::const_iterator
			i = _weapons.begin();
			i != _weapons.end();
			++i)
	{
		delete *i;
	}

	delete _items;

	for (std::vector<Vehicle*>::const_iterator
			i = _vehicles.begin();
			i != _vehicles.end();
			++i)
	{
		delete *i;
	}
}

/**
 * Loads this Craft from a YAML file.
 * @param node		- reference a YAML node
 * @param rules		- pointer to Ruleset
 * @param gameSave	- pointer to the SavedGame
 */
void Craft::load(
		const YAML::Node& node,
		const Ruleset* const rules,
		SavedGame* const gameSave)
{
	MovingTarget::load(node);

	_id			= node["id"]	.as<int>(_id);
	_fuel		= node["fuel"]	.as<int>(_fuel);
	_damage		= node["damage"].as<int>(_damage);
	_warning	= static_cast<CraftWarning>(node["warning"].as<int>(_warning));

	size_t j = 0;
	for (YAML::const_iterator
			i = node["weapons"].begin();
			i != node["weapons"].end();
			++i)
	{
		if (_crRule->getWeapons() > static_cast<int>(j))
		{
			const std::string type = (*i)["type"].as<std::string>();
			if (type != "0"
				&& rules->getCraftWeapon(type) != NULL)
			{
				CraftWeapon* const cw = new CraftWeapon(
													rules->getCraftWeapon(type),
													0);
				cw->load(*i);
				_weapons[j++] = cw;
			}
			else
				_weapons[j++] = NULL;
		}
	}

	_items->load(node["items"]);
	for (std::map<std::string, int>::const_iterator
			i = _items->getContents()->begin();
			i != _items->getContents()->end();
			)
	{
		if (std::find(
					rules->getItemsList().begin(),
					rules->getItemsList().end(),
					i->first) == rules->getItemsList().end())
		{
			i = _items->getContents()->erase(i);
		}
		else
			++i;
	}

	for (YAML::const_iterator
			i = node["vehicles"].begin();
			i != node["vehicles"].end();
			++i)
	{
		const std::string type = (*i)["type"].as<std::string>();
		if (rules->getItem(type) != NULL)
		{
			const int armorSize = rules->getArmor(rules->getUnit(type)->getArmor())->getSize();
			Vehicle* const vhcl = new Vehicle(
											rules->getItem(type),
											0,
											armorSize * armorSize);
			vhcl->load(*i);
			_vehicles.push_back(vhcl);
		}
	}

	_status			= node["status"]		.as<std::string>(_status);
	_lowFuel		= node["lowFuel"]		.as<bool>(_lowFuel);
	_tacticalDone	= node["tacticalDone"]	.as<bool>(_tacticalDone);
	_kills			= node["kills"]			.as<int>(_kills);

	if (const YAML::Node name = node["name"])
		_name = Language::utf8ToWstr(name.as<std::string>());

	if (const YAML::Node& dest = node["dest"])
	{
		const std::string type = dest["type"].as<std::string>();

		int id = dest["id"].as<int>();

		if (type == "STR_BASE")
			returnToBase();
		else if (type == "STR_UFO")
		{
			for (std::vector<Ufo*>::const_iterator
					i = gameSave->getUfos()->begin();
					i != gameSave->getUfos()->end();
					++i)
			{
				if ((*i)->getId() == id)
				{
					setDestination(*i);
					break;
				}
			}
		}
		else if (type == "STR_WAYPOINT")
		{
			for (std::vector<Waypoint*>::const_iterator
					i = gameSave->getWaypoints()->begin();
					i != gameSave->getWaypoints()->end();
					++i)
			{
				if ((*i)->getId() == id)
				{
					setDestination(*i);
					break;
				}
			}
		}
		else if (type == "STR_ALIEN_BASE")
		{
			for (std::vector<AlienBase*>::const_iterator
					i = gameSave->getAlienBases()->begin();
					i != gameSave->getAlienBases()->end();
					++i)
			{
				if ((*i)->getId() == id)
				{
					setDestination(*i);
					break;
				}
			}
		}
		else // type = "STR_TERROR_SITE" (was "STR_ALIEN_TERROR")
		{
			for (std::vector<MissionSite*>::iterator
					i = gameSave->getMissionSites()->begin();
					i != gameSave->getMissionSites()->end();
					++i)
			{
				if ((*i)->getId() == id
					&& (*i)->getDeployment()->getMarkerId() == type)
				{
					setDestination(*i);
					break;
				}
			}
		}
	}

	_takeOff	= node["takeOff"]	.as<int>(_takeOff);
	_tactical	= node["tactical"]	.as<bool>(_tactical);
	if (_tactical == true)
		setSpeed(0);

	_loadCur = getNumEquipment() + (getNumSoldiers() + getNumVehicles(true) * 10); // note: 10 is the 'load' that a single 'space' uses.
}

/**
 * Saves this Craft to a YAML file.
 * @return, YAML node
 */
YAML::Node Craft::save() const
{
	YAML::Node node = MovingTarget::save();

	node["type"]	= _crRule->getType();
	node["id"]		= _id;

	for (std::vector<CraftWeapon*>::const_iterator
			i = _weapons.begin();
			i != _weapons.end();
			++i)
	{
		YAML::Node subnode;
		if (*i != 0)
			subnode = (*i)->save();
		else
			subnode["type"] = "0";
		node["weapons"].push_back(subnode);
	}

	node["items"] = _items->save();

	for (std::vector<Vehicle*>::const_iterator
			i = _vehicles.begin();
			i != _vehicles.end();
			++i)
	{
		node["vehicles"].push_back((*i)->save());
	}

	node["status"] = _status;

	if (_fuel != 0)				node["fuel"]			= _fuel;
	if (_damage != 0)			node["damage"]			= _damage;
	if (_lowFuel == true)		node["lowFuel"]			= _lowFuel;
	if (_tacticalDone == true)	node["tacticalDone"]	= _tacticalDone;
	if (_tactical == true)		node["tactical"]		= _tactical;
	if (_kills != 0)			node["kills"]			= _kills;
	if (_takeOff != 0)			node["takeOff"]			= _takeOff;
	if (_name.empty() == false)	node["name"]			= Language::wstrToUtf8(_name);
	if (_warning != CW_NONE)	node["warning"]			= static_cast<int>(_warning);

	return node;
}

/**
 * Loads this Craft's unique identifier from a YAML file.
 * @param node - reference a YAML node
 * @return, unique craft id
 */
CraftId Craft::loadId(const YAML::Node& node) // static.
{
	return std::make_pair(
						node["type"].as<std::string>(),
						node["id"]	.as<int>());
}

/**
 * Saves this Craft's unique identifiers to a YAML file.
 * @return, YAML node
 */
YAML::Node Craft::saveId() const
{
	YAML::Node node = MovingTarget::saveId();

	const CraftId uniqueId = getUniqueId();

	node["type"]	= uniqueId.first;
	node["id"]		= uniqueId.second;

	return node;
}

/**
 * Gets the ruleset for this Craft's type.
 * @return, pointer to RuleCraft
 */
RuleCraft* Craft::getRules() const
{
	return _crRule;
}

/**
 * Sets the ruleset for this Craft's type.
 * @param crRule - pointer to a different RuleCraft
 * @warning ONLY FOR NEW BATTLE USE!
 */
void Craft::changeRules(RuleCraft* const crRule)
{
	_crRule = crRule;
	_weapons.clear();

	for (int
			i = 0;
			i != _crRule->getWeapons();
			++i)
	{
		_weapons.push_back(NULL);
	}
}

/**
 * Gets this Craft's unique ID.
 * @note Each craft can be identified by its type and ID.
 * @return, unique ID
 */
int Craft::getId() const
{
	return _id;
}

/**
 * Gets this Craft's unique identifying name.
 * @note If there's no custom name the language default is used.
 * @param lang - pointer to a Language to get strings from
 * @return, full name of craft
 */
std::wstring Craft::getName(const Language* const lang) const
{
	if (_name.empty() == true)
		return lang->getString("STR_CRAFTNAME").arg(lang->getString(_crRule->getType())).arg(_id);

	return _name;
}

/**
 * Sets this Craft's custom name.
 * @param newName -  reference a new custom name; if blank, the language default is used
 */
void Craft::setName(const std::wstring& newName)
{
	_name = newName;
}

/**
 * Gets the globe marker for this Craft.
 * @return, marker sprite (-1 if based)
 */
int Craft::getMarker() const
{
	if (_status != "STR_OUT")
		return -1;

	if (_crRule->getMarker() == -1)
		return Globe::GLM_CRAFT;

	return _crRule->getMarker();
}

/**
 * Gets the base this Craft belongs to.
 * @return, pointer to the Base
 */
Base* Craft::getBase() const
{
	return _base;
}

/**
 * Sets the base this Craft belongs to.
 * @param base		- pointer to Base
 * @param transfer	- true to move the craft to the Base coordinates (default true)
 */
void Craft::setBase(
		Base* const base,
		bool transfer)
{
	_base = base;

	if (transfer == true)
	{
		_lon = base->getLongitude();
		_lat = base->getLatitude();
	}
}

/**
 * Gets the current status of this Craft.
 *		STR_READY
 *		STR_REPAIRS
 *		STR_REARMING
 *		STR_REFUELLING
 *		STR_OUT
 * @return, status string
 */
std::string Craft::getCraftStatus() const
{
	return _status;
}

/**
 * Sets the current status of this Craft.
 * @param status - reference a status string
 */
void Craft::setCraftStatus(const std::string& status)
{
	_status = status;
}

/**
 * Gets the current altitude of this Craft.
 * @return, altitude string
 */
std::string Craft::getAltitude() const
{
	if (_dest == NULL)
		return "STR_HIGH_UC";

	const Ufo* const ufo = dynamic_cast<Ufo*>(_dest);
	if (ufo != NULL)
	{
		if (ufo->getAltitude() != "STR_GROUND")
			return ufo->getAltitude();

		return "STR_VERY_LOW";
	}

	switch (RNG::generate(0,3))
	{
		default:
		case 0:
		case 1: return "STR_LOW_UC";
		case 2: return "STR_HIGH_UC";
		case 3: return "STR_VERY_HIGH";
	}
}

/**
 * Sets the destination this Craft is heading to.
 * @param dest - pointer to Target destination
 */
void Craft::setDestination(Target* const dest)
{
	if (_status != "STR_OUT")
	{
		_takeOff = 75;
		setSpeed(_crRule->getMaxSpeed() / 10);
	}
	else if (dest == NULL)
		setSpeed(_crRule->getMaxSpeed() / 2);
	else
		setSpeed(_crRule->getMaxSpeed());

	MovingTarget::setDestination(dest);
}

/**
 * Gets the amount of weapons currently equipped on this craft.
 * @return, number of weapons
 */
int Craft::getNumWeapons() const
{
	if (_crRule->getWeapons() == 0)
		return 0;

	int ret = 0;

	for (std::vector<CraftWeapon*>::const_iterator
			i = _weapons.begin();
			i != _weapons.end();
			++i)
	{
		if (*i != NULL)
			++ret;
	}

	return ret;
}

/**
 * Gets the amount of soldiers from a list that are currently attached to this craft.
 * @return, number of soldiers
 */
int Craft::getNumSoldiers() const
{
	if (_crRule->getSoldiers() == 0)
		return 0;

	int ret = 0;

	for (std::vector<Soldier*>::const_iterator
			i = _base->getSoldiers()->begin();
			i != _base->getSoldiers()->end();
			++i)
	{
		if ((*i)->getCraft() == this)
			++ret;
	}

	return ret;
}

/**
 * Gets the amount of equipment currently equipped on this craft.
 * @return, number of items
 */
int Craft::getNumEquipment() const
{
	return _items->getTotalQuantity();
}

/**
 * Gets the amount of vehicles currently contained in this craft.
 * @param tiles - true to return tile-spaces used in a transport (default false)
 * @return, either number of vehicles or tile-space used
 */
int Craft::getNumVehicles(bool tiles) const
{
	if (tiles == true)
	{
		int ret = 0;

		for (std::vector<Vehicle*>::const_iterator
				i = _vehicles.begin();
				i != _vehicles.end();
				++i)
		{
			ret += (*i)->getSize();
		}

		return ret;
	}

	return static_cast<int>(_vehicles.size());
}

/**
 * Gets the list of weapons currently equipped in the craft.
 * @return, pointer to a vector of pointers to CraftWeapon(s)
 */
std::vector<CraftWeapon*>* Craft::getWeapons()
{
	return &_weapons;
}

/**
 * Gets the list of items in the craft.
 * @return, pointer to the ItemContainer list
 */
ItemContainer* Craft::getItems() const
{
	return _items;
}

/**
 * Gets the list of vehicles currently equipped in the craft.
 * @return, pointer to a vector of pointers to Vehicle(s)
 */
std::vector<Vehicle*>* Craft::getVehicles()
{
	return &_vehicles;
}

/**
 * Gets the amount of fuel currently contained in this craft.
 * @return, amount of fuel
 */
int Craft::getFuel() const
{
	return _fuel;
}

/**
 * Sets the amount of fuel currently contained in this craft.
 * @param fuel - amount of fuel
 */
void Craft::setFuel(int fuel)
{
	_fuel = fuel;

	if (_fuel > _crRule->getMaxFuel())
		_fuel = _crRule->getMaxFuel();
	else if (_fuel < 0)
		_fuel = 0;
}

/**
 * Gets the ratio between the amount of fuel currently contained in this craft
 * and the total it can carry.
 * @return, fuel remaining as percent
 */
int Craft::getFuelPct() const
{
	return static_cast<int>(std::floor(
		   static_cast<double>(_fuel) / static_cast<double>(_crRule->getMaxFuel()) * 100.));
}

/**
 * Gets the amount of damage this craft has taken.
 * @return, amount of damage
 */
int Craft::getCraftDamage() const
{
	return _damage;
}

/**
 * Sets the amount of damage this craft has taken.
 * @param damage - amount of damage
 */
void Craft::setCraftDamage(const int damage)
{
	_damage = damage;

	if (_damage < 0)
		_damage = 0;
}

/**
 * Gets the ratio between the amount of damage this craft has taken and the
 * total it can take before it's destroyed.
 * @return, damage taken as percent
 */
int Craft::getCraftDamagePct() const
{
	return static_cast<int>(std::ceil(
		   static_cast<double>(_damage) / static_cast<double>(_crRule->getMaxDamage()) * 100.));
}

/**
 * Gets whether the craft is currently low on fuel - only has enough to get
 * back to base.
 * @return, true if fuel is low
 */
bool Craft::getLowFuel() const
{
	return _lowFuel;
}

/**
 * Sets whether this craft is currently low on fuel - only has enough to get
 * back to its Base.
 * @param low - true if fuel is low (default true)
 */
void Craft::setLowFuel(bool low)
{
	_lowFuel = low;
}

/**
 * Gets whether this craft has just done a ground mission and is forced to
 * return to its Base.
 * @return, true if this craft needs to return to base
 */
bool Craft::getTacticalReturn() const
{
	return _tacticalDone;
}

/**
 * Sets that this craft has just done a ground mission and is forced to return
 * to its Base.
 */
void Craft::setTacticalReturn()
{
	_tacticalDone = true;
}

/**
 * Gets the amount of fuel this craft uses while it's airborne.
 * @return, fuel amount
 */
int Craft::getFuelConsumption() const
{
	if (_crRule->getRefuelItem().empty() == false) // Firestorm, Lightning, Avenger, etc.
		return 1;

	return _speed; // Skyranger, Interceptor, etc.
}

/**
 * Gets the minimum required fuel for this craft to get back to Base.
 * @return, fuel amount
 */
int Craft::getFuelLimit() const
{
	return getFuelLimit(_base);
}

/**
 * Gets the minimum required fuel for this craft to get back to Base.
 * @param base - pointer to a target Base
 * @return, fuel amount
 */
int Craft::getFuelLimit(const Base* const base) const
{
	double
		distRads = getDistance(base),
		patrol_factor = 1.;

	if (_dest != NULL)
		distRads = getDistance(_dest) + _base->getDistance(_dest);
	else if (_crRule->getRefuelItem().empty() == true)
		patrol_factor = 2.;	// Elerium-powered Craft do not suffer this; they use 1 fuel per 10-min regardless of patrol speed.

	const double speedRads = static_cast<double>(_crRule->getMaxSpeed()) * unitToRads / 6.;

	return static_cast<int>(std::ceil(
		   static_cast<double>(getFuelConsumption()) * distRads * patrol_factor / speedRads));
}

/**
 * Sends the craft back to its origin Base.
 */
void Craft::returnToBase()
{
	setDestination(_base);
}

/**
 * Moves the craft to its destination.
 */
void Craft::think()
{
	if (_takeOff != 0)
	{
		--_takeOff;

		if (_takeOff == 0)
			setSpeed(_crRule->getMaxSpeed());
	}
	else
	{
		moveTarget();

		if (reachedDestination() == true
			&& _dest == dynamic_cast<Target*>(_base))
		{
			setDestination(NULL);
			setSpeed(0);

			_lowFuel =
			_tacticalDone = false;
			_warning = CW_NONE;
			_takeOff = 0;

			checkup();
		}
	}
}

/**
 * Checks the condition of all the craft's systems to define its new status
 * when arriving at Base by flight or transfer.
 * @note This is actually used here and all over there too - basically whenever
 * a status-phase completes a checkup is done.
 */
void Craft::checkup()
{
	int
		cw = 0,
		loaded = 0;

	for (std::vector<CraftWeapon*>::const_iterator
			i = _weapons.begin();
			i != _weapons.end();
			++i)
	{
		if (*i != NULL)
		{
			++cw;

			if ((*i)->getAmmo() >= (*i)->getRules()->getAmmoMax())
				++loaded;
			else
				(*i)->setRearming();
		}
	}

	if (_damage > 0)
		_status = "STR_REPAIRS";	// 1st stage
	else if (cw > loaded)
		_status = "STR_REARMING";	// 2nd stage
	else if (_fuel < _crRule->getMaxFuel())
		_status = "STR_REFUELLING";	// 3rd stage
	else
		_status = "STR_READY";		// 4th Ready.
}

/**
 * Gets if a UFO is detected by the craft's radar.
 * @param target - pointer to target
 * @return, true if detected
 */
bool Craft::detect(const Target* const target) const
{
	const int radarRange = _crRule->getRadarRange();
	if (radarRange != 0)
	{
		const double
			range = static_cast<double>(radarRange) * greatCircleConversionFactor,
			dist = getDistance(target) * earthRadius;

		if (range >= dist)
			return true;
	}

	return false;
}

/**
 * Consumes the craft's fuel every 10 minutes while it's in the air.
 */
void Craft::consumeFuel()
{
	setFuel(_fuel - getFuelConsumption());
}

/**
 * Repairs the craft's damage every half-hour while it's docked at the base.
 */
void Craft::repair()
{
	_warning = CW_NONE;
	_warned = false;

	setCraftDamage(_damage - _crRule->getRepairRate());

	if (_damage == 0)
		checkup();
}

/**
 * Rearms the craft's weapons by adding ammo every half-hour while it's docked
 * at the base.
 * @param rules - pointer to Ruleset
 * @return, blank string if ArmOk else a string for cantLoad
 */
std::string Craft::rearm(const Ruleset* const rules)
{
	std::string
		ret,
		test;

	_warning = CW_NONE;

	for (std::vector<CraftWeapon*>::const_iterator
			i = _weapons.begin();
			i != _weapons.end();
			++i)
	{
		if (*i != NULL
			&& (*i)->getRearming() == true)
		{
			test.clear();

			const std::string clip = (*i)->getRules()->getClipItem();
			const int baseClips = _base->getItems()->getItemQty(clip);

			if (clip.empty() == true)
				(*i)->rearm();
			else if (baseClips > 0)
			{
				int clipsUsed = (*i)->rearm(
										baseClips,
										rules->getItem(clip)->getClipSize());
				if (clipsUsed != 0)
				{
					if (clipsUsed < 0) // trick. See CraftWeapon::rearm() - not enough clips at Base
					{
						clipsUsed = -clipsUsed;
						test = clip;

						_warning = CW_CANTREARM;
					}

					_base->getItems()->removeItem(
												clip,
												clipsUsed);
				}
			}
			else // no ammo at base
			{
				test = clip;

				_warning = CW_CANTREARM;
				(*i)->setCantLoad();
			}

			if (test.empty() == false) // warning
			{
				if (ret.empty() == false) // double warning
					ret = "STR_ORDNANCE_LC";
				else // check next weapon
					ret = test;
			}
			else // armok
				break;
		}
	} // That handles only 2 craft weapons.

	if (ret.empty() == true)
		checkup();

	return ret;
}

/**
 * Refuels the craft every half-hour while docked at its Base.
 */
void Craft::refuel()
{
	_warning = CW_NONE;
	_warned = false;

	setFuel(_fuel + _crRule->getRefuelRate());

	if (_fuel == _crRule->getMaxFuel())
		checkup();
}

/**
 * Gets this Craft's battlescape status.
 * @return, true if Craft is on the battlescape
 */
bool Craft::isInBattlescape() const
{
	return _tactical;
}

/**
 * Sets this Craft's battlescape status.
 * @param tactical - true if Craft is on the battlescape (default true)
 */
void Craft::setInBattlescape(bool tactical)
{
	if (tactical == true)
		setSpeed(0);

	_tactical = tactical;
}

/**
 * Gets the craft destroyed status.
 * @note If the amount of damage the craft takes is more than its health it will
 * be destroyed.
 * @return, true if the craft is destroyed
 */
bool Craft::isDestroyed() const
{
	return (_damage >= _crRule->getMaxDamage());
}

/**
 * Gets the amount of space available for soldiers and vehicles.
 * @return, space available
 */
int Craft::getSpaceAvailable() const
{
	return _crRule->getSoldiers() - getSpaceUsed();
}

/**
 * Gets the amount of space in use by soldiers and vehicles.
 * @return, space used
 */
int Craft::getSpaceUsed() const
{
	int vehicleSpaceUsed = 0; // <- could use getNumVehicles(true)

	for (std::vector<Vehicle*>::const_iterator
			i = _vehicles.begin();
			i != _vehicles.end();
			++i)
	{
		vehicleSpaceUsed += (*i)->getSize();
	}

	return getNumSoldiers() + vehicleSpaceUsed;
}

/**
 * Gets the total amount of vehicles of a certain type stored in the craft.
 * @param vehicle - reference a vehicle type
 * @return, number of vehicles
 */
int Craft::getVehicleCount(const std::string& vehicle) const
{
	int total = 0;

	for (std::vector<Vehicle*>::const_iterator
			i = _vehicles.begin();
			i != _vehicles.end();
			++i)
	{
		if ((*i)->getRules()->getType() == vehicle)
			++total;
	}

	return total;
}

/**
 * Gets the craft's dogfight status.
 * @return, true if this Craft is in a dogfight
 */
bool Craft::isInDogfight() const
{
	return _inDogfight;
}

/**
 * Sets the craft's dogfight status.
 * @param inDogfight - true if this Craft is in a dogfight
 */
void Craft::setInDogfight(const bool inDogfight)
{
	_inDogfight = inDogfight;
}

/**
 * Gets this Craft's unique id.
 * @return, a tuple of the craft's type and per-type id
 */
CraftId Craft::getUniqueId() const
{
	return std::make_pair(
						_crRule->getType(),
						_id);
}

/**
 * Sets capacity load.
 * @param load - capacity load
 */
void Craft::setLoadCapacity(const int load)
{
	_loadCap = load;
}

/**
 * Gets capacity load.
 * @return, capacity load
 */
int Craft::getLoadCapacity() const
{
	return _loadCap;
}

/*
 * Sets current load.
 * @param load - current load
 *
void Craft::setLoadCurrent(const int load)
{
	_loadCur = load;
} */

/**
 * Gets current load.
 * @note Also recalculates '_loadCur' value.
 * @return, current load
 */
int Craft::getLoadCurrent()
{
	return (_loadCur = (getNumEquipment() + getSpaceUsed() * 10));
}

/**
 * Gets this craft's current CraftWarning status.
 * @return, CraftWarning enum
 */
CraftWarning Craft::getWarning() const
{
	return _warning;
}

/**
 * Sets this craft's CraftWarning status.
 * @param warning - a CraftWarning enum
 */
void Craft::setWarning(const CraftWarning warning)
{
	_warning = warning;
}

/**
 * Gets whether a warning has been issued for this Craft.
 * @return, true if player has been warned about low resources
 */
bool Craft::getWarned() const
{
	return _warned;
}

/**
 * Sets whether a warning has been issued for this Craft.
 * @param warn - true if player has been warned about low resources
 */
void Craft::setWarned(const bool warn)
{
	_warned = warn;
}

/**
 * Gets the amount of time this Craft will be repairing/rearming/refueling.
 * @note Those are checked & try to happen every half hour.
 * @param delayed - reference set true if this Craft's Base will run out of materiel
 * @return, hours before Craft can fly
 */
int Craft::getDowntime(bool& delayed)
{
	int hours = 0;
	delayed = false;

	if (_damage > 0)
	{
		hours += static_cast<int>(std::ceil(
				 static_cast<double>(_damage) / static_cast<double>(_crRule->getRepairRate())
				 / 2.));
	}

	for (std::vector<CraftWeapon*>::const_iterator
			i = _weapons.begin();
			i != _weapons.end();
			++i)
	{
		if (*i != NULL
			&& (*i)->getRearming() == true)
		{
			const int reqQty = (*i)->getRules()->getAmmoMax() - (*i)->getAmmo();

			hours += static_cast<int>(std::ceil(
					 static_cast<double>(reqQty)
						/ static_cast<double>((*i)->getRules()->getRearmRate())
					 / 2.));

			if (delayed == false)
			{
				const std::string clip = (*i)->getRules()->getClipItem();
				if (clip.empty() == false)
				{
					int baseQty = _base->getItems()->getItemQty(clip);
					if (baseQty < reqQty)
					{
						for (std::vector<Transfer*>::const_iterator // check Transfers
								j = _base->getTransfers()->begin();
								j != _base->getTransfers()->end();
								++j)
						{
							if ((*j)->getType() == TRANSFER_ITEM
								&& (*j)->getItems() == clip)
							{
								baseQty += (*j)->getQuantity();

								if (baseQty >= reqQty)
									break;
							}
						}
					}

					if (baseQty < reqQty)
						delayed = true;
				}
			}
		}
	}

	if (_fuel < _crRule->getMaxFuel())
	{
		const int reqQty = _crRule->getMaxFuel() - _fuel;

		hours += static_cast<int>(std::ceil(
				 static_cast<double>(reqQty)
					/ static_cast<double>(_crRule->getRefuelRate())
				 / 2.));

		if (delayed == false)
		{
			const std::string fuel = _crRule->getRefuelItem();
			if (fuel.empty() == false)
			{
				int baseQty = _base->getItems()->getItemQty(fuel);
				if (baseQty < reqQty) // check Transfers
				{
					for (std::vector<Transfer*>::const_iterator // check Transfers
							i = _base->getTransfers()->begin();
							i != _base->getTransfers()->end();
							++i)
					{
						if ((*i)->getType() == TRANSFER_ITEM
							&& (*i)->getItems() == fuel)
						{
							baseQty += (*i)->getQuantity();

							if (baseQty >= reqQty)
								break;
						}
					}
				}

				if (baseQty < reqQty)
					delayed = true;
			}
		}
	}

	return hours;
}

/**
 * Adds a dogfight kill.
 */
void Craft::addKill() // <- cap this or do a log or an inverted exponential increase or ...
{
	++_kills;
}

/**
 * Gets this Craft's dogfight kills.
 * @return, kills
 */
int Craft::getKills() const
{
	return _kills;
}

/**
 * Gets if this Craft has left the ground.
 * @return, true if airborne
 */
bool Craft::getTakeoff() const
{
	return (_takeOff == 0);
}

}
