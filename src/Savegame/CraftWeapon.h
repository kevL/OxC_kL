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

#ifndef OPENXCOM_CRAFTWEAPON_H
#define OPENXCOM_CRAFTWEAPON_H

#include <string>

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

class CraftWeaponProjectile;
class RuleCraftWeapon;
class Ruleset;


/**
 * Represents a craft weapon equipped by a craft.
 * Contains variable info about a craft weapon like ammo.
 * @sa RuleCraftWeapon
 */
class CraftWeapon
{

private:
	bool
		_cantLoad,
		_rearming;
	int _ammo;

	RuleCraftWeapon* _rules;


	public:
		/// Creates a craft weapon of the specified type.
		CraftWeapon(
				RuleCraftWeapon* rules,
				int ammo);
		/// Cleans up the craft weapon.
		~CraftWeapon();

		/// Loads the craft weapon from YAML.
		void load(const YAML::Node& node);
		/// Saves the craft weapon to YAML.
		YAML::Node save() const;

		/// Gets the craft weapon's ruleset.
		RuleCraftWeapon* getRules() const;

		/// Gets the craft weapon's ammo.
		int getAmmo() const;
		/// Sets the craft weapon's ammo.
		bool setAmmo(int ammo);
		/// Gets the craft weapon's rearming status.
		bool getRearming() const;
		/// Sets the craft weapon's rearming status
		void setRearming(const bool rearming = true);
		/// Rearms the craft weapon.
		int rearm(
				const int baseClips,
				const int clipSize);
		/// Gets this CraftWeapon's cantLoad status - no stock in Base Stores.
		bool getCantLoad() const;
		/// Sets this CraftWeapon's cantLoad status - no stock in Base Stores.
		void setCantLoad(const bool cantLoad = true);

		/// Fires the craft weapon - used during dogfights.
		CraftWeaponProjectile* fire() const;

		/// Gets how many clips are loaded into this weapon.
		int getClipsLoaded(Ruleset* ruleset);
};

}

#endif
