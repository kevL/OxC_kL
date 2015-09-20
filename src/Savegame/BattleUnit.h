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

#ifndef OPENXCOM_BATTLEUNIT_H
#define OPENXCOM_BATTLEUNIT_H

//#include <string>
//#include <vector>

#include "SavedGame.h" // GameDifficulty enum
#include "Soldier.h"

#include "../Battlescape/BattlescapeGame.h"
#include "../Battlescape/Position.h"

#include "../Ruleset/MapData.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/RuleUnit.h"


namespace OpenXcom
{

class AlienBAIState;
class BattleAIState;
class BattleItem;
class BattlescapeGame;
class CivilianBAIState;
class Language;
class Node;
class RuleArmor;
class RuleInventory;
class RuleUnit;
class SavedBattleGame;
class SavedGame;
class Soldier;
class Surface;
class Tile;


enum UnitStatus
{
	STATUS_STANDING,	//  0
	STATUS_WALKING,		//  1
	STATUS_FLYING,		//  2
	STATUS_TURNING,		//  3
	STATUS_AIMING,		//  4
	STATUS_COLLAPSING,	//  5
	STATUS_DEAD,		//  6
	STATUS_UNCONSCIOUS,	//  7
	STATUS_PANICKING,	//  8
	STATUS_BERSERK,		//  9
	STATUS_LIMBO,		// 10 won't participate in a 'next stage' battle.
	STATUS_DISABLED		// 11 dead or unconscious but doesn't know it yet.
};

enum UnitFaction
{
	FACTION_NONE = -1,	//-1
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
	BODYPART_LEFTLEG,	// 5
	BODYPART_NONE		// 6
};

enum OutCheck
{
	OUT_ALL,		// 0
	OUT_STAT,		// 1
	OUT_HLTH,		// 2
	OUT_STUN,		// 3
	OUT_HLTH_STUN	// 4
};


/**
 * SoldierDiary container for battle unit kills statistics.
 */
struct BattleUnitKills
{
	std::string
		_rank,
		_race,
		_weapon,
		_weaponAmmo;
	int
		_mission,
		_turn,
		_points;

	UnitFaction _faction;
	UnitStatus _status;


	/// Makes turn unique across all kills.
	void makeTurnUnique()
	{
		_turn += _mission * 300; // Maintains divisibility by 3 as well.
	}

	/// Checks to see if turn was on HOSTILE side.
	bool hostileTurn() const
	{
		if ((_turn - 1) % 3 == 0)
			return true;

		return false;
	}

	///
	void load(const YAML::Node &node)
	{
		_rank		= node["rank"]					.as<std::string>(_rank);
		_race		= node["race"]					.as<std::string>(_race);
		_weapon		= node["weapon"]				.as<std::string>(_weapon);
		_weaponAmmo	= node["weaponAmmo"]			.as<std::string>(_weaponAmmo);
		_mission	= node["mission"]				.as<int>(_mission);
		_turn		= node["turn"]					.as<int>(_turn);
		_points		= node["points"]				.as<int>(_points);
		_status		= static_cast<UnitStatus>(node["status"]	.as<int>());
		_faction	= static_cast<UnitFaction>(node["faction"]	.as<int>());
	}

	///
	YAML::Node save() const
	{
		YAML::Node node;

		node["rank"]		= _rank;
		node["race"]		= _race;
		node["weapon"]		= _weapon;
		node["weaponAmmo"]	= _weaponAmmo;
		node["mission"]		= _mission;
		node["turn"]		= _turn;
		node["points"]		= _points;
		node["status"]		= static_cast<int>(_status);
		node["faction"]		= static_cast<int>(_faction);

		return node;
	}

	/// Converts victim Status to string.
	std::string getUnitStatusString() const
	{
		switch (_status)
		{
			case STATUS_DEAD:			return "STATUS_DEAD";
			case STATUS_UNCONSCIOUS:	return "STATUS_UNCONSCIOUS";
		}

		return "status error";
	}

	/// Converts victim Faction to string.
	std::string getUnitFactionString() const
	{
		switch (_faction)
		{
			case FACTION_NONE:		return "FACTION_NONE";
			case FACTION_PLAYER:	return "FACTION_PLAYER";
			case FACTION_HOSTILE:	return "FACTION_HOSTILE";
			case FACTION_NEUTRAL:	return "FACTION_NEUTRAL";
		}

		return "faction error";
	}

	///
	explicit BattleUnitKills(const YAML::Node& node)
	{
		load(node);
	}

	/// cTor.
	BattleUnitKills(
			std::string unitRank,
			std::string race,
			std::string weapon,
			std::string weaponAmmo,
			UnitFaction faction,
			UnitStatus status,
			int mission,
			int turn,
			int points)
		:
			_rank(unitRank),
			_race(race),
			_weapon(weapon),
			_weaponAmmo(weaponAmmo),
			_faction(faction),
			_status(status),
			_mission(mission),
			_turn(turn),
			_points(points)
	{}

	/// dTor.
	~BattleUnitKills()
	{}
};


/**
 * Container for BattleUnitStatistics.
 */
struct BattleUnitStatistics
{
	bool
		ironMan,		// Tracks if the soldier was the only soldier on the mission
		KIA,			// Tracks if the soldier was killed in battle
		loneSurvivor,	// Tracks if the soldier was the only survivor
		MIA,			// Tracks if the soldier was left behind :(
		nikeCross,		// Tracks if a soldier killed every alien
		wasUnconscious;	// Tracks if the soldier fell unconscious

	int
		daysWounded,			// Tracks how many days the unit was wounded for
		hitCounter,				// Tracks how many times the unit was hit
		longDistanceHitCounter,	// Tracks how many long distance shots were landed
		lowAccuracyHitCounter,	// Tracks how many times the unit landed a low probability shot
		shotAtCounter,			// Tracks how many times the unit was shot at
		shotByFriendlyCounter,	// Tracks how many times the unit was hit by a friendly
		shotFriendlyCounter,	// Tracks how many times the unit was hit a friendly
		shotsFiredCounter,		// Tracks how many times a unit has shot
		shotsLandedCounter,		// Tracks how many times a unit has hit his target
		medikitApplications,	// Tracks how many times a unit has used the medikit
		revivedSoldier;			// Tracks how many times this soldier revived another unit

	std::vector<BattleUnitKills*> kills; // Tracks kills


	/// Checks if unit has fired on a friendly.
	bool hasFriendlyFired() const
	{
		for (std::vector<BattleUnitKills*>::const_iterator
				i = kills.begin();
				i != kills.end();
				++i)
		{
			if ((*i)->_faction == FACTION_PLAYER)
				return true;
		}

		return false;
	}

	/// Gets if unit has killed or stunned a hostile.
	bool hasKillOrStun() const
	{
		for (std::vector<BattleUnitKills*>::const_iterator
				i = kills.begin();
				i != kills.end();
				++i)
		{
			if ((*i)->_faction == FACTION_HOSTILE
				&& ((*i)->_status == STATUS_DEAD
					|| (*i)->_status == STATUS_UNCONSCIOUS))
			{
				return true;
			}
		}

		return false;
	}

	/// Checks if these statistics are default.
	bool statsDefault() const
	{
		if (kills.empty() == true
			&& wasUnconscious == false
			&& shotAtCounter == 0
			&& hitCounter == 0
			&& shotByFriendlyCounter == 0
			&& shotFriendlyCounter == 0
			&& longDistanceHitCounter == 0
			&& lowAccuracyHitCounter == 0
			&& shotsFiredCounter == 0
			&& shotsLandedCounter == 0
			&& medikitApplications == 0
			&& revivedSoldier == 0)
		{
			return true;
		}

		return false;
	}

	///
	void load(const YAML::Node& node)
	{
		if (const YAML::Node& YAMLkills = node["kills"])
		{
			for (YAML::const_iterator
					i = YAMLkills.begin();
					i != YAMLkills.end();
					++i)
			{
				kills.push_back(new BattleUnitKills(*i));
			}
		}

		wasUnconscious			= node["wasUnconscious"]		.as<bool>(wasUnconscious);
		shotAtCounter			= node["shotAtCounter"]			.as<int>(shotAtCounter);
		hitCounter				= node["hitCounter"]			.as<int>(hitCounter);
		shotByFriendlyCounter	= node["shotByFriendlyCounter"]	.as<int>(shotByFriendlyCounter);
		shotFriendlyCounter		= node["shotFriendlyCounter"]	.as<int>(shotFriendlyCounter);
		longDistanceHitCounter	= node["longDistanceHitCounter"].as<int>(longDistanceHitCounter);
		lowAccuracyHitCounter	= node["lowAccuracyHitCounter"]	.as<int>(lowAccuracyHitCounter);
		shotsFiredCounter		= node["shotsFiredCounter"]		.as<int>(shotsFiredCounter);
		shotsLandedCounter		= node["shotsLandedCounter"]	.as<int>(shotsLandedCounter);
		medikitApplications		= node["medikitApplications"]	.as<int>(medikitApplications);
		revivedSoldier			= node["revivedSoldier"]		.as<int>(revivedSoldier);
/*		ironMan =
		loneSurvivor =
		nikeCross =
		KIA =
		MIA = false;
		daysWounded = 0; */
	}

	///
	YAML::Node save() const
	{
		YAML::Node node;

		if (kills.empty() == false)
		{
			for (std::vector<BattleUnitKills*>::const_iterator
					i = kills.begin();
					i != kills.end();
					++i)
			{
				node["kills"].push_back((*i)->save());
			}
		}

		if (wasUnconscious != false)		node["wasUnconscious"]			= wasUnconscious;
		if (shotAtCounter != 0)				node["shotAtCounter"]			= shotAtCounter;
		if (hitCounter != 0)				node["hitCounter"]				= hitCounter;
		if (shotByFriendlyCounter != 0)		node["shotByFriendlyCounter"]	= shotByFriendlyCounter;
		if (shotFriendlyCounter != 0)		node["shotFriendlyCounter"]		= shotFriendlyCounter;
		if (longDistanceHitCounter != 0)	node["longDistanceHitCounter"]	= longDistanceHitCounter;
		if (lowAccuracyHitCounter != 0)		node["lowAccuracyHitCounter"]	= lowAccuracyHitCounter;
		if (shotsFiredCounter != 0)			node["shotsFiredCounter"]		= shotsFiredCounter;
		if (shotsLandedCounter != 0)		node["shotsLandedCounter"]		= shotsLandedCounter;
		if (medikitApplications != 0)		node["medikitApplications"]		= medikitApplications;
		if (revivedSoldier != 0)			node["revivedSoldier"]			= revivedSoldier;

		return node;
	}

	///
	explicit BattleUnitStatistics(const YAML::Node& node)
	{
		load(node);
	}

	/// cTor.
	BattleUnitStatistics()
		:
			wasUnconscious(false),
			shotAtCounter(0),
			hitCounter(0),
			shotByFriendlyCounter(0),
			shotFriendlyCounter(0),
			longDistanceHitCounter(0),
			lowAccuracyHitCounter(0),
			shotsFiredCounter(0),
			shotsLandedCounter(0),
			medikitApplications(0),
			revivedSoldier(0),
			ironMan(false), // ->> calculated at end of Tactical, do not save ->
			loneSurvivor(false),
			nikeCross(false),
			KIA(false),
			MIA(false),
			daysWounded(0)
	{}

	/// dTor.
	~BattleUnitStatistics()
	{}
};


/**
 * This holds info about a mobile object in the battlescape whether controlled
 * by player or AI.
 */
class BattleUnit // no copy cTor.
{

	public:
		static const size_t PARTS_BODY = 6;

private:
	static const size_t PARTS_ARMOR	= 5; // doubles as both armorValues and sprites' cache
	static const int DOSE_LETHAL	= 3;
//	static const int SPEC_WEAPON_MAX = 3;

	bool
		_aboutToDie,
		_cacheInvalid,
		_dashing,
		_diedByFire,
		_dontReselect,
		_floating,
		_hidingForTurn,
		_kneeled,
		_revived,
		_stopShot, // to stop a unit from firing/throwing if it spots a new opponent during turning
		_takenExpl, // used to stop large units from taking damage for each part.
		_takenFire,
		_visible;
	int
		_aimPhase,
		_coverReserve,
		_armorHp[PARTS_ARMOR],
		_dir,
		_dirTurret,
		_dirTo,
		_dirToTurret,
		_dirFace, // used only during strafing moves
		_dirTurn, // used for determining 180 degree turn direction
		_dirVertical,
		_energy,
		_expBravery,
		_expFiring,
		_expMelee,
		_expPsiSkill,
		_expPsiStrength,
		_expReactions,
		_expThrowing,
		_fallPhase,
		_fatalWounds[PARTS_BODY],
		_fire,
		_health,
		_id,
		_kills,
		_morale,
		_moraleRestored,
		_motionPoints,
		_drugDose,
		_spinPhase,
		_stunLevel,
		_tu,
		_turnsExposed,
		_walkPhase,
		_mcStrength,
		_mcSkill;
	size_t _battleOrder;

	BattleAIState* _currentAIState;
//	BattleItem* _specWeapon[SPEC_WEAPON_MAX];
	BattlescapeGame* _battleGame;
	BattleUnit* _charging;
	Surface* _cache[5];
	Tile* _tile;

	Position
		_pos,
		_posLast,
		_posDest;
	UnitFaction
		_faction,
		_originalFaction,
		_killedBy;
	UnitStatus _status;

	std::list<BattleUnit*> _rfSpotters;

	std::vector<int> _spottedId; // for saving/loading '_hostileUnitsThisTurn'

	std::vector<BattleItem*> _inventory;
	std::vector<BattleUnit*>
		_hostileUnits,
		_hostileUnitsThisTurn;
//	std::vector<Tile*> _visibleTiles;

	std::string
		_activeHand,
		_spawnUnit;

	// static data
	UnitStats _stats;

	bool _isZombie;

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

	std::vector<size_t> _loftSet;
	std::vector<std::pair<Uint8, Uint8> > _recolor;

	const RuleArmor* _armor;
	Soldier* _geoscapeSoldier;
	RuleUnit* _unitRule;

	MovementType _moveType;
	SoldierGender _gender;
	SpecialAbility _specab;

	BattleUnitStatistics* _statistics;
	int _murdererId; // used to credit the murderer with the kills that this unit got by blowing up on death


	/// Converts an amount of experience to a stat increase.
	int improveStat(int xp);

	/// Helper function initing recolor vector.
	void setRecolor(
			int basicLook,
			int utileLook,
			int rankLook);

	/// Gets if a grenade is suitable for an AI or panic situation.
	bool isGrenadeSuitable(const BattleItem* const grenade) const;


	public:
		static const int MAX_SOLDIER_ID = 1000000;

		// scratch value for AI's left hand to tell its right hand what's up...
		// don't zone out and start patrolling again
		Position _lastCover;


		/// Creates a BattleUnit from a geoscape Soldier.
		BattleUnit( // xCom operatives
				Soldier* const soldier,
				const GameDifficulty diff); // for VictoryPts value per death.
		/// Creates a BattleUnit from Unit rule.
		BattleUnit( // aLiens, civies, & Tanks
				RuleUnit* const unitRule,
				const UnitFaction faction,
				const int id,
				RuleArmor* const armor,
				const GameDifficulty diff,
				const int month = 0, // for upping aLien stats as time progresses.
				BattlescapeGame* const battleGame = NULL);
		/// Cleans up the BattleUnit.
		~BattleUnit();

		/// Loads this unit from YAML.
		void load(const YAML::Node& node);
		/// Loads the vector of units spotted this turn during SavedBattleGame load.
		void loadSpotted(SavedBattleGame* const battleSave);
		/// Saves this unit to YAML.
		YAML::Node save() const;

		/// Gets this BattleUnit's ID.
		int getId() const;

		/// Sets this unit's position.
		void setPosition(
				const Position& pos,
				bool updateLast = true);
		/// Gets this unit's position.
		const Position& getPosition() const;
		/// Gets this unit's position.
		const Position& getLastPosition() const;

		/// Sets this unit's direction 0-7.
		void setDirection(
				int dir,
				bool turret = true);
		/// Gets this unit's direction.
		int getDirection() const;
		/// Sets this unit's face direction - only used by strafing moves.
		void setFaceDirection(int dir);
		/// Gets this unit's face direction - only used by strafing moves.
		int getFaceDirection() const;
		/// Gets this unit's turret direction.
		int getTurretDirection() const;
		/// Sets this unit's turret direction.
		void setTurretDirection(int dir);
		/// Gets this unit's turret To direction.
		int getTurretToDirection() const;
		/// Gets this unit's vertical direction.
		int getVerticalDirection() const;

		/// Gets this unit's status.
		UnitStatus getUnitStatus() const;
		/// Sets this unit's status.
		void setUnitStatus(const UnitStatus status);

		/// Starts the walkingPhase.
		void startWalking(
				int dir,
				const Position& dest,
				const Tile* const tileBelow);
		/// Advances the walkingPhase.
		void keepWalking(
				const Tile* const tileBelow,
				bool recache);
		/// Calculates the mid- and end-phases of walking.
		void walkPhaseCutoffs(
				int& halfPhase,
				int& fullPhase) const;
		/// Gets the walking phase for animation and sound.
		int getWalkPhase() const;
		/// Gets the walking phase for diagonal walking.
		int getDiagonalWalkPhase() const;

		/// Gets this unit's destination when walking.
		const Position& getDestination() const;

		/// Looks at a certain point.
		void lookAt(
				const Position& point,
				bool turret = false);
		/// Looks in a certain direction.
		void lookAt(
				int direction,
				bool force = false);
		/// Turns to the destination direction.
		void turn(bool turret = false);

		/// Gets the soldier's gender.
		SoldierGender getGender() const;
		/// Gets the unit's faction.
		UnitFaction getFaction() const;

		/// Sets this unit's cache and the cached flag.
		void setCache(
				Surface* const cache,
				int quadrant = 0);
		/// Gets this unit's cache for the battlescape.
		Surface* getCache(
				bool* invalid,
				int quadrant = 0) const;
		/// Gets unit sprite recolors values.
		const std::vector<std::pair<Uint8, Uint8> >& getRecolor() const;

		/// Kneels or stands this unit.
		void kneel(bool kneel);
		/// Gets if this unit is kneeled.
		bool isKneeled() const;

		/// Gets if this unit is floating.
		bool isFloating() const;

		/// Aims this unit's weapon.
		void aim(bool aim = true);

		/// Gets direction to a certain point.
		int directionTo(const Position& point) const;

		/// Gets this unit's time units.
		int getTimeUnits() const;
		/// Gets this unit's stamina.
		int getEnergy() const;
		/// Gets this unit's health.
		int getHealth() const;
		/// Gets this unit's bravery.
		int getMorale() const;
		/// Gets this unit's effective strength.
		int getStrength() const;

		/// Do damage to this unit.
		int damage(
				const Position& relPos,
				int power,
				ItemDamageType dType,
				const bool ignoreArmor = false);

		/// Plays a grunt sFx when hit/damaged.
		void playHitSound();

		/// Sets this unit's health level.
		void setHealth(int health);

		/// Heals stun level of this unit.
		bool healStun(int power);
		/// Gets this unit's stun level.
		int getStun() const;
		/// Sets this unit's stun level.
		void setStun(int stun);

		/// Knocks this unit out instantly.
		void knockOut();

		/// Starts the falling sequence. This is only for collapsing dead or unconscious units.
		void startFalling();
		/// Advances the falling sequence.
		void keepFalling();
		/// Gets the falling sequence phase.
		int getFallingPhase() const;

		/// Starts the aiming sequence. This is only for celatids.
		void startAiming();
		/// Advances the aiming sequence.
		void keepAiming();
		/// Gets aiming sequence phase.
		int getAimingPhase() const;
		/// Sets aiming sequence phase.
		void setAimingPhase(int phase);

		/// Gets if this unit is out - either dead or unconscious.
		bool isOut(
				bool checkHealth = false,
				bool checkStun = false) const;
		const bool isOut_t(const OutCheck test = OUT_ALL) const;

		/// Gets the number of time units a certain action takes.
		int getActionTUs(
				const BattleActionType bat,
				const BattleItem* item) const;
		int getActionTUs(
				const BattleActionType bat,
				const RuleItem* itRule = NULL) const;

		/// Spends time units if possible.
		bool spendTimeUnits(int tu);
		/// Spends energy if possible.
		bool spendEnergy(int energy);
		/// Sets time units.
		void setTimeUnits(int tu);
		/// Sets the unit's energy level.
		void setEnergy(int energy);

		/// Sets whether this unit is visible.
		void setUnitVisible(bool flag = true);
		/// Gets whether this unit is visible.
		bool getUnitVisible() const;
		/// Adds unit to visible units.
		void addToHostileUnits(BattleUnit* const unit);
		/// Gets the list of visible units.
		std::vector<BattleUnit*>* getHostileUnits();
		/// Clears visible units.
		void clearHostileUnits();
		/// Gets the vector of BattleUnits that this unit has seen this turn.
		std::vector<BattleUnit*>& getHostileUnitsThisTurn();

		/// Adds tile to visible tiles.
//		bool addToVisibleTiles(Tile* const tile);
		/// Gets this unit's list of visible tiles.
//		std::vector<Tile*>* getVisibleTiles();
		/// Clears this unit's visible tiles.
//		void clearVisibleTiles();

		/// Calculates firing or throwing accuracy.
		double getAccuracy(
				const BattleAction& action,
				const BattleActionType bat = BA_NONE) const;
		/// Calculates this unit's accuracy modifier.
		double getAccuracyModifier(const BattleItem* const item = NULL) const;

		/// Sets this unit's armor value.
		void setArmor(
				int armor,
				UnitSide side);
		/// Gets this unit's Armor.
		const RuleArmor* getArmor() const;
		/// Gets this unit's armor value on a particular side.
		int getArmor(UnitSide side) const;
		/// Checks if this unit is wearing a PowerSuit.
//		bool hasPowerSuit() const;
		/// Checks if this unit is wearing a FlightSuit.
//		bool hasFlightSuit() const;

		/// Gets this unit's total number of fatal wounds.
		int getFatalWounds() const;

		/// Gets this unit's current reaction score.
		int getInitiative(const int tuSpent = 0) const;

		/// Prepares this unit for a new turn.
		void prepUnit(bool full = true);
		/// Calculates and resets this BattleUnit's time units and energy.
		void initTu(
				bool preBattle = false,
				bool hasPanicked = false);

		/// Changes this unit's morale.
		void moraleChange(int change);

		/// Don't reselect this unit.
		void dontReselect();
		/// Reselect this unit.
		void allowReselect();
		/// Checks whether reselecting this unit is allowed.
		bool reselectAllowed() const;

		/// Sets this unit's fire value.
		void setFireOnUnit(int fire);
		/// Gets this unit's fire value.
		int getFireOnUnit() const;
		/// Gives this BattleUnit damage from personal fire.
		void takeFire();

		/// Gets the list of items in this unit's inventory.
		std::vector<BattleItem*>* getInventory();

		/// Lets AI do its thing.
		void think(BattleAction* const action);
		/// Gets current AI state.
		BattleAIState* getCurrentAIState() const;
		/// Sets next AI State.
		void setAIState(BattleAIState* const aiState);

		/// Sets the Tile this unit is standing on.
		void setTile(
				Tile* const tile,
				const Tile* const tileBelow = NULL);
		/// Gets this unit's Tile.
		Tile* getTile() const;

		/// Gets the item in the specified slot of this unit's inventory.
		BattleItem* getItem(
				const RuleInventory* const slot,
				int x = 0,
				int y = 0) const;
		/// Gets the item in the specified slot of this unit's inventory.
		BattleItem* getItem(
				const std::string& slot,
				int x = 0,
				int y = 0) const;

		/// Gets the item in this unit's main hand.
		BattleItem* getMainHandWeapon(bool quickest = true) const;
		/// Gets a grenade from this unit's belt if possible.
		BattleItem* getGrenade() const;
		/// Gets the name of a melee weapon this unit may be carrying or that's innate.
		std::string getMeleeWeapon() const;
//		BattleItem* getMeleeWeapon(); // kL_note: changed.

		/// Reloads righthand weapon of this unit if needed.
		bool checkAmmo();

		/// Checks if this unit is in the exit area.
		bool isInExitArea(SpecialTileType stt = START_POINT) const;

		/// Gets this unit's height taking into account kneeling/standing.
		int getHeight(bool floating = false) const;
		/// Gets this unit's floating elevation.
		int getFloatHeight() const;

		/// Gets a soldier's Firing experience.
		int getExpFiring() const;
		/// Gets a soldier's Throwing experience.
		int getExpThrowing() const;
		/// Gets a soldier's Melee experience.
		int getExpMelee() const;
		/// Gets a soldier's Reactions experience.
		int getExpReactions() const;
		/// Gets a soldier's Bravery experience.
		int getExpBravery() const;
		/// Gets a soldier's PsiSkill experience.
		int getExpPsiSkill() const;
		/// Gets a soldier's PsiStrength experience.
		int getExpPsiStrength() const;

		/// Adds one to the reaction exp counter.
		void addReactionExp();
		/// Adds one to the firing exp counter.
		void addFiringExp();
		/// Adds one to the throwing exp counter.
		void addThrowingExp();
		/// Adds qty to the psiSkill exp counter.
		void addPsiSkillExp(int qty = 1);
		/// Adds qty to the psiStrength exp counter.
		void addPsiStrengthExp(int qty = 1);
		/// Adds qty to the melee exp counter.
		void addMeleeExp(int qty = 1);

		/// Calculates experience and days wounded.
		void postMissionProcedures(
				const SavedGame* const gameSave,
				const bool dead = false);

		/// Gets the sprite index of this unit for the MiniMap.
		int getMiniMapSpriteIndex() const;

		/// Sets the turret type of this unit (-1 is no turret).
		void setTurretType(int turretType);
		/// Gets the turret type of this unit (-1 is no turret).
		int getTurretType() const;

		/// Gets fatal wound amount of a body part.
		int getFatalWound(UnitBodyPart part) const;
		/// Heals fatal wounds.
		void heal(
				UnitBodyPart part,
				int wounds,
				int health);
		/// Gives pain killers to this unit.
		void morphine();
		/// Gives stimulants to this unit.
		bool amphetamine(
				int energy,
				int stun);

		/// Gets if the unit has overdosed on morphine.
		bool getOverDose() const;

		/// Gets motion points of this unit for the motion scanner.
		int getMotionPoints() const;

		/// Gets this unit's name.
		std::wstring getName(
				const Language* const lang = NULL,
				bool debugId = false) const;

		/// Gets this unit's stats.
		const UnitStats* getBaseStats() const;

		/// Gets this unit's stand height.
		int getStandHeight() const;
		/// Gets this unit's kneel height.
		int getKneelHeight() const;

		/// Gets this unit's loft ID.
		size_t getLoft(size_t layer = 0) const;

		/// Gets this unit's victory point value.
		int getValue() const;

		/// Gets this unit's death sound.
		int getDeathSound() const;
		/// Gets this unit's move sound.
		int getMoveSound() const;
		/// Gets this unit's aggro sound.
		int getAggroSound() const;

		/// Gets whether this unit is affected by fatal wounds.
		bool isWoundable() const;
		/// Gets whether this unit is affected by fear.
		bool isFearable() const;
		/// Gets whether this unit can be accessed with the Medikit.
		bool isHealable() const;

		/// Gets this unit's intelligence.
		int getIntelligence() const;
		/// Gets this unit's aggression.
		int getAggression() const;

		/// Gets this unit's special ability.
		SpecialAbility getSpecialAbility() const;
		/// Sets this unit's special ability.
		void setSpecialAbility(const SpecialAbility specab);

		/// Gets this unit's rank string.
		std::string getRankString() const;
		/// Gets this unit's race string.
		std::string getRaceString() const;

		/// Gets this unit's geoscape Soldier object.
		Soldier* getGeoscapeSoldier() const;

		/// Adds a kill to this unit's kill-counter.
		void addKillCount();
		/// Gets if this is a Rookie and has made his/her first kill.
		bool hasFirstKill() const;

		/// Gets this unit's type as a string.
		std::string getType() const;

		/// Sets the hand this unit is using.
		void setActiveHand(const std::string& slot);
		/// Gets this unit's active hand.
		std::string getActiveHand() const;

		/// Gets this unit's original faction
		UnitFaction getOriginalFaction() const;
		/// Converts this unit to a faction.
		void setFaction(UnitFaction faction);

		/// Sets this unit's health to 0 and status to dead.
		void instaKill();

		/// Gets this unit's spawn unit.
		std::string getSpawnUnit() const;
		/// Sets this unit's spawn unit.
		void setSpawnUnit(const std::string& spawnUnit);

		/// Gets the faction that killed this unit.
		UnitFaction killedBy() const;
		/// Sets the faction that killed this unit.
		void killedBy(UnitFaction faction);

		/// Sets the BattleUnits that this unit is charging towards.
		void setChargeTarget(BattleUnit* const chargeTarget);
		/// Gets the BattleUnits that this unit is charging towards.
		BattleUnit* getChargeTarget() const;

		/// Gets the carried weight in strength units.
		int getCarriedWeight(const BattleItem* const dragItem = NULL) const;

		/// Sets how many turns this unit will be exposed for.
		void setExposed(int turns = 0);
		/// Sets how many turns this unit will be exposed for.
		int getExposed() const;

		/// This call this after the default copy constructor deletes this unit's sprite-cache.
		void invalidateCache();

		/// Gets this BattleUnit's rules if non-Soldier else NULL.
		RuleUnit* getUnitRules() const
		{ return _unitRule; }

		/// Sets this unit's rank integer.
		void setRankInt(int rankInt);
		/// Gets this unit's rank integer.
		int getRankInt() const;
		/// Derives a rank integer based on rank string (for xcom soldiers ONLY)
		void deriveRank();

		/// This function checks if a tile is visible using maths.
		bool checkViewSector(const Position& pos) const;

		/// Adjusts this unit's stats according to difficulty.
		void adjustStats(
				const GameDifficulty diff,
				const int month);

		/// Sets this unit's cover-reserve TU.
		void setCoverReserve(int tuReserve);
		/// Gets this unit's cover-reserve TU.
		int getCoverReserve() const;

		/// Initializes a death spin.
		void initDeathSpin();
		/// Continues a death spin.
		void contDeathSpin();
		/// Regulates inititialization, direction & duration of the death spin-cycle.
		int getSpinPhase() const;
		/// Sets the spinPhase of this unit.
		void setSpinPhase(int spinphase);

		/// To stop a unit from firing/throwing if it spots a new opponent during turning.
		void setStopShot(const bool stop = true);
		/// To stop a unit from firing/throwing if it spots a new opponent during turning.
		bool getStopShot() const;

		/// Sets this unit as dashing.
		void setDashing(bool dash = true);
		/// Gets if this unit is dashing.
		bool isDashing() const;

		/// Sets this unit as having been damaged in a single explosion.
		void setTakenExpl(bool beenhit = true);
		/// Gets if this unit has aleady been damaged in a single explosion.
		bool getTakenExpl() const;

		/// Sets this unit as having been damaged in a single fire.
		void setTakenFire(bool beenhit = true);
		/// Gets if this unit has aleady been damaged in a single fire.
		bool getTakenFire() const;

		/// Returns true if this unit is selectable.
		bool isSelectable(
				UnitFaction faction,
				bool checkReselect = false,
				bool checkInventory = false) const;

		/// Returns true if this unit has an inventory.
		bool hasInventory() const;

		/// Gets this unit's movement type.
		MovementType getMoveTypeUnit() const;

		/// Gets if this unit is hiding or not.
		bool isHiding() const
		{ return _hidingForTurn; };
		/// Sets this unit hiding or not.
		void setHiding(const bool hiding)
		{ _hidingForTurn = hiding; };

		/// Puts the unit in the corner to think about what he's done.
//		void goToTimeOut();

		/// Creates special weapon for the unit.
//		void setSpecialWeapon(SavedBattleGame* save, const Ruleset* rule);
		/// Get special weapon.
//		BattleItem* getSpecialWeapon(BattleType type) const;

		/// Gets this unit's mission statistics.
		BattleUnitStatistics* getStatistics() const;
		/// Sets this unit murderer's id.
		void setMurdererId(int id);
		/// Gets this unit murderer's id.
		int getMurdererId() const;

		/// Sets this unit's order in battle.
		void setBattleOrder(size_t order);
		/// Gets this unit's order in battle.
		size_t getBattleOrder() const;

		/// Sets the BattleGame for this unit.
		void setBattleForUnit(BattlescapeGame* const battleGame);

		/// Gets if this unit is about to die.
		bool getAboutToDie() const;
		/// Sets this unit's parameters as down (collapsed/ unconscious/ dead).
		void putDown();

		/// Sets this BattleUnit's turn direction when spinning 180 degrees.
		void setTurnDirection(int dir);

		/// Sets this BattleUnit as having just revived during a Turnover.
		void setRevived(bool revived = true);

		/// Gets all units in the battlescape that are valid RF-spotters of this BattleUnit.
		std::list<BattleUnit*>* getRfSpotters();

		/// Sets the parameters of a successful mind-control psi attack.
		void hostileMcParameters(
				int& strength,
				int& skill);

		/// Plays the death sound.
		void playDeathSound() const;

		/// Gets if this unit is a Zombie.
		bool isZombie() const;
};

}

#endif
