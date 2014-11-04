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

#include "CraftWeapon.h"

#include "CraftWeaponProjectile.h"

#include "../Ruleset/RuleCraftWeapon.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"


namespace OpenXcom
{

/**
 * Initializes a craft weapon of the specified type.
 * @param rules	- pointer to RuleCraftWeapon
 * @param ammo	- initial ammo quantity
 */
CraftWeapon::CraftWeapon(
		RuleCraftWeapon* rules,
		int ammo)
	:
		_rules(rules),
		_ammo(ammo),
		_rearming(false),
		_cantLoad(false)
{
}

/**
 * dTor.
 */
CraftWeapon::~CraftWeapon()
{
}

/**
 * Loads the craft weapon from a YAML file.
 * @param node - reference a YAML node
 */
void CraftWeapon::load(const YAML::Node& node)
{
	_ammo		= node["ammo"]		.as<int>(_ammo);
	_rearming	= node["rearming"]	.as<bool>(_rearming);
	_cantLoad	= node["cantLoad"]	.as<bool>(_cantLoad);
}

/**
 * Saves the base to a YAML file.
 * @return, YAML node
 */
YAML::Node CraftWeapon::save() const
{
	YAML::Node node;

	node["type"]			= _rules->getType();
	node["ammo"]			= _ammo;
	if (_rearming)
		node["rearming"]	= _rearming;
	if (_cantLoad)
		node["cantLoad"]	= _cantLoad;

	return node;
}

/**
 * Gets the ruleset for a craft weapon's type.
 * @return, pointer to RuleCraftWeapon
 */
RuleCraftWeapon* CraftWeapon::getRules() const
{
	return _rules;
}

/**
 * Gets the amount of ammo contained in this craft weapon.
 * @return, amount of ammo
 */
int CraftWeapon::getAmmo() const
{
	return _ammo;
}

/**
 * Sets the amount of ammo contained in this craft weapon.
 * Also maintains min/max levels.
 * @param ammo - amount of ammo
 * @return, true if there was enough ammo to fire a round off
 */
bool CraftWeapon::setAmmo(int ammo)
{
	_ammo = ammo;

	if (_ammo < 0)
	{
		_ammo = 0;
		return false;
	}

	if (_ammo > _rules->getAmmoMax())
		_ammo = _rules->getAmmoMax();

	return true;
}

/**
 * Gets whether this craft weapon needs rearming.
 * @return, rearming status
 */
bool CraftWeapon::getRearming() const
{
	return _rearming;
}

/**
 * Sets whether this craft weapon needs rearming - in case there's no more ammo.
 * @param rearming - rearming status (default true)
 */
void CraftWeapon::setRearming(const bool rearming)
{
	_rearming = rearming;
}

/**
 * Rearms this craft weapon's ammo.
 * @param baseClips	- the amount of clips available at the Base (default 0)
 * @param clipSize	- the amount of rounds in a clip (default 0)
 * @return, the amount of clips used (negative if not enough clips at Base)
 */
int CraftWeapon::rearm(
		const int baseClips,
		const int clipSize)
{
	const int
		fullQty = _rules->getAmmoMax(),
		rateQty = _rules->getRearmRate();

	int clipsRequest = 0;
	if (clipSize > 0)
	{
		clipsRequest = std::min( // (+clipSize-1) <- round up int
							rateQty + clipSize - 1,
							fullQty - _ammo + clipSize - 1)
						/ clipSize;
	}

	int loadQty = 0;
	if (baseClips >= clipsRequest)
	{
		_cantLoad = false;

		if (clipSize == 0)
			loadQty = rateQty;
		else
			loadQty = clipsRequest * clipSize;
	}
	else // baseClips < clipsRequest
	{
		_cantLoad = true;

		loadQty = baseClips * clipSize;
	}

	setAmmo(_ammo + loadQty);
	_rearming = (_ammo < fullQty);

	if (clipSize == 0)
		return 0;

	int ret = (loadQty + clipSize - 1) / clipSize;
	if (clipsRequest > baseClips)
		ret = -ret; // trick to tell Craft there isn't enough clips at Base.

	return ret;
}

/**
 * Gets this CraftWeapon's cantLoad status - no stock in Base Stores.
 * @return, true if weapon ammo is low in stock
 */
bool CraftWeapon::getCantLoad() const
{
	return _cantLoad;
}

/**
 * Sets this CraftWeapon's cantLoad status - no stock in Base Stores.
 * @param cantLoad - true if weapon ammo is low in stock (default true)
 */
void CraftWeapon::setCantLoad(const bool cantLoad)
{
	_cantLoad = cantLoad;
}

/**
 * Fires a projectile from this CraftWeapon.
 * @return, pointer to the fired CraftWeaponProjectile
 */
CraftWeaponProjectile* CraftWeapon::fire() const
{
	CraftWeaponProjectile* const proj = new CraftWeaponProjectile();

	proj->setType(this->getRules()->getProjectileType());
	proj->setSpeed(this->getRules()->getProjectileSpeed());
	proj->setAccuracy(this->getRules()->getAccuracy());
	proj->setDamage(this->getRules()->getDamage());
	proj->setRange(this->getRules()->getRange());

	return proj;
}

/**
 * Gets how many clips are loaded in this CraftWeapon.
 * @param ruleset - pointer to the core Ruleset
 * @return, the number of clips loaded
 */
int CraftWeapon::getClipsLoaded(Ruleset* ruleset)
{
	int ret = static_cast<int>(
					floor(static_cast<float>(_ammo) / static_cast<float>(_rules->getRearmRate())));

	RuleItem* clip = ruleset->getItem(_rules->getClipItem());
	if (clip
		&& clip->getClipSize() > 0)
	{
		ret = static_cast<int>(
					floor(static_cast<float>(_ammo) / static_cast<float>(clip->getClipSize())));
	}

	return ret;
}

}
