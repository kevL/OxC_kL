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

#include "RuleItem.h"

#include "RuleInventory.h"

//#include "../Engine/Surface.h"
#include "../Engine/SurfaceSet.h"


namespace OpenXcom
{

/**
 * Creates a blank ruleset for a certain type of item.
 * @param type - string defining the type
 */
RuleItem::RuleItem(const std::string& type)
	:
		_type(type),
		_name(type),
		_size(0.),
		_costBuy(0),
		_costSell(0),
		_transferTime(24),
		_weight(3),
		_bigSprite(-1),
		_floorSprite(-1),
		_handSprite(120),
		_bulletSprite(-1),
		_fireSound(-1),
		_hitSound(-1),
		_hitAnimation(-1),
		_power(0),
		_damageType(DT_NONE),
		_maxRange(200), // could be -1 for infinite.
		_aimRange(20),
		_snapRange(10),
		_autoRange(5),
		_minRange(0), // should be -1 so unit can shoot it's own tile (don't ask why .. just for logical consistency)
		_dropoff(1),
		_accuracyAimed(0),
		_accuracySnap(0),
		_accuracyAuto(0),
		_accuracyMelee(0),
		_tuLaunch(0), // these could all be -1 meaning not possible, but this would be problematic re. ReactionFire etc etc.
		_tuAimed(0),
		_tuSnap(0),
		_tuAuto(0),
		_tuMelee(0),
		_tuUse(0),
		_tuReload(15),
		_tuUnload(12),
		_tuPrime(0),
		_clipSize(0),
		_blastRadius(-1),
		_battleType(BT_NONE),
		_arcingShot(false),
		_twoHanded(false),
		_waypoint(0),
		_fixedWeapon(false),
		_flatRate(false),
		_invWidth(1),
		_invHeight(1),
		_painKiller(0),
		_heal(0),
		_stimulant(0),
		_woundRecovery(0),
		_healthRecovery(0),
		_stunRecovery(0),
		_energyRecovery(0),
		_recoveryPoints(0),
		_armor(20),
		_turretType(-1),
		_recover(true),
		_liveAlien(false),
		_attraction(0),
		_listOrder(0),
		_bulletSpeed(0),
		_explosionSpeed(0),
		_autoShots(3),
		_autoKick(12),
		_shotgunPellets(0),
		_shotgunPattern(5),
		_strengthApplied(false),
		_skillApplied(true),
		_LOSRequired(false),
		_noReaction(false),
		_noResearch(false),
		_meleePower(0),
		_meleeAnimation(0),
		_meleeSound(-1),
		_meleeHitSound(-1),
		_specialType(STT_NONE),
//		_specialType(-1),
		_canExecute(false),
		_defusePulse(false)
{}

/**
 * dTor.
 */
RuleItem::~RuleItem()
{}

/**
 * Loads the item from a YAML file.
 * @param node		- reference the YAML node
 * @param modIndex	- offsets the sounds and sprite values to avoid conflicts
 * @param listOrder	- the list weight for this item
 */
void RuleItem::load(
		const YAML::Node& node,
		int modIndex,
		int listOrder)
{
	_type			= node["type"]			.as<std::string>(_type);
	_name			= node["name"]			.as<std::string>(_name);
	_requires		= node["requires"]		.as<std::vector<std::string> >(_requires);
	_size			= node["size"]			.as<double>(_size);
	_costBuy		= node["costBuy"]		.as<int>(_costBuy);
	_costSell		= node["costSell"]		.as<int>(_costSell);
	_transferTime	= node["transferTime"]	.as<int>(_transferTime);
	_weight			= node["weight"]		.as<int>(_weight);

	if (node["bigSprite"])
	{
		_bigSprite = node["bigSprite"].as<int>(_bigSprite);
		if (_bigSprite > 56) // BIGOBS.PCK: 57 entries
			_bigSprite += modIndex;
	}

	if (node["floorSprite"])
	{
		_floorSprite = node["floorSprite"].as<int>(_floorSprite);
		if (_floorSprite > 72) // FLOOROB.PCK: 73 entries
			_floorSprite += modIndex;
	}

	if (node["handSprite"])
	{
		_handSprite = node["handSprite"].as<int>(_handSprite);
		if (_handSprite > 127) // HANDOB.PCK: 128 entries
			_handSprite += modIndex;
	}

	if (node["bulletSprite"])
	{
		// Projectiles: 385 entries ((105*33) / (3*3)) (35 sprites per projectile(0-34), 11 projectiles (0-10))
		_bulletSprite = node["bulletSprite"].as<int>(_bulletSprite) * 35;
		if (_bulletSprite >= 385)
			_bulletSprite += modIndex;
	}

	if (node["fireSound"])
	{
		_fireSound = node["fireSound"].as<int>(_fireSound);
		if (_fireSound > 54) // BATTLE.CAT: 55 entries
			_fireSound += modIndex;
	}

	if (node["hitSound"])
	{
		_hitSound = node["hitSound"].as<int>(_hitSound);
		if (_hitSound > 54) // BATTLE.CAT: 55 entries
			_hitSound += modIndex;
	}

	if (node["meleeSound"])
	{
		_meleeSound = node["meleeSound"].as<int>(_meleeSound);
		if (_meleeSound > 54) // BATTLE.CAT: 55 entries
			_meleeSound += modIndex;
	}

	if (node["hitAnimation"])
	{
		_hitAnimation = node["hitAnimation"].as<int>(_hitAnimation);
		if (_hitAnimation > 55) // SMOKE.PCK: 56 entries
			_hitAnimation += modIndex;
	}

	if (node["meleeAnimation"])
	{
		_meleeAnimation = node["meleeAnimation"].as<int>(_meleeAnimation);
		if (_meleeAnimation > 3) // HIT.PCK: 4 entries
			_meleeAnimation += modIndex;
	}

	if (node["meleeHitSound"])
	{
		_meleeHitSound = node["meleeHitSound"].as<int>(_meleeHitSound);
		if (_meleeHitSound > 54) // BATTLE.CAT: 55 entries
			_meleeHitSound += modIndex;
	}

	_damageType		= static_cast<DamageType>(node["damageType"]		.as<int>(0));
	_battleType		= static_cast<BattleType>(node["battleType"]		.as<int>(0));
	_specialType	= static_cast<SpecialTileType>(node["specialType"]	.as<int>(-1));

	_power				= node["power"]				.as<int>(_power);
	_clipSize			= node["clipSize"]			.as<int>(_clipSize);
	_compatibleAmmo		= node["compatibleAmmo"]	.as< std::vector<std::string> >(_compatibleAmmo);
	_accuracyAuto		= node["accuracyAuto"]		.as<int>(_accuracyAuto);
	_accuracySnap		= node["accuracySnap"]		.as<int>(_accuracySnap);
	_accuracyAimed		= node["accuracyAimed"]		.as<int>(_accuracyAimed);
	_accuracyMelee		= node["accuracyMelee"]		.as<int>(_accuracyMelee);
	_tuAuto				= node["tuAuto"]			.as<int>(_tuAuto);
	_tuSnap				= node["tuSnap"]			.as<int>(_tuSnap);
	_tuAimed			= node["tuAimed"]			.as<int>(_tuAimed);
	_tuLaunch			= node["tuLaunch"]			.as<int>(_tuLaunch);
	_tuUse				= node["tuUse"]				.as<int>(_tuUse);
	_tuPrime			= node["tuPrime"]			.as<int>(_tuPrime);
	_tuMelee			= node["tuMelee"]			.as<int>(_tuMelee);
	_tuReload			= node["tuReload"]			.as<int>(_tuReload);
	_tuUnload			= node["tuUnload"]			.as<int>(_tuUnload);
	_twoHanded			= node["twoHanded"]			.as<bool>(_twoHanded);
	_waypoint			= node["waypoint"]			.as<int>(_waypoint);
	_fixedWeapon		= node["fixedWeapon"]		.as<bool>(_fixedWeapon);
	_invWidth			= node["invWidth"]			.as<int>(_invWidth);
	_invHeight			= node["invHeight"]			.as<int>(_invHeight);
	_painKiller			= node["painKiller"]		.as<int>(_painKiller);
	_heal				= node["heal"]				.as<int>(_heal);
	_stimulant			= node["stimulant"]			.as<int>(_stimulant);
	_woundRecovery		= node["woundRecovery"]		.as<int>(_woundRecovery);
	_healthRecovery		= node["healthRecovery"]	.as<int>(_healthRecovery);
	_stunRecovery		= node["stunRecovery"]		.as<int>(_stunRecovery);
	_energyRecovery		= node["energyRecovery"]	.as<int>(_energyRecovery);
	_recoveryPoints		= node["recoveryPoints"]	.as<int>(_recoveryPoints);
	_armor				= node["armor"]				.as<int>(_armor);
	_turretType			= node["turretType"]		.as<int>(_turretType);
	_recover			= node["recover"]			.as<bool>(_recover);
	_liveAlien			= node["liveAlien"]			.as<bool>(_liveAlien);
	_blastRadius		= node["blastRadius"]		.as<int>(_blastRadius);
	_attraction			= node["attraction"]		.as<int>(_attraction);
	_flatRate			= node["flatRate"]			.as<bool>(_flatRate);
	_arcingShot			= node["arcingShot"]		.as<bool>(_arcingShot);
	_maxRange			= node["maxRange"]			.as<int>(_maxRange);
	_aimRange			= node["aimRange"]			.as<int>(_aimRange);
	_snapRange			= node["snapRange"]			.as<int>(_snapRange);
	_autoRange			= node["autoRange"]			.as<int>(_autoRange);
	_minRange			= node["minRange"]			.as<int>(_minRange);
	_dropoff			= node["dropoff"]			.as<int>(_dropoff);
	_bulletSpeed		= node["bulletSpeed"]		.as<int>(_bulletSpeed);
	_explosionSpeed		= node["explosionSpeed"]	.as<int>(_explosionSpeed);
	_autoShots			= node["autoShots"]			.as<int>(_autoShots);
	_autoKick			= node["autoKick"]			.as<int>(_autoKick);
	_shotgunPellets		= node["shotgunPellets"]	.as<int>(_shotgunPellets);
	_shotgunPattern		= node["shotgunPattern"]	.as<int>(_shotgunPattern);
	_zombieUnit			= node["zombieUnit"]		.as<std::string>(_zombieUnit);
	_strengthApplied	= node["strengthApplied"]	.as<bool>(_strengthApplied);
	_skillApplied		= node["skillApplied"]		.as<bool>(_skillApplied);
	_LOSRequired		= node["LOSRequired"]		.as<bool>(_LOSRequired);
	_noReaction			= node["noReaction"]		.as<bool>(_noReaction);
	_noResearch			= node["noResearch"]		.as<bool>(_noResearch);
	_meleePower			= node["meleePower"]		.as<int>(_meleePower);
	_defusePulse		= node["defusePulse"]		.as<bool>(_defusePulse);

	switch (_damageType)
	{
		case DT_AP:
		case DT_LASER:
		case DT_PLASMA:
		case DT_MELEE:
		case DT_ACID:
			_canExecute = true;
	}

	_listOrder = node["listOrder"].as<int>(_listOrder);
	if (_listOrder == 0)
		_listOrder = listOrder;
}

/**
 * Gets the item type.
 * @note Each item has a unique type.
 * @return, the item's type
 */
const std::string& RuleItem::getType() const
{
	return _type;
}

/**
 * Gets the language string that names this item.
 * @note This is not necessarily unique.
 * @return, the item's name
 */
const std::string& RuleItem::getName() const
{
	return _name;
}

/**
 * Gets the list of research required to use this item.
 * @return, the list of research IDs
 */
const std::vector<std::string>& RuleItem::getRequirements() const
{
	return _requires;
}

/**
 * Gets the amount of space this item takes up in a storage facility.
 * @return, the storage size
 */
double RuleItem::getSize() const
{
	return _size;
}

/**
 * Gets the amount of money this item costs to purchase (0 if not purchasable).
 * @return, the buy cost
 */
int RuleItem::getBuyCost() const
{
	return _costBuy;
}

/**
 * Gets the amount of money this item is worth to sell.
 * @return, the sell cost
 */
int RuleItem::getSellCost() const
{
	return _costSell;
}

/**
 * Gets the amount of time this item takes to arrive at a base.
 * @return, the time in hours
 */
int RuleItem::getTransferTime() const
{
	return _transferTime;
}

/**
 * Gets the weight of the item.
 * @return, the weight in strength units
 */
int RuleItem::getWeight() const
{
	return _weight;
}

/**
 * Gets the reference in BIGOBS.PCK for use in inventory.
 * @return, the sprite reference
 */
int RuleItem::getBigSprite() const
{
	return _bigSprite;
}

/**
 * Gets the reference in FLOOROB.PCK for use in inventory.
 * @return, the sprite reference
 */
int RuleItem::getFloorSprite() const
{
	return _floorSprite;
}

/**
 * Gets the reference in HANDOB.PCK for use in inventory.
 * @return, the sprite reference
 */
int RuleItem::getHandSprite() const
{
	return _handSprite;
}

/**
 * Returns whether the Item is held with two hands.
 * @return, true if Item is two-handed
 */
bool RuleItem::isTwoHanded() const
{
	return _twoHanded;
}

/**
 * Gets if the item is a launcher and if so how many waypoints can be set.
 * @return, maximum waypoints for the Item
 */
int RuleItem::isWaypoints() const
{
	return _waypoint;
}

/**
 * Returns whether the Item is a fixed weapon.
 * You can't move/throw/drop fixed weapons - e.g. HWP turrets.
 * @return, true if a fixed weapon
 */
bool RuleItem::isFixed() const
{
	return _fixedWeapon;
}

/**
 * Gets the Item's bullet sprite reference.
 * @return, the sprite reference
 */
int RuleItem::getBulletSprite() const
{
	return _bulletSprite;
}

/**
 * Gets the Item's fire sound.
 * @return, the fire sound ID
 */
int RuleItem::getFireSound() const
{
	return _fireSound;
}

/**
 * Gets the item's hit sound.
 * @return, the hit sound ID
 */
int RuleItem::getHitSound() const
{
	return _hitSound;
}

/**
 * Gets the item's hit animation.
 * @return, the hit animation ID
 */
int RuleItem::getHitAnimation() const
{
	return _hitAnimation;
}

/**
 * Gets the item's damage-power or light-power if its BattleType is BT_FLARE.
 * @return, the power
 */
int RuleItem::getPower() const
{
	return _power;
}

/**
 * Gets the item's accuracy for snapshots.
 * @return, the snapshot accuracy
 */
int RuleItem::getAccuracySnap() const
{
	return _accuracySnap;
}

/**
 * Gets the item's accuracy for autoshots.
 * @return, the autoshot accuracy
 */
int RuleItem::getAccuracyAuto() const
{
	return _accuracyAuto;
}

/**
 * Gets the item's accuracy for aimed shots.
 * @return, the aimed accuracy
 */
int RuleItem::getAccuracyAimed() const
{
	return _accuracyAimed;
}

/**
 * Gets the item's accuracy for melee attacks.
 * @return, the melee accuracy
 */
int RuleItem::getAccuracyMelee() const
{
	return _accuracyMelee;
}

/**
 * Gets the item's time unit percentage for snapshots.
 * @return, the snapshot TU percentage
 */
int RuleItem::getSnapTu() const
{
	return _tuSnap;
}

/**
 * Gets the item's time unit percentage for autoshots.
 * @return, the autoshot TU percentage
 */
int RuleItem::getAutoTu() const
{
	return _tuAuto;
}

/**
 * Gets the item's time unit percentage for aimed shots.
 * @return, the aimed shot TU percentage
 */
int RuleItem::getAimedTu() const
{
	return _tuAimed;
}

/**
 * Gets the item's time unit percentage for launch shots.
 * @return, the launch shot TU percentage
 */
int RuleItem::getLaunchTu() const
{
	return _tuLaunch;
}

/**
 * Gets the item's time unit percentage for melee attacks.
 * @return, the melee TU percentage
 */
int RuleItem::getMeleeTu() const
{
	return _tuMelee;
}

/**
 * Gets the number of Time Units needed to use this item.
 * @return, the number of TU needed to use this item
 */
int RuleItem::getUseTu() const
{
	return _tuUse;
}

/**
 * Gets the number of Time Units needed to reload this item.
 * @return, the number of TU needed to reload this item
 */
int RuleItem::getReloadTu() const
{
	return _tuReload;
}

/**
 * Gets the number of Time Units needed to unload this item.
 * @return, the number of TU needed to unload this item
 */
int RuleItem::getUnloadTu() const
{
	return _tuUnload;
}

/**
 * Gets the number of Time Units needed to prime this item.
 * @return, the number of TU needed to prime this item
 */
int RuleItem::getPrimeTu() const
{
	return _tuPrime;
}

/**
 * Gets a list of compatible ammo.
 * @return, pointer to a vector of compatible ammo strings
 */
const std::vector<std::string>* RuleItem::getCompatibleAmmo() const
{
	return &_compatibleAmmo;
}

/**
 * Gets the item's damage type.
 * @return, the damage type
 */
DamageType RuleItem::getDamageType() const
{
	return _damageType;
}

/**
 * Gets the item's battle type.
 * @return, the battle type
 */
BattleType RuleItem::getBattleType() const
{
	return _battleType;
}

/**
 * Gets the item's width in a soldier's inventory.
 * @return, the width
 */
int RuleItem::getInventoryWidth() const
{
	return _invWidth;
}

/**
 * Gets the item's height in a soldier's inventory.
 * @return, the height
 */
int RuleItem::getInventoryHeight() const
{
	return _invHeight;
}

/**
 * Gets the item's ammo clip size.
 * Melee items have clipsize(0), lasers etc have clipsize(-1).
 * @return, the ammo clip size
 */
int RuleItem::getClipSize() const
{
	return _clipSize;
}

/**
 * Draws and centers the hand sprite on a surface according to its dimensions.
 * @param texture - pointer to the surface set to get the sprite from
 * @param surface - pointer to the surface to draw to
 */
void RuleItem::drawHandSprite(
		SurfaceSet* const texture,
		Surface* const surface) const
{
	Surface* const frame = texture->getFrame(_bigSprite);
	if (frame != NULL) // safety.
	{
		frame->setX(
				(RuleInventory::HAND_W - _invWidth)
			   * RuleInventory::SLOT_W / 2);
		frame->setY(
				(RuleInventory::HAND_H - _invHeight)
			   * RuleInventory::SLOT_H / 2);

		texture->getFrame(_bigSprite)->blit(surface);
	}
	else Log(LOG_INFO) << "ERROR: bigob not found [3] #" << _bigSprite; // also in Inventory object.
}

/**
 * Gets the heal quantity of the item.
 * @return, the new heal quantity
 */
int RuleItem::getHealQuantity() const
{
	return _heal;
}

/**
 * Gets the pain killer quantity of the item.
 * @return, the new pain killer quantity
 */
int RuleItem::getPainKillerQuantity() const
{
	return _painKiller;
}

/**
 * Gets the stimulant quantity of the item.
 * @return, the new stimulant quantity
 */
int RuleItem::getStimulantQuantity() const
{
	return _stimulant;
}

/**
 * Gets the amount of fatal wound healed per usage.
 * @return, the amount of fatal wounds healed
 */
int RuleItem::getWoundRecovery() const
{
	return _woundRecovery;
}

/**
 * Gets the amount of health added to a wounded soldier's health.
 * @return, the amount of health to add
 */
int RuleItem::getHealthRecovery() const
{
	return _healthRecovery;
}

/**
 * Gets the amount of energy added to a soldier's energy.
 * @return, the amount of energy to add
 */
int RuleItem::getEnergyRecovery() const
{
	return _energyRecovery;
}

/**
 * Gets the amount of stun removed from a soldier's stun level.
 * @return, the amount of stun removed
 */
int RuleItem::getStunRecovery() const
{
	return _stunRecovery;
}

/**
 * Returns the item's max explosion radius.
 * @note Small explosions don't have a restriction. Larger explosions are
 * restricted using a formula with a maximum of radius 10 no matter how large
 * the explosion. kL_note: nah...
 * @return, the radius (-1 if not AoE)
 */
int RuleItem::getExplosionRadius() const
{
	if (_blastRadius == -1
		&& (_damageType == DT_HE
			|| _damageType == DT_STUN
			|| _damageType == DT_SMOKE
			|| _damageType == DT_IN))
	{
		return _power / 20;
	}

//	if (_damageType == DT_IN)
//		return _power / 30;

	return _blastRadius;
}

/**
 * Returns the item's recovery points.
 * @note This is used during the battlescape debriefing score calculation.
 * @return, the recovery points
 */
int RuleItem::getRecoveryPoints() const
{
	return _recoveryPoints;
}

/**
 * Returns the item's armor.
 * @note The item is destroyed when an explosion power bigger than its armor hits it.
 * @return, the armor
 */
int RuleItem::getArmor() const
{
	return _armor;
}

/**
 * Returns if the item should be recoverable from the battlescape.
 * @return, true if item is recoverable
 */
bool RuleItem::isRecoverable() const
{
	return _recover;
}

/**
 * Returns the item's Turret Type.
 * @return, the turret index (-1 for no turret)
 */
int RuleItem::getTurretType() const
{
	return _turretType;
}

/**
 * Returns if this is a live alien.
 * @return, true if this is a live alien
 */
bool RuleItem::isAlien() const
{
	return _liveAlien;
}

/**
 * Returns whether this item charges a flat TU rate.
 * @return, true if this item charges a flat TU rate
 */
bool RuleItem::getFlatRate() const
{
	return _flatRate;
}

/**
 * Returns if this weapon should arc its shots.
 * @return, true if this weapon arcs its shots
 */
bool RuleItem::getArcingShot() const
{
	return _arcingShot;
}

/**
 * Gets the attraction value for this item (for AI).
 * @return, the attraction value
 */
int RuleItem::getAttraction() const
{
	return _attraction;
}

/**
 * Gets the list weight for this research item
 * @return, the list weight
 */
int RuleItem::getListOrder() const
{
	 return _listOrder;
}

/**
 * Gets the maximum range of this weapon
 * @return, the maximum range
 */
int RuleItem::getMaxRange() const
{
	return _maxRange;
}

/**
 * Gets the maximum effective range of this weapon when using Aimed Shot.
 * @return, the maximum range
 */
int RuleItem::getAimRange() const
{
	return _aimRange;
}

/**
 * Gets the maximim effective range of this weapon for Snap Shot.
 * @return, the maximum range
 */
int RuleItem::getSnapRange() const
{
	return _snapRange;
}

/**
 * Gets the maximim effective range of this weapon for Auto Shot.
 * @return, the maximum range
 */
int RuleItem::getAutoRange() const
{
	return _autoRange;
}

/**
 * Gets the minimum effective range of this weapon.
 * @return, the minimum effective range
 */
int RuleItem::getMinRange() const
{
	return _minRange;
}

/**
 * Gets the accuracy dropoff value of this weapon.
 * @return, the per-tile dropoff
 */
int RuleItem::getDropoff() const
{
	return _dropoff;
}

/**
 * Gets the speed at which this bullet travels.
 * @return, the speed
 */
int RuleItem::getBulletSpeed() const
{
	return _bulletSpeed;
}

/**
 * Gets the speed at which this bullet explodes.
 * @return, the speed
 */
int RuleItem::getExplosionSpeed() const
{
	return _explosionSpeed;
}

/**
* Gets the amount of auto shots fired by this weapon.
* @return, the shots
*/
int RuleItem::getAutoShots() const
{
	return _autoShots;
}

/**
 * Gets the kick this weapon does on autoshot.
 */
int RuleItem::getAutoKick() const
{
	return _autoKick;
}

/**
* Gets if this item is a rifle.
* @return, true if rifle
*/
bool RuleItem::isRifle() const
{
	return _twoHanded == true
			&& (_battleType == BT_FIREARM
				|| _battleType == BT_MELEE);
}

/**
* Gets if this item is a pistol.
* @return, true if pistol
*/
bool RuleItem::isPistol() const
{
	return _twoHanded == false
			&& (_battleType == BT_FIREARM
				|| _battleType == BT_MELEE);
}

/**
 * Gets if this item is a grenade.
 * @return, true if grenade
 */
bool RuleItem::isGrenade() const
{
	return _battleType == BT_GRENADE
		|| _battleType == BT_PROXYGRENADE;
}

/**
 * Gets the number of projectiles this ammo shoots at once - a shotgun effect.
 * @return, the number of projectiles
 */
int RuleItem::getShotgunPellets() const
{
	return _shotgunPellets;
}

/**
 * Gets the breadth of cone for shotgun projectiles.
 * @return, the breadth of cone
 */
int RuleItem::getShotgunPattern() const
{
	return _shotgunPattern;
}

/**
 * Gets the unit that the victim is morphed into when attacked.
 * @return, the weapon's zombie unit
 */
const std::string& RuleItem::getZombieUnit() const
{
	return _zombieUnit;
}

/**
 * Used to determine if a unit's strength is added to melee damage.
 * @return, true if strength is added to melee damage
 */
bool RuleItem::isStrengthApplied() const
{
	return _strengthApplied;
}

/**
 * Used to determine if skill is applied to the accuracy of this weapon.
 * This applies only to melee weapons.
 * @return, true if skill is applied to accuracy
 */
bool RuleItem::isSkillApplied() const
{
	return _skillApplied;
}

/**
 * Used to determine if a weapon is capable of Reaction Fire.
 * @return, true if a weapon can react during opponent's turn
 */
bool RuleItem::canReactionFire() const
{
	return (_noReaction == false);
}

/**
 * The sound this weapon makes when you swing it at someone.
 * @return, the weapon's melee attack sound
 */
int RuleItem::getMeleeSound() const
{
	return _meleeSound;
}

/**
 * The sound this weapon makes when you punch someone in the face with it.
 * @return, the weapon's melee hit sound
 */
int RuleItem::getMeleeHitSound() const
{
	return _meleeHitSound;
}

/**
 * The damage this weapon does when you punch someone in the face with it.
 * @return, the weapon's melee power
 */
int RuleItem::getMeleePower() const
{
	return _meleePower;
}

/**
 * The starting frame offset in hit.pck to use for the animation.
 * @return, the starting frame offset in HIT.PCK to use for the animation
 */
int RuleItem::getMeleeAnimation() const
{
	return _meleeAnimation;
}

/**
 * Checks if line of sight is required for this psionic weapon to function.
 * @return, true if line of sight is required
 */
bool RuleItem::isLosRequired() const
{
	return _LOSRequired;
}

/**
 * Gets the associated special type of this item.
 * @note Type 14 is the alien brain and types 0 and 1 are "regular tile" and
 * "starting point" so try not to use those ones.
 * @return, special type
 */
SpecialTileType RuleItem::getSpecialType() const
{
	return _specialType;
}

/**
 * Gets the item's default BattleAction.
 * @note Used to show a TU cost in InventoryState. Lifted from ActionMenuState cTor.
 * @param isPrimed - true if checking a grenade and it's primed (default false)
 * @return, BattleActionType (BattlescapeGame.h)
 */
BattleActionType RuleItem::getDefaultAction(bool isPrimed) const
{
	if (_fixedWeapon == true)
		return BA_NONE;

	if (_battleType == BT_FIREARM)
	{
		if (_waypoint != 0)
			return BA_LAUNCH;

		if (_accuracySnap != 0)
			return BA_SNAPSHOT;

		if (_accuracyAuto != 0)
			return BA_AUTOSHOT;

		if (_accuracyAimed != 0)
			return BA_AIMEDSHOT;
	}

	if (_battleType == BT_GRENADE
		|| _battleType == BT_PROXYGRENADE)
	{
		if (isPrimed == false)
			return BA_PRIME;
		else
			return BA_DEFUSE;
	}

	if (_battleType == BT_MEDIKIT
		|| _battleType == BT_SCANNER
		|| _battleType == BT_MINDPROBE)
	{
		return BA_USE;
	}

	if (_battleType == BT_PSIAMP)
		return BA_PSIPANIC;

	if (_tuMelee != 0)
		return BA_HIT;

	return BA_NONE;
}

/**
 * Checks if an item is exempt from research.
 * @note Currently this is used to exclude SHADICS ARMORS from getting marked
 * as unresearched in various lists, such as Stores & Transfers ...
 * This boolean should be set in the Rulesets under these ITEMS respectively.
 * and then the checks both here and in those lists ought be simplified.
 * @note Put in Ruleset done.
 * @return, true if this item shows in lists without being researched
 */
bool RuleItem::isResearchExempt() const
{
	return _noResearch;
/*	if (getType() == "STR_BLACKSUIT_ARMOR"
		|| getType() == "STR_BLUESUIT_ARMOR"
		|| getType() == "STR_GREENSUIT_ARMOR"
		|| getType() == "STR_ORANGESUIT_ARMOR"
		|| getType() == "STR_PINKSUIT_ARMOR"
		|| getType() == "STR_PURPLESUIT_ARMOR"
		|| getType() == "STR_REDSUIT_ARMOR"
		|| getType() == "STR_BLACK_ARMOR"
		|| getType() == "STR_BLUE_ARMOR"
		|| getType() == "STR_GREEN_ARMOR"
		|| getType() == "STR_ORANGE_ARMOR"
		|| getType() == "STR_PINK_ARMOR"
		|| getType() == "STR_PURPLE_ARMOR"
		|| getType() == "STR_RED_ARMOR")
	{
		return true;
	}
	return false; */
}

/**
 * Checks if this item is capable of executing a BattleUnit.
 * @return, true if executable
 */
bool RuleItem::canExecute() const
{
	return _canExecute;
}

/**
 * Gets if an explosion of the item causes an electro-magnetic pulse.
 * @return, true if EM pulse
 */
bool RuleItem::defusePulse() const
{
	return _defusePulse;
}

}
