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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_MAP_H
#define OPENXCOM_MAP_H

#include <set>
#include <vector>

#include "Position.h"

#include "../Engine/Options.h"
#include "../Engine/InteractiveSurface.h"


namespace OpenXcom
{

extern bool kL_preReveal; // kL


class BattlescapeMessage;
class BattleUnit;
class Camera;
class Explosion;
class NumberText; // kL
class Projectile;
class ResourcePack;
class SavedBattleGame;
class Surface;
//kL class Text;
class Timer;


enum CursorType
{
	CT_NONE,		// 0
	CT_NORMAL,		// 1
	CT_AIM,			// 2
	CT_PSI,			// 3
	CT_WAYPOINT,	// 4
	CT_THROW		// 5
};


/**
 * Interactive map of the battlescape.
 */
class Map
	:
		public InteractiveSurface
{

private:
	static const int SCROLL_INTERVAL	= 15;
	static const int BULLET_SPRITES		= 35;

	CursorType _cursorType;

	bool
		_launch,
		_explosionInFOV,
		_projectileInFOV,
//kL		_smoothCamera,
		_smoothingEngaged,
		_unitDying;
	int
		_animFrame,
		_cursorSize,
		_mouseX,
		_mouseY,
		_reveal, // kL
		_selectorX,
		_selectorY,
		_spriteWidth,
		_spriteHeight,
		_visibleMapHeight;

	PathPreview			_previewSetting;

	BattlescapeMessage	* _message;
	Camera				* _camera;
	Game				* _game;
	NumberText			* _txtAccuracy; // kL
	Projectile			* _projectile;
	ResourcePack		* _res;
	SavedBattleGame		* _save;
	Surface				* _arrow;
//kL	Text* _txtAccuracy;
	Timer
						* _scrollMouseTimer,
						* _scrollKeyTimer;

//	std::set<Explosion*>	_explosions; // kL
	std::list<Explosion*>	_explosions; // new CTD
	std::vector<Position>	_waypoints;

	///
	void drawTerrain(Surface* surface);
	///
	int getTerrainLevel(
			Position pos,
			int size);


	public:
		static const int ICON_WIDTH		= 320;
		static const int ICON_HEIGHT	= 56;

		/// Creates a new map at the specified position and size.
		Map(
				Game* game,
				int width,
				int height,
				int x,
				int y,
				int visibleMapHeight);
		/// Cleans up the map.
		~Map();

		/// Initializes the map.
		void init();
		/// Handles timers.
		void think();
		/// Draws the surface.
		void draw();

		/// Sets the palette.
		void setPalette(
				SDL_Color* colors,
				int firstcolor = 0,
				int ncolors = 256);

		/// Special handling for mouse press.
		void mousePress(Action* action, State* state);
		/// Special handling for mouse release.
		void mouseRelease(Action* action, State* state);
		/// Special handling for mouse over
		void mouseOver(Action* action, State* state);
		/// Special handling for key presses.
		void keyboardPress(Action* action, State* state);
		/// Special handling for key releases.
		void keyboardRelease(Action* action, State* state);

		/// Rotates the tileframes 0-7
		void animate(bool redraw);

		/// Sets the battlescape selector position relative to mouseposition.
		void setSelectorPosition(
				int mx,
				int my);
		/// Gets the currently selected position.
		void getSelectorPosition(Position* pos) const;

		/// Calculates the offset of a soldier, when it is walking in the middle of 2 tiles.
		void calculateWalkingOffset(
				BattleUnit* unit,
				Position* offset);

		/// Sets the 3D cursor type.
		void setCursorType(
				CursorType type,
				int size = 1);
		/// Gets the 3D cursor type.
		CursorType getCursorType() const;

		/// Caches units.
		void cacheUnits();
		/// Caches the unit.
		void cacheUnit(BattleUnit* unit);

		/// Sets projectile.
		void setProjectile(Projectile* projectile);
		/// Gets projectile.
		Projectile* getProjectile() const;

		/// Gets explosion set.
//		std::set<Explosion*>* getExplosions(); // kL
		std::list<Explosion*>* getExplosions(); // new CTD

		/// Gets the pointer to the camera.
		Camera* getCamera();
		/// Mouse-scrolls the camera.
		void scrollMouse();
		/// Keyboard-scrolls the camera.
		void scrollKey();
		/// Refreshes the battlescape selector after scrolling.
		void refreshSelectorPosition();

		/// Get waypoints vector.
		std::vector<Position>* getWaypoints();

		/// Set mouse-buttons' pressed state.
		void setButtonsPressed(
				Uint8 button,
				bool pressed);

		/// Sets the unitDying flag.
		void setUnitDying(bool flag);

		/// Special handling for updating map height.
		void setHeight(int height);
		/// Special handling for updating map width.
		void setWidth(int width);
};

}

#endif
