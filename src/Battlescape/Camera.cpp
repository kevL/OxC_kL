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

#define _USE_MATH_DEFINES

#include "Camera.h"

#include <cmath>
#include <fstream>

#include "Map.h"

#include "../Engine/Action.h"
#include "../Engine/Options.h"
#include "../Engine/Timer.h"


namespace OpenXcom
{

/**
 * Sets up a camera.
 * @param spriteWidth, Width of map sprite.
 * @param spriteHeight, Height of map sprite.
 * @param mapsize_x, Current map size in X axis.
 * @param mapsize_y, Current map size in Y axis.
 * @param mapsize_z, Current map size in Z axis.
 * @param map, Pointer to map surface.
 * @param visibleMapHeight, Current height the view is at.
 */
Camera::Camera(
		int spriteWidth,
		int spriteHeight,
		int mapsize_x,
		int mapsize_y,
		int mapsize_z,
		Map* map,
		int visibleMapHeight)
	:
		_scrollMouseTimer(0),
		_scrollKeyTimer(0),
		_spriteWidth(spriteWidth),
		_spriteHeight(spriteHeight),
		_mapsize_x(mapsize_x),
		_mapsize_y(mapsize_y),
		_mapsize_z(mapsize_z),
		_screenWidth(map->getWidth()),
		_screenHeight(map->getHeight()),
		_mapOffset(-250, 250, 0),
		_center(),
		_scrollMouseX(0),
		_scrollMouseY(0),
		_scrollKeyX(0),
		_scrollKeyY(0),
		_scrollTrigger(false),
		_visibleMapHeight(visibleMapHeight),
		_showAllLayers(false),
		_map(map)
{
}

/**
 * Deletes the Camera.
 */
Camera::~Camera()
{
}

/**
 * Sets the camera's scrolling timer.
 * @param mouse Pointer to mouse timer.
 * @param key Pointer to key timer.
 */
void Camera::setScrollTimer(
		Timer* mouse,
		Timer* key)
{
	_scrollMouseTimer = mouse;
	_scrollKeyTimer = key;
}

/**
 * Sets the value to min if it is below min and to max if it is above max.
 * @param value, Pointer to the value.
 * @param minValue, The minimum value.
 * @param maxValue, The maximum value.
 */
void Camera::minMaxInt(
		int* value,
		const int minValue,
		const int maxValue) const
{
	if (*value < minValue)
		*value = minValue;
	else if (*value > maxValue)
		*value = maxValue;
}

/**
 * Handles camera mouse shortcuts.
 * @param action Pointer to an action.
 * @param state State that the action handlers belong to.
 */
void Camera::mousePress(Action* action, State*)
{
	if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
		down();
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
		up();
	else if (action->getDetails()->button.button == SDL_BUTTON_LEFT
		&& Options::battleEdgeScroll == SCROLL_TRIGGER)
	{
		_scrollTrigger = true;
		mouseOver(action, 0);
	}
}

/**
 * Handles camera mouse shortcuts.
 * @param action Pointer to an action.
 * @param state State that the action handlers belong to.
 */
void Camera::mouseRelease(Action* action, State*)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT
		&& Options::battleEdgeScroll == SCROLL_TRIGGER)
	{
		_scrollMouseX = 0;
		_scrollMouseY = 0;
		_scrollMouseTimer->stop();
		_scrollTrigger = false;

		int posX = action->getXMouse();
		int posY = action->getYMouse();
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
 * @param action Pointer to an action.
 * @param state State that the action handlers belong to.
 */
void Camera::mouseOver(Action* action, State*)
{
	if (_map->getCursorType() == CT_NONE) return;


	if (Options::battleEdgeScroll == SCROLL_AUTO
		|| _scrollTrigger)
	{
		int posX = action->getXMouse();
		int posY = action->getYMouse();
		int scrollSpeed = Options::battleScrollSpeed;

//kL		if (posX < (SCROLL_BORDER * action->getXScale()) && posX >= 0) // left scroll
		if (posX < SCROLL_BORDER * action->getXScale()) // kL
		{
			_scrollMouseX = scrollSpeed;

			// if close to top or bottom, also scroll diagonally
//kL			if (posY < (SCROLL_DIAGONAL_EDGE * action->getYScale()) && posY >= 0) // down left
			if (posY < SCROLL_DIAGONAL_EDGE * action->getYScale()) // kL
				_scrollMouseY = scrollSpeed / 2;
			else if (posY > (_screenHeight - SCROLL_DIAGONAL_EDGE) * action->getYScale()) // up left
				_scrollMouseY = -scrollSpeed / 2;
			else
				_scrollMouseY = 0; // kL
		}
		else if (posX > (_screenWidth - SCROLL_BORDER) * action->getXScale()) // right scroll
		{
			_scrollMouseX = -scrollSpeed;

			// if close to top or bottom, also scroll diagonally
//kL			if (posY <= (SCROLL_DIAGONAL_EDGE * action->getYScale()) && posY >= 0) // down right
			if (posY < SCROLL_DIAGONAL_EDGE * action->getYScale()) // kL
				_scrollMouseY = scrollSpeed / 2;
			else if (posY > (_screenHeight - SCROLL_DIAGONAL_EDGE) * action->getYScale()) // up right
				_scrollMouseY = -scrollSpeed / 2;
			else
				_scrollMouseY = 0; // kL
		}
		else if (posX)
			_scrollMouseX = 0;

//kL		if (posY < (SCROLL_BORDER * action->getYScale()) && posY >= 0) // up scroll
		if (posY < SCROLL_BORDER * action->getYScale()) // kL
		{
			_scrollMouseY = scrollSpeed;

			// if close to left or right edge, also scroll diagonally
//kL			if (posX < (SCROLL_DIAGONAL_EDGE * action->getXScale()) && posX > 0) // up left
			if (posX < SCROLL_DIAGONAL_EDGE * action->getXScale()) // kL
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
//kL			if (posX < (SCROLL_DIAGONAL_EDGE * action->getXScale()) && posX >= 0) // down left
			if (posX < SCROLL_DIAGONAL_EDGE * action->getXScale()) // kL
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
		else if (posY
			&& _scrollMouseX == 0)
		{
			_scrollMouseY = 0;
		}

		if ((_scrollMouseX
				|| _scrollMouseY)
			&& !_scrollMouseTimer->isRunning()
			&& !_scrollKeyTimer->isRunning()
			&& (SDL_GetMouseState(0, 0) & SDL_BUTTON(Options::battleDragScrollButton)) == 0)
		{
			_scrollMouseTimer->start();
		}
		else if (_scrollMouseTimer->isRunning()
			&& !_scrollMouseX
			&& !_scrollMouseY)
		{
			_scrollMouseTimer->stop();
		}
	}
}

/**
 * Handles camera keyboard shortcuts.
 * @param action Pointer to an action.
 * @param state State that the action handlers belong to.
 */
void Camera::keyboardPress(Action* action, State*)
{
	if (_map->getCursorType() == CT_NONE) return;

	int scrollSpeed = Options::battleScrollSpeed;

	int key = action->getDetails()->key.keysym.sym;
	if (key == Options::keyBattleLeft)
		_scrollKeyX = scrollSpeed;
	else if (key == Options::keyBattleRight)
		_scrollKeyX = -scrollSpeed;
	else if (key == Options::keyBattleUp)
		_scrollKeyY = scrollSpeed;
	else if (key == Options::keyBattleDown)
		_scrollKeyY = -scrollSpeed;

	if ((_scrollKeyX
			|| _scrollKeyY)
		&& !_scrollKeyTimer->isRunning()
		&& !_scrollMouseTimer->isRunning()
		&& (SDL_GetMouseState(0, 0) & SDL_BUTTON(Options::battleDragScrollButton)) == 0)
	{
		_scrollKeyTimer->start();
	}
	else if (_scrollKeyTimer->isRunning()
		&& !_scrollKeyX
		&& !_scrollKeyY)
	{
		_scrollKeyTimer->stop();
	}
}

/**
 * Handles camera keyboard shortcuts.
 * @param action Pointer to an action.
 * @param state State that the action handlers belong to.
 */
void Camera::keyboardRelease(Action* action, State*)
{
	if (_map->getCursorType() == CT_NONE) return;

	int key = action->getDetails()->key.keysym.sym;
	if (key == Options::keyBattleLeft)
		_scrollKeyX = 0;
	else if (key == Options::keyBattleRight)
		_scrollKeyX = 0;
	else if (key == Options::keyBattleUp)
		_scrollKeyY = 0;
	else if (key == Options::keyBattleDown)
		_scrollKeyY = 0;

	if ((_scrollKeyX
			|| _scrollKeyY)
		&& !_scrollKeyTimer->isRunning()
		&& !_scrollMouseTimer->isRunning()
		&& (SDL_GetMouseState(0, 0) & SDL_BUTTON(Options::battleDragScrollButton)) == 0)
	{
		_scrollKeyTimer->start();
	}
	else if (_scrollKeyTimer->isRunning()
		&& !_scrollKeyX
		&& !_scrollKeyY)
	{
		_scrollKeyTimer->stop();
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
 * @param x, X deviation.
 * @param y, Y deviation.
 * @param redraw, Redraw map or not.
 */
void Camera::scrollXY(
		int x,
		int y,
		bool redraw)
{
	_mapOffset.x += x;
	_mapOffset.y += y;

	do
	{
		convertScreenToMap(
					_screenWidth / 2,
					_visibleMapHeight / 2,
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

		break;
	}
	while (true);

	_map->refreshSelectorPosition();

	if (redraw)
//kL		_map->invalidate();
		_map->draw(); // kL, old code.
}


/**
 * Handles jumping with given deviation.
 * @param x, X deviation.
 * @param y, Y deviation.
 */
void Camera::jumpXY(
		int x,
		int y)
{
	_mapOffset.x += x;
	_mapOffset.y += y;

	convertScreenToMap(
				_screenWidth / 2,
				_visibleMapHeight / 2,
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
		_mapOffset.z++;
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
		_mapOffset.z--;
		_mapOffset.y -= (_spriteHeight / 2) + 4;

		_map->draw();
	}
}

/**
 * Gets the displayed level.
 * @return, The displayed layer.
 */
int Camera::getViewLevel() const
{
	return _mapOffset.z;
}

/**
 * Sets the view level.
 * @param viewLevel, New view level.
 */
void Camera::setViewLevel(int viewLevel)
{
	_mapOffset.z = viewLevel;
	minMaxInt(
			&_mapOffset.z,
			0,
			_mapsize_z - 1);

	_map->draw();
}


/**
 * Centers map on a certain position.
 * @param mapPos, Position to center on.
 * @param redraw, Redraw map or not.
 */
void Camera::centerOnPosition(
		const Position& mapPos,
		bool redraw)
{
	Position screenPos;

	_center = mapPos;

	minMaxInt(
			&_center.x,
			-1,
			_mapsize_x);
	minMaxInt(
			&_center.y,
			-1,
			_mapsize_y);

	convertMapToScreen(
					_center,
					&screenPos);

	_mapOffset.x = -(screenPos.x - (_screenWidth / 2) + 16);
	_mapOffset.y = -(screenPos.y - (_visibleMapHeight / 2) + 16);
	_mapOffset.z = _center.z;

	if (redraw)
		_map->draw();
}

/**
 * Gets map's center position.
 * @return, Map's center position.
 */
Position Camera::getCenterPosition()
{
	_center.z = _mapOffset.z;

	return _center;
}

/**
 * Converts screen coordinates to map coordinates.
 * @param screenX, Screen x position.
 * @param screenY, Screen y position.
 * @param mapX, Map x position.
 * @param mapY, Map y position.
 */
void Camera::convertScreenToMap(
		int screenX,
		int screenY,
		int* mapX,
		int* mapY) const
{
	// add half a tileheight to the mouseposition per layer we are above the floor
	screenY += (-_spriteWidth / 2) + (_mapOffset.z) * ((_spriteHeight + _spriteWidth / 4) / 2);

	// calculate the actual x/y pixelposition on a diamond shaped map
	// taking the view offset into account
	*mapY = - screenX + _mapOffset.x + 2 * screenY - 2 * _mapOffset.y;
	*mapX = screenY - _mapOffset.y - (*mapY) / 4 - (_spriteWidth / 4);

	// to get the row&col itself, divide by the size of a tile
	*mapX /= (_spriteWidth / 4);
	*mapY /= _spriteWidth;

	minMaxInt(
			mapX,
			-1,
			_mapsize_x);
	minMaxInt(
			mapY,
			-1,
			_mapsize_y);
}

/**
 * Converts map coordinates X,Y,Z to screen positions X, Y.
 * @param mapPos, X,Y,Z coordinates on the map.
 * @param screenPos, Screen position.
 */
void Camera::convertMapToScreen(
		const Position& mapPos,
		Position* screenPos) const
{
	screenPos->z = 0; // not used
	screenPos->x = mapPos.x * (_spriteWidth / 2) - mapPos.y * (_spriteWidth / 2);
	screenPos->y = mapPos.x * (_spriteWidth / 4) + mapPos.y * (_spriteWidth / 4) - mapPos.z * ((_spriteHeight + _spriteWidth / 4) / 2);
}

/**
 * Converts voxel coordinates X,Y,Z to screen positions X, Y.
 * @param voxelPos, X,Y,Z coordinates of the voxel.
 * @param screenPos, Screen position.
 */
void Camera::convertVoxelToScreen(
		const Position& voxelPos,
		Position* screenPos) const
{
	Position mapPosition = Position(
								voxelPos.x / 16,
								voxelPos.y / 16,
								voxelPos.z / 24);
	convertMapToScreen(
					mapPosition,
					screenPos);

	double
		dx = voxelPos.x - (mapPosition.x * 16),
		dy = voxelPos.y - (mapPosition.y * 16),
		dz = voxelPos.z - (mapPosition.z * 24);

	screenPos->x += static_cast<int>(dx - dy) + (_spriteWidth / 2);
	screenPos->y += static_cast<int>(((static_cast<double>(_spriteHeight) / 2.0)) + (dx / 2.0) + (dy / 2.0) - dz);
	screenPos->x += _mapOffset.x;
	screenPos->y += _mapOffset.y;
}

/**
 * Gets the map size x.
 * @return, The map size x.
 */
int Camera::getMapSizeX() const
{
	return _mapsize_x;
}

/**
 * Gets the map size y.
 * @return, The map size y.
 */
int Camera::getMapSizeY() const
{
	return _mapsize_y;
}

/**
 * Gets the map offset.
 * @return, The map offset.
 */
Position Camera::getMapOffset()
{
	return _mapOffset;
}

/**
 * Sets the map offset.
 * @param pos, The map offset.
 */
void Camera::setMapOffset(Position pos)
{
	_mapOffset = pos;
}

/**
 * Toggles showing all map layers.
 * @return, New layer setting.
 */
int Camera::toggleShowAllLayers()
{
	_showAllLayers = !_showAllLayers;

	return _showAllLayers? 2: 1;
}

/**
 * Checks if the camera is showing all map layers.
 * @return Current layer setting.
 */
bool Camera::getShowAllLayers() const
{
	return _showAllLayers;
}

/**
 * Checks if map coordinates X,Y,Z are on screen.
 * @param mapPos, Coordinates to check.
 * @return, True if the map coordinates are on screen.
 */
bool Camera::isOnScreen(const Position& mapPos) const
{
	Position screenPos;
	convertMapToScreen(
					mapPos,
					&screenPos);
	screenPos.x += _mapOffset.x;
	screenPos.y += _mapOffset.y;

/*kL	return screenPos.x >= -24
			&& screenPos.x <= _screenWidth + 24
			&& screenPos.y >= -32
			&& screenPos.y <= _screenHeight - 48; */ // kL:
/*	return screenPos.x > -20
			&& screenPos.x < _screenWidth + 20
			&& screenPos.y > -28
			&& screenPos.y < _screenHeight - 52; */
	return screenPos.x > 0
			&& screenPos.x < _screenWidth
			&& screenPos.y > 0
			&& screenPos.y < _screenHeight - 72; // <- icons.
}

/**
 * Resizes the viewable window of the camera.
 */
void Camera::resize()
{
	_screenWidth = _map->getWidth();
	_screenHeight = _map->getHeight();

	_visibleMapHeight = _map->getHeight() - Map::ICON_HEIGHT;
}

}
