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

#include "SavedGame.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>

#include <yaml-cpp/yaml.h>

#include "AlienBase.h"
#include "AlienMission.h"
#include "AlienStrategy.h"
#include "Base.h"
#include "Country.h"
#include "Craft.h"
#include "GameTime.h"
#include "ItemContainer.h"
#include "Production.h"
#include "Region.h"
#include "ResearchProject.h"
#include "SavedBattleGame.h"
#include "Soldier.h"
#include "SoldierDead.h" // kL
#include "TerrorSite.h"
#include "Transfer.h"
#include "Ufo.h"
#include "Waypoint.h"

#include "../version.h"

#include "../Engine/CrossPlatform.h"
#include "../Engine/Exception.h"
#include "../Engine/Language.h"
#include "../Engine/Logger.h"
#include "../Engine/Options.h"
#include "../Engine/RNG.h"

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
	findRuleResearch(RuleResearch* toFind);

	bool operator()(const ResearchProject* r) const;
};

///
findRuleResearch::findRuleResearch(RuleResearch* toFind)
	:
		_toFind(toFind)
{
}

///
bool findRuleResearch::operator()(const ResearchProject* r) const
{
	return _toFind == r->getRules();
}

///
struct equalProduction
	:
		public std::unary_function<Production*, bool>
{
	RuleManufacture* _item;
	equalProduction(RuleManufacture* item);

	bool operator()(const Production* p) const;
};

///
equalProduction::equalProduction(RuleManufacture* item)
	:
		_item(item)
{
}

///
bool equalProduction::operator()(const Production* p) const
{
	return p->getRules() == _item;
}


/**
 * Initializes a brand new saved game according to the specified difficulty.
 */
SavedGame::SavedGame()
	:
		_difficulty(DIFF_BEGINNER),
		_ironman(false),
		_globeLon(0.f),
		_globeLat(0.f),
		_globeZoom(0),
		_battleGame(0),
		_debug(false),
		_warned(false),
//		_detail(true),
//		_radarLines(false),
		_monthsPassed(-1),
		_graphRegionToggles(""),
		_graphCountryToggles(""),
		_graphFinanceToggles(""),
		_curRowMatrix(0), // kL
		_curGraph(0) // kL
//		_curGraphRowCountry(0) // kL
//kL	_selectedBase(0),
//kL	_lastselectedArmor("STR_ARMOR_NONE_UC")
{
	_time = new GameTime(6, 1, 1, 1999, 12, 0, 0);

	_alienStrategy = new AlienStrategy();

	_funds.push_back(0);
	_maintenance.push_back(0);
	_researchScores.push_back(0);
	_income.push_back(0);		// kL
	_expenditure.push_back(0);	// kL
}

/**
 * Deletes the game content from memory.
 */
SavedGame::~SavedGame()
{
	delete _time;

	for (std::vector<Country*>::iterator
			i = _countries.begin();
			i != _countries.end();
			++i)
	{
		delete *i;
	}

	for (std::vector<Region*>::iterator
			i = _regions.begin();
			i != _regions.end();
			++i)
	{
		delete *i;
	}

	for (std::vector<Base*>::iterator
			i = _bases.begin();
			i != _bases.end();
			++i)
	{
		delete *i;
	}

	for (std::vector<Ufo*>::iterator
			i = _ufos.begin();
			i != _ufos.end();
			++i)
	{
		delete *i;
	}

	for (std::vector<Waypoint*>::iterator
			i = _waypoints.begin();
			i != _waypoints.end();
			++i)
	{
		delete *i;
	}

	for (std::vector<TerrorSite*>::iterator
			i = _terrorSites.begin();
			i != _terrorSites.end();
			++i)
	{
		delete *i;
	}

	for (std::vector<AlienBase*>::iterator
			i = _alienBases.begin();
			i != _alienBases.end();
			++i)
 	{
		delete *i;
	}

	delete _alienStrategy;

	for (std::vector<AlienMission*>::iterator
			i = _activeMissions.begin();
			i != _activeMissions.end();
			++i)
	{
		delete *i;
	}

	for (std::vector<Soldier*>::iterator
			i = _soldiers.begin();
			i != _soldiers.end();
			++i)
	{
		delete *i;
	}

	for (std::vector<SoldierDead*>::iterator // kL_begin:
			i = _deadSoldiers.begin();
			i != _deadSoldiers.end();
			++i)
	{
		delete *i;
	} // kL_end.

	for (std::vector<MissionStatistics*>::iterator
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
 * @param lang		- pointer to a loaded language
 * @param autoquick	- true to include autosaves and quicksaves
 * @return, vector of saves info
 */
std::vector<SaveInfo> SavedGame::getList(
		Language* lang,
		bool autoquick)
{
	std::vector<SaveInfo> info;

	if (autoquick)
	{
		std::vector<std::string> saves = CrossPlatform::getFolderContents(
																		Options::getUserFolder(),
																		"asav");
		for (std::vector<std::string>::iterator
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

	std::vector<std::string> saves = CrossPlatform::getFolderContents(
																	Options::getUserFolder(),
																	"sav");
	for (std::vector<std::string>::iterator
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
 * @param file Save filename.
 * @param lang Loaded language.
 */
SaveInfo SavedGame::getSaveInfo(
		const std::string& file,
		Language* lang)
{
	std::string fullname = Options::getUserFolder() + file;
	YAML::Node doc = YAML::LoadFile(fullname);
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
	std::pair<std::wstring, std::wstring> str = CrossPlatform::timeToString(save.timestamp);
	save.isoDate = str.first;
	save.isoTime = str.second;

	std::wostringstream details;
/*kL	if (doc["turn"])
	{
		details << lang->getString("STR_BATTLESCAPE") << L": " << lang->getString(doc["mission"].as<std::string>()) << L" ";
		details << lang->getString("STR_TURN").arg(doc["turn"].as<int>());
	}
	else
	{
		GameTime time = GameTime(6, 1, 1, 1999, 12, 0, 0);
		time.load(doc["time"]);
		details << lang->getString("STR_GEOSCAPE") << L": ";
		details << time.getDayString(lang) << L" " << lang->getString(time.getMonthString()) << L" " << time.getYear() << L" ";
		details << time.getHour() << L":" << std::setfill(L'0') << std::setw(2) << time.getMinute();
	} */
	// kL_begin:
	if (doc["base"])
	{
		details << Language::utf8ToWstr(doc["base"].as<std::string>());
		details << L" - ";
	}

	GameTime time = GameTime(6, 1, 1, 1999, 12, 0, 0);
	time.load(doc["time"]);
	details << time.getDayString(lang) << L" " << lang->getString(time.getMonthString()) << L" " << time.getYear() << L" ";
	details << time.getHour() << L":" << std::setfill(L'0') << std::setw(2) << time.getMinute();

	if (doc["turn"])
	{
		details << L" - ";
		details << lang->getString(doc["mission"].as<std::string>()) << L" ";
		details << lang->getString("STR_TURN").arg(doc["turn"].as<int>());
	} // kL_end.

	if (doc["ironman"].as<bool>(false))
		details << L" " << lang->getString("STR_IRONMAN");

	save.details = details.str();

	if (doc["rulesets"])
		save.rulesets = doc["rulesets"].as<std::vector<std::string> >();

	return save;
}

/**
 * Loads a saved game's contents from a YAML file.
 * @note Assumes the saved game is blank.
 * @param filename YAML filename.
 * @param rule Ruleset for the saved game.
 */
void SavedGame::load(
		const std::string& filename,
		Ruleset* rule)
{
	//Log(LOG_INFO) << "SavedGame::load()";
	std::string s = Options::getUserFolder() + filename;

	std::vector<YAML::Node> file = YAML::LoadAllFromFile(s);
	if (file.empty())
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

	_difficulty = (GameDifficulty)doc["difficulty"].as<int>(_difficulty);
	//Log(LOG_INFO) << "SavedGame::load(), difficulty = " << _difficulty;

	if (doc["rng"]
		&& (_ironman || !Options::newSeedOnLoad))
	{
		RNG::setSeed(doc["rng"].as<uint64_t>());
	}

	_monthsPassed			= doc["monthsPassed"].as<int>(_monthsPassed);
//	_radarLines				= doc["radarLines"].as<bool>(_radarLines);
//	_detail					= doc["detail"].as<bool>(_detail);
	_graphRegionToggles		= doc["graphRegionToggles"].as<std::string>(_graphRegionToggles);
	_graphCountryToggles	= doc["graphCountryToggles"].as<std::string>(_graphCountryToggles);
	_graphFinanceToggles	= doc["graphFinanceToggles"].as<std::string>(_graphFinanceToggles);
	_funds					= doc["funds"].as<std::vector<int64_t> >(_funds);
	_maintenance			= doc["maintenance"].as<std::vector<int64_t> >(_maintenance);
	_researchScores			= doc["researchScores"].as<std::vector<int> >(_researchScores);
	_income					= doc["income"].as<std::vector<int64_t> >(_income);				// kL
	_expenditure			= doc["expenditure"].as<std::vector<int64_t> >(_expenditure);	// kL
	_warned					= doc["warned"].as<bool>(_warned);
	_globeLon				= doc["globeLon"].as<double>(_globeLon);
	_globeLat				= doc["globeLat"].as<double>(_globeLat);
	_globeZoom				= doc["globeZoom"].as<int>(_globeZoom);
	_ids					= doc["ids"].as<std::map<std::string, int> >(_ids);

	for (YAML::const_iterator
			i = doc["countries"].begin();
			i != doc["countries"].end();
			++i)
	{
		std::string type = (*i)["type"].as<std::string>();
		if (rule->getCountry(type))
		{
			Country* c = new Country(
								rule->getCountry(type),
								false);
			c->load(*i);
			_countries.push_back(c);
		}
	}

	for (YAML::const_iterator
			i = doc["regions"].begin();
			i != doc["regions"].end();
			++i)
	{
		std::string type = (*i)["type"].as<std::string>();
		if (rule->getRegion(type))
		{
			Region* r = new Region(rule->getRegion(type));
			r->load(*i);
			_regions.push_back(r);
		}
	}

	// Alien bases must be loaded before alien missions
	for (YAML::const_iterator
			i = doc["alienBases"].begin();
			i != doc["alienBases"].end();
			++i)
	{
		AlienBase* b = new AlienBase();
		b->load(*i);
		_alienBases.push_back(b);
	}

	// Missions must be loaded before UFOs.
	const YAML::Node& missions = doc["alienMissions"];
	for (YAML::const_iterator
			it = missions.begin();
			it != missions.end();
			++it)
	{
		std::string missionType = (*it)["type"].as<std::string>();
		const RuleAlienMission& mRule = *rule->getAlienMission(missionType);
		std::auto_ptr<AlienMission> mission(new AlienMission(mRule));
		mission->load(
					*it,
					*this);
		_activeMissions.push_back(mission.release());
	}

	for (YAML::const_iterator
			i = doc["ufos"].begin();
			i != doc["ufos"].end();
			++i)
	{
		std::string type = (*i)["type"].as<std::string>();
		if (rule->getUfo(type))
		{
			Ufo* u = new Ufo(rule->getUfo(type));
			u->load(
					*i,
					*rule,
					*this);
			_ufos.push_back(u);
		}
	}

	for (YAML::const_iterator
			i = doc["waypoints"].begin();
			i != doc["waypoints"].end();
			++i)
	{
		Waypoint* w = new Waypoint();
		w->load(*i);
		_waypoints.push_back(w);
	}

	for (YAML::const_iterator
			i = doc["terrorSites"].begin();
			i != doc["terrorSites"].end();
			++i)
	{
		TerrorSite* t = new TerrorSite();
		t->load(*i);
		_terrorSites.push_back(t);
	}

	// Discovered Techs Should be loaded before Bases (e.g. for PSI evaluation)
	for (YAML::const_iterator
			it = doc["discovered"].begin();
			it != doc["discovered"].end();
			++it)
	{
		std::string research = it->as<std::string>();
		if (rule->getResearch(research))
			_discovered.push_back(rule->getResearch(research));
	}

	for (YAML::const_iterator
			i = doc["bases"].begin();
			i != doc["bases"].end();
			++i)
	{
		Base* b = new Base(rule);
		b->load(
				*i,
				this,
				false);
		_bases.push_back(b);
	}

	const YAML::Node& research = doc["poppedResearch"];
	for (YAML::const_iterator
			it = research.begin();
			it != research.end();
			++it)
	{
		std::string research = it->as<std::string>();
		if (rule->getResearch(research))
			_poppedResearch.push_back(rule->getResearch(research));
	}

	_alienStrategy->load(
						rule,
						doc["alienStrategy"]);

	for (YAML::const_iterator
			i = doc["deadSoldiers"].begin();
			i != doc["deadSoldiers"].end();
			++i)
	{
/*kL		Soldier* s = new Soldier(
							rule->getSoldier("XCOM"),
							rule->getArmor("STR_ARMOR_NONE_UC"));
		s->load(
				*i,
				rule,
				this);
		_deadSoldiers.push_back(s); */
		SoldierDead* ds = new SoldierDead( // kL
										L"",
										0,
										RANK_ROOKIE,
										GENDER_MALE,
										LOOK_BLONDE,
										0,
										0,
										NULL,
										UnitStats(),
										UnitStats(),
										NULL);
		ds->load(*i);							// kL
		_deadSoldiers.push_back(ds);			// kL
	}

	for (YAML::const_iterator
			i = doc["missionStatistics"].begin();
			i != doc["missionStatistics"].end();
			++i)
	{
		MissionStatistics* ms = new MissionStatistics();
		ms->load(*i);

		_missionStatistics.push_back(ms);
	}

	if (const YAML::Node& battle = doc["battleGame"])
	{
		_battleGame = new SavedBattleGame();
		_battleGame->load(
						battle,
						rule,
						this);
	}
}

/**
 * Saves a saved game's contents to a YAML file.
 * @param filename YAML filename.
 */
void SavedGame::save(const std::string& filename) const
{
	std::string s = Options::getUserFolder() + filename;
	std::ofstream sav(s.c_str());
	if (!sav)
	{
		throw Exception("Failed to save " + filename);
	}

	YAML::Emitter out;

	// Saves the brief game info used in the saves list
	YAML::Node brief;

	brief["name"]		= Language::wstrToUtf8(_name);
	brief["version"]	= OPENXCOM_VERSION_SHORT;
	brief["build"]		= OPENXCOM_VERSION_GIT;
	brief["time"]		= _time->save();

	Base* base			= _bases.front();							// kL
	brief["base"]		= Language::wstrToUtf8(base->getName());	// kL

	if (_battleGame != NULL)
	{
		brief["mission"]	= _battleGame->getMissionType();
		brief["turn"]		= _battleGame->getTurn();
	}

	brief["rulesets"] = Options::rulesets;

	out << brief;

	// Saves the full game data to the save
	out << YAML::BeginDoc;

	YAML::Node node;

	node["difficulty"]			= static_cast<int>(_difficulty);
	node["monthsPassed"]		= _monthsPassed;
//	node["radarLines"]			= _radarLines;
//	node["detail"]				= _detail;
	node["graphRegionToggles"]	= _graphRegionToggles;
	node["graphCountryToggles"]	= _graphCountryToggles;
	node["graphFinanceToggles"]	= _graphFinanceToggles;
	node["rng"]					= RNG::getSeed();
	node["funds"]				= _funds;
	node["maintenance"]			= _maintenance;
	node["researchScores"]		= _researchScores;
	node["income"]				= _income;		// kL
	node["expenditure"]			= _expenditure;	// kL
	node["warned"]				= _warned;
	node["globeLon"]			= _globeLon;
	node["globeLat"]			= _globeLat;
	node["globeZoom"]			= _globeZoom;
	node["ids"]					= _ids;

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

	for (std::vector<TerrorSite*>::const_iterator
			i = _terrorSites.begin();
			i != _terrorSites.end();
			++i)
	{
		node["terrorSites"].push_back((*i)->save());
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

//kL	for (std::vector<Soldier*>::const_iterator
	for (std::vector<SoldierDead*>::const_iterator // kL
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

	out << node;
	sav << out.c_str();
	sav.close();
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
 * @return Tony Stark
 */
bool SavedGame::isIronman() const
{
	return _ironman;
}

/**
 * Changes if the game is set to ironman mode.
 * Ironman games cannot be manually saved.
 * @param ironman Tony Stark
 */
void SavedGame::setIronman(bool ironman)
{
	_ironman = ironman;
}

/**
 * Returns the current longitude of the Geoscape globe.
 * @return Longitude.
 */
double SavedGame::getGlobeLongitude() const
{
	return _globeLon;
}

/**
 * Changes the current longitude of the Geoscape globe.
 * @param lon Longitude.
 */
void SavedGame::setGlobeLongitude(double lon)
{
	_globeLon = lon;
}

/**
 * Returns the current latitude of the Geoscape globe.
 * @return Latitude.
 */
double SavedGame::getGlobeLatitude() const
{
	return _globeLat;
}

/**
 * Changes the current latitude of the Geoscape globe.
 * @param lat Latitude.
 */
void SavedGame::setGlobeLatitude(double lat)
{
	_globeLat = lat;
}

/**
 * Returns the current zoom level of the Geoscape globe.
 * @return, Zoom level.
 */
size_t SavedGame::getGlobeZoom() const
{
	return _globeZoom;
}

/**
 * Changes the current zoom level of the Geoscape globe.
 * @param zoom, Zoom level.
 */
void SavedGame::setGlobeZoom(size_t zoom)
{
	_globeZoom = zoom;
}

/**
 * Gives the player his monthly funds, taking in account
 * all maintenance and profit costs. Also stores monthly
 * totals for GraphsState.
 */
void SavedGame::monthlyFunding()
{
	// kL_begin: INCOME & EXPENDITURE
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

		// zero each Base's cash income values.
		(*i)->setCashIncome(-(*i)->getCashIncome());
		// zero each Base's cash spent values.
		(*i)->setCashSpent(-(*i)->getCashSpent());
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
// kL_end.


	// MAINTENANCE
	int baseMaintenance = getBaseMaintenance();

	_maintenance.back() = baseMaintenance;
	_maintenance.push_back(0);
	if (_maintenance.size() > 12)
		_maintenance.erase(_maintenance.begin());

	// BALANCE
	_funds.back() += getCountryFunding() - baseMaintenance;
	_funds.push_back(_funds.back());
	if (_funds.size() > 12)
		_funds.erase(_funds.begin());

	// SCORE (doesn't include xCom - aLien activity)
	_researchScores.push_back(0);
	if (_researchScores.size() > 12)
		_researchScores.erase(_researchScores.begin());

}

/**
 * Returns the player's current funds.
 * @return Current funds.
 */
int64_t SavedGame::getFunds() const
{
	return _funds.back();
}

/**
 * Returns the player's funds for the last 12 months.
 * @return funds.
 */
std::vector<int64_t>& SavedGame::getFundsList()
{
	return _funds;
}

/**
 * Changes the player's funds to a new value.
 * @param funds New funds.
 */
void SavedGame::setFunds(int64_t funds)
{
	_funds.back() = funds;
}

/**
 * return the list of monthly maintenance costs
 * @return list of maintenances.
 */
std::vector<int64_t>& SavedGame::getMaintenances()
{
	return _maintenance;
}

/**
 * kL. Return the list of monthly income values.
 * @return vector<int>, List of income values
 */
std::vector<int64_t>& SavedGame::getIncomeList() // kL
{
	return _income;
}

/**
 * kL. Return the list of monthly expenditure values.
 * @return vector<int>, List of income values
 */
std::vector<int64_t>& SavedGame::getExpenditureList() // kL
{
	return _expenditure;
}

/**
 * Returns the current time of the game.
 * @return, Pointer to the game time.
 */
GameTime* SavedGame::getTime() const
{
	return _time;
}

/**
 * Changes the current time of the game.
 * @param time, Game time.
 */
void SavedGame::setTime(GameTime time)
{
	_time = new GameTime(time);
}

/**
 * Returns the latest ID for the specified object and increases it.
 * @param name, Object name.
 * @return, Latest ID number.
 */
int SavedGame::getId(const std::string& name)
{
	std::map<std::string, int>::iterator i = _ids.find(name);

	if (i != _ids.end())
		return i->second++;
	else
	{
		_ids[name] = 1;

		return _ids[name]++;
	}
}

/**
 * Returns the list of countries in the game world.
 * @return, Pointer to country list.
 */
std::vector<Country*>* SavedGame::getCountries()
{
	return &_countries;
}

/**
 * Adds up the monthly funding of all the countries.
 * @return, Total funding.
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
 * @return, Pointer to region list.
 */
std::vector<Region*>* SavedGame::getRegions()
{
	return &_regions;
}

/**
 * Returns the list of player bases.
 * @return, Pointer to base list.
 */
std::vector<Base*>* SavedGame::getBases()
{
	return &_bases;
}

/**
 * Returns an immutable list of player bases.
 * @return, Pointer to base list.
 */
const std::vector<Base*>* SavedGame::getBases() const
{
	return &_bases;
}

/**
 * Adds up the monthly maintenance of all the bases.
 * @return, Total maintenance.
 */
int SavedGame::getBaseMaintenance() const
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
 * @return, Pointer to UFO list.
 */
std::vector<Ufo*>* SavedGame::getUfos()
{
	return &_ufos;
}

/**
 * Returns the list of craft waypoints.
 * @return, Pointer to waypoint list.
 */
std::vector<Waypoint*>* SavedGame::getWaypoints()
{
	return &_waypoints;
}

/**
 * Returns the list of terror sites.
 * @return, Pointer to terror site list.
 */
std::vector<TerrorSite*>* SavedGame::getTerrorSites()
{
	return &_terrorSites;
}

/**
 * Get pointer to the battleGame object.
 * @return, Pointer to the battleGame object.
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
 */
void SavedGame::addFinishedResearch(
		const RuleResearch* resRule,
		const Ruleset* rules)
{
	std::vector<const RuleResearch*>::const_iterator iDisc = std::find(
																	_discovered.begin(),
																	_discovered.end(),
																	resRule);
	if (iDisc == _discovered.end())
	{
		_discovered.push_back(resRule);

/*		if (rules
			&& resRule->getName() == "STR_POWER_SUIT")
		{
			_discovered.push_back(rules->getResearch("STR_BLACKSUIT_ARMOR"));
		} */ // i don't want to do this. I'd have to create RuleResearch's for each new armor!!!

		removePoppedResearch(resRule);
		addResearchScore(resRule->getPoints());
	}

	if (rules)
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

		for (std::vector<RuleResearch*>::iterator
				i = availableResearch.begin();
				i != availableResearch.end();
				++i)
		{
			if ((*i)->getCost() == 0
				&& (*i)->getRequirements().empty())
			{
				addFinishedResearch(
								*i,
								rules);
			}
			else if ((*i)->getCost() == 0)
			{
				size_t entry (0); // init.

				for (std::vector<std::string>::const_iterator
						iReq = (*i)->getRequirements().begin();
						iReq != (*i)->getRequirements().end();
						++iReq)
				{
					if ((*i)->getRequirements().at(entry) == *iReq)
						addFinishedResearch(
										*i,
										rules);

					entry++;
				}
			}
		}
	}
}

/**
 * Gets the list of already discovered ResearchProjects.
 * @return, the list of already discovered ResearchProjects
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
	std::vector<std::string> researchProjects = ruleset->getResearchList();
	const std::vector<ResearchProject*>& baseResearchProjects = base->getResearch();
	std::vector<const RuleResearch*> unlocked;

	for (std::vector<const RuleResearch*>::const_iterator
			it = discovered.begin();
			it != discovered.end();
			++it)
	{
		for (std::vector<std::string>::const_iterator
				itUnlocked = (*it)->getUnlocked().begin();
				itUnlocked != (*it)->getUnlocked().end();
				++itUnlocked)
		{
			unlocked.push_back(ruleset->getResearch(*itUnlocked));
		}
	}

	for (std::vector<std::string>::const_iterator
			iter = researchProjects.begin();
			iter != researchProjects.end();
			++iter)
	{
		RuleResearch* research = ruleset->getResearch(*iter);
		if (!isResearchAvailable(
							research,
							unlocked,
							ruleset))
		{
			continue;
		}

		std::vector<const RuleResearch*>::const_iterator itDiscovered = std::find(
																				discovered.begin(),
																				discovered.end(),
																				research);

		bool liveAlien = (ruleset->getUnit(research->getName()) != NULL);

		if (itDiscovered != discovered.end())
		{
			bool cull = true;
			if (research->getGetOneFree().size() != 0)
			{
				for (std::vector<std::string>::const_iterator
						ohBoy = research->getGetOneFree().begin();
						ohBoy != research->getGetOneFree().end();
						++ohBoy)
				{
					std::vector<const RuleResearch*>::const_iterator more_iteration = std::find(
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

			if (!liveAlien
				&& cull)
			{
				continue;
			}
			else
			{
				std::vector<std::string>::const_iterator leaderCheck = std::find(
																			research->getUnlocked().begin(),
																			research->getUnlocked().end(),
																			"STR_LEADER_PLUS");
				std::vector<std::string>::const_iterator cmnderCheck = std::find(
																			research->getUnlocked().begin(),
																			research->getUnlocked().end(),
																			"STR_COMMANDER_PLUS");

				bool leader = (leaderCheck != research->getUnlocked().end());
				bool cmnder = (cmnderCheck != research->getUnlocked().end());

				if (leader)
				{
					std::vector<const RuleResearch*>::const_iterator found = std::find(
																					discovered.begin(),
																					discovered.end(),
																					ruleset->getResearch("STR_LEADER_PLUS"));
					if (found == discovered.end())
						cull = false;
				}

				if (cmnder)
				{
					std::vector<const RuleResearch*>::const_iterator found = std::find(
																					discovered.begin(),
																					discovered.end(),
																					ruleset->getResearch("STR_COMMANDER_PLUS"));
					if (found == discovered.end())
						cull = false;
				}

				if (cull)
					continue;
			}
		}

		if (std::find_if(
					baseResearchProjects.begin(),
					baseResearchProjects.end(),
					findRuleResearch(research))
				!= baseResearchProjects.end())
		{
			continue;
		}

		if (research->needItem()
			&& base->getItems()->getItem(research->getName()) == 0)
		{
			continue;
		}

		if (research->getRequirements().size() != 0)
		{
			size_t tally(0);
			for (size_t
					itreq = 0;
					itreq != research->getRequirements().size();
					++itreq)
			{
				itDiscovered = std::find(
									discovered.begin(),
									discovered.end(),
									ruleset->getResearch(research->getRequirements().at(itreq)));
				if (itDiscovered != discovered.end())
					tally++;
			}

			if (tally != research->getRequirements().size())
				continue;
		}

		projects.push_back(research);
	}
}

/**
 * Get the list of RuleManufacture which can be manufacture in a Base.
 * @param productions the list of Productions which are available.
 * @param ruleset the Game Ruleset
 * @param base a pointer to a Base
 */
void SavedGame::getAvailableProductions(
		std::vector<RuleManufacture*>& productions,
		const Ruleset* ruleset,
		Base* base) const
{
	const std::vector<std::string>& items = ruleset->getManufactureList();
	const std::vector<Production*> baseProductions (base->getProductions()); // init.

	for (std::vector<std::string>::const_iterator
			iter = items.begin();
			iter != items.end();
			++iter)
	{
		RuleManufacture* m = ruleset->getManufacture(*iter);
		if (!isResearched(m->getRequirements()))
			continue;

		if (std::find_if(
						baseProductions.begin(),
						baseProductions.end(),
						equalProduction(m))
					!= baseProductions.end())
		{
			continue;
		}

		productions.push_back(m);
	}
}

/**
 * Checks whether a ResearchProject can be researched.
 * @param r			- pointer to a RuleResearch to test
 * @param unlocked	- reference to a vector of pointers to the currently unlocked RuleResearch
 * @param ruleset	- pointer to the current Ruleset
 * @return, true if the RuleResearch can be researched
 */
bool SavedGame::isResearchAvailable(
		RuleResearch* r,
		const std::vector<const RuleResearch*>& unlocked,
		const Ruleset* ruleset) const
{
	if (r == NULL)
		return false;

	std::vector<std::string> deps = r->getDependencies();
	const std::vector<const RuleResearch*>& discovered (getDiscoveredResearch()); // init.

	bool liveAlien = (ruleset->getUnit(r->getName()) != NULL);

	if (_debug
		|| std::find(
				unlocked.begin(),
				unlocked.end(),
				r)
			!= unlocked.end())
	{
		return true;
	}
	else if (liveAlien)
	{
		if (!r->getGetOneFree().empty())
		{
			std::vector<std::string>::const_iterator leaderCheck = std::find(
																		r->getUnlocked().begin(),
																		r->getUnlocked().end(),
																		"STR_LEADER_PLUS");
			std::vector<std::string>::const_iterator cmnderCheck = std::find(
																		r->getUnlocked().begin(),
																		r->getUnlocked().end(),
																		"STR_COMMANDER_PLUS");

			bool leader = (leaderCheck != r->getUnlocked().end());
			bool cmnder = (cmnderCheck != r->getUnlocked().end());

			if (leader)
			{
				std::vector<const RuleResearch*>::const_iterator found = std::find(
																				discovered.begin(),
																				discovered.end(),
																				ruleset->getResearch("STR_LEADER_PLUS"));
				if (found == discovered.end())
					return true;
			}

			if (cmnder)
			{
				std::vector<const RuleResearch*>::const_iterator found = std::find(
																				discovered.begin(),
																				discovered.end(),
																				ruleset->getResearch("STR_COMMANDER_PLUS"));
				if (found == discovered.end())
					return true;
			}
		}
	}

	for (std::vector<std::string>::const_iterator
			itFree = r->getGetOneFree().begin();
			itFree != r->getGetOneFree().end();
			++itFree)
	{
		if (std::find(
					unlocked.begin(),
					unlocked.end(),
					ruleset->getResearch(*itFree))
				== unlocked.end())
		{
			return true;
		}
	}

	for (std::vector<std::string>::const_iterator
			iter = deps.begin();
			iter != deps.end();
			++ iter)
	{
		RuleResearch* research = ruleset->getResearch(*iter);
		std::vector<const RuleResearch*>::const_iterator itDiscovered = std::find(
																				discovered.begin(),
																				discovered.end(),
																				research);
		if (itDiscovered == discovered.end())
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
			iter = _discovered.begin();
			iter != _discovered.end();
			++iter)
	{
		if ((*iter)->getCost() == 0)
		{
			if (std::find(
						(*iter)->getDependencies().begin(),
						(*iter)->getDependencies().end(),
						research->getName())
					!= (*iter)->getDependencies().end())
			{
				getDependableResearchBasic(
										dependables,
										*iter,
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
void SavedGame::getDependableResearchBasic(
		std::vector<RuleResearch*>& dependables,
		const RuleResearch* research,
		const Ruleset* ruleset,
		Base* base) const
{
	std::vector<RuleResearch*> possibleProjects;
	getAvailableResearchProjects(
							possibleProjects,
							ruleset,
							base);

	for (std::vector<RuleResearch*>::iterator
			iter = possibleProjects.begin();
			iter != possibleProjects.end();
			++iter)
	{
		if (std::find(
					(*iter)->getDependencies().begin(),
					(*iter)->getDependencies().end(),
					research->getName())
				!= (*iter)->getDependencies().end()
			|| std::find(
					(*iter)->getUnlocked().begin(),
					(*iter)->getUnlocked().end(),
					research->getName())
				!= (*iter)->getUnlocked().end())
		{
			dependables.push_back(*iter);

			if ((*iter)->getCost() == 0)
				getDependableResearchBasic(
										dependables,
										*iter,
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
	const std::vector<std::string> &mans = ruleset->getManufactureList();
	for (std::vector<std::string>::const_iterator
			iter = mans.begin();
			iter != mans.end();
			++iter)
	{
		RuleManufacture* m = ruleset->getManufacture(*iter);

		const std::vector<std::string>& reqs = m->getRequirements();
		if (isResearched(m->getRequirements())
			&& std::find(
						reqs.begin(),
						reqs.end(),
						research->getName())
					!= reqs.end())
		{
			dependables.push_back(m);
		}
	}
}

/**
 * Returns if a certain research has been completed.
 * @param research Research ID.
 * @return Whether it's researched or not.
 */
bool SavedGame::isResearched(const std::string& research) const
{
	if (research.empty() || _debug)
		return true;

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
 * @param research List of research IDs.
 * @return Whether it's researched or not.
 */
bool SavedGame::isResearched(const std::vector<std::string>& research) const
{
	if (research.empty() || _debug)
		return true;

	std::vector<std::string> matches = research;
	for (std::vector<const RuleResearch*>::const_iterator
			i = _discovered.begin();
			i != _discovered.end();
			++i)
	{
		for (std::vector<std::string>::iterator
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

		if (matches.empty())
			return true;
	}

	return false;
}

/**
 * Returns pointer to the Soldier given its unique ID.
 * @param id, A soldier's unique id.
 * @return, Pointer to Soldier.
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
 * Handles the higher promotions (not the rookie-squaddie ones).
 * @param participants a list of soldiers that were actually present at the battle.
 * @return, Whether or not some promotions happened - to show the promotions screen.
 */
bool SavedGame::handlePromotions(std::vector<Soldier*>& participants)
{
	size_t
		soldiersPromoted = 0,
		soldiersTotal = 0;

	for (std::vector<Base*>::iterator
			i = _bases.begin();
			i != _bases.end();
			++i)
	{
		soldiersTotal += (*i)->getSoldiers()->size();
	}

	Soldier* highestRanked = NULL;

	// now determine the number of positions we have of each rank,
	// and the soldier with the heighest promotion score of the rank below it

	size_t
		filledPositions = 0,
		filledPositions2 = 0;
	std::vector<Soldier*>::const_iterator
		soldier,
		stayedHome;

	stayedHome = participants.end();

	inspectSoldiers(
				&highestRanked,
				&filledPositions,
				RANK_COMMANDER);
	inspectSoldiers(
				&highestRanked,
				&filledPositions2,
				RANK_COLONEL);
	soldier = std::find(
					participants.begin(),
					participants.end(),
					highestRanked);

	if (filledPositions < 1
		&& filledPositions2 > 0
		&& (!Options::fieldPromotions
			|| soldier != stayedHome))
	{
		// only promote one colonel to commander
		highestRanked->promoteRank();
		soldiersPromoted++;
	}

	inspectSoldiers(
				&highestRanked,
				&filledPositions,
				RANK_COLONEL);
	inspectSoldiers(
				&highestRanked,
				&filledPositions2,
				RANK_CAPTAIN);
	soldier = std::find(
					participants.begin(),
					participants.end(),
					highestRanked);

	if (filledPositions < soldiersTotal / 23
		&& filledPositions2 > 0
		&& (!Options::fieldPromotions
			|| soldier != stayedHome))
	{
		highestRanked->promoteRank();
		soldiersPromoted++;
	}

	inspectSoldiers(
				&highestRanked,
				&filledPositions,
				RANK_CAPTAIN);
	inspectSoldiers(
				&highestRanked,
				&filledPositions2,
				RANK_SERGEANT);
	soldier = std::find(
					participants.begin(),
					participants.end(),
					highestRanked);

	if (filledPositions < soldiersTotal / 11
		&& filledPositions2 > 0
		&& (!Options::fieldPromotions
			|| soldier != stayedHome))
	{
		highestRanked->promoteRank();
		soldiersPromoted++;
	}

	inspectSoldiers(
				&highestRanked,
				&filledPositions,
				RANK_SERGEANT);
	inspectSoldiers(
				&highestRanked,
				&filledPositions2,
				RANK_SQUADDIE);
	soldier = std::find(
					participants.begin(),
					participants.end(),
					highestRanked);

	if (filledPositions < soldiersTotal / 5
		&& filledPositions2 > 0
		&& (!Options::fieldPromotions
			|| soldier != stayedHome))
	{
		highestRanked->promoteRank();
		soldiersPromoted++;
	}

	return soldiersPromoted > 0;
}

/**
 * Checks how many soldiers of a rank exist and which one has the highest score.
 * @param highestRanked - pointer to a pointer to store the highest-scoring soldier of that rank in
 * @param total			- pointer to store the total in
 * @param rank			- rank to inspect
 */
void SavedGame::inspectSoldiers(
		Soldier** highestRanked,
		size_t* total,
		int rank)
{
	int highestScore = 0;
	*total = 0;

	for (std::vector<Base*>::iterator
			i = _bases.begin();
			i != _bases.end();
			++i)
	{
		for (std::vector<Soldier*>::iterator
				j = (*i)->getSoldiers()->begin();
				j != (*i)->getSoldiers()->end();
				++j)
		{
			if ((*j)->getRank() == (SoldierRank)rank)
			{
				(*total)++;

				int score = getSoldierScore(*j);
				if (score > highestScore)
				{
					highestScore = score;
					*highestRanked = *j;
				}
			}
		}

		for (std::vector<Transfer*>::iterator
				j = (*i)->getTransfers()->begin();
				j != (*i)->getTransfers()->end();
				++j)
		{
			if ((*j)->getType() == TRANSFER_SOLDIER
				&& (*j)->getSoldier()->getRank() == (SoldierRank)rank)
			{
				(*total)++;

				int score = getSoldierScore((*j)->getSoldier());
				if (score > highestScore)
				{
					highestScore = score;
					*highestRanked = (*j)->getSoldier();
				}
			}
		}
	}
}

/**
 * Evaluate the score of a soldier based on all of his stats, missions and kills.
 * @param soldier - pointer to the soldier to get a score for
 * @return, the soldier's score
 */
int SavedGame::getSoldierScore(Soldier* soldier)
{
	UnitStats* stats = soldier->getCurrentStats();

	int score = 2 * stats->health
				+ 2 * stats->stamina
				+ 4 * stats->reactions
				+ 4 * stats->bravery
				+ 3 * (stats->tu + 2 * (stats->firing))
				+ stats->melee
				+ stats->throwing
				+ stats->strength;

	if (stats->psiSkill > 0)
		score += stats->psiStrength
				+ 2 * stats->psiSkill;

	score += 10 * (soldier->getMissions() + soldier->getKills());

	return score;
}

/**
  * Returns the list of alien bases.
  * @return Pointer to alien base list.
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
 * @return Debug mode.
 */
bool SavedGame::getDebugMode() const
{
	return _debug;
}


/** @brief Match a mission based on region and type.
 * This function object will match alien missions based on region and type.
 */
class matchRegionAndType
	:
		public std::unary_function<AlienMission*, bool>
{
private:
	const std::string& _region;
	const std::string& _type;

	public:
		/// Store the region and type.
		matchRegionAndType(
				const std::string& region,
				const std::string& type)
			:
				_region(region),
				_type(type)
		{
		}
		/// Match against stored values.
		bool operator()(const AlienMission* mis) const
		{
			return mis->getRegion() == _region
				&& mis->getType() == _type;
		}
};

/**
 * Find a mission from the active alien missions.
 * @param region The region ID.
 * @param type The mission type ID.
 * @return A pointer to the mission, or 0 if no mission matched.
 */
AlienMission* SavedGame::getAlienMission(
		const std::string& region,
		const std::string& type) const
{
	std::vector<AlienMission*>::const_iterator
			am = std::find_if(
						_activeMissions.begin(),
						_activeMissions.end(),
						matchRegionAndType(
										region,
										type));
	if (am == _activeMissions.end())
		return 0;

	return *am;
}

/**
 * adds to this month's research score
 * @param score the amount to add.
 */
void SavedGame::addResearchScore(int score)
{
	_researchScores.back() += score;
}

/**
 * return the list of research scores
 * @return list of research scores.
 */
std::vector<int>& SavedGame::getResearchScores()
{
	return _researchScores;
}

/**
 * return if the player has been warned about poor performance.
 * @return true or false.
 */
bool SavedGame::getWarned() const
{
	return _warned;
}

/**
 * sets the player's "warned" status.
 * @param warned set "warned" to this.
 */
void SavedGame::setWarned(bool warned)
{
	_warned = warned;
}


/** @brief Check if a point is contained in a region.
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
		{
			/* Empty by design. */
		}
		/// Check is the region contains the stored point.
		bool operator()(const Region* region) const
		{
			return region->getRules()->insideRegion(
												_lon,
												_lat);
		}
};

/**
 * Find the region containing this location.
 * @param lon The longtitude.
 * @param lat The latitude.
 * @return Pointer to the region, or 0.
 */
Region* SavedGame::locateRegion(
		double lon,
		double lat) const
{
	std::vector<Region*>::const_iterator found = std::find_if(
														_regions.begin(),
														_regions.end(),
														ContainsPoint(
																	lon,
																	lat));
	if (found != _regions.end())
	{
		return *found;
	}

	return NULL;
}

/**
 * Find the region containing this target.
 * @param target The target to locate.
 * @return Pointer to the region, or 0.
 */
Region* SavedGame::locateRegion(const Target& target) const
{
	return locateRegion(
					target.getLongitude(),
					target.getLatitude());
}

/**
 * @return the month counter.
 */
int SavedGame::getMonthsPassed() const
{
	return _monthsPassed;
}

/**
 * @return the GraphRegionToggles.
 */
const std::string& SavedGame::getGraphRegionToggles() const
{
	return _graphRegionToggles;
}

/**
 * @return the GraphCountryToggles.
 */
const std::string& SavedGame::getGraphCountryToggles() const
{
	return _graphCountryToggles;
}

/**
 * @return the GraphFinanceToggles.
 */
const std::string& SavedGame::getGraphFinanceToggles() const
{
	return _graphFinanceToggles;
}

/**
 * Sets the GraphRegionToggles.
 * @param value The new value for GraphRegionToggles.
 */
void SavedGame::setGraphRegionToggles(const std::string& value)
{
	_graphRegionToggles = value;
}

/**
 * Sets the GraphCountryToggles.
 * @param value The new value for GraphCountryToggles.
 */
void SavedGame::setGraphCountryToggles(const std::string& value)
{
	_graphCountryToggles = value;
}

/**
 * Sets the GraphFinanceToggles.
 * @param value The new value for GraphFinanceToggles.
 */
void SavedGame::setGraphFinanceToggles(const std::string& value)
{
	_graphFinanceToggles = value;
}

/**
 * kL. Sets the current Graph page.
 * @param page - current page shown by Graphs
 */
void SavedGame::setCurrentGraph(int page) // kL
{
	_curGraph = page;
}

/**
 * kL. Gets the current Graph page.
 * @return, current page to show in Graphs
 */
int SavedGame::getCurrentGraph() const // kL
{
	return _curGraph;
}

/**
 * Increment the month counter.
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
 * @return the state of the radar line drawing.
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
 * @return the state of the detail drawing.
 */
/* bool SavedGame::getDetail()
{
	return _detail;
} */

/**
 * marks a research topic as having already come up as "we can now research"
 * @param research is the project we want to add to the vector
 */
void SavedGame::addPoppedResearch(const RuleResearch* research)
{
	if (!wasResearchPopped(research))
		_poppedResearch.push_back(research);
}

/**
 * checks if an unresearched topic has previously been popped up.
 * @param research is the project we are checking for
 * @return whether or not it has been popped up.
 */
bool SavedGame::wasResearchPopped(const RuleResearch* research)
{
	return std::find(
				_poppedResearch.begin(),
				_poppedResearch.end(),
				research)
			!= _poppedResearch.end();
}

/**
 * checks for and removes a research project from the "has been popped up" array
 * @param research is the project we are checking for and removing, if necessary.
 */
void SavedGame::removePoppedResearch(const RuleResearch* research)
{
	std::vector<const RuleResearch*>::iterator r = std::find(
														_poppedResearch.begin(),
														_poppedResearch.end(),
														research);
	if (r != _poppedResearch.end())
		_poppedResearch.erase(r);
}

/**
 * Returns the list of dead soldiers.
 * @return, Pointer to soldier list.
 */
//kL std::vector<Soldier*>* SavedGame::getDeadSoldiers()
std::vector<SoldierDead*>* SavedGame::getDeadSoldiers() // kL
{
	//Log(LOG_INFO) << "SavedGame::getDeadSoldiers()";
	return &_deadSoldiers;
}

/**
 * Returns the last selected player base.
 * @return, pointer to base
 */
/*kL Base* SavedGame::getSelectedBase()
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
/*kL void SavedGame::setSelectedBase(size_t base)
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
 * kL. Sets the current Matrix row.
 * @param row - current Matrix row
 */
void SavedGame::setCurrentRowMatrix(size_t row) // kL
{
	_curRowMatrix = row;
}

/**
 * kL. Gets the current Matrix row.
 * @return, current Matrix row
 */
size_t SavedGame::getCurrentRowMatrix() const // kL
{
	return _curRowMatrix;
}

/**
 * Gets mission statistics for soldier commendations.
 * @return, a vector of pointers to mission statistics
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
/*kL
Craft* SavedGame::findCraftByUniqueId(const CraftId& craftId) const
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
