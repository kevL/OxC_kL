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

#include "BaseView.h"

//#include <cmath>
//#include <limits>
//#include <sstream>

#include "../Engine/Action.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
#include "../Engine/Timer.h"
#include "../Engine/SurfaceSet.h"

#include "../Interface/Text.h"

#include "../Ruleset/RuleBaseFacility.h"
#include "../Ruleset/RuleCraft.h"

#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/Craft.h"


namespace OpenXcom
{

/**
 * Sets up a base view with the specified size and position.
 * @param width		- width in pixels
 * @param height	- height in pixels
 * @param x			- X position in pixels (default 0)
 * @param y			- Y position in pixels (default 0)
 */
BaseView::BaseView(
		int width,
		int height,
		int x,
		int y)
	:
		InteractiveSurface(
			width,
			height,
			x,
			y),
		_base(NULL),
		_texture(NULL),
		_selFacility(NULL),
		_big(NULL),
		_small(NULL),
		_lang(NULL),
		_gridX(0),
		_gridY(0),
		_selSize(0),
		_selector(NULL),
		_blink(true)
{
	for (int
			x = 0;
			x < BASE_SIZE;
			++x)
	{
		for (int
				y = 0;
				y < BASE_SIZE;
				++y)
		{
			_facilities[x][y] = NULL;
		}
	}

	_timer = new Timer(125);
	_timer->onTimer((SurfaceHandler)& BaseView::blink);
	_timer->start();
}

/**
 * Deletes contents.
 */
BaseView::~BaseView()
{
	delete _selector;
	delete _timer;
}

/**
 * Changes the various resources needed for text rendering.
 * The different fonts need to be passed in advance since the text size can change mid-text.
 * text size can change mid-text, and the language affects how the text is rendered.
 * @param big	- pointer to large-size font
 * @param small	- pointer to small-size font
 * @param lang	- pointer to current language
 */
void BaseView::initText(
		Font* big,
		Font* small,
		Language* lang)
{
	_big	= big;
	_small	= small;
	_lang	= lang;
}

/**
 * Changes the current base to display and initializes the internal base grid.
 * @param base - pointer to Base to display
 */
void BaseView::setBase(Base* base)
{
	_base = base;
	_selFacility = NULL;

	for (int // clear grid
			x = 0;
			x < BASE_SIZE;
			++x)
	{
		for (int
				y = 0;
				y < BASE_SIZE;
				++y)
		{
			_facilities[x][y] = NULL;
		}
	}

	for (std::vector<BaseFacility*>::iterator // fill grid with base facilities
			fac = _base->getFacilities()->begin();
			fac != _base->getFacilities()->end();
			++fac)
	{
		for (int
				y = (*fac)->getY();
				y < (*fac)->getY() + (*fac)->getRules()->getSize();
				++y)
		{
			for (int
					x = (*fac)->getX();
					x < (*fac)->getX() + (*fac)->getRules()->getSize();
					++x)
			{
				_facilities[x][y] = *fac;
			}
		}
	}

	_redraw = true;
}

/**
 * Changes the texture to use for drawing the various base elements.
 * @param texture - pointer to SurfaceSet to use
 */
void BaseView::setTexture(SurfaceSet* texture)
{
	_texture = texture;
}

/**
 * Returns the facility the mouse is currently over.
 * @return, pointer to base facility (0 if none)
 */
BaseFacility* BaseView::getSelectedFacility() const
{
	return _selFacility;
}

/**
 * Prevents any mouseover bugs on dismantling base facilities
 * before setBase has had time to update the base.
 */
void BaseView::resetSelectedFacility()
{
	_facilities[_selFacility->getX()][_selFacility->getY()] = NULL;
	_selFacility = NULL;
}

/**
 * Returns the X position of the grid square the mouse is currently over.
 * @return, X position on the grid
 */
int BaseView::getGridX() const
{
	return _gridX;
}

/**
 * Returns the Y position of the grid square the mouse is currently over.
 * @return, Y position on the grid
 */
int BaseView::getGridY() const
{
	return _gridY;
}

/**
 * If enabled, the base view will respond to player input,
 * highlighting the selected facility.
 * @param facSize - facility length (0 to disable)
 */
void BaseView::setSelectable(int facSize)
{
	_selSize = facSize;
	if (_selSize > 0)
	{
		_selector = new Surface(
							facSize * GRID_SIZE,
							facSize * GRID_SIZE,
							_x,
							_y);
		_selector->setPalette(getPalette());

		SDL_Rect r;
		r.w = _selector->getWidth();
		r.h = _selector->getHeight();
		r.x = 0;
		r.y = 0;
		_selector->drawRect(&r, _selectorColor); //Palette::blockOffset(1));

		r.w -= 2;
		r.h -= 2;
		++r.x;
		++r.y;
		_selector->drawRect(&r, 0);

		_selector->setVisible(false);
	}
	else
		delete _selector;
}

/**
 * Returns if a certain facility can be successfully
 * placed on the currently selected square.
 * @param rule - pointer to facility type
 * @return, true if placeable
 */
bool BaseView::isPlaceable(RuleBaseFacility* rule) const
{
	for (int // check if square is occupied
			y = _gridY;
			y < _gridY + rule->getSize();
			++y)
	{
		for (int
				x = _gridX;
				x < _gridX + rule->getSize();
				++x)
		{
			if (x < 0
				|| x >= BASE_SIZE
				|| y < 0
				|| y >= BASE_SIZE)
			{
				return false;
			}

			if (_facilities[x][y] != NULL)
				return false;
		}
	}

	bool bq = Options::allowBuildingQueue;

	for (int // check for another facility to connect to
			i = 0;
			i < rule->getSize();
			++i)
	{
		if ((_gridX > 0
				&& _facilities[_gridX - 1][_gridY + i] != 0
				&& (bq
					|| _facilities[_gridX - 1][_gridY + i]->getBuildTime() == 0))
			|| (_gridY > 0
				&& _facilities[_gridX + i][_gridY - 1] != 0
				&& (bq
					|| _facilities[_gridX + i][_gridY - 1]->getBuildTime() == 0))
			|| (_gridX + rule->getSize() < BASE_SIZE
				&& _facilities[_gridX + rule->getSize()][_gridY + i] != 0
				&& (bq
					|| _facilities[_gridX + rule->getSize()][_gridY + i]->getBuildTime() == 0))
			|| (_gridY + rule->getSize() < BASE_SIZE
				&& _facilities[_gridX + i][_gridY + rule->getSize()] != 0
				&& (bq
					|| _facilities[_gridX + i][_gridY + rule->getSize()]->getBuildTime() == 0)))
		{
			return true;
		}
	}

	return false;
}

/**
 * Returns if the placed facility is placed in queue or not.
 * @param rule - pointer to facility type
 * @return, true if queued
 */
bool BaseView::isQueuedBuilding(RuleBaseFacility* rule) const
{
	for (int
			i = 0;
			i < rule->getSize();
			++i)
	{
		if ((_gridX > 0
				&& _facilities[_gridX - 1][_gridY + i] != 0
				&& _facilities[_gridX - 1][_gridY + i]->getBuildTime() == 0)
			|| (_gridY > 0
				&& _facilities[_gridX + i][_gridY - 1] != 0
				&& _facilities[_gridX + i][_gridY - 1]->getBuildTime() == 0)
			|| (_gridX + rule->getSize() < BASE_SIZE
				&& _facilities[_gridX + rule->getSize()][_gridY + i] != 0
				&& _facilities[_gridX + rule->getSize()][_gridY + i]->getBuildTime() == 0)
			|| (_gridY + rule->getSize() < BASE_SIZE
				&& _facilities[_gridX + i][_gridY + rule->getSize()] != 0
				&& _facilities[_gridX + i][_gridY + rule->getSize()]->getBuildTime() == 0))
		{
			return false;
		}
	}

	return true;
}

/**
 * ReCalculates the remaining build-time of all queued buildings.
 */
void BaseView::reCalcQueuedBuildings()
{
	setBase(_base);

	std::vector<BaseFacility*> facilities;
	for (std::vector<BaseFacility*>::iterator
			fac = _base->getFacilities()->begin();
			fac != _base->getFacilities()->end();
			++fac)
	{
		if ((*fac)->getBuildTime() > 0)
		{
			// set all queued buildings to infinite.
			if ((*fac)->getBuildTime() > (*fac)->getRules()->getBuildTime())
				(*fac)->setBuildTime(std::numeric_limits<int>::max());

			facilities.push_back(*fac);
		}
	}

	while (facilities.empty() == false) // applying a simple Dijkstra Algorithm
	{
		std::vector<BaseFacility*>::iterator facMin = facilities.begin();
		for (std::vector<BaseFacility*>::iterator
				fac = facilities.begin();
				fac != facilities.end();
				++fac)
		{
			if ((*fac)->getBuildTime() < (*facMin)->getBuildTime())
				facMin = fac;
		}

		BaseFacility* const facility = *facMin;
		facilities.erase(facMin);

		const RuleBaseFacility* const rule = facility->getRules();
		const int
			x = facility->getX(),
			y = facility->getY();

		for (int
				fac = 0;
				fac < rule->getSize();
				++fac)
		{
			if (x > 0)
				updateNeighborFacilityBuildTime(
											facility,
											_facilities[x - 1][y + fac]);

			if (y > 0)
				updateNeighborFacilityBuildTime(
											facility,
											_facilities[x + fac][y - 1]);

			if (x + rule->getSize() < BASE_SIZE)
				updateNeighborFacilityBuildTime(
											facility,
											_facilities[x + rule->getSize()][y + fac]);

			if (y + rule->getSize() < BASE_SIZE)
				updateNeighborFacilityBuildTime(
											facility,
											_facilities[x + fac][y + rule->getSize()]);
		}
	}
}

/**
 * Updates the neighborFacility's build time.
 * This is for internal use only (reCalcQueuedBuildings()).
 * @param facility - pointer to a base facility
 * @param neighbor - pointer to a neighboring base facility
 */
void BaseView::updateNeighborFacilityBuildTime(
		BaseFacility* facility,
		BaseFacility* neighbor)
{
	if (facility != NULL
		&& neighbor != NULL
		&& neighbor->getBuildTime() > neighbor->getRules()->getBuildTime()
		&& facility->getBuildTime() + neighbor->getRules()->getBuildTime() < neighbor->getBuildTime())
	{
		neighbor->setBuildTime(facility->getBuildTime() + neighbor->getRules()->getBuildTime());
	}
}

/**
 * Keeps the animation timers running.
 */
void BaseView::think()
{
	_timer->think(NULL, this);
}

/**
 * Makes the facility selector blink.
 */
void BaseView::blink()
{
	_blink = !_blink;

	if (_selSize > 0)
	{
		SDL_Rect r;

		if (_blink)
		{
			r.w = _selector->getWidth();
			r.h = _selector->getHeight();
			r.x = 0;
			r.y = 0;
			_selector->drawRect(&r, _selectorColor); //Palette::blockOffset(1));

			r.w -= 2;
			r.h -= 2;
			++r.x;
			++r.y;
			_selector->drawRect(&r, 0);
		}
		else
		{
			r.w = _selector->getWidth();
			r.h = _selector->getHeight();
			r.x = 0;
			r.y = 0;
			_selector->drawRect(&r, 0);
		}
	}
}

/**
 * Draws the view of all the facilities in the base,
 * connectors between them, and crafts based in hangars.
 */
void BaseView::draw()
{
	Surface::draw();

	for (int // draw grid squares
			x = 0;
			x < BASE_SIZE;
			++x)
	{
		for (int
				y = 0;
				y < BASE_SIZE;
				++y)
		{
			Surface* frame = _texture->getFrame(0);
			frame->setX(x * GRID_SIZE);
			frame->setY(y * GRID_SIZE);
			frame->blit(this);
		}
	}


	for (std::vector<BaseFacility*>::const_iterator // draw facility shape
			i = _base->getFacilities()->begin();
			i != _base->getFacilities()->end();
			++i)
	{
		int num = 0;

		for (int
				y = (*i)->getY();
				y < (*i)->getY() + (*i)->getRules()->getSize();
				++y)
		{
			for (int
					x = (*i)->getX();
					x < (*i)->getX() + (*i)->getRules()->getSize();
					++x)
			{
				Surface* frame;

				const int outline = std::max(
										(*i)->getRules()->getSize() * (*i)->getRules()->getSize(),
										3);
				if ((*i)->getBuildTime() == 0)
					frame = _texture->getFrame((*i)->getRules()->getSpriteShape() + num);
				else
					frame = _texture->getFrame((*i)->getRules()->getSpriteShape() + num + outline);

				frame->setX(x * GRID_SIZE);
				frame->setY(y * GRID_SIZE);
				frame->blit(this);

				++num;
			}
		}
	}

	for (std::vector<BaseFacility*>::const_iterator // draw connectors
			fac = _base->getFacilities()->begin();
			fac != _base->getFacilities()->end();
			++fac)
	{
		if ((*fac)->getBuildTime() == 0)
		{
			const int x = (*fac)->getX() + (*fac)->getRules()->getSize(); // facilities to the right
			if (x < BASE_SIZE)
			{
				for (int
						y = (*fac)->getY();
						y < (*fac)->getY() + (*fac)->getRules()->getSize();
						++y)
				{
					if (_facilities[x][y] != NULL
						&& _facilities[x][y]->getBuildTime() == 0)
					{
						Surface* const frame = _texture->getFrame(7);
						frame->setX(x * GRID_SIZE - GRID_SIZE / 2);
						frame->setY(y * GRID_SIZE);

						frame->blit(this);
					}
				}
			}

			const int y = (*fac)->getY() + (*fac)->getRules()->getSize(); // facilities to the bottom
			if (y < BASE_SIZE)
			{
				for (int
						x = (*fac)->getX();
						x < (*fac)->getX() + (*fac)->getRules()->getSize();
						++x)
				{
					if (_facilities[x][y] != NULL
						&& _facilities[x][y]->getBuildTime() == 0)
					{
						Surface* const frame = _texture->getFrame(8);
						frame->setX(x * GRID_SIZE);
						frame->setY(y * GRID_SIZE - GRID_SIZE / 2);

						frame->blit(this);
					}
				}
			}
		}
	}

	for (std::vector<BaseFacility*>::const_iterator // draw facility graphic
			fac = _base->getFacilities()->begin();
			fac != _base->getFacilities()->end();
			++fac)
	{
		(*fac)->setCraft(NULL); // NULL these to prepare hangers for population by Crafts.

		int num = 0;

		for (int
				y = (*fac)->getY();
				y < (*fac)->getY() + (*fac)->getRules()->getSize();
				++y)
		{
			for (int
					x = (*fac)->getX();
					x < (*fac)->getX() + (*fac)->getRules()->getSize();
					++x)
			{
				if ((*fac)->getRules()->getSize() == 1)
				{
					Surface* srfFac = _texture->getFrame((*fac)->getRules()->getSpriteFacility() + num);
					srfFac->setX(x * GRID_SIZE);
					srfFac->setY(y * GRID_SIZE);

					srfFac->blit(this);
				}

				++num;
			}
		}

		if ((*fac)->getBuildTime() > 0) // draw time remaining
		{
			Text* const text = new Text(
									GRID_SIZE * (*fac)->getRules()->getSize(),
									16,
									0,
									0);

			text->setPalette(getPalette());
			text->initText(
						_big,
						_small,
						_lang);
			text->setX(((*fac)->getX() * GRID_SIZE) - 1);
			text->setY((*fac)->getY() * GRID_SIZE + (GRID_SIZE * (*fac)->getRules()->getSize() - 16) / 2);
			text->setBig();

			std::wostringstream ss;
			ss << (*fac)->getBuildTime();
			text->setAlign(ALIGN_CENTER);
			text->setColor(_cellColor); //Palette::blockOffset(13)+5);
			text->setText(ss.str());

			text->blit(this);

			delete text;
		}
	}

/*	for (std::vector<BaseFacility*>::iterator // remove crafts from Facilities
			fac = _base->getFacilities()->begin();
			fac != _base->getFacilities()->end();
			++fac)
	{
		(*fac)->setCraft(NULL); // done above^
	} */

	std::vector<Craft*>::const_iterator craft = _base->getCrafts()->begin();
	for (int // draw crafts left to right, top row to bottom.
			y = 0;
			y < BASE_SIZE;
			++y)
	{
		for (int
				x = 0;
				x < BASE_SIZE;
				++x)
		{
			if (_facilities[x][y] != NULL)
			{
				BaseFacility* const fac = _facilities[x][y];
				if (fac->getBuildTime() == 0
					&& fac->getRules()->getCrafts() > 0
					&& fac->getCraft() == NULL)
				{
					if (craft != _base->getCrafts()->end())
					{
						if ((*craft)->getStatus() != "STR_OUT")
						{
							Surface* const srfCraft = _texture->getFrame((*craft)->getRules()->getSprite() + 33);
							srfCraft->setX(fac->getX() * GRID_SIZE + (fac->getRules()->getSize() - 1) * GRID_SIZE / 2 + 2);
							srfCraft->setY(fac->getY() * GRID_SIZE + (fac->getRules()->getSize() - 1) * GRID_SIZE / 2 - 4);

							srfCraft->blit(this);
						}

						fac->setCraft(*craft);

						++craft;
					}
				}
			}
		}
	}
}

/**
 * Blits the base view and selector.
 * @param surface - pointer to a Surface to blit onto
 */
void BaseView::blit(Surface* surface)
{
	Surface::blit(surface);

	if (_selector != NULL)
		_selector->blit(surface);
}

/**
 * Selects the facility the mouse is over.
 * @param action - pointer to an Action
 * @param state - state that the action handlers belong to
 */
void BaseView::mouseOver(Action* action, State* state)
{
	_gridX = static_cast<int>(
				std::floor(action->getRelativeXMouse() / (static_cast<double>(GRID_SIZE) * action->getXScale())));
	_gridY = static_cast<int>(
				std::floor(action->getRelativeYMouse() / (static_cast<double>(GRID_SIZE) * action->getYScale())));

	if (_gridX >= 0
		&& _gridX < BASE_SIZE
		&& _gridY >= 0
		&& _gridY < BASE_SIZE)
	{
		_selFacility = _facilities[_gridX][_gridY];
		if (_selSize > 0)
		{
			if (_gridX + _selSize - 1 < BASE_SIZE
				&& _gridY + _selSize - 1 < BASE_SIZE)
			{
				_selector->setX(_x + _gridX * GRID_SIZE);
				_selector->setY(_y + _gridY * GRID_SIZE);

				_selector->setVisible();
			}
			else
				_selector->setVisible(false);
		}
	}
	else
	{
		_selFacility = NULL;

		if (_selSize > 0)
			_selector->setVisible(false);
	}

	InteractiveSurface::mouseOver(action, state);
}

/**
 * Deselects the facility.
 * @param action - pointer to an Action
 * @param state - state that the action handlers belong to
 */
void BaseView::mouseOut(Action* action, State* state)
{
	_selFacility = NULL;

	if (_selSize > 0)
		_selector->setVisible(false);

	InteractiveSurface::mouseOut(action, state);
}

/**
 *
 */
void BaseView::setColor(Uint8 color)
{
	_cellColor = color;
}

/**
 *
 */
void BaseView::setSecondaryColor(Uint8 color)
{
	_selectorColor = color;
}

}
