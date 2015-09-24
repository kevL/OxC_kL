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

#ifndef OPENXCOM_TILE_H
#define OPENXCOM_TILE_H

//#include <list>
//#include <vector>
//#include <SDL_types.h>

#include "BattleUnit.h"

#include "../Battlescape/Position.h"

#include "../Ruleset/MapData.h"


namespace OpenXcom
{

class BattleItem;
class BattleUnit;
class MapData;
class RuleInventory;
class SavedBattleGame;
class Surface;


/**
 * Basic element of which a battle map is built.
 * @sa http://www.ufopaedia.org/index.php?title=MAPS
 */
class Tile
{

protected:
	static const size_t
		LIGHTLAYERS	= 3,
		SECTIONS	= 3;

	bool
		_danger,
		_discovered[3],
		_visible;
	int
		_animOffset,
		_curFrame[4],
		_explosive,
		_explosiveType,
		_fire,
		_light[LIGHTLAYERS],
		_mapDataId[4],
		_mapDataSetId[4],
		_preview,
		_smoke,
		_tuMarker;
	Uint8 _markerColor;

	BattleUnit* _unit;
	MapData* _objects[4];

	Position _pos;

	std::vector<BattleItem*> _inventory;

	/// Gets if this Tile will accept '_smoke' value.
	bool canSmoke() const;
	/// Converts obscure inverse MCD notation to understandable percentages.
	int convertBurnToPCT(int burn) const;


	public:
		static const size_t PARTS_TILE = 4;
		static const int
			DR_NONE			= -1,
			DR_OPEN_WOOD	=  0,
			DR_OPEN_METAL	=  1,
			DR_WAIT_METAL	=  3,
			DR_ERR_TU		=  4,
			DR_ERR_RESERVE	=  5;

		static struct SerializationKey
		{
			// how many bytes to store for each variable or each member of array of the same name
			Uint8 index; // for indexing the actual tile array
			Uint8 _mapDataSetId;
			Uint8 _mapDataId;
			Uint8 _smoke;
			Uint8 _fire;
			Uint8 _animOffset;
			Uint8 boolFields;
			Uint32 totalBytes; // per structure including any data not mentioned here and accounting for all array members
		} serializationKey;

		/// Creates a tile.
		explicit Tile(const Position& pos);
		/// Cleans up a tile.
		~Tile();

		/// Loads the tile from yaml.
		void load(const YAML::Node& node);
		/// Loads the tile from binary buffer in memory.
		void loadBinary(
				Uint8* buffer,
				Tile::SerializationKey& serializationKey);
		/// Saves the tile to yaml.
		YAML::Node save() const;
		/// Saves the tile to binary
		void saveBinary(Uint8** buffer) const;

		/**
		 * Gets a pointer to MapData for a part of the Tile.
		 * @param part - the part 0-3
		 * @return, pointer to MapData
		 */
		MapData* getMapData(MapDataType part) const
		{ return _objects[part]; }

		/// Sets the pointer to the mapdata for a specific part of the Tile.
		void setMapData(
				MapData* const data,
				const int dataID,
				const int dataSetID,
				const MapDataType part);
		/// Gets the IDs of the mapdata for a specific part of the Tile.
		void getMapData(
				int* dataID,
				int* dataSetID,
				MapDataType part) const;

		/// Gets whether this tile has no objects
		bool isVoid(
				const bool testInventory = true,
				const bool testVolatiles = true) const;

		/// Gets the TU cost to walk over a certain part of the Tile.
		int getTuCostTile(
				MapDataType part,
				MovementType moveType) const;

		/// Checks if this tile has a floor.
		bool hasNoFloor(const Tile* const tileBelow = NULL) const;

		/// Checks if the Tile is a big wall.
		bool isBigWall() const;

		/// Gets terrain level.
		int getTerrainLevel() const;

		/**
		 * Gets the Tile's position.
		 * @return, position
		 */
		const Position& getPosition() const
		{ return _pos; }

		/// Gets the floor object footstep sound.
		int getFootstepSound(const Tile* const tileBelow) const;

		/// Opens a door.
		int openDoor(
				const MapDataType part,
				const BattleUnit* const unit = NULL);
//				const BattleActionType reserved = BA_NONE);
		/**
		 * Checks if the ufo door is open or opening.
		 * @note Used for visibility/light blocking checks. This function
		 * assumes that there never are 2 doors on 1 tile or a door and another
		 * wall on 1 tile.
		 * @param part		- the tile part to consider
		 * @return, bool	- true if ufo-door is valid and not closed
		 */
		bool isUfoDoorOpen(MapDataType part) const
		{	return _objects[part] != NULL
				&& _objects[part]->isUfoDoor() == true
				&& _curFrame[part] != 0; }
		/// Closes ufo door.
		int closeUfoDoor();

		/// Sets the black fog of war status of this tile.
		void setDiscovered(
				bool visible,
				int section);
		/// Gets the black fog of war status of this tile.
		bool isDiscovered(int section) const;

		/// Resets light to zero for this tile.
		void resetLight(size_t layer);
		/// Adds light to this tile.
		void addLight(
				int light,
				size_t layer);
		/// Gets the shade amount.
		int getShade() const;

		/// Destroys a tile part.
		bool destroy(
				MapDataType part,
				SpecialTileType type);
		/// Damages a tile part.
		bool damage(
				MapDataType part,
				int power,
				SpecialTileType type);

		/// Sets a virtual explosive on this tile to detonate later.
		void setExplosive(
				int power,
				int explType,
				bool force = false);
		/// Gets explosive power of this tile.
		int getExplosive() const;
		/// Gets explosive type of this tile.
		int getExplosiveType() const;

		/// Gets flammability.
		int getFlammability() const;
		/// Gets flammability of part.
		int getFlammability(MapDataType part) const;

		/// Gets turns to burn of part.
		int getFuel(MapDataType part = O_NULTYPE) const;

		/// Tries to start fire on this Tile.
		bool ignite(int power);
		/// Adds fire to this Tile.
		void addFire(int turns);
		/// Reduces the number of turns this Tile will burn.
		void decreaseFire();
		/// Gets fire.
		int getFire() const;	// kL_note: Made this inline, but may result in UB if say BattleUnit->getFire() conflicts.
//		{ return _fire; }		// So ... don't. ie: change function names, THANKS c++
								// ps. I changed the BattleUnit class function identifier to "getFireUnit" .....

		/// Adds smoke to this Tile.
		void addSmoke(int turns);
		/// Reduces the number of turns this Tile will smoke.
		void decreaseSmoke();
		/// Gets smoke.
		int getSmoke() const; // kL_note: Made this inline, but may result in UB if say BattleUnit->getFire() conflicts. So ... don't.
//		{ return _smoke; }

		/// Ends this tile's turn. Units catch on fire.
		void hitStuff(SavedBattleGame* const battleSave = NULL);

		/// Animates the tile parts.
		void animateTile();
		/// Gets fire and smoke animation offset.
		int getAnimationOffset() const;

		/// Gets object sprites.
		Surface* getSprite(int part) const;

		/// Sets a unit on this tile.
		void setUnit(
				BattleUnit* const unit,
				const Tile* const tileBelow = NULL);
		/// Sets a unit transitorily on this Tile.
		void setTransitUnit(BattleUnit* const unit);
		/**
		 * Gets the (alive) unit on this tile.
		 * @return, pointer to a BattleUnit
		 */
		BattleUnit* getUnit() const
		{ return _unit; }

		/// Adds item
		void addItem(
				BattleItem* const item,
				RuleInventory* const ground);
		/// Removes item
		void removeItem(BattleItem* const item);

		/// Gets corpse-sprite.
		int getCorpseSprite(bool* fired) const;
		/// Gets top-most item-sprite.
		int getTopSprite(bool* primed) const;
		/// Gets if the tile has an unconscious unit in its inventory.
		int hasUnconsciousUnit(bool playerOnly = true) const;

		/// Gets inventory on this tile.
		std::vector<BattleItem*>* getInventory();

		/// Sets the tile visible flag.
		void setTileVisible(bool vis = true);
		/// Gets the tile visible flag.
		bool getTileVisible() const;

		/// Sets the direction of path preview arrows.
		void setPreviewDir(int dir);
		/// Gets the direction of path preview arrows.
		int getPreviewDir() const;
		/// Sets the number to be displayed for path preview.
		void setPreviewTu(int tu);
		/// Gets the number to be displayed for path preview.
		int getPreviewTu() const;
		/// Sets the preview tile marker color.
		void setPreviewColor(Uint8 color);
		/// Gets the preview tile marker color.
		int getPreviewColor() const;

		/// Sets the danger flag on this tile so the AI may avoid it.
		void setDangerous(bool danger = true);
		/// Checks the danger flag on this tile.
		bool getDangerous() const;
};

}

#endif
