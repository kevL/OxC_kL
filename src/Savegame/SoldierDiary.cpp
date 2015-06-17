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
		_scoreTotal(0),
		_pointTotal(0),
		_killTotal(0),
		_stunTotal(0),
		_missionTotal(0),
		_winTotal(0),
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
		_allAliensKilledTotal(0),
		_mediApplicationsTotal(0),
		_revivedUnitTotal(0),
		_MIA(0)
{}

/**
 * Constructs a copy of a SoldierDiary.
 * @param copyThis - reference the diary to copy to this SoldierDiary
 */
SoldierDiary::SoldierDiary(const SoldierDiary& copyThis)
	:
		_scoreTotal(copyThis._scoreTotal),
		_pointTotal(copyThis._pointTotal),
		_killTotal(copyThis._killTotal),
		_stunTotal(copyThis._stunTotal),
		_missionTotal(copyThis._missionTotal),
		_winTotal(copyThis._winTotal),
		_daysWoundedTotal(copyThis._daysWoundedTotal),
		_baseDefenseMissionTotal(copyThis._baseDefenseMissionTotal),
		_totalShotByFriendlyCounter(copyThis._totalShotByFriendlyCounter),
		_totalShotFriendlyCounter(copyThis._totalShotFriendlyCounter),
		_loneSurvivorTotal(copyThis._loneSurvivorTotal),
		_terrorMissionTotal(copyThis._terrorMissionTotal),
		_nightMissionTotal(copyThis._nightMissionTotal),
		_nightTerrorMissionTotal(copyThis._nightTerrorMissionTotal),
		_monthsService(copyThis._monthsService),
		_unconsciousTotal(copyThis._unconsciousTotal),
		_shotAtCounterTotal(copyThis._shotAtCounterTotal),
		_hitCounterTotal(copyThis._hitCounterTotal),
		_ironManTotal(copyThis._ironManTotal),
		_importantMissionTotal(copyThis._importantMissionTotal),
		_longDistanceHitCounterTotal(copyThis._longDistanceHitCounterTotal),
		_lowAccuracyHitCounterTotal(copyThis._lowAccuracyHitCounterTotal),
		_shotsFiredCounterTotal(copyThis._shotsFiredCounterTotal),
		_shotsLandedCounterTotal(copyThis._shotsLandedCounterTotal),
		_shotAtCounter10in1Mission(copyThis._shotAtCounter10in1Mission),
		_hitCounter5in1Mission(copyThis._hitCounter5in1Mission),
		_reactionFireTotal(copyThis._reactionFireTotal),
		_timesWoundedTotal(copyThis._timesWoundedTotal),
		_valiantCruxTotal(copyThis._valiantCruxTotal),
		_KIA(copyThis._KIA),
		_trapKillTotal(copyThis._trapKillTotal),
		_alienBaseAssaultTotal(copyThis._alienBaseAssaultTotal),
		_allAliensKilledTotal(copyThis._allAliensKilledTotal),
		_mediApplicationsTotal(copyThis._mediApplicationsTotal),
		_revivedUnitTotal(copyThis._revivedUnitTotal),
		_MIA(copyThis._MIA)
{
	for (size_t
			i = 0;
			i != copyThis._missionIdList.size();
			++i)
	{
		if (copyThis._missionIdList.at(i) != NULL)
			_missionIdList.push_back(copyThis._missionIdList.at(i));
	}

	std::map<std::string, int>::const_iterator i;
	for (
			i = copyThis._regionTotal.begin();
			i != copyThis._regionTotal.end();
			++i)
	{
		_regionTotal[(*i).first] = (*i).second;
	}

	for (
			i = copyThis._countryTotal.begin();
			i != copyThis._countryTotal.end();
			++i)
	{
		_countryTotal[(*i).first] = (*i).second;
	}

	for (
			i = copyThis._typeTotal.begin();
			i != copyThis._typeTotal.end();
			++i)
	{
		_typeTotal[(*i).first] = (*i).second;
	}

	for (
			i = copyThis._UFOTotal.begin();
			i != copyThis._UFOTotal.end();
			++i)
	{
		_UFOTotal[(*i).first] = (*i).second;
	}


	for (size_t
			i = 0;
			i != copyThis._awards.size();
			++i)
	{
		if (copyThis._awards.at(i) != NULL)
		{
			std::string
				type = copyThis._awards.at(i)->getType(),
				noun = copyThis._awards.at(i)->getNoun();

			_awards.push_back(new SoldierCommendations(
													type,
													noun));
		}
	}

	for (size_t
			i = 0;
			i != copyThis._killList.size();
			++i)
	{
		if (copyThis._killList.at(i) != NULL)
		{
			std::string
				unitRank = copyThis._killList.at(i)->_rank,
				race = copyThis._killList.at(i)->_race,
				weapon = copyThis._killList.at(i)->_weapon,
				weaponAmmo = copyThis._killList.at(i)->_weaponAmmo;
			int
				mission = copyThis._killList.at(i)->_mission,
				turn = copyThis._killList.at(i)->_turn,
				points = copyThis._killList.at(i)->_points;

			UnitFaction faction = copyThis._killList.at(i)->_faction;
			UnitStatus status = copyThis._killList.at(i)->_status;

			_killList.push_back(new BattleUnitKills(
												unitRank,
												race,
												weapon,
												weaponAmmo,
												faction,
												status,
												mission,
												turn,
												points));
		}
	}
}

/**
 * dTor.
 */
SoldierDiary::~SoldierDiary()
{
	for (std::vector<SoldierCommendations*>::const_iterator
			i = _awards.begin();
			i != _awards.end();
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
 * Overloads the assignment operator.
 * @param assignThis - reference the diary to assign to this SoldierDiary
 * @return, address of the new diary
 */
SoldierDiary& SoldierDiary::operator=(const SoldierDiary& assignThis)
{
	if (this != &assignThis)
	{
		_scoreTotal = assignThis._scoreTotal;
		_pointTotal = assignThis._pointTotal;
		_killTotal = assignThis._killTotal;
		_stunTotal = assignThis._stunTotal;
		_missionTotal = assignThis._missionTotal;
		_winTotal = assignThis._winTotal;
		_daysWoundedTotal = assignThis._daysWoundedTotal;
		_baseDefenseMissionTotal = assignThis._baseDefenseMissionTotal;
		_totalShotByFriendlyCounter = assignThis._totalShotByFriendlyCounter;
		_totalShotFriendlyCounter = assignThis._totalShotFriendlyCounter;
		_loneSurvivorTotal = assignThis._loneSurvivorTotal;
		_terrorMissionTotal = assignThis._terrorMissionTotal;
		_nightMissionTotal = assignThis._nightMissionTotal;
		_nightTerrorMissionTotal = assignThis._nightTerrorMissionTotal;
		_monthsService = assignThis._monthsService;
		_unconsciousTotal = assignThis._unconsciousTotal;
		_shotAtCounterTotal = assignThis._shotAtCounterTotal;
		_hitCounterTotal = assignThis._hitCounterTotal;
		_ironManTotal = assignThis._ironManTotal;
		_importantMissionTotal = assignThis._importantMissionTotal;
		_longDistanceHitCounterTotal = assignThis._longDistanceHitCounterTotal;
		_lowAccuracyHitCounterTotal = assignThis._lowAccuracyHitCounterTotal;
		_shotsFiredCounterTotal = assignThis._shotsFiredCounterTotal;
		_shotsLandedCounterTotal = assignThis._shotsLandedCounterTotal;
		_shotAtCounter10in1Mission = assignThis._shotAtCounter10in1Mission;
		_hitCounter5in1Mission = assignThis._hitCounter5in1Mission;
		_reactionFireTotal = assignThis._reactionFireTotal;
		_timesWoundedTotal = assignThis._timesWoundedTotal;
		_valiantCruxTotal = assignThis._valiantCruxTotal;
		_KIA = assignThis._KIA;
		_trapKillTotal = assignThis._trapKillTotal;
		_alienBaseAssaultTotal = assignThis._alienBaseAssaultTotal;
		_allAliensKilledTotal = assignThis._allAliensKilledTotal;
		_mediApplicationsTotal = assignThis._mediApplicationsTotal;
		_revivedUnitTotal = assignThis._revivedUnitTotal;
		_MIA = assignThis._MIA;

		_missionIdList.clear();
		for (std::vector<int>::const_iterator
				i = assignThis._missionIdList.begin();
				i != assignThis._missionIdList.end();
				++i)
		{
			_missionIdList.push_back(*i);
		}

		_regionTotal.clear();
		std::map<std::string, int>::const_iterator i;
		for (
				i = assignThis._regionTotal.begin();
				i != assignThis._regionTotal.end();
				++i)
		{
			_regionTotal[(*i).first] = (*i).second;
		}

		_countryTotal.clear();
		for (
				i = assignThis._countryTotal.begin();
				i != assignThis._countryTotal.end();
				++i)
		{
			_countryTotal[(*i).first] = (*i).second;
		}

		_typeTotal.clear();
		for (
				i = assignThis._typeTotal.begin();
				i != assignThis._typeTotal.end();
				++i)
		{
			_typeTotal[(*i).first] = (*i).second;
		}

		_UFOTotal.clear();
		for (
				i = assignThis._UFOTotal.begin();
				i != assignThis._UFOTotal.end();
				++i)
		{
			_UFOTotal[(*i).first] = (*i).second;
		}


		for (std::vector<SoldierCommendations*>::const_iterator
				i = _awards.begin();
				i != _awards.end();
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

		for (size_t
				i = 0;
				i != assignThis._awards.size();
				++i)
		{
			if (assignThis._awards.at(i) != NULL)
			{
				std::string
					type = assignThis._awards.at(i)->getType(),
					noun = assignThis._awards.at(i)->getNoun();

				_awards.push_back(new SoldierCommendations(
														type,
														noun));
			}
		}

		for (size_t
				i = 0;
				i != assignThis._killList.size();
				++i)
		{
			if (assignThis._killList.at(i) != NULL)
			{
				std::string
					unitRank = assignThis._killList.at(i)->_rank,
					race = assignThis._killList.at(i)->_race,
					weapon = assignThis._killList.at(i)->_weapon,
					weaponAmmo = assignThis._killList.at(i)->_weaponAmmo;
				int
					mission = assignThis._killList.at(i)->_mission,
					turn = assignThis._killList.at(i)->_turn,
					points = assignThis._killList.at(i)->_points;

				UnitFaction faction = assignThis._killList.at(i)->_faction;
				UnitStatus status = assignThis._killList.at(i)->_status;

				_killList.push_back(new BattleUnitKills(
													unitRank,
													race,
													weapon,
													weaponAmmo,
													faction,
													status,
													mission,
													turn,
													points));
			}
		}
	}

	return *this;
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
			_awards.push_back(new SoldierCommendations(*i));
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
	_stunTotal						= node["stunTotal"]						.as<int>(_stunTotal);
	_missionTotal					= node["missionTotal"]					.as<int>(_missionTotal);
	_winTotal						= node["winTotal"]						.as<int>(_winTotal);
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
	_mediApplicationsTotal			= node["mediApplicationsTotal"]			.as<int>(_mediApplicationsTotal);
	_revivedUnitTotal				= node["revivedUnitTotal"]				.as<int>(_revivedUnitTotal);

	_KIA =
	_MIA = 0;
}

/**
 * Saves the diary to a YAML file.
 * @return, YAML node
 */
YAML::Node SoldierDiary::save() const
{
	YAML::Node node;

	for (std::vector<SoldierCommendations*>::const_iterator
			i = _awards.begin();
			i != _awards.end();
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
	if (_stunTotal)							node["stunTotal"]					= _stunTotal;
	if (_missionTotal)						node["missionTotal"]				= _missionTotal;
	if (_winTotal)							node["winTotal"]					= _winTotal;
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
	if (_mediApplicationsTotal)				node["mediApplicationsTotal"]		= _mediApplicationsTotal;
	if (_revivedUnitTotal)					node["revivedUnitTotal"]			= _revivedUnitTotal;

	return node;
}

/**
 * Updates this SoldierDiary's statistics.
 * @note BattleUnitKills is a substruct of BattleUnitStatistics.
 * @param unitStatistics	- pointer to BattleUnitStatistics to get stats from (BattleUnit.h)
 * @param missionStatistics	- pointer to MissionStatistics to get stats from (SavedGame.h)
 * @param rules				- pointer to Ruleset
 */
void SoldierDiary::updateDiary(
		const BattleUnitStatistics* const unitStatistics,
		MissionStatistics* const missionStatistics,
		const Ruleset* const rules)
{
	//Log(LOG_INFO) << "SoldierDiary::updateDiary()";
	const std::vector<BattleUnitKills*> unitKills = unitStatistics->kills;
	for (std::vector<BattleUnitKills*>::const_iterator
			i = unitKills.begin();
			i != unitKills.end();
			++i)
	{
		_killList.push_back(*i);

		(*i)->makeTurnUnique();

		_pointTotal += (*i)->_points; // kL - if hostile unit was MC'd this should be halved

		if ((*i)->getUnitFactionString() == "FACTION_HOSTILE")
		{
			if ((*i)->getUnitStatusString() == "STATUS_DEAD")
				++_killTotal;
			else if ((*i)->getUnitStatusString() == "STATUS_UNCONSCIOUS")
				++_stunTotal;

			if ((*i)->hostileTurn() == true)
			{
				if (rules->getItem((*i)->_weapon)->getBattleType() == BT_GRENADE
					|| rules->getItem((*i)->_weapon)->getBattleType() == BT_PROXYGRENADE)
				{
					++_trapKillTotal;
				}
				else
					++_reactionFireTotal;
			}
		}
	}

	++_regionTotal[missionStatistics->region.c_str()];
	++_countryTotal[missionStatistics->country.c_str()];
	++_typeTotal[missionStatistics->getMissionTypeLowerCase().c_str()];
	++_UFOTotal[missionStatistics->ufo.c_str()];
	_scoreTotal += missionStatistics->score;

	if (missionStatistics->success == true)
	{
		++_winTotal;

		if (   missionStatistics->type != "STR_SMALL_SCOUT"
			&& missionStatistics->type != "STR_MEDIUM_SCOUT"
			&& missionStatistics->type != "STR_LARGE_SCOUT"
			&& missionStatistics->type != "STR_SUPPLY_SHIP")
		{
			++_importantMissionTotal;
		}

		if (missionStatistics->type == "STR_ALIEN_BASE_ASSAULT")
			++_alienBaseAssaultTotal;

		if (unitStatistics->loneSurvivor == true)
			++_loneSurvivorTotal;

		if (unitStatistics->ironMan == true)
			++_ironManTotal;
	}

	if (unitStatistics->daysWounded > 0)
	{
		_daysWoundedTotal += unitStatistics->daysWounded;
		++_timesWoundedTotal;
	}

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

	if (unitStatistics->wasUnconscious == true)
		++_unconsciousTotal;

	_shotAtCounterTotal += unitStatistics->shotAtCounter;
	_shotAtCounter10in1Mission += (unitStatistics->shotAtCounter) / 10;
	_hitCounterTotal += unitStatistics->hitCounter;
	_hitCounter5in1Mission += (unitStatistics->hitCounter) / 5;
	_totalShotByFriendlyCounter += unitStatistics->shotByFriendlyCounter;
	_totalShotFriendlyCounter += unitStatistics->shotFriendlyCounter;
	_longDistanceHitCounterTotal += unitStatistics->longDistanceHitCounter;
	_lowAccuracyHitCounterTotal += unitStatistics->lowAccuracyHitCounter;
	_mediApplicationsTotal += unitStatistics->medikitApplications;
	_revivedUnitTotal += unitStatistics->revivedSoldier;

	if (missionStatistics->valiantCrux == true)
		++_valiantCruxTotal;

	if (unitStatistics->KIA == true)
		++_KIA;

	if (unitStatistics->MIA == true)
		++_MIA;

	if (unitStatistics->nikeCross == true)
		++_allAliensKilledTotal;

	_missionIdList.push_back(missionStatistics->id);

//	_missionTotal = _missionIdList.size(); // CAN GET RID OF MISSION TOTAL
	//Log(LOG_INFO) << "SoldierDiary::updateDiary() EXIT";
}

/**
 * Get soldier commendations.
 * @return, pointer to a vector of pointers to SoldierCommendations - a list of soldier's commendations
 */
std::vector<SoldierCommendations*>* SoldierDiary::getSoldierCommendations()
{
	return &_awards;
}

/**
 * Manage the soldier's commendations - award new ones if earned.
 * @param rules - pointer to Ruleset
 * @return, true if an award has been awarded
 */
bool SoldierDiary::manageAwards(const Ruleset* const rules)
{
	//Log(LOG_INFO) << "sd:manageAwards()";
	bool
		doCeremony = false,	// this value is returned TRUE if at least one award is given
		doAward;			// this value determines if an award will be given

	std::map<std::string, size_t> reqdLevel;	// <noun, qtyLevels>
	std::vector<std::string> modularAwards;		// <types>

	// loop over all possible RuleCommendations
	const std::map<std::string, RuleCommendations*> awardList = rules->getCommendations();
	for (std::map<std::string, RuleCommendations*>::const_iterator
			i = awardList.begin();
			i != awardList.end();
			)
	{
		//Log(LOG_INFO) << ". iter awardList";
		const std::string& type = (*i).first;

		reqdLevel.clear();
		reqdLevel["noNoun"] = 0;

		// loop over all of soldier's SoldierCommendations, see if he/she
		// already has the award; get the level and noun if so
		for (std::vector<SoldierCommendations*>::const_iterator
				j = _awards.begin();
				j != _awards.end();
				++j)
		{
			if ((*j)->getType() == type)
				reqdLevel[(*j)->getNoun()] = (*j)->getDecorLevelInt() + 1;
		}

		// go through each possible criteria. Assume the award is awarded, set to false if not;
		// as soon as an award criteria that *fails to be achieved* is found, then NO award
		modularAwards.clear();

		doAward = true;
		int val;

		const std::map<std::string, std::vector<int> >* critList = (*i).second->getCriteria();
		for (std::map<std::string, std::vector<int> >::const_iterator
				j = critList->begin();
				j != critList->end();
				++j)
		{
			//Log(LOG_INFO) << ". . iter Criteria";
			const std::string& criterion = (*j).first;

			// skip this 'noNoun' award if its max award level has been reached
			// or if it has a noun skip it if it has 0 total levels (which ain't gonna happen);
			// you see, Rules can't be positively examined for nouns - only awards already given to soldiers can.
			if ((*j).second.size() <= reqdLevel["noNoun"])
			{
				doAward = false;
				break;
			}


			// these criteria have no nouns, so only the reqdLevel["noNoun"] will ever be compared
			// kL_NOTE: All these class variables ought be 'size_t'!!!!
			val = (*j).second.at(reqdLevel["noNoun"]);
			if (//reqdLevel.count("noNoun") == 1 && // <- this is relevant only if entry "noNoun" were removed from the map in the sections following this one.
//				reqdLevel["noNoun"] != 0 && // kL_add
//				reqdLevel["noNoun"] == 0 || // kL_add
				(   (   criterion == "totalKills"				&& static_cast<int>(_killList.size()) < val)
					|| (criterion == "totalMissions"			&& static_cast<int>(_missionIdList.size()) < val)
					|| (criterion == "totalWins"				&& _winTotal < val)
					|| (criterion == "totalScore"				&& _scoreTotal < val)
					|| (criterion == "totalPoints"				&& _pointTotal < val)
					|| (criterion == "totalStuns"				&& _stunTotal < val)
					|| (criterion == "totalBaseDefenseMissions"	&& _baseDefenseMissionTotal < val)
					|| (criterion == "totalTerrorMissions"		&& _terrorMissionTotal < val)
					|| (criterion == "totalNightMissions"		&& _nightMissionTotal < val)
					|| (criterion == "totalNightTerrorMissions"	&& _nightTerrorMissionTotal < val)
					|| (criterion == "totalMonthlyService"		&& _monthsService < val)
					|| (criterion == "totalFellUnconscious"		&& _unconsciousTotal < val)
					|| (criterion == "totalShotAt10Times"		&& _shotAtCounter10in1Mission < val)
					|| (criterion == "totalHit5Times"			&& _hitCounter5in1Mission < val)
					|| (criterion == "totalFriendlyFired"		&& (_totalShotByFriendlyCounter < val
																	|| _KIA != 0 || _MIA != 0)) // didn't survive ......
					|| (criterion == "totalLoneSurvivor"		&& _loneSurvivorTotal < val)
					|| (criterion == "totalIronMan"				&& _ironManTotal < val)
					|| (criterion == "totalImportantMissions"	&& _importantMissionTotal < val)
					|| (criterion == "totalLongDistanceHits"	&& _longDistanceHitCounterTotal < val)
					|| (criterion == "totalLowAccuracyHits"		&& _lowAccuracyHitCounterTotal < val)
					|| (criterion == "totalReactionFire"		&& _reactionFireTotal < val)
					|| (criterion == "totalTimesWounded"		&& _timesWoundedTotal < val)
					|| (criterion == "totalDaysWounded"			&& _daysWoundedTotal < val)
					|| (criterion == "totalValientCrux"			&& _valiantCruxTotal < val)
					|| (criterion == "isDead"					&& _KIA < val)
					|| (criterion == "totalTrapKills"			&& _trapKillTotal < val)
					|| (criterion == "totalAlienBaseAssaults"	&& _alienBaseAssaultTotal < val)
					|| (criterion == "totalAllAliensKilled"		&& _allAliensKilledTotal < val)
					|| (criterion == "totalMediApplications"	&& _mediApplicationsTotal < val)
					|| (criterion == "totalRevives"				&& _revivedUnitTotal < val)
					|| (criterion == "isMIA"					&& _MIA < val)))
			{
				doAward = false;
				break;
			}
else
			// awards with the following criteria are unique because they need a noun
			// and they loop over a map<> (this allows for "maximum" mod-ability)
			if (   criterion == "totalKillsWithAWeapon"
				|| criterion == "totalMissionsInARegion"
				|| criterion == "totalKillsByRace"
				|| criterion == "totalKillsByRank")
			{
				std::map<std::string, int> total;
				if (	 criterion == "totalKillsWithAWeapon")
					total = getWeaponTotal();
				else if (criterion == "totalMissionsInARegion")
					total = _regionTotal;
				else if (criterion == "totalKillsByRace")
					total = getAlienRaceTotal();
				else if (criterion == "totalKillsByRank")
					total = getAlienRankTotal();

				// loop over the 'total' map, match nouns and decoration levels
				for (std::map<std::string, int>::const_iterator
						k = total.begin();
						k != total.end();
						++k)
				{
					int threshold = -1;
					const std::string noun = (*k).first;

					// if there is no matching noun get the first award criteria
					if (reqdLevel.count(noun) == 0)
						threshold = (*j).second.front();
					// otherwise get the criteria per the SoldierCommendation level
					else if (reqdLevel[noun] != (*j).second.size())
						threshold = (*j).second.at(reqdLevel[noun]);

					// if a criteria was set AND the stat's count exceeds the criteria
					if (threshold != -1
						&& threshold <= (*k).second)
					{
						modularAwards.push_back(noun);
					}
				}

				// if 'modularAwards' is still empty soldier did not get an award
				if (modularAwards.empty() == true)
				{
					doAward = false;
					break;
				}
//kL			else doAward = true;
			}
else
			if (   criterion == "killsWithCriteriaCareer"
				|| criterion == "killsWithCriteriaMission"
				|| criterion == "killsWithCriteriaTurn")
			{
				// fetch the kill criteria list
				const std::vector<std::map<int, std::vector<std::string> > >* killCriteriaList = (*i).second->getKillCriteria();

				for (std::vector<std::map<int, std::vector<std::string> > >::const_iterator // loop over the OR vectors
						orCriteria = killCriteriaList->begin();
						orCriteria != killCriteriaList->end();
						++orCriteria)
				{
					for (std::map<int, std::vector<std::string> >::const_iterator // loop over the AND vectors
							andCriteria = orCriteria->begin();
							andCriteria != orCriteria->end();
							++andCriteria)
					{
						int qty = 0; // how many AND vectors (list of DETAILs) have been successful
						if (//criterion == "killsWithCriteriaTurn" || criterion == "killsWithCriteriaMission"
							criterion != "killsWithCriteriaCareer")
						{
							++qty; // turns and missions start at 1 because of how thisTime and lastTime work
						}

						bool goToNextTime = false;
						int // time, being a turn or a mission
							thisTime = -1,
							lastTime = -1;

						for (std::vector<BattleUnitKills*>::const_iterator // loop over the KILLS
								singleKill = _killList.begin();
								singleKill != _killList.end();
								++singleKill)
						{
							if (criterion == "killsWithCriteriaMission")
							{
								thisTime = (*singleKill)->_mission;
								if (singleKill != _killList.begin())
								{
									--singleKill;
									lastTime = (*singleKill)->_mission;
									++singleKill;
								}
							}
							else if (criterion == "killsWithCriteriaTurn")
							{
								thisTime = (*singleKill)->_turn;
								if (singleKill != _killList.begin())
								{
									--singleKill;
									lastTime = (*singleKill)->_turn;
									++singleKill;
								}
							}

							// skip kill-groups that soldier already got an award for
							// and skip kills that are inbetween turns
							if (thisTime == lastTime
								&& goToNextTime == true
								&& criterion != "killsWithCriteriaCareer")
							{
								continue;
							}
							else if (thisTime != lastTime)
							{
								qty = 1; // reset.
								goToNextTime = false;

								continue;
							}

							bool found = true;

							// loop over the DETAILs of the AND vector
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
									"BT_PROXYGRENADE",
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

								// see if there are NO matches with any criteria; break and try the next Criteria if so
								if (   (*singleKill)->_weapon == "STR_WEAPON_UNKNOWN"
									|| (*singleKill)->_weaponAmmo == "STR_WEAPON_UNKNOWN"
									|| (   (*singleKill)->_rank != *detail
										&& (*singleKill)->_race != *detail
										&& (*singleKill)->_weapon != *detail
										&& (*singleKill)->_weaponAmmo != *detail
										&& (*singleKill)->getUnitStatusString() != *detail
										&& (*singleKill)->getUnitFactionString() != *detail
										&& rules->getItem((*singleKill)->_weaponAmmo)->getDamageType() != dType
										&& rules->getItem((*singleKill)->_weapon)->getBattleType() != bType))
										// kL_note: *singleKill's _points value might want in there somehow ...
								{
									found = false;
									break;
								}
							}

							if (found == true)
							{
								++qty;
								if (qty == (*andCriteria).first)
									goToNextTime = true; // criteria met, move to next mission/turn
							}
						}

						// if one of the AND criteria fail, stop looking
						const int multiCriteria = (*andCriteria).first;
						if (multiCriteria == 0
							|| qty / multiCriteria < (*j).second.at(reqdLevel["noNoun"]))
						{
							doAward = false;
							break;
						}
						else
							doAward = true;
					}

					if (doAward == true)
						break; // stop looking because soldier is getting one regardless
				}
			}
		}


		if (doAward == true)
		{
			doCeremony = true;

			// if there are NO modular awards but *are* awarded a different
			// award its noun will be "noNoun"
			if (modularAwards.empty() == true)
				modularAwards.push_back("noNoun");

			for (std::vector<std::string>::const_iterator
					j = modularAwards.begin();
					j != modularAwards.end();
					++j)
			{
				bool newAward = true;
				for (std::vector<SoldierCommendations*>::const_iterator
						k = _awards.begin();
						k != _awards.end();
						++k)
				{
					if ((*k)->getType() == type
						&& (*k)->getNoun() == *j)
					{
						newAward = false;
						(*k)->addDecoration();

						break;
					}
				}

				if (newAward == true)
					_awards.push_back(new SoldierCommendations(
															type,
															*j));
			}
		}
		else
			++i;
	}

	//Log(LOG_INFO) << "sd:manageAwards() EXIT";
	return doCeremony;
}

/**
 * Manages modular commendations. (private)
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
 * Awards commendations to the soldier.
 * @param type - reference the type
 * @param noun - reference the noun (default "noNoun")
 */
/* void SoldierDiary::awardCommendation(
		const std::string& type,
		const std::string& noun)
{
	bool newAward = true;

	for (std::vector<SoldierCommendations*>::const_iterator
			i = _awards.begin();
			i != _awards.end();
			++i)
	{
		if ((*i)->getType() == type
			&& (*i)->getNoun() == noun)
		{
			(*i)->addDecoration();
			newAward = false;

			break;
		}
	}

	if (newAward == true)
		_awards.push_back(new SoldierCommendations(
												type,
												noun));
} */

/**
 * Gets a vector of mission ids.
 * @return, address of a vector of mission IDs
 */
std::vector<int>& SoldierDiary::getMissionIdList()
{
	return _missionIdList;
}

/**
 * Gets a vector of all kills in this SoldierDiary.
 * @return, address of a vector of pointers to BattleUnitKills
 */
std::vector<BattleUnitKills*>& SoldierDiary::getKills()
{
	return _killList;
}

/**
 * Gets list of kills by rank.
 * @return, map of alien ranks to qty killed
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
 * Gets list of kills by race.
 * @return, map of alien races to qty killed
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
 * Gets list of kills by weapon.
 * @return, map of weapons to qty killed with
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
 * Gets list of kills by ammo.
 * @return, map of ammos to qty killed with
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
 * Gets a list of quantity of missions done in a region.
 * @return, address of a map of regions to missions done there
 */
std::map<std::string, int>& SoldierDiary::getRegionTotal()
{
	return _regionTotal;
}

/**
 * Gets a list of quantity of missions done in a country.
 * @return, address of a map of countries to missions done there
 */
std::map<std::string, int>& SoldierDiary::getCountryTotal()
{
	return _countryTotal;
}

/**
 * Gets a list of quantity of missions done of a mission-type.
 * @return, address of a map of mission types to qty
 */
std::map<std::string, int>& SoldierDiary::getTypeTotal()
{
	return _typeTotal;
}

/**
 * Gets a list of quantity of missions done of a UFO-type
 * @return, address of a map of UFO types to qty
 */
std::map<std::string, int>& SoldierDiary::getUFOTotal()
{
	return _UFOTotal;
}

/**
 * Gets a total score for all missions.
 * @return, sum score of all missions engaged
 */
int SoldierDiary::getScoreTotal() const
{
	return _scoreTotal;
}

/**
 * Gets the total point-value of aLiens killed or stunned.
 * @return, sum points for all aliens killed or stunned
 */
int SoldierDiary::getScorePoints() const
{
	return _pointTotal;
}

/**
 * Gets the total quantity of kills.
 * @return, qty of kills
 */
int SoldierDiary::getKillTotal() const
{
	return _killTotal;
}

/**
 * Gets the total quantity of stuns.
 * @return, qty of stuns
 */
int SoldierDiary::getStunTotal() const
{
	return _stunTotal;
}

/**
 * Gets the total quantity of missions.
 * @return, qty of missions
 */
int SoldierDiary::getMissionTotal() const
{
	return static_cast<int>(_missionIdList.size());
}

/**
 * Gets the quantity of successful missions.
 * @return, qty of successful missions
 */
int SoldierDiary::getWinTotal() const
{
	return _winTotal;
}

/**
 * Gets the total quantity of days wounded.
 * @return, qty of days in sickbay
 */
int SoldierDiary::getDaysWoundedTotal() const
{
	return _daysWoundedTotal;
}

/**
 * Gets whether soldier died or went missing.
 * @return, kia or mia - or an empty string if neither
 */
std::string SoldierDiary::getKiaOrMia() const
{
	if (_KIA != 0)
		return "STR_KIA";

	if (_MIA != 0)
		return "STR_MIA";

	return "";
}

/**
 * Increments soldier's service time by one month.
 */
void SoldierDiary::addMonthlyService()
{
	++_monthsService;
}

/**
 * Award special commendation to the original 8 soldiers.
 */
void SoldierDiary::awardOriginalEight()
{
	_awards.push_back(new SoldierCommendations(
										"STR_MEDAL_ORIGINAL8_NAME",
										"noNoun"));
}


/*___________________________________/*
/*
/* ** SOLDIER COMMENDATIONS class ***
/*___________________________________*/
/**
 * Initializes a SoldierCommendations.
 * @param type - reference the type
 * @param noun - reference the noun (default "noNoun")
 */
SoldierCommendations::SoldierCommendations(
		const std::string& type,
		const std::string& noun)
	:
		_type(type),
		_noun(noun),
		_decorLevel(0),
		_isNew(true)
{}

/**
 * Initializes a new SoldierCommendations entry from YAML.
 * @param node - YAML node
 */
SoldierCommendations::SoldierCommendations(const YAML::Node& node)
{
	load(node);
}

/**
 * dTor.
 */
SoldierCommendations::~SoldierCommendations()
{}

/**
 * Loads the SoldierCommendations from a YAML file.
 * @param node - reference a YAML node
 */
void SoldierCommendations::load(const YAML::Node& node)
{
	_type		= node["type"]	.as<std::string>(_type);
	_noun		= node["noun"]	.as<std::string>("noNoun");
	_isNew		= node["isNew"]	.as<bool>(false);
	_decorLevel	= static_cast<size_t>(node["decorLevel"].as<int>(_decorLevel));
}

/**
 * Saves this SoldierCommendations to a YAML file.
 * @return, YAML node
 */
YAML::Node SoldierCommendations::save() const
{
	YAML::Node node;

	node["type"]		= _type;
	node["decorLevel"]	= static_cast<int>(_decorLevel);

	if (_noun != "noNoun")
		node["noun"] = _noun;

	return node;
}

/**
 * Gets this SoldierCommendations' name.
 * @return, commendation name
 */
const std::string SoldierCommendations::getType() const
{
	return _type;
}

/**
 * Get this SoldierCommendations' noun.
 * @return, commendation noun
 */
const std::string SoldierCommendations::getNoun() const
{
	return _noun;
}

/**
 * Gets this SoldierCommendations' level's name.
 * @param skip -
 * @return, commendation level
 */
const std::string SoldierCommendations::getDecorLevelType(const int skip) const
{
	std::ostringstream oststr;
	oststr << "STR_AWARD_" << _decorLevel - skip;

	return oststr.str();
}

/**
 * Gets this SoldierCommendations' level's description.
 * @return, commendation level description
 */
const std::string SoldierCommendations::getDecorDesc() const
{
	std::ostringstream oststr;
	oststr << "STR_AWARD_DECOR_" << _decorLevel;

	return oststr.str();
}

/**
 * Gets this SoldierCommendations' level's class - stars.
 * @return, commendation level class
 */
const std::string SoldierCommendations::getDecorClass() const
{
	std::ostringstream oststr;
	oststr << "STR_AWARD_CLASS_" << _decorLevel;

	return oststr.str();
}

/**
 * Gets this SoldierCommendations' level's int.
 * @return, commendation level int
 */
size_t SoldierCommendations::getDecorLevelInt() const
{
	return _decorLevel;
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
 */
void SoldierCommendations::addDecoration()
{
	++_decorLevel;
	_isNew = true;
}

}
