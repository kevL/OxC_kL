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

#include "MiniMapView.h"

#include <cmath>
#include <sstream>

#include "../fmath.h"

#include "Camera.h"
#include "Map.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Options.h"
#include "../Engine/Screen.h"
#include "../Engine/SurfaceSet.h"

#include "../Interface/Cursor.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/Armor.h"

#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

const int CELL_WIDTH	= 4;
const int CELL_HEIGHT	= 4;
const int MAX_LEVEL		= 3;
const int MAX_FRAME		= 2;


/**
 * Initializes all the elements in the MiniMapView.
 * @param w, The MiniMapView width.
 * @param h, The MiniMapView height.
 * @param x, The MiniMapView x origin.
 * @param y, The MiniMapView y origin.
 * @param game, Pointer to the core game.
 * @param camera, The Battlescape camera.
 * @param battleGame, Pointer to the SavedBattleGame.
 */
MiniMapView::MiniMapView(
		int w,
		int h,
		int x,
		int y,
		Game* game,
		Camera* camera,
		SavedBattleGame* battleGame)
	:
		InteractiveSurface(
			w,
			h,
			x,
			y),
		_game(game),
		_camera(camera),
		_battleGame(battleGame),
		_frame(0),
		_isMouseScrolling(false),
		_isMouseScrolled(false),
		_xBeforeMouseScrolling(0),
		_yBeforeMouseScrolling(0),
		_mouseScrollX(0),
		_mouseScrollY(0),
		_totalMouseMoveX(0),
		_totalMouseMoveY(0),
		_mouseOverThreshold(false)
{
	_set = _game->getResourcePack()->getSurfaceSet("SCANG.DAT");
}

/**
 * Draws the minimap.
 */
void MiniMapView::draw()
{
	int
		_startX = _camera->getCenterPosition().x - (getWidth() / 2 / CELL_WIDTH),
		_startY = _camera->getCenterPosition().y - (getHeight() / 2 / CELL_HEIGHT);

	InteractiveSurface::draw();

	if (!_set)
		return;

	SDL_Rect current;
	current.x = 0;
	current.y = 0;
	current.w = getWidth();
	current.h = getHeight();
	drawRect(&current, 0);

	this->lock();
	for (int
			lvl = 0;
			lvl <= _camera->getCenterPosition().z;
			lvl++)
	{
		int py = _startY;

		for (int
				y = Surface::getY();
				y < getHeight() + Surface::getY();
				y += CELL_HEIGHT)
		{
			int px = _startX;

			for (int
					x = Surface::getX();
					x < getWidth() + Surface::getX();
					x += CELL_WIDTH)
			{
				MapData* data = 0;

				Tile* tile = 0;
//				Position p (px, py, lvl); // <- initialization. kL_note
				Position p = Position(px, py, lvl); // kL (supposedly not as efficient)
				tile = _battleGame->getTile(p);

				if (!tile)
				{
					px++;

					continue;
				}

				int tileShade = 16;
				if (tile->isDiscovered(2))
					tileShade = tile->getShade();

				for (int
						i = 0;
						i < 4;
						i++)
				{
					Surface* s = 0;

					data = tile->getMapData(i);
					if (data
						&& data->getMiniMapIndex())
					{
						s = _set->getFrame(data->getMiniMapIndex() + 35);
					}

					if (s)
						s->blitNShade(
									this,
									x,
									y,
									tileShade);
				}

				if (tile->getUnit()
					&& tile->getUnit()->getVisible()) // visible, alive units
				{
					int
						size	= tile->getUnit()->getArmor()->getSize(),
						frame	= tile->getUnit()->getMiniMapSpriteIndex()
								+ tile->getPosition().x - tile->getUnit()->getPosition().x
								+ ((tile->getPosition().y - tile->getUnit()->getPosition().y) * size)
								+ (_frame * size * size);

					Surface* s = _set->getFrame(frame);
					// kL_begin:
					if (tile->getUnit() == _battleGame->getSelectedUnit())
					{
						s->blitNShade(
									this,
									x,
									y,
									0,
									false,
									4);		// should be same as 36, pale green.
//									1);		// white -> these numbers are blockOffsets + 1block ( ie. block 0 = block 1)
//									17);	// white again. % > 1
//									35);	// red. % > 3
//									36);	// pale green % > 4
//									37);	// paler green % > 5
//									8);		// blue again. % > 8
//									25);	// pale blue % > 9
					}
					else // kL_end.
					{
						s->blitNShade(
									this,
									x,
									y,
									0);
					}
				}

				if (tile->isDiscovered(2)
					&& !tile->getInventory()->empty()) // at least one item on this tile
				{
					int frame = 9 + _frame;

					Surface* s = _set->getFrame(frame);
					s->blitNShade(
								this,
								x,
								y,
								0);
				}

				px++;
			}

			py++;
		}
	}
	this->unlock();


	// kL_note: looks like the crosshairs for the MiniMap
	Sint16 centerX = static_cast<Sint16>(getWidth() / 2);
	Sint16 centerY = static_cast<Sint16>(getHeight() / 2);
	Sint16 xOffset = static_cast<Sint16>(CELL_WIDTH / 2);
	Sint16 yOffset = static_cast<Sint16>(CELL_HEIGHT / 2);

	Uint8 color = static_cast<Uint8>(_frame) * 3 + 1;

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
 * @return New display level.
 */
int MiniMapView::up()
{
	_camera->setViewLevel(_camera->getViewLevel() + 1);
	_redraw = true;

	return _camera->getViewLevel();
}

/**
 * Decrements the displayed level.
 * @return New display level.
 */
int MiniMapView::down()
{
	_camera->setViewLevel(_camera->getViewLevel() - 1);
	_redraw = true;

	return _camera->getViewLevel();
}

/**
 * Handles mouse presses on the minimap. Enters mouse-moving mode when the drag-scroll button is used.
 * @param action Pointer to an action.
 * @param state State that the action handlers belong to.
 */
void MiniMapView::mousePress(Action* action, State* state)
{
	InteractiveSurface::mousePress(action, state);

	if (action->getDetails()->button.button == Options::battleDragScrollButton)
	{
		_isMouseScrolling = true;
		_isMouseScrolled = false;

		SDL_GetMouseState(
						&_xBeforeMouseScrolling,
						&_yBeforeMouseScrolling);

		_posBeforeDragScroll = _camera->getCenterPosition();

/*kL
		if (!Options::battleDragScrollInvert
			&& _cursorPosition.z == 0)
		{
			_cursorPosition.x = static_cast<int>(action->getDetails()->motion.x);
			_cursorPosition.y = static_cast<int>(action->getDetails()->motion.y);
			// the Z is irrelevant to our mouse position, but we can use it as a boolean to check if the position is set or not
			_cursorPosition.z = 1;
		} */

		_mouseScrollX = 0;
		_mouseScrollY = 0;
		_totalMouseMoveX = 0;
		_totalMouseMoveY = 0;

		_mouseOverThreshold = false;
		_mouseScrollingStartTime = SDL_GetTicks();
	}
}

/**
 * Handles mouse clicks on the minimap. Will change the camera center to the clicked point.
 * @param action Pointer to an action.
 * @param state State that the action handlers belong to.
 */
void MiniMapView::mouseClick(Action* action, State* state)
{
	InteractiveSurface::mouseClick(action, state);

	// The following is the workaround for a rare problem where sometimes
	// the mouse-release event is missed for any reason.
	// However if the SDL is also missed the release event, then it is to no avail :(
	// (this part handles the release if it is missed and now an other button is used)
	if (_isMouseScrolling)
	{
		if (action->getDetails()->button.button != Options::battleDragScrollButton
			&& (SDL_GetMouseState(0, 0) & SDL_BUTTON(Options::battleDragScrollButton)) == 0)
		{
			// so we missed again the mouse-release :(
			// Check if we have to revoke the scrolling, because it was too short in time, so it was a click
			if (!_mouseOverThreshold
				&& SDL_GetTicks() - _mouseScrollingStartTime <= static_cast<Uint32>(Options::dragScrollTimeTolerance))
			{
				_camera->centerOnPosition(_posBeforeDragScroll);
				_redraw = true;
			}

			_isMouseScrolled = _isMouseScrolling = false;
			stopScrolling(action); // newScroll
		}
	}

	if (_isMouseScrolling) // Drag-Scroll release: release mouse-scroll-mode
	{
		// While scrolling, other buttons are ineffective
		if (action->getDetails()->button.button == Options::battleDragScrollButton)
		{
			_isMouseScrolling = false;
			stopScrolling(action); // newScroll
		}
		else
			return;

		// Check if we have to revoke the scrolling, because it was too short in time, so it was a click
		if (!_mouseOverThreshold
			&& SDL_GetTicks() - _mouseScrollingStartTime <= static_cast<Uint32>(Options::dragScrollTimeTolerance))
		{
			_isMouseScrolled = false;
			stopScrolling(action); // newScroll

			_camera->centerOnPosition(_posBeforeDragScroll);
			_redraw = true;
		}

		if (_isMouseScrolled)
			return;
	}

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		int origX = static_cast<int>(action->getRelativeXMouse() / action->getXScale());
		int origY = static_cast<int>(action->getRelativeYMouse() / action->getYScale());

		// get offset (in cells) of the click relative to center of screen
		int xOff = (origX / CELL_WIDTH) - (getWidth() / 2 / CELL_WIDTH);
		int yOff = (origY / CELL_HEIGHT) - (getHeight() / 2 / CELL_HEIGHT);

		// center the camera on this new position
		int newX = _camera->getCenterPosition().x + xOff;
		int newY = _camera->getCenterPosition().y + yOff;
		_camera->centerOnPosition(Position(
										newX,
										newY,
										_camera->getViewLevel()));
		_redraw = true;
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)	// kL
		_game->popState(); // Closes the window on right-click.			// kL
}

/**
 * Handles moving over the minimap.
 * Will change the camera center when the mouse is moved in mouse-moving mode.
 * @param action, Pointer to an action.
 * @param state, State that the action handlers belong to.
 */
void MiniMapView::mouseOver(Action* action, State* state)
{
	InteractiveSurface::mouseOver(action, state);

	if (_isMouseScrolling
		&& action->getDetails()->type == SDL_MOUSEMOTION)
	{
		// The following is the workaround for a rare problem where sometimes
		// the mouse-release event is missed for any reason.
		// However if the SDL is also missed the release event, then it is to no avail :(
		// (checking: is the dragScroll-mouse-button still pressed?)
		if ((SDL_GetMouseState(0, 0) & SDL_BUTTON(Options::battleDragScrollButton)) == 0)
		{
			// so we missed again the mouse-release :(
			// Check if we have to revoke the scrolling, because it was too short in time, so it was a click
			if (!_mouseOverThreshold
				&& SDL_GetTicks() - _mouseScrollingStartTime <= static_cast<Uint32>(Options::dragScrollTimeTolerance))
			{
					_camera->centerOnPosition(_posBeforeDragScroll);
					_redraw = true;
			}

			_isMouseScrolled = _isMouseScrolling = false;
			stopScrolling(action); // newScroll

			return;
		}

		_isMouseScrolled = true;

/*kL
		// Set the mouse cursor back ( or not )
		SDL_EventState(
					SDL_MOUSEMOTION,
					SDL_IGNORE);
		SDL_WarpMouse(
					static_cast<Uint16>(_xBeforeMouseScrolling),
					static_cast<Uint16>(_yBeforeMouseScrolling));
		SDL_EventState(
					SDL_MOUSEMOTION,
					SDL_ENABLE); */


		_totalMouseMoveX += static_cast<int>(action->getDetails()->motion.xrel);
		_totalMouseMoveY += static_cast<int>(action->getDetails()->motion.yrel);

		if (!_mouseOverThreshold) // check threshold
			_mouseOverThreshold = std::abs(_totalMouseMoveX) > Options::dragScrollPixelTolerance
									|| std::abs(_totalMouseMoveY) > Options::dragScrollPixelTolerance;

		// Calculate the move
/*		int
			newX,
			newY;

		if (Options::battleDragScrollInvert)
		{
			_mouseScrollX += static_cast<int>(action->getDetails()->motion.xrel);
			_mouseScrollY += static_cast<int>(action->getDetails()->motion.yrel);
			newX = _posBeforeDragScroll.x + _mouseScrollX / 3;
			newY = _posBeforeDragScroll.y + _mouseScrollY / 3;

			// Keep the limits...
			if (newX < -1
				|| newX > _camera->getMapSizeX())
			{
				_mouseScrollX -= static_cast<int>(action->getDetails()->motion.xrel);
				newX = _posBeforeDragScroll.x + _mouseScrollX / 3;
			}

			if (newY < -1
				|| newY > _camera->getMapSizeY())
			{
				_mouseScrollY -= static_cast<int>(action->getDetails()->motion.yrel);
				newY = _posBeforeDragScroll.y + _mouseScrollY / 3;
			}
		}
		else
		{
			newX = _posBeforeDragScroll.x - static_cast<int>(
													static_cast<double>(_totalMouseMoveX) / action->getXScale() / 3.0);
			newY = _posBeforeDragScroll.y - static_cast<int>(
													static_cast<double>(_totalMouseMoveY) / action->getYScale() / 3.0);

			// Keep the limits...
			if (newX < -1)
				newX = -1;
			else if (newX > _camera->getMapSizeX())
				newX = _camera->getMapSizeX();

			if (newY < -1)
				newY = -1;
			else if (newY > _camera->getMapSizeY())
				newY = _camera->getMapSizeY();
		} */ // kL
		int
			newX,
			newY;
//			scrollX,
//			scrollY;

/*kL
		if (Options::battleDragScrollInvert)
		{
			scrollX = static_cast<int>(static_cast<double>(action->getDetails()->motion.xrel) / action->getXScale());
			scrollY = static_cast<int>(static_cast<double>(action->getDetails()->motion.yrel) / action->getYScale());
		}
		else
		{
			scrollX = static_cast<int>(static_cast<double>(-action->getDetails()->motion.xrel) / action->getXScale());
			scrollY = static_cast<int>(static_cast<double>(-action->getDetails()->motion.yrel) / action->getYScale());
		} */

//		_mouseScrollX += static_cast<int>(action->getDetails()->motion.xrel);
//		_mouseScrollY += static_cast<int>(action->getDetails()->motion.yrel);
//		newX = _posBeforeDragScroll.x + (_mouseScrollX / 4);
//		newY = _posBeforeDragScroll.y + (_mouseScrollY / 4);
		_mouseScrollX -= static_cast<int>(action->getDetails()->motion.xrel);
		_mouseScrollY -= static_cast<int>(action->getDetails()->motion.yrel);
		newX = _posBeforeDragScroll.x + (_mouseScrollX / 10);
		newY = _posBeforeDragScroll.y + (_mouseScrollY / 10);

/*kL
		// keep the limits...
		if (newX < -1
			|| _camera->getMapSizeX() < newX)
		{
			_mouseScrollX -= scrollX;
			newX = _posBeforeDragScroll.x + (_mouseScrollX / 4);
		}

		if (newY < -1
			|| _camera->getMapSizeY() < newY)
		{
			_mouseScrollY -= scrollY;
			newY = _posBeforeDragScroll.y + (_mouseScrollY / 4);
		} */

		_camera->centerOnPosition(Position( // scroll
										newX,
										newY,
										_camera->getViewLevel()));
		_redraw = true;

/*kL
		// We don't want to look the mouse-cursor jumping :)
		if (Options::battleDragScrollInvert)
		{
			action->getDetails()->motion.x = static_cast<Uint16>(_xBeforeMouseScrolling);
			action->getDetails()->motion.y = static_cast<Uint16>(_yBeforeMouseScrolling);
		}
		else
		{
			Position delta(
						-scrollX,
						-scrollY,
						0);

			_cursorPosition.x = static_cast<int>(Round(static_cast<double>
									(std::min(
											getX() + getWidth(),
											std::max(
													getX(),
													static_cast<int>(Round(static_cast<double>(_cursorPosition.x) / action->getXScale())) + delta.x)))
									* action->getXScale()));
				//std::min(Round(getWidth() * action->getXScale() + getX() * action->getXScale()), std::max(Round(getX() * action->getXScale()), _cursorPosition.x + Round(delta.x * action->getXScale())));
			_cursorPosition.y = static_cast<int>(Round(static_cast<double>
									(std::min(
											getY() + getHeight(),
											std::max(
													getY(),
													static_cast<int>(Round(static_cast<double>(_cursorPosition.y) / action->getYScale())) + delta.y)))
									* action->getYScale()));
				//std::min(Round(getHeight() * action->getYScale() + getY() * action->getYScale()), std::max(Round(getY() * action->getYScale()), _cursorPosition.y + Round(delta.y * action->getYScale())));
			action->getDetails()->motion.x = static_cast<Uint16>(_cursorPosition.x);
			action->getDetails()->motion.y = static_cast<Uint16>(_cursorPosition.y);
		} */
//new_End.

		_game->getCursor()->handle(action);
	}
}

/**
 * Handles moving into the minimap.
 * Stops the mouse-scrolling mode, if it's left on.
 * @param action Pointer to an action.
 * @param state State that the action handlers belong to.
 */
void MiniMapView::mouseIn(Action* action, State* state)
{
	InteractiveSurface::mouseIn(action, state);

	_isMouseScrolling = false;

	setButtonPressed(SDL_BUTTON_RIGHT, false);
}

/**
 *
 */
void MiniMapView::stopScrolling(Action* action)
{
/*kL
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

	// reset our "mouse position stored" flag
	_cursorPosition.z = 0; */
}

/**
 * Updates the minimap animation.
 */
void MiniMapView::animate()
{
	_frame++;

	if (_frame > MAX_FRAME)
		_frame = 0;

	_redraw = true;
}

}
