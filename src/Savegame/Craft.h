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
		_tacticalDone,
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

	/// Calculates the Craft's minimum fuel required to go to a base.
	int calcFuelLimit(const Base* const base) const;


	public:
		/// Creates a Craft of the specified type.
		Craft(
				RuleCraft* const crRule,
				Base* const base,
				int id = 0);
		/// Cleans up the Craft.
		~Craft();

		/// Loads the Craft from YAML.
		void load(
				const YAML::Node& node,
				const Ruleset* const rules,
				SavedGame* const save);
		/// Saves the Craft to YAML.
		YAML::Node save() const;
		/// Saves the Craft's ID to YAML.
		YAML::Node saveId() const;
		/// Loads a Craft ID from YAML.
		static CraftId loadId(const YAML::Node& node);

		/// Gets the Craft's ruleset.
		RuleCraft* getRules() const;
		/// Sets the Craft's ruleset.
		void changeRules(RuleCraft* const crRule);

		/// Gets the Craft's ID.
		int getId() const;
		/// Gets the Craft's name.
		std::wstring getName(const Language* const lang) const;
		/// Sets the Craft's name.
		void setName(const std::wstring& newName);

		/// Gets the Craft's marker.
		int getMarker() const;

		/// Gets the Craft's base.
		Base* getBase() const;
		/// Sets the Craft's base.
		void setBase(
				Base* const base,
				bool transfer = true);

		/// Gets the Craft's status.
		std::string getCraftStatus() const;
		/// Sets the Craft's status.
		void setCraftStatus(const std::string& status);

		/// Gets the Craft's altitude.
		std::string getAltitude() const;

		/// Sets the Craft's destination.
		void setDestination(Target* const dest);

		/// Gets the Craft's amount of weapons.
		int getNumWeapons() const;
		/// Gets the Craft's amount of soldiers.
		int getNumSoldiers() const;
		/// Gets the Craft's amount of equipment.
		int getNumEquipment() const;
		/// Gets the Craft's amount of vehicles.
		int getNumVehicles(bool tiles = false) const;

		/// Gets the Craft's weapons.
		std::vector<CraftWeapon*>* getWeapons();

		/// Gets the Craft's items.
		ItemContainer* getCraftItems() const;
		/// Gets the Craft's vehicles.
		std::vector<Vehicle*>* getVehicles();

		/// Gets the Craft's amount of damage.
		int getCraftDamage() const;
		/// Sets the Craft's amount of damage.
		void setCraftDamage(const int damage);
		/// Gets the Craft's percentage of damage.
		int getCraftDamagePct() const;

		/// Gets the Craft's amount of fuel.
		int getFuel() const;
		/// Sets the Craft's amount of fuel.
		void setFuel(int fuel);
		/// Gets the Craft's percentage of fuel.
		int getFuelPct() const;
		/// Gets whether the Craft is running out of fuel.
		bool getLowFuel() const;
		/// Sets whether the Craft is running out of fuel.
		void setLowFuel(bool low = true);
		/// Consumes the Craft's fuel.
		void consumeFuel();
		/// Gets the Craft's fuel consumption.
		int getFuelConsumption() const;
		/// Gets the Craft's minimum fuel limit.
		int getFuelLimit() const;

		/// Returns the Craft to its base.
		void returnToBase();
		/// Gets whether the Craft has just finished a mission.
		bool getTacticalReturn() const;
		/// Sets that the Craft has just finished a mission.
		void setTacticalReturn();

		/// Handles Craft logic.
		void think();

		/// Does a Craft full checkup.
		void checkup();

		/// Checks if a target is detected by the Craft's radar.
		bool detect(const Target* const target) const;

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
		int calcLoadCurrent();

		/// Gets the Craft's current CraftWarning status.
		CraftWarning getWarning() const;
		/// Sets the Craft's CraftWarning status.
		void setWarning(const CraftWarning warning);
		/// Gets whether a warning has been issued for this Craft.
		bool getWarned() const;
		/// Sets whether a warning has been issued for this Craft.
		void setWarned(const bool warned = true);

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
