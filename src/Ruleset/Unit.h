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

#ifndef OPENXCOM_UNIT_H
#define OPENXCOM_UNIT_H

#include <string>

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

enum SpecialAbility
{
	SPECAB_NONE,			// 0
	SPECAB_EXPLODEONDEATH,	// 1
	SPECAB_BURNFLOOR,		// 2
	SPECAB_RESPAWN			// 3
};


/**
 * This struct holds some plain unit attribute data together.
 */
struct UnitStats
{
	int
		tu,
		stamina,
		health,
		bravery,
		reactions,
		firing,
		throwing,
		strength,
		psiStrength,
		psiSkill,
		melee;

//	http://courses.cms.caltech.edu/cs11/material/cpp/donnie/cpp-ops.html
/*  MyClass& MyClass::operator=(const MyClass &rhs) {

    // Only do assignment if RHS is a different object from this.
    if (this != &rhs) {
      ... // Deallocate, allocate new space, copy values...
    }

    return *this;
  } */
	public:
		UnitStats()
			:
				tu(0),
				stamina(0),
				health(0),
				bravery(0),
				reactions(0),
				firing(0),
				throwing(0),
				strength(0),
				psiStrength(0),
				psiSkill(0),
				melee(0)
		{
		};

		UnitStats(
				int tu_,
				int stamina_,
				int health_,
				int bravery_,
				int reactions_,
				int firing_,
				int throwing_,
				int strength_,
				int psiStrength_,
				int psiSkill_,
				int melee_)
			:
				tu(tu_),
				stamina(stamina_),
				health(health_),
				bravery(bravery_),
				reactions(reactions_),
				firing(firing_),
				throwing(throwing_),
				strength(strength_),
				psiStrength(psiStrength_),
				psiSkill(psiSkill_),
				melee(melee_)
		{
		};

		UnitStats& operator+=(const UnitStats& stats)
		{
			if (this != &stats) // kL: cf. Position.h
			{
				tu			+= stats.tu;
				stamina		+= stats.stamina;
				health		+= stats.health;
				bravery		+= stats.bravery;
				reactions	+= stats.reactions;
				firing		+= stats.firing;
				throwing	+= stats.throwing;
				strength	+= stats.strength;
				psiStrength	+= stats.psiStrength;
				psiSkill	+= stats.psiSkill;
				melee		+= stats.melee;
			}

			return *this;
		}

		UnitStats operator+(const UnitStats& stats) const
		{
			return UnitStats(
						tu			+ stats.tu,
						stamina		+ stats.stamina,
						health		+ stats.health,
						bravery		+ stats.bravery,
						reactions	+ stats.reactions,
						firing		+ stats.firing,
						throwing	+ stats.throwing,
						strength	+ stats.strength,
						psiStrength	+ stats.psiStrength,
						psiSkill	+ stats.psiSkill,
						melee		+ stats.melee);
		}
};


/**
 * Represents the static data for a unit that is generated on
 * the battlescape, this includes: HWPs, aliens and civilians.
 * @sa Soldier BattleUnit
 */
class Unit
{

private:
	bool _livingWeapon;
	int
		_floatHeight,
		_kneelHeight,
		_standHeight;
	int
		_aggression,
		_aggroSound,
		_deathSound,
		_energyRecovery,
		_intelligence,
		_moveSound,
		_value;

	std::string
		_armor,
		_race,
		_rank,
		_type,
		_spawnUnit;

	SpecialAbility _specab;
	UnitStats _stats;


	public:
		/// Creates a blank unit ruleset.
		Unit(const std::string& type);
		/// Cleans up the unit ruleset.
		~Unit();

		/// Loads the unit data from YAML.
		void load(
				const YAML::Node& node,
				int modIndex);
		/// Gets the unit's type.
		std::string getType() const;
		/// Gets the unit's stats.
		UnitStats* getStats();
		/// Gets the unit's height when standing.
		int getStandHeight() const;
		/// Gets the unit's height when kneeling.
		int getKneelHeight() const;
		/// Gets the unit's float elevation.
		int getFloatHeight() const;
		/// Gets the armor type.
		std::string getArmor() const;
		/// Gets the alien race type.
		std::string getRace() const;
		/// Gets the alien rank.
		std::string getRank() const;
		/// Gets the value - for score calculation.
		int getValue() const;
		/// Gets the death sound id.
		int getDeathSound() const;
		/// Gets the move sound id.
		int getMoveSound() const;
		/// Gets the intelligence. This is the number of turns AI remembers your troop positions.
		int getIntelligence() const;
		/// Gets the aggression. Determines the chance of revenge and taking cover.
		int getAggression() const;
		/// Gets the alien's special ability.
		int getSpecialAbility() const;
		/// Gets the unit's spawn unit.
		std::string getSpawnUnit() const;
		/// Gets the unit's war cry.
		int getAggroSound() const;
		/// Checks if this unit has a built in weapon.
		bool isLivingWeapon() const;
		///
		int getEnergyRecovery() const;
};

}


namespace YAML
{

template<>
struct convert<OpenXcom::UnitStats>
{
	static Node encode(const OpenXcom::UnitStats& rhs)
	{
		Node node;

		node["tu"]			= rhs.tu;
		node["stamina"]		= rhs.stamina;
		node["health"]		= rhs.health;
		node["bravery"]		= rhs.bravery;
		node["reactions"]	= rhs.reactions;
		node["firing"]		= rhs.firing;
		node["throwing"]	= rhs.throwing;
		node["strength"]	= rhs.strength;
		node["psiStrength"]	= rhs.psiStrength;
		node["psiSkill"]	= rhs.psiSkill;
		node["melee"]		= rhs.melee;

		return node;
	}

	static bool decode(
			const Node& node,
			OpenXcom::UnitStats& rhs)
	{
		if (!node.IsMap()) return false;

		rhs.tu			= node["tu"].as<int>(rhs.tu);
		rhs.stamina		= node["stamina"].as<int>(rhs.stamina);
		rhs.health		= node["health"].as<int>(rhs.health);
		rhs.bravery		= node["bravery"].as<int>(rhs.bravery);
		rhs.reactions	= node["reactions"].as<int>(rhs.reactions);
		rhs.firing		= node["firing"].as<int>(rhs.firing);
		rhs.throwing	= node["throwing"].as<int>(rhs.throwing);
		rhs.strength	= node["strength"].as<int>(rhs.strength);
		rhs.psiStrength	= node["psiStrength"].as<int>(rhs.psiStrength);
		rhs.psiSkill	= node["psiSkill"].as<int>(rhs.psiSkill);
		rhs.melee		= node["melee"].as<int>(rhs.melee);

		return true;
	}
};

}

#endif
