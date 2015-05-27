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

#include "SavedGame.h"

//#include <algorithm>
//#include <fstream>
//#include <iomanip>
//#include <sstream>
//#include <yaml-cpp/yaml.h>

#include "AlienBase.h"
#include "AlienMission.h"
#include "AlienStrategy.h"
#include "Base.h"
#include "Country.h"
#include "Craft.h"
#include "GameTime.h"
#include "ItemContainer.h"
#include "MissionSite.h"
#include "Production.h"
#include "Region.h"
#include "ResearchProject.h"
#include "SavedBattleGame.h"
#include "SerializationHelper.h"
#include "SoldierDead.h"
#include "Transfer.h"
#include "Ufo.h"
#include "Waypoint.h"

//#include "../version.h"

//#include "../Engine/CrossPlatform.h"
//#include "../Engine/Exception.h"
#include "../Engine/Language.h"
//#include "../Engine/Logger.h"
//#include "../Engine/Options.h"
//#include "../Engine/RNG.h"

#include "../Ruleset/RuleManufacture.h"
#include "../Ruleset/RuleRegion.h"
#include "../Ruleset/RuleResearch.h"
#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

const std::string
	SavedGame::AUTOSAVE_GEOSCAPE	= "_autogeo_.asav",
	SavedGame::AUTOSAVE_BATTLESCAPE	= "_autobattle_.asav",
	SavedGame::QUICKSAVE			= "_quick_.asav";


///
struct findRuleResearch
	:
		public std::unary_function<ResearchProject*, bool>
{
	RuleResearch* _toFind;
	explicit findRuleResearch(RuleResearch* toFind);

	bool operator() (const ResearchProject* r) const;
};

///
findRuleResearch::findRuleResearch(RuleResearch* toFind)
	:
		_toFind(toFind)
{}

///
bool findRuleResearch::operator() (const ResearchProject* r) const
{
	return _toFind == r->getRules();
}

///
struct equalProduction
	:
		public std::unary_function<Production*, bool>
{
	RuleManufacture* _item;
	explicit equalProduction(RuleManufacture* item);

	bool operator() (const Production* p) const;
};

///
equalProduction::equalProduction(RuleManufacture* item)
	:
		_item(item)
{}

///
bool equalProduction::operator() (const Production* p) const
{
	return p->getRules() == _item;
}


/**
 * Initializes a brand new saved game according to the specified difficulty.
 * @param rules - pointer to the Ruleset kL
 */
SavedGame::SavedGame(const Ruleset* const rules)
	:
		_rules(rules),
		_difficulty(DIFF_BEGINNER),
		_ironman(false),
		_globeLon(0.),
		_globeLat(0.),
		_globeZoom(0),
		_dfLon(0.),
		_dfLat(0.),
		_dfZoom(0),
		_battleGame(NULL),
		_debug(false),
		_warned(false),
//		_detail(true),
//		_radarLines(false),
		_monthsPassed(-1),
		_curRowMatrix(0),		// kL
		_curGraph(0),			// kL
//		_curGraphRowCountry(0)	// kL
//		_selectedBase(0),
//		_lastselectedArmor("STR_ARMOR_NONE_UC")
		_debugArgDone(false)
{
	_time = new GameTime(6,1,1,1999,12,0,0);

	_alienStrategy = new AlienStrategy();

	_funds.push_back(0);
	_maintenance.push_back(0);
	_researchScores.push_back(0);
	_income.push_back(0);
	_expenditure.push_back(0);
}

/**
 * Deletes the game content from memory.
 */
SavedGame::~SavedGame()
{
	delete _time;

	for (std::vector<Country*>::const_iterator
			i = _countries.begin();
			i != _countries.end();
			++i)
	{
		delete *i;
	}

	for (std::vector<Region*>::const_iterator
			i = _regions.begin();
			i != _regions.end();
			++i)
	{
		delete *i;
	}

	for (std::vector<Base*>::const_iterator
			i = _bases.begin();
			i != _bases.end();
			++i)
	{
		delete *i;
	}

	for (std::vector<Ufo*>::const_iterator
			i = _ufos.begin();
			i != _ufos.end();
			++i)
	{
		delete *i;
	}

	for (std::vector<Waypoint*>::const_iterator
			i = _waypoints.begin();
			i != _waypoints.end();
			++i)
	{
		delete *i;
	}

	for (std::vector<MissionSite*>::const_iterator
			i = _missionSites.begin();
			i != _missionSites.end();
			++i)
	{
		delete *i;
	}

	for (std::vector<AlienBase*>::const_iterator
			i = _alienBases.begin();
			i != _alienBases.end();
			++i)
 	{
		delete *i;
	}

	delete _alienStrategy;

	for (std::vector<AlienMission*>::const_iterator
			i = _activeMissions.begin();
			i != _activeMissions.end();
			++i)
	{
		delete *i;
	}

	for (std::vector<SoldierDead*>::const_iterator
			i = _deadSoldiers.begin();
			i != _deadSoldiers.end();
			++i)
	{
		delete *i;
	}

	for (std::vector<MissionStatistics*>::const_iterator
			i = _missionStatistics.begin();
			i != _missionStatistics.end();
			++i)
	{
		delete *i;
	}

	delete _battleGame;
}

/**
 * Gets all the info of the saves found in the user folder.
 * @param lang		- pointer to the loaded Language
 * @param autoquick	- true to include autosaves and quicksaves
 * @return, vector of SavesInfo structs (SavedGame.h)
 */
std::vector<SaveInfo> SavedGame::getList(
		Language* lang,
		bool autoquick)
{
	std::vector<SaveInfo> info;

	if (autoquick == true)
	{
		const std::vector<std::string> saves = CrossPlatform::getFolderContents(
																			Options::getUserFolder(),
																			"asav");
		for (std::vector<std::string>::const_iterator
				i = saves.begin();
				i != saves.end();
				++i)
		{
			try
			{
				info.push_back(getSaveInfo(
										*i,
										lang));
			}
			catch (Exception &e)
			{
				Log(LOG_ERROR) << e.what();
				continue;
			}
			catch (YAML::Exception &e)
			{
				Log(LOG_ERROR) << e.what();
				continue;
			}
		}
	}

	const std::vector<std::string> saves = CrossPlatform::getFolderContents(
																		Options::getUserFolder(),
																		"sav");
	for (std::vector<std::string>::const_iterator
			i = saves.begin();
			i != saves.end();
			++i)
	{
		try
		{
			info.push_back(getSaveInfo(
									*i,
									lang));
		}
		catch (Exception &e)
		{
			Log(LOG_ERROR) << e.what();
			continue;
		}
		catch (YAML::Exception &e)
		{
			Log(LOG_ERROR) << e.what();
			continue;
		}
	}

	return info;
}

/**
 * Gets the info of a specific save file.
 * @param file - reference the filename of a save
 * @param lang - pointer to the loaded Language
 * @return, SaveInfo struct (SavedGame.h)
 */
SaveInfo SavedGame::getSaveInfo( // private.
		const std::string& file,
		Language* lang)
{
	const std::string fullname = Options::getUserFolder() + file;
	const YAML::Node doc = YAML::LoadFile(fullname);
	SaveInfo save;

	save.fileName = file;

	if (save.fileName == QUICKSAVE)
	{
		save.displayName = lang->getString("STR_QUICK_SAVE_SLOT");
		save.reserved = true;
	}
	else if (save.fileName == AUTOSAVE_GEOSCAPE)
	{
		save.displayName = lang->getString("STR_AUTO_SAVE_GEOSCAPE_SLOT");
		save.reserved = true;
	}
	else if (save.fileName == AUTOSAVE_BATTLESCAPE)
	{
		save.displayName = lang->getString("STR_AUTO_SAVE_BATTLESCAPE_SLOT");
		save.reserved = true;
	}
	else
	{
		if (doc["name"])
			save.displayName = Language::utf8ToWstr(doc["name"].as<std::string>());
		else
			save.displayName = Language::fsToWstr(CrossPlatform::noExt(file));

		save.reserved = false;
	}

	save.timestamp = CrossPlatform::getDateModified(fullname);
	const std::pair<std::wstring, std::wstring> strTime = CrossPlatform::timeToString(save.timestamp);
	save.isoDate = strTime.first;
	save.isoTime = strTime.second;

	std::wostringstream details;
	if (doc["base"])
	{
		details << Language::utf8ToWstr(doc["base"].as<std::string>());
		details << L" - ";
	}

	GameTime gt = GameTime(6,1,1,1999,12,0,0);
	gt.load(doc["time"]);
	details << gt.getDayString(lang) << L" " << lang->getString(gt.getMonthString()) << L" " << gt.getYear() << L" ";
	details << gt.getHour() << L":" << std::setfill(L'0') << std::setw(2) << gt.getMinute();

	if (doc["turn"])
	{
		details << L" - ";
		details << lang->getString(doc["mission"].as<std::string>()) << L" ";
		details << lang->getString("STR_TURN").arg(doc["turn"].as<int>());
	}

	if (doc["ironman"].as<bool>(false))
		details << L" " << lang->getString("STR_IRONMAN");

	save.details = details.str();

	if (doc["rulesets"])
		save.rulesets = doc["rulesets"].as<std::vector<std::string> >();

	save.mode = (GameMode)doc["mode"].as<int>();

	return save;
}

/**
 * Loads a saved game's contents from a YAML file.
 * @note Assumes the saved game is blank.
 * @param filename	- reference a YAML filename
 * @param rule		- pointer to Ruleset
 */
void SavedGame::load(
		const std::string& filename,
		Ruleset* rule)
{
	Log(LOG_INFO) << "SavedGame::load()";

	const std::string st = Options::getUserFolder() + filename;
	const std::vector<YAML::Node> file = YAML::LoadAllFromFile(st);
	if (file.empty() == true)
	{
		throw Exception(filename + " is not a valid save file");
	}

	YAML::Node brief = file[0]; // Get brief save info

/*	std::string version = brief["version"].as<std::string>();
	if (version != OPENXCOM_VERSION_SHORT)
	{
		throw Exception("Version mismatch");
	} */

	_time->load(brief["time"]);
	if (brief["name"])
		_name = Language::utf8ToWstr(brief["name"].as<std::string>());
	else
		_name = Language::fsToWstr(filename);

	YAML::Node doc = file[1]; // Get full save data

	if (doc["rng"]
		&& (_ironman == true
			|| Options::newSeedOnLoad == false))
	{
		RNG::setSeed(doc["rng"].as<uint64_t>());
	}
	else
		RNG::setSeed(0);

	_difficulty = static_cast<GameDifficulty>(doc["difficulty"].as<int>(_difficulty));

	_monthsPassed			= doc["monthsPassed"]		.as<int>(_monthsPassed);
	_graphRegionToggles		= doc["graphRegionToggles"]	.as<std::string>(_graphRegionToggles);
	_graphCountryToggles	= doc["graphCountryToggles"].as<std::string>(_graphCountryToggles);
	_graphFinanceToggles	= doc["graphFinanceToggles"].as<std::string>(_graphFinanceToggles);
	_funds					= doc["funds"]				.as<std::vector<int64_t> >(_funds);
	_maintenance			= doc["maintenance"]		.as<std::vector<int64_t> >(_maintenance);
	_researchScores			= doc["researchScores"]		.as<std::vector<int> >(_researchScores);
	_income					= doc["income"]				.as<std::vector<int64_t> >(_income);
	_expenditure			= doc["expenditure"]		.as<std::vector<int64_t> >(_expenditure);
	_warned					= doc["warned"]				.as<bool>(_warned);
	_ids					= doc["ids"]				.as<std::map<std::string, int> >(_ids);
//	_radarLines				= doc["radarLines"]			.as<bool>(_radarLines);
//	_detail					= doc["detail"]				.as<bool>(_detail);

	_globeLon				= doc["globeLon"].as<double>(_globeLon);
	_globeLat				= doc["globeLat"].as<double>(_globeLat);
	_globeZoom				= static_cast<size_t>(doc["globeZoom"].as<int>(_globeZoom));


	Log(LOG_INFO) << ". load countries";
	for (YAML::const_iterator
			i = doc["countries"].begin();
			i != doc["countries"].end();
			++i)
	{
		const std::string type = (*i)["type"].as<std::string>();
		if (rule->getCountry(type))
		{
			Country* const c = new Country(
										rule->getCountry(type),
										false);
			c->load(*i);
			_countries.push_back(c);
		}
	}

	Log(LOG_INFO) << ". load regions";
	for (YAML::const_iterator
			i = doc["regions"].begin();
			i != doc["regions"].end();
			++i)
	{
		const std::string type = (*i)["type"].as<std::string>();
		if (rule->getRegion(type))
		{
			Region* const r = new Region(rule->getRegion(type));
			r->load(*i);
			_regions.push_back(r);
		}
	}

	Log(LOG_INFO) << ". load alien bases";
	// Alien bases must be loaded before alien missions
	for (YAML::const_iterator
			i = doc["alienBases"].begin();
			i != doc["alienBases"].end();
			++i)
	{
		AlienBase* const b = new AlienBase();
		b->load(*i);
		_alienBases.push_back(b);
	}

	Log(LOG_INFO) << ". load missions";
	// Missions must be loaded before UFOs.
	const YAML::Node& missions = doc["alienMissions"];
	for (YAML::const_iterator
			i = missions.begin();
			i != missions.end();
			++i)
	{
		const std::string missionType = (*i)["type"].as<std::string>();
		const RuleAlienMission& missionRule = *rule->getAlienMission(missionType);
		std::auto_ptr<AlienMission> mission (new AlienMission( // init.
															missionRule,
															*this));
		mission->load(*i);
		_activeMissions.push_back(mission.release());
	}

	Log(LOG_INFO) << ". load ufos";
	for (YAML::const_iterator
			i = doc["ufos"].begin();
			i != doc["ufos"].end();
			++i)
	{
		const std::string type = (*i)["type"].as<std::string>();
		if (rule->getUfo(type))
		{
			Ufo* const u = new Ufo(rule->getUfo(type));
			u->load(
					*i,
					*rule,
					*this);
			_ufos.push_back(u);
		}
	}

	Log(LOG_INFO) << ". load waypoints";
	for (YAML::const_iterator
			i = doc["waypoints"].begin();
			i != doc["waypoints"].end();
			++i)
	{
		Waypoint* const w = new Waypoint();
		w->load(*i);
		_waypoints.push_back(w);
	}

	Log(LOG_INFO) << ". load mission sites";
	for (YAML::const_iterator
			i = doc["missionSites"].begin();
			i != doc["missionSites"].end();
			++i)
	{
		const std::string
			type = (*i)["type"].as<std::string>(),
			deployment = (*i)["deployment"].as<std::string>("STR_TERROR_MISSION");
		MissionSite* const ms = new MissionSite(
											rule->getAlienMission(type),
											rule->getDeployment(deployment));
		ms->load(*i);
		_missionSites.push_back(ms);
	}

	Log(LOG_INFO) << ". load discovered research";
	// Discovered Techs should be loaded before Bases (e.g. for PSI evaluation)
	for (YAML::const_iterator
			it = doc["discovered"].begin();
			it != doc["discovered"].end();
			++it)
	{
		const std::string research = it->as<std::string>();
		if (rule->getResearch(research))
			_discovered.push_back(rule->getResearch(research));
	}

	Log(LOG_INFO) << ". load xcom bases";
	for (YAML::const_iterator
			i = doc["bases"].begin();
			i != doc["bases"].end();
			++i)
	{
		Base* const b = new Base(rule);
		b->load(
				*i,
				this,
				false);
		_bases.push_back(b);
	}

	Log(LOG_INFO) << ". load popped research";
	const YAML::Node& research = doc["poppedResearch"];
	for (YAML::const_iterator
			it = research.begin();
			it != research.end();
			++it)
	{
		const std::string research = it->as<std::string>();
		if (rule->getResearch(research))
			_poppedResearch.push_back(rule->getResearch(research));
	}

	Log(LOG_INFO) << ". load alien strategy";
	_alienStrategy->load(
						rule,
						doc["alienStrategy"]);

	Log(LOG_INFO) << ". load dead soldiers";
	for (YAML::const_iterator
			i = doc["deadSoldiers"].begin();
			i != doc["deadSoldiers"].end();
			++i)
	{
		SoldierDead* const deadSoldier = new SoldierDead(
													L"",
													0,
													RANK_ROOKIE,
													GENDER_MALE,
													LOOK_BLONDE,
													0,
													0,
													NULL,
													UnitStats(),
													UnitStats());
		deadSoldier->load(*i);
		_deadSoldiers.push_back(deadSoldier);
	}

	Log(LOG_INFO) << ". load mission statistics";
	for (YAML::const_iterator
			i = doc["missionStatistics"].begin();
			i != doc["missionStatistics"].end();
			++i)
	{
		MissionStatistics* const ms = new MissionStatistics();
		ms->load(*i);

		_missionStatistics.push_back(ms);
	}

	if (const YAML::Node& battle = doc["battleGame"])
	{
		Log(LOG_INFO) << "SavedGame: loading battlegame";
		_battleGame = new SavedBattleGame();
		_battleGame->load(
						battle,
						rule,
						this);
		Log(LOG_INFO) << "SavedGame: loading battlegame DONE";
	}
}

/**
 * Saves a saved game's contents to a YAML file.
 * @param filename - reference to a YAML filename
 */
void SavedGame::save(const std::string& filename) const
{
	const std::string st = Options::getUserFolder() + filename;
	std::ofstream save(st.c_str());
	if (save.fail() == true)
	{
		throw Exception("Failed to save " + filename);
	}

	YAML::Emitter emit;
	YAML::Node brief; // Saves the brief game info used in the saves list

	brief["name"]		= Language::wstrToUtf8(_name);
	brief["edition"]	= OPENXCOM_VERSION_GIT;
//	brief["version"]	= OPENXCOM_VERSION_SHORT;
	brief["build"]		= Version::getBuildDate(false);
	brief["savedate"]	= Version::timeStamp();
	brief["time"]		= _time->save();

	const Base* const base	= _bases.front();
	brief["base"]			= Language::wstrToUtf8(base->getName());

	if (_battleGame != NULL)
	{
		brief["mission"]	= _battleGame->getMissionType();
		brief["turn"]		= _battleGame->getTurn();
		brief["mode"]		= static_cast<int>(MODE_BATTLESCAPE);
	}
	else
		brief["mode"]		= static_cast<int>(MODE_GEOSCAPE);

	brief["rulesets"] = Options::rulesets;

	emit << brief;
	emit << YAML::BeginDoc; // Saves the full game data to the save

	YAML::Node node;

	node["rng"]					= RNG::getSeed();
	node["difficulty"]			= static_cast<int>(_difficulty);
	node["monthsPassed"]		= _monthsPassed;
	node["graphRegionToggles"]	= _graphRegionToggles;
	node["graphCountryToggles"]	= _graphCountryToggles;
	node["graphFinanceToggles"]	= _graphFinanceToggles;
	node["funds"]				= _funds;
	node["maintenance"]			= _maintenance;
	node["researchScores"]		= _researchScores;
	node["income"]				= _income;
	node["expenditure"]			= _expenditure;
	node["warned"]				= _warned;
	node["ids"]					= _ids;
//	node["radarLines"]			= _radarLines;
//	node["detail"]				= _detail;

	node["globeLon"]	= serializeDouble(_globeLon);
	node["globeLat"]	= serializeDouble(_globeLat);
	node["globeZoom"]	= static_cast<int>(_globeZoom);


	for (std::vector<Country*>::const_iterator
			i = _countries.begin();
			i != _countries.end();
			++i)
	{
		node["countries"].push_back((*i)->save());
	}

	for (std::vector<Region*>::const_iterator
			i = _regions.begin();
			i != _regions.end();
			++i)
	{
		node["regions"].push_back((*i)->save());
	}

	for (std::vector<Base*>::const_iterator
			i = _bases.begin();
			i != _bases.end();
			++i)
	{
		node["bases"].push_back((*i)->save());
	}

	for (std::vector<Waypoint*>::const_iterator
			i = _waypoints.begin();
			i != _waypoints.end();
			++i)
	{
		node["waypoints"].push_back((*i)->save());
	}

	for (std::vector<MissionSite*>::const_iterator
			i = _missionSites.begin();
			i != _missionSites.end();
			++i)
	{
		node["missionSites"].push_back((*i)->save());
	}

	// Alien bases must be saved before alien missions.
	for (std::vector<AlienBase*>::const_iterator
			i = _alienBases.begin();
			i != _alienBases.end();
			++i)
	{
		node["alienBases"].push_back((*i)->save());
	}

	// Missions must be saved before UFOs, but after alien bases.
	for (std::vector<AlienMission*>::const_iterator
			i = _activeMissions.begin();
			i != _activeMissions.end();
			++i)
	{
		node["alienMissions"].push_back((*i)->save());
	}

	// UFOs must be after missions
	for (std::vector<Ufo*>::const_iterator
			i = _ufos.begin();
			i != _ufos.end();
			++i)
	{
		node["ufos"].push_back((*i)->save(getMonthsPassed() == -1));
	}

	for (std::vector<const RuleResearch*>::const_iterator
			i = _discovered.begin();
			i != _discovered.end();
			++i)
	{
		node["discovered"].push_back((*i)->getName());
	}

	for (std::vector<const RuleResearch*>::const_iterator
			i = _poppedResearch.begin();
			i != _poppedResearch.end();
			++i)
	{
		node["poppedResearch"].push_back((*i)->getName());
	}

	node["alienStrategy"] = _alienStrategy->save();

	for (std::vector<SoldierDead*>::const_iterator
			i = _deadSoldiers.begin();
			i != _deadSoldiers.end();
			++i)
	{
		node["deadSoldiers"].push_back((*i)->save());
	}

	for (std::vector<MissionStatistics*>::const_iterator
			i = _missionStatistics.begin();
			i != _missionStatistics.end();
			++i)
	{
		node["missionStatistics"].push_back((*i)->save());
	}

	if (_battleGame != NULL)
		node["battleGame"] = _battleGame->save();

	emit << node;
	save << emit.c_str();
	save.close();
}

/**
 * Gets the game's name shown in Save screens.
 * @return, save name
 */
std::wstring SavedGame::getName() const
{
	return _name;
}

/**
 * Sets the game's name shown in Save screens.
 * @param name - reference to the new save name
 */
void SavedGame::setName(const std::wstring& name)
{
	_name = name;
}

/**
 * Gets the game's difficulty setting.
 * @return, difficulty level
 */
GameDifficulty SavedGame::getDifficulty() const
{
	return _difficulty;
}

/**
 * Sets the game's difficulty to a new level.
 * @param difficulty - new difficulty setting
 */
void SavedGame::setDifficulty(GameDifficulty difficulty)
{
	_difficulty = difficulty;
}

/**
 * Returns if the game is set to ironman mode.
 * Ironman games cannot be manually saved.
 * @return, Tony Stark
 */
bool SavedGame::isIronman() const
{
	return _ironman;
}

/**
 * Changes if the game is set to ironman mode.
 * Ironman games cannot be manually saved.
 * @param ironman - Tony Stark
 */
void SavedGame::setIronman(bool ironman)
{
	_ironman = ironman;
}

/**
 * Returns the current longitude of the Geoscape globe.
 * @return, longitude
 */
double SavedGame::getGlobeLongitude() const
{
	return _globeLon;
}

/**
 * Changes the current longitude of the Geoscape globe.
 * @param lon - longitude
 */
void SavedGame::setGlobeLongitude(double lon)
{
	_globeLon = lon;
}

/**
 * Returns the current latitude of the Geoscape globe.
 * @return, latitude
 */
double SavedGame::getGlobeLatitude() const
{
	return _globeLat;
}

/**
 * Changes the current latitude of the Geoscape globe.
 * @param lat - latitude
 */
void SavedGame::setGlobeLatitude(double lat)
{
	_globeLat = lat;
}

/**
 * Returns the current zoom level of the Geoscape globe.
 * @return, zoom level
 */
size_t SavedGame::getGlobeZoom() const
{
	return _globeZoom;
}

/**
 * Changes the current zoom level of the Geoscape globe.
 * @param zoom - zoom level
 */
void SavedGame::setGlobeZoom(size_t zoom)
{
	_globeZoom = zoom;
}

/**
 * Returns the preDogfight longitude of the Geoscape globe.
 * @return, longitude
 */
double SavedGame::getDfLongitude() const
{
	return _dfLon;
}

/**
 * Changes the preDogfight longitude of the Geoscape globe.
 * @param lon - longitude
 */
void SavedGame::setDfLongitude(double lon)
{
	_dfLon = lon;
}

/**
 * Returns the preDogfight latitude of the Geoscape globe.
 * @return, latitude
 */
double SavedGame::getDfLatitude() const
{
	return _dfLat;
}

/**
 * Changes the preDogfight latitude of the Geoscape globe.
 * @param lat - latitude
 */
void SavedGame::setDfLatitude(double lat)
{
	_dfLat = lat;
}

/**
 * Returns the preDogfight zoom level of the Geoscape globe.
 * @return, zoom level
 */
size_t SavedGame::getDfZoom() const
{
	return _dfZoom;
}

/**
 * Changes the preDogfight zoom level of the Geoscape globe.
 * @param zoom - zoom level
 */
void SavedGame::setDfZoom(size_t zoom)
{
	_dfZoom = zoom;
}

/**
 * Gives the player his monthly funds taking in account all maintenance and
 * profit costs. Also stores monthly totals for GraphsState.
 */
void SavedGame::monthlyFunding()
{
	int
		income = 0,
		expenditure = 0;

	for (std::vector<Base*>::const_iterator
			i = _bases.begin();
			i != _bases.end();
			++i)
	{
		income += (*i)->getCashIncome();
		expenditure += (*i)->getCashSpent();

		(*i)->setCashIncome(-(*i)->getCashIncome());	// zero each Base's cash income values.
		(*i)->setCashSpent(-(*i)->getCashSpent());		// zero each Base's cash spent values.
	}

	// INCOME
	_income.back() = income;
	_income.push_back(0);
	if (_income.size() > 12)
		_income.erase(_income.begin());

	// EXPENDITURE
	_expenditure.back() = expenditure;
	_expenditure.push_back(0);
	if (_expenditure.size() > 12)
		_expenditure.erase(_expenditure.begin());


	// MAINTENANCE
	const int maintenance = getBaseMaintenances();

	_maintenance.back() = maintenance;
	_maintenance.push_back(0);
	if (_maintenance.size() > 12)
		_maintenance.erase(_maintenance.begin());

	// BALANCE
	_funds.back() += getCountryFunding() - maintenance;
	_funds.push_back(_funds.back());
	if (_funds.size() > 12)
		_funds.erase(_funds.begin());

	// SCORE (doesn't include xCom - aLien activity)
	_researchScores.push_back(0);
	if (_researchScores.size() > 12)
		_researchScores.erase(_researchScores.begin());

}

/**
 * Changes the player's funds to a new value.
 * @param funds - new funds
 */
void SavedGame::setFunds(int64_t funds)
{
	_funds.back() = funds;
}

/**
 * Returns the player's current funds.
 * @return, current funds
 */
int64_t SavedGame::getFunds() const
{
	return _funds.back();
}

/**
 * Returns the player's funds for the last 12 months.
 * @return, reference a vector of funds
 */
std::vector<int64_t>& SavedGame::getFundsList()
{
	return _funds;
}

/**
 * Returns the list of monthly maintenance costs.
 * @return, reference a vector of maintenances
 */
std::vector<int64_t>& SavedGame::getMaintenanceList()
{
	return _maintenance;
}

/**
 * Returns the list of monthly income values.
 * @return, reference a vector of incomes
 */
std::vector<int64_t>& SavedGame::getIncomeList()
{
	return _income;
}

/**
 * Returns the list of monthly expenditure values.
 * @return, reference a vector of expenditures
 */
std::vector<int64_t>& SavedGame::getExpenditureList()
{
	return _expenditure;
}

/**
 * Returns the current time of the game.
 * @return, pointer to the GameTime
 */
GameTime* SavedGame::getTime() const
{
	return _time;
}

/**
 * Changes the current time of the game.
 * @param time - GameTime
 */
void SavedGame::setTime(GameTime gt)
{
	_time = new GameTime(gt);
}

/**
 * Returns the latest ID for the specified object and increases it.
 * @param objectType - reference an object string
 * @return, latest ID number
 */
int SavedGame::getId(const std::string& objectType)
{
	std::map<std::string, int>::iterator i = _ids.find(objectType);
	if (i != _ids.end())
		return i->second++;

	_ids[objectType] = 1;
	return _ids[objectType]++;
}

/**
 * Resets the list of unique object IDs.
 * @param ids - new ID list as a reference to a map of strings & ints
 */
void SavedGame::setIds(const std::map<std::string, int>& ids)
{
	_ids = ids;
}

/**
 * Returns the list of countries in the game world.
 * @return, pointer to a vector of pointers to the Countries
 */
std::vector<Country*>* SavedGame::getCountries()
{
	return &_countries;
}

/**
 * Adds up the monthly funding of all the countries.
 * @return, total funding
 */
int SavedGame::getCountryFunding() const
{
	int total = 0;

	for (std::vector<Country*>::const_iterator
			i = _countries.begin();
			i != _countries.end();
			++i)
	{
		total += (*i)->getFunding().back();
	}

	return total;
}

/**
 * Returns the list of world regions.
 * @return, pointer to a vector of pointers to the Regions
 */
std::vector<Region*>* SavedGame::getRegions()
{
	return &_regions;
}

/**
 * Returns the list of player bases.
 * @return, pointer to a vector of pointers to all Bases
 */
std::vector<Base*>* SavedGame::getBases()
{
	return &_bases;
}

/**
 * Returns an immutable list of player bases.
 * @return, pointer to a vector of pointers to all Bases
 */
const std::vector<Base*>* SavedGame::getBases() const
{
	return &_bases;
}

/**
 * Adds up the monthly maintenance of all the bases.
 * @return, total maintenance
 */
int SavedGame::getBaseMaintenances() const
{
	int total = 0;

	for (std::vector<Base*>::const_iterator
			i = _bases.begin();
			i != _bases.end();
			++i)
	{
		total += (*i)->getMonthlyMaintenace();
	}

	return total;
}

/**
 * Returns the list of alien UFOs.
 * @return, pointer to a vector of pointers to all Ufos
 */
std::vector<Ufo*>* SavedGame::getUfos()
{
	return &_ufos;
}

/**
 * Returns the list of craft waypoints.
 * @return, pointer to a vector of pointers to all Waypoints
 */
std::vector<Waypoint*>* SavedGame::getWaypoints()
{
	return &_waypoints;
}

/**
 * Returns the list of mission sites.
 * @return, pointer to a vector of pointers to all MissionSites
 */
std::vector<MissionSite*>* SavedGame::getMissionSites()
{
	return &_missionSites;
}

/**
 * Get pointer to the SavedBattleGame object.
 * @return, pointer to the SavedBattleGame object
 */
SavedBattleGame* SavedGame::getSavedBattle()
{
	return _battleGame;
}

/**
 * Set SavedBattleGame object.
 * @param battleGame - pointer to a new SavedBattleGame object
 */
void SavedGame::setBattleGame(SavedBattleGame* battleGame)
{
	delete _battleGame;
	_battleGame = battleGame;
}

/**
 * Adds a ResearchProject to the list of already discovered ResearchProjects.
 * @param resRule	- the newly found ResearchProject
 * @param rules		- the game Ruleset (NULL if single battle skirmish)
 * @param score		- true to score points for research done (default true)
 */
void SavedGame::addFinishedResearch(
		const RuleResearch* resRule,
		const Ruleset* rules,
		bool score)
{
	const std::vector<const RuleResearch*>::const_iterator i = std::find(
																	_discovered.begin(),
																	_discovered.end(),
																	resRule);
	if (i == _discovered.end())
	{
		_discovered.push_back(resRule);

/*		if (rules
			&& resRule->getName() == "STR_POWER_SUIT")
		{
			_discovered.push_back(rules->getResearch("STR_BLACKSUIT_ARMOR"));
		} */ // i don't want to do this. I'd have to create RuleResearch's for each new armor!!!

		removePoppedResearch(resRule);

		if (score == true)
			addResearchScore(resRule->getPoints());
	}

	if (rules != NULL)
	{
		std::vector<RuleResearch*> availableResearch;
		for (std::vector<Base*>::const_iterator
				i = _bases.begin();
				i != _bases.end();
				++i)
		{
			getDependableResearchBasic(
									availableResearch,
									resRule,
									rules,
									*i);
		}

		for (std::vector<RuleResearch*>::const_iterator
				i = availableResearch.begin();
				i != availableResearch.end();
				++i)
		{
			if ((*i)->getCost() == 0
				&& (*i)->getRequirements().empty() == true)
			{
				addFinishedResearch(
								*i,
								rules);
			}
			else if ((*i)->getCost() == 0)
			{
				size_t entry (0); // init.

				for (std::vector<std::string>::const_iterator
						j = (*i)->getRequirements().begin();
						j != (*i)->getRequirements().end();
						++j)
				{
					if ((*i)->getRequirements().at(entry) == *j)
						addFinishedResearch(
										*i,
										rules);

					++entry;
				}
			}
		}
	}
}

/**
 * Gets the list of already discovered ResearchProjects.
 * @return, address of the vector of pointers of already discovered research projects
 */
const std::vector<const RuleResearch*>& SavedGame::getDiscoveredResearch() const
{
	return _discovered;
}

/**
 * Gets the list of RuleResearch which can be researched in a Base.
 * @param projects	- reference a vector of pointers to the ResearchProjects that are available
 * @param ruleset	- pointer to the game Ruleset
 * @param base		- pointer to a Base
 */
void SavedGame::getAvailableResearchProjects(
		std::vector<RuleResearch*>& projects,
		const Ruleset* ruleset,
		Base* base) const
{
	const std::vector<const RuleResearch*>& discovered (getDiscoveredResearch()); // init.
	const std::vector<std::string> researchProjects = ruleset->getResearchList();
	const std::vector<ResearchProject*>& baseResearchProjects = base->getResearch();
	std::vector<const RuleResearch*> unlocked;

	for (std::vector<const RuleResearch*>::const_iterator
			i = discovered.begin();
			i != discovered.end();
			++i)
	{
		for (std::vector<std::string>::const_iterator
				j = (*i)->getUnlocked().begin();
				j != (*i)->getUnlocked().end();
				++j)
		{
			unlocked.push_back(ruleset->getResearch(*j));
		}
	}

	for (std::vector<std::string>::const_iterator
			i = researchProjects.begin();
			i != researchProjects.end();
			++i)
	{
		RuleResearch* const resRule = ruleset->getResearch(*i);
		if (isResearchAvailable(
							resRule,
							unlocked,
							ruleset) == false)
		{
			continue;
		}

		std::vector<const RuleResearch*>::const_iterator resRules = std::find(
																			discovered.begin(),
																			discovered.end(),
																			resRule);
		const bool liveAlien = (ruleset->getUnit(resRule->getName()) != NULL);

		if (resRules != discovered.end())
		{
			bool cull = true;
			if (resRule->getGetOneFree().empty() == false)
			{
				for (std::vector<std::string>::const_iterator
						ohBoy = resRule->getGetOneFree().begin();
						ohBoy != resRule->getGetOneFree().end();
						++ohBoy)
				{
					const std::vector<const RuleResearch*>::const_iterator more_iteration = std::find(
																									discovered.begin(),
																									discovered.end(),
																									ruleset->getResearch(*ohBoy));
					if (more_iteration == discovered.end())
					{
						cull = false;
						break;
					}
				}
			}

			if (liveAlien == false
				&& cull == true)
			{
				continue;
			}
			else
			{
				const std::vector<std::string>::const_iterator
					leaderCheck = std::find(
										resRule->getUnlocked().begin(),
										resRule->getUnlocked().end(),
										"STR_LEADER_PLUS"),
					cmnderCheck = std::find(
										resRule->getUnlocked().begin(),
										resRule->getUnlocked().end(),
										"STR_COMMANDER_PLUS");

				const bool
					leader = (leaderCheck != resRule->getUnlocked().end()),
					cmnder = (cmnderCheck != resRule->getUnlocked().end());

				if (leader == true)
				{
					const std::vector<const RuleResearch*>::const_iterator j = std::find(
																					discovered.begin(),
																					discovered.end(),
																					ruleset->getResearch("STR_LEADER_PLUS"));
					if (j == discovered.end())
						cull = false;
				}

				if (cmnder == true)
				{
					const std::vector<const RuleResearch*>::const_iterator j = std::find(
																					discovered.begin(),
																					discovered.end(),
																					ruleset->getResearch("STR_COMMANDER_PLUS"));
					if (j == discovered.end())
						cull = false;
				}

				if (cull == true)
					continue;
			}
		}

		if (std::find_if(
					baseResearchProjects.begin(),
					baseResearchProjects.end(),
					findRuleResearch(resRule)) != baseResearchProjects.end())
		{
			continue;
		}

		if (resRule->needItem() == true
			&& base->getItems()->getItemQty(resRule->getName()) == 0)
		{
			continue;
		}

		if (resRule->getRequirements().empty() == true)
		{
			size_t tally (0); // init.
			for (size_t
					j = 0;
					j != resRule->getRequirements().size();
					++j)
			{
				resRules = std::find(
								discovered.begin(),
								discovered.end(),
								ruleset->getResearch(resRule->getRequirements().at(j)));
				if (resRules != discovered.end())
					++tally;
			}

			if (tally != resRule->getRequirements().size())
				continue;
		}

		projects.push_back(resRule);
	}
}

/**
 * Get the list of RuleManufacture which can be manufactured in a Base.
 * @param productions	- reference the list of Productions to be made available
 * @param ruleset		- pointer to the Ruleset
 * @param base			- pointer to a Base
 */
void SavedGame::getAvailableProductions(
		std::vector<RuleManufacture*>& productions,
		const Ruleset* ruleset,
		Base* base) const
{
	const std::vector<std::string>& items = ruleset->getManufactureList();
	const std::vector<Production*> baseProductions (base->getProductions()); // init.

	for (std::vector<std::string>::const_iterator
			i = items.begin();
			i != items.end();
			++i)
	{
		RuleManufacture* const manufRule = ruleset->getManufacture(*i);
		if (isResearched(manufRule->getRequirements()) == false)
			continue;

		if (std::find_if(
					baseProductions.begin(),
					baseProductions.end(),
					equalProduction(manufRule)) != baseProductions.end())
		{
			continue;
		}

		productions.push_back(manufRule);
	}
}

/**
 * Checks whether a ResearchProject can be researched.
 * @param resRule	- pointer to a RuleResearch to test
 * @param unlocked	- reference to a vector of pointers to the currently unlocked RuleResearch
 * @param ruleset	- pointer to the current Ruleset
 * @return, true if the RuleResearch can be researched
 */
bool SavedGame::isResearchAvailable(
		RuleResearch* resRule,
		const std::vector<const RuleResearch*>& unlocked,
		const Ruleset* ruleset) const
{
	if (resRule == NULL)
		return false;

	const std::vector<const RuleResearch*>& discovered (getDiscoveredResearch()); // init.

	const bool liveAlien = (ruleset->getUnit(resRule->getName()) != NULL);

	if (_debug == true
		|| std::find(
					unlocked.begin(),
					unlocked.end(),
					resRule) != unlocked.end())
	{
		return true;
	}
	else if (liveAlien == true)
	{
		if (resRule->getGetOneFree().empty() == false)
		{
			const std::vector<std::string>::const_iterator
				leaderCheck = std::find(
									resRule->getUnlocked().begin(),
									resRule->getUnlocked().end(),
									"STR_LEADER_PLUS"),
				cmnderCheck = std::find(
									resRule->getUnlocked().begin(),
									resRule->getUnlocked().end(),
									"STR_COMMANDER_PLUS");
			const bool
				leader = (leaderCheck != resRule->getUnlocked().end()),
				cmnder = (cmnderCheck != resRule->getUnlocked().end());

			if (leader == true)
			{
				const std::vector<const RuleResearch*>::const_iterator i = std::find(
																				discovered.begin(),
																				discovered.end(),
																				ruleset->getResearch("STR_LEADER_PLUS"));
				if (i == discovered.end())
					return true;
			}

			if (cmnder == true)
			{
				const std::vector<const RuleResearch*>::const_iterator i = std::find(
																				discovered.begin(),
																				discovered.end(),
																				ruleset->getResearch("STR_COMMANDER_PLUS"));
				if (i == discovered.end())
					return true;
			}
		}
	}

	for (std::vector<std::string>::const_iterator
			i = resRule->getGetOneFree().begin();
			i != resRule->getGetOneFree().end();
			++i)
	{
		if (std::find(
					unlocked.begin(),
					unlocked.end(),
					ruleset->getResearch(*i)) == unlocked.end())
		{
			return true;
		}
	}

	const std::vector<std::string> deps = resRule->getDependencies();
	for (std::vector<std::string>::const_iterator
			i = deps.begin();
			i != deps.end();
			++ i)
	{
		const RuleResearch* const research = ruleset->getResearch(*i);
		const std::vector<const RuleResearch*>::const_iterator j = std::find(
																		discovered.begin(),
																		discovered.end(),
																		research);
		if (j == discovered.end())
			return false;
	}

	return true;
}

/**
 * Gets a list of newly available research projects once a ResearchProject
 * has been completed. This function checks for fake ResearchProjects.
 * @param dependables	- reference to a vector of pointers to the RuleResearches that are now available
 * @param research		- pointer to the RuleResearch that has just been discovered
 * @param ruleset		- pointer to the game Ruleset
 * @param base			- pointer to a Base
 */
void SavedGame::getDependableResearch(
		std::vector<RuleResearch*>& dependables,
		const RuleResearch* research,
		const Ruleset* ruleset,
		Base* base) const
{
	getDependableResearchBasic(
							dependables,
							research,
							ruleset,
							base);

	for (std::vector<const RuleResearch*>::const_iterator
			i = _discovered.begin();
			i != _discovered.end();
			++i)
	{
		if ((*i)->getCost() == 0)
		{
			if (std::find(
						(*i)->getDependencies().begin(),
						(*i)->getDependencies().end(),
						research->getName()) != (*i)->getDependencies().end())
			{
				getDependableResearchBasic(
										dependables,
										*i,
										ruleset,
										base);
			}
		}
	}
}

/**
 * Gets the list of newly available research projects once a ResearchProject
 * has been completed. This function doesn't check for fake ResearchProjects.
 * @param dependables	- reference to a vector of pointers to the RuleResearches that are now available
 * @param research		- pointer to the RuleResearch that has just been discovered
 * @param ruleset		- pointer to the game Ruleset
 * @param base			- pointer to a Base
 */
void SavedGame::getDependableResearchBasic( // private.
		std::vector<RuleResearch*>& dependables,
		const RuleResearch* research,
		const Ruleset* ruleset,
		Base* base) const
{
	std::vector<RuleResearch*> possible;
	getAvailableResearchProjects(
							possible,
							ruleset,
							base);

	for (std::vector<RuleResearch*>::const_iterator
			i = possible.begin();
			i != possible.end();
			++i)
	{
		if (std::find(
					(*i)->getDependencies().begin(),
					(*i)->getDependencies().end(),
					research->getName()) != (*i)->getDependencies().end()
			|| std::find(
					(*i)->getUnlocked().begin(),
					(*i)->getUnlocked().end(),
					research->getName()) != (*i)->getUnlocked().end())
		{
			dependables.push_back(*i);

			if ((*i)->getCost() == 0)
				getDependableResearchBasic(
										dependables,
										*i,
										ruleset,
										base);
		}
	}
}

/**
 * Gets the list of newly available manufacture projects once a ResearchProject
 * has been completed. This function checks for fake ResearchProjects.
 * @param dependables	- reference to a vector of pointers to the RuleResearches that are now available
 * @param research		- pointer to the RuleResearch that has just been discovered
 * @param ruleset		- pointer to the game Ruleset
 * @param base			- pointer to a Base
 */
void SavedGame::getDependableManufacture(
		std::vector<RuleManufacture*>& dependables,
		const RuleResearch* research,
		const Ruleset* ruleset,
		Base*) const
{
	const std::vector<std::string>& manuList = ruleset->getManufactureList();
	for (std::vector<std::string>::const_iterator
			i = manuList.begin();
			i != manuList.end();
			++i)
	{
		RuleManufacture* const manufRule = ruleset->getManufacture(*i);

		const std::vector<std::string>& reqs = manufRule->getRequirements();
		if (isResearched(manufRule->getRequirements()) == true
			&& std::find(
						reqs.begin(),
						reqs.end(),
						research->getName()) != reqs.end())
		{
			dependables.push_back(manufRule);
		}
	}
}

/**
 * Returns if a certain research has been completed.
 * @param research - reference a research ID
 * @return, true if research has been researched
 */
bool SavedGame::isResearched(const std::string& research) const
{
	if (_debug == true
		|| research.empty() == true
		|| _rules->getResearch(research) == NULL	// kL_add
		|| (_rules->getItem(research) != NULL		// kL_add->
			&& _rules->getItem(research)->isResearchExempt() == true))
	{
		return true;
	}

	for (std::vector<const RuleResearch*>::const_iterator
			i = _discovered.begin();
			i != _discovered.end();
			++i)
	{
		if ((*i)->getName() == research)
			return true;
	}

	return false;
}

/**
 * Returns if a certain list of research has been completed.
 * @param research - reference a vector of strings of research IDs
 * @return, true if research has been researched
 */
bool SavedGame::isResearched(const std::vector<std::string>& research) const
{
	if (_debug == true
		|| research.empty() == true)
	{
		return true;
	}

	std::vector<std::string> matches = research;

	for (std::vector<const RuleResearch*>::const_iterator
			i = _discovered.begin();
			i != _discovered.end();
			++i)
	{
		for (std::vector<std::string>::const_iterator
				j = matches.begin();
				j != matches.end();
				++j)
		{
			if ((*i)->getName() == *j)
			{
				j = matches.erase(j);
				break;
			}
		}

		if (matches.empty() == true)
			return true;
	}

	return false;
}

/**
 * Returns pointer to the Soldier given its unique ID.
 * @param id - a soldier's unique id
 * @return, pointer to Soldier
 */
Soldier* SavedGame::getSoldier(int id) const
{
	for (std::vector<Base*>::const_iterator
			i = _bases.begin();
			i != _bases.end();
			++i)
	{
		for (std::vector<Soldier*>::const_iterator
				j = (*i)->getSoldiers()->begin();
				j != (*i)->getSoldiers()->end();
				++j)
		{
			if ((*j)->getId() == id)
				return *j;
		}
	}

	return NULL;
}

/**
 * Handles the higher promotions - not the rookie-squaddie ones.
 * @param participants - a list of soldiers that were actually present at the battle
 * @return, true if some promotions happened - to show the promotions screen
 */
bool SavedGame::handlePromotions(std::vector<Soldier*>& participants)
{
	PromotionInfo data;
	std::vector<Soldier*> soldiers;

	for (std::vector<Base*>::const_iterator
			i = _bases.begin();
			i != _bases.end();
			++i)
	{
		for (std::vector<Soldier*>::const_iterator
				j = (*i)->getSoldiers()->begin();
				j != (*i)->getSoldiers()->end();
				++j)
		{
			soldiers.push_back(*j);
			processSoldier(
						*j,
						data);
		}

		for (std::vector<Transfer*>::const_iterator
				j = (*i)->getTransfers()->begin();
				j != (*i)->getTransfers()->end();
				++j)
		{
			if ((*j)->getType() == TRANSFER_SOLDIER)
			{
				soldiers.push_back((*j)->getSoldier());
				processSoldier(
							(*j)->getSoldier(),
							data);
			}
		}
	}


	int pro = 0;

	Soldier* fragBait = NULL;
	const int totalSoldiers = static_cast<int>(soldiers.size());

	if (data.totalCommanders == 0 // There can be only one.
		&& totalSoldiers > 29)
	{
		fragBait = inspectSoldiers(
								soldiers,
								participants,
								RANK_COLONEL);
		if (fragBait != NULL)
		{
			fragBait->promoteRank();
			++pro;
			++data.totalCommanders;
			--data.totalColonels;
		}
	}

	while (data.totalColonels < totalSoldiers / 23)
	{
		fragBait = inspectSoldiers(
								soldiers,
								participants,
								RANK_CAPTAIN);
		if (fragBait == NULL)
			break;

		fragBait->promoteRank();
		++pro;
		++data.totalColonels;
		--data.totalCaptains;
	}

	while (data.totalCaptains < totalSoldiers / 11)
	{
		fragBait = inspectSoldiers(
								soldiers,
								participants,
								RANK_SERGEANT);
		if (fragBait == NULL)
			break;

		fragBait->promoteRank();
		++pro;
		++data.totalCaptains;
		--data.totalSergeants;
	}

	while (data.totalSergeants < totalSoldiers / 5)
	{
		fragBait = inspectSoldiers(
								soldiers,
								participants,
								RANK_SQUADDIE);
		if (fragBait == NULL)
			break;

		fragBait->promoteRank();
		++pro;
		++data.totalSergeants;
	}

	return (pro != 0);
}

/**
 * Processes a soldier and adds their rank to the promotions data struct.
 * @param soldier	- pointer to the Soldier to process
 * @param promoData	- reference the PromotionInfo data struct to put the info into
 */
void SavedGame::processSoldier(
		Soldier* soldier,
		PromotionInfo& promoData)
{
	switch (soldier->getRank())
	{
		case RANK_COMMANDER:
			++promoData.totalCommanders;
		break;

		case RANK_COLONEL:
			++promoData.totalColonels;
		break;

		case RANK_CAPTAIN:
			++promoData.totalCaptains;
		break;

		case RANK_SERGEANT:
			++promoData.totalSergeants;
	}
}

/**
 * Checks how many soldiers of a rank exist and which one has the highest score.
 * @param soldiers		- reference a vector of pointers to Soldiers for full list of live soldiers
 * @param participants	- reference a vector of pointers to Soldiers for list of participants on a mission
 * @param soldierRank	- rank to inspect
 * @return, pointer to the highest ranked soldier
 */
Soldier* SavedGame::inspectSoldiers(
		std::vector<Soldier*>& soldiers,
		std::vector<Soldier*>& participants,
		SoldierRank soldierRank)
{
	Soldier* fragBait = NULL;
	int
		highScore = 0,
		testScore;

	for (std::vector<Soldier*>::const_iterator
			i = soldiers.begin();
			i != soldiers.end();
			++i)
	{
		if ((*i)->getRank() == soldierRank)
		{
			testScore = getSoldierScore(*i);
			if (testScore > highScore
				&& (Options::fieldPromotions == false
					|| std::find(
							participants.begin(),
							participants.end(),
							*i) != participants.end()))
			{
				highScore = testScore;
				fragBait = *i;
			}
		}
	}

	return fragBait;
}

/**
 * Evaluate the score of a soldier based on all of his stats, missions and kills.
 * @param soldier - pointer to the soldier to get a score for
 * @return, the soldier's score
 */
int SavedGame::getSoldierScore(Soldier* soldier)
{
	const UnitStats* const stats = soldier->getCurrentStats();

	int score = stats->health * 2
			  + stats->stamina * 2
			  + stats->reactions * 4
			  + stats->bravery * 4
			  + (stats->tu + stats->firing * 2) * 3
			  + stats->melee
			  + stats->throwing
			  + stats->strength
			  + stats->psiStrength * 4	// kL_add. Include these even if not yet revealed.
			  + stats->psiSkill * 2;	// kL_add.

//	if (stats->psiSkill > 0)
//		score += stats->psiStrength
//			   + stats->psiSkill * 2;

	score += (soldier->getMissions() + soldier->getKills()) * 10;

	return score;
}

/**
 * Returns a list of the AlienBases.
 * @return, address of a vector of pointers to AlienBases
 */
std::vector<AlienBase*>* SavedGame::getAlienBases()
{
	return &_alienBases;
}

/**
 * Toggles debug mode.
 */
void SavedGame::setDebugMode()
{
	_debug = !_debug;
}

/**
 * Gets the current debug mode.
 * @return, true if debug mode
 */
bool SavedGame::getDebugMode() const
{
	return _debug;
}


/**
 * @brief Match a mission based on region and type.
 * This function object will match alien missions based on region and type.
 */
class matchRegionAndType
	:
		public std::unary_function<AlienMission*, bool>
{

private:
	const std::string& _region;
	MissionObjective _objective;


	public:
		/// Store the region and type.
		matchRegionAndType(
				const std::string& region,
				MissionObjective objective)
			:
				_region(region),
				_objective(objective)
		{}

		/// Match against stored values.
		bool operator() (const AlienMission* mission) const
		{
			return mission->getRegion() == _region
				&& mission->getRules().getObjective() == _objective;
		}
};

/**
 * Find a mission type in the active alien missions.
 * @param region	- the region's string ID
 * @param objective	- the active mission's objective
 * @return, pointer to the AlienMission (NULL if no mission matched)
 */
AlienMission* SavedGame::findAlienMission(
		const std::string& region,
		MissionObjective objective) const
{
	std::vector<AlienMission*>::const_iterator
			mission = std::find_if(
								_activeMissions.begin(),
								_activeMissions.end(),
								matchRegionAndType(
												region,
												objective));
	if (mission == _activeMissions.end())
		return NULL;

	return *mission;
}

/**
 * Adds to this month's research score
 * @param score - the amount to add
 */
void SavedGame::addResearchScore(int score)
{
	_researchScores.back() += score;
}

/**
 * Returns the list of research scores
 * @return, list of research scores
 */
std::vector<int>& SavedGame::getResearchScores()
{
	return _researchScores;
}

/**
 * Returns if the player has been warned about poor performance.
 * @return, true if warned
 */
bool SavedGame::getWarned() const
{
	return _warned;
}

/**
 * Sets the player's warned status.
 * @param warned - true if warned
 */
void SavedGame::setWarned(bool warned)
{
	_warned = warned;
}


/**
 * Checks if a point is contained in a region.
 * This function object checks if a point is contained inside a region.
 */
class ContainsPoint: public std::unary_function<const Region*, bool>
{
private:
	double
		_lon,
		_lat;

	public:
		/// Remember the coordinates.
		ContainsPoint(
				double lon,
				double lat)
			:
				_lon(lon),
				_lat(lat)
		{}

		/// Check is the region contains the stored point.
		bool operator()(const Region* region) const
		{
			return region->getRules()->insideRegion(
												_lon,
												_lat);
		}
};

/**
 * Finds the region containing this location.
 * @param lon - the longtitude
 * @param lat - the latitude
 * @return, pointer to the Region or NULL
 */
Region* SavedGame::locateRegion(
		double lon,
		double lat) const
{
	const std::vector<Region*>::const_iterator found = std::find_if(
																_regions.begin(),
																_regions.end(),
																ContainsPoint(
																			lon,
																			lat));
	if (found != _regions.end())
		return *found;

	return NULL;
}

/**
 * Find the region containing this target.
 * @param target - the target to locate
 * @return, pointer to the Region or NULL
 */
Region* SavedGame::locateRegion(const Target& target) const
{
	return locateRegion(
					target.getLongitude(),
					target.getLatitude());
}

/**
 * @return, the month counter
 */
int SavedGame::getMonthsPassed() const
{
	return _monthsPassed;
}

/**
 * @return, address of the GraphRegionToggles
 */
const std::string& SavedGame::getGraphRegionToggles() const
{
	return _graphRegionToggles;
}

/**
 * @return, address of the GraphCountryToggles
 */
const std::string& SavedGame::getGraphCountryToggles() const
{
	return _graphCountryToggles;
}

/**
 * @return, address of the GraphFinanceToggles
 */
const std::string& SavedGame::getGraphFinanceToggles() const
{
	return _graphFinanceToggles;
}

/**
 * Sets the GraphRegionToggles.
 * @param value - reference the new value for GraphRegionToggles
 */
void SavedGame::setGraphRegionToggles(const std::string& value)
{
	_graphRegionToggles = value;
}

/**
 * Sets the GraphCountryToggles.
 * @param value - reference the new value for GraphCountryToggles
 */
void SavedGame::setGraphCountryToggles(const std::string& value)
{
	_graphCountryToggles = value;
}

/**
 * Sets the GraphFinanceToggles.
 * @param value - reference the new value for GraphFinanceToggles
 */
void SavedGame::setGraphFinanceToggles(const std::string& value)
{
	_graphFinanceToggles = value;
}

/**
 * Sets the current Graph page.
 * @param page - current page shown by Graphs
 */
void SavedGame::setCurrentGraph(int page)
{
	_curGraph = page;
}

/**
 * Gets the current Graph page.
 * @return, current page to show in Graphs
 */
int SavedGame::getCurrentGraph() const
{
	return _curGraph;
}

/**
 * Increments the month counter.
 */
void SavedGame::addMonth()
{
	++_monthsPassed;
}

/**
 * Toggles the state of the radar line drawing.
 */
/* void SavedGame::toggleRadarLines()
{
	_radarLines = !_radarLines;
} */

/**
 * @return, the state of the radar line drawing.
 */
/* bool SavedGame::getRadarLines()
{
	return _radarLines;
} */

/**
 * Toggles the state of the detail drawing.
 */
/* void SavedGame::toggleDetail()
{
	_detail = !_detail;
} */

/**
 * @return, the state of the detail drawing.
 */
/* bool SavedGame::getDetail()
{
	return _detail;
} */

/**
 * Marks a research topic as having already come up as "we can now research".
 * @param research - pointer to the project to add
 */
void SavedGame::addPoppedResearch(const RuleResearch* research)
{
	if (wasResearchPopped(research) == false)
		_poppedResearch.push_back(research);
}

/**
 * Checks if an unresearched topic has previously been popped up.
 * @param research - pointer to the project to check for
 * @return, true if it has popped up before
 */
bool SavedGame::wasResearchPopped(const RuleResearch* research)
{
	return std::find(
				_poppedResearch.begin(),
				_poppedResearch.end(),
				research) != _poppedResearch.end();
}

/**
 * Checks for and removes a research project from the "has been popped up" array.
 * @param research - pointer to the project to check for and remove if necessary
 */
void SavedGame::removePoppedResearch(const RuleResearch* research)
{
	std::vector<const RuleResearch*>::const_iterator r = std::find(
															_poppedResearch.begin(),
															_poppedResearch.end(),
															research);
	if (r != _poppedResearch.end())
		_poppedResearch.erase(r);
}

/**
 * Returns the list of dead soldiers.
 * @return, pointer to a vector of pointers to SoldierDead.
 */
std::vector<SoldierDead*>* SavedGame::getDeadSoldiers()
{
	return &_deadSoldiers;
}

/**
 * Returns the last selected player base.
 * @return, pointer to base
 */
/* Base* SavedGame::getSelectedBase()
{
	// in case a base was destroyed or something...
	if (_selectedBase < _bases.size())
		return _bases.at(_selectedBase);
	else
		return _bases.front();
} */

/**
 * Sets the last selected player base.
 * @param base - # of the base
 */
/* void SavedGame::setSelectedBase(size_t base)
{
	_selectedBase = base;
} */

/**
 * Sets the last selected armour.
 * @param value - the new value for last selected armor - Armor type string
 */
/* void SavedGame::setLastSelectedArmor(const std::string& value)
{
	_lastselectedArmor = value;
} */

/**
 * Gets the last selected armour.
 * @return, last used armor type string
 */
/* std::string SavedGame::getLastSelectedArmor()
{
	return _lastselectedArmor;
} */

/**
 * Sets the current matrix row.
 * @param row - current matrix row
 */
void SavedGame::setCurrentRowMatrix(size_t row)
{
	_curRowMatrix = row;
}

/**
 * Gets the current matrix row.
 * @return, current matrix row
 */
size_t SavedGame::getCurrentRowMatrix() const
{
	return _curRowMatrix;
}

/**
 * Sets a debug argument from Globe for GeoscapeState.
 * @param arg - debug string
 */
void SavedGame::setDebugArg(const std::string& debug)
{
	_debugArgDone = true;
	_debugArg = debug;
}

/**
 * Gets a debug argument from Globe for GeoscapeState.
 * @return, debug string
 */
std::string SavedGame::getDebugArg() const
{
	return _debugArg;
}

/**
 * Gets if the debug argument has just been set - and resets flag if true.
 * @return, true if debug arg is ready for display
 */
bool SavedGame::getDebugArgDone()
{
	const bool ret = _debugArgDone;

	if (_debugArgDone == true)
		_debugArgDone = false;

	return ret;
}

/**
 * Gets mission statistics for soldier commendations.
 * @return, pointer to a vector of pointers to MissionStatistics
 */
std::vector<MissionStatistics*>* SavedGame::getMissionStatistics()
{
	return &_missionStatistics;
}

/**
 * Returns the craft corresponding to the specified unique id.
 * @param craftId - the unique craft id to look up
 * @return, the craft with the specified id, or NULL
 */
/* Craft* SavedGame::findCraftByUniqueId(const CraftId& craftId) const
{
	for (std::vector<Base*>::const_iterator
			base = _bases.begin();
			base != _bases.end();
			++base)
	{
		for (std::vector<Craft*>::const_iterator
				craft = (*base)->getCrafts()->begin();
				craft != (*base)->getCrafts()->end();
				++craft)
		{
			if ((*craft)->getUniqueId() == craftId)
				return *craft;
		}
	}
	return NULL;
} */

}
