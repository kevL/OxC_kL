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

#ifndef OPENXCOM_BATTLESCAPEGENERATOR_H
#define OPENXCOM_BATTLESCAPEGENERATOR_H

//#include <vector>

#include "../Ruleset/MapScript.h"


namespace OpenXcom
{

class AlienBase;
class AlienDeployment;
class AlienRace;
class Base;
class BattleItem;
class BattleUnit;
class Craft;
class Game;
class MapBlock;
class MapScript;
class MissionSite;
class ResourcePack;
class RuleItem;
class Ruleset;
class RuleTerrain;
class RuleUnit;
class SavedBattleGame;
class SavedGame;
class Tile;
class Ufo;
class Vehicle;


/**
 * A utility class that generates the initial battlescape data.
 * @note Taking into account mission type, craft and ufo involved, terrain type
 * ... etc.
 */
class BattlescapeGenerator
{

private:
	bool
		_baseEquipScreen,
		_craftDeployed,
		_generateFuel;
	int
		_alienItemLevel,
		_blocksLeft,
		_craftZ,
		_mapsize_x,
		_mapsize_y,
		_mapsize_z,
		_shade,
		_unitSequence;
	size_t _battleOrder;

	SDL_Rect
		_craftPos,
		_ufoPos;

	std::vector<std::vector<bool> > _landingzone;
	std::vector<std::vector<int> >
		_drillMap,
		_segments;
	std::vector<std::vector<MapBlock*> > _blocks;

	AlienBase* _alienBase;
	Base* _base;
	Craft* _craft;
	Game* _game;
	MapBlock* _testBlock;
	MissionSite* _mission;
	ResourcePack* _res;
	Ruleset* _rules;
	RuleTerrain* _terraRule;
	SavedBattleGame* _battleSave;
	SavedGame* _gameSave;
	Tile* _tileEquipt;
	Ufo* _ufo;

	std::string _alienRace;


	/// Sets the map size and associated vars.
	void init();

	/// Deploys the XCOM units on the mission.
	void deployXCOM();
	/// Adds a vehicle to the game.
	BattleUnit* addXCOMVehicle(Vehicle* const vehicle);
	/// Adds a soldier to the game.
	BattleUnit* addXCOMUnit(BattleUnit* const unit);
	/// Runs necessary checks before physically setting the position.
	bool canPlaceXCOMUnit(Tile* const tile);
	/// Loads a weapon on the inventoryTile.
	void loadGroundWeapon(BattleItem* const item);
	/// Places an item on a soldier based on equipment layout.
	bool placeItemByLayout(BattleItem* const item);
	/// Sets xCom soldiers' combat clothing style - spritesheets & paperdolls.
	void setTacticalSprites() const;

	/// Adds an item to a unit and the game.
	bool addItem(
			BattleItem* const item,
			BattleUnit* const unit);

	/// Deploys the aliens according to the AlienDeployment rule.
	void deployAliens(AlienDeployment* const deployRule);
	/// Adds an alien to the game.
	BattleUnit* addAlien(
			RuleUnit* const unitRule,
			int aLienRank,
			bool outside);
	/// Finds a spot near a friend to spawn at.
	bool placeUnitNearFriend(BattleUnit* const unit);

	/// Spawns civilians on a terror mission.
	void deployCivilians(int civilians);
	/// Adds a civlian to the game.
	void addCivilian(RuleUnit* const unitRule);

	/// Loads an XCom MAP file.
	int loadMAP(
			MapBlock* const block,
			int offset_x,
			int offset_y,
			const RuleTerrain* const terraRule,
			int dataSetOffset,
			bool discovered = false,
			bool craft = false);
	/// Loads an XCom RMP file.
	void loadRMP(
			MapBlock* const block,
			int xoff,
			int yoff,
			int segment);

	/// Fills power sources with an alien fuel object.
	void fuelPowerSources();
	/// Possibly explodes ufo powersources.
	void explodePowerSources();

	/// Generates a new battlescape map.
	void generateMap(const std::vector<MapScript*>* const script);
	/// Generates a map from base modules.
	void generateBaseMap();

	/// Finds Alien Base start modules for Xcom equipment spawning.
	void placeXcomProperty();

	/// Clears a module from the map.
	void clearModule(
			int x,
			int y,
			int sizeX,
			int sizeY);
	/// Load the nodes from the associated map blocks.
	void loadNodes();
	/// Connects all the nodes together.
	void attachNodeLinks();
	/// Selects an unused position on the map of a given size.
	bool selectPosition(
			const std::vector<SDL_Rect*>* const rects,
			int& ret_x,
			int& ret_y,
			int size_x,
			int size_y);
	/// Adds a craft (either a ufo or an xcom craft) somewhere on the map.
	bool addCraft(
			const MapBlock* const craftBlock,
			MapScript* const scriptCommand,
			SDL_Rect& rectCraft);
	/// Adds a line (generally a road) to the map.
	bool addLine(
			MapDirection dir,
			const std::vector<SDL_Rect*>* rects);
	/// Adds a single block at a given position.
	bool addBlock(
			int x,
			int y,
			MapBlock* const block);
	/// Drills some tunnels between map blocks.
	void drillModules(
			TunnelData* tunnelInfo,
			const std::vector<SDL_Rect*>* rects,
			MapDirection dir);
	/// Clears all modules in a rect from a command.
	bool removeBlocks(MapScript* const scriptCommand);


	public:
		/// Creates a new BattlescapeGenerator class
		explicit BattlescapeGenerator(Game* game);
		/// Cleans up the BattlescapeGenerator.
		~BattlescapeGenerator();

		/// Sets the XCom craft.
		void setCraft(Craft* craft);
		/// Sets the ufo.
		void setUfo(Ufo* ufo);
		/// Sets the XCom base.
		void setBase(Base* base);
		/// Sets the mission site.
		void setMissionSite(MissionSite* mission);
		/// Sets the alien base
		void setAlienBase(AlienBase* base);
		/// Sets the terrain.
		void setTacTerrain(RuleTerrain* terrain);
		/// Sets the polygon shade.
		void setTacShade(int shade);
		/// Sets the alien race.
		void setAlienRace(const std::string& alienRace);
		/// Sets the alien item level.
		void setAlienItemlevel(int alienItemLevel);

		/// Runs the generator.
		void run();
		/// Sets up the next stage (for Cydonia/TFTD missions).
		void nextStage();

		/// Generates a fake battlescape for Craft & Base soldier-inventory.
		void runInventory(
				Craft* craft,
				Base* base = NULL,
				size_t selUnit = 0);

		/// Sets up the objectives for the map.
		void setupObjectives(const AlienDeployment* const deployRule);
};

}

#endif
