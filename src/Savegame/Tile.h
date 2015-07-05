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

//#include <string>
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
//class Particle;
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
		PARTS		= 4,
		SECTS		= 3;

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
//		_lastLight[LIGHTLAYERS],
		_light[LIGHTLAYERS],
		_mapDataID[4],
		_mapDataSetID[4],
		_overlapsINC,
		_overlapsSMK,
		_preview,
		_smoke,
		_tuMarker;
	Uint8 _markerColor;

	BattleUnit* _unit;
	MapData* _objects[4];

	Position _pos;

	std::vector<BattleItem*> _inventory;
//	std::list<Particle*> _particles;

	/// Gets if this Tile will accept '_smoke' value.
	bool canSmoke() const;
	/// Converts obscure inverse MCD notation to understandable percentages.
	int convertBurnToPCT(int burn) const;


	public:
//		static const int NOT_CALCULATED = -1;

		static struct SerializationKey
		{
			// how many bytes to store for each variable or each member of array of the same name
			Uint8 index; // for indexing the actual tile array
			Uint8 _mapDataSetID;
			Uint8 _mapDataID;
			Uint8 _smoke;
			Uint8 _fire;
			Uint8 _animOffset;
			Uint8 boolFields;
			Uint32 totalBytes; // per structure, including any data not mentioned here and accounting for all array members!
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
		 * Gets the MapData pointer of a part of the tile.
		 * @param part - the part 0-3
		 * @return, pointer to MapData
		 */
		MapData* getMapData(int part) const
		{ return _objects[part]; }

		/// Sets the pointer to the mapdata for a specific part of the tile.
		void setMapData(
				MapData* const data,
				const int dataID,
				const int dataSetID,
				const int part);
		/// Gets the IDs to the mapdata for a specific part of the tile.
		void getMapData(
				int* dataID,
				int* dataSetID,
				int part) const;

		/// Gets whether this tile has no objects
		bool isVoid(
				const bool checkInv = true,
				const bool checkSmoke = true) const;

		/// Gets the TU cost to walk over a certain part of the tile.
		int getTUCostTile(
				int part,
				MovementType moveType) const;

		/// Checks if this tile has a floor.
		bool hasNoFloor(const Tile* const tileBelow) const;

		/// Checks if this tile is a big wall.
		bool isBigWall() const;

		/// Gets terrain level.
		int getTerrainLevel() const;

		/**
		 * Gets the tile's position.
		 * @return, position
		 */
		const Position& getPosition() const
		{ return _pos; }

		/// Gets the floor object footstep sound.
		int getFootstepSound(const Tile* const tileBelow) const;

		/// Opens a door.
		int openDoor(
				const int part,
				const BattleUnit* const unit = NULL,
				const BattleActionType reserved = BA_NONE);
		/**
		 * Checks if the ufo door is open or opening. Used for visibility/light blocking checks.
		 * This function assumes that there never are 2 doors on 1 tile or a door and another wall on 1 tile.
		 * @param part	-
		 * @return bool	-
		 */
		bool isUfoDoorOpen(int part) const
		{	return _objects[part] != NULL
				&& _objects[part]->isUFODoor() == true
				&& _curFrame[part] != 0; }
//				&& _curFrame[part] != 4; } // <- not sure if 4 is ever relevant. It's not, see Tile::animate()
		/// Closes ufo door.
		int closeUfoDoor();

		/// Sets the black fog of war status of this tile.
		void setDiscovered(
				bool vis,
				int part);
		/// Gets the black fog of war status of this tile.
		bool isDiscovered(int part) const;

		/// Resets light to zero for this tile.
		void resetLight(size_t layer);
		/// Adds light to this tile.
		void addLight(
				int light,
				size_t layer);
		/// Gets the shade amount.
		int getShade() const;

		/// Destroys a tile part.
		bool destroy(int part);
		/// Damages a tile part.
		bool damage(
				int part,
				int power);

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
		int getFlammability(int part) const;

		/// Gets turns to burn of part.
		int getFuel(int part = -1) const;

		/// Tries to start fire on this Tile.
		bool ignite(int power);
		/// Adds fire to this Tile.
		void addFire(int turns);
		/// Reduces the number of turns this Tile will burn.
		void decreaseFire();
		/// Gets fire.
		int getFire() const;	// kL_note: Made this inline, but may result in UB if say BattleUnit->getFire() conflicts.
//		{ return _fire; }		// So ... don't. ie: change function names, THANKS c++
								// ps. I changed the BattleUnit class function identifier to "getFireOnUnit" .....

		/// Adds smoke to this Tile.
		void addSmoke(int turns);
		/// Reduces the number of turns this Tile will smoke.
		void decreaseSmoke();
		/// Gets smoke.
		int getSmoke() const; // kL_note: Made this inline, but may result in UB if say BattleUnit->getFire() conflicts. So ... don't.
//		{ return _smoke; }

		/// Gets how many times has this tile been overlapped with smoke (runtime only).
//		int getOverlapsSK() const;
		/// Gets how many times has this tile been overlapped with fire (runtime only).
//		int getOverlapsIN() const;
		/// New turn preparations.
//		void resolveOverlaps();

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

		/// Gets top-most item-sprite
		int getTopItemSprite(bool* ptrPrimed = NULL) const;
		/// Gets if the tile has an unconscious xCom unit in its inventory.
		int getHasUnconsciousSoldier() const;

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
		void setPreviewTU(int tu);
		/// Gets the number to be displayed for path preview.
		int getPreviewTU() const;
		/// Sets the preview tile marker color.
		void setPreviewColor(Uint8 color);
		/// Gets the preview tile marker color.
		int getPreviewColor() const;

		/// Sets the danger flag on this tile so the AI may avoid it.
		void setDangerous(bool danger = true);
		/// Checks the danger flag on this tile.
		bool getDangerous() const;

		/// Adds a particle to this tile's array.
//		void addParticle(Particle* particle);
		/// Gets a pointer to this tile's particle array.
//		std::list<Particle*>* getParticleCloud();
};

}

#endif
