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

#include "SoldierDiary.h"

#include "GameTime.h"

#include "../Ruleset/RuleCommendations.h"
#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

/**
 * Initializes a new diary entry from YAML.
 * @param node - YAML node
 */
SoldierDiary::SoldierDiary(const YAML::Node& node)
{
	load(node);
}

/**
 * Initializes a new blank diary.
 */
SoldierDiary::SoldierDiary()
	:
//		_killList(),
//		_regionTotal(),
//		_countryTotal(),
//		_typeTotal(),
//		_UFOTotal(),
//		_missionIdList(),
//		_commendations(),
		_scoreTotal(0),
		_pointTotal(0),
		_killTotal(0),
		_missionTotal(0),
		_winTotal(0),
		_stunTotal(0),
		_baseDefenseMissionTotal(0),
		_daysWoundedTotal(0),
		_terrorMissionTotal(0),
		_nightMissionTotal(0),
		_nightTerrorMissionTotal(0),
		_monthsService(0),
		_unconsciousTotal(0),
		_shotAtCounterTotal(0),
		_hitCounterTotal(0),
		_loneSurvivorTotal(0),
		_totalShotByFriendlyCounter(0),
		_totalShotFriendlyCounter(0),
		_ironManTotal(0),
		_importantMissionTotal(0),
		_longDistanceHitCounterTotal(0),
		_lowAccuracyHitCounterTotal(0),
		_shotsFiredCounterTotal(0),
		_shotsLandedCounterTotal(0),
		_shotAtCounter10in1Mission(0),
		_hitCounter5in1Mission(0),
		_reactionFireTotal(0),
		_timesWoundedTotal(0),
		_valiantCruxTotal(0),
		_KIA(0),
		_trapKillTotal(0),
		_alienBaseAssaultTotal(0),
		_allAliensKilledTotal(0)
{
}

/**
 * dTor.
 */
SoldierDiary::~SoldierDiary()
{
	for (std::vector<SoldierCommendations*>::const_iterator
			i = _commendations.begin();
			i != _commendations.end();
			++i)
	{
		delete *i;
	}

	for (std::vector<BattleUnitKills*>::const_iterator
			i = _killList.begin();
			i != _killList.end();
			++i)
	{
		delete *i;
	}
}

/**
 * Loads the diary from a YAML file.
 * @param node - reference a YAML node
 */
void SoldierDiary::load(const YAML::Node& node)
{
	if (const YAML::Node& commendations = node["commendations"])
	{
		for (YAML::const_iterator
				i = commendations.begin();
				i != commendations.end();
				++i)
		{
			_commendations.push_back(new SoldierCommendations(*i));
		}
	}

	if (const YAML::Node& killList = node["killList"])
	{
		for (YAML::const_iterator
				i = killList.begin();
				i != killList.end();
				++i)
		{
			_killList.push_back(new BattleUnitKills(*i));
		}
	}

	_missionIdList					= node["missionIdList"]					.as<std::vector<int> >(_missionIdList);
	_regionTotal					= node["regionTotal"]					.as<std::map<std::string, int> >(_regionTotal);
	_countryTotal					= node["countryTotal"]					.as<std::map<std::string, int> >(_countryTotal);
	_typeTotal						= node["typeTotal"]						.as<std::map<std::string, int> >(_typeTotal);
	_UFOTotal						= node["UFOTotal"]						.as<std::map<std::string, int> >(_UFOTotal);
	_scoreTotal						= node["scoreTotal"]					.as<int>(_scoreTotal);
	_pointTotal						= node["pointTotal"]					.as<int>(_pointTotal);
	_killTotal						= node["killTotal"]						.as<int>(_killTotal);
	_missionTotal					= node["missionTotal"]					.as<int>(_missionTotal);
	_winTotal						= node["winTotal"]						.as<int>(_winTotal);
	_stunTotal						= node["stunTotal"]						.as<int>(_stunTotal);
	_daysWoundedTotal				= node["daysWoundedTotal"]				.as<int>(_daysWoundedTotal);
	_baseDefenseMissionTotal		= node["baseDefenseMissionTotal"]		.as<int>(_baseDefenseMissionTotal);
	_totalShotByFriendlyCounter		= node["totalShotByFriendlyCounter"]	.as<int>(_totalShotByFriendlyCounter);
	_totalShotFriendlyCounter		= node["totalShotFriendlyCounter"]		.as<int>(_totalShotFriendlyCounter);
	_loneSurvivorTotal				= node["loneSurvivorTotal"]				.as<int>(_loneSurvivorTotal);
	_terrorMissionTotal				= node["terrorMissionTotal"]			.as<int>(_terrorMissionTotal);
	_nightMissionTotal				= node["nightMissionTotal"]				.as<int>(_nightMissionTotal);
	_nightTerrorMissionTotal		= node["nightTerrorMissionTotal"]		.as<int>(_nightTerrorMissionTotal);
	_monthsService					= node["monthsService"]					.as<int>(_monthsService);
	_unconsciousTotal				= node["unconsciousTotal"]				.as<int>(_unconsciousTotal);
	_shotAtCounterTotal				= node["shotAtCounterTotal"]			.as<int>(_shotAtCounterTotal);
	_hitCounterTotal				= node["hitCounterTotal"]				.as<int>(_hitCounterTotal);
	_ironManTotal					= node["ironManTotal"]					.as<int>(_ironManTotal);
	_importantMissionTotal			= node["importantMissionTotal"]			.as<int>(_importantMissionTotal);
	_longDistanceHitCounterTotal	= node["longDistanceHitCounterTotal"]	.as<int>(_longDistanceHitCounterTotal);
	_lowAccuracyHitCounterTotal		= node["lowAccuracyHitCounterTotal"]	.as<int>(_lowAccuracyHitCounterTotal);
	_shotsFiredCounterTotal			= node["shotsFiredCounterTotal"]		.as<int>(_shotsFiredCounterTotal);
	_shotsLandedCounterTotal		= node["shotsLandedCounterTotal"]		.as<int>(_shotsLandedCounterTotal);
	_shotAtCounter10in1Mission		= node["shotAtCounter10in1Mission"]		.as<int>(_shotAtCounter10in1Mission);
	_hitCounter5in1Mission			= node["hitCounter5in1Mission"]			.as<int>(_hitCounter5in1Mission);
	_reactionFireTotal				= node["reactionFireTotal"]				.as<int>(_reactionFireTotal);
	_timesWoundedTotal				= node["timesWoundedTotal"]				.as<int>(_timesWoundedTotal);
	_valiantCruxTotal				= node["valiantCruxTotal"]				.as<int>(_valiantCruxTotal);
	_trapKillTotal					= node["trapKillTotal"]					.as<int>(_trapKillTotal);
	_alienBaseAssaultTotal			= node["alienBaseAssaultTotal"]			.as<int>(_alienBaseAssaultTotal);
	_allAliensKilledTotal			= node["allAliensKilledTotal"]			.as<int>(_allAliensKilledTotal);
}

/**
 * Saves the diary to a YAML file.
 * @return, YAML node
 */
YAML::Node SoldierDiary::save() const
{
	YAML::Node node;

	for (std::vector<SoldierCommendations*>::const_iterator
			i = _commendations.begin();
			i != _commendations.end();
			++i)
	{
		node["commendations"].push_back((*i)->save());
	}

	for (std::vector<BattleUnitKills*>::const_iterator
			i = _killList.begin();
			i != _killList.end();
			++i)
	{
		node["killList"].push_back((*i)->save());
	}

	if (_missionIdList.empty() == false)	node["missionIdList"]				= _missionIdList;
	if (_regionTotal.empty() == false)		node["regionTotal"]					= _regionTotal;
	if (_countryTotal.empty() == false)		node["countryTotal"]				= _countryTotal;
	if (_typeTotal.empty() == false)		node["typeTotal"]					= _typeTotal;
	if (_UFOTotal.empty() == false)			node["UFOTotal"]					= _UFOTotal;
	if (_scoreTotal)						node["scoreTotal"]					= _scoreTotal;
	if (_pointTotal)						node["pointTotal"]					= _pointTotal;
	if (_killTotal)							node["killTotal"]					= _killTotal;
	if (_missionTotal)						node["missionTotal"]				= _missionTotal;
	if (_winTotal)							node["winTotal"]					= _winTotal;
	if (_stunTotal)							node["stunTotal"]					= _stunTotal;
	if (_daysWoundedTotal)					node["daysWoundedTotal"]			= _daysWoundedTotal;
	if (_baseDefenseMissionTotal)			node["baseDefenseMissionTotal"]		= _baseDefenseMissionTotal;
	if (_totalShotByFriendlyCounter)		node["totalShotByFriendlyCounter"]	= _totalShotByFriendlyCounter;
	if (_totalShotFriendlyCounter)			node["totalShotFriendlyCounter"]	= _totalShotFriendlyCounter;
	if (_loneSurvivorTotal)					node["loneSurvivorTotal"]			= _loneSurvivorTotal;
	if (_terrorMissionTotal)				node["terrorMissionTotal"]			= _terrorMissionTotal;
	if (_nightMissionTotal)					node["nightMissionTotal"]			= _nightMissionTotal;
	if (_nightTerrorMissionTotal)			node["nightTerrorMissionTotal"]		= _nightTerrorMissionTotal;
	if (_monthsService)						node["monthsService"]				= _monthsService;
	if (_unconsciousTotal)					node["unconsciousTotal"]			= _unconsciousTotal;
	if (_shotAtCounterTotal)				node["shotAtCounterTotal"]			= _shotAtCounterTotal;
	if (_hitCounterTotal)					node["hitCounterTotal"]				= _hitCounterTotal;
	if (_ironManTotal)						node["ironManTotal"]				= _ironManTotal;
	if (_importantMissionTotal)				node["importantMissionTotal"]		= _importantMissionTotal;
	if (_longDistanceHitCounterTotal)		node["longDistanceHitCounterTotal"]	= _longDistanceHitCounterTotal;
	if (_lowAccuracyHitCounterTotal)		node["lowAccuracyHitCounterTotal"]	= _lowAccuracyHitCounterTotal;
	if (_shotsFiredCounterTotal)			node["shotsFiredCounterTotal"]		= _shotsFiredCounterTotal;
	if (_shotsLandedCounterTotal)			node["shotsLandedCounterTotal"]		= _shotsLandedCounterTotal;
	if (_shotAtCounter10in1Mission)			node["shotAtCounter10in1Mission"]	= _shotAtCounter10in1Mission;
	if (_hitCounter5in1Mission)				node["hitCounter5in1Mission"]		= _hitCounter5in1Mission;
	if (_reactionFireTotal)					node["reactionFireTotal"]			= _reactionFireTotal;
	if (_timesWoundedTotal)					node["timesWoundedTotal"]			= _timesWoundedTotal;
	if (_valiantCruxTotal)					node["valiantCruxTotal"]			= _valiantCruxTotal;
	if (_trapKillTotal)						node["trapKillTotal"]				= _trapKillTotal;
	if (_alienBaseAssaultTotal)				node["alienBaseAssaultTotal"]		= _alienBaseAssaultTotal;
	if (_allAliensKilledTotal)				node["allAliensKilledTotal"]		= _allAliensKilledTotal;

	return node;
}

/**
 * Updates this SoldierDiary's statistics.
 * @param unitStatistics	- pointer to BattleUnitStatistics to get stats from
 * @param missionStatistics	- pointer to MissionStatistics to get stats from
 * @param rules				- pointer to Ruleset
 */
void SoldierDiary::updateDiary(
		const BattleUnitStatistics* const unitStatistics,
		MissionStatistics* const missionStatistics,
		const Ruleset* const rules)
{
	const std::vector<BattleUnitKills*> unitKills = unitStatistics->kills;
	for (std::vector<BattleUnitKills*>::const_iterator
			i = unitKills.begin();
			i != unitKills.end();
			++i)
	{
		(*i)->makeTurnUnique();

		if ((*i)->getUnitStatusString() == "STATUS_DEAD")
			++_killTotal;
		else if ((*i)->getUnitStatusString() == "STATUS_UNCONSCIOUS")
			++_stunTotal;

		_killList.push_back(*i);

		if ((*i)->hostileTurn() == true)
		{
			if (rules->getItem((*i)->_weapon)->getBattleType() == BT_GRENADE
				|| rules->getItem((*i)->_weapon)->getBattleType() == BT_PROXIMITYGRENADE)
			{
				++_trapKillTotal;
			}
			else
				++_reactionFireTotal;
		}
	}

	++_regionTotal[missionStatistics->region.c_str()];
	++_countryTotal[missionStatistics->country.c_str()];
	++_typeTotal[missionStatistics->getMissionTypeLowerCase().c_str()];
	++_UFOTotal[missionStatistics->ufo.c_str()];
	_scoreTotal += missionStatistics->score;
	_pointTotal += missionStatistics->points;

	if (missionStatistics->success)
	{
		++_winTotal;

		if (missionStatistics->type != "STR_SMALL_SCOUT"
			&& missionStatistics->type != "STR_MEDIUM_SCOUT"
			&& missionStatistics->type != "STR_LARGE_SCOUT"
			&& missionStatistics->type != "STR_SUPPLY_SHIP")
		{
			++_importantMissionTotal;
		}

		if (missionStatistics->type == "STR_ALIEN_BASE_ASSAULT")
			++_alienBaseAssaultTotal;

		if (unitStatistics->loneSurvivor)
			++_loneSurvivorTotal;

		if (unitStatistics->ironMan)
			++_ironManTotal;
	}

	_daysWoundedTotal += unitStatistics->daysWounded;

	if (unitStatistics->daysWounded)
		++_timesWoundedTotal;

	if (missionStatistics->type == "STR_BASE_DEFENSE")
		++_baseDefenseMissionTotal;
	else if (missionStatistics->type == "STR_TERROR_MISSION")
	{
		++_terrorMissionTotal;

		if (missionStatistics->shade > 8)
			++_nightTerrorMissionTotal;
	}

	if (missionStatistics->shade > 8)
		++_nightMissionTotal;

	if (unitStatistics->wasUnconscious)
		++_unconsciousTotal;

	_shotAtCounterTotal += unitStatistics->shotAtCounter;
	_shotAtCounter10in1Mission += (unitStatistics->shotAtCounter) / 10;
	_hitCounterTotal += unitStatistics->hitCounter;
	_hitCounter5in1Mission += (unitStatistics->hitCounter) / 5;
	_totalShotByFriendlyCounter += unitStatistics->shotByFriendlyCounter;
	_totalShotFriendlyCounter += unitStatistics->shotFriendlyCounter;
	_longDistanceHitCounterTotal += unitStatistics->longDistanceHitCounter;
	_lowAccuracyHitCounterTotal += unitStatistics->lowAccuracyHitCounter;

	if (missionStatistics->valiantCrux)
		++_valiantCruxTotal;

	if (unitStatistics->KIA)
		++_KIA;

	if (unitStatistics->nikeCross)
		++_allAliensKilledTotal;

	_missionIdList.push_back(missionStatistics->id);

//	_missionTotal = _missionIdList.size(); // CAN GET RID OF MISSION TOTAL
}

/**
 * Get soldier commendations.
 * @return, pointer to a vector of pointers to SoldierCommendations - a list of soldier's commendations
 */
std::vector<SoldierCommendations*>* SoldierDiary::getSoldierCommendations()
{
	return &_commendations;
}

/**
 * Manage the soldier's commendations. Award new ones, if deserved.
 * @param rules - pointer to Ruleset
 * @return, true if a commendation has been awarded
 */
bool SoldierDiary::manageCommendations(const Ruleset* const rules)
{
	bool
		wasAwarded = false,	// This value is returned if at least one commendation was given.
		doAward = false;	// This value determines if a commendation will be given.

	std::map<std::string, int> nextComLevel;	// Noun, threshold.
	std::vector<std::string> modularComs;		// Commendation name.

	// Loop over all possible commendations.
	const std::map<std::string, RuleCommendations*> comsList = rules->getCommendation();
	for (std::map<std::string, RuleCommendations*>::const_iterator
			i = comsList.begin();
			i != comsList.end();
			)
	{
		modularComs.clear();
		nextComLevel.clear();

		nextComLevel["noNoun"] = 0;
		doAward = true;

		// Loop over all this SoldierCommendations, see if we already have
		// the commendation; if so, get the level and noun.
		for (std::vector<SoldierCommendations*>::const_iterator
				j = _commendations.begin();
				j != _commendations.end();
				++j)
		{
			if ((*i).first == (*j)->getType())
			{
				nextComLevel[(*j)->getNoun()] = (*j)->getDecorationLevelInt() + 1;
//				break; // hm.
			}
		}

		// Go through each possible criteria. Assume the medal is awarded, set to false if not.
		// As soon as we find a medal criteria that we FAIL TO achieve, then we are not awarded a medal.
		for (std::map<std::string, std::vector<int> >::const_iterator
				j = (*i).second->getCriteria()->begin();
				j != (*i).second->getCriteria()->end();
				++j)
		{
			// Skip this medal if we have reached its max award level.
			if (nextComLevel["noNoun"] >= static_cast<int>((*j).second.size()))
			{
				doAward = false;
				break;
			}
			// These criteria have no nouns, so only the nextComLevel["noNoun"] will ever be used.
			else if (nextComLevel.count("noNoun") == 1
				&& (((*j).first == "totalKills"
						&& static_cast<int>(_killList.size()) < (*j).second.at(nextComLevel["noNoun"]))
					|| ((*j).first == "totalMissions"
						&& static_cast<int>(_missionIdList.size()) < (*j).second.at(nextComLevel["noNoun"]))
					|| ((*j).first == "totalWins"
						&& _winTotal < (*j).second.at(nextComLevel["noNoun"]))
					|| ((*j).first == "totalScore"
						&& _scoreTotal < (*j).second.at(nextComLevel["noNoun"]))
					|| ((*j).first == "totalStuns"
						&& _stunTotal < (*j).second.at(nextComLevel["noNoun"]))
					|| ((*j).first == "totalDaysWounded"
						&& _daysWoundedTotal < (*j).second.at(nextComLevel["noNoun"]))
					|| ((*j).first == "totalBaseDefenseMissions"
						&& _baseDefenseMissionTotal < (*j).second.at(nextComLevel["noNoun"]))
					|| ((*j).first == "totalTerrorMissions"
						&& _terrorMissionTotal < (*j).second.at(nextComLevel["noNoun"]))
					|| ((*j).first == "totalNightMissions"
						&& _nightMissionTotal < (*j).second.at(nextComLevel["noNoun"]))
					|| ((*j).first == "totalNightTerrorMissions"
						&& _nightTerrorMissionTotal < (*j).second.at(nextComLevel["noNoun"]))
					|| ((*j).first == "totalMonthlyService"
						&& _monthsService < (*j).second.at(nextComLevel["noNoun"]))
					|| ((*j).first == "totalFellUnconscious"
						&& _unconsciousTotal < (*j).second.at(nextComLevel["noNoun"]))
					|| ((*j).first == "totalShotAt10Times"
						&& _shotAtCounter10in1Mission < (*j).second.at(nextComLevel["noNoun"]))
					|| ((*j).first == "totalHit5Times"
						&& _hitCounter5in1Mission < (*j).second.at(nextComLevel["noNoun"]))
					|| ((*j).first == "totalFriendlyFired"
						&& _totalShotByFriendlyCounter < (*j).second.at(nextComLevel["noNoun"]))
					|| ((*j).first == "total_lone_survivor"
						&& _loneSurvivorTotal < (*j).second.at(nextComLevel["noNoun"]))
					|| ((*j).first == "totalIronMan"
						&& _ironManTotal < (*j).second.at(nextComLevel["noNoun"]))
					|| ((*j).first == "totalImportantMissions"
						&& _importantMissionTotal < (*j).second.at(nextComLevel["noNoun"]))
					|| ((*j).first == "totalLongDistanceHits"
						&& _longDistanceHitCounterTotal < (*j).second.at(nextComLevel["noNoun"]))
					|| ((*j).first == "totalLowAccuracyHits"
						&& _lowAccuracyHitCounterTotal < (*j).second.at(nextComLevel["noNoun"]))
					|| ((*j).first == "totalReactionFire"
						&& _reactionFireTotal < (*j).second.at(nextComLevel["noNoun"]))
					|| ((*j).first == "totalTimesWounded"
						&& _timesWoundedTotal < (*j).second.at(nextComLevel["noNoun"]))
					|| ((*j).first == "totalDaysWounded"
						&& _daysWoundedTotal < (*j).second.at(nextComLevel["noNoun"]))
					|| ((*j).first == "totalValientCrux"
						&& _valiantCruxTotal < (*j).second.at(nextComLevel["noNoun"]))
					|| ((*j).first == "isDead"
						&& _KIA < (*j).second.at(nextComLevel["noNoun"])))
					|| ((*j).first == "totalTrapKills"
						&& _trapKillTotal < (*j).second.at(nextComLevel["noNoun"]))
					|| ((*j).first == "totalAlienBaseAssaults"
						&& _alienBaseAssaultTotal < (*j).second.at(nextComLevel["noNoun"]))
					|| ((*j).first == "totalAllAliensKilled"
						&& _allAliensKilledTotal < (*j).second.at(nextComLevel["noNoun"])))
			{
				doAward = false;
				break;
			}
			// Medals with the following criteria are unique because they need a noun.
			// And because they loop over a map<> (this allows for maximum moddability).
			else if ((*j).first == "totalKillsWithAWeapon"
				|| (*j).first == "totalMissionsInARegion"
				|| (*j).first == "totalKillsByRace"
				|| (*j).first == "totalKillsByRank")
			{
				std::map<std::string, int> tempTotal;

				if ((*j).first == "totalKillsWithAWeapon")
					tempTotal = getWeaponTotal();
				else if ((*j).first == "totalMissionsInARegion")
					tempTotal = _regionTotal;
				else if ((*j).first == "totalKillsByRace")
					tempTotal = getAlienRaceTotal();
				else if ((*j).first == "totalKillsByRank")
					tempTotal = getAlienRankTotal();

				// Loop over the temporary map; match nouns and decoration levels.
				for(std::map<std::string, int>::const_iterator
						k = tempTotal.begin();
						k != tempTotal.end();
						++k)
				{
					int criteria = -1;
					const std::string noun = (*k).first;

					// If there is no matching noun, get the first award criteria.
					if (nextComLevel.count(noun) == 0)
						criteria = (*j).second.front();
					// Otherwise, get the criteria that reflects this SoldierCommendation level.
					else if (nextComLevel[noun] != (*j).second.size())
						criteria = (*j).second.at(nextComLevel[noun]);

					// If a criteria was set AND the stat's count exceeds the criteria.
					if (criteria != -1
						&& (*k).second >= criteria)
					{
						modularComs.push_back(noun);
					}
				}

				// If it is still empty, we did not get a commendation.
				if (modularComs.empty() == true)
				{
					doAward = false;
					break;
				}
				else
					doAward = true;
			}
			else if ((*j).first == "killsWithCriteriaCareer"
				|| (*j).first == "killsWithCriteriaMission"
				|| (*j).first == "killsWithCriteriaTurn")
			{
				// Fetch the kill criteria list.
				const std::vector<std::map<int, std::vector<std::string> > >* killCriteriaList = (*i).second->getKillCriteria();
				// Loop over the OR vectors.
				for (std::vector<std::map<int, std::vector<std::string> > >::const_iterator
						orCriteria = killCriteriaList->begin();
						orCriteria != killCriteriaList->end();
						++orCriteria)
				{
					// Loop over the AND vectors.
					for (std::map<int, std::vector<std::string> >::const_iterator
							andCriteria = orCriteria->begin();
							andCriteria != orCriteria->end();
							++andCriteria)
					{
						int qty = 0; // How many AND vectors (list of DETAILs) have been successful.
						if ((*j).first == "killsWithCriteriaTurn"
							|| (*j).first == "killsWithCriteriaMission")
						{
							++qty; // Turns and missions start at 1 because of how thisTime and lastTime work.
						}

						bool goToNextTime = false;
						int // Time being a turn or a mission.
							thisTime = -1,
							lastTime = -1;

						// Loop over the KILLS.
						for (std::vector<BattleUnitKills*>::const_iterator
								singleKill = _killList.begin();
								singleKill != _killList.end();
								++singleKill)
						{
							if ((*j).first == "killsWithCriteriaMission")
							{
								thisTime = (*singleKill)->_mission;
								if (singleKill != _killList.begin())
								{
									--singleKill;
									lastTime = (*singleKill)->_mission;
									++singleKill;
								}
							}
							else if ((*j).first == "killsWithCriteriaTurn")
							{
								thisTime = (*singleKill)->_turn;
								if (singleKill != _killList.begin())
								{
									--singleKill;
									lastTime = (*singleKill)->_turn;
									++singleKill;
								}
							}

							// Skip kill-groups that we already got an award for; skip kills that are inbetween turns.
							if (thisTime == lastTime
								&& goToNextTime == true
								&& (*j).first != "killsWithCriteriaCareer")
							{
								continue;
							}
							else if (thisTime != lastTime)
							{
								qty = 1; // Reset.
								goToNextTime = false;

								continue;
							}

							bool found = true;

							// Loop over the DETAILs of the AND vector.
							for (std::vector<std::string>::const_iterator
									detail = andCriteria->second.begin();
									detail != andCriteria->second.end();
									++detail)
							{
								const std::string bType_array[] =
								{
									"BT_NONE",
									"BT_FIREARM",
									"BT_AMMO",
									"BT_MELEE",
									"BT_GRENADE",
									"BT_PROXIMITYGRENADE",
									"BT_MEDIKIT",
									"BT_SCANNER",
									"BT_MINDPROBE",
									"BT_PSIAMP",
									"BT_FLARE",
									"BT_CORPSE",
									"BT_END"
								};

								const std::vector<std::string> bType_vect (bType_array, bType_array + 13); // init.
								int bType = 0;
								for (
										;
										bType != static_cast<int>(bType_vect.size());
										++bType)
								{
									if (*detail == bType_vect[bType])
										break;
								}

								const std::string dType_array[] =
								{
									"DT_NONE",
									"DT_AP",
									"DT_IN",
									"DT_HE",
									"DT_LASER",
									"DT_PLASMA",
									"DT_STUN",
									"DT_MELEE",
									"DT_ACID",
									"DT_SMOKE",
									"DT_END"
								};

								const std::vector<std::string> dType_vect (dType_array, dType_array + 11); // init.
								int dType = 0;
								for (
										;
										dType != static_cast<int>(dType_vect.size());
										++dType)
								{
									if (*detail == dType_vect[dType])
										break;
								}

								// See if we find no matches with any criteria; if so, break and try the next kill.
								if ((*singleKill)->_weapon == "STR_WEAPON_UNKNOWN"
									|| (*singleKill)->_weaponAmmo == "STR_WEAPON_UNKNOWN"
									|| ((*singleKill)->_rank != *detail
										&& (*singleKill)->_race != *detail
										&& (*singleKill)->_weapon != *detail
										&& (*singleKill)->_weaponAmmo != *detail
										&& (*singleKill)->getUnitStatusString() != *detail
										&& (*singleKill)->getUnitFactionString() != *detail
										&& rules->getItem((*singleKill)->_weaponAmmo)->getDamageType() != dType
										&& rules->getItem((*singleKill)->_weapon)->getBattleType() != bType))
								{
									found = false;
									break;
								}
							}

							if (found == true)
							{
								++qty;
								if (qty == (*andCriteria).first)
									goToNextTime = true; // Criteria met, move to next mission/turn.
							}
						}

						// If one of the AND criteria fail, stop looking.
						const int multiCriteria = (*andCriteria).first;
						if (multiCriteria == 0
							|| qty / multiCriteria < (*j).second.at(nextComLevel["noNoun"]))
						{
							doAward = false;
							break;
						}
						else
							doAward = true;
					}

					if (doAward == true)
						break; // Stop looking because we are getting one regardless.
				}

				if (doAward == true)
					break; // Stop looking because we are getting one regardless.
			}
		}

//		bool wasAwardedMod = false;

		if (doAward == true)
		{
			// If we do not have modular medals, but are awarded
			// a different medal, its noun will be "noNoun".
			if (modularComs.empty() == true)
				modularComs.push_back("noNoun");
//			else
//				wasAwardedMod = true;

			for (std::vector<std::string>::const_iterator
					j = modularComs.begin();
					j != modularComs.end();
					++j)
			{
				bool newCom = true;

				for (std::vector<SoldierCommendations*>::const_iterator
						k = _commendations.begin();
						k != _commendations.end();
						++k)
				{
					if ((*k)->getType() == (*i).first
						&& (*k)->getNoun() == *j)
					{
						newCom = false;
						(*k)->addDecoration();

						break;
					}
				}

				if (newCom == true)
					_commendations.push_back(new SoldierCommendations(
																	(*i).first,
																	*j));
			}

			wasAwarded = true;
		}
		else
			++i;
	}

	return wasAwarded;
}

/**
 * Manage modular commendations (private)
 * @param nextCommendationLevel	- refrence map<string, int>
 * @param modularCommendations	- reference map<string, int>
 * @param statTotal				- pair<string, int>
 * @param criteria				- int
 */
/* void SoldierDiary::manageModularCommendations(
		std::map<std::string, int>& nextCommendationLevel,
		std::map<std::string, int>& modularCommendations,
		std::pair<std::string, int> statTotal,
		int criteria)
{
	// If criteria is 0, we don't have this noun OR if we meet
	// the criteria, remember the noun for award purposes.
	if ((modularCommendations.count(statTotal.first) == 0
			&& statTotal.second >= criteria)
		|| (modularCommendations.count(statTotal.first) != 0
			&& nextCommendationLevel.at(statTotal.first) >= criteria))
	{
		modularCommendations[statTotal.first]++;
	}
} */

/**
 * Award commendations to the soldier.
 * @param type - string
 * @param noun - string (default "noNoun")
 */
void SoldierDiary::awardCommendation(
		const std::string type,
		const std::string noun)
{
	bool newCom = true;

	for (std::vector<SoldierCommendations*>::iterator
			i = _commendations.begin();
			i != _commendations.end();
			++i)
	{
		if ((*i)->getType() == type
			&& (*i)->getNoun() == noun)
		{
			(*i)->addDecoration();
			newCom = false;

			break;
		}
	}

	if (newCom == true)
		_commendations.push_back(new SoldierCommendations(
														type,
														noun));
}

/**
 * Get vector of mission ids.
 * @return, reference a vector of mission IDs
 */
std::vector<int>& SoldierDiary::getMissionIdList()
{
	return _missionIdList;
}

/**
 * Get vector of kills.
 * @return, reference a vector of BattleUnitKills
 */
std::vector<BattleUnitKills*>& SoldierDiary::getKills()
{
	return _killList;
}

/**
 * Get list of kills sorted by rank
 * @return,
 */
std::map<std::string, int> SoldierDiary::getAlienRankTotal() const
{
	std::map<std::string, int> ret;

	for(std::vector<BattleUnitKills*>::const_iterator
			i = _killList.begin();
			i != _killList.end();
			++i)
	{
		++ret[(*i)->_rank.c_str()];
	}

	return ret;
}

/**
 *
 */
std::map<std::string, int> SoldierDiary::getAlienRaceTotal() const
{
	std::map<std::string, int> ret;

	for(std::vector<BattleUnitKills*>::const_iterator
			i = _killList.begin();
			i != _killList.end();
			++i)
	{
		++ret[(*i)->_race.c_str()];
	}

	return ret;
}
/**
 *
 */
std::map<std::string, int> SoldierDiary::getWeaponTotal() const
{
	std::map<std::string, int> ret;

	for(std::vector<BattleUnitKills*>::const_iterator
			i = _killList.begin();
			i != _killList.end();
			++i)
	{
		++ret[(*i)->_weapon.c_str()];
	}

	return ret;
}

/**
 *
 */
std::map<std::string, int> SoldierDiary::getWeaponAmmoTotal() const
{
	std::map<std::string, int> ret;

	for(std::vector<BattleUnitKills*>::const_iterator
			i = _killList.begin();
			i != _killList.end();
			++i)
	{
		++ret[(*i)->_weaponAmmo.c_str()];
	}

	return ret;
}

/**
 *
 */
std::map<std::string, int>& SoldierDiary::getRegionTotal()
{
	return _regionTotal;
}

/**
 *
 */
std::map<std::string, int>& SoldierDiary::getCountryTotal()
{
	return _countryTotal;
}

/**
 *
 */
std::map<std::string, int>& SoldierDiary::getTypeTotal()
{
	return _typeTotal;
}

/**
 *
 */
std::map<std::string, int>& SoldierDiary::getUFOTotal()
{
	return _UFOTotal;
}

/**
 *
 */
int SoldierDiary::getScoreTotal() const
{
	return _scoreTotal;
}

/**
 * Gets the total point-value of aLiens killed or stunned.
 */
int SoldierDiary::getScorePoints() const
{
	return _pointTotal;
}

/**
 *
 */
int SoldierDiary::getKillTotal() const
{
	return _killTotal;
}

/**
 *
 */
int SoldierDiary::getMissionTotal() const
{
	return _missionIdList.size();
}

/**
 *
 */
int SoldierDiary::getWinTotal() const
{
	return _winTotal;
}

/**
 *
 */
int SoldierDiary::getStunTotal() const
{
	return _stunTotal;
}

/**
 *
 */
int SoldierDiary::getDaysWoundedTotal() const
{
	return _daysWoundedTotal;
}

/**
 * Increments soldier's service time one month.
 */
void SoldierDiary::addMonthlyService()
{
	++_monthsService;
}


/*____________________________________
/* ** SOLDIER COMMENDATIONS class ** */
/**
 * Initializes a new SoldierCommendations entry from YAML.
 * @param node - YAML node
 */
SoldierCommendations::SoldierCommendations(const YAML::Node& node)
{
	load(node);
}

/**
 * Initializes a SoldierCommendations.
 * @param commendationName	-
 * @param noun				-
 */
SoldierCommendations::SoldierCommendations(
		std::string commendationName,
		std::string noun)
	:
		_type(commendationName),
		_noun(noun),
		_decorationLevel(0),
		_isNew(true)
{
}

/**
 * dTor.
 */
SoldierCommendations::~SoldierCommendations()
{
}

/**
 * Loads the SoldierCommendations from a YAML file.
 * @param node - reference a YAML node
 */
void SoldierCommendations::load(const YAML::Node& node)
{
	_type				= node["commendationName"]	.as<std::string>(_type);
	_noun				= node["noun"]				.as<std::string>("noNoun");
	_decorationLevel	= node["decorationLevel"]	.as<int>(_decorationLevel);
	_isNew				= node["isNew"]				.as<bool>(false);
}

/**
 * Saves this SoldierCommendations to a YAML file.
 * @return, YAML node
 */
YAML::Node SoldierCommendations::save() const
{
	YAML::Node node;

	node["commendationName"]	= _type;
	if (_noun != "noNoun")
		node["noun"]			= _noun;
	node["decorationLevel"]		= _decorationLevel;

	return node;
}

/**
 * Gets this SoldierCommendations' name.
 * @return, commendation name
 */
std::string SoldierCommendations::getType() const
{
	return _type;
}

/**
 * Get this SoldierCommendations' noun.
 * @return, commendation noun
 */
std::string SoldierCommendations::getNoun() const
{
	return _noun;
}

/**
 * Gets this SoldierCommendations' level's name.
 * @param skip -
 * @return, commendation level
 */
std::string SoldierCommendations::getDecorationLevelName(const int skip) const
{
	std::ostringstream ss;
	ss << "STR_AWARD_" << _decorationLevel - skip;

	return ss.str();
}

/**
 * Gets this SoldierCommendations' level's description.
 * @return, commendation level description
 */
std::string SoldierCommendations::getDecorationDescription() const
{
	std::ostringstream ss;
	ss << "STR_AWARD_DECOR_" << _decorationLevel;

	return ss.str();
}

/**
 * kL. Gets this SoldierCommendations' level's class.
 * @return, commendation level class
 */
std::string SoldierCommendations::getDecorationClass() const // kL
{
	std::ostringstream ss;
	ss << "STR_AWARD_CLASS_" << _decorationLevel;

	return ss.str();
}

/**
 * Gets this SoldierCommendations' level's int.
 * @return, commendation level int
 */
int SoldierCommendations::getDecorationLevelInt() const
{
	return _decorationLevel;
}

/**
 * Gets newness of this SoldierCommendations.
 * @return, true if the commendation is new
 */
bool SoldierCommendations::isNew() const
{
	return _isNew;
}

/**
 * Sets the newness of this SoldierCommendations to old.
 */
void SoldierCommendations::makeOld()
{
	_isNew = false;
}

/**
 * Adds a level of decoration to this SoldierCommendations.
 * Sets isNew to true.
 */
void SoldierCommendations::addDecoration()
{
	++_decorationLevel;
	_isNew = true;
}

}
