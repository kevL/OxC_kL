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

#ifndef OPENXCOM_SOLDIERDIARY_H
#define OPENXCOM_SOLDIERDIARY_H

//#include <yaml-cpp/yaml.h>

#include "BattleUnit.h"
#include "SavedGame.h"

#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

class Ruleset;


/**
 * Each entry will be its own commendation.
 */
class SoldierCommendations
{

private:
	bool _isNew;
	size_t _decorLevel;
	std::string
		_type,
		_noun;


	public:
		/// Creates a commendation of the specified type.
		SoldierCommendations(
				const std::string type,
				const std::string noun = "noNoun");
		/// Creates a new commendation and loads its contents from YAML.
		SoldierCommendations(const YAML::Node& node);
		/// Cleans up the commendation.
		~SoldierCommendations();

		/// Loads the commendation information from YAML.
		void load(const YAML::Node& node);
		/// Saves the commendation information to YAML.
		YAML::Node save() const;

		/// Gets commendation name.
		const std::string getType() const;
		/// Gets commendation noun.
		const std::string getNoun() const;
		/// Gets the commendation's decoration level's name.
		const std::string getDecorLevelType(const int skip) const;
		/// Gets the commendation's decoration description.
		const std::string getDecorDesc() const;
		/// Gets the commendation's decoration class.
		const std::string getDecorClass() const;
		/// Gets the commendation's decoration level's int.
		size_t getDecorLevelInt() const;

		/// Gets the newness of the commendation.
		bool isNew() const;
		/// Sets the commendation newness to false.
		void makeOld();

		/// Increment decoration level. Sets _isNew to true.
		void addDecoration();
};


class SoldierDiary
{

private:
	int
		_scoreTotal,
		_pointTotal,
		_killTotal,
		_missionTotal,
		_winTotal,
		_stunTotal,
		_daysWoundedTotal,
		_baseDefenseMissionTotal,
		_totalShotByFriendlyCounter,
		_totalShotFriendlyCounter,
		_loneSurvivorTotal,
		_terrorMissionTotal,
		_nightMissionTotal,
		_nightTerrorMissionTotal,
		_monthsService,
		_unconsciousTotal,
		_shotAtCounterTotal,
		_hitCounterTotal,
		_ironManTotal,
		_importantMissionTotal,
		_longDistanceHitCounterTotal,
		_lowAccuracyHitCounterTotal,
		_shotsFiredCounterTotal,
		_shotsLandedCounterTotal,
		_shotAtCounter10in1Mission,
		_hitCounter5in1Mission,
		_reactionFireTotal,
		_timesWoundedTotal,
		_valiantCruxTotal,
		_KIA,
		_trapKillTotal,
		_alienBaseAssaultTotal,
		_allAliensKilledTotal,
		_mediApplicationsTotal,
		_revivedUnitTotal,
		_MIA;

	std::vector<int> _missionIdList;
	std::vector<SoldierCommendations*> _awards;
	std::vector<BattleUnitKills*> _killList;

	std::map<std::string, int>
		_regionTotal,
		_countryTotal,
		_typeTotal,
		_UFOTotal;

	///
/*	void manageModularCommendations(
			std::map<std::string, int>& nextCommendationLevel,
			std::map<std::string, int>& modularCommendations,
			std::pair<std::string, int> statTotal,
			int criteria); */
	///
	void awardCommendation(
			const std::string type,
			const std::string noun = "noNoun");


	public:
		/// Creates a new soldier-diary and loads its contents from YAML.
		SoldierDiary(const YAML::Node& node);
		/// Constructs a diary.
		SoldierDiary();
		/// Constructs a copy of a diary.
		SoldierDiary(const SoldierDiary& copyThis);
		/// Deconstructs a diary.
		~SoldierDiary();

		/// Overloads assignment operator.
		SoldierDiary& operator= (const SoldierDiary& assignThis);

		/// Load a diary.
		void load(const YAML::Node& node);
		/// Save a diary.
		YAML::Node save() const;

		/// Update the diary statistics.
		void updateDiary(
				const BattleUnitStatistics* const unitStatistics,
				MissionStatistics* const missionStatistics,
				const Ruleset* const rules);

		/// Gets the list of kills, mapped by rank.
		std::map<std::string, int> getAlienRankTotal() const;
		/// Gets the list of kills, mapped by race.
		std::map<std::string, int> getAlienRaceTotal() const;
		/// Gets the list of kills, mapped by weapon used.
		std::map<std::string, int> getWeaponTotal() const;
		/// Gets the list of kills, mapped by weapon ammo used.
		std::map<std::string, int> getWeaponAmmoTotal() const;
		/// Gets the list of missions, mapped by region.
		std::map<std::string, int>& getRegionTotal();
		/// Gets the list of missions, mapped by country.
		std::map<std::string, int>& getCountryTotal();
		/// Gets the list of missions, mapped by type.
		std::map<std::string, int>& getTypeTotal();
		/// Gets the list of missions, mapped by UFO.
		std::map<std::string, int>& getUFOTotal();

		/// Gets the total score.
		int getScoreTotal() const;
		/// Gets the total point-value of aLiens killed or stunned.
		int getScorePoints() const;
		/// Gets the total number of kills.
		int getKillTotal() const;
		/// Gets the total number of stuns.
		int getStunTotal() const;
		/// Gets the total number of missions.
		int getMissionTotal() const;
		/// Gets the total number of wins.
		int getWinTotal() const;
		/// Gets the total number of days wounded.
		int getDaysWoundedTotal() const;
		/// Gets whether soldier died or went missing.
		std::string getKiaOrMia() const;

		/// Gets the solder's commendations.
		std::vector<SoldierCommendations*>* getSoldierCommendations();

		/// Manage commendations, return true if a medal is awarded.
		bool manageAwards(const Ruleset* const rules);

		/// Increment the soldier's service time.
		void addMonthlyService();

		/// Gets the mission id list.
		std::vector<int>& getMissionIdList();

		/// Gets the kill list.
		std::vector<BattleUnitKills*>& getKills();
};

}

#endif
