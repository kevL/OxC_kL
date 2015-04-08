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

#ifndef OPENXCOM_SAVEDBATTLEGAME_H
#define OPENXCOM_SAVEDBATTLEGAME_H

//#include <algorithm>
//#include <iostream>
//#include <string>
//#include <vector>
//#include <SDL.h>
//#include <yaml-cpp/yaml.h>

#include "BattleUnit.h"


namespace OpenXcom
{

class BattleItem;
class BattlescapeState;
class Game;
class MapDataSet;
class Node;
class OperationPool;
class Pathfinding;
class Position;
class Ruleset;
class SavedGame;
class State;
class Tile;
class TileEngine;


/**
 * The battlescape data that gets written to disk when the game is saved.
 * This holds all the variable info in a battlegame like mapdata, soldiers, items, etc.
 */
class SavedBattleGame
{

private:
	bool
		_aborted,
		_debugMode,
		_cheating,
		_kneelReserved,
		_unitsFalling;
	int
		_ambience,
		_depth,
		_globalShade,
		_groundLevel,
		_itemId,
		_mapsize_x,
		_mapsize_y,
		_mapsize_z,
		_objectivesDestroyed,
		_objectivesNeeded,
		_turn;

	BattleActionType _batReserved;
	UnitFaction _side;

	BattlescapeState* _battleState;
	BattleUnit
		* _selectedUnit,
		* _lastSelectedUnit;
	Pathfinding* _pathfinding;
	Tile* _invBattle; // kL
	Tile** _tiles;
	TileEngine* _tileEngine;

	std::string
		_alienRace, // kL
		_missionType,
		_music,
		_terrain; // sza_MusicRules
	std::wstring _operationTitle;

	std::list<BattleUnit*> _fallingUnits;

	std::vector<BattleItem*>
		_deleted,
		_items,
		_recoverGuaranteed,
		_recoverConditional;
	std::vector<BattleUnit*>
//kL	_exposedUnits,
		_units;
	std::vector<MapDataSet*> _mapDataSets;
	std::vector<Node*> _nodes;
	std::vector<Position>
		_tileSearch,
		_storageSpace;
	std::vector<std::vector<std::pair<int, int> > > _baseModules;

	Uint8 _dragButton;			// this is a cache for Options::getString("battleScrollDragButton")
	bool _dragInvert;			// this is a cache for Options::getString("battleScrollDragInvert")
	int
		_dragTimeTolerance,		// this is a cache for Options::getInt("battleScrollDragTimeTolerance")
		_dragPixelTolerance;	// this is a cache for Options::getInt("battleScrollDragPixelTolerance")

	/// Selects a soldier.
	BattleUnit* selectFactionUnit(
			int dir,
			bool checkReselect = false,
			bool setDontReselect = false,
			bool checkInventory = false);


	public:
		/// Creates a new battle save based on the current generic save.
		SavedBattleGame(const std::vector<OperationPool*>* titles = NULL);
		/// Cleans up the saved game.
		~SavedBattleGame();

		/// Loads a saved battle game from YAML.
		void load(
				const YAML::Node& node,
				Ruleset* rule,
				SavedGame* savedGame);
		/// Saves a saved battle game to YAML.
		YAML::Node save() const;

		/// Sets the dimensions of the map and initializes it.
		void initMap(
				const int mapsize_x,
				const int mapsize_y,
				const int mapsize_z);
		/// Initialises the pathfinding and tileengine.
		void initUtilities(ResourcePack* res);

		/// Gets the game's mapdatafiles.
		std::vector<MapDataSet*>* getMapDataSets();

		/// Sets the mission type.
		void setMissionType(const std::string& missionType);
		/// Gets the mission type.
		std::string getMissionType() const;

		/// Sets the global shade.
		void setGlobalShade(int shade);
		/// Gets the global shade.
		int getGlobalShade() const;

		/// Gets a pointer to the tiles, a tile is the smallest component of battlescape.
		Tile** getTiles() const;

		/// Gets a pointer to the list of nodes.
		std::vector<Node*>* getNodes();
		/// Gets a pointer to the list of units.
		std::vector<BattleUnit*>* getUnits();
		/// Gets a pointer to the list of items.
		std::vector<BattleItem*>* getItems();

		/// Gets terrain size x.
		int getMapSizeX() const;
		/// Gets terrain size y.
		int getMapSizeY() const;
		/// Gets terrain size z.
		int getMapSizeZ() const;
		/// Gets terrain x*y*z
		int getMapSizeXYZ() const;

		/// Sets the type of terrain for the mission.
		void setBattleTerrain(const std::string& terrain);
		/// Gets the type of terrain for the mission.
		std::string getBattleTerrain() const;

		/**
		 * Converts coordinates into a unique index.
		 * getTile() calls this every time, so should be inlined along with it.
		 * @note Are not functions that are defined inside the class def'n here
		 * supposedly assumed as 'inlined' ......
		 * @param pos - a position to convert
		 * @return, the unique index
		 */
		inline int getTileIndex(const Position& pos) const
		{ return (pos.z * _mapsize_y * _mapsize_x) + ((pos.y * _mapsize_x) + pos.x); }

		/// Converts a tile index to its coordinates.
		void getTileCoords(
				int index,
				int* x,
				int* y,
				int* z) const;

		/**
		 * Gets the Tile at a given position on the map.
		 * This method is called over 50mil+ times per turn so it seems useful to inline it.
		 * @note Are not functions that are defined inside the class def'n here
		 * supposedly assumed as 'inlined' ......
		 * @param pos - reference a Map position
		 * @return, pointer to the tile at that position
		 */
		inline Tile* getTile(const Position& pos) const
		{	if (   pos.x < 0
				|| pos.y < 0
				|| pos.z < 0
				|| pos.x >= _mapsize_x
				|| pos.y >= _mapsize_y
				|| pos.z >= _mapsize_z)
			{
				return NULL;
			}

			return _tiles[getTileIndex(pos)]; }

		/// Gets the currently selected unit.
		BattleUnit* getSelectedUnit() const;
		/// Sets the currently selected unit.
		void setSelectedUnit(BattleUnit* unit);
		/// Selects the previous soldier.
		BattleUnit* selectPreviousFactionUnit(
				bool checkReselect = false,
				bool setDontReselect = false,
				bool checkInventory = false);
		/// Selects the next soldier.
		BattleUnit* selectNextFactionUnit(
				bool checkReselect = false,
				bool setDontReselect = false,
				bool checkInventory = false);
		/// Selects the unit with position on map.
		BattleUnit* selectUnit(const Position& pos);

		/// Gets the pathfinding object.
		Pathfinding* getPathfinding() const;
		/// Gets a pointer to the tileengine.
		TileEngine* getTileEngine() const;

		/// Gets the playing side.
		UnitFaction getSide() const;

		/// Gets the turn number.
		int getTurn() const;

		/// Ends the turn.
		bool endBattlePhase();

		/// Sets debug mode.
		void setDebugMode();
		/// Gets debug mode.
		bool getDebugMode() const;

		/// Load map resources.
		void loadMapResources(Game* game);
		/// Resets tiles units are standing on
		void resetUnitTiles();

		/// Removes an item from the game.
		void removeItem(BattleItem* const item);

		/// Sets whether the mission was aborted.
		void setAborted(bool flag);
		/// Checks if the mission was aborted.
		bool isAborted() const;

		/// Sets how many objectives need to be destroyed.
		void addToObjectiveCount();
		/// Increments the objectives-destroyed counter.
		void addDestroyedObjective();
		/// Checks if all the objectives are destroyed.
		bool allObjectivesDestroyed() const;

		/// Sets the next available item ID value.
		void setNextItemId();
		/// Gets the next available item ID value.
		int* getNextItemId();

		/// Gets a spawn node.
		Node* getSpawnNode(
				int unitRank,
				BattleUnit* unit);
		/// Gets a patrol node.
		Node* getPatrolNode(
				bool scout,
				BattleUnit* unit,
				Node* curNode);

		/// Carries out new turn preparations.
		void prepareBattleTurn();

		/// Revives unconscious units of @a faction.
//		void reviveUnits(const UnitFaction faction);
		/// Revives unconscious units.
		void reviveUnit(
				BattleUnit* const unit,
				bool atTurnStart = false);
		/// Removes the body item that corresponds to the unit.
		void removeCorpse(const BattleUnit* const unit);

		/// Sets or tries to set a unit of a certain size on a certain position of the map.
		bool setUnitPosition(
				BattleUnit* unit,
				const Position& pos,
				bool testOnly = false);
		/// Attempts to place a unit on or near Position pos.
		bool placeUnitNearPosition(
				BattleUnit* unit,
				Position pos);

		/// Adds this unit to the vector of falling units.
		bool addFallingUnit(BattleUnit* unit);
		/// Gets the vector of falling units.
		std::list<BattleUnit*>* getFallingUnits();
		/// Toggles the switch that says "there are units falling, start the fall state".
		void setUnitsFalling(bool fall);
		/// Checks the status of the switch that says "there are units falling".
		bool getUnitsFalling() const;

		/// Gets a pointer to the BattlescapeGame.
		BattlescapeGame* getBattleGame() const;
		/// Gets a pointer to the BattlescapeState.
		BattlescapeState* getBattleState() const;
		/// Sets the pointer to the BattlescapeState.
		void setBattleState(BattlescapeState* bs);

		/// Gets the highest ranked, living unit of faction.
		BattleUnit* getHighestRanked(
				int& qtyAllies,
				bool isXCOM = true);
		/// Gets the morale modifier based on the highest ranked, living xcom/alien unit, or for a unit passed into this function.
		int getMoraleModifier(
				BattleUnit* unit = NULL,
				bool isXCOM = true);

		/// Checks whether a particular faction has eyes on *unit (whether any unit on that faction sees *unit).
//		bool eyesOnTarget(UnitFaction faction, BattleUnit* unit);

		/// Resets the turn counter.
		void resetTurnCounter();
		/// Resets the visibility of all tiles on the map.
		void resetTiles();

		/// Gets an 11x11 grid of positions (-10 to +10) to check.
		const std::vector<Position> getTileSearch();

		/// Checks if the AI has engaged cheat mode.
		bool isCheating();

		/// Gets the reserved fire mode.
		BattleActionType getBATReserved() const;
		/// Sets the reserved fire mode.
		void setBATReserved(BattleActionType reserved);
		/// Gets whether we are reserving TUs to kneel.
		bool getKneelReserved() const;
		/// Sets whether we are reserving TUs to kneel.
		void setKneelReserved(bool reserved);

		/// Gives me access to the storage tiles vector.
		std::vector<Position>& getStorageSpace();
		/// Moves all the leftover items to random locations in the storage tiles vector.
		void randomizeItemLocations(Tile* tile);

		/// Gets a reference to the baseModules map.
		std::vector<std::vector<std::pair<int, int> > >& getModuleMap();
		/// Calculates the number of map modules remaining
		void calculateModuleMap();

		/// Gets a pointer to the Geoscape save.
		SavedGame* getGeoscapeSave() const;

		/// Gets the depth of the battlescape game.
		int getDepth() const;
		/// Sets the depth of the battlescape game.
		void setDepth(int depth);

		/// Uses the depth variable to set a palette.
		void setPaletteByDepth(State* state);

		/// Sets the ambient sound effect;
		void setAmbientSound(int sound);
		/// Gets the ambient sound effect;
		int getAmbientSound() const;

		/// Gets the list of items guaranteed to be recovered.
		std::vector<BattleItem*>* getGuaranteedRecoveredItems();
		/// Gets the list of items that MIGHT get recovered.
		std::vector<BattleItem*>* getConditionalRecoveredItems();

		/// Get the name of the music track.
//		std::string& getMusic();
		/// Set the name of the music track.
		void setMusic(std::string track);

		/// Sets the inventory tile when BattlescapeGenerator runs.
		void setBattleInventory(Tile* invBattle);
		/// Gets the inventory tile for preBattle InventoryState OK click.
		Tile* getBattleInventory() const;

		/// Sets the alien race for this battle game.
		void setAlienRace(const std::string& alienRace);
		/// Gets the alien race participating in this battle game.
		const std::string& getAlienRace() const;

		/// Sets the ground level.
		void setGroundLevel(const int level);
		/// Gets the ground level.
		int getGroundLevel() const;

		/// Gets the operation title of the mission.
		const std::wstring& getOperation() const;

		/// Sets variables for what music to play in a particular terrain.
		void calibrateMusic(
				std::string& music,
				std::string& terrain);
};

}

#endif
