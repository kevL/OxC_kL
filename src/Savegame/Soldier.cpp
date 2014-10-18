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

#include "Craft.h"
#include "EquipmentLayoutItem.h"
#include "SavedGame.h"
#include "SoldierDead.h"
#include "SoldierDeath.h"
#include "SoldierDiary.h"

#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/RNG.h"

#include "../Interface/Text.h"

#include "../Ruleset/Armor.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleSoldier.h"
#include "../Ruleset/SoldierNamePool.h"
#include "../Ruleset/StatString.h"


namespace OpenXcom
{

/**
 * Initializes a new soldier, either blank or randomly generated.
 * @param rules	- pointer to RuleSoldier
 * @param armor	- pointer to Armor
 * @param names	- pointer to a vector of pointers to SoldierNamePool
 * @param id	- unique soldier ID for soldier generation
 */
Soldier::Soldier(
		RuleSoldier* rules,
		Armor* armor,
		const std::vector<SoldierNamePool*>* names,
		int id)
	:
		_id(id),
		_gainPsiSkl(0),
		_gainPsiStr(0),
		_rules(rules),
		_rank(RANK_ROOKIE),
		_craft(NULL),
		_gender(GENDER_MALE),
		_look(LOOK_BLONDE),
		_missions(0),
		_kills(0),
		_recovery(0),
		_recentlyPromoted(false),
		_psiTraining(false),
		_armor(armor)
{
	_diary = new SoldierDiary();

	if (names != NULL)
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

//kL	_initialStats.psiSkill = minStats.psiSkill;
		_initialStats.psiSkill = 0; // kL

		_currentStats = _initialStats;

		// kL_begin: gender Ratios
		RuleGender gRatio = rules->getGenderRatio();
		double
			male = static_cast<double>(gRatio.male),
			total = static_cast<double>(male + gRatio.female);

		if (AreSame(total, 0.0)
			|| RNG::percent(static_cast<int>(Round(male / total * 100.0))))
		{
			_gender = GENDER_MALE;
		}
		else
			_gender = GENDER_FEMALE;

//		_gender = (SoldierGender)RNG::generate(0, 1);
		_look = (SoldierLook)RNG::generate(0, 3);

		if (_gender == GENDER_MALE)
			_name = L"pfc.Fritz";
		else
			_name = L"pfc.Frita";
		// kL_end.

/*kL
		if (names->empty() == false)
		{
			size_t nationality = RNG::generate(0, names->size() - 1);
			_name = names->at(nationality)->genName(&_gender, rules->getFemaleFrequency());

			// Once we add the ability to mod in extra looks, this will
			// need to reference the ruleset for the maximum amount of looks.
			_look = (SoldierLook)names->at(nationality)->genLook(4);
		}
		else
		{
			_name	= L"";
			_gender = (RNG::percent(rules->getFemaleFrequency())? GENDER_FEMALE: GENDER_MALE);
			_look	= (SoldierLook)RNG::generate(0, 3);
		} */
	}
}

/**
 * dTor.
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

	delete _diary;
}

/**
 * Loads the soldier from a YAML file.
 * @param node - reference a YAML node
 * @param rule - pointer to the Ruleset
 * @param save - pointer to SavedGame
 */
void Soldier::load(
		const YAML::Node& node,
		const Ruleset* rule,
		SavedGame* save)
{
	_id				= node["id"]						.as<int>(_id);
	_name			= Language::utf8ToWstr(node["name"]	.as<std::string>());
	_initialStats	= node["initialStats"]				.as<UnitStats>(_initialStats);
	_currentStats	= node["currentStats"]				.as<UnitStats>(_currentStats);
	_rank			= (SoldierRank)node["rank"]			.as<int>();
	_gender			= (SoldierGender)node["gender"]		.as<int>();
	_look			= (SoldierLook)node["look"]			.as<int>();
	_missions		= node["missions"]					.as<int>(_missions);
	_kills			= node["kills"]						.as<int>(_kills);
	_recovery		= node["recovery"]					.as<int>(_recovery);
	_psiTraining	= node["psiTraining"]				.as<bool>(_psiTraining);
	_gainPsiSkl		= node["gainPsiSkl"]				.as<int>(_gainPsiSkl);
	_gainPsiStr		= node["gainPsiStr"]				.as<int>(_gainPsiStr);

	Armor* armor = rule->getArmor(node["armor"].as<std::string>());
	if (armor == NULL)
		armor = rule->getArmor("STR_ARMOR_NONE_UC");

	_armor = armor;

	if (const YAML::Node& layout = node["equipmentLayout"])
	{
		for (YAML::const_iterator
				i = layout.begin();
				i != layout.end();
				++i)
		{
			EquipmentLayoutItem* layoutItem = new EquipmentLayoutItem(*i);
			if (rule->getInventory(layoutItem->getSlot()))
				_equipmentLayout.push_back(layoutItem);
			else
				delete layoutItem;
		}
	}

	if (node["diary"])
	{
		_diary = new SoldierDiary();
		_diary->load(node["diary"]);
	}

//	calcStatString(
//			rule->getStatStrings(),
//			(Options::psiStrengthEval
//				&& save->isResearched(rule->getPsiRequirements())));
}

/**
 * Saves the soldier to a YAML file.
 * @return, YAML node.
 */
YAML::Node Soldier::save() const
{
	YAML::Node node;

	node["id"]				= _id;
	node["name"]			= Language::wstrToUtf8(_name);
	node["initialStats"]	= _initialStats;
	node["currentStats"]	= _currentStats;
	node["rank"]			= (int)_rank;
	if (_craft != NULL)
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
	node["gainPsiSkl"]		= _gainPsiSkl;
	node["gainPsiStr"]		= _gainPsiStr;

	if (_equipmentLayout.empty() == false)
	{
		for (std::vector<EquipmentLayoutItem*>::const_iterator
				i = _equipmentLayout.begin();
				i != _equipmentLayout.end();
				++i)
		{
			node["equipmentLayout"].push_back((*i)->save());
		}
	}

	if (_diary->getMissionIdList().empty() == false
		|| _diary->getSoldierCommendations()->empty() == false)
	{
		node["diary"] = _diary->save();
	}

	return node;
}

/**
 * Returns the soldier's rules.
 * @return, rulesoldier
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
 * Gets the soldier's full name (and, optionally, statString).
 * @param statstring	- true to add stat string
 * @param maxLength		- restrict length to this value
 * @return, soldier name
 */
std::wstring Soldier::getName(
		bool statstring,
		size_t maxLength) const
{
	if (statstring
		&& _statString.empty() == false)
	{
		if (_name.length() + _statString.length() > maxLength)
		{
			return _name.substr(
							0,
							maxLength - _statString.length()) + L"/" + _statString;
		}
		else
			return _name + L"/" + _statString;
	}
/*	else if (_recovery > 0)
	{
		std::wostringstream wStr;
		wStr << _name << L" (" << _recovery << L")";
		return wStr.str();
	} */ // kL_add.

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
		sCraft = lang->getString("STR_WOUNDED").arg(Text::formatNumber(_recovery));
	else if (_craft == NULL)
		sCraft = lang->getString("STR_NONE_UC");
	else
		sCraft = _craft->getName(lang);

	return sCraft;
}

/**
 * Returns a localizable-string representation of
 * the soldier's military rank.
 * @return, String ID for rank.
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
 * @return, Rank enum.
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
 * @return, Missions.
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
 * @return, Kills.
 */
int Soldier::getKills() const
{
	return _kills;
}

/**
 * Returns the soldier's gender.
 * @return, Gender.
 */
SoldierGender Soldier::getGender() const
{
	return _gender;
}

/**
 * Returns the soldier's look.
 * @return, Look.
 */
SoldierLook Soldier::getLook() const
{
	return _look;
}

/**
 * Returns the unit's promotion status and resets it.
 * @return, True if recently promoted, False otherwise.
 */
bool Soldier::isPromoted()
{
	bool promoted = _recentlyPromoted;
	_recentlyPromoted = false;

	return promoted;
}

/**
 * Returns the unit's current armor.
 * @return, pointer to Armor data
 */
Armor* Soldier::getArmor() const
{
	return _armor;
}

/**
 * Changes the unit's current armor.
 * @param armor - pointer to Armor data
 */
void Soldier::setArmor(Armor* armor)
{
	_armor = armor;
}

/**
 * Returns the amount of time until the soldier is healed.
 * @return, Number of days.
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
		_craft = NULL;
}

/**
 * Heals soldier wounds.
 */
void Soldier::heal()
{
	_recovery--;

	if (_recovery < 0) // kL
		_recovery = 0; // kL
}

/**
 * kL. Gets a soldier's wounds as a percent.
 */
int Soldier::getWoundPercent() const // kL
{
	return static_cast<int>(
				floor(static_cast<float>(_recovery) / static_cast<float>(_currentStats.health)
					* 100.f));
}

/**
 * Returns the list of EquipmentLayoutItems of a soldier.
 * @return, Pointer to the EquipmentLayoutItem list.
 */
std::vector<EquipmentLayoutItem*>* Soldier::getEquipmentLayout()
{
	return &_equipmentLayout;
}

/**
 * Trains a soldier's Psychic abilities after 1 month.
 * kL_note: called from GeoscapeState, per 1month
 * kL_note: I don't use this, btw.
 */
void Soldier::trainPsi()
{
	int psiSkillCap = _rules->getStatCaps().psiSkill;
	int psiStrengthCap = _rules->getStatCaps().psiStrength;

	_gainPsiSkl = _gainPsiStr = 0;
	// -10 days - tolerance threshold for switch from anytimePsiTraining option.
	// If soldier has psiskill -10..-1, he was trained 20..59 days. 81.7% probability, he was trained more that 30 days.
	if (_currentStats.psiSkill < -10 + _rules->getMinStats().psiSkill)
		_currentStats.psiSkill = _rules->getMinStats().psiSkill;
	else if (_currentStats.psiSkill <= _rules->getMaxStats().psiSkill)
	{
		int max = _rules->getMaxStats().psiSkill + _rules->getMaxStats().psiSkill / 2;
		_gainPsiSkl = RNG::generate(_rules->getMaxStats().psiSkill, max);
	}
	else
	{
		if (_currentStats.psiSkill <= (psiSkillCap / 2)) _gainPsiSkl = RNG::generate(5, 12);
		else if (_currentStats.psiSkill < psiSkillCap) _gainPsiSkl = RNG::generate(1, 3);

		if (Options::allowPsiStrengthImprovement)
		{
			if (_currentStats.psiStrength <= (psiStrengthCap / 2)) _gainPsiStr = RNG::generate(5, 12);
			else if (_currentStats.psiStrength < psiStrengthCap) _gainPsiStr = RNG::generate(1, 3);
		}
	}
	_currentStats.psiSkill += _gainPsiSkl;
	_currentStats.psiStrength += _gainPsiStr;
	if (_currentStats.psiSkill > psiSkillCap) _currentStats.psiSkill = psiSkillCap;
	if (_currentStats.psiStrength > psiStrengthCap) _currentStats.psiStrength = psiStrengthCap;
/* kL_begin:
// http://www.ufopaedia.org/index.php?title=Psi_Skill
// -End of Month PsiLab Increase-
// Skill	Range	Average
// 0-16		16-24	20.0
// 17-50	5-12	8.5
// 51+		1-3		2.0

	_gainPsiSkl = 0;
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
//kL		_gainPsiSkl = RNG::generate(_rules->getMaxStats().psiSkill, max);
		_gainPsiSkl = RNG::generate(16, 24);
	}
//kL	else if (_currentStats.psiSkill <= _rules->getStatCaps().psiSkill / 2)
//	else if (_currentStats.psiSkill <= _rules->getStatCaps().psiSkill / 4)		// kL
	else if (_currentStats.psiSkill < 51)										// kL
	{
		_gainPsiSkl = RNG::generate(5, 12);
	}
//kL	else if (_currentStats.psiSkill < _rules->getStatCaps().psiSkill)
	else if (_currentStats.psiSkill < psiCap)
	{
		_gainPsiSkl = RNG::generate(1, 3);
	}

	_currentStats.psiSkill += _gainPsiSkl;

	if (_currentStats.psiSkill > psiCap)	// kL
	{
		_currentStats.psiSkill = psiCap;	// kL
	}

	// kL_note: This isn't right; in Orig. the only way to increase psiSkill
	// when it was over 100 is to keep the soldier in training.
//kL	if (_currentStats.psiSkill > 100)
//kL		_currentStats.psiSkill = 100;
*/ // kL_end.
}

/**
 * Trains a soldier's Psychic abilities ('anytimePsiTraining' option) after 1 day.
 * kL_note: called from GeoscapeState, per 1day. Re-write done.
 */
void Soldier::trainPsi1Day()
{
	_gainPsiSkl = 0; // was used in AllocatePsiTrainingState.

	if (_psiTraining == false)
		return;

	int const psiSkill = _currentStats.psiSkill;
	int const rulesMin = _rules->getMinStats().psiSkill;

	if (psiSkill >= _rules->getStatCaps().psiSkill) // hard cap. Note this auto-caps psiStrength also
		return;
	else if (psiSkill >= rulesMin) // Psi unlocked.
	{
		int chance = std::max(
							1,
							std::min(
									100,
									500 / psiSkill));
		if (RNG::percent(chance))
		{
			_gainPsiSkl++;
			_currentStats.psiSkill++;
		}

		if (Options::allowPsiStrengthImprovement)
		{
			int const psiStrength = _currentStats.psiStrength;
			if (psiStrength < _rules->getStatCaps().psiStrength)
			{
				chance = std::max(
								1,
								std::min(
										100,
										500 / psiStrength));
				if (RNG::percent(chance))
				{
					_gainPsiStr++;
					_currentStats.psiStrength++;
				}
			}
		}
	}
	else // start here.
	{
		if (RNG::percent(5)) // 5% per day per soldier (to become psionic-active)
		{
			_gainPsiSkl = RNG::generate(
									rulesMin,
									_rules->getMaxStats().psiSkill);

			_currentStats.psiSkill =_initialStats.psiSkill = _gainPsiSkl;
		}
	}
}

/**
 * Gets whether or not a soldier is in psi training.
 * @return, true if training
 */
bool Soldier::isInPsiTraining()
{
	return _psiTraining;
}

/**
 * Toggles whether or not a soldier is in psi training.
 */
void Soldier::setPsiTraining()
{
	_psiTraining = !_psiTraining;
}

/**
 * Gets this soldier's psiSkill improvement during the current month.
 * @return, psi-skill improvement
 */
int Soldier::getImprovement()
{
	return _gainPsiSkl;
}

/**
 * Gets this soldier's psiStrength improvement during the current month.
 * @return, psi-strength improvement
 */
int Soldier::getPsiStrImprovement()
{
	return _gainPsiStr;
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
 * @param death - pointer to SoldierDeath time-data
 * @return, pointer to SoldierDead
 */
SoldierDead* Soldier::die(SoldierDeath* death)
{
	SoldierDead* dead = new SoldierDead(
									_name,
									_id,
									_rank,
									_gender,
									_look,
									_missions,
									_kills,
									death,
									_initialStats,
									_currentStats,
									_diary);
									// base if I want to...

	return dead;
}

/**
 * Gets the soldier's diary.
 * @return, pointer to SoldierDiary
 */
SoldierDiary* Soldier::getDiary()
{
	return _diary;
}

/**
 * Calculates a soldier's statString.
 * @param statStrings		- reference to a vector of pointers to statString rules
 * @param psiStrengthEval	- true if psi stats are available
 */
/* void Soldier::calcStatString(
		const std::vector<StatString*>& statStrings,
		bool psiStrengthEval)
{
	_statString = StatString::calcStatString(
										_currentStats,
										statStrings,
										psiStrengthEval);
} */

}
