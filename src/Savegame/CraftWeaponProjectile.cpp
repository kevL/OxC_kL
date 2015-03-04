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

#include "CraftWeaponProjectile.h"

//#include <iostream>

//#include "../Engine/Palette.h"
#include "../Engine/Surface.h"
#include "../Engine/SurfaceSet.h"

#include "../Ruleset/RuleCraftWeapon.h"


namespace OpenXcom
{

/**
 * cTor.
 */
CraftWeaponProjectile::CraftWeaponProjectile()
	:
		_type(CWPT_CANNON_ROUND),
		_globalType(CWPGT_MISSILE),
		_speed(0),
		_direction(D_NONE),
		_currentPosition(0),
		_horizontalPosition(0),
		_state(0),
		_accuracy(0),
		_damage(0),
		_range(0),
		_toBeRemoved(false),
		_missed(false),
		_distanceCovered(0)
{}

/**
 * dTor.
 */
CraftWeaponProjectile::~CraftWeaponProjectile()
{}

/**
 * Sets the type of projectile according to the type of weapon it was shot from.
 * This is used for drawing the projectiles.
 * @param type - CraftWeaponProjectileType (CraftWeaponProjectile.h)
 */
void CraftWeaponProjectile::setType(CraftWeaponProjectileType type)
{
	_type = type;

	if (type >= CWPT_LASER_BEAM)
	{
		_globalType = CWPGT_BEAM;
		_state = 8;
	}
}

/**
 * Returns the type of projectile.
 * @return, projectile type as an integer value (CraftWeaponProjectile.h)
 */
CraftWeaponProjectileType CraftWeaponProjectile::getType() const
{
	return _type;
}

/**
 * Returns the global type of projectile.
 * @return, 0 - if it's a missile, 1 if beam (CraftWeaponProjectile.h)
 */
CraftWeaponProjectileGlobalType CraftWeaponProjectile::getGlobalType() const
{
	return _globalType;
}

/**
 * Sets the direction of the projectile.
 * @param direction - reference the direction
 */
void CraftWeaponProjectile::setDirection(const int& directon)
{
	_direction = directon;

	if (_direction == D_UP)
		_currentPosition = 0;
}

/**
 * Gets the direction of the projectile.
 * @return, the direction
 */
int CraftWeaponProjectile::getDirection() const
{
	return _direction;
}

/**
 * Moves the projectile according to its speed or changes the phase of beam animation.
 */
void CraftWeaponProjectile::moveProjectile()
{
	if (_globalType == CWPGT_MISSILE)
	{
		int positionChange = _speed;

		// Check if projectile would reach its maximum range this tick.
		if ((_distanceCovered / 8) < getRange()
			&& ((_distanceCovered + _speed) / 8) >= getRange())
		{
			positionChange = getRange() * 8 - _distanceCovered;
		}

		// Check if projectile passed its maximum range on previous tick.
		if ((_distanceCovered / 8) >= getRange())
			setMissed(true);

		if (_direction == D_UP)
			_currentPosition += positionChange;
		else if (_direction == D_DOWN)
			_currentPosition -= positionChange;

		_distanceCovered += positionChange;
	}
	else if (_globalType == CWPGT_BEAM)
	{
		_state /= 2;

		if (_state == 1)
			_toBeRemoved = true;
	}
}

/**
 * Sets the y position of the projectile on the radar.
 * @param position - reference the position
 */
void CraftWeaponProjectile::setPosition(const int& position)
{
	_currentPosition = position;
}

/**
 * Gets the y position of the projectile on the radar.
 * @return, the position
 */
int CraftWeaponProjectile::getPosition() const
{
	return _currentPosition;
}

/**
 * Sets the x position of the projectile on the radar.
 * It's used only once for each projectile during firing.
 * @param position - the x position
 */
void CraftWeaponProjectile::setHorizontalPosition(int position)
{
	_horizontalPosition = position;
}

/**
 * Gets the x position of the projectile.
 * @return, the x position
 */
int CraftWeaponProjectile::getHorizontalPosition() const
{
	return _horizontalPosition;
}

/**
 * Marks the projectile to be removed.
 */
void CraftWeaponProjectile::removeProjectile()
{
	_toBeRemoved = true;
}

/**
 * Returns if a projectile should be removed.
 * @return, true to remove
 */
bool CraftWeaponProjectile::toBeRemoved() const
{
	return _toBeRemoved;
}

/**
 * Returns animation state of a beam.
 * @return, the state of the beam
 */
int CraftWeaponProjectile::getState() const
{
	return _state;
}

/**
 * Sets the amount of damage the projectile can do when hitting its target.
 * @param damage - reference the damage value
 */
void CraftWeaponProjectile::setDamage(const int& damage)
{
	_damage = damage;
}

/**
 * Gets the amount of damage the projectile can do when hitting its target.
 * @return, the damage to deliver
 */
int CraftWeaponProjectile::getDamage() const
{
	return _damage;
}

/**
 * Sets the accuracy of the projectile.
 * @param accuracy - reference the accuracy
 */
void CraftWeaponProjectile::setAccuracy(const int& accuracy)
{
	_accuracy = accuracy;
}

/**
 * Gets the accuracy of the projectile.
 * @return, the accuracy
 */
int CraftWeaponProjectile::getAccuracy() const
{
	return _accuracy;
}

/**
 * Marks the projectile as a one which missed its target.
 * @param missed - reference true for missed
 */
void CraftWeaponProjectile::setMissed(const bool& missed)
{
	_missed = missed;
}

/**
 * Returns true if the projectile missed its target.
 * @return, true if missed
 */
bool CraftWeaponProjectile::getMissed() const
{
	return _missed;
}

/**
 * Sets maximum range of projectile.
 * @param range - reference the range
 */
void CraftWeaponProjectile::setRange(const int& range)
{
	_range = range;
}

/**
 * Returns maximum range of projectile.
 * @return, the range
 */
int CraftWeaponProjectile::getRange() const
{
	return _range;
}

/**
 * Sets the speed of the projectile.
 * @param speed - the speed
 */
void CraftWeaponProjectile::setSpeed(int speed)
{
	_speed = speed;
}

}
