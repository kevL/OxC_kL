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

#ifndef OPENXCOM_WEAPONPROJECTILE_H
#define OPENXCOM_WEAPONPROJECTILE_H


namespace OpenXcom
{

// Do not change the order of these enums because they are related to blob order.
enum CwpGlobal
{
	PGT_MISSILE,	// 0
	PGT_BEAM		// 1
};

enum CwpType
{
	PT_STINGRAY_MISSILE,	// 0
	PT_AVALANCHE_MISSILE,	// 1
	PT_CANNON_ROUND,		// 2
	PT_FUSION_BALL,			// 3
	PT_LASER_BEAM,			// 4
	PT_PLASMA_BEAM			// 5
};

enum CwpDirection
{
	PD_NONE,	// 0
	PD_UP,		// 1
	PD_DOWN		// 2
};

const int
	PH_LEFT		= -1,
	PH_CENTER	=  0,
	PH_RIGHT	=  1;


class CraftWeaponProjectile
{

private:
	bool
		_missed,
		_done;
	int
		_accuracy,
		_pos,	// relative to interceptor, apparently, which is a problem
				// when the interceptor disengages while projectile is in flight.

				// kL_note: also, something screws with when a missile is launched
				// but UFO is downed, by other weapon, before it hits; the missile
				// is then not removed from the craft's ordnance.
		_power,
		_dist,
		_posHori,
		_range,
		_speed,
		_beamPhase;

	CwpDirection _dir;
	CwpGlobal _globalType;
	CwpType _type;


	public:
		/// cTor.
		CraftWeaponProjectile();
		/// dTor.
		~CraftWeaponProjectile();

		/// Sets projectile type. This determines its speed.
		void setType(CwpType type);
		/// Returns projectile type.
		CwpType getType() const;
		/// Returns projectile global type.
		CwpGlobal getGlobalType() const;

		/// Sets projectile direction. This determines its initial position.
		void setDirection(CwpDirection dir);
		/// Gets projectile direction.
		CwpDirection getDirection() const;

		/// Moves projectile in '_dir' with '_speed'.
		void moveProjectile();

		/// Sets projectile position.
		void setPosition(int pos);
		/// Gets projectile position.
		int getPosition() const;
		/// Sets horizontal position. This determines from which weapon projectile has been fired.
		void setHorizontalPosition(int pos);
		/// Gets horizontal position.
		int getHorizontalPosition() const;

		/// Marks projectile to be removed.
		void removeProjectile();
		/// Returns true if the projectile should be removed.
		bool toBeRemoved() const;

		/// Returns state of the beam.
		int getBeamPhase() const;

		/// Sets power of the projectile.
		void setPower(int power);
		/// Gets power of the projectile.
		int getPower() const;

		/// Sets accuracy of the projectile.
		void setAccuracy(int accuracy);
		/// Gets accuracy of the projectile.
		int getAccuracy() const;

		/// Sets the projectile to missed status.
		void setMissed(bool missed = true);
		/// Gets the projectile missed status.
		bool getMissed() const;

		/// Sets maximum range of projectile.
		void setRange(int range);
		/// Gets maximum range of projectile.
		int getRange() const;

		/// Sets the speed of a missile type projectile.
		void setSpeed(const int speed);
};

}

#endif
