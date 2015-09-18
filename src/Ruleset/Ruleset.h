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

#ifndef OPENXCOM_RULESET_H
#define OPENXCOM_RULESET_H

//#include <map>
//#include <string>
//#include <vector>
//#include <SDL.h>
//#include <yaml-cpp/yaml.h>

#include "../Ruleset/RuleAlienMission.h"

#include "../Savegame/GameTime.h"


namespace OpenXcom
{

class AlienDeployment;
class AlienRace;
class ArticleDefinition;
class Base;
class ExtraSounds;
class ExtraSprites;
class ExtraStrings;
class Game;
class MapDataSet;
class MapScript;
class MCDPatch;
class OperationPool;
class ResourcePack;
class RuleAlienMission;
class RuleArmor;
class RuleAward;
class RuleBaseFacility;
class RuleCountry;
class RuleCraft;
class RuleCraftWeapon;
class RuleGlobe;
class RuleInterface;
class RuleInventory;
class RuleItem;
class RuleManufacture;
class RuleMissionScript;
class RuleMusic;
class RuleRegion;
class RuleResearch;
class RuleSoldier;
class RuleTerrain;
class RuleUfo;
class RuleUnit;
class RuleVideo;
class SavedGame;
class Soldier;
class SoldierNamePool;
class SoundDefinition;
//class StatString;
class UfoTrajectory;


/**
 * Set of rules and stats for a game.
 * @note A ruleset holds all the constant info that never changes throughout a
 * game, like stats of all the in-game items, countries, research tree, soldier
 * names, starting base, etc.
 */
class Ruleset
{

protected:
	int
//		_initialFunding,
		_firstGrenade,
		_retalChance,

		_costEngineer,
		_costScientist,
		_costSoldier,
		_timePersonnel,
		_radarCutoff,

		_modIndex,

		_craftListOrder,
		_facilityListOrder,
		_invListOrder,
		_itemListOrder,
		_manufactureListOrder,
		_researchListOrder,
		_ufopaediaListOrder;

	GameTime _startingTime;
	YAML::Node _startingBase;

	const Game* const _game;
	RuleGlobe* _globe;

	std::string
		_finalResearch,
		_font;

	std::vector<std::string>
		_alienMissionsIndex,
		_aliensIndex,
		_armorsIndex,
		_countriesIndex,
		_craftsIndex,
		_craftWeaponsIndex,
		_deploymentsIndex,
		_extraSoundsIndex,
		_extraSpritesIndex,
		_extraStringsIndex,
		_facilitiesIndex,
		_invsIndex,
		_itemsIndex,
		_manufactureIndex,
		_MCDPatchesIndex,
		_missionScriptIndex,
		_musicIndex,
		_regionsIndex,
		_researchIndex,
		_terrainIndex,
		_ufopaediaIndex,
		_ufosIndex,

		_psiRequirements; // it's a cache for psiStrengthEval

	std::vector<std::vector<int> > _alienItemLevels;

	std::vector<OperationPool*> _operationTitles;
	std::vector<SoldierNamePool*> _names;
//	std::vector<StatString*> _statStrings;

	std::map<std::string, AlienDeployment*>		_alienDeployments;
	std::map<std::string, AlienRace*>			_alienRaces;
	std::map<std::string, RuleArmor*>			_armors;
	std::map<std::string, ArticleDefinition*>	_ufopaediaArticles;
	std::map<std::string, ExtraStrings*>		_extraStrings;
	std::map<std::string, MapDataSet*>			_mapDataSets;
	std::map<std::string, MCDPatch*>			_MCDPatches;
	std::map<std::string, RuleAlienMission*>	_alienMissions;
	std::map<std::string, RuleAward*>			_awards;
	std::map<std::string, RuleBaseFacility*>	_facilities;
	std::map<std::string, RuleCountry*>			_countries;
	std::map<std::string, RuleCraft*>			_crafts;
	std::map<std::string, RuleCraftWeapon*>		_craftWeapons;
	std::map<std::string, RuleInterface*>		_interfaces;
	std::map<std::string, RuleInventory*>		_invs;
	std::map<std::string, RuleItem*>			_items;
	std::map<std::string, RuleManufacture*>		_manufacture;
	std::map<std::string, RuleMissionScript*>	_missionScripts;
	std::map<std::string, RuleRegion*>			_regions;
	std::map<std::string, RuleResearch*>		_research;
	std::map<std::string, RuleSoldier*>			_soldiers;
	std::map<std::string, RuleTerrain*>			_terrains;
	std::map<std::string, RuleUfo*>				_ufos;
	std::map<std::string, RuleVideo*>			_videos;
	std::map<std::string, SoundDefinition*>		_soundDefs;
	std::map<std::string, UfoTrajectory*>		_ufoTrajectories;
	std::map<std::string, RuleUnit*>			_units;

	std::map<std::string, std::vector<MapScript*> >	_mapScripts;

	std::vector<std::pair<std::string, ExtraSounds*> >	_extraSounds;
	std::vector<std::pair<std::string, ExtraSprites*> >	_extraSprites;
	std::vector<std::pair<std::string, RuleMusic*> >	_music;

	std::pair<std::string, int> _alienFuel;

	/// Loads all ruleset files from a directory.
	void loadFiles(const std::string& dir);
	/// Loads a ruleset from a YAML file.
	void loadFile(const std::string& file);

	/// Loads a ruleset element.
	template<typename T>
	T* loadRule(
			const YAML::Node& node,
			std::map<std::string, T*>* types,
			std::vector<std::string>* index = NULL,
			const std::string& keyId = "type");


	public:
		/// Creates a blank ruleset.
		explicit Ruleset(const Game* const game);
		/// Cleans up the ruleset.
		~Ruleset();

		/// Reloads the country lines.
		void reloadCountryLines() const;

		/// Checks to ensure Mission scripts are okay.
		void validateMissionScripts() const;

		/// Loads a ruleset from the given source.
		void load(const std::string& source);

		/// Generates the starting saved game.
		SavedGame* newSave() const;

		/// Gets the pool list for soldier names.
		const std::vector<SoldierNamePool*>& getPools() const;
		/// Gets the pool list for operation titles.
		const std::vector<OperationPool*>& Ruleset::getOperations() const;

		/// Gets the ruleset for a country type.
		RuleCountry* getCountry(const std::string& id) const;
		/// Gets the available countries.
		const std::vector<std::string>& getCountriesList() const;
		/// Gets the ruleset for a region type.
		RuleRegion* getRegion(const std::string& id) const;
		/// Gets the available regions.
		const std::vector<std::string>& getRegionsList() const;

		/// Gets the ruleset for a facility type.
		RuleBaseFacility* getBaseFacility(const std::string& id) const;
		/// Gets the available facilities.
		const std::vector<std::string>& getBaseFacilitiesList() const;

		/// Gets the ruleset for a craft type.
		RuleCraft* getCraft(const std::string& id) const;
		/// Gets the available crafts.
		const std::vector<std::string>& getCraftsList() const;
		/// Gets the ruleset for a craft weapon type.
		RuleCraftWeapon* getCraftWeapon(const std::string& id) const;
		/// Gets the available craft weapons.
		const std::vector<std::string>& getCraftWeaponsList() const;

		/// Gets the ruleset for an item type.
		RuleItem* getItem(const std::string& id) const;
		/// Gets the available items.
		const std::vector<std::string>& getItemsList() const;

		/// Gets the ruleset for a UFO type.
		RuleUfo* getUfo(const std::string& id) const;
		/// Gets the available UFOs.
		const std::vector<std::string>& getUfosList() const;

		/// Gets the available terrains.
		const std::vector<std::string>& getTerrainList() const;
		/// Gets terrains for battlescape games.
		RuleTerrain* getTerrain(const std::string& type) const;

		/// Gets mapdatafile for battlescape games.
		MapDataSet* getMapDataSet(const std::string& name);

		/// Gets award rules.
		std::map<std::string, RuleAward*> getAwardsList() const;

		/// Gets soldier unit rules.
		RuleSoldier* getSoldier(const std::string& type) const;
		/// Gets non-soldier unit rules.
		RuleUnit* getUnit(const std::string& type) const;

		/// Gets alien race rules.
		AlienRace* getAlienRace(const std::string& type) const;
		/// Gets the available alien races.
		const std::vector<std::string>& getAlienRacesList() const;
		/// Gets deployment rules.
		AlienDeployment* getDeployment(const std::string& name) const;
		/// Gets the available alien deployments.
		const std::vector<std::string>& getDeploymentsList() const;

		/// Gets armor rules.
		RuleArmor* getArmor(const std::string& name) const;
		/// Gets the available armors.
		const std::vector<std::string>& getArmorsList() const;

		/// Gets Ufopaedia ArticleDefinition.
		ArticleDefinition* getUfopaediaArticle(const std::string& name) const;
		/// Gets the available articles.
		const std::vector<std::string>& getUfopaediaList() const;

		/// Gets the inventory list.
		std::map<std::string, RuleInventory*>* getInventories();
		/// Gets the ruleset for a specific inventory.
		RuleInventory* getInventory(const std::string& id) const;

		/// Gets the cost of a soldier.
		int getSoldierCost() const;
		/// Gets the cost of an engineer.
		int getEngineerCost() const;
		/// Gets the cost of a scientist.
		int getScientistCost() const;
		/// Gets the transfer time of personnel.
		int getPersonnelTime() const;

		/// Gets the ruleset for a specific research project.
		RuleResearch* getResearch(const std::string& id) const;
		/// Gets the list of all research projects.
		const std::vector<std::string>& getResearchList() const;
		/// Gets the ruleset for a specific manufacture project.
		RuleManufacture* getManufacture(const std::string& id) const;
		/// Gets the list of all manufacture projects.
		const std::vector<std::string>& getManufactureList() const;

		/// Gets facilities for custom bases.
		std::vector<OpenXcom::RuleBaseFacility*> getCustomBaseFacilities() const;

		/// Gets a specific UfoTrajectory.
		const UfoTrajectory* getUfoTrajectory(const std::string& id) const;
		/// Gets the ruleset for a specific alien mission.
		const RuleAlienMission* getAlienMission(const std::string& id) const;
		/// Gets the ruleset for a random alien mission.
		const RuleAlienMission* getRandomMission(
				MissionObjective objective,
				size_t monthsPassed) const;
		/// Gets the list of all alien missions.
		const std::vector<std::string>& getAlienMissionList() const;

		/// Gets the alien item level table.
		const std::vector<std::vector<int> >& getAlienItemLevels() const;

		/// Gets the pre-defined starting base.
		const YAML::Node& getStartingBase() const;
		/// Gets the pre-defined start time of a game.
		const GameTime& getStartingTime() const;

		/// Gets an MCDPatch.
		MCDPatch* getMCDPatch(const std::string& name) const;

		/// Gets the music rules
		std::vector<std::pair<std::string, RuleMusic*> > getMusic() const;

		/// Gets the list of external Sprites.
		std::vector<std::pair<std::string, ExtraSprites*> > getExtraSprites() const;
		/// Gets the list of external Sounds.
		std::vector<std::pair<std::string, ExtraSounds*> > getExtraSounds() const;
		/// Gets the list of external Strings.
		std::map<std::string, ExtraStrings*> getExtraStrings() const;

		/// Gets the list of StatStrings.
//		std::vector<StatString*> getStatStrings() const;

		/// Sorts all the lists according to their weight.
		void sortLists();

		/// Gets the research-requirements for Psi-Lab (it's a cache for psiStrengthEval)
		std::vector<std::string> getPsiRequirements() const;

		/// Returns the sorted list of inventories.
		const std::vector<std::string>& getInvsList() const;

		/// Generates a new soldier.
		Soldier* genSoldier(SavedGame* const save) const;

		/// Gets the item to be used as fuel for ships.
		const std::string& getAlienFuelType() const;
		/// Gets the amount of alien fuel to recover
		int getAlienFuelQuantity() const;

		/// Gets the font name.
		std::string getFontName() const;

		/// Gets the minimum radar's range.
//		int getMinRadarRange() const;
		/// Gets maximum radar range out of all facilities.
		int getMaxRadarRange() const;
		/// Gets the cutoff between small & large radars.
		int getRadarCutoffRange() const;

		/// Gets the turn aliens are allowed to throw their first grenades.
		int getFirstGrenade() const;

		/// Gets the basic retaliation chance.
		int getRetaliationChance() const;

		/// Gets information on an interface element.
		RuleInterface* getInterface(const std::string& id) const;

		/// Gets the ruleset for the globe.
		RuleGlobe* getGlobe() const;

		/// Gets the list of selective files for insertion into internal Cat files.
		const std::map<std::string, SoundDefinition*>* getSoundDefinitions() const;

		/// Gets the list of videos for intro/outro etc.
		const std::map<std::string, RuleVideo*>* getVideos() const;

		/// Gets the list of mission scripts.
		const std::vector<std::string>* getMissionScriptList() const;
		/// Gets a mission script.
		RuleMissionScript* getMissionScript(const std::string& id) const;

		/// Gets the list of MapScripts.
		const std::vector<MapScript*>* getMapScript(const std::string& id) const;

		/// Gets the final research Id.
		const std::string& getFinalResearch() const;

		/// Gets the current Game.
		const Game* const getGame() const;
};

}

#endif
