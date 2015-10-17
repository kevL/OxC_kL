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

#ifndef OPENXCOM_PROJECTILE_H
#define OPENXCOM_PROJECTILE_H

//#include <vector>

#include "BattlescapeGame.h"
#include "Position.h"

#include "../Ruleset/MapData.h"


namespace OpenXcom
{

class BattleItem;
class ResourcePack;
class SavedBattleGame;
class Surface;
class Tile;


/**
 * A class that represents a projectile.
 * @note Map is the owner of an instance of this class during its short life.
 * It calculates its own trajectory and then moves along this pre-calculated
 * trajectory in voxel space.
 */
class Projectile
{

private:
	static const double PCT;
	static Position targetVoxel_cache;


//	bool _reversed;
	int
		_bulletSprite,
		_speed;
	size_t _trjId;

	BattleAction _action;
	Position
		_posOrigin,
		_targetVoxel;
	const SavedBattleGame* _battleSave;
	Surface* _throwSprite;

	std::vector<Position> _trj;

	///
	void applyAccuracy(
			const Position& originVoxel,
			Position* const targetVoxel,
			double accuracy,
			const Tile* const tileTarget);
	/// Gets distance modifiers to accuracy.
	double rangeAccuracy( // private.
			const RuleItem* const itRule,
			int dist) const;
	/// Gets target-terrain and/or target-unit modifiers to accuracy.
	double targetAccuracy(
			const BattleUnit* const targetUnit,
			int elevation,
			const Tile* tileTarget) const;
	///
	bool verifyTarget(const Position& originVoxel);


	public:
		/// Creates a new Projectile.
		Projectile(
				const ResourcePack* const res,
				const SavedBattleGame* const battleSave,
				const BattleAction& action,
				const Position& posOrigin,
				const Position& targetVoxel);
		/// Cleans up the Projectile.
		~Projectile();

		/// Calculates the trajectory for a straight path.
		VoxelType calculateShot(double accuracy);
		VoxelType calculateShot(
				double accuracy,
				const Position& originVoxel);
		/// Calculates the trajectory for a curved path.
		VoxelType calculateThrow(double accuracy);

		/// Moves the projectile one step in its trajectory.
		bool traceProjectile();
		/// Skips the bullet flight.
		void skipTrajectory();

		/// Gets the projectile's current position in voxel-space.
		Position getPosition(int offsetId = 0) const;
		/// Gets an ID for the projectile's current surface.
		int getBulletSprite(int id) const;
		/// Gets the thrown item.
		BattleItem* getThrowItem() const;
		/// Gets the thrown item's sprite.
		Surface* getThrowSprite() const;

		/// Gets the BattleAction associated with this projectile.
		BattleAction* getBattleAction();

		/// Gets the ACTUAL target-position for this projectile.
		Position getFinalPosition() const;
		/// Gets the final direction of the projectile's trajectory as a unit-vector.
		Position getStrikeVector() const;

		/// Stores the final direction of a missile or thrown-object.
//		void storeProjectileDirection() const;
		/// Gets the Position of origin for the projectile.
//		Position getOrigin();
		/// Gets the INTENDED target-position for the projectile.
//		Position getTarget() const;
		/// Gets if this this projectile is being drawn back-to-front or front-to-back.
//		bool isReversed() const;
};

}

#endif
