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

#include "Soldier.h"

#include "Craft.h"
#include "EquipmentLayoutItem.h"
#include "SavedGame.h"
#include "SoldierDead.h"
#include "SoldierDeath.h"
#include "SoldierDiary.h"

#include "../Engine/Language.h"
//#include "../Engine/RNG.h"

#include "../Interface/Text.h"

#include "../Ruleset/RuleArmor.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleSoldier.h"
//#include "../Ruleset/SoldierNamePool.h"
//#include "../Ruleset/StatString.h"


namespace OpenXcom
{

/**
 * Initializes a new soldier, either blank or randomly generated.
 * @param rules	- pointer to RuleSoldier
 * @param armor	- pointer to RuleArmor
 * @param names	- pointer to a vector of pointers to SoldierNamePool (default NULL)
 * @param id	- unique soldier ID for soldier generation (default 0)
 */
Soldier::Soldier(
		RuleSoldier* rules,
		RuleArmor* armor,
		const std::vector<SoldierNamePool*>* names,
		int id)
	:
		_rules(rules),
		_armor(armor),
		_id(id),
		_rank(RANK_ROOKIE),
		_gender(GENDER_MALE),
		_look(LOOK_BLONDE),
		_craft(NULL),
		_missions(0),
		_kills(0),
		_recovery(0),
//		_gainPsiSkl(0),
//		_gainPsiStr(0),
		_psiTraining(false),
		_recentlyPromoted(false)
{
	_diary = new SoldierDiary();

	if (names != NULL)
	{
		const UnitStats
			minStats = _rules->getMinStats(),
			maxStats = _rules->getMaxStats();

		_initialStats.tu			= RNG::generate(minStats.tu,			maxStats.tu);
		_initialStats.stamina		= RNG::generate(minStats.stamina,		maxStats.stamina);
		_initialStats.health		= RNG::generate(minStats.health,		maxStats.health);
		_initialStats.bravery		= RNG::generate(minStats.bravery / 10,	maxStats.bravery / 10) * 10;
		_initialStats.reactions		= RNG::generate(minStats.reactions,		maxStats.reactions);
		_initialStats.firing		= RNG::generate(minStats.firing,		maxStats.firing);
		_initialStats.throwing		= RNG::generate(minStats.throwing,		maxStats.throwing);
		_initialStats.strength		= RNG::generate(minStats.strength,		maxStats.strength);
		_initialStats.psiStrength	= RNG::generate(minStats.psiStrength,	maxStats.psiStrength);
		_initialStats.melee			= RNG::generate(minStats.melee,			maxStats.melee);

		_initialStats.psiSkill = 0;

		_currentStats = _initialStats;

		_look = static_cast<SoldierLook>(RNG::generate(0,3));
//		_gender = (SoldierGender)RNG::generate(0, 1);

		// kL_begin: gender Ratios
		const RuleGender* const gRatio = _rules->getGenderRatio();
		const double
			male = static_cast<double>(gRatio->male),
			total = static_cast<double>(male + gRatio->female);

		if (AreSame(total, 0.)
			|| RNG::percent(static_cast<int>(Round(male / total * 100.))))
		{
			_gender = GENDER_MALE;
			_name = L"pfc.Fritz";
		}
		else
		{
			_gender = GENDER_FEMALE;
			_name = L"pfc.Frita";
		} // kL_end.

/*		if (names->empty() == false)
		{
			size_t nationality = RNG::generate(0, names->size() - 1);
			_name = names->at(nationality)->genName(&_gender, _rules->getFemaleFrequency());

			// Once we add the ability to mod in extra looks, this will
			// need to reference the ruleset for the maximum amount of looks.
			_look = (SoldierLook)names->at(nationality)->genLook(4);
		}
		else
		{
			_name = L"";
			_gender = (RNG::percent(_rules->getFemaleFrequency()) ? GENDER_FEMALE : GENDER_MALE);
			_look = (SoldierLook)RNG::generate(0, 3);
		} */
	}
}

/**
 * dTor.
 */
Soldier::~Soldier()
{
	for (std::vector<EquipmentLayoutItem*>::const_iterator
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
// * @param save - pointer to SavedGame
 */
void Soldier::load(
		const YAML::Node& node,
		const Ruleset* rule)
//		SavedGame* save)
{
	_rank			= static_cast<SoldierRank>(node["rank"]		.as<int>());
	_gender			= static_cast<SoldierGender>(node["gender"]	.as<int>());
	_look			= static_cast<SoldierLook>(node["look"]		.as<int>());

	_id				= node["id"]						.as<int>(_id);
	_name			= Language::utf8ToWstr(node["name"]	.as<std::string>());
	_initialStats	= node["initialStats"]				.as<UnitStats>(_initialStats);
	_currentStats	= node["currentStats"]				.as<UnitStats>(_currentStats);
	_missions		= node["missions"]					.as<int>(_missions);
	_kills			= node["kills"]						.as<int>(_kills);
	_recovery		= node["recovery"]					.as<int>(_recovery);
	_psiTraining	= node["psiTraining"]				.as<bool>(_psiTraining);
//	_gainPsiSkl		= node["gainPsiSkl"]				.as<int>(_gainPsiSkl);
//	_gainPsiStr		= node["gainPsiStr"]				.as<int>(_gainPsiStr);

	RuleArmor* armor = rule->getArmor(node["armor"].as<std::string>());
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
			EquipmentLayoutItem* const layoutItem = new EquipmentLayoutItem(*i);
			if (rule->getInventory(layoutItem->getSlot()))
				_equipmentLayout.push_back(layoutItem);
			else
				delete layoutItem;
		}
	}

	if (node["diary"])
	{
//		_diary = new SoldierDiary();
		_diary->load(node["diary"]);
	}

//	calcStatString(
//			rule->getStatStrings(),
//			(Options::psiStrengthEval
//				&& save->isResearched(rule->getPsiRequirements())));
}

/**
 * Saves the soldier to a YAML file.
 * @return, YAML node
 */
YAML::Node Soldier::save() const
{
	YAML::Node node;

	node["rank"]			= static_cast<int>(_rank);
	node["gender"]			= static_cast<int>(_gender);
	node["look"]			= static_cast<int>(_look);

	node["id"]				= _id;
	node["name"]			= Language::wstrToUtf8(_name);
	node["initialStats"]	= _initialStats;
	node["currentStats"]	= _currentStats;
	node["missions"]		= _missions;
	node["kills"]			= _kills;
	node["armor"]			= _armor->getType();
//	node["gainPsiSkl"]		= _gainPsiSkl;
//	node["gainPsiStr"]		= _gainPsiStr;

	if (_craft != NULL)
		node["craft"]		= _craft->saveId();
	if (_recovery > 0)
		node["recovery"]	= _recovery;
	if (_psiTraining)
		node["psiTraining"]	= _psiTraining;

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
		|| _diary->getSoldierAwards()->empty() == false)
	{
		node["diary"] = _diary->save();
	}

	return node;
}

/**
 * Gets this Soldier's rules.
 * @return, pointer to RuleSoldier
 */
const RuleSoldier* const Soldier::getRules() const
{
	return _rules;
}

/**
 * Gets this Soldier's initial stats.
 * @return, address of UnitStats
 */
UnitStats* Soldier::getInitStats()
{
	return &_initialStats;
}

/**
 * Gets this Soldier's current stats.
 * @return, address of UnitStats
 */
UnitStats* Soldier::getCurrentStats()
{
	return &_currentStats;
}

/**
 * Gets this Soldier's unique ID.
 * @note Each soldier is uniquely identified by its ID.
 * @return, unique ID
 */
int Soldier::getId() const
{
	return _id;
}

/**
 * Gets this Soldier's full name and optionally statString.
 * @param statstring	- true to add stat string
 * @param maxLength		- restrict length to this value
 * @return, soldier name
 */
std::wstring Soldier::getName() const
{
	return _name;
}
/* std::wstring Soldier::getName(
		bool statstring,
		size_t maxLength) const
{
	if (statstring == true && _statString.empty() == false)
	{
		if (_name.length() + _statString.length() > maxLength)
			return _name.substr(0, maxLength - _statString.length()) + L"/" + _statString;
		else return _name + L"/" + _statString;
	}
	return _name;
} */

/**
 * Changes this Soldier's full name.
 * @param name - reference the soldier name
 */
void Soldier::setName(const std::wstring& name)
{
	_name = name;
}

/**
 * Gets the craft this Soldier is assigned to.
 * @return, pointer to Craft
 */
Craft* Soldier::getCraft() const
{
	return _craft;
}

/**
 * Assigns this Soldier to a craft.
 * @param craft - pointer to Craft (default NULL)
 */
void Soldier::setCraft(Craft* const craft)
{
	_craft = craft;
}

/**
 * Gets this Soldier's craft string, which is either the
 * soldier's wounded status, the assigned craft name, or none.
 * @param lang - pointer to Language to get strings from
 * @return, full name
 */
std::wstring Soldier::getCraftString(Language* lang) const
{
	std::wstring ret;

	if (_recovery > 0)
		ret = lang->getString("STR_WOUNDED").arg(Text::formatNumber(_recovery));
	else if (_craft == NULL)
		ret = lang->getString("STR_NONE_UC");
	else
		ret = _craft->getName(lang);

	return ret;
}

/**
 * Gets a localizable-string representation of this Soldier's military rank.
 * @return, string ID for rank
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
	}

	return "";
}

/**
 * Gets a graphic representation of this Soldier's military rank.
 * @note THE MEANING OF LIFE
 * @return, sprite ID for rank
 */
int Soldier::getRankSprite() const
{
	return 42 + _rank;
}

/**
 * Gets this Soldier's military rank.
 * @return, rank enum
 */
SoldierRank Soldier::getRank() const
{
	return _rank;
}

/**
 * Increases this Soldier's military rank.
 */
void Soldier::promoteRank()
{
	_rank = static_cast<SoldierRank>(static_cast<int>(_rank) + 1);

	if (_rank > RANK_SQUADDIE) // only promotions above SQUADDIE are worth mentioning.
		_recentlyPromoted = true;
}

/**
 * Adds kills and a mission to this Soldier's stats.
 * @param kills - qty of kills
 */
void Soldier::postTactical(int kills)
{
	++_missions;
	_kills += kills;
}

/**
 * Gets this Soldier's quantity of missions.
 * @return, missions
 */
int Soldier::getMissions() const
{
	return _missions;
}

/**
 * Gets this Soldier's quantity of kills.
 * @return, kills
 */
int Soldier::getKills() const
{
	return _kills;
}

/**
 * Gets this Soldier's gender.
 * @return, gender enum
 */
SoldierGender Soldier::getGender() const
{
	return _gender;
}

/**
 * Gets this Soldier's look.
 * @return, look enum
 */
SoldierLook Soldier::getLook() const
{
	return _look;
}

/**
 * Returns this Soldier's promotion status and resets it.
 * @return, true if recently promoted
 */
bool Soldier::isPromoted()
{
	const bool promoted = _recentlyPromoted;
	_recentlyPromoted = false;

	return promoted;
}

/**
 * Gets this Soldier's current armor.
 * @return, pointer to Armor rule
 */
RuleArmor* Soldier::getArmor() const
{
	return _armor;
}

/**
 * Sets this Soldier's current armor.
 * @param armor - pointer to Armor rule
 */
void Soldier::setArmor(RuleArmor* const armor)
{
	_armor = armor;
}

/**
 * Gets the amount of time until this Soldier is fully healed.
 * @return, quantity of days
 */
int Soldier::getRecovery() const
{
	return _recovery;
}

/**
 * Sets the amount of time until this Soldier is fully healed.
 * @param recovery - quantity of days
 */
void Soldier::setRecovery(int recovery)
{
	_recovery = recovery;

	if (_recovery > 0) // dismiss from craft
		_craft = NULL;
}

/**
 * Gets this Soldier's remaining woundage as a percent.
 * @return, wounds as percent
 */
int Soldier::getRecoveryPCT() const
{
	return static_cast<int>(std::floor(
		   static_cast<float>(_recovery) / static_cast<float>(_currentStats.health) * 100.f));
}

/**
 * Heals this Soldier's wounds a single day.
 */
void Soldier::heal()
{
	--_recovery;

	if (_recovery < 0)
		_recovery = 0;
}

/**
 * Gets the list of EquipmentLayoutItems of this Soldier.
 * @return, pointer to a vector of pointers to EquipmentLayoutItem
 */
std::vector<EquipmentLayoutItem*>* Soldier::getEquipmentLayout()
{
	return &_equipmentLayout;
}

/**
 * Trains this Soldier's psychic abilities after 1 month.
 * @note Called from GeoscapeState per 1month.
 * kL_note: I don't use this.
 */
/*
void Soldier::trainPsi()
{
	const int
		capPsiSkl = _rules->getStatCaps().psiSkill,
		capPsiStr = _rules->getStatCaps().psiStrength;

	_gainPsiSkl =
	_gainPsiStr = 0;

	// -10 days - tolerance threshold for switch from anytimePsiTraining option.
	// If soldier has psiskill -10..-1, he was trained 20..59 days. 81.7% probability, he was trained more that 30 days.
	if (_currentStats.psiSkill < -10 + _rules->getMinStats().psiSkill)
		_currentStats.psiSkill = _rules->getMinStats().psiSkill;
	else if (_currentStats.psiSkill <= _rules->getMaxStats().psiSkill)
		_gainPsiSkl = RNG::generate(
								_rules->getMaxStats().psiSkill,
								_rules->getMaxStats().psiSkill + _rules->getMaxStats().psiSkill / 2);
	else
	{
		if (_currentStats.psiSkill <= (capPsiSkl / 2))
			_gainPsiSkl = RNG::generate(5,12);
		else if (_currentStats.psiSkill < capPsiSkl)
			_gainPsiSkl = RNG::generate(1,3);

		if (Options::allowPsiStrengthImprovement == true)
		{
			if (_currentStats.psiStrength <= (capPsiStr / 2))
				_gainPsiStr = RNG::generate(5,12);
			else if (_currentStats.psiStrength < capPsiStr)
				_gainPsiStr = RNG::generate(1,3);
		}
	}

	_currentStats.psiSkill += _gainPsiSkl;
	if (_currentStats.psiSkill > capPsiSkl)
		_currentStats.psiSkill = capPsiSkl;

	_currentStats.psiStrength += _gainPsiStr;
	if (_currentStats.psiStrength > capPsiStr)
		_currentStats.psiStrength = capPsiStr;
} */
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

/**
 * Trains this Soldier's psychic abilities once per day.
 * @note Called from GeoscapeState::time1Day() if Options::anytimePsiTraining is enabled.
 * @return, true if Soldier's psi-stat(s) increased
 */
bool Soldier::trainPsiDay()
{
	bool ret = false;

	if (_psiTraining == true)
	{
		static const int PSI_PCT = 5; // % per day per soldier to become psionic-active

//		_gainPsiSkl = 0; // was used in AllocatePsiTrainingState.
//		if (_currentStats.psiSkill >= _rules->getStatCaps().psiSkill)	// hard cap. Note this auto-caps psiStrength also
//			return false;												// REMOVED: Allow psi to train past cap in PsiLabs.

		if (_currentStats.psiSkill == 0)
		{
			if (RNG::percent(PSI_PCT) == true)
			{
				ret = true;
				_currentStats.psiSkill =
				_initialStats.psiSkill = RNG::generate(
												_rules->getMinStats().psiSkill,
												_rules->getMaxStats().psiSkill);
			}
		}
		else // Psi unlocked already.
		{
			int pct = std::max(
							1,
							500 / _currentStats.psiSkill);
			if (RNG::percent(pct) == true)
			{
				ret = true;
				++_currentStats.psiSkill;
//				++_gainPsiSkl;
			}

			if (_currentStats.psiStrength < _rules->getStatCaps().psiStrength
				&& Options::allowPsiStrengthImprovement == true)
			{
				pct = std::max(
							1,
							500 / _currentStats.psiStrength);
				if (RNG::percent(pct) == true)
				{
					ret = true;
					++_currentStats.psiStrength;
//					++_gainPsiStr;
				}
			}
		}
	}

	return ret;
}

/**
 * Gets whether or not this Soldier is in psi training.
 * @return, true if training
 */
bool Soldier::isInPsiTraining() const
{
	return _psiTraining;
}

/**
 * Toggles whether or not this Soldier is in psi training.
 */
void Soldier::togglePsiTraining()
{
	_psiTraining = !_psiTraining;
}

/**
 * Gets this Soldier's psiSkill improvement during the current month.
 * @return, psi-skill improvement
 */
/* int Soldier::getImprovement()
{
	return _gainPsiSkl;
} */

/**
 * Gets this Soldier's psiStrength improvement during the current month.
 * @return, psi-strength improvement
 */
/* int Soldier::getPsiStrImprovement()
{
	return _gainPsiStr;
} */

/**
 * Returns this Soldier's death time.
 * @return, pointer to death data - NULL if no death has occurred
 */
/* SoldierDeath* Soldier::getDeath() const
{
	return _death;
} */

/**
 * Kills this Soldier in Debriefing or the Geoscape.
 * @param gameSave - pointer to the SavedGame
 */
void Soldier::die(SavedGame* const gameSave)
{
	SoldierDeath* const deathTime = new SoldierDeath();
	deathTime->setTime(*gameSave->getTime());

	SoldierDead* const deadSoldier = new SoldierDead(
												_name,
												_id,
												_rank,
												_gender,
												_look,
												_missions,
												_kills,
												deathTime,
												_initialStats,
												_currentStats,
												*_diary); // base if I want to...

	gameSave->getDeadSoldiers()->push_back(deadSoldier);
}

/**
 * Gets this Soldier's diary.
 * @return, pointer to SoldierDiary
 */
SoldierDiary* Soldier::getDiary() const
{
	return _diary;
}

/**
 * Calculates this Soldier's statString.
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

/**
 * Automatically renames this Soldier according to his/her current statistics.
 */
void Soldier::autoStat()
{
	std::wostringstream stat;

	switch (_rank)
	{
		case 0: stat << "r"; break;
		case 1: stat << "q"; break;
		case 2: stat << "t"; break;
		case 3: stat << "p"; break;
		case 4: stat << "n"; break;
		case 5: stat << "x"; break;

		default: stat << "z";
	}

	stat << _currentStats.firing << ".";
	stat << _currentStats.reactions << ".";
	stat << _currentStats.strength;

	switch (_currentStats.bravery)
	{
		case  10: stat << "a";	break;
		case  20: stat << "b";	break;
		case  30: stat << "c";	break;
		case  40: stat << "d";	break;
		case  50: stat << "e";	break;
		case  60: stat << "f";	break;
		case  70: stat << "g";	break;
		case  80: stat << "h";	break;
		case  90: stat << "i";	break;
		case 100: stat << "j";	break;

		default: stat << "z";
	}

	if (_currentStats.psiSkill >= _rules->getMinStats().psiSkill)
	{
		stat << (_currentStats.psiStrength + _currentStats.psiSkill / 5);

		if (_currentStats.psiSkill >= _rules->getStatCaps().psiSkill)
			stat << ":";
		else
			stat << ".";

		stat << (_currentStats.psiStrength * _currentStats.psiSkill / 100);
	}

	_name = stat.str();
}

}
