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

#ifndef OPENXCOM_RULEITEM_H
#define OPENXCOM_RULEITEM_H

#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>


namespace OpenXcom
{

enum ItemDamageType
{
	DT_NONE,	// 0
	DT_AP,		// 1
	DT_IN,		// 2
	DT_HE,		// 3
	DT_LASER,	// 4
	DT_PLASMA,	// 5
	DT_STUN,	// 6
	DT_MELEE,	// 7
	DT_ACID,	// 8
	DT_SMOKE	// 9
};


enum BattleType
{
	BT_NONE,				// 0
	BT_FIREARM,				// 1
	BT_AMMO,				// 2
	BT_MELEE,				// 3
	BT_GRENADE,				// 4
	BT_PROXIMITYGRENADE,	// 5 // kL_note: where is SMOKEGRENADE ?!?!? (and Alien_Grenade)
	BT_MEDIKIT,				// 6
	BT_SCANNER,				// 7
	BT_MINDPROBE,			// 8
	BT_PSIAMP,				// 9
	BT_FLARE,				// 10
	BT_CORPSE				// 11
};


class Surface;
class SurfaceSet;


/**
 * Represents a specific type of item.
 * Contains constant info about an item like
 * storage size, sell price, etc.
 * @sa Item
 */
class RuleItem
{

private:
	bool
		_arcingShot,
		_fixedWeapon,
		_flatRate,
		_liveAlien,
		_recover,
		_twoHanded,
		_waypoint;
	int
		_armor,
		_attraction,
		_costBuy,
		_costSell,
		_invWidth,
		_invHeight,
		_listOrder,
		_recoveryPoints,
		_transferTime,
		_turretType,
		_tuUse,
		_weight,

		_heal,
		_painKiller,
		_stimulant,

		_autoShots,
		_blastRadius,
		_bulletSpeed,
		_clipSize,
		_explosionSpeed,
		_fireSound,
		_hitSound,
		_hitAnimation,
		_power,
		_shotgunPellets,

		_accuracyAimed,
		_accuracyAuto,
		_accuracyMelee,
		_accuracySnap,
		_tuAimed,
		_tuAuto,
		_tuMelee,
		_tuSnap,

		_maxRange,
		_aimRange,
		_snapRange,
		_autoRange,
		_minRange,
		_dropoff,

		_woundRecovery,
		_healthRecovery,
		_stunRecovery,
		_energyRecovery,

		_bigSprite,
		_floorSprite,
		_handSprite,
		_bulletSprite;

	float _size;

	std::string
		_type,
		_name; // two types of objects can have the same name

	BattleType _battleType;
	ItemDamageType _damageType;

	std::vector<std::string>
		_compatibleAmmo,
		_requires;


	std::string _zombieUnit;
	public:
		/// Creates a blank item ruleset.
		RuleItem(const std::string& type);
		/// Cleans up the item ruleset.
		~RuleItem();

		/// Loads item data from YAML.
		void load(
				const YAML::Node& node,
				int modIndex,
				int listIndex);

		/// Gets the item's type.
		std::string getType() const;
		/// Gets the item's name.
		std::string getName() const;

		/// Gets the item's research requirements.
		const std::vector<std::string>& getRequirements() const;

		/// Gets the item's size.
		float getSize() const;

		/// Gets the item's purchase cost.
		int getBuyCost() const;
		/// Gets the item's sale cost.
		int getSellCost() const;
		/// Gets the item's transfer time.
		int getTransferTime() const;

		/// Gets the item's weight.
		int getWeight() const;

		/// Gets the item's reference in BIGOBS.PCK for use in inventory.
		int getBigSprite() const;
		/// Gets the item's reference in FLOOROB.PCK for use in inventory.
		int getFloorSprite() const;
		/// Gets the item's reference in HANDOB.PCK for use in inventory.
		int getHandSprite() const;

		/// Gets if the item is two-handed.
		bool isTwoHanded() const;
		/// Gets if the item is a launcher.
		bool isWaypoint() const;
		/// Gets if the item is fixed.
		bool isFixed() const;

		/// Gets the item's bullet sprite reference.
		int getBulletSprite() const;
		/// Gets the item's fire sound.
		int getFireSound() const;
		/// Gets the item's hit sound.
		int getHitSound() const;
		/// Gets the item's hit animation.
		int getHitAnimation() const;

		/// Gets the item's power.
		int getPower() const;

		/// Gets the item's snapshot accuracy.
		int getAccuracySnap() const;
		/// Gets the item's autoshot accuracy.
		int getAccuracyAuto() const;
		/// Gets the item's aimed shot accuracy.
		int getAccuracyAimed() const;
		/// Gets the item's melee accuracy.
		int getAccuracyMelee() const;

		/// Gets the item's snapshot TU cost.
		int getTUSnap() const;
		/// Gets the item's autoshot TU cost.
		int getTUAuto() const;
		/// Gets the item's aimed shot TU cost.
		int getTUAimed() const;
		/// Gets the item's melee TU cost.
		int getTUMelee() const;

		/// Gets list of compatible ammo.
		std::vector<std::string>* getCompatibleAmmo();

		/// Gets the item's damage type.
		ItemDamageType getDamageType() const;
		/// Gets the item's type.
		BattleType getBattleType() const;

		/// Gets the item's inventory width.
		int getInventoryWidth() const;
		/// Gets the item's inventory height.
		int getInventoryHeight() const;

		/// Gets the ammo amount.
		int getClipSize() const;

		/// Draws the item's hand sprite onto a surface.
		void drawHandSprite(
				SurfaceSet* texture,
				Surface* surface) const;

		/// Gets the medikit heal quantity.
		int getHealQuantity() const;
		/// Gets the medikit pain killer quantity.
		int getPainKillerQuantity() const;
		/// Gets the medikit stimulant quantity.
		int getStimulantQuantity() const;
		/// Gets the medikit wound healed per shot.
		int getWoundRecovery() const;
		/// Gets the medikit health recovered per shot.
		int getHealthRecovery() const;
		/// Gets the medikit energy recovered per shot.
		int getEnergyRecovery() const;
		/// Gets the medikit stun recovered per shot.
		int getStunRecovery() const;

		/// Gets the Time Unit use.
		int getTUUse() const;

		/// Gets the max explosion radius.
		int getExplosionRadius() const;

		/// Gets the recovery points score
		int getRecoveryPoints() const;

		/// Gets the item's armor.
		int getArmor() const;

		/// Gets the item's recoverability.
		bool isRecoverable() const;

		/// Gets the item's turret type.
		int getTurretType() const;

		/// Checks if this a live alien.
		bool getAlien() const;

		/// Should we charge a flat rate?
		bool getFlatRate() const;

		/// Should this weapon arc?
		bool getArcingShot() const;

		/// How much do aliens want this thing?
		int getAttraction() const;

		/// Get the list weight for this item.
		int getListOrder() const;

		/// How fast does a projectile fired from this weapon travel?
		int getBulletSpeed() const;
		/// How fast does the explosion animation play?
		int getExplosionSpeed() const;

		/// How many auto shots does this weapon fire.
		int getAutoShots() const;

		/// is this item a 2 handed weapon?
		bool isRifle() const;
		/// is this item a single handed weapon?
		bool isPistol() const;

		/// Get the max range of this weapon.
		int getMaxRange() const;
		/// Get the max range of aimed shots with this weapon.
		int getAimRange() const;
		/// Get the max range of snap shots with this weapon.
		int getSnapRange() const;
		/// Get the max range of auto shots with this weapon.
		int getAutoRange() const;
		/// Get the minimum effective range of this weapon.
		int getMinRange() const;
		/// Get the accuracy dropoff of this weapon.
		int getDropoff() const;
		///
		int getShotgunPellets() const;
		/// Gets a weapon's zombie unit, if any.
		std::string getZombieUnit() const;
};

}

#endif
