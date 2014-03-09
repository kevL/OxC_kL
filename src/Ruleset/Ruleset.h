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

#ifndef OPENXCOM_RULESET_H
#define OPENXCOM_RULESET_H

#include <map>
#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "../Savegame/GameTime.h"


namespace OpenXcom
{

class AlienDeployment;
class AlienRace;
class Armor;
class ArticleDefinition;
class Base;
class City;
class ExtraMusic; // sza_ExtraMusic
class ExtraSounds;
class ExtraSprites;
class ExtraStrings;
class MapDataSet;
class MCDPatch;
class ResourcePack;
class RuleAlienMission;
class RuleBaseFacility;
class RuleCountry;
class RuleCraft;
class RuleCraftWeapon;
class RuleInventory;
class RuleItem;
class RuleManufacture;
class RuleMusic; // sza_MusicRules
class RuleRegion;
class RuleResearch;
class RuleSoldier;
class RuleTerrain;
class RuleUfo;
class SavedGame;
class SoldierNamePool;
class UfoTrajectory;
class Unit;


/**
 * Set of rules and stats for a game.
 * A ruleset holds all the constant info that never changes
 * throughout a game, like stats of all the in-game items,
 * countries, research tree, soldier names, starting base, etc.
 */
class Ruleset
{

protected:
	int
		_initialFunding,

		_costEngineer,
		_costScientist,
		_costSoldier,
		_timePersonnel,

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

	std::vector<std::string>
		_alienMissionsIndex,
		_aliensIndex,
		_armorsIndex,
		_countriesIndex,
		_craftsIndex,
		_craftWeaponsIndex,
		_deploymentsIndex,
		_extraMusicIndex, // sza_ExtraMusic
		_extraSoundsIndex,
		_extraSpritesIndex,
		_extraStringsIndex,
		_facilitiesIndex,
		_invsIndex,
		_itemsIndex,
		_manufactureIndex,
		_MCDPatchesIndex,
		_musicIndex, // sza_MusicRules
		_regionsIndex,
		_researchIndex,
		_terrainIndex,
		_ufopaediaIndex,
		_ufosIndex,

		_psiRequirements; // it's a cache for psiStrengthEval

	std::vector<std::vector<int> > _alienItemLevels;

	std::vector<SoldierNamePool*> _names;

	std::map<std::string, AlienDeployment*> _alienDeployments;
	std::map<std::string, AlienRace*> _alienRaces;
	std::map<std::string, Armor*> _armors;
	std::map<std::string, ArticleDefinition*> _ufopaediaArticles;
	std::map<std::string, ExtraStrings*> _extraStrings;
	std::map<std::string, MapDataSet*> _mapDataSets;
	std::map<std::string, MCDPatch*> _MCDPatches;
	std::map<std::string, RuleAlienMission*> _alienMissions;
	std::map<std::string, RuleBaseFacility*> _facilities;
	std::map<std::string, RuleCountry*> _countries;
	std::map<std::string, RuleCraft*> _crafts;
	std::map<std::string, RuleCraftWeapon*> _craftWeapons;
	std::map<std::string, RuleInventory*> _invs;
	std::map<std::string, RuleItem*> _items;
	std::map<std::string, RuleManufacture*> _manufacture;
	std::map<std::string, RuleRegion*> _regions;
	std::map<std::string, RuleResearch*> _research;
	std::map<std::string, RuleSoldier*> _soldiers;
	std::map<std::string, RuleTerrain*> _terrains;
	std::map<std::string, RuleUfo*> _ufos;
	std::map<std::string, UfoTrajectory*> _ufoTrajectories;
	std::map<std::string, Unit*> _units;

	std::vector<std::pair<std::string, ExtraMusic*> > _extraMusic; // sza_ExtraMusic
	std::vector<std::pair<std::string, ExtraSounds*> > _extraSounds;
	std::vector<std::pair<std::string, ExtraSprites*> > _extraSprites;
	std::vector<std::pair<std::string, RuleMusic*> > _music; // sza_MusicRules


	/// Loads a ruleset from a YAML file.
	void loadFile(const std::string& filename);
	/// Loads all ruleset files from a directory.
	void loadFiles(const std::string& dirname);

	/// Loads a ruleset element.
	template <typename T> T* loadRule(
			const YAML::Node& node,
			std::map<std::string, T*>* map,
			std::vector<std::string>* index = 0,
			const std::string& key = "type");

	public:
		/// Creates a blank ruleset.
		Ruleset();
		/// Cleans up the ruleset.
		virtual ~Ruleset();

		/// Loads a ruleset from the given source.
		void load(const std::string& source);
		/// Generates the starting saved game.
		virtual SavedGame* newSave() const;
		/// Gets the pool list for soldier names.
		const std::vector<SoldierNamePool*>& getPools() const;
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
		RuleTerrain* getTerrain(const std::string& name) const;
		/// Gets mapdatafile for battlescape games.
		MapDataSet* getMapDataSet(const std::string& name);
		/// Gets soldier unit rules.
		RuleSoldier* getSoldier(const std::string& name) const;
		/// Gets generated unit rules.
		Unit* getUnit(const std::string& name) const;
		/// Gets alien race rules.
		AlienRace* getAlienRace(const std::string& name) const;
		/// Gets the available alien races.
		const std::vector<std::string>& getAlienRacesList() const;
		/// Gets deployment rules.
		AlienDeployment* getDeployment(const std::string& name) const;
		/// Gets the available alien deployments.
		const std::vector<std::string>& getDeploymentsList() const;
		/// Gets armor rules.
		Armor* getArmor(const std::string& name) const;
		/// Gets the available armors.
		const std::vector<std::string>& getArmorsList() const;
		/// Gets Ufopaedia article definition.
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
		/// Gets the list of all alien missions.
		const std::vector<std::string>& getAlienMissionList() const;
		/// Gets the city at the specified coordinates.
		const City* locateCity(double lon, double lat) const;
		/// Gets the alien item level table.
		const std::vector<std::vector<int> >& getAlienItemLevels() const;
		/// Gets the Defined starting base.
		const YAML::Node& getStartingBase();
		/// Gets an MCDPatch.
		MCDPatch* getMCDPatch(const std::string name) const;
		/// Gets the music rules
		std::vector<std::pair<std::string, RuleMusic*> > getMusic() const; // sza_MusicRules
		/// Gets the list of external Sprites.
		std::vector<std::pair<std::string, ExtraSprites*> > getExtraSprites() const;
		/// Gets the list of external Sounds.
		std::vector<std::pair<std::string, ExtraSounds*> > getExtraSounds() const;
		/// Gets the list of external music rules.
		std::vector<std::pair<std::string, ExtraMusic*> > getExtraMusic() const; // sza_ExtraMusic
		/// Gets the list of external Strings.
		std::map<std::string, ExtraStrings*> getExtraStrings() const;
		/// Sorts all our lists according to their weight.
		void sortLists();
		/// Gets the research-requirements for Psi-Lab (it's a cache for psiStrengthEval)
		std::vector<std::string> getPsiRequirements();
		/// Returns the sorted list of inventories.
		const std::vector<std::string>& getInvsList() const;
};

}

#endif
