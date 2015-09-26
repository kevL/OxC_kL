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

#ifndef OPENXCOM_UNITSPRITE_H
#define OPENXCOM_UNITSPRITE_H

#include "../Engine/Surface.h"

#include "../Ruleset/MapData.h"


namespace OpenXcom
{

class BattleItem;
class BattleUnit;
class SurfaceSet;


/**
 * A class that renders a specific unit given its render rules combining the
 * right frames from the SurfaceSet.
 */
class UnitSprite
	:
		public Surface
{

private:
	int
		_animationFrame,
		_colorSize,
		_drawingRoutine,
		_part;

	BattleItem
		* _itemA,
		* _itemB;
	BattleUnit* _unit;
	SurfaceSet
		* _itemSurfaceA,
		* _itemSurfaceB,
		* _unitSurface;

	const std::pair<Uint8, Uint8>* _color;

	/// Drawing routine for XCom soldiers in overalls, sectoids (routine 0),
	/// mutons (routine 10).
	void drawRoutine0();
	/// Drawing routine for floaters.
	void drawRoutine1();
	/// Drawing routine for XCom tanks.
	void drawRoutine2();
	/// Drawing routine for cyberdiscs.
	void drawRoutine3();
	/// Drawing routine for civilians, ethereals, zombies (routine 4).
	void drawRoutine4();
	/// Drawing routine for sectopods and reapers.
	void drawRoutine5();
	/// Drawing routine for snakemen.
	void drawRoutine6();
	/// Drawing routine for chryssalid.
	void drawRoutine7();
	/// Drawing routine for silacoids.
	void drawRoutine8();
	/// Drawing routine for celatids.
	void drawRoutine9();

	/// sort two handed sprites out.
	void sortRifles();
	/// Draw surface with changed colors.
	void drawRecolored(Surface* src);


	public:
		/// Creates a new UnitSprite at the specified position and size.
		UnitSprite(
				int width,
				int height,
				int x = 0,
				int y = 0);
		/// Cleans up the UnitSprite.
		~UnitSprite();

		/// Sets surfacesets for rendering.
		void setSurfaces(
				SurfaceSet* const unitSurface,
				SurfaceSet* const itemSurfaceA,
				SurfaceSet* const itemSurfaceB);
		/// Sets the battleunit to be rendered.
		void setBattleUnit(
				BattleUnit* const unit,
				int part = 0);
		/// Sets the battleitem to be rendered.
		void setBattleItem(
				BattleItem* const item);
		/// Sets the animation frame.
		void setAnimationFrame(
				int frame);

		/// Draws the unit.
		void draw();
};

}

#endif
