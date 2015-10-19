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
//#include "../Engine/Options.h"
//#include "../Engine/RNG.h"

#include "../Ruleset/RuleCountry.h"
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


/// *** FUNCTOR ***
struct findRuleResearch
	:
		public std::unary_function<ResearchProject*, bool>
{
	const RuleResearch* _resRule;
	explicit findRuleResearch(const RuleResearch* const resRule);

	bool operator() (const ResearchProject* const r) const;
};
///
findRuleResearch::findRuleResearch(const RuleResearch* const resRule)
	:
		_resRule(resRule)
{}
///
bool findRuleResearch::operator() (const ResearchProject* const r) const
{
	return (_resRule == r->getRules());
}


/// *** FUNCTOR ***
struct equalProduction
	:
		public std::unary_function<Production*, bool>
{
	const RuleManufacture* _manufRule;
	explicit equalProduction(const RuleManufacture* const manufRule);

	bool operator() (const Production* const p) const;
};
///
equalProduction::equalProduction(const RuleManufacture* const manufRule)
	:
		_manufRule(manufRule)
{}
///
bool equalProduction::operator() (const Production* const p) const
{
	return (p->getRules() == _manufRule);
}


/**
 * Initializes a brand new saved game according to the specified difficulty.
 * @param rules - pointer to the Ruleset
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
		_battleSave(NULL),
		_debug(false),
		_warned(false),
		_monthsPassed(-1),
		_debugArgDone(false)
//		_detail(true),
//		_radarLines(false),
//		_selectedBase(0),
//		_lastselectedArmor("STR_ARMOR_NONE_UC")
{
//	_time = new GameTime(6,1,1,1999,12,0,0);
	_time = new GameTime(1,1,1999,12,0,0);

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

	delete _battleSave;
}

/**
 * Gets all the info of the saves found in the user folder.
 * @param lang		- pointer to the loaded Language
 * @param autoquick	- true to include autosaves and quicksaves
 * @return, vector of SavesInfo structs (SavedGame.h)
 */
std::vector<SaveInfo> SavedGame::getList( // static.
		Language* lang,
		bool autoquick)
{
	std::vector<SaveInfo> info;

	if (autoquick == true)
	{
		const std::vector<std::string> saves (CrossPlatform::getFolderContents(
																			Options::getUserFolder(),
																			"asav"));
		for (std::vector<std::string>::const_iterator
				i = saves.begin();
				i != saves.end();
				++i)
		{
			try
			{
				info.push_back(getSaveInfo(*i, lang));
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

	const std::vector<std::string> saves (CrossPlatform::getFolderContents(
																		Options::getUserFolder(),
																		"sav"));
	for (std::vector<std::string>::const_iterator
			i = saves.begin();
			i != saves.end();
			++i)
	{
		try
		{
			info.push_back(getSaveInfo(*i, lang));
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
 * @param file - reference a save by filename
 * @param lang - pointer to the loaded Language
 * @return, the SaveInfo (SavedGame.h)
 */
SaveInfo SavedGame::getSaveInfo( // private/static.
		const std::string& file,
		const Language* const lang)
{
	const std::string path = Options::getUserFolder() + file;
	const YAML::Node doc = YAML::LoadFile(path);

	SaveInfo save;
	save.file = file;

	if (save.file == QUICKSAVE)
	{
		save.label = lang->getString("STR_QUICK_SAVE_SLOT");
		save.reserved = true;
	}
	else if (save.file == AUTOSAVE_GEOSCAPE)
	{
		save.label = lang->getString("STR_AUTO_SAVE_GEOSCAPE_SLOT");
		save.reserved = true;
	}
	else if (save.file == AUTOSAVE_BATTLESCAPE)
	{
		save.label = lang->getString("STR_AUTO_SAVE_BATTLESCAPE_SLOT");
		save.reserved = true;
	}
	else
	{
		if (doc["name"])
			save.label = Language::utf8ToWstr(doc["name"].as<std::string>());
		else
			save.label = Language::fsToWstr(CrossPlatform::noExt(file));

		save.reserved = false;
	}

	save.timestamp = CrossPlatform::getDateModified(path);
	const std::pair<std::wstring, std::wstring> timePair = CrossPlatform::timeToString(save.timestamp);
	save.isoDate = timePair.first;
	save.isoTime = timePair.second;

	std::wostringstream details;
	if (doc["base"])
	{
		details << Language::utf8ToWstr(doc["base"].as<std::string>())
				<< L" - ";
	}

	GameTime gt = GameTime(1,1,1999,12,0,0);
	gt.load(doc["time"]);
	details << gt.getDayString(lang)
			<< L" "
			<< lang->getString(gt.getMonthString())
			<< L" "
			<< gt.getYear()
			<< L" "
			<< gt.getHour()
			<< L":"
			<< std::setfill(L'0')
			<< std::setw(2)
			<< gt.getMinute();

	if (doc["turn"])
	{
		details << L" - "
				<< lang->getString(doc["mission"].as<std::string>())
				<< L" "
				<< lang->getString("STR_TURN").arg(doc["turn"].as<int>());
	}

	if (doc["ironman"].as<bool>(false))
		details << L" "
				<< lang->getString("STR_IRONMAN");

	save.details = details.str();

	if (doc["rulesets"])
		save.rulesets = doc["rulesets"].as<std::vector<std::string> >();

	save.mode = static_cast<GameMode>(doc["mode"].as<int>());

	return save;
}

/**
 * Loads a saved game's contents from a YAML file.
 * @note Assumes the saved game is blank.
 * @param file	- reference a YAML file
 * @param rules	- pointer to Ruleset
 */
void SavedGame::load(
		const std::string& file,
		Ruleset* const rules) // <- used only to obviate const if loading battleSave.
{
	//Log(LOG_INFO) << "SavedGame::load()";
	const std::string st = Options::getUserFolder() + file;
	const std::vector<YAML::Node> nodes = YAML::LoadAllFromFile(st);
	if (nodes.empty() == true)
	{
		throw Exception(file + " is not a valid save file");
	}

	YAML::Node brief (nodes[0]); // Get brief save info

/*	std::string version = brief["version"].as<std::string>();
	if (version != OPENXCOM_VERSION_SHORT)
	{
		throw Exception("Version mismatch");
	} */

	_time->load(brief["time"]);
	if (brief["name"])
		_name = Language::utf8ToWstr(brief["name"].as<std::string>());
	else
		_name = Language::fsToWstr(file);

	YAML::Node doc = nodes[1]; // Get full save data

	if (doc["rng"]
		&& (Options::reSeedOnLoad == false || _ironman == true))
	{
		RNG::setSeed(doc["rng"].as<uint64_t>());
	}
	else
		RNG::setSeed(0);

	int diff = doc["difficulty"].as<int>(_difficulty);
	if (diff < 0) // safety.
	{
		diff = 0;
		Log(LOG_WARNING) << "Difficulty in the save file is negative ... loading as BEGINNER.";
	}
	_difficulty = static_cast<GameDifficulty>(diff);

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
		const std::string type ((*i)["type"].as<std::string>());
		if (_rules->getCountry(type))
		{
			Country* const c (new Country(_rules->getCountry(type)));
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
		const std::string type ((*i)["type"].as<std::string>());
		if (_rules->getRegion(type))
		{
			Region* const r (new Region(_rules->getRegion(type)));
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
		AlienBase* const b (new AlienBase());
		b->load(*i);
		_alienBases.push_back(b);
	}

	Log(LOG_INFO) << ". load missions";
	// Missions must be loaded before UFOs.
	const YAML::Node& missions (doc["alienMissions"]);
	for (YAML::const_iterator
			i = missions.begin();
			i != missions.end();
			++i)
	{
		const std::string missionType ((*i)["type"].as<std::string>());
		const RuleAlienMission& missionRule (*_rules->getAlienMission(missionType));
		std::auto_ptr<AlienMission> mission (new AlienMission(missionRule, *this));
		mission->load(*i);
		_activeMissions.push_back(mission.release());
	}

	Log(LOG_INFO) << ". load ufos";
	for (YAML::const_iterator
			i = doc["ufos"].begin();
			i != doc["ufos"].end();
			++i)
	{
		const std::string type ((*i)["type"].as<std::string>());
		if (_rules->getUfo(type))
		{
			Ufo* const u (new Ufo(_rules->getUfo(type)));
			u->load(*i, *_rules, *this);
			_ufos.push_back(u);
		}
	}

	Log(LOG_INFO) << ". load waypoints";
	for (YAML::const_iterator
			i = doc["waypoints"].begin();
			i != doc["waypoints"].end();
			++i)
	{
		Waypoint* const w (new Waypoint());
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
			type ((*i)["type"].as<std::string>()),
			deployment ((*i)["deployment"].as<std::string>("STR_TERROR_MISSION"));
		MissionSite* const ms (new MissionSite(
											_rules->getAlienMission(type),
											_rules->getDeployment(deployment)));
		ms->load(*i);
		_missionSites.push_back(ms);
	}

	Log(LOG_INFO) << ". load discovered research";
	// Discovered Techs should be loaded before Bases (e.g. for PSI evaluation)
	for (YAML::const_iterator
			i = doc["discovered"].begin();
			i != doc["discovered"].end();
			++i)
	{
		const std::string research (i->as<std::string>());
		if (_rules->getResearch(research) != NULL)
			_discovered.push_back(_rules->getResearch(research));
	}

	Log(LOG_INFO) << ". load xcom bases";
	for (YAML::const_iterator
			i = doc["bases"].begin();
			i != doc["bases"].end();
			++i)
	{
		Base* const b (new Base(_rules));
		b->load(*i, this, false);
		_bases.push_back(b);
	}

	Log(LOG_INFO) << ". load popped research";
	for (YAML::const_iterator
			i = doc["poppedResearch"].begin();
			i != doc["poppedResearch"].end();
			++i)
	{
		const std::string research (i->as<std::string>());
		if (_rules->getResearch(research) != NULL)
			_poppedResearch.push_back(_rules->getResearch(research));
	}

	Log(LOG_INFO) << ". load alien strategy";
	_alienStrategy->load(doc["alienStrategy"]);

	Log(LOG_INFO) << ". load dead soldiers";
	for (YAML::const_iterator
			i = doc["deadSoldiers"].begin();
			i != doc["deadSoldiers"].end();
			++i)
	{
		SoldierDead* const solDead (new SoldierDead(
												L"", 0,
												RANK_ROOKIE,
												GENDER_MALE,
												LOOK_BLONDE,
												0,0, NULL,
												UnitStats(),
												UnitStats()));
		solDead->load(*i);
		_deadSoldiers.push_back(solDead);
	}

	Log(LOG_INFO) << ". load mission statistics";
	for (YAML::const_iterator
			i = doc["missionStatistics"].begin();
			i != doc["missionStatistics"].end();
			++i)
	{
		MissionStatistics* const ms (new MissionStatistics());
		ms->load(*i);
		_missionStatistics.push_back(ms);
	}

	if (const YAML::Node& battle = doc["battleGame"])
	{
		Log(LOG_INFO) << "SavedGame: loading battlegame";
		_battleSave = new SavedBattleGame();
		_battleSave->load(battle, rules, this);
		Log(LOG_INFO) << "SavedGame: loading battlegame DONE";
	}
}

/**
 * Saves a saved game's contents to a YAML file.
 * @param file - reference to a YAML file
 */
void SavedGame::save(const std::string& file) const
{
	const std::string st (Options::getUserFolder() + file);
	std::ofstream ofstr (st.c_str()); // init.
	if (ofstr.fail() == true)
	{
		throw Exception("Failed to save " + file);
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

	if (_battleSave != NULL)
	{
		brief["mission"]	= _battleSave->getTacticalType();
		brief["turn"]		= _battleSave->getTurn();
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
		node["discovered"].push_back((*i)->getType());
	}

	for (std::vector<const RuleResearch*>::const_iterator
			i = _poppedResearch.begin();
			i != _poppedResearch.end();
			++i)
	{
		node["poppedResearch"].push_back((*i)->getType());
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

	if (_battleSave != NULL)
		node["battleGame"] = _battleSave->save();

	emit << node;
	ofstr << emit.c_str();
	ofstr.close();
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
	delete _time;
	_time = new GameTime(gt);
}

/**
 * Returns the highest ID for the specified object and increases it.
 * @param objectType - reference an object string
 * @return, highest ID number
 */
int SavedGame::getCanonicalId(const std::string& objectType)
{
	std::map<std::string, int>::iterator i = _ids.find(objectType);
	if (i != _ids.end())
		return i->second++;

	_ids[objectType] = 1;
	return _ids[objectType]++;
}

/*
 * Resets the list of unique object IDs.
 * @param ids - new ID list as a reference to a map of strings & ints
 *
void SavedGame::setCanonicalIds(const std::map<std::string, int>& ids)
{
	_ids = ids;
} */

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
SavedBattleGame* SavedGame::getBattleSave()
{
	return _battleSave;
}

/**
 * Set SavedBattleGame object.
 * @param battleSave - pointer to a new SavedBattleGame object
 */
void SavedGame::setBattleSave(SavedBattleGame* const battleSave)
{
	delete _battleSave;
	_battleSave = battleSave;
}

/**
 * Gets the list of already discovered RuleResearch's.
 * @return, reference to the vector of pointers listing discovered RuleResearch's
 */
const std::vector<const RuleResearch*>& SavedGame::getDiscoveredResearch() const
{
	return _discovered;
}

/**
 * Adds a RuleResearch to the list of already discovered RuleResearch's.
 * @param resRule	- the newly found RuleResearch
 * @param score		- true to score points and add dependents for research done;
 * false for skirmish battles (default true)
 */
void SavedGame::addFinishedResearch(
		const RuleResearch* const resRule,
		bool score)
{
	if (std::find(
			_discovered.begin(),
			_discovered.end(),
			resRule) == _discovered.end())
	{
		_discovered.push_back(resRule);
		removePoppedResearch(resRule);

		if (score == true) addResearchScore(resRule->getPoints());
	}

	if (score == true)
	{
		std::vector<const RuleResearch*> dependents;
		for (std::vector<Base*>::const_iterator
				i = _bases.begin();
				i != _bases.end();
				++i)
		{
			getDependentResearchBasic(dependents, resRule, *i);
		}

		const std::vector<std::string>* prereqs;
		for (std::vector<const RuleResearch*>::const_iterator
				i = dependents.begin();
				i != dependents.end();
				++i)
		{
			if ((*i)->getCost() == 0) // fake project.
			{
				prereqs = &((*i)->getPrerequisites()); // gawd i hate c++
				if (prereqs->empty() == true)
					addFinishedResearch(*i);
				else
				{
					size_t id (0);
					for (std::vector<std::string>::const_iterator
							j = prereqs->begin();
							j != prereqs->end();
							++j, ++id)
					{
						if (prereqs->at(id) == *j) // wtf this.
							addFinishedResearch(*i);
					}
				}
			}
		}
	}
}

/**
 * Gets a list of RuleResearch's that can be started at a particular Base.
 * @note Used in basescape's 'start project' as well as (indirectly) in
 * geoscape's 'we can now research'.
 * @param availableProjects	- reference to a vector of pointers to RuleResearch
 * in which to put the projects that are currently available
 * @param base				- pointer to a Base
 */
void SavedGame::getAvailableResearchProjects(
		std::vector<const RuleResearch*>& availableProjects,
		Base* const base) const
{
	const RuleResearch* resRule;
	bool cullProject;

	const std::vector<std::string> researchList (_rules->getResearchList());
	for (std::vector<std::string>::const_iterator
			i = researchList.begin();
			i != researchList.end();
			++i)
	{
		resRule = _rules->getResearch(*i);
		if (isResearchAvailable(resRule) == true)											// <- resRule is tested there first.
		{
			if (_rules->getUnit(resRule->getType()) == NULL									// kL_add. Always allow aLiens to be researched & re-researched.
				&& std::find(
						_discovered.begin(),
						_discovered.end(),
						resRule) != _discovered.end())										// if resRule is already discovered ->
			{
				cullProject = true;
				for (std::vector<std::string>::const_iterator
						j = resRule->getGetOneFree().begin();
						j != resRule->getGetOneFree().end();
						++j)
				{
					if (std::find(
							_discovered.begin(),
							_discovered.end(),
							_rules->getResearch(*j)) == _discovered.end())
					{
						cullProject = false;												// resRule's getOneFree still undiscovered.
						break;
					}
				}

				if ((std::find(
							_discovered.begin(),
							_discovered.end(),
							_rules->getResearch("STR_LEADER_PLUS")) == _discovered.end()	// player still needs LeaderPlus
						&& std::find(
								resRule->getUnlocks().begin(),
								resRule->getUnlocks().end(),
								"STR_LEADER_PLUS") != resRule->getUnlocks().end())			// and the resRule can unlock LeaderPlus
					|| (std::find(
							_discovered.begin(),
							_discovered.end(),
							_rules->getResearch("STR_COMMANDER_PLUS")) == _discovered.end()	// player still needs CommanderPlus
						&& std::find(
								resRule->getUnlocks().begin(),
								resRule->getUnlocks().end(),
								"STR_COMMANDER_PLUS") != resRule->getUnlocks().end()))		// and the resRule can unlock CommanderPlus
				{
					cullProject = false;													// do NOT cull
				}

				if (cullProject == true) continue;
			}

			if (std::find_if(
						base->getResearch().begin(),
						base->getResearch().end(),
						findRuleResearch(resRule)) == base->getResearch().end() // if not already a ResearchProject @ Base ->
				&& (resRule->needsItem() == false
					|| base->getStorageItems()->getItemQty(resRule->getType()) != 0))
			{
				const std::vector<std::string>* const prereqs (&(resRule->getPrerequisites()));
				size_t
					tally (0),
					reqSize (prereqs->size());
				for (size_t
						j = 0;
						j != reqSize;
						++j)
				{
					if (std::find(
							_discovered.begin(),
							_discovered.end(),
							_rules->getResearch(prereqs->at(j))) != _discovered.end())
					{
						++tally;
					}
				}

				if (tally == reqSize)
					availableProjects.push_back(resRule);
			}
		}
	}
}

/**
 * Checks whether a RuleResearch can be started as a project.
 * @note If it's unlocked it can be started. If it's not unlocked but has a
 * getOneFree it's also considered available; if it's not unlocked and does not
 * have a getOneFree and its dependencies are as yet undiscovered it cannot be
 * started.
 * @param resRule - pointer to a RuleResearch to test
 * @return, true if @a resRule can be researched
 */
bool SavedGame::isResearchAvailable(const RuleResearch* const resRule) const // private.
{
	if (resRule != NULL)
	{
		if (_rules->getUnit(resRule->getType()) != NULL // kL_add. Always allow aLiens to be researched & re-researched.
			|| _debug == true)
		{
			return true;
		}

		std::vector<const RuleResearch*> unlocksList;
		std::vector<std::string> unlockedTypes;
		for (std::vector<const RuleResearch*>::const_iterator
				i = _discovered.begin();
				i != _discovered.end();
				++i)
		{
			unlockedTypes = (*i)->getUnlocks();
			for (std::vector<std::string>::const_iterator
					j = unlockedTypes.begin();
					j != unlockedTypes.end();
					++j)
			{
				unlocksList.push_back(_rules->getResearch(*j)); // load up *all* unlocks.
			}
		}

		if (std::find(
				unlocksList.begin(),
				unlocksList.end(),
				resRule) == unlocksList.end()) // if resRule has NOT been revealed by any prior project yet ->
		{
/*			if (_rules->getUnit(resRule->getType()) != NULL									// if it's an aLien
				&& resRule->getGetOneFree().empty() == false								// and it grants a getOneFree
				&& ((std::find(																// and ...
							_discovered.begin(),
							_discovered.end(),
							_rules->getResearch("STR_LEADER_PLUS")) == _discovered.end()	// player still needs LeaderPlus
						&& std::find(
								resRule->getUnlocks().begin(),
								resRule->getUnlocks().end(),
								"STR_LEADER_PLUS") != resRule->getUnlocks().end())			// and the resRule can grant LeaderPlus
					|| (std::find(
							_discovered.begin(),
							_discovered.end(),
							_rules->getResearch("STR_COMMANDER_PLUS")) == _discovered.end()	// player still needs CommanderPlus
						&& std::find(
								resRule->getUnlocks().begin(),
								resRule->getUnlocks().end(),
								"STR_COMMANDER_PLUS") != resRule->getUnlocks().end())))		// and the resRule can grant CommanderPlus
			{
				return true; // that makes no sense; aLiens that grant LeaderPlus and/or CommanderPlus do not have a getOneFree entry.
			} */// It only makes sense (kind of) if a non-alien project has Leader/Commander-Plus as a getOneFree AND as an 'unlocks' value ....
				// Anyway, I'm doing my own thing because it's become obvious how sloppily the research was coded.


			const RuleResearch* ruleTest;
			std::vector<std::string> test (resRule->getGetOneFree());
			for (std::vector<std::string>::const_iterator
					i = test.begin();
					i != test.end();
					++i)
			{
				ruleTest = _rules->getResearch(*i);
				if (std::find(
						unlocksList.begin(),
						unlocksList.end(),
						ruleTest) == unlocksList.end())
				{
					return true; // resRule's getOneFree is not yet unlocked by any project. (so what)
				}
			}

			test = resRule->getDependencies();
			for (std::vector<std::string>::const_iterator
					i = test.begin();
					i != test.end();
					++i)
			{
				ruleTest = _rules->getResearch(*i);
				if (std::find(
						_discovered.begin(),
						_discovered.end(),
						ruleTest) == _discovered.end())
				{
					return false; // a dependency has not been discovered yet.
				}
			}
		}

		return true; // debug=true or resRule is unlocked already.
	}

	return false; // resRule = NULL.
}

/**
 * Assigns a list of RuleManufacture's that can be manufactured at a particular
 * Base.
 * @param availableProductions	- reference to a vector of pointers to
 * RuleManufacture in which to put productions that are currently available
 * @param base					- pointer to a Base
 */
void SavedGame::getAvailableProductions(
		std::vector<const RuleManufacture*>& availableProductions,
		const Base* const base) const
{
	const RuleManufacture* manufRule;
	const std::vector<Production*> baseProductions (base->getProductions());
	const std::vector<std::string> manufList (_rules->getManufactureList());
	for (std::vector<std::string>::const_iterator
			i = manufList.begin();
			i != manufList.end();
			++i)
	{
		manufRule = _rules->getManufacture(*i);
		if (isResearched(manufRule->getRequirements()) == true
			&& std::find_if(
					baseProductions.begin(),
					baseProductions.end(),
					equalProduction(manufRule)) == baseProductions.end())
		{
			availableProductions.push_back(manufRule);
		}
	}
}

/**
 * Assigns a list of newly available ResearchProjects that appear when a research
 * is completed.
 * @note This function checks for fake research and adds fake-research's dependents.
 * @note Called from GeoscapeState::time1Day() for NewPossibleResearchInfo screen.
 * @param dependents	- reference to a vector of pointers to the RuleResearch's
 * that are now available
 * @param resRule		- pointer to the RuleResearch that has just been discovered
 * @param base			- pointer to a Base
 */
void SavedGame::getDependentResearch(
		std::vector<const RuleResearch*>& dependents,
		const RuleResearch* const resRule,
		Base* const base) const
{
	getDependentResearchBasic(dependents, resRule, base);

	for (std::vector<const RuleResearch*>::const_iterator
			i = _discovered.begin();
			i != _discovered.end();
			++i)
	{
		if ((*i)->getCost() == 0 // discovered fake project.
			&& std::find(
					(*i)->getDependencies().begin(), // if resRule is found as a dependency of a discovered fake research ->
					(*i)->getDependencies().end(),
					resRule->getType()) != (*i)->getDependencies().end())
		{
			getDependentResearchBasic(dependents, *i, base);
		}
	}
}

/**
 * Assigns a list of newly available ResearchProjects that appear when a research
 * is completed.
 * @note This function doesn't check for fake research.
 * @note Called from addFinishedResearch(), getDependentResearch(), and itself.
 * @param dependents	- reference to a vector of pointers to the RuleResearch's
 * that are now available
 * @param resRule		- pointer to the RuleResearch that has just been discovered
 * @param base			- pointer to a Base
 */
void SavedGame::getDependentResearchBasic( // private.
		std::vector<const RuleResearch*>& dependents,
		const RuleResearch* const resRule,
		Base* const base) const
{
	std::vector<const RuleResearch*> availableProjects;
	getAvailableResearchProjects(availableProjects, base); // <---|			should be: any research that
	for (std::vector<const RuleResearch*>::const_iterator					//	(1) has all its dependencies discovered
			i = availableProjects.begin();									//	(2) or has been unlocked,
			i != availableProjects.end();									//	(3) and has all its requirements discovered -
			++i)															//	(4) but has *not* been discovered itself.
	{
		if (std::find(
					(*i)->getDependencies().begin(),
					(*i)->getDependencies().end(),
					resRule->getType()) != (*i)->getDependencies().end()	// if an availableProject has resRule as a dependent
			|| std::find(
					(*i)->getUnlocks().begin(),
					(*i)->getUnlocks().end(),
					resRule->getType()) != (*i)->getUnlocks().end())		// or an availableProject unlocks resRule
		{
			dependents.push_back(*i);										// push_back the availableProject as a dependent.

			if ((*i)->getCost() == 0)										// and if that's a fake project -> repeat
				getDependentResearchBasic(dependents, *i, base);
		}
	}
}

/**
 * Assigns a list of newly available RuleManufacture's that appear when a
 * research is completed.
 * @note This function checks for fake research.
 * @param dependents	- reference to a vector of pointers to the RuleManufacture's
 * that are now available
 * @param resRule		- pointer to the RuleResearch that has just been discovered
 */
void SavedGame::getDependentManufacture(
		std::vector<const RuleManufacture*>& dependents,
		const RuleResearch* const resRule) const
{
	const RuleManufacture* manufRule;
	const std::vector<std::string>& manufList (_rules->getManufactureList());
	for (std::vector<std::string>::const_iterator
			i = manufList.begin();
			i != manufList.end();
			++i)
	{
		manufRule = _rules->getManufacture(*i);
		const std::vector<std::string>& reqs (manufRule->getRequirements());
		if (isResearched(manufRule->getRequirements()) == true
			&& std::find(
					reqs.begin(),
					reqs.end(),
					resRule->getType()) != reqs.end())
		{
			dependents.push_back(manufRule);
		}
	}
}

/**
 * Checks if a RuleResearch is discovered.
 * @param resType - reference a research type
 * @return, true if type has been researched
 */
bool SavedGame::isResearched(const std::string& resType) const
{
	if (_debug == true || resType.empty() == true
		|| _rules->getResearch(resType) == NULL
		|| (_rules->getItem(resType) != NULL
			&& _rules->getItem(resType)->isResearchExempt() == true))
	{
		return true;
	}

	for (std::vector<const RuleResearch*>::const_iterator
			i = _discovered.begin();
			i != _discovered.end();
			++i)
	{
		if ((*i)->getType() == resType)
			return true;
	}

	return false;
}

/**
 * Checks if a list of RuleResearch is discovered.
 * @param resTypes - reference a vector of strings of research types
 * @return, true if all types have been researched
 */
bool SavedGame::isResearched(const std::vector<std::string>& resTypes) const
{
	if (_debug == true || resTypes.empty() == true)
		return true;

	for (std::vector<std::string>::const_iterator
			j = resTypes.begin();
			j != resTypes.end();
			++j)
	{
		if (isResearched(*j) == false)
			return false;
	}

	return true;
}

/**
 * Gets the soldier matching an ID.
 * @param id - a soldier's unique id
 * @return, pointer to the Soldier
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
 * @return, true if any promotions happened - so show the promotions screen
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
			processSoldier(*j, data);
		}

		for (std::vector<Transfer*>::const_iterator
				j = (*i)->getTransfers()->begin();
				j != (*i)->getTransfers()->end();
				++j)
		{
			if ((*j)->getTransferType() == PST_SOLDIER)
			{
				soldiers.push_back((*j)->getSoldier());
				processSoldier((*j)->getSoldier(), data);
			}
		}
	}


	int pro = 0;

	Soldier* fragBait = NULL;
	const int totalSoldiers = static_cast<int>(soldiers.size());

	if (data.totalCommanders == 0 // There can be only one.
		&& totalSoldiers > 29)
	{
		fragBait = inspectSoldiers(soldiers, participants, RANK_COLONEL);
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
		fragBait = inspectSoldiers(soldiers, participants, RANK_CAPTAIN);
		if (fragBait == NULL)
			break;

		fragBait->promoteRank();
		++pro;
		++data.totalColonels;
		--data.totalCaptains;
	}

	while (data.totalCaptains < totalSoldiers / 11)
	{
		fragBait = inspectSoldiers(soldiers, participants, RANK_SERGEANT);
		if (fragBait == NULL)
			break;

		fragBait->promoteRank();
		++pro;
		++data.totalCaptains;
		--data.totalSergeants;
	}

	while (data.totalSergeants < totalSoldiers / 5)
	{
		fragBait = inspectSoldiers(soldiers, participants, RANK_SQUADDIE);
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
		const Soldier* const soldier,
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
		const std::vector<Soldier*>& soldiers,
		const std::vector<Soldier*>& participants,
		SoldierRank soldierRank)
{
	Soldier* fragBait = NULL;
	int
		score = 0,
		scoreTest;

	for (std::vector<Soldier*>::const_iterator
			i = soldiers.begin();
			i != soldiers.end();
			++i)
	{
		if ((*i)->getRank() == soldierRank)
		{
			scoreTest = getSoldierScore(*i);
			if (scoreTest > score
				&& (Options::fieldPromotions == false
					|| std::find(
							participants.begin(),
							participants.end(),
							*i) != participants.end()))
			{
				score = scoreTest;
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
void SavedGame::toggleDebugMode()
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
 * *** FUNCTOR ***
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
		bool operator() (const AlienMission* const mission) const
		{
			return mission->getRegion() == _region
				&& mission->getRules().getObjective() == _objective;
		}
};

/**
 * Find a mission type in the active alien missions.
 * @param region	- the region's string ID
 * @param objective	- the active mission's objective
 * @return, pointer to the AlienMission or NULL
 */
AlienMission* SavedGame::findAlienMission(
		const std::string& region,
		MissionObjective objective) const
{
	std::vector<AlienMission*>::const_iterator mission = std::find_if(
																_activeMissions.begin(),
																_activeMissions.end(),
																matchRegionAndType(region, objective));
	if (mission != _activeMissions.end())
		return *mission;

	return NULL;
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
 * @param warned - true if warned (default true)
 */
void SavedGame::setWarned(bool warned)
{
	_warned = warned;
}

/**
 * *** FUNCTOR ***
 * Checks if a point is contained in a region.
 * @note This function object checks if a point is contained inside a region.
 */
class ContainsPoint
	:
		public std::unary_function<const Region*, bool>
{
private:
	double
		_lon,_lat;

	public:
		/// Remember the coordinates.
		ContainsPoint(
				double lon,
				double lat)
			:
				_lon(lon),
				_lat(lat)
		{}

		/// Check if the region contains the stored point.
		bool operator() (const Region* const region) const
		{
			return region->getRules()->insideRegion(_lon,_lat);
		}
};

/**
 * Finds the Region containing this location.
 * @param lon - the longtitude
 * @param lat - the latitude
 * @return, pointer to the region or NULL
 */
Region* SavedGame::locateRegion(
		double lon,
		double lat) const
{
	const std::vector<Region*>::const_iterator i = std::find_if(
															_regions.begin(),
															_regions.end(),
															ContainsPoint(lon,lat));
	if (i != _regions.end())
		return *i;

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
 * Increments the month counter.
 */
void SavedGame::addMonth()
{
	++_monthsPassed;
}

/*
 * Toggles the state of the radar line drawing.
 *
void SavedGame::toggleRadarLines()
{
	_radarLines = !_radarLines;
} */
/*
 * @return, the state of the radar line drawing.
 *
bool SavedGame::getRadarLines()
{
	return _radarLines;
} */
/*
 * Toggles the state of the detail drawing.
 *
void SavedGame::toggleDetail()
{
	_detail = !_detail;
} */
/*
 * @return, the state of the detail drawing.
 *
bool SavedGame::getDetail()
{
	return _detail;
} */

/**
 * Marks a ResearchProject as having popped up -> "we can now research".
 * @note Used by NewPossibleResearchState.
 * @param research - pointer to the RuleResearch
 */
void SavedGame::addPoppedResearch(const RuleResearch* const resRule)
{
	if (wasResearchPopped(resRule) == false)
		_poppedResearch.push_back(resRule);
}

/**
 * Checks if a ResearchProject has previously popped up.
 * @note Used by NewPossibleResearchState.
 * @param resRule - pointer to the RuleResearch
 * @return, true if popped
 */
bool SavedGame::wasResearchPopped(const RuleResearch* const resRule)
{
	return std::find(
				_poppedResearch.begin(),
				_poppedResearch.end(),
				resRule) != _poppedResearch.end();
}

/**
 * Checks for and removes a ResearchProject from the popped-up list.
 * @note Called from addFinishedResearch().
 * @param resRule - pointer to the RuleResearch
 */
void SavedGame::removePoppedResearch(const RuleResearch* const resRule) // private.
{
	for (std::vector<const RuleResearch*>::const_iterator
			i = _poppedResearch.begin();
			i != _poppedResearch.end();
			++i)
	{
		if (*i == resRule)
		{
			_poppedResearch.erase(i);
			return;
		}
	}
}

/**
 * Returns the list of dead soldiers.
 * @return, pointer to a vector of pointers to SoldierDead.
 */
std::vector<SoldierDead*>* SavedGame::getDeadSoldiers()
{
	return &_deadSoldiers;
}

/*
 * Returns the last selected player base.
 * @return, pointer to base
 *
Base* SavedGame::getRecallBase()
{
	// in case a base was destroyed or something...
	if (_selectedBase < _bases.size())
		return _bases.at(_selectedBase);
	else
		return _bases.front();
} */
/*
 * Sets the last selected player base.
 * @param base - # of the base
 *
void SavedGame::setRecallBase(size_t base)
{
	_selectedBase = base;
} */
/*
 * Sets the last selected armour.
 * @param value - the new value for last selected armor - Armor type string
 *
void SavedGame::setRecallArmor(const std::string& value)
{
	_lastselectedArmor = value;
} */
/*
 * Gets the last selected armour.
 * @return, last used armor type string
 *
std::string SavedGame::getRecallArmor()
{
	return _lastselectedArmor;
} */

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
	const bool ret (_debugArgDone);

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
 * Scores points for XCom or aLiens.
 * @param lon	- longitude
 * @param lat	- latitude
 * @param pts	- points to award
 * @param aLien	- true if aLienPts; false if XCom points
 */
void SavedGame::scorePoints(
		double lon,
		double lat,
		int pts,
		bool aLien) const
{
	for (std::vector<Region*>::const_iterator
			i = _regions.begin();
			i != _regions.end();
			++i)
	{
		if ((*i)->getRules()->insideRegion(lon,lat) == true)
		{
			if (aLien == true)
			{
				(*i)->addActivityAlien(pts);
				(*i)->recentActivityAlien();
			}
			else // XCom
			{
				(*i)->addActivityXCom(pts);
				(*i)->recentActivityXCom();
			}
			break;
		}
	}

	for (std::vector<Country*>::const_iterator
			i = _countries.begin();
			i != _countries.end();
			++i)
	{
		if ((*i)->getRules()->insideCountry(lon,lat) == true)
		{
			if (aLien == true)
			{
				(*i)->addActivityAlien(pts);
				(*i)->recentActivityAlien();
			}
			else // XCom
			{
				(*i)->addActivityXCom(pts);
				(*i)->recentActivityXCom();
			}
			break;
		}
	}
}

/*
 * Returns the craft corresponding to the specified ID.
 * @param craftId - the unique craft id to look up
 * @return, the craft with the specified id, or NULL
 *
Craft* SavedGame::findCraftByUniqueId(const CraftId& craftId) const
{
	for (std::vector<Base*>::const_iterator i = _bases.begin(); i != _bases.end(); ++i)
		for (std::vector<Craft*>::const_iterator j = (*i)->getCrafts()->begin(); j != (*i)->getCrafts()->end(); ++j)
			if ((*j)->getUniqueId() == craftId) return *j;
	return NULL;
} */

}
