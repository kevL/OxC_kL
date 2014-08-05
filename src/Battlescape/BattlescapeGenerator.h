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

#ifndef OPENXCOM_BATTLESCAPEGENERATOR_H
#define OPENXCOM_BATTLESCAPEGENERATOR_H

#include <string> // kL

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
class ResourcePack;
class RuleItem;
class RuleTerrain;
class SavedBattleGame;
class TerrorSite;
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
		_baseCraftEquip, // kL
		_generateFuel;
	int
		_alienItemLevel,
		_craftX,
		_craftY,
		_craftZ,
		_mapsize_x,
		_mapsize_y,
		_mapsize_z,
//		_tankPos, // kL
		_unitSequence,
		_worldTexture,
		_worldShade;
	size_t _battleOrder; // kL

	AlienBase		* _alienBase;
	Base			* _base;
	Craft			* _craft;
	Game			* _game;
	ResourcePack	* _res;
	RuleTerrain		* _terrain;
	SavedBattleGame	* _save;
	TerrorSite		* _terror;
	Tile			* _craftInventoryTile;
	Ufo				* _ufo;

	std::string _alienRace;


	/// Generates a new battlescape map.
	void generateMap();
	/// Gets battlescape terrain.
	RuleTerrain* getTerrain(
			int tex,
			double lat);

	/// Loads an XCom MAP file.
	int loadMAP(
			MapBlock* mapblock,
			int xoff,
			int yoff,
			RuleTerrain* terrain,
			int objectIDOffset,
			bool discovered = false,
			bool craft = false);
	/// Loads an XCom RMP file.
	void loadRMP(
			MapBlock* mapblock,
			int xoff,
			int yoff,
			int segment);

	/// Deploys the XCOM units on the mission.
	void deployXCOM();
	/// Runs necessary checks before physically setting the position.
	bool canPlaceXCOMUnit(Tile* tile);
	/// Adds a vehicle to the game.
	BattleUnit* addXCOMVehicle(Vehicle* tank);
	/// Adds a soldier to the game.
	BattleUnit* addXCOMUnit(BattleUnit* unit);

	/// Loads a weapon on the inventoryTile.
	void loadGroundWeapon(BattleItem* item);
	/// Places an item on a soldier based on equipment layout.
	bool placeItemByLayout(BattleItem* item);
	/// Adds an item to a unit and the game.
	bool addItem(
			BattleItem* item,
			BattleUnit* unit);
//			bool allowSecondClip = false

	/// Deploys the aliens, according to the alien deployment rules.
	void deployAliens(
			AlienRace* race,
			AlienDeployment* deployment);
	/// Adds an alien to the game.
	BattleUnit* addAlien(
			Unit* rules,
			int alienRank,
			bool outside);

	/// Spawns civilians on a terror mission.
	void deployCivilians(int civilians);
	/// Adds a civlian to the game.
	BattleUnit* addCivilian(Unit* rules);

	/// Fills power sources with an alien fuel object.
	void fuelPowerSources();
	/// Possibly explodes ufo powersources.
	void explodePowerSources();


	public:
		/// Creates a new BattlescapeGenerator class
		BattlescapeGenerator(Game* game);
		/// Cleans up the BattlescapeGenerator.
		~BattlescapeGenerator();

		/// Sets the polygon texture.
		void setWorldTexture(int texture);
		/// Sets the polygon shade.
		void setWorldShade(int shade);

		/// Runs the generator.
		void run();
		/// Sets up the next stage (for cydonia/tftd terror missions).
		void nextStage();

		/// Sets the XCom craft.
		void setCraft(Craft* craft);
		/// Sets the ufo.
		void setUfo(Ufo* ufo);

		/// Sets the XCom base.
		void setBase(Base* base);
		/// Sets the terror site.
		void setTerrorSite(TerrorSite* site);
		/// Sets the alien base
		void setAlienBase(AlienBase* base);

		/// Sets the alien race.
		void setAlienRace(const std::string& alienRace);
		/// Sets the alien item level.
		void setAlienItemlevel(int alienItemLevel);

		/// Finds a spot near a friend to spawn at.
		bool placeUnitNearFriend(BattleUnit* unit);

		/// Generates an inventory battlescape.
		void runInventory(Craft* craft);

		/// Load all Xcom weapons.
		void loadWeapons();
};

}

#endif
