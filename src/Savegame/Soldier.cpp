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

#include "Soldier.h"

#include "SoldierDead.h" // kL
//#include "SoldierDeath.h" // kL

#include "../Engine/Language.h"
#include "../Engine/RNG.h"

#include "../Ruleset/Armor.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleSoldier.h"
#include "../Ruleset/SoldierNamePool.h"

#include "../Savegame/Craft.h"
#include "../Savegame/EquipmentLayoutItem.h"
#include "../Savegame/SoldierDeath.h"


namespace OpenXcom
{

/**
 * Initializes a new soldier, either blank or randomly generated.
 * @param rules, Soldier ruleset.
 * @param armor, Soldier armor.
 * @param names, List of name pools for soldier generation.
 * @param id, Pointer to unique soldier id for soldier generation.
 */
Soldier::Soldier(
		RuleSoldier* rules,
		Armor* armor,
		const std::vector<SoldierNamePool*>* names,
		int id)
	:
		_name(L""),
		_id(0),
		_improvement(0),
		_rules(rules),
		_initialStats(),
		_currentStats(),
		_rank(RANK_ROOKIE),
		_craft(0),
		_gender(GENDER_MALE),
		_look(LOOK_BLONDE),
		_missions(0),
		_kills(0),
		_recovery(0),
		_recentlyPromoted(false),
		_psiTraining(false),
		_armor(armor),
		_equipmentLayout(),
		_death(0)
{
	if (names != 0)
	{
		UnitStats minStats = rules->getMinStats();
		UnitStats maxStats = rules->getMaxStats();

		_initialStats.tu			= RNG::generate(minStats.tu, maxStats.tu);
		_initialStats.stamina		= RNG::generate(minStats.stamina, maxStats.stamina);
		_initialStats.health		= RNG::generate(minStats.health, maxStats.health);
		_initialStats.bravery		= RNG::generate(minStats.bravery / 10, maxStats.bravery / 10) * 10;
		_initialStats.reactions		= RNG::generate(minStats.reactions, maxStats.reactions);
		_initialStats.firing		= RNG::generate(minStats.firing, maxStats.firing);
		_initialStats.throwing		= RNG::generate(minStats.throwing, maxStats.throwing);
		_initialStats.strength		= RNG::generate(minStats.strength, maxStats.strength);
		_initialStats.psiStrength	= RNG::generate(minStats.psiStrength, maxStats.psiStrength);
		_initialStats.melee			= RNG::generate(minStats.melee, maxStats.melee);

//kL		_initialStats.psiSkill = minStats.psiSkill;
		_initialStats.psiSkill = minStats.psiSkill - 2; // kL

		_currentStats = _initialStats;

		if (!names->empty())
		{
			int nationality = RNG::generate(0, names->size() - 1);
			_name = names->at(nationality)->genName(&_gender);

			// Once we add the ability to mod in extra looks, this will
			// need to reference the ruleset for the maximum amount of looks.
			_look = (SoldierLook)names->at(nationality)->genLook(4);
		}
		else
		{
			_name = L"";
			_gender = (SoldierGender)RNG::generate(0, 1);
			_look = (SoldierLook)RNG::generate(0, 3);
		}
	}

	if (id != 0)
		_id = id;
}

/**
 *
 */
Soldier::~Soldier()
{
	for (std::vector<EquipmentLayoutItem*>::iterator
			i = _equipmentLayout.begin();
			i != _equipmentLayout.end();
			++i)
	{
		delete *i;
	}

	delete _death;
}

/**
 * Loads the soldier from a YAML file.
 * @param node YAML node.
 * @param rule Game ruleset.
 */
void Soldier::load(
		const YAML::Node& node,
		const Ruleset* rule)
{
	_id				= node["id"].as<int>(_id);
	_name			= Language::utf8ToWstr(node["name"].as<std::string>());
//	_name			= Language::utf8ToWstr(node["name"].as<std::wstring>()); // kL
	_initialStats	= node["initialStats"].as<UnitStats>(_initialStats);
	_currentStats	= node["currentStats"].as<UnitStats>(_currentStats);
	_rank			= (SoldierRank)node["rank"].as<int>();
	_gender			= (SoldierGender)node["gender"].as<int>();
	_look			= (SoldierLook)node["look"].as<int>();
	_missions		= node["missions"].as<int>(_missions);
	_kills			= node["kills"].as<int>(_kills);
	_recovery		= node["recovery"].as<int>(_recovery);
	_armor			= rule->getArmor(node["armor"].as<std::string>());
	_psiTraining	= node["psiTraining"].as<bool>(_psiTraining);
	_improvement	= node["improvement"].as<int>(_improvement);

	if (const YAML::Node& layout = node["equipmentLayout"])
	{
		for (YAML::const_iterator
				i = layout.begin();
				i != layout.end();
				++i)
			_equipmentLayout.push_back(new EquipmentLayoutItem(*i));
	}

	// kL_note: This may be obsolete, since SoldierDead was put in.
	// ie, SoldierDeath should be part of SoldierDead, not class Soldier here.
	if (node["death"])
	{
		_death = new SoldierDeath();
		_death->load(node["death"]);
	}
}

/**
 * Saves the soldier to a YAML file.
 * @return YAML node.
 */
YAML::Node Soldier::save() const
{
	YAML::Node node;

	node["id"]				= _id;
	node["name"]			= Language::wstrToUtf8(_name);
	node["initialStats"]	= _initialStats;
	node["currentStats"]	= _currentStats;
	node["rank"]			= (int)_rank;
	if (_craft != 0)
		node["craft"]		= _craft->saveId();
	node["gender"]			= (int)_gender;
	node["look"]			= (int)_look;
	node["missions"]		= _missions;
	node["kills"]			= _kills;
	if (_recovery > 0)
		node["recovery"]	= _recovery;
	node["armor"]			= _armor->getType();
	if (_psiTraining)
		node["psiTraining"]	= _psiTraining;
	node["improvement"]		= _improvement;

	if (!_equipmentLayout.empty())
	{
		for (std::vector<EquipmentLayoutItem*>::const_iterator
				i = _equipmentLayout.begin();
				i != _equipmentLayout.end();
				++i)
			node["equipmentLayout"].push_back((*i)->save());
	}

	// kL_note: This may be obsolete, since SoldierDead was put in.
	// ie, SoldierDeath should be part of SoldierDead, not class Soldier.
	if (_death != 0)
		node["death"] = _death->save();

	return node;
}

/**
 * Returns the soldier's rules.
 * @return rulesoldier
 */
RuleSoldier* Soldier::getRules() const
{
	return _rules;
}

/**
 * Add a mission to the counter.
 */
/**
 * Get pointer to initial stats.
 */
UnitStats* Soldier::getInitStats()
{
	return &_initialStats;
}

/**
 * Get pointer to current stats.
 */
UnitStats* Soldier::getCurrentStats()
{
	return &_currentStats;
}

/**
 * Returns the soldier's unique ID. Each soldier
 * can be identified by its ID. (not its name)
 * @return, Unique ID.
 */
int Soldier::getId() const
{
	return _id;
}

/**
 * Returns the soldier's full name.
 * @return, Soldier name.
 */
std::wstring Soldier::getName() const
{
	return _name;
}

/**
 * Changes the soldier's full name.
 * @param name, Soldier name.
 */
void Soldier::setName(const std::wstring& name)
{
	_name = name;
}

/**
 * Returns the craft the soldier is assigned to.
 * @return, Pointer to craft.
 */
Craft* Soldier::getCraft() const
{
	return _craft;
}

/**
 * Assigns the soldier to a craft.
 * @param, craft Pointer to craft.
 */
void Soldier::setCraft(Craft* craft)
{
	_craft = craft;
}

/**
 * Returns the soldier's craft string, which is either the
 * soldier's wounded status, the assigned craft name, or none.
 * @param lang, Language to get strings from.
 * @return, Full name.
 */
std::wstring Soldier::getCraftString(Language* lang) const
{
	std::wstring sCraft;
	if (_recovery > 0)
		sCraft = lang->getString("STR_WOUNDED");
	else if (_craft == 0)
		sCraft = lang->getString("STR_NONE_UC");
	else
		sCraft = _craft->getName(lang);

	return sCraft;
}

/**
 * Returns a localizable-string representation of
 * the soldier's military rank.
 * @return String ID for rank.
 */
std::string Soldier::getRankString() const
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
 * Returns a graphic representation of the soldier's military rank.
 * @note THE MEANING OF LIFE
 * @return, Sprite ID for rank
 */
int Soldier::getRankSprite() const
{
	return 42 + _rank;
}

/**
 * Returns the soldier's military rank.
 * @return Rank enum.
 */
SoldierRank Soldier::getRank() const
{
	return _rank;
}

/**
 * Increase the soldier's military rank.
 */
void Soldier::promoteRank()
{
	_rank = static_cast<SoldierRank>(static_cast<int>(_rank) + 1);

	if (_rank > RANK_SQUADDIE) // only promotions above SQUADDIE are worth mentioning.
		_recentlyPromoted = true;
}

void Soldier::addMissionCount()
{
	_missions++;
}

/**
 * Returns the soldier's amount of missions.
 * @return Missions.
 */
int Soldier::getMissions() const
{
	return _missions;
}

/**
 * Add a kill to the counter.
 */
void Soldier::addKillCount(int count)
{
	_kills += count;
}

/**
 * Returns the soldier's amount of kills.
 * @return Kills.
 */
int Soldier::getKills() const
{
	return _kills;
}

/**
 * Returns the soldier's gender.
 * @return Gender.
 */
SoldierGender Soldier::getGender() const
{
	return _gender;
}

/**
 * Returns the soldier's look.
 * @return Look.
 */
SoldierLook Soldier::getLook() const
{
	return _look;
}

/**
 * Returns the unit's promotion status and resets it.
 * @return True if recently promoted, False otherwise.
 */
bool Soldier::isPromoted()
{
	bool promoted = _recentlyPromoted;
	_recentlyPromoted = false;

	return promoted;
}

/**
 * Returns the unit's current armor.
 * @return, Pointer to armor data.
 */
Armor* Soldier::getArmor() const
{
	return _armor;
}

/**
 * Changes the unit's current armor.
 * @param armor Pointer to armor data.
 */
void Soldier::setArmor(Armor* armor)
{
	_armor = armor;
}

/**
 * Returns the amount of time until the soldier is healed.
 * @return Number of days.
 */
int Soldier::getWoundRecovery() const
{
	return _recovery;
}

/**
 * Changes the amount of time until the soldier is healed.
 * @param recovery Number of days.
 */
void Soldier::setWoundRecovery(int recovery)
{
	_recovery = recovery;
	if (_recovery > 0) // dismiss from craft
		_craft = 0;
}

/**
 * Heals soldier wounds.
 */
void Soldier::heal()
{
	_recovery--;
}

/**
 * Returns the list of EquipmentLayoutItems of a soldier.
 * @return Pointer to the EquipmentLayoutItem list.
 */
std::vector<EquipmentLayoutItem*>* Soldier::getEquipmentLayout()
{
	return &_equipmentLayout;
}

/**
 * Trains a soldier's Psychic abilities
 * kL_note: called from GeoscapeState, per 1month
 * kL_note: I don't use this, btw.
 */
void Soldier::trainPsi()
{
// http://www.ufopaedia.org/index.php?title=Psi_Skill
// -End of Month PsiLab Increase-
// Skill	Range	Average
// 0-16		16-24	20.0
// 17-50	5-12	8.5
// 51+		1-3		2.0

	_improvement = 0;
	int const psiCap = _rules->getStatCaps().psiSkill; // kL

	// -10 days : tolerance threshold for switch from anytimePsiTraining option.
	// If soldier has psiSkill -10..-1, he was trained 20..59 days.
	// 81.7% probability, he was trained more than 30 days.
	if (_currentStats.psiSkill < _rules->getMinStats().psiSkill - 10)
	{
		_currentStats.psiSkill = _rules->getMinStats().psiSkill;
	}
//kL	else if (_currentStats.psiSkill <= _rules->getMaxStats().psiSkill)
	else if (_currentStats.psiSkill < 17) // kL
	{
//kL		int max = _rules->getMaxStats().psiSkill + _rules->getMaxStats().psiSkill / 2;
//kL		_improvement = RNG::generate(_rules->getMaxStats().psiSkill, max);
		_improvement = RNG::generate(16, 24);
	}
//kL	else if (_currentStats.psiSkill <= _rules->getStatCaps().psiSkill / 2)
//	else if (_currentStats.psiSkill <= _rules->getStatCaps().psiSkill / 4)		// kL
	else if (_currentStats.psiSkill < 51)										// kL
	{
		_improvement = RNG::generate(5, 12);
	}
//kL	else if (_currentStats.psiSkill < _rules->getStatCaps().psiSkill)
	else if (_currentStats.psiSkill < psiCap)
	{
		_improvement = RNG::generate(1, 3);
	}

	_currentStats.psiSkill += _improvement;

	if (_currentStats.psiSkill > psiCap)	// kL
	{
		_currentStats.psiSkill = psiCap;	// kL
	}

	// kL_note: This isn't right; in Orig. the only way to increase psiSkill
	// when it was over 100 is to keep the soldier in training.
//kL	if (_currentStats.psiSkill > 100)
//kL		_currentStats.psiSkill = 100;
}

/**
 * Trains a soldier's Psychic abilities ('anytimePsiTraining' option)
 * kL_note: called from GeoscapeState, per 1day. Re-write done.
 */
void Soldier::trainPsi1Day()
{
	_improvement = 0; // was used in AllocatePsiTrainingState.

	if (!_psiTraining) return;

	int const psiSkill = _currentStats.psiSkill;
	int const rulesMin = _rules->getMinStats().psiSkill;

	if (psiSkill >= _rules->getStatCaps().psiSkill) // hard cap.
		return;
	else if (psiSkill >= rulesMin) // farther along
	{
		int chance = std::max(
							1,
							std::min(
									100,
									500 / psiSkill));
		if (RNG::percent(chance))
		{
			++_improvement;
			++_currentStats.psiSkill;
		}
	}
	else if (psiSkill == rulesMin - 1) // ready to go
	{
		_improvement = RNG::generate(
								rulesMin,
								_rules->getMaxStats().psiSkill);

		_currentStats.psiSkill = _improvement;
	}
	else // start here
	{
		if (RNG::percent(1))
			_currentStats.psiSkill = rulesMin - 1;
//		else
//			_currentStats.psiSkill = rulesMin - 2;
	}
}

/**
 * returns whether or not the unit is in psi training
 */
bool Soldier::isInPsiTraining()
{
	return _psiTraining;
}

/**
 * changes whether or not the unit is in psi training
 */
void Soldier::setPsiTraining()
{
	_psiTraining = !_psiTraining;
}

/**
 * returns this soldier's psionic improvement score for this month.
 */
int Soldier::getImprovement()
{
	return _improvement;
}

/**
 * Returns the soldier's death time.
 * @return, Pointer to death data. NULL if no death has occured.
 */
/*kL SoldierDeath* Soldier::getDeath() const
{
	return _death;
} */

/**
 * Kills the soldier in the Geoscape.
 * @param, death Pointer to death data.
 * @return SoldierDead*, Pointer to a DeadSoldier template.
 */
//kL void Soldier::die(SoldierDeath* death)
SoldierDead* Soldier::die(SoldierDeath* death)
{
	delete _death;
	_death = death;

	// Clean up associations
	_craft = 0;
	_psiTraining = false;
	_recentlyPromoted = false;
	_recovery = 0;

	for (std::vector<EquipmentLayoutItem*>::iterator
			i = _equipmentLayout.begin();
			i != _equipmentLayout.end();
			++i)
		delete *i;

	_equipmentLayout.clear();

	// kL_begin:
	SoldierDead* dead = new SoldierDead(
									_name,
									_id,
									_rank,
									_gender,
									_look,
									_missions,
									_kills,
									_death);
/*										L"",
										0,
										RANK_ROOKIE,
										GENDER_MALE,
										LOOK_BLONDE,
										0,
										0,
										NULL); */

	return dead;
//	SavedGame::getDeadSoldiers()->push_back(ds);
	// kL_end.
}

}
