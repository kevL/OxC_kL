/*
 * Copyright 2010-2013 OpenXcom Developers.
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

#ifndef OPENXCOM_BATTLEUNIT_H
#define OPENXCOM_BATTLEUNIT_H

#include <string>
#include <vector>

#include "Soldier.h"

#include "../Battlescape/BattlescapeGame.h"
#include "../Battlescape/Position.h"

#include "../Ruleset/MapData.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Unit.h"


namespace OpenXcom
{

class AlienBAIState;
class Armor;
class BattleAIState;
class BattleItem;
class CivilianBAIState;
class Language;
class Node;
class RuleInventory;
class SavedGame;
class Soldier;
class Surface;
class Tile;
class Unit;


enum UnitStatus
{
	STATUS_STANDING,	// 0
	STATUS_WALKING,		// 1
	STATUS_FLYING,		// 2
	STATUS_TURNING,		// 3
	STATUS_AIMING,		// 4
	STATUS_COLLAPSING,	// 5
	STATUS_DEAD,		// 6
	STATUS_UNCONSCIOUS,	// 7
	STATUS_PANICKING,	// 8
	STATUS_BERSERK		// 9
};


enum UnitFaction
{
	FACTION_PLAYER,		// 0
	FACTION_HOSTILE,	// 1
	FACTION_NEUTRAL		// 2
};


enum UnitSide
{
	SIDE_FRONT,			// 0
	SIDE_LEFT,			// 1
	SIDE_RIGHT,			// 2
	SIDE_REAR,			// 3
	SIDE_UNDER			// 4
};


enum UnitBodyPart
{
	BODYPART_HEAD,		// 0
	BODYPART_TORSO,		// 1
	BODYPART_RIGHTARM,	// 2
	BODYPART_LEFTARM,	// 3
	BODYPART_RIGHTLEG,	// 4
	BODYPART_LEFTLEG	// 5
};


/**
 * Represents a moving unit in the battlescape, player controlled or AI controlled
 * it holds info about it's position, items carrying, stats, etc
 */
class BattleUnit
{

private:
	bool
		_cacheInvalid,
		_dontReselect,
		_floating,
		_hitByFire,
		_kneeled,
		_stopShot, // kL, to stop a unit from firing/throwing if it spots a new opponent during turning
		_visible;
	int
		_coverReserve,
		_currentArmor[5],
		_direction,
		_directionTurret,
		_energy,
		_expBravery,
		_expFiring,
		_expMelee,
		_expPsiSkill,
		_expReactions,
		_expThrowing,
		_faceDirection, // used only during strafeing moves
		_fallPhase,
		_fatalWounds[6],
		_fire,
		_health,
		_id,
		_kills,
		_morale,
		_moraleRestored,
		_motionPoints,
		_spinPhase,
		_stunlevel,
		_toDirection,
		_toDirectionTurret,
		_tu,
		_turnsExposed,
		_verticalDirection,
		_walkPhase;

	BattleAIState* _currentAIState;
	BattleUnit* _charging;
	Position
		_lastPos,
		_pos,
		_destination;
	Surface* _cache[5];
	Tile* _tile;
	UnitFaction
		_faction,
		_killedBy,
		_originalFaction;
	UnitStatus _status;

	std::vector<BattleItem*> _inventory;
	std::vector<BattleUnit*>
		_visibleUnits,
		_unitsSpottedThisTurn;
	std::vector<Tile*> _visibleTiles;

	std::string
		_activeHand,
		_spawnUnit,
		_zombieUnit;

	// static data
	UnitStats _stats;

	int
		_aggression,
		_aggroSound,
		_deathSound,
		_floatHeight,
		_kneelHeight,
		_intelligence,
		_moveSound,
		_rankInt,
		_standHeight,
		_turretType,
		_value;

	std::string
		_race,
		_rank,
		_type;

	std::wstring _name;

	std::vector<int> _loftempsSet;

	Armor* _armor;
	Soldier* _geoscapeSoldier;
	SoldierGender _gender;
	SpecialAbility _specab;
	Unit* _unitRules;

	/// Converts an amount of experience to a stat increase.
	int improveStat(int exp);


	public:
		static const int MAX_SOLDIER_ID = 1000000;

		// scratch value for AI's left hand to tell its right hand what's up...
		// don't zone out and start patrolling again
		bool _hidingForTurn;
		Position lastCover;

		/// Creates a BattleUnit.
		BattleUnit(
				Soldier* soldier,
				UnitFaction faction);
		BattleUnit(
				Unit* unit,
				UnitFaction faction,
				int id,
				Armor* armor,
				int diff);
		/// Cleans up the BattleUnit.
		~BattleUnit();

		/// Loads the unit from YAML.
		void load(const YAML::Node& node);
		/// Saves the unit to YAML.
		YAML::Node save() const;

		/// Gets the BattleUnit's ID.
		int getId() const;

		/// Sets the unit's position
		void setPosition(
				const Position& pos,
				bool updateLastPos = true);
		/// Gets the unit's position.
		const Position& getPosition() const;
		/// Gets the unit's position.
		const Position& getLastPosition() const;

		/// Sets the unit's direction 0-7.
		void setDirection(int direction);
		/// Gets the unit's direction.
		int getDirection() const;
		/// Sets the unit's face direction (only used by strafing moves)
		void setFaceDirection(int direction);
		/// Gets the unit's face direction (only used by strafing moves)
		int getFaceDirection() const;
		/// Gets the unit's turret direction.
		int getTurretDirection() const;
		/// Gets the unit's turret To direction.
		int getTurretToDirection() const;
		/// Gets the unit's vertical direction.
		int getVerticalDirection() const;

		/// Gets the unit's status.
		UnitStatus getStatus() const;
		/// kL. Sets a unit's status.
		void setStatus(int status);

		/// Start the walkingPhase
		void startWalking(
				int direction,
				const Position& destination,
				Tile* tileBelow,
				bool cache);
		/// Increase the walkingPhase
		void keepWalking(
				Tile* tileBelow,
				bool cache);
		/// Gets the walking phase for animation and sound
		int getWalkingPhase() const;
		/// Gets the walking phase for diagonal walking
		int getDiagonalWalkingPhase() const;

		/// Gets the unit's destination when walking
		const Position& getDestination() const;

		/// Look at a certain point.
		void lookAt(
				const Position& point,
				bool turret = false);
		/// Look at a certain direction.
		void lookAt(
				int direction,
				bool force = false);
		/// Turn to the destination direction.
		void turn(bool turret = false);
		/// Abort turning.
//kL		void abortTurn();

		/// Gets the soldier's gender.
		SoldierGender getGender() const;

		/// Gets the unit's faction.
		UnitFaction getFaction() const;

		/// Set the cached flag.
		void setCache(
				Surface* cache,
				int part = 0);
		/// If this unit is cached on the battlescape.
		Surface* getCache(
				bool* invalid,
				int part = 0) const;

		/// Kneel down.
		void kneel(bool kneeled);
		/// Is kneeled?
		bool isKneeled() const;

		/// Is floating?
		bool isFloating() const;

		/// Aim.
		void aim(bool aiming);

		/// Get direction to a certain point
		int directionTo(const Position& point) const;

		/// Gets the unit's time units.
		int getTimeUnits() const;
		/// Gets the unit's stamina.
		int getEnergy() const;
		/// Gets the unit's health.
		int getHealth() const;
		/// Gets the unit's bravery.
		int getMorale() const;

		/// Do damage to the unit.
		int damage(
				const Position& relative,
				int power,
				ItemDamageType type,
				bool ignoreArmor = false);

		/// Heal stun level of the unit.
		void healStun(int power);
		/// Gets the unit's stun level.
		int getStunlevel() const;

		/// Knocks the unit out instantly.
		void knockOut(BattlescapeGame* battle);

		/// Start falling sequence. This is only for collapsing dead or unconscious units.
		void startFalling();
		/// Increment the falling sequence.
		void keepFalling();
		/// Get falling sequence.
		int getFallingPhase() const;

		/// The unit is out - either dead or unconscious.
//kL		bool isOut() const;
		bool isOut(
				bool checkHealth = false,
				bool checkStun = false) const; // kL

		/// Get the number of time units a certain action takes.
		int getActionTUs(
				BattleActionType actionType,
				BattleItem* item);
		/// Spend time units if it can.
		bool spendTimeUnits(int tu);
		/// Spend energy if it can.
		bool spendEnergy(int energy);
		/// Set time units.
		void setTimeUnits(int tu);
		/// Sets the unit's energy level.
		void setEnergy(int energy);

		/// Set whether this unit is visible
		void setVisible(bool flag);
		/// Get whether this unit is visible
		bool getVisible() const;
		/// Add unit to visible units.
		bool addToVisibleUnits(BattleUnit* unit);
		/// Get the list of visible units.
		std::vector<BattleUnit*>* getVisibleUnits();
		/// Clear visible units.
		void clearVisibleUnits();

		/// Add unit to visible tiles.
		bool addToVisibleTiles(Tile* tile);
		/// Get the list of visible tiles.
		std::vector<Tile*>* getVisibleTiles();
		/// Clear visible tiles.
		void clearVisibleTiles();

		/// Calculate firing accuracy.
		double getFiringAccuracy(
				BattleActionType actionType,
				BattleItem* item);
		/// Calculate accuracy modifier.
		double getAccuracyModifier();
		/// Calculate throwing accuracy.
		double getThrowingAccuracy();

		/// Set armor value.
		void setArmor(
				int armor,
				UnitSide side);
		/// Gets the unit's armor.
		Armor* getArmor() const;
		/// Get armor value.
		int getArmor(UnitSide side) const;

		/// Get total number of fatal wounds.
		int getFatalWounds() const;

		/// Get the current reaction score.
		double getInitiative();

		/// Prepare for a new turn.
		void prepareNewTurn();

		/// Morale change
		void moraleChange(int change);

		/// Don't reselect this unit
		void dontReselect();
		/// Reselect this unit
		void allowReselect();
		/// Check whether reselecting this unit is allowed.
		bool reselectAllowed() const;

		/// Set fire.
		void setFire(int fire);
		/// Get fire.
		int getFire() const;

		/// Get the list of items in the inventory.
		std::vector<BattleItem*>* getInventory();

		/// Let AI do their thing.
		void think(BattleAction* action);
		/// Get current AI state.
		BattleAIState* getCurrentAIState() const;
		/// Set next AI State
		void setAIState(BattleAIState* aiState);

		/// Sets the unit's tile it's standing on
		void setTile(
				Tile* tile,
				Tile* tileBelow = 0);
		/// Gets the unit's tile.
		Tile* getTile() const;

		/// Gets the item in the specified slot.
		BattleItem* getItem(
				RuleInventory* slot,
				int x = 0,
				int y = 0) const;
		/// Gets the item in the specified slot.
		BattleItem* getItem(
				const std::string& slot,
				int x = 0,
				int y = 0) const;
		/// Gets the item in the main hand.
		BattleItem* getMainHandWeapon(bool quickest = true) const;
		/// Gets a grenade from the belt, if any.
		BattleItem* getGrenadeFromBelt() const;

		/// Reloads righthand weapon if needed.
		bool checkAmmo();

		/// Check if this unit is in the exit area
		bool isInExitArea(SpecialTileType stt = START_POINT) const;

		/// Gets the unit height taking into account kneeling/standing.
		int getHeight() const;
		/// Gets the unit floating elevation.
		int getFloatHeight() const;

		/// Adds one to the reaction exp counter.
		void addReactionExp();
		/// Adds one to the firing exp counter.
		void addFiringExp();
		/// Adds one to the throwing exp counter.
		void addThrowingExp();
		/// Adds qty to the psi exp counter.
		void addPsiExp(int qty = 1); // kL
		/// Adds one to the melee exp counter.
		void addMeleeExp();

		/// Updates the stats of a Geoscape soldier.
		void updateGeoscapeStats(Soldier* soldier);

		/// Check if unit eligible for squaddie promotion.
		bool postMissionProcedures(SavedGame* geoscape);

		/// Get the sprite index for the minimap
		int getMiniMapSpriteIndex() const;

		/// Set the turret type. -1 is no turret.
		void setTurretType(int turretType);
		/// Get the turret type. -1 is no turret.
		int getTurretType() const;

		/// Get fatal wound amount of a body part
		int getFatalWound(int part) const;
		/// Heal one fatal wound
		void heal(
				int part,
				int woundAmount,
				int healthAmount);
		/// Give pain killers to this unit
		void painKillers();
		/// Give stimulant to this unit
		void stimulant(
				int energy,
				int stun);

		/// Get motion points for the motion scanner.
		int getMotionPoints() const;

		/// Gets the unit's name.
		std::wstring getName(
				Language* lang,
				bool debugAppendId = false) const;

		/// Gets the unit's stats.
		UnitStats* getStats();

		/// Get the unit's stand height.
		int getStandHeight() const;
		/// Get the unit's kneel height.
		int getKneelHeight() const;

		/// Get the unit's loft ID.
		int getLoftemps(int entry = 0) const;

		/// Get the unit's value.
		int getValue() const;

		/// Get the unit's death sound.
		int getDeathSound() const;
		/// Get the unit's move sound.
		int getMoveSound() const;
		/// Gets the unit's aggro sound.
		int getAggroSound() const;

		/// Get whether the unit is affected by fatal wounds.
		bool isWoundable() const;
		/// Get whether the unit is affected by fear.
		bool isFearable() const;

		/// Get the unit's intelligence.
		int getIntelligence() const;
		/// Get the unit's aggression.
		int getAggression() const;

		/// Get the unit's special ability.
		int getSpecialAbility() const;
		/// Set the unit's special ability.
		void setSpecialAbility(SpecialAbility specab);

		/// Get the unit's rank string.
		std::string getRankString() const;
		/// Get the unit's race string.
		std::string getRaceString() const; // kL

		/// Get the geoscape-soldier object.
		Soldier* getGeoscapeSoldier() const;

		/// Add a kill to the counter.
		void addKillCount();

		/// Get unit type.
		std::string getType() const;

		/// Set the hand this unit is using;
		void setActiveHand(const std::string& slot);
		/// Get unit's active hand.
		std::string getActiveHand() const;

		/// Get this unit's original faction
		UnitFaction getOriginalFaction() const;
		/// Convert's unit to a faction
		void convertToFaction(UnitFaction f);

		/// Set health to 0 and set status dead
		void instaKill();

		/// Gets the unit's zombie unit.
		std::string getZombieUnit() const;
		/// Gets the unit's spawn unit.
		std::string getSpawnUnit() const;
		/// Sets the unit's spawn unit.
		void setSpawnUnit(std::string spawnUnit);

		/// Get the faction that killed this unit.
		UnitFaction killedBy() const;
		/// Set the faction that killed this unit.
		void killedBy(UnitFaction f);

		/// Set the units we are charging towards.
		void setCharging(BattleUnit* chargeTarget);
		/// Get the units we are charging towards.
		BattleUnit* getCharging();

		/// Get the carried weight in strength units.
		int getCarriedWeight(BattleItem* draggingItem = 0) const;

		/// Set how many turns this unit will be exposed for.
		void setTurnsExposed(int turns);
		/// Set how many turns this unit will be exposed for.
		int getTurnsExposed() const;

		/// call this after the default copy constructor deletes the cache
		void invalidateCache();

		///
		Unit* getUnitRules() const
		{
			return _unitRules;
		}

		/// get the vector of units we've seen this turn.
		std::vector<BattleUnit*> getUnitsSpottedThisTurn();

		/// set the rank integer
		void setRankInt(int rank);
		/// get the rank integer
		int getRankInt() const;
		/// derive a rank integer based on rank string (for xcom soldiers ONLY)
		void deriveRank();

		/// this function checks if a tile is visible, using maths.
		bool checkViewSector(Position pos) const;

		/// adjust this unit's stats according to difficulty.
		void adjustStats(const int diff);
		/// Halve the unit's armor values.
		void halveArmor();

		/// did this unit already take fire damage this turn? (used to avoid damaging large units multiple times.)
		bool tookFireDamage() const;

		/// switch the state of the fire damage tracker.
		void toggleFireDamage();

		///
		void setCoverReserve(int reserve);
		///
		int getCoverReserve() const;

		// kL_begin:
		/// Initializes a death spin.
		void initDeathSpin();
		/// Continues a death spin.
		void contDeathSpin();
		/// Regulates init, direction & duration of the death spin-cycle.
		int getSpinPhase() const;
		/// Sets the spinPhase of a unit.
		void setSpinPhase(int spinphase);

		/// Set a unit to STATUS_UNCONSCIOUS.
		void knockOut();

		/// Sets a unit's health level.
		void setHealth(int health);

		/// to stop a unit from firing/throwing if it spots a new opponent during turning
		void setStopShot(bool stop);
		/// to stop a unit from firing/throwing if it spots a new opponent during turning
		bool getStopShot() const;
		// kL_end.

		/// Is this unit selectable?
		bool isSelectable(
				UnitFaction faction,
				bool checkReselect = false,
				bool checkInventory = false) const;

		/// Does this unit have an inventory?
		bool hasInventory() const;
};

}

#endif
