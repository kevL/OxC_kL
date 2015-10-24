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

#include "Camera.h"

//#define _USE_MATH_DEFINES
//#include <cmath>
//#include <fstream>

#include "BattlescapeState.h"
#include "Map.h"

#include "../Engine/Action.h"
//#include "../Engine/Options.h"
#include "../Engine/Timer.h"

#include "../Savegame/SavedBattleGame.h"


namespace OpenXcom
{

/**
 * Sets up a camera.
 * @param spriteWidth		- width of map sprite
 * @param spriteHeight		- height of map sprite
 * @param mapsize_x			- current map size in X axis
 * @param mapsize_y			- current map size in Y axis
 * @param mapsize_z			- current map size in Z axis
 * @param battleMap			- pointer to Map
 * @param playableHeight	- height of Map surface minus icons-height
 */
Camera::Camera(
		int spriteWidth,
		int spriteHeight,
		int mapsize_x,
		int mapsize_y,
		int mapsize_z,
		Map* battleMap,
		int playableHeight)
	:
		_spriteWidth(spriteWidth),
		_spriteHeight(spriteHeight),
		_mapsize_x(mapsize_x),
		_mapsize_y(mapsize_y),
		_mapsize_z(mapsize_z),
		_map(battleMap),
		_playableHeight(playableHeight),
		_screenWidth(battleMap->getWidth()),
		_screenHeight(battleMap->getHeight()),
		_mapOffset(-250,250,0),
		_scrollMouseTimer(0),
		_scrollKeyTimer(0),
		_scrollMouseX(0),
		_scrollMouseY(0),
		_scrollKeyX(0),
		_scrollKeyY(0),
		_scrollTrigger(false),
		_showLayers(false),
		_pauseAfterShot(false)
{}

/**
 * Deletes the Camera.
 */
Camera::~Camera()
{}

/**
 * Sets this Camera's mouse- and keyboard- scrolling timers for use in the
 * battlefield.
 * @note Goebbels wants pics.
 * @param mouseTimer	- pointer to mouse Timer
 * @param keyboardTimer	- pointer to keyboard Timer
 */
void Camera::setScrollTimers(
		Timer* mouseTimer,
		Timer* keyboardTimer)
{
	_scrollMouseTimer = mouseTimer;
	_scrollKeyTimer = keyboardTimer;
}

/**
 * Sets the value to min if it is below min and to max if it is above max.
 * @param value		- pointer to the value
 * @param minValue	- the minimum value
 * @param maxValue	- the maximum value
 */
void Camera::intMinMax( // private.
		int* value,
		int minValue,
		int maxValue) const
{
	if (*value < minValue)
		*value = minValue;
	else if (*value > maxValue)
		*value = maxValue;
}

/**
 * Handles camera mouse shortcuts.
 * @param action	- pointer to an Action
 * @param state		- State that the action handlers belong to
 */
void Camera::mousePress(Action* action, State*)
{
	if (Options::battleDragScrollButton != SDL_BUTTON_MIDDLE
		|| (SDL_GetMouseState(NULL,NULL) & SDL_BUTTON(Options::battleDragScrollButton)) == 0)
	{
		if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
			down();
		else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
			up();
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_LEFT
		&& Options::battleEdgeScroll == SCROLL_TRIGGER)
	{
		_scrollTrigger = true;
		mouseOver(action, NULL);
	}
}

/**
 * Handles camera mouse shortcuts.
 * @param action	- pointer to an Action
 * @param state		- State that the action handlers belong to
 */
void Camera::mouseRelease(Action* action, State*)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT
		&& Options::battleEdgeScroll == SCROLL_TRIGGER)
	{
		_scrollMouseX =
		_scrollMouseY = 0;
		_scrollMouseTimer->stop();
		_scrollTrigger = false;

		int
			posX = action->getXMouse(),
			posY = action->getYMouse();

		if ((posX > 0
				&& posX < SCROLL_BORDER * action->getXScale())
			|| posX > (_screenWidth - SCROLL_BORDER) * action->getXScale()
			|| (posY > 0
				&& posY < SCROLL_BORDER * action->getYScale())
			|| posY > (_screenHeight - SCROLL_BORDER) * action->getYScale())
		{
			// A cheap hack to avoid handling this event as a click
			// on the map when the mouse is on the scroll-border
			action->getDetails()->button.button = 0;
		}
	}
}

/**
 * Handles mouse over events.
 * @param action	- pointer to an Action
 * @param state		- State that the action handlers belong to
 */
void Camera::mouseOver(Action* action, State*)
{
	if (_map->getCursorType() == CT_NONE)
		return;

	if (Options::battleEdgeScroll == SCROLL_AUTO
		|| _scrollTrigger == true)
	{
		int
			posX = action->getXMouse(),
			posY = action->getYMouse(),
			scrollSpeed = Options::battleScrollSpeed;

		if (posX < SCROLL_BORDER * action->getXScale()) // left scroll
		{
			_scrollMouseX = scrollSpeed;

			// if close to top or bottom, also scroll diagonally
			if (posY < SCROLL_DIAGONAL_EDGE * action->getYScale()) // down left
				_scrollMouseY = scrollSpeed / 2;
			else if (posY > (_screenHeight - SCROLL_DIAGONAL_EDGE) * action->getYScale()) // up left
				_scrollMouseY = -scrollSpeed / 2;
			else
				_scrollMouseY = 0;
		}
		else if (posX > (_screenWidth - SCROLL_BORDER) * action->getXScale()) // right scroll
		{
			_scrollMouseX = -scrollSpeed;

			// if close to top or bottom, also scroll diagonally
			if (posY < SCROLL_DIAGONAL_EDGE * action->getYScale()) // down right
				_scrollMouseY = scrollSpeed / 2;
			else if (posY > (_screenHeight - SCROLL_DIAGONAL_EDGE) * action->getYScale()) // up right
				_scrollMouseY = -scrollSpeed / 2;
			else
				_scrollMouseY = 0;
		}
		else if (posX != 0)
			_scrollMouseX = 0;

		if (posY < SCROLL_BORDER * action->getYScale()) // up scroll
		{
			_scrollMouseY = scrollSpeed;

			// if close to left or right edge, also scroll diagonally
			if (posX < SCROLL_DIAGONAL_EDGE * action->getXScale()) // up left
			{
				_scrollMouseX = scrollSpeed;
				_scrollMouseY /= 2;
			}
			else if (posX > (_screenWidth - SCROLL_DIAGONAL_EDGE) * action->getXScale()) // up right
			{
				_scrollMouseX = -scrollSpeed;
				_scrollMouseY /= 2;
			}
		}
		else if (posY > (_screenHeight - SCROLL_BORDER) * action->getYScale()) // down scroll
		{
			_scrollMouseY = -scrollSpeed;

			// if close to left or right edge, also scroll diagonally
			if (posX < SCROLL_DIAGONAL_EDGE * action->getXScale()) // down left
			{
				_scrollMouseX = scrollSpeed;
				_scrollMouseY /= 2;
			}
			else if (posX > (_screenWidth - SCROLL_DIAGONAL_EDGE) * action->getXScale()) // down right
			{
				_scrollMouseX = -scrollSpeed;
				_scrollMouseY /= 2;
			}
		}
		else if (posY != 0
			&& _scrollMouseX == 0)
		{
			_scrollMouseY = 0;
		}

		if ((_scrollMouseX != 0 || _scrollMouseY != 0)
			&& _scrollMouseTimer->isRunning() == false
			&& _scrollKeyTimer->isRunning() == false
			&& (SDL_GetMouseState(NULL,NULL) & SDL_BUTTON(Options::battleDragScrollButton)) == 0)
		{
			_scrollMouseTimer->start();
		}
		else if (_scrollMouseTimer->isRunning() == true
			&& _scrollMouseX == 0
			&& _scrollMouseY == 0)
		{
			_scrollMouseTimer->stop();
		}
	}
}

/**
 * Handles camera keyboard shortcuts.
 * @param action - pointer to an Action
 * @param state - State that the action handlers belong to
 */
void Camera::keyboardPress(Action* action, State*)
{
	if (_map->getCursorType() != CT_NONE)
	{
/*		const int keyB = action->getDetails()->key.keysym.sym;
		if (keyB == Options::keyBattleLeft || keyB == SDLK_KP4)
			_scrollKeyX = scrollSpeed;
		else if (keyB == Options::keyBattleRight || keyB == SDLK_KP6)
			_scrollKeyX = -scrollSpeed;
		else if (keyB == Options::keyBattleUp || keyB == SDLK_KP8)
			_scrollKeyY = scrollSpeed;
		else if (keyB == Options::keyBattleDown || keyB == SDLK_KP2)
			_scrollKeyY = -scrollSpeed; */

		const int scrollSpeed = Options::battleScrollSpeed;
		switch (action->getDetails()->key.keysym.sym)
		{
			case SDLK_LEFT: // hardcoding these ... ->
			case SDLK_KP4:
				_scrollKeyX = scrollSpeed;
			break;

			case SDLK_RIGHT:
			case SDLK_KP6:
				_scrollKeyX = -scrollSpeed;
			break;

			case SDLK_UP:
			case SDLK_KP8:
				_scrollKeyY = scrollSpeed;
			break;

			case SDLK_DOWN:
			case SDLK_KP2:
				_scrollKeyY = -scrollSpeed;
			break;

			case SDLK_KP7:
				_scrollKeyX =
				_scrollKeyY = scrollSpeed;
			break;

			case SDLK_KP9:
				_scrollKeyX = -scrollSpeed;
				_scrollKeyY =  scrollSpeed;
			break;

			case SDLK_KP1:
				_scrollKeyX =  scrollSpeed;
				_scrollKeyY = -scrollSpeed;
			break;

			case SDLK_KP3:
				_scrollKeyX =
				_scrollKeyY = -scrollSpeed;
		}

		if ((_scrollKeyX != 0 || _scrollKeyY != 0)
			&& _scrollKeyTimer->isRunning() == false
			&& _scrollMouseTimer->isRunning() == false
			&& (SDL_GetMouseState(NULL,NULL) & SDL_BUTTON(Options::battleDragScrollButton)) == 0)
		{
			_scrollKeyTimer->start();
		}
		else if (_scrollKeyTimer->isRunning() == true
			&& _scrollKeyX == 0
			&& _scrollKeyY == 0)
		{
			_scrollKeyTimer->stop();
		}
	}
}

/**
 * Handles camera keyboard shortcuts.
 * @param action - pointer to an Action
 * @param state - State that the action handlers belong to
 */
void Camera::keyboardRelease(Action* action, State*)
{
	if (_map->getCursorType() != CT_NONE)
	{
/*		const int keyB = action->getDetails()->key.keysym.sym;
		if (keyB == Options::keyBattleLeft || keyB == SDLK_KP4)
			_scrollKeyX = 0;
		else if (keyB == Options::keyBattleRight || keyB == SDLK_KP6)
			_scrollKeyX = 0;
		else if (keyB == Options::keyBattleUp || keyB == SDLK_KP8)
			_scrollKeyY = 0;
		else if (keyB == Options::keyBattleDown || keyB == SDLK_KP2)
			_scrollKeyY = 0; */

		switch (action->getDetails()->key.keysym.sym)
		{
			case SDLK_LEFT: // hardcoding these ... ->
			case SDLK_KP4:
			case SDLK_RIGHT:
			case SDLK_KP6:
				_scrollKeyX = 0;
			break;

			case SDLK_UP:
			case SDLK_KP8:
			case SDLK_DOWN:
			case SDLK_KP2:
				_scrollKeyY = 0;
			break;

			default:
				_scrollKeyX =
				_scrollKeyY = 0;
		}

		if ((_scrollKeyX != 0 || _scrollKeyY != 0)
			&& _scrollKeyTimer->isRunning() == false
			&& _scrollMouseTimer->isRunning() == false
			&& (SDL_GetMouseState(NULL,NULL) & SDL_BUTTON(Options::battleDragScrollButton)) == 0)
		{
			_scrollKeyTimer->start();
		}
		else if (_scrollKeyTimer->isRunning() == true
			&& _scrollKeyX == 0
			&& _scrollKeyY == 0)
		{
			_scrollKeyTimer->stop();
		}
	}
}

/**
 * Handles mouse-scrolling.
 */
void Camera::scrollMouse()
{
	scrollXY(
			_scrollMouseX,
			_scrollMouseY,
			true);
}

/**
 * Handles keyboard-scrolling.
 */
void Camera::scrollKey()
{
	scrollXY(
			_scrollKeyX,
			_scrollKeyY,
			true);
}

/**
 * Handles scrolling with given deviation.
 * @param x			- X deviation
 * @param y			- Y deviation
 * @param redraw	- true to redraw map
 */
void Camera::scrollXY(
		int x,
		int y,
		bool redraw)
{
	_mapOffset.x += x;
	_mapOffset.y += y;

	bool stop = false;
	do
	{
		convertScreenToMap(
					_screenWidth / 2,
					_playableHeight / 2,
					&_center.x,
					&_center.y);
		// Handling map bounds...
		// Ok, this is a prototype, it should be optimized.
		// Actually this should be calculated instead of slow-approximation.
		if (_center.x < 0)
		{
			_mapOffset.x -= 1;
			_mapOffset.y -= 1;
			continue;
		}

		if (_center.x > _mapsize_x - 1)
		{
			_mapOffset.x += 1;
			_mapOffset.y += 1;
			continue;
		}

		if (_center.y < 0)
		{
			_mapOffset.x += 1;
			_mapOffset.y -= 1;
			continue;
		}

		if (_center.y > _mapsize_y - 1)
		{
			_mapOffset.x -= 1;
			_mapOffset.y += 1;
			continue;
		}

		stop = true;
	}
	while (stop == false);

	_map->refreshSelectorPosition();

	if (redraw == true)
		_map->draw(); // kL, old code.
//		_map->invalidate();
}

/**
 * Handles jumping with given deviation.
 * @param x - X deviation
 * @param y - Y deviation
 */
void Camera::jumpXY(
		int x,
		int y)
{
	_mapOffset.x += x;
	_mapOffset.y += y;

	convertScreenToMap(
				_screenWidth / 2,
				_playableHeight / 2,
				&_center.x,
				&_center.y);
}

/**
 * Goes one level up.
 */
void Camera::up()
{
	if (_mapOffset.z < _mapsize_z - 1)
	{
		_map->getBattleSave()->getBattleState()->setLayerValue(++_mapOffset.z);
		_mapOffset.y += (_spriteHeight / 2) + 4;
		_map->draw();
	}
}

/**
 * Goes one level down.
 */
void Camera::down()
{
	if (_mapOffset.z > 0)
	{
		_map->getBattleSave()->getBattleState()->setLayerValue(--_mapOffset.z);
		_mapOffset.y -= (_spriteHeight / 2) + 4;
		_map->draw();
	}
}

/**
 * Gets the view level.
 * @return, the displayed level
 */
int Camera::getViewLevel() const
{
	return _mapOffset.z;
}

/**
 * Sets the view level.
 * @param viewLevel - new view level
 */
void Camera::setViewLevel(int viewLevel)	// The call from Map::drawTerrain() causes a stack overflow loop when projectile in FoV.
{											// Solution: remove draw() call below_
	_mapOffset.z = viewLevel;				// - might have to pass in a 'redraw' bool to compensate for other calls ...
	intMinMax(
			&_mapOffset.z,
			0,
			_mapsize_z - 1);

	_map->getBattleSave()->getBattleState()->setLayerValue(_mapOffset.z);
//	_map->draw();
}

/**
 * Centers map on a certain position.
 * @param posMap - reference the Position to center on
 * @param redraw - true to redraw map (default true)
 */
void Camera::centerOnPosition(
		const Position& posMap,
		bool redraw)
{
	_center = posMap;

	intMinMax(
			&_center.x,
			-1,
			_mapsize_x);
	intMinMax(
			&_center.y,
			-1,
			_mapsize_y);

	Position posScreen;
	convertMapToScreen(
					_center,
					&posScreen);

	_mapOffset.x = -(posScreen.x - (_screenWidth / 2) + 16);
	_mapOffset.y = -(posScreen.y - (_playableHeight / 2));
	_mapOffset.z = _center.z;

	_map->getBattleSave()->getBattleState()->setLayerValue(_mapOffset.z);

	if (redraw == true)
		_map->draw();
}

/**
 * Gets map's center position.
 * @return, center Position
 */
Position Camera::getCenterPosition()
{
	_center.z = _mapOffset.z;
	return _center;
}

/**
 * Converts screen coordinates XY to map coordinates XYZ.
 * @param screenX	- screen x position
 * @param screenY	- screen y position
 * @param mapX		- pointer to the map x position
 * @param mapY		- pointer to the map y position
 */
void Camera::convertScreenToMap(
		int screenX,
		int screenY,
		int* mapX,
		int* mapY) const
{
	// add half a tileheight to the mouseposition per layer above the floor
	screenY += (-_spriteWidth / 2) + (_mapOffset.z) * ((_spriteHeight + _spriteWidth / 4) / 2);

	// calculate the actual x/y pixelposition on a diamond shaped map
	// taking the view offset into account
	*mapY = -screenX + _mapOffset.x + 2 * screenY - 2 * _mapOffset.y;
	*mapX =  screenY - _mapOffset.y - (*mapY) / 4 - (_spriteWidth / 4);

	// to get the row&col itself, divide by the size of a tile
	*mapX /= (_spriteWidth / 4);
	*mapY /= _spriteWidth;

	intMinMax(
			mapX,
			-1,
			_mapsize_x);
	intMinMax(
			mapY,
			-1,
			_mapsize_y);
}

/**
 * Converts map coordinates XYZ to screen positions XY.
 * @param posMap	- reference the XYZ coordinates on the map (tilespace)
 * @param posScreen	- pointer to the screen Position pixel (upper left corner of sprite-rectangle)
 */
void Camera::convertMapToScreen(
		const Position& posMap,
		Position* const posScreen) const
{
	posScreen->x = posMap.x * (_spriteWidth / 2) - posMap.y * (_spriteWidth / 2);
	posScreen->y = posMap.x * (_spriteWidth / 4) + posMap.y * (_spriteWidth / 4) - posMap.z * ((_spriteHeight + _spriteWidth / 4) / 2);
	posScreen->z = 0; // not used
}

/**
 * Converts voxel coordinates XYZ to screen positions XY.
 * @param posVoxel	- reference the XYZ coordinates of the voxel
 * @param posScreen	- pointer to the screen Position
 */
void Camera::convertVoxelToScreen(
		const Position& posVoxel,
		Position* const posScreen) const
{
	const Position mapPosition = Position(
										posVoxel.x >> 4, // yeh i know: just say no.
										posVoxel.y >> 4,
										posVoxel.z / 24);
	convertMapToScreen(
					mapPosition,
					posScreen);

	const double
		dx = posVoxel.x - (mapPosition.x << 4),
		dy = posVoxel.y - (mapPosition.y << 4),
		dz = posVoxel.z - (mapPosition.z * 24);

	posScreen->x += static_cast<int>(dx - dy) + (_spriteWidth / 2);
	posScreen->y += static_cast<int>(((static_cast<double>(_spriteHeight) / 2.)) + (dx / 2.) + (dy / 2.) - dz);
	posScreen->x += _mapOffset.x;
	posScreen->y += _mapOffset.y;
}

/**
 * Gets the map size x.
 * @return, the map size x
 */
int Camera::getMapSizeX() const
{
	return _mapsize_x;
}

/**
 * Gets the map size y.
 * @return, the map size y
 */
int Camera::getMapSizeY() const
{
	return _mapsize_y;
}

/**
 * Gets the map offset.
 * @return, the map offset Position
 */
Position Camera::getMapOffset() const
{
	return _mapOffset;
}

/**
 * Sets the map offset.
 * @param pos - the map offset Position
 */
void Camera::setMapOffset(const Position& pos)
{
	_mapOffset = pos;
}

/**
 * Toggles showing all map layers.
 * @return, true if all layers showed
 */
bool Camera::toggleShowLayers()
{
	return (_showLayers = !_showLayers);
}

/**
 * Checks if the camera is showing all map layers.
 * @return, true if all layers are currently showing
 */
bool Camera::getShowLayers() const
{
	return _showLayers;
}

/**
 * Checks if map coordinates XYZ are on screen.
 * @note This does not care about Map's Z-level; only whether @a posMap is on screen.
 * @param posMap - reference the coordinates to check
 * @return, true if the map coordinates are on screen
 */
bool Camera::isOnScreen(const Position& posMap) const
{
	Position posScreen;
	convertMapToScreen(
					posMap,			// tile Position
					&posScreen);	// pixel Position
	posScreen.x += _mapOffset.x;
	posScreen.y += _mapOffset.y;

	static const int border = 28; // buffer the edges a bit.

	return posScreen.x > 8 + border // -> try these
		&& posScreen.x < _screenWidth - 8 - _spriteWidth - border
		&& posScreen.y > -16 + border
		&& posScreen.y < _screenHeight - 80 - border; // <- icons.
}
/*
 * Checks if map coordinates X,Y,Z are on screen.
 * @param posMap Coordinates to check.
 * @param unitWalking True to offset coordinates for a unit walking.
 * @param unitSize size of unit (0 - single, 1 - 2x2, etc, used for walking only
 * @param boundary True if it's for caching calculation
 * @return True if the map coordinates are on screen.
 */
/* bool Camera::isOnScreen(const Position &posMap, const bool unitWalking, const int unitSize, const bool boundary) const
{
	Position posScreen;
	convertMapToScreen(posMap, &posScreen);
	int posx = _spriteWidth/2, posy = _spriteHeight - _spriteWidth/4;
	int sizex = _spriteWidth/2, sizey = _spriteHeight/2;
	if (unitSize > 0)
	{
		posy -= _spriteWidth /4;
		sizex = _spriteWidth*unitSize;
		sizey = _spriteWidth*unitSize/2;
	}
	posScreen.x += _mapOffset.x + posx;
	posScreen.y += _mapOffset.y + posy;
	if (unitWalking)
	{
//pretty hardcoded hack to handle overlapping by icons
//(they are always in the center at the bottom of the screen)
//Free positioned icons would require more complex workaround.
//__________
//|________|
//||      ||
//|| ____ ||
//||_|XX|_||
//|________|
//
		if (boundary) //to make sprite updates even being slightly outside of screen
		{
			sizex += _spriteWidth;
			sizey += _spriteWidth/2;
		}
		if ( posScreen.x < 0 - sizex
			|| posScreen.x >= _screenWidth + sizex
			|| posScreen.y < 0 - sizey
			|| posScreen.y >= _screenHeight + sizey ) return false; //totally outside
		int side = ( _screenWidth - _map->getIconWidth() ) / 2;
		if ( (posScreen.y < (_screenHeight - _map->getIconHeight()) + sizey) ) return true; //above icons
		if ( (side > 1) && ( (posScreen.x < side + sizex) || (posScreen.x >= (_screenWidth - side - sizex)) ) ) return true; //at sides (if there are any)
		return false;
	}
	else
	{
		return posScreen.x >= 0
			&& posScreen.x <= _screenWidth - 10
			&& posScreen.y >= 0
			&& posScreen.y <= _screenHeight - 10;
	}
} */

/**
 * Resizes the viewable window of the camera.
 */
void Camera::resize()
{
	_screenWidth = _map->getWidth();
	_screenHeight = _map->getHeight();

	_playableHeight = _map->getHeight() - _map->getIconHeight();
}

/**
 * Stops the mouse scrolling.
 */
void Camera::stopMouseScrolling()
{
	_scrollMouseTimer->stop();
}

/**
 * Sets whether to pause the camera a moment before reverting to the position
 * it originally had before following a shot per Map tracing.
 * @sa BattleAction::cameraPosition
 * @param pause - true to pause (default true)
 */
void Camera::setPauseAfterShot(bool pause)
{
	_pauseAfterShot = pause;
}

/**
 * Gets whether to pause the camera a moment before reverting to the position
 * it originally had before following a shot per Map tracing.
 * @sa BattleAction::cameraPosition
 * @return, true to pause
 */
bool Camera::getPauseAfterShot() const
{
	return _pauseAfterShot;
}

}
