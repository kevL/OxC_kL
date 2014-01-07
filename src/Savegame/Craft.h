/*
 * Copyright 2010-2013 OpenXcom Developers.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_CRAFT_H
#define OPENXCOM_CRAFT_H

#include <string>
#include <vector>

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


/**
 * Represents a craft stored in a base.
 * Contains variable info about a craft like
 * position, fuel, damage, etc.
 * @sa RuleCraft
 */
class Craft
	:
		public MovingTarget
{

private:
	bool
		_inBattlescape,
		_inDogfight,
		_lowFuel;
	int
		_damage,
		_fuel,
		_id,
		_interceptionOrder;

	std::string _status;
	std::wstring _name;

	Base* _base;
	ItemContainer* _items;
	RuleCraft* _rules;

	std::vector<CraftWeapon*> _weapons;
	std::vector<Vehicle*> _vehicles;


	public:
		/// Creates a craft of the specified type.
		Craft(
				RuleCraft* rules,
				Base* base,
				int id = 0);
		/// Cleans up the craft.
		~Craft();

		/// Loads the craft from YAML.
		void load(
				const YAML::Node& node,
				const Ruleset* rule,
				SavedGame* save);
		/// Saves the craft to YAML.
		YAML::Node save() const;
		/// Saves the craft's ID to YAML.
		YAML::Node saveId() const;

		/// Gets the craft's ruleset.
		RuleCraft* getRules() const;
		/// Sets the craft's ruleset.
		void setRules(RuleCraft* rules);

		/// Gets the craft's ID.
		int getId() const;
		/// Gets the craft's name.
		std::wstring getName(Language* lang) const;
		/// Sets the craft's name.
		void setName(const std::wstring& newName);

		/// Gets the craft's base.
		Base* getBase() const;
		/// Sets the craft's base.
		void setBase(Base* base);
		/// Sets the craft's base. (without setting the craft's coordinates)
		void setBaseOnly(Base* base);

		/// Gets the craft's status.
		std::string getStatus() const;
		/// Sets the craft's status.
		void setStatus(const std::string& status);

		/// Gets the craft's altitude.
		std::string getAltitude() const;

		/// Sets the craft's destination.
		void setDestination(Target* dest);

		/// Gets the craft's amount of weapons.
		int getNumWeapons() const;
		/// Gets the craft's amount of soldiers.
		int getNumSoldiers() const;
		/// Gets the craft's amount of equipment.
		int getNumEquipment() const;
		/// Gets the craft's amount of vehicles.
		int getNumVehicles() const;

		/// Gets the craft's weapons.
		std::vector<CraftWeapon*>* getWeapons();

		/// Gets the craft's items.
		ItemContainer* getItems();
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
		void setDamage(int damage);
		/// Gets the craft's percentage of damage.
		int getDamagePercentage() const;

		/// Gets whether the craft is running out of fuel.
		bool getLowFuel() const;
		/// Sets whether the craft is running out of fuel.
		void setLowFuel(bool low);

		/// Gets the craft's distance from its base.
		double getDistanceFromBase() const;

		/// Gets the craft's fuel consumption.
		int getFuelConsumption() const;
		/// Gets the craft's minimum fuel limit.
		int getFuelLimit() const;
		/// Gets the craft's minimum fuel limit to go to a base.
		int getFuelLimit(Base* base) const;

		/// Returns the craft to its base.
		void returnToBase();

		/// Handles craft logic.
		void think();

		/// Does a craft full checkup.
		void checkup();

		/// Checks if a target is detected by the craft's radar.
		bool detect(Target* target) const;

		/// Consumes the craft's fuel.
		void consumeFuel();

		/// Repairs the craft.
		void repair();
		/// Rearms the craft.
		std::string rearm(Ruleset* rules);
		/// Refuels the craft.
		void refuel();

		/// Sets the craft's battlescape status.
		void setInBattlescape(bool inbattle);
		/// Gets if the craft is in battlescape.
		bool isInBattlescape() const;

		/// Gets if craft is destroyed during dogfights.
		bool isDestroyed() const;

		/// Gets the amount of space available inside a craft.
		int getSpaceAvailable() const;
		/// Gets the amount of space used inside a craft.
		int getSpaceUsed() const;
		/// Gets the craft's vehicles of a certain type.
		int getVehicleCount(const std::string& vehicle) const;

		/// Sets the craft's dogfight status.
		void setInDogfight(const bool inDogfight);
		/// Gets if the craft is in dogfight.
		bool isInDogfight() const;

		/// Sets interception order (first craft to leave the base gets 1, second 2, etc.).
		void setInterceptionOrder(const int order);
		/// Gets interception number.
		int getInterceptionOrder() const;
};

}

#endif
