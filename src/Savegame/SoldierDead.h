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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_SOLDIERDEAD_H
#define OPENXCOM_SOLDIERDEAD_H

#include <string>

#include <yaml-cpp/yaml.h>

//#include "../Ruleset/RuleSoldier.h"
//#include "../Ruleset/Unit.h"

#include "../Savegame/Soldier.h"


namespace OpenXcom
{

//class Armor;
class Language;
//class Ruleset;
//class RuleSoldier;
class Soldier;
class SoldierDeath;
//enum SoldierGender;
//enum SoldierLook;
//enum SoldierRank;


/**
 * Represents a dead soldier killed by the player.
 * Dead Soldiers have a wide variety of stats that affect
 * our memory of their heroic, and not so heroic, battles.
 */
class SoldierDead
{

private:
	int
		_id,
		_kills,
		_missions;

	std::wstring _name;

//	Armor* _armor;
//	RuleSoldier* _rules;
	SoldierDeath* _death;
	SoldierGender _gender;
	SoldierLook _look;
	SoldierRank _rank;
//	UnitStats
//		_initialStats,
//		_currentStats;


	public:
		/// Creates a new dead soldier.
		SoldierDead(
				std::wstring name,
				int id,
				SoldierRank rank,
				SoldierGender gender,
				SoldierLook look,
				int missions,
				int kills,
				SoldierDeath* death);
//				RuleSoldier* rules,
//				Armor* armor,
		/// Cleans up the dead soldier.
		~SoldierDead();

		/// Loads the dead soldier from YAML.
		void load(
				const YAML::Node& node);
//				const Ruleset* rule);
		/// Saves the dead soldier to YAML.
		YAML::Node save() const;

		/// Gets the dead soldier's name.
		std::wstring getName() const;
		/// Sets the dead soldier's name.
//		void setName(const std::wstring& name);

		/// Gets a string version of the dead soldier's rank.
		std::string getRankString() const;
		/// Gets a sprite version of the dead soldier's rank.
//		int getRankSprite() const;
		/// Gets the dead soldier's rank.
//		SoldierRank getRank() const;

		/// Gets the dead soldier's missions.
//		int getMissions() const;
		/// Gets the dead soldier's kills.
//		int getKills() const;

		/// Gets the dead soldier's gender.
//		SoldierGender getGender() const;
		/// Gets the dead soldier's look.
//		SoldierLook getLook() const;
		/// Gets dead soldier rules.
//		RuleSoldier* getRules() const;
		/// Gets the dead soldier's unique ID.
//		int getId() const;

		/// Get pointer to initial stats.
//		UnitStats* getInitStats();
		/// Get pointer to current stats.
//		UnitStats* getCurrentStats();

		/// Gets the dead soldier armor.
//		Armor* getArmor() const;
		/// Sets the dead soldier armor.
//		void setArmor(Armor* armor);

		/// Gets the dead soldier death info.
		SoldierDeath* getDeath() const;
};

}

#endif
