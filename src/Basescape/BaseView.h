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

#ifndef OPENXCOM_BASEVIEW_H
#define OPENXCOM_BASEVIEW_H

#include "../Engine/InteractiveSurface.h"

#include "../Savegame/Base.h"


namespace OpenXcom
{

class Base;
class BaseFacility;
class Font;
class Language;
class RuleBaseFacility;
class Surface;
class SurfaceSet;
class Timer;


/**
 * Interactive view of a base.
 * @note Takes a Base and displays all its facilities and status.
 */
class BaseView
	:
		public InteractiveSurface
{

private:
	static const int GRID_SIZE = 32;

	bool _blink;
	int
		_gridX,
		_gridY;
	size_t _selSize;
	Uint8
		_cellColor,
		_selectorColor;

	Base* _base;
	BaseFacility
		* _facilities[Base::BASE_SIZE]
					 [Base::BASE_SIZE],
		* _selFacility;
	Font
		* _big,
		* _small;
	Language* _lang;
	Surface
		* _srfDog,
		* _selector;
	SurfaceSet* _texture;
	Timer* _timer;

	/// Updates the neighborFacility's build time. This is for internal use only (reCalcQueuedBuildings()).
	void updateNeighborFacilityBuildTime(
			const BaseFacility* const facility,
			BaseFacility* const neighbor);


	public:
		/// Creates a new base view at the specified position and size.
		BaseView(
				int width,
				int height,
				int x = 0,
				int y = 0);
		/// Cleans up the base view.
		~BaseView();

		/// Initializes the base view's various resources.
		void initText(
				Font* big,
				Font* small,
				Language* lang);

		/// Sets the base to display.
		void setBase(Base* base);

		/// Sets the texture for this base view.
		void setTexture(SurfaceSet* const texture);
		/// Sets the dog for this base view.
		void setDog(Surface* const dog);

		/// Gets the currently selected facility.
		BaseFacility* getSelectedFacility() const;
		/// Prevents any mouseover bugs on dismantling base facilities before setBase has had time to update the base.
		void resetSelectedFacility();

		/// Gets the X position of the currently selected square.
		int getGridX() const;
		/// Gets the Y position of the currently selected square.
		int getGridY() const;

		/// Sets whether the base view is selectable.
		void setSelectable(size_t facSize);

		/// Checks if a facility can be placed.
		bool isPlaceable(const RuleBaseFacility* const facRule) const;
		/// Checks if the placed facility is placed in queue or not.
		bool isQueuedBuilding(const RuleBaseFacility* const facRule) const;
		/// ReCalculates the remaining build-time of all queued buildings.
		void reCalcQueuedBuildings();

		/// Handles the timers.
		void think();
		/// Blinks the selector.
		void blink();
		/// Draws the base view.
		void draw();
		/// Blits the base view onto another surface.
		void blit(Surface* surface);

		/// Special handling for mouse hovers.
		void mouseOver(Action* action, State* state);
		/// Special handling for mouse hovering out.
		void mouseOut(Action* action, State* state);

		/// Sets the primary color.
		void setColor(Uint8 color);
		/// Sets the secondary color.
		void setSecondaryColor(Uint8 color);
};

}

#endif
