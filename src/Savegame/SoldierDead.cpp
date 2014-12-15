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

#include "SoldierDeath.h"
#include "SoldierDiary.h"

#include "../Engine/Language.h"


namespace OpenXcom
{

/**
 * Initializes a new dead soldier.
 * @param name			-
 * @param id			-
 * @param unitRank		-
 * @param gender		-
 * @param look			-
 * @param missions		-
 * @param kills			-
 * @param death			- pointer to SoldierDeath time
 * @param initialStats	-
 * @param currentStats	-
 * @param diary			- pointer to SoldierDiary
 */
SoldierDead::SoldierDead(
		const std::wstring name,
		const int id,
		const SoldierRank unitRank,
		const SoldierGender gender,
		const SoldierLook look,
		const int missions,
		const int kills,
		SoldierDeath* const death,
		const UnitStats initialStats,
		const UnitStats currentStats,
		SoldierDiary* const diary)
		// base if I want to...
	:
		_name(name),
		_id(id),
		_rank(unitRank),
		_gender(gender),
		_look(look),
		_missions(missions),
		_kills(kills),
		_death(death),
		_initialStats(initialStats),
		_currentStats(currentStats)
//		_diary(diary)
{
	// Copy the diary instead of setting a pointer,
	// because delete Soldier will delete the old diary
	// (unless you want to do tricky things that i don't).
//	if (_diary == NULL)
	_diary = new SoldierDiary();
	_diary = diary;
}

/**
 *
 */
SoldierDead::~SoldierDead()
{
	delete _death;
	delete _diary;
}

/**
 * Loads a dead soldier from a YAML file.
 * @param node - reference a YAML node
 */
void SoldierDead::load(const YAML::Node& node)
{
	_name			= Language::utf8ToWstr(node["name"]	.as<std::string>());
	_id				= node["id"]						.as<int>(_id);
	_initialStats	= node["initialStats"]				.as<UnitStats>(_initialStats);
	_currentStats	= node["currentStats"]				.as<UnitStats>(_currentStats);
	_rank			= (SoldierRank)node["rank"]			.as<int>();
	_gender			= (SoldierGender)node["gender"]		.as<int>();
	_look			= (SoldierLook)node["look"]			.as<int>();
	_missions		= node["missions"]					.as<int>(_missions);
	_kills			= node["kills"]						.as<int>(_kills);

	_death = new SoldierDeath();
	_death->load(node["death"]);

	if (node["diary"])
	{
		_diary = new SoldierDiary();
		_diary->load(node["diary"]);
	}
}

/**
 * Saves this dead soldier to a YAML file.
 * @return, YAML node
 */
YAML::Node SoldierDead::save() const
{
	YAML::Node node;

	node["name"]			= Language::wstrToUtf8(_name);
	node["id"]				= _id;
	node["initialStats"]	= _initialStats;
	node["currentStats"]	= _currentStats;
	node["rank"]			= static_cast<int>(_rank);
	node["gender"]			= static_cast<int>(_gender);
	node["look"]			= static_cast<int>(_look);
	node["missions"]		= _missions;
	node["kills"]			= _kills;

	node["death"]			= _death->save();

	if (_diary != NULL
		&& (_diary->getMissionIdList().empty() == false
			|| _diary->getSoldierCommendations()->empty() == false))
	{
		node["diary"] = _diary->save();
	}

	return node;
}

/**
 * Returns this dead soldier's full name.
 * @return, name string
 */
std::wstring SoldierDead::getName() const
{
	return _name;
}

/**
 * Returns a localizable-string representation of this dead soldier's military rank.
 * @return, string ID of rank
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
	}

	return "";
}

/**
 * Returns a graphic representation of this dead soldier's military rank.
 * @note THE MEANING OF LIFE
 * @return, sprite ID of rank
 */
int SoldierDead::getRankSprite() const
{
	return 42 + _rank;
}

/**
 * Returns this dead soldier's military rank.
 * @return, rank enum
 */
SoldierRank SoldierDead::getRank() const
{
	return _rank;
}

/**
 * Returns this dead soldier's amount of missions.
 * @return, missions
 */
int SoldierDead::getMissions() const
{
	return _missions;
}

/**
 * Returns this dead soldier's amount of kills.
 * @return, kills
 */
int SoldierDead::getKills() const
{
	return _kills;
}

/**
 * Returns this dead soldier's gender.
 * @return, gender enum
 */
SoldierGender SoldierDead::getGender() const
{
	return _gender;
}

/**
 * Returns this dead soldier's look.
 * @return, look enum
 */
SoldierLook SoldierDead::getLook() const
{
	return _look;
}

/**
 * Returns this dead soldier's unique ID.
 * Each dead soldier can be identified by its ID (not it's name).
 * @return, unique ID
 */
int SoldierDead::getId() const
{
	return _id;
}

/**
 * Gets pointer to initial stats.
 * @return, pointer to UnitStats struct
 */
UnitStats* SoldierDead::getInitStats()
{
	return &_initialStats;
}

/**
 * Gets pointer to current stats.
 * @return, pointer to UnitStats struct
 */
UnitStats* SoldierDead::getCurrentStats()
{
	return &_currentStats;
}

/**
 * Returns the dead soldier's time of death.
 * @return, pointer to SoldierDeath
 */
SoldierDeath* SoldierDead::getDeath() const
{
	return _death;
}

/**
 * Gets the soldier's diary.
 * @return, pointer to SoldierDiary
 */
SoldierDiary* SoldierDead::getDiary() const
{
	return _diary;
}

}
