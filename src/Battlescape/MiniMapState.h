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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_MINIMAP_H
#define OPENXCOM_MINIMAP_H

#include "../Engine/State.h"


namespace OpenXcom
{

class Camera;
class InteractiveSurface;
class MiniMapView;
class SavedBattleGame;
class Text;
class Timer;


/**
 * The MiniMap is an overhead representation of the Battlescape map;
 * a strategic view that allows you to see more of the map.
 */
class MiniMapState
	:
		public State
{

private:
	InteractiveSurface
		* _btnLvlDwn,
		* _btnLvlUp,
		* _btnOk,
		* _surface;
	MiniMapView* _miniMapView;
	Text* _txtLevel;
	Timer* _timerAnimate;

	/// Handles Minimap animation.
	void animate();


	public:
		/// Creates the MiniMapState.
		MiniMapState(
				Game* game,
				Camera* camera,
				SavedBattleGame* battleGame);
		/// Cleans up the MiniMapState.
		~MiniMapState();

		/// Handler for the OK button.
		void btnOkClick(Action* action);
		/// Handler for the one level up button.
		void btnLevelUpClick(Action* action);
		/// Handler for the one level down button.
		void btnLevelDownClick(Action* action);
		/// Handler for right-clicking anything.
		void handle(Action* action);

		/// Handles timers.
		void think ();
};

}

#endif
