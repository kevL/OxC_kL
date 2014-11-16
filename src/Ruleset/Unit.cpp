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

#include "Unit.h"

#include "../Engine/Exception.h"


namespace OpenXcom
{

/**
 * Creates a certain type of unit.
 * @param type - string defining the Unit's type
 */
Unit::Unit(const std::string& type)
	:
		_type(type),
		_standHeight(0),
		_kneelHeight(0),
		_floatHeight(0),

		_value(0),
		_deathSound(0),
		_aggroSound(-1),
		_moveSound(-1),
		_intelligence(0),
		_aggression(0),
		_energyRecovery(30),
		_specab(SPECAB_NONE),
		_livingWeapon(false),
		_female(false),
		_isMechanical(false), // kL: these two should perhaps go to Armor class.
		_isPsiImmune(false)
{
}

/**
 * dTor.
 */
Unit::~Unit()
{
	//Log(LOG_INFO) << "Delete Unit";
}

/**
 * Loads the unit from a YAML file.
 * @param node		- YAML node
 * @param modIndex	- a value that offsets the sounds and sprite values to avoid conflicts
 */
void Unit::load(
		const YAML::Node& node,
		int modIndex)
{
	//Log(LOG_INFO) << "Unit::load()";
	_type			= node["type"]					.as<std::string>(_type);
	_race			= node["race"]					.as<std::string>(_race);
	_rank			= node["rank"]					.as<std::string>(_rank);
	_stats			.mergeStats(node["stats"]		.as<UnitStats>(_stats));
	_armor			= node["armor"]					.as<std::string>(_armor);
	_standHeight	= node["standHeight"]			.as<int>(_standHeight);
	_kneelHeight	= node["kneelHeight"]			.as<int>(_kneelHeight);
	_floatHeight	= node["floatHeight"]			.as<int>(_floatHeight);
	if (_floatHeight + _standHeight > 25)
	{
		throw Exception("Error with unit " + _type + ": Unit height may not exceed 25");
	}
	_value			= node["value"]					.as<int>(_value);
	_intelligence	= node["intelligence"]			.as<int>(_intelligence);
	_aggression		= node["aggression"]			.as<int>(_aggression);
	_energyRecovery	= node["energyRecovery"]		.as<int>(_energyRecovery);
	_specab			= (SpecialAbility)node["specab"].as<int>(_specab);
	_spawnUnit		= node["spawnUnit"]				.as<std::string>(_spawnUnit);
	_livingWeapon	= node["livingWeapon"]			.as<bool>(_livingWeapon);
	_meleeWeapon	= node["meleeWeapon"]			.as<std::string>(_meleeWeapon);
	_builtInWeapons	= node["builtInWeapons"]		.as<std::vector<std::string> >(_builtInWeapons);
	_female			= node["female"]				.as<bool>(_female);
	_isMechanical	= node["isMechanical"]			.as<bool>(_isMechanical);
	_isPsiImmune	= node["isPsiImmune"]			.as<bool>(_isPsiImmune);

	if (node["deathSound"])
	{
		_deathSound = node["deathSound"].as<int>(_deathSound);
		if (_deathSound > 54) // BATTLE.CAT: 55 entries
			_deathSound += modIndex;
	}

	if (node["aggroSound"])
	{
		_aggroSound = node["aggroSound"].as<int>(_aggroSound);
		if (_aggroSound > 54) // BATTLE.CAT: 55 entries
			_aggroSound += modIndex;
	}

	if (node["moveSound"])
	{
		_moveSound = node["moveSound"].as<int>(_moveSound);
		if (_moveSound > 54) // BATTLE.CAT: 55 entries
			_moveSound += modIndex;
	}
}

/**
 * Returns the language string that names this unit. Each unit type has a unique name.
 * @return, the unit's name
 */
std::string Unit::getType() const
{
	return _type;
}

/**
 * Returns the unit's stats data object.
 * @return, pointer to the unit's stats
 */
UnitStats* Unit::getStats()
{
	return &_stats;
}

/**
 * Returns the unit's height at standing.
 * @return, the unit's height
 */
int Unit::getStandHeight() const
{
	return _standHeight;
}

/**
 * Returns the unit's height at kneeling.
 * @return, the unit's kneeling height
 */
int Unit::getKneelHeight() const
{
	return _kneelHeight;
}

/**
 * Returns the unit's floating elevation.
 * @return, the unit's floating height
 */
int Unit::getFloatHeight() const
{
	return _floatHeight;
}

/**
 * Gets the unit's armor type.
 * @return, the unit's armor type
 */
std::string Unit::getArmor() const
{
	return _armor;
}

/**
 * Gets the alien's race.
 * @return, the alien's race
 */
std::string Unit::getRace() const
{
	return _race;
}

/**
 * Gets the unit's rank.
 * @return, the unit's rank
 */
std::string Unit::getRank() const
{
	return _rank;
}

/**
 * Gets the unit's value - for scoring.
 * @return, the unit's value
 */
int Unit::getValue() const
{
	return _value;
}

/**
 * Gets the unit's death sound.
 * @return, the ID of the unit's death sound
 */
int Unit::getDeathSound() const
{
	return _deathSound;
}

/**
 * Gets the unit's move sound.
 * @return, the ID of the unit's movement sound
 */
int Unit::getMoveSound() const
{
	return _moveSound;
}

/**
 * Gets the unit's war cry.
 * @return, the ID of the unit's aggro sound
 */
int Unit::getAggroSound() const
{
	return _aggroSound;
}

/**
 * Gets the intelligence. This is the number of turns the AI remembers your troop positions.
 * @return, the unit's intelligence
 */
int Unit::getIntelligence() const
{
	return _intelligence;
}

/**
 * Gets the aggression. Determines the chance of revenge and taking cover.
 * @return, the unit's aggression
 */
int Unit::getAggression() const
{
	return _aggression;
}

/**
 * Gets the unit's special ability.
 * @return, the unit's specab
 */
int Unit::getSpecialAbility() const
{
	return static_cast<int>(_specab);
}

/**
 * Gets the unit that is spawned when this one dies.
 * @return, the unit's spawn unit
 */
std::string Unit::getSpawnUnit() const
{
	return _spawnUnit;
}

/**
 * Gets stamina recovery per turn as a percentage.
 * @return, rate of stamina recovery
 */
const int Unit::getEnergyRecovery() const
{
	return _energyRecovery;
}

/**
 * Checks if this unit is a living weapon. (ie: chryssalid).
 * A living weapon ignores any loadout that may be available to
 * its rank and uses the one associated with its race.
 * @return, true if this unit is a living weapon
 */
const bool Unit::isLivingWeapon() const
{
	return _livingWeapon;
}

/**
 * Gets this unit's built in melee weapon if any.
 * @return, the name of the weapon
 */
const std::string Unit::getMeleeWeapon() const
{
	return _meleeWeapon;
}

/**
 * Gets what weapons this unit has built in.
 * This is a vector of strings representing any weapons that may be inherent
 * to this creature.
 * Note: unlike "livingWeapon" this is used in ADDITION to any loadout or
 * living weapon item that may be defined.
 * @return, list of weapons that are integral to this unit.
 */
const std::vector<std::string>& Unit::getBuiltInWeapons() const
{
	return _builtInWeapons;
}

/**
 * Gets if this Unit is female.
 * @return, true if female
 */
const bool Unit::isFemale() const
{
	return _female;
}

/**
 * Gets if this Unit is a mechanical apparatus.
 * NOTE about _isMechanical:
 * This var subsumes several more detailed ideas:
 * - isTrackedVehicle
 * - isPsiAttackable / isSentient <- DONE
 * - canRevive (from status_Unconscious)
 * - canChangeMorale (see isFearable())
 * - isInfectable (can have a spawnUnit string set on it)
 * - isMetal (cannot catch fire)
 * - canOpenDoors
 * etc.
 * @return, true if this is a non-organic, purely mechanical unit on the battlefield
 */
const bool Unit::getMechanical() const
{
	return _isMechanical;
}

/**
 * Gets if this Unit is immune to psionic attacks.
 * @return, true if unit is immune to Psi
 */
const bool Unit::getPsiImmune() const
{
	return _isPsiImmune;
}

}
