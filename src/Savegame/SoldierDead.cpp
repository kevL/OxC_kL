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

#include "SoldierDead.h"

#include "Soldier.h"
#include "SoldierDeath.h"

#include "../Engine/Language.h"
//#include "../Engine/RNG.h"

//#include "../Ruleset/Armor.h"
//#include "../Ruleset/Ruleset.h"
//#include "../Ruleset/RuleSoldier.h"
//#include "../Ruleset/SoldierNamePool.h"

//#include "../Savegame/Craft.h"
//#include "../Savegame/EquipmentLayoutItem.h"
//#include "../Savegame/SoldierDeath.h"


namespace OpenXcom
{

/**
 * Initializes a new dead soldier, either blank or randomly generated.
 */
SoldierDead::SoldierDead(
		std::wstring name,
		int id,
		SoldierRank rank,
		SoldierGender gender,
		SoldierLook look,
		int missions,
		int kills,
		SoldierDeath* death)
//		RuleSoldier* rules,
//		Armor* armor)
	:
		_name(name),
		_id(id),
		_rank(rank),
		_gender(gender),
		_look(look),
		_missions(missions),
		_kills(kills),
		_death(death)
/*		_name(L""),
		_id(0),
//		_rules(rules),
//		_initialStats(),
//		_currentStats(),
		_rank(RANK_ROOKIE),
		_gender(GENDER_MALE),
		_look(LOOK_BLONDE),
		_missions(0),
		_kills(0),
//		_armor(armor),
		_death(0) */
{
/*	UnitStats minStats = rules->getMinStats();
	UnitStats maxStats = rules->getMaxStats();

	_initialStats.tu			= RNG::generate(minStats.tu, maxStats.tu);
	_initialStats.stamina		= RNG::generate(minStats.stamina, maxStats.stamina);
	_initialStats.health		= RNG::generate(minStats.health, maxStats.health);
	_initialStats.bravery		= RNG::generate(minStats.bravery/10, maxStats.bravery/10)*10;
	_initialStats.reactions		= RNG::generate(minStats.reactions, maxStats.reactions);
	_initialStats.firing		= RNG::generate(minStats.firing, maxStats.firing);
	_initialStats.throwing		= RNG::generate(minStats.throwing, maxStats.throwing);
	_initialStats.strength		= RNG::generate(minStats.strength, maxStats.strength);
	_initialStats.psiStrength	= RNG::generate(minStats.psiStrength, maxStats.psiStrength);
	_initialStats.melee			= RNG::generate(minStats.melee, maxStats.melee);

	_initialStats.psiSkill = minStats.psiSkill;

	_currentStats = _initialStats; */

/*	_name = name;
	_id = id;
	_rank = rank;
	_gender = gender;
	_look = look;
	_missions = missions;
	_kills = kills;

	_death = death; */
}

/**
 *
 */
SoldierDead::~SoldierDead()
{
	delete _death;
}

/**
 * Loads the dead soldier from a YAML file.
 * @param node, YAML node.
 * @param rule, Game ruleset.
 */
void SoldierDead::load(
		const YAML::Node& node)
//		const Ruleset* rule)
{
	_name			= Language::utf8ToWstr(node["name"].as<std::string>());
	_id				= node["id"].as<int>(_id);
//	_initialStats	= node["initialStats"].as<UnitStats>(_initialStats);
//	_currentStats	= node["currentStats"].as<UnitStats>(_currentStats);
	_rank			= (SoldierRank)node["rank"].as<int>();
	_gender			= (SoldierGender)node["gender"].as<int>();
	_look			= (SoldierLook)node["look"].as<int>();
	_missions		= node["missions"].as<int>(_missions);
	_kills			= node["kills"].as<int>(_kills);
//	_armor			= rule->getArmor(node["armor"].as<std::string>());

//	if (node["death"])
//	{
	_death = new SoldierDeath();
	_death->load(node["death"]);
//	}
}

/**
 * Saves the dead soldier to a YAML file.
 * @return, YAML node.
 */
YAML::Node SoldierDead::save() const
{
	YAML::Node node;

	node["name"]			= Language::wstrToUtf8(_name);
	node["id"]				= _id;
//	node["initialStats"]	= _initialStats;
//	node["currentStats"]	= _currentStats;
	node["rank"]			= static_cast<int>(_rank);
	node["gender"]			= static_cast<int>(_gender);
	node["look"]			= static_cast<int>(_look);
	node["missions"]		= _missions;
	node["kills"]			= _kills;
//	node["armor"]			= _armor->getType();

//	if (_death != 0)
	node["death"] = _death->save();

	return node;
}

/**
 * Returns the dead soldier's full name.
 * @return Soldier name.
 */
std::wstring SoldierDead::getName() const
{
	return _name;
}

/**
 * Changes the dead soldier's full name.
 * @param name Soldier name.
 */
/* void SoldierDead::setName(const std::wstring& name)
{
	_name = name;
} */

/**
 * Returns a localizable-string representation of
 * the dead soldier's military rank.
 * @return String ID for rank.
 */
std::string SoldierDead::getRankString() const
{
	switch (_rank)
	{
		case RANK_ROOKIE:		return "STR_ROOKIE";
		case RANK_SQUADDIE:		return "STR_SQUADDIE";
		case RANK_SERGEANT:		return "STR_SERGEANT";
		case RANK_CAPTAIN:		return "STR_CAPTAIN";
		case RANK_COLONEL:		return "STR_COLONEL";
		case RANK_COMMANDER:	return "STR_COMMANDER";

		default:
			return "";
	}

	return "";
}

/**
 * Returns a graphic representation of the dead soldier's military rank.
 * @note THE MEANING OF LIFE
 * @return, Sprite ID for rank.
 */
/* int SoldierDead::getRankSprite() const
{
	return 42 + _rank;
} */

/**
 * Returns the dead soldier's military rank.
 * @return Rank enum.
 */
/* SoldierRank SoldierDead::getRank() const
{
	return _rank;
} */

/**
 * Returns the dead soldier's amount of missions.
 * @return Missions.
 */
/* int SoldierDead::getMissions() const
{
	return _missions;
} */

/**
 * Returns the dead soldier's amount of kills.
 * @return Kills.
 */
/* int SoldierDead::getKills() const
{
	return _kills;
} */

/**
 * Returns the dead soldier's gender.
 * @return Gender.
 */
/* SoldierGender SoldierDead::getGender() const
{
	return _gender;
} */

/**
 * Returns the dead soldier's look.
 * @return Look.
 */
/* SoldierLook SoldierDead::getLook() const
{
	return _look;
} */

/**
 * Returns the dead soldier's rules.
 * @return rulesoldier
 */
/* RuleSoldier* SoldierDead::getRules() const
{
	return _rules;
} */

/**
 * Returns the dead soldier's unique ID. Each dead soldier
 * can be identified by its ID. (not it's name)
 * @return Unique ID.
 */
/* int SoldierDead::getId() const
{
	return _id;
} */

/**
 * Get pointer to initial stats.
 */
/* UnitStats* Soldier::getInitStats()
{
	return &_initialStats;
} */

/**
 * Get pointer to current stats.
 */
/* UnitStats* Soldier::getCurrentStats()
{
	return &_currentStats;
} */

/**
 * Returns the unit's current armor.
 * @return Pointer to armor data.
 */
/* Armor* Soldier::getArmor() const
{
	return _armor;
} */

/**
 * Changes the unit's current armor.
 * @param armor Pointer to armor data.
 */
/* void Soldier::setArmor(Armor* armor)
{
	_armor = armor;
} */

/**
 * Returns the dead soldier's time of death.
 * @return, Pointer to death data. NULL if no death has occured.
 */
SoldierDeath* SoldierDead::getDeath() const
{
	return _death;
}

}
