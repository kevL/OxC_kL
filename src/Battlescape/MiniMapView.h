/*
 * Copyright 2010-2013 OpenXcom Developers.
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

#ifndef OPENXCOM_MINIMAPVIEW_H
#define OPENXCOM_MINIMAPVIEW_H

#include "Position.h"

#include "../Engine/InteractiveSurface.h"


namespace OpenXcom
{

class Camera;
class Game;
class SavedBattleGame;
class SurfaceSet;


/**
 * MiniMapView is the class used to display the map in the MiniMapState.
 */
class MiniMapView
	:
		public InteractiveSurface
{

private:
	bool
		_isMouseScrolled,
		_isMouseScrolling,
		_mouseMovedOverThreshold;

	int
		_frame,
		_mouseScrollX,
		_mouseScrollY,
		_totalMouseMoveX,
		_totalMouseMoveY,
		_xBeforeMouseScrolling,
		_yBeforeMouseScrolling;

	Camera* _camera;
	Game* _game;
	SavedBattleGame* _battleGame;
	SurfaceSet* _set;

	// these two are required for right-button scrolling on the minimap
	Position _posBeforeMouseScrolling;
	Uint32 _mouseScrollingStartTime;

	/// Handles pressing on the MiniMap.
	void mousePress(Action* action, State* state);
	/// Handles clicking on the MiniMap.
	void mouseClick(Action* action, State* state);
	/// Handles moving mouse over the MiniMap.
	void mouseOver(Action* action, State* state);
	/// Handles moving the mouse into the MiniMap surface.
	void mouseIn(Action* action, State* state);


	public:
		/// Creates the MiniMapView.
		MiniMapView(
				int w,
				int h,
				int x,
				int y,
				Game* game,
				Camera* camera,
				SavedBattleGame* battleGame);

		/// Draws the minimap.
		void draw();

		/// Changes the displayed minimap level.
		int up();
		/// Changes the displayed minimap level.
		int down();

		/// Updates the minimap animation.
		void animate();
};

}

#endif
