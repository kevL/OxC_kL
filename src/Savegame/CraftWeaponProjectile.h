/*
 * Copyright 2010 OpenXcom Developers.
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

#ifndef OPENXCOM_WEAPONPROJECTILE_H
#define OPENXCOM_WEAPONPROJECTILE_H

#include <string>


namespace OpenXcom
{

class Surface;


// Do not change the order of these enums because they are related to blob order.
enum CraftWeaponProjectileGlobalType
{
	CWPGT_MISSILE,	// 0
	CWPGT_BEAM		// 1
};

enum CraftWeaponProjectileType
{
	CWPT_STINGRAY_MISSILE,	// 0
	CWPT_AVALANCHE_MISSILE,	// 1
	CWPT_CANNON_ROUND,		// 2
	CWPT_FUSION_BALL,		// 3
	CWPT_LASER_BEAM,		// 4
	CWPT_PLASMA_BEAM		// 5
};

enum Directions
{
	D_NONE,	// 0
	D_UP,	// 1
	D_DOWN	// 2
};

const int HP_LEFT	= -1;
const int HP_CENTER	= 0;
const int HP_RIGHT	= 1;


class CraftWeaponProjectile
{

private:
	bool
		_missed,
		_toBeRemoved;
	int
		_accuracy,
		_currentPosition,	// relative to interceptor, apparently, which is a problem
							// when the interceptor disengages while projectile is in flight.

							// kL_note: also, something screws with when a missile is launched
							// but UFO is downed, by other weapon, before it hits; the missile
							// is then not removed from the craft's ordnance.
		_damage,
		_direction,
		_distanceCovered,
		_horizontalPosition,
		_range,
		_speed,
		_state;

	CraftWeaponProjectileGlobalType _globalType;
	CraftWeaponProjectileType _type;

	
	public:
		///
		CraftWeaponProjectile();
		///
		~CraftWeaponProjectile(void);

		/// Sets projectile type. This determines its speed.
		void setType(CraftWeaponProjectileType type);
		/// Returns projectile type.
		CraftWeaponProjectileType getType() const;
		/// Returns projectile global type.
		CraftWeaponProjectileGlobalType getGlobalType() const;

		/// Sets projectile direction. This determines it's initial position.
		void setDirection(const int &directon);
		/// Gets projectile direction.
		int getDirection() const;

		/// Moves projectile in _direction with _speed.
		void move();

		/// Gets projectile position.
		int getPosition() const;
		/// Sets projectile position.
		void setPosition(const int& position);
		/// Sets horizontal position. This determines from which weapon projectile has been fired.
		void setHorizontalPosition(int position);
		/// Gets horizontal position.
		int getHorizontalPosition() const;

		/// Marks projectile to be removed.
		void remove();
		/// Returns true if the projectile should be removed.
		bool toBeRemoved() const;

		/// Returns state of the beam.
		int getState() const;

		/// Sets power of the projectile.
		void setDamage(const int& damage);
		/// Gets power of the projectile.
		int getDamage() const;

		/// Sets accuracy of the projectile.
		void setAccuracy(const int& accuracy);
		/// Gets accuracy of the projectile.
		int getAccuracy() const;

		/// Sets the projectile to missed status.
		void setMissed(const bool& missed);
		/// Gets the projectile missed status.
		bool getMissed() const;

		/// Sets maximum range of projectile.
		void setRange(const int& range);
		/// Gets maximum range of projectile.
		int getRange() const;

		/// Sets the speed of a missile type projectile.
		void setSpeed(const int speed);
};

}

#endif
