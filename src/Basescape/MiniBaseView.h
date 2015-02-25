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

#ifndef OPENXCOM_MINIBASEVIEW_H
#define OPENXCOM_MINIBASEVIEW_H

//#include <vector>

#include "../Engine/InteractiveSurface.h"


namespace OpenXcom
{

class Base;
class SurfaceSet;
class Timer;


enum MiniBaseViewType
{
	MBV_STANDARD,	// 0
	MBV_RESEARCH,	// 1
	MBV_PRODUCTION	// 2
};


/**
 * Mini view of a base.
 * Takes all the bases and displays their layout
 * and allows players to swap between them.
 */
class MiniBaseView
	:
		public InteractiveSurface
{

private:
	static const int MINI_SIZE = 14;

	bool _blink;
	size_t
		_baseID,
		_hoverBase;
	Uint8
		_green,
		_red;

	SurfaceSet* _texture;
	Timer* _timer;

	std::vector<Base*>* _bases;

	MiniBaseViewType _mode;


	public:
		static const size_t MAX_BASES = 8;

		/// Creates a new mini base view at the specified position and size.
		MiniBaseView(
				int width,
				int height,
				int x = 0,
				int y = 0,
				MiniBaseViewType mode = MBV_STANDARD);
		/// Cleans up the mini base view.
		~MiniBaseView();

		/// Sets the base list to display.
		void setBases(std::vector<Base*>* bases);

		/// Sets the texture for the mini base view.
		void setTexture(SurfaceSet* texture);

		/// Gets the base the mouse is over.
		size_t getHoveredBase() const;

		/// Sets the selected base for the mini base view.
		void setSelectedBase(size_t base);

		/// Draws the mini base view.
		void draw();

		/// Special handling for mouse hovers.
		void mouseOver(Action* action, State* state);

		///
		void setColor(Uint8 color);
		///
		void setSecondaryColor(Uint8 color);

		/// Handles timer.
		void think();
		/// Blinks the craft status indicators.
		void MiniBaseView::blink();
};

}

#endif
