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

#ifndef OPENXCOM_SAVEDGAME_H
#define OPENXCOM_SAVEDGAME_H

//#include <map>
//#include <string>
//#include <vector>
//#include <time.h>
//#include <stdint.h>

//#include "CraftId.h"
#include "GameTime.h"
#include "Soldier.h"

#include "../Ruleset/RuleAlienMission.h"


namespace OpenXcom
{

class AlienBase;
class AlienMission;
class AlienStrategy;
class Base;
class Country;
class Craft;
class GameTime;
class Language;
class MissionSite;
class Region;
class ResearchProject;
class RuleManufacture;
class RuleResearch;
class Ruleset;
class SavedBattleGame;
class Soldier;
class SoldierDead;
class Target;
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
 * Enumerator for the game mode.
 */
enum GameMode
{
	MODE_GEOSCAPE,		// 0
	MODE_BATTLESCAPE	// 1
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
		id,
		score,
		shade;

	std::string
		alienRace,
		country,
		rating,
		region,
		type,
		ufo;

	std::map<int, int> injuryList;

	GameTime timeStat;


	///
	std::string getMissionTypeLowerCase()
	{
		if		(type == "STR_UFO_CRASH_RECOVERY")	return "STR_UFO_CRASH_RECOVERY_LC";
		else if (type == "STR_UFO_GROUND_ASSAULT")	return "STR_UFO_GROUND_ASSAULT_LC";
		else if (type == "STR_BASE_DEFENSE")		return "STR_BASE_DEFENSE_LC";
		else if (type == "STR_ALIEN_BASE_ASSAULT")	return "STR_ALIEN_BASE_ASSAULT_LC";
		else if (type == "STR_TERROR_MISSION")		return "STR_TERROR_MISSION_LC";
		else if (type == "STR_PORT_ATTACK")			return "STR_PORT_ATTACK_LC";
		else										return "type error";
	}

	///
	void load(const YAML::Node& node)
	{
		timeStat.load(node["time"]);

		id			= node["id"]			.as<int>(id);
		region		= node["region"]		.as<std::string>(region);
		country		= node["country"]		.as<std::string>(country);
		type		= node["type"]			.as<std::string>(type);
		ufo			= node["ufo"]			.as<std::string>(ufo);
		success		= node["success"]		.as<bool>(success);
		score		= node["score"]			.as<int>(score);
		rating		= node["rating"]		.as<std::string>(rating);
		alienRace	= node["alienRace"]		.as<std::string>(alienRace);
		shade		= node["shade"]			.as<int>(shade);
		injuryList	= node["injuryList"]	.as<std::map<int, int> >(injuryList);
		valiantCrux	= node["valiantCrux"]	.as<bool>(valiantCrux);
	}

	///
	YAML::Node save() const
	{
		YAML::Node node;

		node["id"]			= id;
		node["time"]		= timeStat.save();
		node["region"]		= region;
		node["country"]		= country;
		node["type"]		= type;
		node["ufo"]			= ufo;
		node["success"]		= success;
		node["score"]		= score;
		node["rating"]		= rating;
		node["alienRace"]	= alienRace;
		node["shade"]		= shade;
		node["injuryList"]	= injuryList;

		if (valiantCrux == true)
			 node["valiantCrux"] = valiantCrux;

		return node;
	}

	/// cTor.
	explicit MissionStatistics(const YAML::Node& node)
		:
			timeStat(0,0,0,0,0,0,0)
	{
		load(node);
	}

	/// cTor.
	MissionStatistics()
		:
			id(0),
			timeStat(0,0,0,0,0,0,0),
			region("STR_REGION_UNKNOWN"),
			country("STR_UNKNOWN"),
			ufo("NUL_UFO"),
			success(false),
			score(0),
			alienRace("STR_UNKNOWN"),
			shade(0),
			valiantCrux(false)
	{}

	/// dTor.
	~MissionStatistics()
	{}
};


/**
 * Container for SaveGame info displayed on listings.
 */
struct SaveInfo
{
	int mode;
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
 * Container for Promotion info.
 */
struct PromotionInfo
{
	int
		totalCommanders,
		totalColonels,
		totalCaptains,
		totalSergeants;

	/// Builds this struct.
	PromotionInfo()
		:
			totalCommanders(0),
			totalColonels(0),
			totalCaptains(0),
			totalSergeants(0)
	{}
};


/**
 * The game data that gets written to disk when the game is saved.
 * @note A saved game holds all the variable info in a game like funds, game
 * time, current bases and contents, world activities, score, etc.
 */
class SavedGame // no copy cTor.
{

private:
	bool
		_debug,
		_debugArgDone,
		_ironman,
		_warned;
	int
		_curGraph,
		_monthsPassed;
	size_t
		_curRowMatrix,
		_dfZoom,
		_globeZoom;
//		_selectedBase;
	double
		_dfLat,
		_dfLon,
		_globeLat,
		_globeLon;

	GameDifficulty _difficulty;

	AlienStrategy* _alienStrategy;
	GameTime* _time;
	const Ruleset* const _rules;
	SavedBattleGame* _battleSave;

	std::wstring _name;
	std::string
		_debugArg,
		_graphRegionToggles,
		_graphCountryToggles,
		_graphFinanceToggles;
//		_lastselectedArmor;

	std::map<std::string, int> _ids;

	std::vector<int> _researchScores;
	std::vector<int64_t>
		_expenditure,
		_funds,
		_income,
		_maintenance;

	std::vector<AlienBase*> _alienBases;
	std::vector<AlienMission*> _activeMissions;
	std::vector<Base*> _bases;
	std::vector<Country*> _countries;
	std::vector<MissionSite*> _missionSites;
	std::vector<MissionStatistics*> _missionStatistics;
	std::vector<Region*> _regions;
	std::vector<const RuleResearch*>
		_discovered,
		_poppedResearch;
	std::vector<SoldierDead*> _deadSoldiers;
	std::vector<Ufo*> _ufos;
	std::vector<Waypoint*> _waypoints;

	/// Checks whether a ResearchProject can be started.
	bool isResearchAvailable(const RuleResearch* const resRule) const;
	/// Gets the list of newly available ResearchProjects that appear when a project is completed.
	void getDependentResearchBasic(
			std::vector<const RuleResearch*>& dependents,
			const RuleResearch* const resRule,
			Base* const base) const;
	/// Removes a research from the "popped up" array.
	void removePoppedResearch(const RuleResearch* const resRule);
	///
	static SaveInfo getSaveInfo(
			const std::string& file,
			const Language* const lang);

	public:
		static const std::string
			AUTOSAVE_GEOSCAPE,
			AUTOSAVE_BATTLESCAPE,
			QUICKSAVE;

		/// Creates a new saved game.
		explicit SavedGame(const Ruleset* const rules);
		/// Cleans up the saved game.
		~SavedGame();

		/// Gets list of saves in the user directory.
		static std::vector<SaveInfo> getList(
				Language* lang,
				bool autoquick);

		/// Loads a saved game from YAML.
		void load(
				const std::string& file,
				Ruleset* const rules);
		/// Saves a saved game to YAML.
		void save(const std::string& file) const;

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

		/// Gets the preDogfight globe longitude.
		double getDfLongitude() const;
		/// Sets the preDogfight globe longitude.
		void setDfLongitude(double lon);
		/// Gets the preDogfight globe latitude.
		double getDfLatitude() const;
		/// Sets the preDogfight globe latitude.
		void setDfLatitude(double lat);
		/// Gets the preDogfight globe zoom.
		size_t getDfZoom() const;
		/// Sets the preDogfight globe zoom.
		void setDfZoom(size_t zoom);

		/// Handles monthly funding.
		void monthlyFunding();

		/// Sets new funds.
		void setFunds(int64_t funds);
		/// Gets the current funds.
		int64_t getFunds() const;
		/// Gets the list of funds from previous months.
		std::vector<int64_t>& getFundsList();

		/// Returns a list of maintenance costs
		std::vector<int64_t>& getMaintenanceList();
		/// Returns the list of monthly income values.
		std::vector<int64_t>& getIncomeList();
		/// Returns the list of monthly expenditure values.
		std::vector<int64_t>& getExpenditureList();

		/// Gets the current game time.
		GameTime* getTime() const;
		/// Sets the current game time.
		void setTime(GameTime gt);

		/// Gets an ID to assign for an object.
		int getCanonicalId(const std::string& objectType);
		/// Resets the list of object IDs.
//		void setCanonicalIds(const std::map<std::string, int>& ids);

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
		int getBaseMaintenances() const;

		/// Gets the list of UFOs.
		std::vector<Ufo*>* getUfos();
		/// Gets the list of waypoints.
		std::vector<Waypoint*>* getWaypoints();
		/// Gets the list of mission sites.
		std::vector<MissionSite*>* getMissionSites();

		/// Gets the current battle save.
		SavedBattleGame* getBattleSave();
		/// Sets the current battle save.
		void setBattleSave(SavedBattleGame* const battleSave);

		/// Gets a list of already discovered ResearchProjects.
		const std::vector<const RuleResearch*>& getDiscoveredResearch() const;
		/// Adds a finished ResearchProject.
		void addFinishedResearch(
				const RuleResearch* const resRule,
				bool score = true);
		/// Gets a list of ResearchProjects that can be started at a particular Base.
		void getAvailableResearchProjects(
				std::vector<const RuleResearch*>& availableProjects,
				Base* const base) const;
		/// Gets a list of Productions that can be manufactured at a particular Base.
		void getAvailableProductions(
				std::vector<const RuleManufacture*>& availableProductions,
				const Base* const base) const;
		/// Gets the list of newly available ResearchProjects that appear when a project is completed.
		void getDependentResearch(
				std::vector<const RuleResearch*>& dependents,
				const RuleResearch* const resRule,
				Base* const base) const;
		/// Gets the list of newly available Productions that appear when a ResearchProject is completed.
		void getDependentManufacture(
				std::vector<const RuleManufacture*>& dependents,
				const RuleResearch* const resRule) const;
		/// Checks if a ResearchProject is discovered.
		bool isResearched(const std::string& resType) const;
		/// Checks if a list of ResearchProjects is discovered.
		bool isResearched(const std::vector<std::string>& resTypes) const;

		/// Gets the soldier matching an ID.
		Soldier* getSoldier(int id) const;
		/// Handles the higher promotions.
		bool handlePromotions(std::vector<Soldier*>& participants);
		/// Processes a soldier for promotion.
		void processSoldier(
				const Soldier* const soldier,
				PromotionInfo& promoData);
		/// Checks how many soldiers of a rank exist and which one has the highest score.
		Soldier* inspectSoldiers(
				const std::vector<Soldier*>& soldiers,
				const std::vector<Soldier*>& participants,
				SoldierRank soldierRank);

		///  Returns the list of alien bases.
		std::vector<AlienBase*>* getAlienBases();

		/// Sets debug mode.
		void toggleDebugMode();
		/// Gets debug mode.
		bool getDebugMode() const;

		/// Sets the research score for the month.
		void addResearchScore(int score);
		/// Gets the list of research scores.
		std::vector<int>& getResearchScores();

		/// Gets whether or not the player has been warned.
		bool getWarned() const;
		/// Sets whether or not the player has been warned.
		void setWarned(bool warned = true);

		/// Full access to the alien strategy data.
		AlienStrategy& getAlienStrategy()
		{ return *_alienStrategy; }
		/// Read-only access to the alien strategy data.
		const AlienStrategy& getAlienStrategy() const
		{ return *_alienStrategy; }
		/// Full access to the current alien missions.
		std::vector<AlienMission*>& getAlienMissions()
		{ return _activeMissions; }
		/// Read-only access to the current alien missions.
		const std::vector<AlienMission*>& getAlienMissions() const
		{ return _activeMissions; }
		/// Finds a mission by region and objective.
		AlienMission* findAlienMission(
				const std::string& region,
				MissionObjective objective) const;

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

		/// Sets the current Graph page.
		void setCurrentGraph(int page);
		/// Gets the current Graph page.
		int getCurrentGraph() const;

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

		/// Adds a research to the "popped up" array.
		void addPoppedResearch(const RuleResearch* const resRule);
		/// Checks if a research is on the "popped up" array.
		bool wasResearchPopped(const RuleResearch* const resRule);

		/// Gets the list of dead soldiers.
		std::vector<SoldierDead*>* getDeadSoldiers();

		/// Gets the last selected player base.
//		Base* getSelectedBase();
		/// Set the last selected player base.
//		void setSelectedBase(size_t base);

		/// Evaluate the score of a soldier based on all of his stats, missions and kills.
		int getSoldierScore(Soldier* soldier);

		/// Sets the last selected armour
//		void setLastSelectedArmor(const std::string& value);
		/// Gets the last selected armour
//		std::string getLastSelectedArmor();

		/// Sets the current Matrix row.
		void setCurrentRowMatrix(size_t row);
		/// Gets the current Matrix row.
		size_t getCurrentRowMatrix() const;

		/// Sets a debug argument to GeoscapeState.
		void setDebugArg(const std::string& debug);
		/// Gets a debug argument from Globe.
		std::string getDebugArg() const;
		/// Gets if the debug argument has been set.
		bool getDebugArgDone();

		/// Gets the list of missions statistics
		std::vector<MissionStatistics*>* getMissionStatistics();

		/// Scores points for XCom or aLiens.
		void scorePoints(
				double lon,
				double lat,
				int pts,
				bool aLien) const;

		/// Returns the craft corresponding to the specified unique id.
//		Craft* findCraftByUniqueId(const CraftId& craftId) const;
};

}

#endif
