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
 * @param rules Pointer to ruleset.
 * @param ammo Initial ammo.
 */
CraftWeapon::CraftWeapon(
		RuleCraftWeapon* rules,
		int ammo)
	:
		_rules(rules),
		_ammo(ammo),
		_rearming(false)
{
}

/**
 *
 */
CraftWeapon::~CraftWeapon()
{
}

/**
 * Loads the craft weapon from a YAML file.
 * @param node YAML node.
 */
void CraftWeapon::load(const YAML::Node& node)
{
	_ammo		= node["ammo"].as<int>(_ammo);
	_rearming	= node["rearming"].as<bool>(_rearming);
}

/**
 * Saves the base to a YAML file.
 * @return YAML node.
 */
YAML::Node CraftWeapon::save() const
{
	YAML::Node node;

	node["type"]			= _rules->getType();
	node["ammo"]			= _ammo;
	if (_rearming)
		node["rearming"]	= _rearming;

	return node;
}

/**
 * Gets the ruleset for a craft weapon's type.
 * @return, pointer to the ruleset
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

	int maxAmmo = _rules->getAmmoMax();
	if (_ammo > maxAmmo)
		_ammo = maxAmmo;

	return true;
}

/**
 * Gets whether this craft weapon needs rearming.
 * @return, rearming status
 */
bool CraftWeapon::isRearming() const
{
	return _rearming;
}

/**
 * Sets whether this craft weapon needs rearming
 * (for example, in case there's no more ammo).
 * @param rearming - rearming status
 */
void CraftWeapon::setRearming(bool rearming)
{
	_rearming = rearming;
}

/**
 * Rearms this craft weapon's ammo.
 * @param baseClips	- the amount of clips available at the Base
 * @param clipSize	- the amount of rounds in a clip
 * @return, the amount of clips used
 */
int CraftWeapon::rearm(
		const int baseClips,
		const int clipSize)
{
	int
		rate = _rules->getRearmRate(),
		full = _rules->getAmmoMax(),
		req = 0;

	if (clipSize > 0)
	{
		req = std::min( // +(clipSize-1) for rounding up
					rate,
					full - _ammo + clipSize - 1) / clipSize;
	}

	int load = baseClips * clipSize;
	if (clipSize < 1 // kL
		|| baseClips >= req)
	{
		load = rate;
	}

	setAmmo(_ammo + load);

	_rearming = (_ammo < full);

	if (clipSize < 1)
		return 0;

	int ret = load / clipSize;

	if (ret < 1) // kL, 1 clip used.
		return 1;

	return ret;
}
//--
//	int
/*	load = 0;

	if (clipSize > 0)
	{
		load = std::min(
					rate / clipSize,
					std::max(
							1,
							(full - _ammo) / clipSize));
	}

	if (baseClips >= load)
		setAmmo(_ammo + rate);
	else
		setAmmo(_ammo + (clipSize * baseClips));

	if (_ammo >= full)
	{
		_ammo = full;
		_rearming = false;
	}

	return load; */

/**
 * Fires a projectile from craft's weapon.
 * @return, Pointer to the new projectile
 */
CraftWeaponProjectile* CraftWeapon::fire() const
{
	CraftWeaponProjectile* p = new CraftWeaponProjectile();

	p->setType(this->getRules()->getProjectileType());
	p->setSpeed(this->getRules()->getProjectileSpeed());
	p->setAccuracy(this->getRules()->getAccuracy());
	p->setDamage(this->getRules()->getDamage());
	p->setRange(this->getRules()->getRange());

	return p;
}

/**
 * Get how many clips are loaded in this weapon.
 * @param ruleset, A pointer to the core ruleset
 * @return, The number of clips loaded
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
