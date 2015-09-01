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

#include "RuleUnit.h"

//#include "../Engine/Exception.h"


namespace OpenXcom
{

/**
 * Creates rules for a certain type of unit.
 * @param type - reference the Unit's type
 */
RuleUnit::RuleUnit(const std::string& type)
	:
		_type(type),
		_standHeight(0),
		_kneelHeight(0),
		_floatHeight(0),
		_value(0),
		_deathSound(-1),
		_aggroSound(-1),
		_moveSound(-1),
		_intelligence(0),
		_aggression(0),
		_energyRecovery(30),
		_specab(SPECAB_NONE),
		_livingWeapon(false),
		_female(false),
		_dog(false),
		_mechanical(false), // kL: these two should perhaps go to Armor class.
		_psiImmune(false),
		_hasHands(true)
{}

/**
 * dTor.
 */
RuleUnit::~RuleUnit()
{}

/**
 * Loads the unit from a YAML file.
 * @param node		- YAML node
 * @param modIndex	- a value that offsets the sounds and sprite values to avoid conflicts
 */
void RuleUnit::load(
		const YAML::Node& node,
		int modIndex)
{
	_type			= node["type"]				.as<std::string>(_type);
	_race			= node["race"]				.as<std::string>(_race);
	_rank			= node["rank"]				.as<std::string>(_rank);
	_stats			.mergeStats(node["stats"]	.as<UnitStats>(_stats));
	_armor			= node["armor"]				.as<std::string>(_armor);
	_standHeight	= node["standHeight"]		.as<int>(_standHeight);
	_kneelHeight	= node["kneelHeight"]		.as<int>(_kneelHeight);
	_floatHeight	= node["floatHeight"]		.as<int>(_floatHeight);

	if (_floatHeight + _standHeight > 24)
	{
		throw Exception("Error with unit " + _type + ": Unit height + float height may not exceed 24");
	}

	_value			= node["value"]			.as<int>(_value);
	_intelligence	= node["intelligence"]	.as<int>(_intelligence);
	_aggression		= node["aggression"]	.as<int>(_aggression);
	_energyRecovery	= node["energyRecovery"].as<int>(_energyRecovery);
	_livingWeapon	= node["livingWeapon"]	.as<bool>(_livingWeapon);
	_meleeWeapon	= node["meleeWeapon"]	.as<std::string>(_meleeWeapon);
	_builtInWeapons	= node["builtInWeapons"].as<std::vector<std::string> >(_builtInWeapons);
	_female			= node["female"]		.as<bool>(_female);
	_dog			= node["dog"]			.as<bool>(_dog);
	_mechanical		= node["mechanical"]	.as<bool>(_mechanical);
	_psiImmune		= node["psiImmune"]		.as<bool>(_psiImmune);
	_hasHands		= node["hasHands"]		.as<bool>(_hasHands);
	_spawnUnit		= node["spawnUnit"]		.as<std::string>(_spawnUnit);
	_specab			= static_cast<SpecialAbility>(node["specab"].as<int>(_specab));

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
 * Returns the string-ID that identifies this type of unit.
 * @note Each unit is identified by race and if alien rank.
 * @return, the unit's name
 */
std::string RuleUnit::getType() const
{
	return _type;
}

/**
 * Returns the unit's stats data object.
 * @return, pointer to the unit's stats
 */
UnitStats* RuleUnit::getStats()
{
	return &_stats;
}

/**
 * Returns the unit's height at standing.
 * @return, the unit's height
 */
int RuleUnit::getStandHeight() const
{
	return _standHeight;
}

/**
 * Returns the unit's height at kneeling.
 * @return, the unit's kneeling height
 */
int RuleUnit::getKneelHeight() const
{
	return _kneelHeight;
}

/**
 * Returns the unit's floating elevation.
 * @return, the unit's floating height
 */
int RuleUnit::getFloatHeight() const
{
	return _floatHeight;
}

/**
 * Gets the unit's armor type.
 * @return, the unit's armor type
 */
std::string RuleUnit::getArmor() const
{
	return _armor;
}

/**
 * Gets the unit's race.
 * @return, the unit's race
 */
std::string RuleUnit::getRace() const
{
	return _race;
}

/**
 * Gets the unit's rank.
 * @return, the unit's rank
 */
std::string RuleUnit::getRank() const
{
	return _rank;
}

/**
 * Gets the unit's value - for scoring.
 * @return, the unit's value
 */
int RuleUnit::getValue() const
{
	return _value;
}

/**
 * Gets the unit's death sound.
 * @return, the ID of the unit's death sound
 */
int RuleUnit::getDeathSound() const
{
	return _deathSound;
}

/**
 * Gets the unit's move sound.
 * @return, the ID of the unit's movement sound
 */
int RuleUnit::getMoveSound() const
{
	return _moveSound;
}

/**
 * Gets the unit's war cry.
 * @return, the ID of the unit's aggro sound
 */
int RuleUnit::getAggroSound() const
{
	return _aggroSound;
}

/**
 * Gets the intelligence. This is the number of turns the AI remembers your troop positions.
 * @return, the unit's intelligence
 */
int RuleUnit::getIntelligence() const
{
	return _intelligence;
}

/**
 * Gets the aggression. Determines the chance of revenge and taking cover.
 * @return, the unit's aggression
 */
int RuleUnit::getAggression() const
{
	return _aggression;
}

/**
 * Gets the unit's special ability.
 * @return, the unit's specab
 */
SpecialAbility RuleUnit::getSpecialAbility() const
{
	return _specab;
}

/**
 * Gets the unit that is spawned when this one dies.
 * @return, the unit's spawn unit
 */
const std::string& RuleUnit::getSpawnUnit() const
{
	return _spawnUnit;
}

/**
 * Gets stamina recovery per turn as a percentage.
 * @return, rate of stamina recovery
 */
int RuleUnit::getEnergyRecovery() const
{
	return _energyRecovery;
}

/**
 * Checks if this unit is a living weapon - eg: chryssalid.
 * @note A living weapon ignores any loadout that may be available to its rank
 * and uses the one associated with its race. This is applied only to aLien
 * terroristic units in BattlescapeGenerator::deployAliens() where the string
 * "_WEAPON" is added to their type to get their weapon.
 * @return, true if this unit is a living weapon
 */
bool RuleUnit::isLivingWeapon() const
{
	return _livingWeapon;
}

/**
 * Gets this unit's built in melee weapon if any.
 * @return, the name of the weapon
 */
const std::string& RuleUnit::getMeleeWeapon() const
{
	return _meleeWeapon;
}

/**
 * Gets what weapons this unit has built in.
 * @note This is a vector of strings representing any weapons that are inherent
 * to this creature. Unlike '_livingWeapon' this is used in ADDITION to any
 * loadout or living weapon item that may be defined.
 * @return, list of weapons that are integral to this unit.
 */
const std::vector<std::string>& RuleUnit::getBuiltInWeapons() const
{
	return _builtInWeapons;
}

/**
 * Gets if this Unit is female.
 * @return, true if female
 */
bool RuleUnit::isFemale() const
{
	return _female;
}

/**
 * Gets if this RuleUnit is dog.
 * @return, true if dog
 */
bool RuleUnit::isDog() const
{
	return _dog;
}

/**
 * Gets if this Unit is a mechanical apparatus.
 * @note This var subsumes several more detailed ideas:
 * - isTrackedVehicle
 * - isPsiAttackable / isSentient <- DONE
 * - canRevive (from status_Unconscious)
 * - canChangeMorale (see isFearable())
 * - isInfectable (can have a spawnUnit string set on it)
 * - isMetal (cannot catch fire)
 * - canOpenDoors etc.
 * @return, true if this is a non-organic purely mechanical unit
 */
bool RuleUnit::isMechanical() const
{
	return _mechanical;
}

/**
 * Gets if this Unit is immune to psionic attacks.
 * @return, true if unit is immune to Psi
 */
bool RuleUnit::isPsiImmune() const
{
	return _psiImmune;
}

/**
 * Gets if this unit can open a door w/ RMB click or prime a grenade during
 * battle.
 * @note Units can always open doors by moving through them if space permits as
 * well as prime grenades if in preBattle inventory.
 * @return, true if this unit can open doors
 */
bool RuleUnit::hasHands() const
{
	return _hasHands;
}

}
