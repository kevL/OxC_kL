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
#include "../Ruleset/RuleTerrain.h"


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
class SavedBattleGame;
class SavedGame;
class Texture;
class Tile;
class Ufo;
class Unit;
class Vehicle;


/**
 * A utility class that generates the initial battlescape data.
 * Taking into account mission type, craft and ufo involved, terrain type,...
 */
class BattlescapeGenerator
{

private:
	bool
		_allowAutoLoadout,
		_baseEquipScreen,
		_craftDeployed,
		_generateFuel,
		_isCity;
	int
		_alienItemLevel,
		_blocksToDo,
		_craftZ,
		_mapsize_x,
		_mapsize_y,
		_mapsize_z,
		_unitSequence,
		_shade;
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
	RuleTerrain* _terrain;
	SavedBattleGame* _battleSave;
	SavedGame* _savedGame;
	Texture* _texture;
	Tile* _tileEquipt;
	Ufo* _ufo;

	std::string
		_alienRace,
		_missionType;


	/// Sets the map size and associated vars.
	void init();

	/// Deploys the XCOM units on the mission.
	void deployXCOM();
	/// Adds a vehicle to the game.
	BattleUnit* addXCOMVehicle(Vehicle* tank);
	/// Adds a soldier to the game.
	BattleUnit* addXCOMUnit(BattleUnit* unit);
	/// Runs necessary checks before physically setting the position.
	bool canPlaceXCOMUnit(Tile* tile);
	/// Loads a weapon on the inventoryTile.
	void loadGroundWeapon(BattleItem* item);
	/// Load all Xcom weapons.
//	void loadWeapons();
	/// Places an item on a soldier based on equipment layout.
	bool placeItemByLayout(BattleItem* item);
	/// Sets xCom soldiers' combat clothing style - spritesheets & paperdolls.
	void setTacticalSprites();

	/// Adds an item to a unit and the game.
	bool addItem(
			BattleItem* item,
			BattleUnit* unit);
//			bool allowSecondClip = false

	/// Deploys the aliens, according to the alien deployment rules.
	void deployAliens(AlienDeployment* const deployRule);
	/// Adds an alien to the game.
	BattleUnit* addAlien(
			Unit* const unitRule,
			int alienRank,
			bool outside);
	/// Finds a spot near a friend to spawn at.
	bool placeUnitNearFriend(BattleUnit* unit);

	/// Spawns civilians on a terror mission.
	void deployCivilians(int civilians);
	/// Adds a civlian to the game.
	BattleUnit* addCivilian(Unit* rules);

	/// Loads an XCom RMP file.
	void loadRMP(
			MapBlock* mapblock,
			int xoff,
			int yoff,
			int segment);
	/// Loads an XCom MAP file.
	int loadMAP(
			MapBlock* const mapblock,
			int offset_x,
			int offset_y,
			RuleTerrain* terrainRule,
			int dataSetOffset,
			bool discovered = false,
			bool craft = false);

	/// Fills power sources with an alien fuel object.
	void fuelPowerSources();
	/// Possibly explodes ufo powersources.
	void explodePowerSources();

	/// Generates a new battlescape map.
	void generateMap(const std::vector<MapScript*>* const script);
	/// Generates a map from base modules.
	void generateBaseMap();

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
			const std::vector<SDL_Rect*>* rects,
			int& X,
			int& Y,
			int sizeX,
			int sizeY);
	/// Adds a craft (either a ufo or an xcom craft) somewhere on the map.
	bool addCraft(
			const MapBlock* const craftMap,
			MapScript* command,
			SDL_Rect& craftPos);
	/// Adds a line (generally a road) to the map.
	bool addLine(
			MapDirection lineType,
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
	bool removeBlocks(MapScript* command);


	public:
		/// Creates a new BattlescapeGenerator class
		BattlescapeGenerator(Game* game);
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
		/// Sets if Ufo has landed/crashed at a city.
//		void setIsCity(const bool isCity = true);
		/// Sets the terrain.
		void setTacTerrain(RuleTerrain* terrain);
		/// Sets the polygon texture.
//		void setTacTexture(Texture* texture);
		/// Sets the polygon shade.
		void setTacShade(int shade);
		/// Sets the alien race.
		void setAlienRace(const std::string& alienRace);
		/// Sets the alien item level.
		void setAlienItemlevel(int alienItemLevel);

		/// Sets up the next stage (for Cydonia/TFTD missions).
		void nextStage();
		/// Runs the generator.
		void run();

		/// Generates a fake battlescape for Craft & Base soldier-inventory.
		void runInventory(
				Craft* craft,
				Base* base = NULL,
				size_t equipUnit = 0);
};

}

#endif
