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

#ifndef OPENXCOM_CRAFT_H
#define OPENXCOM_CRAFT_H

//#include <string>
//#include <vector>

#include "CraftId.h"
#include "MovingTarget.h"


namespace OpenXcom
{

class Base;
class CraftWeapon;
class ItemContainer;
class RuleCraft;
class Ruleset;
class SavedGame;
class Soldier;
class Vehicle;


enum CraftWarning
{
//	CW_STOP = -1,	// -1
	CW_NONE,		//  0
	CW_CANTREPAIR,	//  1
	CW_CANTREARM,	//  2
	CW_CANTREFUEL	//  3
};


/**
 * Represents a Craft stored in a Base.
 * @note Contains variable info about a Craft like position, fuel, damage, etc.
 * @sa RuleCraft
 */
class Craft
	:
		public MovingTarget
{

private:
	bool
		_tactical,
		_inDogfight,
		_lowFuel,
		_mission,
		_warned;
	int
		_loadCap,
		_loadCur,
		_damage,
		_fuel,
		_id,
		_takeOff,

		_kills;

	std::string _status;
	std::wstring _name;

	CraftWarning _warning;

	Base* _base;
	ItemContainer* _items;
	RuleCraft* _crRule;

	std::vector<CraftWeapon*> _weapons;
	std::vector<Vehicle*> _vehicles;


	public:
		/// Creates a craft of the specified type.
		Craft(
				RuleCraft* const crRule,
				Base* const base,
				int id = 0);
		/// Cleans up the craft.
		~Craft();

		/// Loads the craft from YAML.
		void load(
				const YAML::Node& node,
				const Ruleset* const rules,
				SavedGame* const save);
		/// Saves the craft to YAML.
		YAML::Node save() const;
		/// Saves the craft's ID to YAML.
		YAML::Node saveId() const;
		/// Loads a craft ID from YAML.
		static CraftId loadId(const YAML::Node& node);

		/// Gets the craft's ruleset.
		RuleCraft* getRules() const;
		/// Sets the craft's ruleset.
		void changeRules(RuleCraft* const crRule);

		/// Gets the craft's ID.
		int getId() const;
		/// Gets the craft's name.
		std::wstring getName(const Language* const lang) const;
		/// Sets the craft's name.
		void setName(const std::wstring& newName);

		/// Gets the craft's marker.
		int getMarker() const;

		/// Gets the craft's base.
		Base* getBase() const;
		/// Sets the craft's base.
		void setBase(
				Base* const base,
				bool transfer = true);

		/// Gets the craft's status.
		std::string getStatus() const;
		/// Sets the craft's status.
		void setStatus(const std::string& status);

		/// Gets the craft's altitude.
		std::string getAltitude() const;

		/// Sets the craft's destination.
		void setDestination(Target* const dest);

		/// Gets the craft's amount of weapons.
		int getNumWeapons() const;
		/// Gets the craft's amount of soldiers.
		int getNumSoldiers() const;
		/// Gets the craft's amount of equipment.
		int getNumEquipment() const;
		/// Gets the craft's amount of vehicles.
		int getNumVehicles(bool tiles = false) const;

		/// Gets the craft's weapons.
		std::vector<CraftWeapon*>* getWeapons();

		/// Gets the craft's items.
		ItemContainer* getItems() const;
		/// Gets the craft's vehicles.
		std::vector<Vehicle*>* getVehicles();

		/// Gets the craft's amount of fuel.
		int getFuel() const;
		/// Sets the craft's amount of fuel.
		void setFuel(int fuel);
		/// Gets the craft's percentage of fuel.
		int getFuelPercentage() const;

		/// Gets the craft's amount of damage.
		int getDamage() const;
		/// Sets the craft's amount of damage.
		void setDamage(const int damage);
		/// Gets the craft's percentage of damage.
		int getDamagePercent() const;

		/// Gets whether the craft is running out of fuel.
		bool getLowFuel() const;
		/// Sets whether the craft is running out of fuel.
		void setLowFuel(bool low);

		/// Gets whether the craft has just finished a mission.
		bool getMissionReturn() const;
		/// Sets whether the craft has just finished a mission.
		void setMissionReturn(bool mission = true);

		/// Gets the craft's distance from its base.
		double getDistanceFromBase() const;

		/// Gets the craft's fuel consumption.
		int getFuelConsumption() const;
		/// Gets the craft's minimum fuel limit.
		int getFuelLimit() const;
		/// Gets the craft's minimum fuel limit to go to a base.
		int getFuelLimit(const Base* const base) const;

		/// Returns the craft to its base.
		void returnToBase();

		/// Handles craft logic.
		void think();

		/// Does a craft full checkup.
		void checkup();

		/// Checks if a target is detected by the craft's radar.
		bool detect(const Target* const target) const;

		/// Consumes the craft's fuel.
		void consumeFuel();

		/// Repairs the Craft.
		void repair();
		/// Rearms the Craft.
		std::string rearm(const Ruleset* const rules);
		/// Refuels the Craft.
		void refuel();

		/// Sets the Craft's battlescape status.
		void setInBattlescape(bool tactical = true);
		/// Gets if the Craft is in battlescape.
		bool isInBattlescape() const;

		/// Gets if Craft is destroyed during dogfights.
		bool isDestroyed() const;

		/// Gets the amount of space available inside Craft.
		int getSpaceAvailable() const;
		/// Gets the amount of space used inside Craft.
		int getSpaceUsed() const;
		/// Gets the Craft's vehicles of a certain type.
		int getVehicleCount(const std::string& vehicle) const;

		/// Sets the Craft's dogfight status.
		void setInDogfight(const bool inDogfight);
		/// Gets if the Craft is in dogfight.
		bool isInDogfight() const;

		/// Gets the Craft's unique id.
		CraftId getUniqueId() const;

		/// Sets the Craft's capacity load.
		void setLoadCapacity(const int load);
		/// Gets the Craft's capacity load.
		int getLoadCapacity() const;
		/// Sets the Craft's current load.
//		void setLoadCurrent(const int load);
		/// Gets the Craft's current load.
		int getLoadCurrent();

		/// Gets this craft's current CraftWarning status.
		CraftWarning getWarning() const;
		/// Sets this craft's CraftWarning status.
		void setWarning(const CraftWarning warning);
		/// Gets whether a warning has been issued for this Craft.
		bool getWarned() const;
		/// Sets whether a warning has been issued for this Craft.
		void setWarned(const bool warn = true);

		/// Gets the amount of time this Craft will be repairing/rearming/refueling.
		int getDowntime(bool& delayed);

		/// Adds a dogfight kill.
		void addKill();
		/// Gets this Craft's dogfight kills.
		int getKills() const;

		/// Gets if the Craft has left the ground.
		bool getTakeoff() const;
};

}

#endif
