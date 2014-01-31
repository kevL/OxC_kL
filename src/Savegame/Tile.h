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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_TILE_H
#define OPENXCOM_TILE_H

#include <string>
#include <vector>

#include <SDL_types.h> // for Uint8

#include "BattleUnit.h"

#include "../Battlescape/Position.h"
#include "../Ruleset/MapData.h"



namespace OpenXcom
{

class BattleItem;
class BattleUnit;
class MapData;
class RuleInventory;
class Surface;


/**
 * Basic element of which a battle map is build.
 * @sa http://www.ufopaedia.org/index.php?title=MAPS
 */
class Tile
{

protected:
	static const int LIGHTLAYERS = 3;

	bool
		_danger,
		_discovered[3];
	int
		_animOffset,
		_currFrame[4],
		_explosive,
		_fire,
		_lastLight[LIGHTLAYERS],
		_light[LIGHTLAYERS],
		_mapDataID[4],
		_mapDataSetID[4],
		_markerColor,
		_overlaps,
		_preview,
		_smoke,
		_tuMarker,
		_visible;

	BattleUnit* _unit;
	MapData* _objects[4];
	Position _pos;

	std::vector<BattleItem*> _inventory;


	public:
		static const int NOT_CALCULATED = -1;

		static struct SerializationKey
		{
			// how many bytes to store for each variable or each member of array of the same name
			Uint8 index; // for indexing the actual tile array
			Uint8 _mapDataSetID;
			Uint8 _mapDataID;
			Uint8 _smoke;
			Uint8 _fire;
			Uint8 _animOffset; // kL
			Uint8 boolFields;
			Uint32 totalBytes; // per structure, including any data not mentioned here and accounting for all array members!
		} serializationKey;

		/// Creates a tile.
		Tile(const Position& pos);
		/// Cleans up a tile.
		~Tile();

		/// Load the tile from yaml
		void load(const YAML::Node& node);
		/// Saves the tile to yaml
		YAML::Node save() const;

		/// Load the tile from binary buffer in memory
		void loadBinary(
				Uint8* buffer,
				Tile::SerializationKey& serializationKey);
		/// Saves the tile to binary
		void saveBinary(Uint8** buffer) const;

		/**
		 * Get the MapData pointer of a part of the tile.
		 * @param part the part 0-3.
		 * @return pointer to mapdata
		 */
		MapData* getMapData(int part) const
		{
			return _objects[part];
		}

		/// Sets the pointer to the mapdata for a specific part of the tile
		void setMapData(
				MapData* dat,
				int mapDataID,
				int mapDataSetID,
				int part);
		/// Gets the IDs to the mapdata for a specific part of the tile
		void getMapData(
				int* mapDataID,
				int* mapDataSetID,
				int part) const;

		/// Gets whether this tile has no objects
		bool isVoid() const;

		/// Get the TU cost to walk over a certain part of the tile.
		int getTUCost(
				int part,
				MovementType movementType) const;

		/// Checks if this tile has a floor.
		bool hasNoFloor(Tile* tileBelow) const;

		/// Checks if this tile is a big wall.
		bool isBigWall() const;

		/// Get terrain level.
		int getTerrainLevel() const;

		/**
		 * Gets the tile's position.
		 * @return position
		 */
		const Position& getPosition() const
		{
			return _pos;
		}

		/// Gets the floor object footstep sound.
		int getFootstepSound(Tile* tileBelow) const;

		/// Open a door, returns the ID, 0(normal), 1(ufo) or -1 if no door opened.
		int openDoor(
				int part,
				BattleUnit* Unit = 0,
				BattleActionType reserve = BA_NONE);
		/**
		 * Check if the ufo door is open or opening. Used for visibility/light blocking checks.
		 * This function assumes that there never are 2 doors on 1 tile or a door and another wall on 1 tile.
		 * @param part
		 * @return bool
		 */
		bool isUfoDoorOpen(int part) const
		{
			return _objects[part]
					&& _objects[part]->isUFODoor()
					&& _currFrame[part] != 0;
		}
		/// Close ufo door.
		int closeUfoDoor();

		/// Sets the black fog of war status of this tile.
		void setDiscovered(
				bool flag,
				int part);
		/// Gets the black fog of war status of this tile.
		bool isDiscovered(int part) const;

		/// Reset light to zero for this tile.
		void resetLight(int layer);
		/// Add light to this tile.
		void addLight(
				int light,
				int layer);
		/// Get the shade amount.
		int getShade() const;

		/// Destroy a tile part.
		bool destroy(int part);
		/// Damage a tile part.
		bool damage(int part, int power);

		/// Set a "virtual" explosive on this tile, to detonate later.
		void setExplosive(
				int power,
				bool force = false);
		/// Get explosive power of this tile.
		int getExplosive() const;

		/// Animated the tile parts.
		void animate();
		/// Get fire and smoke animation offset.
		int getAnimationOffset() const;

		/// Get object sprites.
		Surface* getSprite(int part) const;

		/// Set a unit on this tile.
		void setUnit(
				BattleUnit* unit,
				Tile* tileBelow = 0);

		/**
		 * Get the (alive) unit on this tile.
		 * @return BattleUnit.
		 */
		BattleUnit* getUnit() const
		{
			return _unit;
		}

		/// Set fire, does not increment overlaps.
		void setFire(int fire);
		/// Get fire.
		int getFire() const;
		/// Add smoke, increments overlap.
		void addSmoke(int smoke);
		/// Set smoke, does not increment overlaps.
		void setSmoke(int smoke);
		/// Get smoke.
		int getSmoke() const;

		/// Get flammability.
		int getFlammability() const;
		/// Get turns to burn
		int getFuel() const;
		/// attempt to set the tile on fire, sets overlaps to one if successful.
		void ignite(int power);

		/// Add item
		void addItem(
				BattleItem* item,
				RuleInventory* ground);
		/// Remove item
		void removeItem(BattleItem* item);

		/// Get top-most item-sprite
		int getTopItemSprite();

		/// New turn preparations.
		void prepareNewTurn();

		/// Get inventory on this tile.
		std::vector<BattleItem*>* getInventory();

		/// Set the tile marker color.
		void setMarkerColor(int color);
		/// Get the tile marker color.
		int getMarkerColor();

		/// Set the tile visible flag.
		void setVisible(int visibility);
		/// Get the tile visible flag.
		int getVisible();

		/// set the direction (used for path previewing)
		void setPreview(int dir);
		/// retrieve the direction stored by the pathfinding.
		int getPreview() const;
		/// set the number to be displayed for pathfinding preview.
		void setTUMarker(int tu);
		/// get the number to be displayed for pathfinding preview.
		int getTUMarker() const;

		/// how many times has this tile been overlapped with smoke/fire (runtime only)
		int getOverlaps() const;
		/// increment the overlap value on this tile.
		void addOverlap();

		/// set the danger flag on this tile (so the AI will avoid it).
		void setDangerous();
		/// check the danger flag on this tile.
		bool getDangerous();
};

}

#endif
