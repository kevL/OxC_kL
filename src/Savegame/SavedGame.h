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

#ifndef OPENXCOM_SAVEDGAME_H
#define OPENXCOM_SAVEDGAME_H

#include <map>
#include <string>
#include <vector>

#include <time.h>

#include "GameTime.h"


namespace OpenXcom
{

class AlienBase;
class AlienMission;
class AlienStrategy;
class Base;
class Country;
class GameTime;
class Language;
class Region;
class ResearchProject;
class RuleManufacture;
class RuleResearch;
class Ruleset;
class SavedBattleGame;
class Soldier;
class SoldierDead; // kL
class Target;
class TerrorSite;
class TextList;
class Ufo;
class Waypoint;


/**
 * Enumerator containing all the possible game difficulties.
 */
enum GameDifficulty
{
	DIFF_BEGINNER,		// 0
	DIFF_EXPERIENCED,	// 1
	DIFF_VETERAN,		// 2
	DIFF_GENIUS,		// 3
	DIFF_SUPERHUMAN		// 4
};

/**
 * Enumerator for the various save types.
 */
enum SaveType
{
	SAVE_DEFAULT,			// 0
	SAVE_QUICK,				// 1
	SAVE_AUTO_GEOSCAPE,		// 2
	SAVE_AUTO_BATTLESCAPE,	// 3
	SAVE_IRONMAN,			// 4
	SAVE_IRONMAN_END		// 5
};


/**
 * Container for mission statistics.
 */
struct MissionStatistics
{
	bool
		success,
		valiantCrux;

	int
		daylight,
		id,
		score;

	std::string
		alienRace,
		country,
		rating,
		region,
		type,
		ufo;

	std::map<int, int> injuryList;

	GameTime time;


	///
	std::string getMissionTypeLowerCase()
	{
		if		(type == "STR_UFO_CRASH_RECOVERY")	return "STR_UFO_CRASH_RECOVERY_LC";
		else if (type == "STR_UFO_GROUND_ASSAULT")	return "STR_UFO_GROUND_ASSAULT_LC";
		else if (type == "STR_BASE_DEFENSE")		return "STR_BASE_DEFENSE_LC";
		else if (type == "STR_ALIEN_BASE_ASSAULT")	return "STR_ALIEN_BASE_ASSAULT_LC";
		else if (type == "STR_TERROR_MISSION")		return "STR_TERROR_MISSION_LC";
		else										return "type error";
	}

	///
	void load(const YAML::Node& node)
	{
		time.load(node["time"]);

		id			= node["id"].as<int>(id);
		region		= node["region"].as<std::string>(region);
		country		= node["country"].as<std::string>(country);
		type		= node["type"].as<std::string>(type);
		ufo			= node["ufo"].as<std::string>(ufo);
		success		= node["success"].as<bool>(success);
		score		= node["score"].as<int>(score);
		rating		= node["rating"].as<std::string>(rating);
		alienRace	= node["alienRace"].as<std::string>(alienRace);
		daylight	= node["daylight"].as<int>(daylight);
		injuryList	= node["injuryList"].as<std::map<int, int> >(injuryList);
		valiantCrux	= node["valiantCrux"].as<bool>(valiantCrux);
	}

	///
	YAML::Node save() const
	{
		YAML::Node node;

		node["id"]			= id;
		node["time"]		= time.save();
		node["region"]		= region;
		node["country"]		= country;
		node["type"]		= type;
		node["ufo"]			= ufo;
		node["success"]		= success;
		node["score"]		= score;
		node["rating"]		= rating;
		node["alienRace"]	= alienRace;
		node["daylight"]	= daylight;
		node["injuryList"]	= injuryList;

		if (valiantCrux)
			 node["valiantCrux"] = valiantCrux;

		return node;
	}

	/// cTor.
	MissionStatistics(const YAML::Node& node)
		:
			time(0,0,0,0,0,0,0)
	{
		load(node);
	}

	/// cTor.
	MissionStatistics()
		:
			id(0),
			time(0,0,0,0,0,0,0),
			region("STR_REGION_UNKNOWN"),
			country("STR_UNKNOWN"),
			type(),
			ufo("NO_UFO"),
			success(false),
			score(0),
			rating(),
			alienRace("STR_UNKNOWN"),
			daylight(0),
			injuryList(),
			valiantCrux(false)
	{
	}

	/// dTor.
	~MissionStatistics()
	{
	}
};


/**
 * Container for savegame info displayed on listings.
 */
struct SaveInfo
{
	bool reserved;
	time_t timestamp;
	std::string fileName;
	std::wstring
		details,
		displayName,
		isoDate,
		isoTime;
	std::vector<std::string> rulesets;
};


/**
 * The game data that gets written to disk when the game is saved.
 * A saved game holds all the variable info in a game like funds,
 * game time, current bases and contents, world activities, score, etc.
 */
class SavedGame
{

private:
	bool
		_debug,
		_ironman,
		_warned;
	int
		_monthsPassed;
//kL	_globeZoom,
	size_t
		_globeZoom; // kL
//kL	_selectedBase;
	double
		_globeLat,
		_globeLon;

	GameDifficulty _difficulty;

	AlienStrategy* _alienStrategy;
	GameTime* _time;
	SavedBattleGame* _battleGame;

	std::wstring _name;
	std::string
		_graphRegionToggles,
		_graphCountryToggles,
		_graphFinanceToggles;

	std::map<std::string, int> _ids;

	std::vector<int>
		_expenditure, // kL
		_funds,
		_income, // kL
		_maintenance,
		_researchScores;

	std::vector<AlienBase*>				_alienBases;
	std::vector<AlienMission*>			_activeMissions;
	std::vector<Base*>					_bases;
	std::vector<Country*>				_countries;
	std::vector<MissionStatistics*>		_missionStatistics;
	std::vector<Region*>				_regions;
	std::vector<const RuleResearch*>
										_discovered,
										_poppedResearch;
	std::vector<Soldier*>				_soldiers; // kL
	std::vector<SoldierDead*>			_deadSoldiers; // kL
	std::vector<TerrorSite*>			_terrorSites;
	std::vector<Ufo*>					_ufos;
	std::vector<Waypoint*>				_waypoints;

	///
	void getDependableResearchBasic(
			std::vector<RuleResearch*>& dependables,
			const RuleResearch* research,
			const Ruleset* ruleset,
			Base* base) const;
	///
	static SaveInfo getSaveInfo(
			const std::string& file,
			Language* lang);

	public:
		static const std::string
			AUTOSAVE_GEOSCAPE,
			AUTOSAVE_BATTLESCAPE,
			QUICKSAVE;

		/// Creates a new saved game.
		SavedGame();
		/// Cleans up the saved game.
		~SavedGame();

		/// Gets list of saves in the user directory.
		static std::vector<SaveInfo> getList(
				Language* lang,
				bool autoquick);

		/// Loads a saved game from YAML.
		void load(
				const std::string& filename,
				Ruleset* rule);
		/// Saves a saved game to YAML.
		void save(const std::string& filename) const;

		/// Gets the game name.
		std::wstring getName() const;
		/// Sets the game name.
		void setName(const std::wstring& name);

		/// Gets the game difficulty.
		GameDifficulty getDifficulty() const;
		/// Sets the game difficulty.
		void setDifficulty(GameDifficulty difficulty);

		/// Gets if the game is in ironman mode.
		bool isIronman() const;
		/// Sets if the game is in ironman mode.
		void setIronman(bool ironman);

		/// Gets the current globe longitude.
		double getGlobeLongitude() const;
		/// Sets the new globe longitude.
		void setGlobeLongitude(double lon);
		/// Gets the current globe latitude.
		double getGlobeLatitude() const;
		/// Sets the new globe latitude.
		void setGlobeLatitude(double lat);

		/// Gets the current globe zoom.
		size_t getGlobeZoom() const;
		/// Sets the new globe zoom.
		void setGlobeZoom(size_t zoom);

		/// Handles monthly funding.
		void monthlyFunding();

		/// Gets the current funds.
		int getFunds() const;
		/// Gets the list of funds from previous months.
		std::vector<int>& getFundsList();
		/// Sets new funds.
		void setFunds(int funds);

		/// Returns a list of maintenance costs
		std::vector<int>& getMaintenances();
		/// kL. Returns the list of monthly income values.
		std::vector<int>& getIncomeList(); // kL
		/// kL. Returns the list of monthly expenditure values.
		std::vector<int>& getExpenditureList(); // kL

		/// Gets the current game time.
		GameTime* getTime() const;
		/// Sets the current game time.
		void setTime(GameTime time);

		/// Gets the current ID for an object.
		int getId(const std::string& name);

		/// Gets the list of countries.
		std::vector<Country*>* getCountries();
		/// Gets the total country funding.
		int getCountryFunding() const;
		/// Gets the list of regions.
		std::vector<Region*>* getRegions();
		/// Gets the list of bases.
		std::vector<Base*>* getBases();
		/// Gets the list of bases.
		const std::vector<Base*>* getBases() const;

		/// Gets the total base maintenance.
		int getBaseMaintenance() const;

		/// Gets the list of UFOs.
		std::vector<Ufo*>* getUfos();
		/// Gets the list of waypoints.
		std::vector<Waypoint*>* getWaypoints();
		/// Gets the list of terror sites.
		std::vector<TerrorSite*>* getTerrorSites();

		/// Gets the current battle game.
		SavedBattleGame* getSavedBattle();
		/// Sets the current battle game.
		void setBattleGame(SavedBattleGame* battleGame);

		/// Add a finished ResearchProject
		void addFinishedResearch(
				const RuleResearch* r,
				const Ruleset* ruleset = NULL);
		/// Get the list of already discovered research projects
		const std::vector<const RuleResearch*>& getDiscoveredResearch() const;
		/// Get the list of ResearchProject which can be researched in a Base
		void getAvailableResearchProjects(
				std::vector<RuleResearch*>& projects,
				const Ruleset* ruleset,
				Base* base) const;
		/// Get the list of Productions which can be manufactured in a Base
		void getAvailableProductions(
				std::vector<RuleManufacture*>& productions,
				const Ruleset* ruleset,
				Base* base) const;
		/// Get the list of newly available research projects once a research has been completed.
		void getDependableResearch(std::vector<RuleResearch*>& dependables,
				const RuleResearch* research,
				const Ruleset* ruleset,
				Base* base) const;
		/// Get the list of newly available manufacture projects once a research has been completed.
		void getDependableManufacture(
				std::vector<RuleManufacture*>& dependables,
				const RuleResearch* research,
				const Ruleset* ruleset,
				Base* base) const;
		/// Check whether a ResearchProject can be researched
		bool isResearchAvailable(
				RuleResearch* r,
				const std::vector<const RuleResearch*>& unlocked,
				const Ruleset* ruleset) const;
		/// Gets if a research has been unlocked.
		bool isResearched(const std::string& research) const;
		/// Gets if a list of research has been unlocked.
		bool isResearched(const std::vector<std::string>& research) const;

		/// Gets the soldier matching this ID.
		Soldier* getSoldier(int id) const;
		/// Handles the higher promotions.
		bool handlePromotions(std::vector<Soldier*>& participants);
		/// Checks how many soldiers of a rank exist and which one has the highest score.
		void inspectSoldiers(
				Soldier** highestRanked,
				size_t* total,
				int rank);

		///  Returns the list of alien bases.
		std::vector<AlienBase*>* getAlienBases();

		/// Sets debug mode.
		void setDebugMode();
		/// Gets debug mode.
		bool getDebugMode() const;

		/// sets the research score for the month
		void addResearchScore(int score);
		/// gets the list of research scores
		std::vector<int>& getResearchScores();

		/// gets whether or not the player has been warned
		bool getWarned() const;
		/// sets whether or not the player has been warned
		void setWarned(bool warned);
		/// Full access to the alien strategy data.
		AlienStrategy& getAlienStrategy()
		{
			return *_alienStrategy;
		}
		/// Read-only access to the alien strategy data.
		const AlienStrategy& getAlienStrategy() const
		{
			return *_alienStrategy;
		}
		/// Full access to the current alien missions.
		std::vector<AlienMission*>& getAlienMissions()
		{
			return _activeMissions;
		}
		/// Read-only access to the current alien missions.
		const std::vector<AlienMission*>& getAlienMissions() const
		{
			return _activeMissions;
		}
		/// Gets a mission matching region and type.
		AlienMission* getAlienMission(
				const std::string& region,
				const std::string& type) const;
		/// Locate a region containing a position.
		Region* locateRegion(
				double lon,
				double lat) const;
		/// Locate a region containing a Target.
		Region* locateRegion(
				const Target& target) const;
		/// Return the month counter.
		int getMonthsPassed() const;
		/// Return the GraphRegionToggles.

		const std::string& getGraphRegionToggles() const;
		/// Return the GraphCountryToggles.
		const std::string& getGraphCountryToggles() const;
		/// Return the GraphFinanceToggles.
		const std::string& getGraphFinanceToggles() const;
		/// Sets the GraphRegionToggles.
		void setGraphRegionToggles(const std::string& value);
		/// Sets the GraphCountryToggles.
		void setGraphCountryToggles(const std::string& value);
		/// Sets the GraphFinanceToggles.
		void setGraphFinanceToggles(const std::string& value);

		/// Increment the month counter.
		void addMonth();
/*		/// toggle the current state of the radar line drawing
		void toggleRadarLines();
		/// check the current state of the radar line drawing
		bool getRadarLines();
		/// toggle the current state of the detail drawing
		void toggleDetail();
		/// check the current state of the detail drawing
		bool getDetail(); */

		/// add a research to the "popped up" array
		void addPoppedResearch(const RuleResearch* research);
		/// check if a research is on the "popped up" array
		bool wasResearchPopped(const RuleResearch* research);
		/// remove a research from the "popped up" array
		void removePoppedResearch(const RuleResearch* research);

		/// Gets the list of dead soldiers.
//kL	std::vector<Soldier*>* getDeadSoldiers();
		std::vector<SoldierDead*>* getDeadSoldiers(); // kL

		/// Gets the last selected player base.
//kL	Base* getSelectedBase();
		/// Set the last selected player base.
//kL	void setSelectedBase(size_t base);

		/// Evaluate the score of a soldier based on all of his stats, missions and kills.
		int getSoldierScore(Soldier* soldier);

		/// Gets the list of missions statistics
		std::vector<MissionStatistics*>* getMissionStatistics();
};

}

#endif
