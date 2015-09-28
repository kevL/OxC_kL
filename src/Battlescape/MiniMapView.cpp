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

#include "MiniMapView.h"

//#include <cmath>
//#include "../fmath.h"

#include "Camera.h"
#include "MiniMapState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
//#include "../Engine/Options.h"
//#include "../Engine/Screen.h"
#include "../Engine/SurfaceSet.h"

#include "../Interface/Cursor.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleArmor.h"

#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

const int
	CELL_WIDTH	= 4,
	CELL_HEIGHT	= 4,
	MAX_FRAME	= 2;


/**
 * Initializes all the elements in the MiniMapView.
 * @param w				- the width
 * @param h				- the height
 * @param x				- the x origin
 * @param y				- the y origin
 * @param game			- pointer to the core Game
 * @param camera		- pointer to the battlescape Camera
 * @param battleSave	- pointer to the SavedBattleGame
 */
MiniMapView::MiniMapView(
		int w,
		int h,
		int x,
		int y,
		const Game* const game,
		Camera* const camera,
		const SavedBattleGame* const battleSave)
	:
		InteractiveSurface(
			w,h,
			x,y),
		_game(game),
		_camera(camera),
		_battleSave(battleSave),
		_frame(0),
		_isMouseScrolling(false),
		_isMouseScrolled(false),
//		_xBeforeMouseScrolling(0),
//		_yBeforeMouseScrolling(0),
		_mouseScrollX(0),
		_mouseScrollY(0),
		_totalMouseMoveX(0),
		_totalMouseMoveY(0),
		_mouseOverThreshold(false)
{
	_set = game->getResourcePack()->getSurfaceSet("SCANG.DAT");
}

/**
 * Draws the minimap.
 */
void MiniMapView::draw()
{
	InteractiveSurface::draw();

	const int
		width = getWidth(),
		height = getHeight(),
		startX = _camera->getCenterPosition().x - (width / 2 / CELL_WIDTH),
		startY = _camera->getCenterPosition().y - (height / 2 / CELL_HEIGHT),
		camera_Z = _camera->getCenterPosition().z;

	drawRect(
		0,0,
		static_cast<Sint16>(width),
		static_cast<Sint16>(height),
		0);

	if (_set != NULL)
	{
		Position pos;
		Tile* tile;
		Surface* srf;
		const MapData* data;
		const BattleUnit* unit;
		const int parts = static_cast<int>(Tile::PARTS_TILE);

		this->lock();
		for (int
				lvl = 0;
				lvl <= camera_Z;
				++lvl)
		{
			int py = startY;

			for (int
					y = Surface::getY();
					y < height + Surface::getY();
					y += CELL_HEIGHT)
			{
				int px = startX;

				for (int
						x = Surface::getX();
						x < width + Surface::getX();
						x += CELL_WIDTH)
				{
					pos = Position(px, py, lvl);
					tile = _battleSave->getTile(pos);

					if (tile == NULL)
					{
						++px;
						continue;
					}


					int
						colorGroup,
						colorOffset;

					if (px == 0
						|| px == _battleSave->getMapSizeX() - 1
						|| py == 0
						|| py == _battleSave->getMapSizeY() - 1)
					{
						colorGroup = 1; // greyscale
						colorOffset = 5;
					}
					else if (tile->isDiscovered(2) == true)
					{
						colorGroup = 0;
						colorOffset = tile->getShade();
					}
					else
					{
						colorGroup = 0;
						colorOffset = 15; // paint it ... black !
					}

					if (colorGroup == 1							// is along the edge
						&& lvl == 0								// is ground level
						&& tile->getMapData(O_OBJECT) == NULL)	// but has no content-object
					{
						srf = _set->getFrame(377);
						srf->blitNShade(
									this,
									x,y,
									colorOffset,
									false,
									colorGroup);
					}
					else // draw tile parts
					{
						for (int
								part = 0;
								part != parts;
								++part)
						{
							data = tile->getMapData(static_cast<MapDataType>(part));
							if (data != NULL
								&& data->getMiniMapIndex() != 0)
							{
								srf = _set->getFrame(data->getMiniMapIndex() + 35);
								if (srf != NULL)
									srf->blitNShade(
												this,
												x,y,
												colorOffset,
												false,
												colorGroup);
								else
									Log(LOG_WARNING) << "MiniMapView::draw() no data for Tile["
													 << part << "] pos " << tile->getPosition()
													 << " frame = " << data->getMiniMapIndex() + 35;
							}
						}
					}

					unit = tile->getUnit();
					if (unit != NULL
						&& unit->getUnitVisible() == true) // alive visible units
					{
						const int
							unitSize = unit->getArmor()->getSize(),
							frame = unit->getMiniMapSpriteIndex()
								  + tile->getPosition().x - unit->getPosition().x
								  + (tile->getPosition().y - unit->getPosition().y) * unitSize
								  + _frame * unitSize * unitSize;

						srf = _set->getFrame(frame);

						if (unit == _battleSave->getSelectedUnit()) // selected unit
						{
							colorGroup = 4; // pale green
							colorOffset = 0;
						}
						else if (unit->getFaction() == FACTION_PLAYER // Mc'd aLien
							&& unit->getOriginalFaction() == FACTION_HOSTILE)
						{
							colorGroup = 11; // brown
							colorOffset = 1;
						}
						else if (unit->getFaction() == FACTION_HOSTILE // Mc'd xCom
							&& unit->getOriginalFaction() == FACTION_PLAYER)
						{
							colorGroup = 8; // steel blue
							colorOffset = 0;
						}
						else // alien.
						{
							colorGroup =
							colorOffset = 0;
						}

						srf->blitNShade(
									this,
									x,y,
									colorOffset,
									false,
									colorGroup);
					}

					if (tile->isDiscovered(2) == true
						&& tile->getInventory()->empty() == false) // at least one item on this tile
					{
						const int frame = 9 + _frame;

						srf = _set->getFrame(frame);
						srf->blitNShade( // draw white cross
									this,
									x,y,0);
					}

					++px;
				}

				++py;
			}
		}
		this->unlock();
	}
	else Log(LOG_INFO) << "ERROR: MiniMapView SCANG.DAT not available";


	// kL_note: looks like the crosshairs for the MiniMap
	const Sint16
		centerX = static_cast<Sint16>(width / 2) - 1,
		centerY = static_cast<Sint16>(height / 2) - 1,
		xOffset = static_cast<Sint16>(CELL_WIDTH / 2),
		yOffset = static_cast<Sint16>(CELL_HEIGHT / 2);

	const Uint8 color = static_cast<Uint8>(_frame) * 3 + 1;

	drawLine( // top left
			centerX - static_cast<Sint16>(CELL_WIDTH),
			centerY - static_cast<Sint16>(CELL_HEIGHT),
			centerX - xOffset,
			centerY - yOffset,
			color);
	drawLine( // top right
			centerX + xOffset,
			centerY - yOffset,
			centerX + static_cast<Sint16>(CELL_WIDTH),
			centerY - static_cast<Sint16>(CELL_HEIGHT),
			color);
	drawLine( // bottom left
			centerX - static_cast<Sint16>(CELL_WIDTH),
			centerY + static_cast<Sint16>(CELL_HEIGHT),
			centerX - xOffset,
			centerY + yOffset,
			color);
	drawLine( // bottom right
			centerX + static_cast<Sint16>(CELL_WIDTH),
			centerY + static_cast<Sint16>(CELL_HEIGHT),
			centerX + xOffset,
			centerY + yOffset,
			color);
}

/**
 * Increments the displayed level.
 * @return, new display level
 */
int MiniMapView::up()
{
	_camera->setViewLevel(_camera->getViewLevel() + 1);
	_redraw = true;

	return _camera->getViewLevel();
}

/**
 * Decrements the displayed level.
 * @return, new display level
 */
int MiniMapView::down()
{
	_camera->setViewLevel(_camera->getViewLevel() - 1);
	_redraw = true;

	return _camera->getViewLevel();
}

/**
 * Handles mouse presses on the minimap. Enters mouse-moving mode when the drag-scroll button is used.
 * @param action	- pointer to an Action
 * @param state		- State that the action handlers belong to
 */
void MiniMapView::mousePress(Action* action, State* state) // private.
{
	InteractiveSurface::mousePress(action, state);

	if (action->getDetails()->button.button == Options::battleDragScrollButton)
	{
		_isMouseScrolling = true;
		_isMouseScrolled = false;

//		SDL_GetMouseState(&_xBeforeMouseScrolling, &_yBeforeMouseScrolling);

		_posPreDragScroll = _camera->getCenterPosition();

/*		if (!Options::battleDragScrollInvert && _cursorPosition.z == 0)
		{
			_cursorPosition.x = static_cast<int>(action->getDetails()->motion.x);
			_cursorPosition.y = static_cast<int>(action->getDetails()->motion.y);
			// the Z is irrelevant to our mouse position, but we can use it as a boolean to check if the position is set or not
			_cursorPosition.z = 1;
		} */

		_mouseScrollX =
		_mouseScrollY =
		_totalMouseMoveX =
		_totalMouseMoveY = 0;

		_mouseOverThreshold = false;
		_mouseScrollStartTime = SDL_GetTicks();
	}
}

/**
 * Handles mouse clicks on the minimap. Will change the camera center to the clicked point.
 * @param action	- pointer to an Action
 * @param state		- state that the action handlers belong to
 */
void MiniMapView::mouseClick(Action* action, State* state) // private.
{
	InteractiveSurface::mouseClick(action, state);

	// The following is the workaround for a rare problem where sometimes
	// the mouse-release event is missed for any reason.
	// However if the SDL is also missed the release event, then it is to no avail :(
	// (this part handles the release if it is missed and now an other button is used)
	if (_isMouseScrolling == true)
	{
		if (action->getDetails()->button.button != Options::battleDragScrollButton
			&& (SDL_GetMouseState(0,0) & SDL_BUTTON(Options::battleDragScrollButton)) == 0)
		{
			// so we missed again the mouse-release :(
			// Check if we have to revoke the scrolling, because it was too short in time, so it was a click
			if (_mouseOverThreshold == false
				&& SDL_GetTicks() - _mouseScrollStartTime <= static_cast<Uint32>(Options::dragScrollTimeTolerance))
			{
				_camera->centerOnPosition(_posPreDragScroll);
				_redraw = true;
			}

			_isMouseScrolled = _isMouseScrolling = false;
//			stopScrolling(action); // newScroll
		}
	}

	if (_isMouseScrolling == true) // Drag-Scroll release: release mouse-scroll-mode
	{
		// While scrolling, other buttons are ineffective
		if (action->getDetails()->button.button == Options::battleDragScrollButton)
		{
			_isMouseScrolling = false;
//			stopScrolling(action); // newScroll
		}
		else
			return;

		// Check if we have to revoke the scrolling, because it was too short in time, so it was a click
		if (_mouseOverThreshold == false
			&& SDL_GetTicks() - _mouseScrollStartTime <= static_cast<Uint32>(Options::dragScrollTimeTolerance))
		{
			_isMouseScrolled = false;
//			stopScrolling(action); // newScroll

			_camera->centerOnPosition(_posPreDragScroll);
			_redraw = true;
		}

		if (_isMouseScrolled == true)
			return;
	}

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		const int
			origX = static_cast<int>(action->getRelativeXMouse() / action->getXScale()),
			origY = static_cast<int>(action->getRelativeYMouse() / action->getYScale()),
			// get offset (in cells) of the click relative to center of screen
			xOff = (origX / CELL_WIDTH)  - (getWidth()  / 2 / CELL_WIDTH),
			yOff = (origY / CELL_HEIGHT) - (getHeight() / 2 / CELL_HEIGHT),
			// center the camera on this new position
			newX = _camera->getCenterPosition().x + xOff,
			newY = _camera->getCenterPosition().y + yOff;

		_camera->centerOnPosition(Position(
										newX, newY,
										_camera->getViewLevel()));
		_redraw = true;
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		dynamic_cast<MiniMapState*>(state)->btnOkClick(action); // kL_note: Close the state.
//		_game->popState(); // Closes the window on right-click. // kL
}

/**
 * Handles moving over the minimap.
 * Will change the camera center when the mouse is moved in mouse-moving mode.
 * @param action	- pointer to an Action
 * @param state		- state that the action handlers belong to
 */
void MiniMapView::mouseOver(Action* action, State* state) // private.
{
	InteractiveSurface::mouseOver(action, state);

	if (_isMouseScrolling == true
		&& action->getDetails()->type == SDL_MOUSEMOTION)
	{
		// The following is the workaround for a rare problem where sometimes
		// the mouse-release event is missed for any reason.
		// However if the SDL is also missed the release event, then it is to no avail :(
		// (checking: is the dragScroll-mouse-button still pressed?)
		if ((SDL_GetMouseState(0,0) & SDL_BUTTON(Options::battleDragScrollButton)) == 0)
		{
			// so we missed again the mouse-release :(
			// Check if we have to revoke the scrolling, because it was too short in time, so it was a click
			if (_mouseOverThreshold == false
				&& SDL_GetTicks() - _mouseScrollStartTime <= static_cast<Uint32>(Options::dragScrollTimeTolerance))
			{
					_camera->centerOnPosition(_posPreDragScroll);
					_redraw = true;
			}

			_isMouseScrolled = _isMouseScrolling = false;
//			stopScrolling(action); // newScroll

			return;
		}

		_isMouseScrolled = true;

/*		// Set the mouse cursor back (or not)
		SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
		SDL_WarpMouse(static_cast<Uint16>(_xBeforeMouseScrolling), static_cast<Uint16>(_yBeforeMouseScrolling));
		SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE); */

		_totalMouseMoveX += static_cast<int>(action->getDetails()->motion.xrel);
		_totalMouseMoveY += static_cast<int>(action->getDetails()->motion.yrel);

		if (_mouseOverThreshold == false) // check threshold
			_mouseOverThreshold = std::abs(_totalMouseMoveX) > Options::dragScrollPixelTolerance
							   || std::abs(_totalMouseMoveY) > Options::dragScrollPixelTolerance;

		// Calculate the move. OLD CODE ->
/*		int newX, newY;
		if (Options::battleDragScrollInvert)
		{
			_mouseScrollX += static_cast<int>(action->getDetails()->motion.xrel);
			_mouseScrollY += static_cast<int>(action->getDetails()->motion.yrel);
			newX = _posPreDragScroll.x + _mouseScrollX / 3;
			newY = _posPreDragScroll.y + _mouseScrollY / 3;

			// Keep the limits...
			if (newX < -1 || newX > _camera->getMapSizeX())
			{
				_mouseScrollX -= static_cast<int>(action->getDetails()->motion.xrel);
				newX = _posPreDragScroll.x + _mouseScrollX / 3;
			}

			if (newY < -1 || newY > _camera->getMapSizeY())
			{
				_mouseScrollY -= static_cast<int>(action->getDetails()->motion.yrel);
				newY = _posPreDragScroll.y + _mouseScrollY / 3;
			}
		}
		else
		{
			newX = _posPreDragScroll.x - static_cast<int>(static_cast<double>(_totalMouseMoveX) / action->getXScale() / 3.);
			newY = _posPreDragScroll.y - static_cast<int>(static_cast<double>(_totalMouseMoveY) / action->getYScale() / 3.);

			// Keep the limits...
			if (newX < -1) newX = -1;
			else if (newX > _camera->getMapSizeX()) newX = _camera->getMapSizeX();
			if (newY < -1) newY = -1;
			else if (newY > _camera->getMapSizeY()) newY = _camera->getMapSizeY();
		}
		int newX, newY; scrollX, scrollY; */

/*		if (Options::battleDragScrollInvert)
		{
			scrollX = static_cast<int>(static_cast<double>(action->getDetails()->motion.xrel));
			scrollY = static_cast<int>(static_cast<double>(action->getDetails()->motion.yrel));
		}
		else
		{
			scrollX = static_cast<int>(static_cast<double>(-action->getDetails()->motion.xrel));
			scrollY = static_cast<int>(static_cast<double>(-action->getDetails()->motion.yrel));
		} */

//		_mouseScrollX += static_cast<int>(action->getDetails()->motion.xrel);
//		_mouseScrollY += static_cast<int>(action->getDetails()->motion.yrel);
//		newX = _posPreDragScroll.x + (_mouseScrollX / action->getXScale() / 4);
//		newY = _posPreDragScroll.y + (_mouseScrollY / action->getYScale() / 4);
		_mouseScrollX -= static_cast<int>(action->getDetails()->motion.xrel);
		_mouseScrollY -= static_cast<int>(action->getDetails()->motion.yrel);

		const int
			newX = _posPreDragScroll.x + (_mouseScrollX / 11),
			newY = _posPreDragScroll.y + (_mouseScrollY / 11);

/*		// keep the limits...
		if (newX < -1 || _camera->getMapSizeX() < newX)
		{
			_mouseScrollX -= scrollX;
			newX = _posPreDragScroll.x + (_mouseScrollX / 4);
		}
		if (newY < -1 || _camera->getMapSizeY() < newY)
		{
			_mouseScrollY -= scrollY;
			newY = _posPreDragScroll.y + (_mouseScrollY / 4);
		} */

		_camera->centerOnPosition(Position( // scroll
										newX, newY,
										_camera->getViewLevel()));
		_redraw = true;

/*		// We don't want to look the mouse-cursor jumping :)
		if (Options::battleDragScrollInvert)
		{
			action->getDetails()->motion.x = static_cast<Uint16>(_xBeforeMouseScrolling);
			action->getDetails()->motion.y = static_cast<Uint16>(_yBeforeMouseScrolling);
		}
		else
		{
			Position delta(-scrollX, -scrollY, 0);
			int barWidth = _game->getScreen()->getCursorLeftBlackBand();
			int barHeight = _game->getScreen()->getCursorTopBlackBand();
			int cursorX = _cursorPosition.x + delta.x;
			int cursorY =_cursorPosition.y + delta.y;
			_cursorPosition.x = std::min((int)Round((getX() + getWidth()) * action->getXScale()) + barWidth, std::max((int)Round(getX() * action->getXScale()) + barWidth, cursorX));
			_cursorPosition.y = std::min((int)Round((getY() + getHeight()) * action->getYScale()) + barHeight, std::max((int)Round(getY() * action->getYScale()) + barHeight, cursorY));
			action->getDetails()->motion.x = static_cast<Uint16>(_cursorPosition.x);
			action->getDetails()->motion.y = static_cast<Uint16>(_cursorPosition.y);
		} //new_End. */

		_game->getCursor()->handle(action);
	}
}

/**
 * Handles moving into the minimap.
 * Stops the mouse-scrolling mode if it's left on.
 * @param action	- pointer to an Action
 * @param state		- State that the action handlers belong to
 */
void MiniMapView::mouseIn(Action* action, State* state) // private.
{
	InteractiveSurface::mouseIn(action, state);

	_isMouseScrolling = false;
	setButtonPressed(SDL_BUTTON_RIGHT, false);
}

/*
 *
 *
void MiniMapView::stopScrolling(Action* action)
{
	if (!Options::battleDragScrollInvert)
	{
		SDL_WarpMouse(
				static_cast<Uint16>(_cursorPosition.x),
				static_cast<Uint16>(_cursorPosition.y));
		action->setMouseAction(
				static_cast<int>(static_cast<double>(_cursorPosition.x) / action->getXScale()),
				static_cast<int>(static_cast<double>(_cursorPosition.y) / action->getYScale()),
				_game->getScreen()->getSurface()->getX(),
				_game->getScreen()->getSurface()->getY());
	}
	_cursorPosition.z = 0; // reset the "mouse position stored" flag
} */

/**
 * Animates the minimap.
 */
void MiniMapView::animate()
{
	if (++_frame > MAX_FRAME)
		_frame = 0;

	_redraw = true;
}

}
