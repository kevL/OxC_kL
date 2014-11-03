/*
 * Copyright 2010-2014 OpenXcom Developers.
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

#define _USE_MATH_DEFINES

#include "Craft.h"

#include <cmath>
#include <sstream>

#include "../fmath.h"

#include "AlienBase.h"
#include "Base.h"
#include "CraftWeapon.h"
#include "ItemContainer.h"
#include "Soldier.h"
#include "TerrorSite.h"
#include "Ufo.h"
#include "Vehicle.h"
#include "Waypoint.h"

#include "../Engine/Language.h"
#include "../Engine/Logger.h"
#include "../Engine/RNG.h"

#include "../Geoscape/GeoscapeState.h"

#include "../Ruleset/Armor.h"
#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/RuleCraftWeapon.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/Unit.h"

#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Creates a Craft of the specified type and assigns it the latest craft ID available.
 * @param rules	- pointer to RuleCraft
 * @param base	- pointer to Base of origin
 * @param id	- ID to assign to the craft; NULL for no id
 */
Craft::Craft(
		RuleCraft* rules,
		Base* base,
		int id)
	:
		MovingTarget(),
		_rules(rules),
		_base(base),
		_id(0),
		_fuel(0),
		_damage(0),
		_interceptionOrder(0),
		_takeoff(0),
		_status("STR_READY"),
		_lowFuel(false),
		_mission(false),
		_inBattlescape(false),
		_inDogfight(false),
		_loadCur(0),
		_warning(CW_NONE),
		_warned(false) // do not save-to-file; ie, warn player after reloading
{
	_items = new ItemContainer();

	if (id != 0)
		_id = id;

	for (int
			i = 0;
			i < _rules->getWeapons();
			++i)
	{
		_weapons.push_back(0);
	}

	setBase(base);

	_loadCap = _rules->getMaxItems() + _rules->getSoldiers() * 10;
}

/**
 * Delete the contents of this Craft from memory.
 */
Craft::~Craft()
{
	for (std::vector<CraftWeapon*>::iterator
			i = _weapons.begin();
			i != _weapons.end();
			++i)
	{
		delete *i;
	}

	delete _items;

	for (std::vector<Vehicle*>::iterator
			i = _vehicles.begin();
			i != _vehicles.end();
			++i)
	{
		delete *i;
	}
}

/**
 * Loads this Craft from a YAML file.
 * @param node	- reference a YAML node
 * @param rules	- pointer to Ruleset
 * @param save	- pointer to the SavedGame
 */
void Craft::load(
		const YAML::Node& node,
		const Ruleset* rules,
		SavedGame* save)
{
	MovingTarget::load(node);

	_id			= node["id"]					.as<int>(_id);
	_fuel		= node["fuel"]					.as<int>(_fuel);
	_damage		= node["damage"]				.as<int>(_damage);
	_warning	= (CraftWarning)node["warning"]	.as<int>(_warning);

	size_t j = 0;
	for (YAML::const_iterator
			i = node["weapons"].begin();
			i != node["weapons"].end();
			++i)
	{
		if (_rules->getWeapons() > static_cast<int>(j))
		{
			std::string type = (*i)["type"].as<std::string>();
			if (type != "0"
				&& rules->getCraftWeapon(type))
			{
				CraftWeapon* w = new CraftWeapon(
											rules->getCraftWeapon(type),
											0);
				w->load(*i);
				_weapons[j] = w;
			}
			else
				_weapons[j] = 0;

			j++;
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
					i->first)
				== rules->getItemsList().end())
		{
			_items->getContents()->erase(i++);
		}
		else
			++i;
	}

	for (YAML::const_iterator
			i = node["vehicles"].begin();
			i != node["vehicles"].end();
			++i)
	{
		std::string type = (*i)["type"].as<std::string>();
		if (rules->getItem(type))
		{
			int unitSize = rules->getArmor(rules->getUnit(type)->getArmor())->getSize();
			Vehicle* v = new Vehicle(
									rules->getItem(type),
									0,
									unitSize * unitSize);
			v->load(*i);
			_vehicles.push_back(v);
		}
	}

	_status				= node["status"]			.as<std::string>(_status);
	_lowFuel			= node["lowFuel"]			.as<bool>(_lowFuel);
	_mission			= node["mission"]			.as<bool>(_mission);
	_interceptionOrder	= node["interceptionOrder"]	.as<int>(_interceptionOrder);

	if (const YAML::Node name = node["name"])
		_name = Language::utf8ToWstr(name.as<std::string>());

	if (const YAML::Node& dest = node["dest"])
	{
		std::string type = dest["type"].as<std::string>();

		int id = dest["id"].as<int>();

		if (type == "STR_BASE")
			returnToBase();
		else if (type == "STR_UFO")
		{
			for (std::vector<Ufo*>::const_iterator
					i = save->getUfos()->begin();
					i != save->getUfos()->end();
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
					i = save->getWaypoints()->begin();
					i != save->getWaypoints()->end();
					++i)
			{
				if ((*i)->getId() == id)
				{
					setDestination(*i);
					break;
				}
			}
		}
		else if (type == "STR_TERROR_SITE")
		{
			for (std::vector<TerrorSite*>::const_iterator
					i = save->getTerrorSites()->begin();
					i != save->getTerrorSites()->end();
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
					i = save->getAlienBases()->begin();
					i != save->getAlienBases()->end();
					++i)
			{
				if ((*i)->getId() == id)
				{
					setDestination(*i);
					break;
				}
			}
		}
	}

	_takeoff = node["takeoff"].as<int>(_takeoff);

	_inBattlescape = node["inBattlescape"].as<bool>(_inBattlescape);
	if (_inBattlescape)
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

	node["type"]	= _rules->getType();
	node["id"]		= _id;
	node["fuel"]	= _fuel;
	node["damage"]	= _damage;
	node["warning"]	= (int)_warning;

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

	if (_lowFuel)
		node["lowFuel"] = _lowFuel;

	if (_mission)
		node["mission"] = _mission;

	if (_inBattlescape)
		node["inBattlescape"] = _inBattlescape;

	if (_interceptionOrder != 0)
		node["interceptionOrder"] = _interceptionOrder;

	if (_takeoff != 0)
		node["takeoff"] = _takeoff;

	if (_name.empty() == false)
		node["name"] = Language::wstrToUtf8(_name);

	return node;
}

/**
 * Loads this Craft's unique identifier from a YAML file.
 * @param node - reference a YAML node
 * @return, unique craft id
 */
CraftId Craft::loadId(const YAML::Node& node)
{
	return std::make_pair(
						node["type"].as<std::string>(),
						node["id"].as<int>());
}

/**
 * Saves this Craft's unique identifiers to a YAML file.
 * @return, YAML node
 */
YAML::Node Craft::saveId() const
{
	YAML::Node node = MovingTarget::saveId();

	CraftId uniqueId = getUniqueId();

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
	return _rules;
}

/**
 * Sets the ruleset for this Craft's type.
 * @param rules - pointer to a different RuleCraft
 * @warning ONLY FOR NEW BATTLE USE!
 */
void Craft::changeRules(RuleCraft* rules)
{
	_rules = rules;
	_weapons.clear();

	for (int
			i = 0;
			i < _rules->getWeapons();
			++i)
	{
		_weapons.push_back(0);
	}
}

/**
 * Gets this Craft's unique ID. Each craft can be identified by its type and ID.
 * @return, unique ID
 */
int Craft::getId() const
{
	return _id;
}

/**
 * Gets this Craft's unique identifying name.
 * If there's no custom name, the language default is used.
 * @param lang - pointer to a Language to get strings from
 * @return, full name of craft
 */
std::wstring Craft::getName(Language* lang) const
{
	if (_name.empty())
		return lang->getString("STR_CRAFTNAME").arg(lang->getString(_rules->getType())).arg(_id);

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
 * @return, marker sprite, -1 if none
 */
int Craft::getMarker() const
{
	if (_status != "STR_OUT")
		return -1;

	return 1;
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
 * @param transfer	- true to move the craft to the Base coordinates (true)
 */
void Craft::setBase(
		Base* base,
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
 * @return, status string
 */
std::string Craft::getStatus() const
{
	return _status;
}

/**
 * Sets the current status of this Craft.
 * @param status - reference a status string
 */
void Craft::setStatus(const std::string& status)
{
	_status = status;
}

/**
 * Gets the current altitude of this Craft.
 * @return, altitude string
 */
std::string Craft::getAltitude() const
{
	Ufo* ufo = dynamic_cast<Ufo*>(_dest);

	// kL_begin:
	if (ufo != NULL)
	{
		if (ufo->getAltitude() != "STR_GROUND")
			return ufo->getAltitude();
		else
			return "STR_VERY_LOW";
	}
	else
	{
		switch (RNG::generate(0, 3))
		{
			case 0:
//				return "STR_VERY_LOW";
			case 1:
				return "STR_LOW_UC";
			case 2:
				return "STR_HIGH_UC";
			case 3:
				return"STR_VERY_HIGH";
		}
	}

	return "STR_VERY_LOW";
} // kL_end.
// kL_begin: Craft::getAltitude(), add strings for based xCom craft.
/*	if (u)
	{
		if (u->getAltitude() != "STR_GROUND")
		{
			return u->getAltitude();
		}
		else return "STR_VERY_LOW";
	}
	else if (getStatus() == "STR_READY"
		|| getStatus() == "STR_REPAIRS"
		|| getStatus() == "STR_REFUELLING"
		|| getStatus() == "STR_REARMING")
	{
		return "STR_GROUNDED";
	}
	// need to add: if xCom craft && inDogFight, return UFO altitude.
	else
	{
		return "STR_VERY_LOW";
	} */ // kL_end.

/**
 * Sets the destination this Craft is heading to.
 * @param dest - pointer to Target destination
 */
void Craft::setDestination(Target* dest)
{
	if (_status != "STR_OUT")
		_takeoff = 75;

	if (dest == NULL)
		setSpeed(_rules->getMaxSpeed() / 2);
	else
		setSpeed(_rules->getMaxSpeed());

	MovingTarget::setDestination(dest);
}

/**
 * Gets the amount of weapons currently equipped on this craft.
 * @return, number of weapons
 */
int Craft::getNumWeapons() const
{
	if (_rules->getWeapons() == 0)
		return 0;

	int ret = 0;

	for (std::vector<CraftWeapon*>::const_iterator
			i = _weapons.begin();
			i != _weapons.end();
			++i)
	{
		if (*i != NULL)
			ret++;
	}

	return ret;
}

/**
 * Gets the amount of soldiers from a list that are currently attached to this craft.
 * @return, number of soldiers
 */
int Craft::getNumSoldiers() const
{
	if (_rules->getSoldiers() == 0)
		return 0;

	int ret = 0;

	for (std::vector<Soldier*>::const_iterator
			i = _base->getSoldiers()->begin();
			i != _base->getSoldiers()->end();
			++i)
	{
		if ((*i)->getCraft() == this)
			ret++;
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
 * @param tileSpace - true to return tile-spaces used in a transport
 * @return, either number of vehicles or tile-space used
 */
int Craft::getNumVehicles(bool tileSpace) const
{
	if (tileSpace == true)
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

	return _vehicles.size();
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
ItemContainer* Craft::getItems()
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
void Craft::setFuel(const int fuel)
{
	_fuel = fuel;

	if (_fuel > _rules->getMaxFuel())
		_fuel = _rules->getMaxFuel();
	else if (_fuel < 0)
		_fuel = 0;
}

/**
 * Gets the ratio between the amount of fuel currently
 * contained in this craft and the total it can carry.
 * @return, fuel remaining as percent
 */
int Craft::getFuelPercentage() const
{
	return static_cast<int>(
			floor((static_cast<double>(_fuel) / static_cast<double>(_rules->getMaxFuel()))
			* 100.0));
}

/**
 * Gets the amount of damage this craft has taken.
 * @return, amount of damage
 */
int Craft::getDamage() const
{
	return _damage;
}

/**
 * Sets the amount of damage this craft has taken.
 * @param damage - amount of damage
 */
void Craft::setDamage(const int damage)
{
	_damage = damage;

	if (_damage < 0)
		_damage = 0;
}

/**
 * Gets the ratio between the amount of damage this craft
 * has taken and the total it can take before it's destroyed.
 * @return, damage taken as percent
 */
int Craft::getDamagePercent() const
{
	return static_cast<int>(
			floor((static_cast<double>(_damage) / static_cast<double>(_rules->getMaxDamage()))
			* 100.0));
}

/**
 * Gets whether the craft is currently low on fuel -
 * only has enough to get back to base.
 * @return, true if fuel is low
 */
bool Craft::getLowFuel() const
{
	return _lowFuel;
}

/**
 * Sets whether this craft is currently low on fuel -
 * only has enough to get back to its Base.
 * @param low - true if fuel is low
 */
void Craft::setLowFuel(const bool low)
{
	_lowFuel = low;
}

/**
 * Gets whether this craft has just done a ground mission
 * and is forced to return to its Base.
 * @return, true if this craft needs to return to base
 */
bool Craft::getMissionComplete() const
{
	return _mission;
}

/**
 * Sets whether this craft has just done a ground mission
 * and is forced to return to its Base.
 * @param mission - true if this craft needs to return to base
 */
void Craft::setMissionComplete(const bool mission)
{
	_mission = mission;
}

/**
 * Gets the current distance between this craft and the Base it belongs to.
 * @return, distance in radian
 */
double Craft::getDistanceFromBase() const
{
	return getDistance(_base);
}

/**
 * Gets the amount of fuel this craft uses while it's in the air.
 * @return, fuel amount
 */
int Craft::getFuelConsumption() const
{
	if (_rules->getRefuelItem().empty() == false) // Firestorm, Lightning, Avenger, etc.
		return 1;

	return static_cast<int>(floor(
			static_cast<double>(_speed) / 100.0)); // Skyranger, Interceptor.
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
int Craft::getFuelLimit(Base* base) const
{
	const double speedRadian = static_cast<double>(_rules->getMaxSpeed()) * (1.0 / 60.0) * (M_PI / 180.0) / 720.0;

	return static_cast<int>(ceil(
			(static_cast<double>(getFuelConsumption()) * getDistance(base)) / (speedRadian * 120.0)));
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
	if (_takeoff == 0)
		move();
	else
		_takeoff--;

	if (reachedDestination()
		&& _dest == dynamic_cast<Target*>(_base))
	{
		setInterceptionOrder(0);
		setDestination(NULL);
		setSpeed(0);

		_lowFuel = false;
		_mission = false;
//		_stopWarning = false;
		_warning = CW_NONE;
		_takeoff = 0;

		checkup();
	}
}

/**
 * Checks the condition of all the craft's systems to define its new status.
 * That is, when arriving at base by flight or transfer.
 * kL_Note: I now use this here and all over there too. Basically whenever
 * a status-phase completes somewhere ....
 */
void Craft::checkup()
{
	_warning = CW_NONE;
	_warned = false;

	int
		cw = 0,
		loaded = 0;

	for (std::vector<CraftWeapon*>::iterator
			i = _weapons.begin();
			i != _weapons.end();
			++i)
	{
		if (*i == NULL)
			continue;

		cw++;

		if ((*i)->getAmmo() >= (*i)->getRules()->getAmmoMax())
			loaded++;
		else
			(*i)->setRearming();
	}

	if (_damage > 0)
		_status = "STR_REPAIRS";	// 1st
	else if (cw > loaded)
		_status = "STR_REARMING";	// 2nd
	else if (_fuel < _rules->getMaxFuel())
		_status = "STR_REFUELLING";	// 3rd
	else
		_status = "STR_READY";		// 4th Ready.
}

/**
 * Gets if a UFO is detected by the craft's radar.
 * @param target - pointer to target
 * @return, true if detected, false otherwise
 */
bool Craft::detect(Target* target) const
{
	const double range = static_cast<double>(_rules->getRadarRange()) * greatCircleConversionFactor;

	if (AreSame(range, 0.0))
		return false;

	const double dist = getDistance(target) * earthRadius;
	if (dist < range)
		return true;

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
 * Repairs the craft's damage every hour while it's docked at the base.
 * kL_note: now every half-hour.
 */
void Craft::repair()
{
	setDamage(_damage - _rules->getRepairRate());

	if (_damage == 0)
		checkup();
}

/**
 * Rearms the craft's weapons by adding ammo every hour
 * while it's docked at the base. kL_note: now every half-hour!
 * @param rules - pointer to Ruleset
 * @return, blank string if ArmOk else a string for cantLoad
 */
std::string Craft::rearm(const Ruleset* rules)
{
	std::string
		ret,
		test;

	for (std::vector<CraftWeapon*>::const_iterator
			i = _weapons.begin();
			i != _weapons.end();
			++i)
	{
		if (*i != NULL
			&& (*i)->getRearming() == true)
		{
			test = "";

			const std::string clip = (*i)->getRules()->getClipItem();
			const int baseClips = _base->getItems()->getItem(clip);

			if (clip.empty() == true)
				(*i)->rearm(0, 0);
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

	return ret;
}

/**
 * Refuels the craft every 30 minutes while docked at its Base.
 */
void Craft::refuel()
{
	setFuel(_fuel + _rules->getRefuelRate());

	if (_fuel == _rules->getMaxFuel())
		checkup();
}

/**
 * Gets the craft's battlescape status.
 * @return, true if the craft is on the battlescape
 */
bool Craft::isInBattlescape() const
{
	return _inBattlescape;
}

/**
 * Sets the craft's battlescape status.
 * @param inbattle - true if it's in battle
 */
void Craft::setInBattlescape(const bool battle)
{
	if (battle)
		setSpeed(0);

	_inBattlescape = battle;
}

/**
 * Gets the craft destroyed status.
 * If the amount of damage the craft take is more than its health it will be destroyed.
 * @return, true if the craft is destroyed
 */
bool Craft::isDestroyed() const
{
	return (_damage >= _rules->getMaxDamage());
}

/**
 * Gets the amount of space available for soldiers and vehicles.
 * @return, space available
 */
int Craft::getSpaceAvailable() const
{
	return _rules->getSoldiers() - getSpaceUsed();
}

/**
 * Gets the amount of space in use by soldiers and vehicles.
 * @return, space used
 */
int Craft::getSpaceUsed() const
{
	int vehicleSpaceUsed = 0;

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
			total++;
	}

	return total;
}

/**
 * Gets the craft's dogfight status.
 * @return, true if the craft is in a dogfight
 */
bool Craft::isInDogfight() const
{
	return _inDogfight;
}

/**
 * Sets the craft's dogfight status.
 * @param inDogfight - true if it's in dogfight
 */
void Craft::setInDogfight(bool inDogfight)
{
	_inDogfight = inDogfight;
}

/**
 * Sets interception order (first craft to leave the base gets 1, second 2, etc.).
 * @param order - interception order
 */
void Craft::setInterceptionOrder(const int order)
{
	_interceptionOrder = order;
}

/**
 * Gets interception order.
 * @return, interception order
 */
int Craft::getInterceptionOrder() const
{
	return _interceptionOrder;
}

/**
 * Gets the craft's unique id.
 * @return, a tuple of the craft's type and per-type id
 */
CraftId Craft::getUniqueId() const
{
	return std::make_pair(
						_rules->getType(),
						_id);
}

/**
 * Sets capacity load.
 * @param load - capacity load
 */
void Craft::setLoadCapacity(int load)
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

/**
 * Sets current load.
 * @param load - current load
 */
void Craft::setLoadCurrent(int load)
{
	_loadCur = load;
}

/**
 * Gets current load.
 * @return, current load
 */
int Craft::getLoadCurrent()
{
	_loadCur = getNumEquipment() + getSpaceUsed() * 10;

	return _loadCur;
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

}
