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


namespace OpenXcom
{

/**
 * cTor.
 */
CraftWeaponProjectile::CraftWeaponProjectile()
	:
		_type(PT_CANNON_ROUND),
		_globalType(PGT_MISSILE),
		_speed(0),
		_dir(PD_NONE),
		_pos(0),
		_posHori(PH_CENTER),
		_beamPhase(0),
		_accuracy(0),
		_power(0),
		_range(0),
		_done(false),
		_missed(false),
		_dist(0)
{}

/**
 * dTor.
 */
CraftWeaponProjectile::~CraftWeaponProjectile()
{}

/**
 * Sets the type of projectile according to the type of weapon it was shot from.
 * @note This is used for drawing the projectiles.
 * @param type - CwpType (CraftWeaponProjectile.h)
 */
void CraftWeaponProjectile::setType(CwpType type)
{
	_type = type;

	if (type >= PT_LASER_BEAM)
	{
		_globalType = PGT_BEAM;
		_beamPhase = 8;
	}
}

/**
 * Returns the type of projectile.
 * @return, projectile type as an integer value (CraftWeaponProjectile.h)
 */
CwpType CraftWeaponProjectile::getType() const
{
	return _type;
}

/**
 * Returns the global type of projectile.
 * @return, 0 if it's a missile, 1 if beam (CraftWeaponProjectile.h)
 */
CwpGlobal CraftWeaponProjectile::getGlobalType() const
{
	return _globalType;
}

/**
 * Sets the direction of the projectile.
 * @param direction - direction
 */
void CraftWeaponProjectile::setDirection(CwpDirection dir)
{
	_dir = dir;

	if (_dir == PD_UP)
		_pos = 0;
}

/**
 * Gets the direction of the projectile.
 * @return, the direction
 */
CwpDirection CraftWeaponProjectile::getDirection() const
{
	return _dir;
}

/**
 * Moves the projectile according to its speed or changes the phase of beam animation.
 */
void CraftWeaponProjectile::moveProjectile()
{
	if (_globalType == PGT_MISSILE)
	{
		// Check if projectile would reach its maximum range this tick.
		int delta;
		if (_range > (_dist / 8)
			&& _range <= ((_dist + _speed) / 8))
		{
			delta = (_range * 8) - _dist;
		}
		else
			delta = _speed;

		// Check if projectile passed its maximum range on previous tick.
		if (_range <= (_dist / 8))
			_missed = true;

		if (_dir == PD_UP)
			_pos += delta;
		else if (_dir == PD_DOWN)
			_pos -= delta;

		_dist += delta;
	}
	else if (_globalType == PGT_BEAM)
	{
		_beamPhase /= 2;

		if (_beamPhase == 1)
			_done = true;
	}
}

/**
 * Sets the y position of the projectile on the radar.
 * @param pos - position
 */
void CraftWeaponProjectile::setPosition(int pos)
{
	_pos = pos;
}

/**
 * Gets the y position of the projectile on the radar.
 * @return, the position
 */
int CraftWeaponProjectile::getPosition() const
{
	return _pos;
}

/**
 * Sets the x position of the projectile on the radar.
 * @note This is used only once for each projectile during firing.
 * @param pos - the x position
 */
void CraftWeaponProjectile::setHorizontalPosition(int pos)
{
	_posHori = pos;
}

/**
 * Gets the x position of the projectile.
 * @return, the x position
 */
int CraftWeaponProjectile::getHorizontalPosition() const
{
	return _posHori;
}

/**
 * Marks the projectile to be removed.
 */
void CraftWeaponProjectile::removeProjectile()
{
	_done = true;
}

/**
 * Returns if a projectile should be removed.
 * @return, true to remove
 */
bool CraftWeaponProjectile::toBeRemoved() const
{
	return _done;
}

/**
 * Returns animation state of a beam.
 * @return, the state of the beam
 */
int CraftWeaponProjectile::getBeamPhase() const
{
	return _beamPhase;
}

/**
 * Sets the amount of damage the projectile can do when hitting its target.
 * @param power - damage value
 */
void CraftWeaponProjectile::setPower(int power)
{
	_power = power;
}

/**
 * Gets the amount of damage the projectile can do when hitting its target.
 * @return, the damage to deliver
 */
int CraftWeaponProjectile::getPower() const
{
	return _power;
}

/**
 * Sets the accuracy of the projectile.
 * @param accuracy - accuracy
 */
void CraftWeaponProjectile::setAccuracy(int accuracy)
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
 * @param missed - true for missed (default true)
 */
void CraftWeaponProjectile::setMissed(bool missed)
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
 * @param range - range
 */
void CraftWeaponProjectile::setRange(int range)
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
