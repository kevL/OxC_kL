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
#include "../Engine/SurfaceSet.h"
#include "../Engine/Timer.h"

#include "../Interface/Text.h"

#include "../Ruleset/RuleBaseFacility.h"
#include "../Ruleset/RuleCraft.h"

#include "../Savegame/BaseFacility.h"
#include "../Savegame/Craft.h"
#include "../Savegame/ItemContainer.h"


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
			x,y),
		_base(NULL),
		_texture(NULL),
		_srfDog(NULL),
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
	for (size_t
			x = 0;
			x != Base::BASE_SIZE;
			++x)
	{
		for (size_t
				y = 0;
				y != Base::BASE_SIZE;
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
 * @note The different fonts need to be passed in advance since the text size can
 * change mid-text and the language affects how text is rendered.
 * @param big	- pointer to large-size font
 * @param small	- pointer to small-size font
 * @param lang	- pointer to current language
 */
void BaseView::initText(
		Font* big,
		Font* small,
		Language* lang)
{
	_big = big;
	_small = small;
	_lang = lang;
}

/**
 * Changes the current base to display and initializes the internal base grid.
 * @param base - pointer to Base to display
 */
void BaseView::setBase(Base* base)
{
	_base = base;
	_selFacility = NULL;

	for (size_t
			x = 0;
			x != Base::BASE_SIZE;
			++x)
	{
		for (size_t
				y = 0;
				y != Base::BASE_SIZE;
				++y)
		{
			_facilities[x][y] = NULL;
		}
	}

	size_t
		facX,
		facY,
		facSize;

	for (std::vector<BaseFacility*>::const_iterator
			i = _base->getFacilities()->begin();
			i != _base->getFacilities()->end();
			++i)
	{
		facX = static_cast<size_t>((*i)->getX());
		facY = static_cast<size_t>((*i)->getY());
		facSize = (*i)->getRules()->getSize();

		for (size_t
				y = facY;
				y != facY + facSize;
				++y)
		{
			for (size_t
					x = facX;
					x != facX + facSize;
					++x)
			{
				_facilities[x][y] = *i;
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
 * Changes the dog to use for drawing the at the base.
 * @param texture - pointer to Surface to use
 */
void BaseView::setDog(Surface* const dog)
{
	_srfDog = dog;
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
 * Prevents any mouseover bugs on dismantling base facilities before setBase has
 * had time to update the base.
 */
void BaseView::resetSelectedFacility()
{
	_facilities[static_cast<size_t>(_selFacility->getX())]
			   [static_cast<size_t>(_selFacility->getY())] = NULL;

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
 * If enabled the base view will highlight the selected facility.
 * @param facSize - X/Y dimension in pixels (0 to disable)
 */
void BaseView::setSelectable(size_t facSize)
{
	_selSize = facSize;
	if (_selSize != 0)
	{
		_selector = new Surface(
							facSize * GRID_SIZE,
							facSize * GRID_SIZE,
							_x,_y);
		_selector->setPalette(getPalette());

		SDL_Rect rect;
		rect.x =
		rect.y = 0;
		rect.w = static_cast<Uint16>(_selector->getWidth());
		rect.h = static_cast<Uint16>(_selector->getHeight());
		_selector->drawRect(&rect, _selectorColor);

		++rect.x;
		++rect.y;
		rect.w -= 2;
		rect.h -= 2;

		_selector->drawRect(&rect, 0);
		_selector->setVisible(false);
	}
	else
		delete _selector;
}

/**
 * Returns if a facility can be placed on the currently selected square.
 * @param facRule - pointer to RuleBaseFacility
 * @return, true if allowed
 */
bool BaseView::isPlaceable(const RuleBaseFacility* const facRule) const
{
	if (   _gridX < 0
		|| _gridY < 0
		|| _gridX > static_cast<int>(Base::BASE_SIZE) - 1
		|| _gridY > static_cast<int>(Base::BASE_SIZE) - 1)
	{
		return false;
	}

	const size_t
		gridX = static_cast<size_t>(_gridX),
		gridY = static_cast<size_t>(_gridY);

	if (_facilities[gridX][gridY] == NULL)
	{
		const bool allowQ = Options::allowBuildingQueue;
		const size_t facSize = facRule->getSize();

		for (size_t // check for a facility to connect to
				i = 0;
				i != facSize;
				++i)
		{
			if ((	   gridX != 0							// check left
					&& gridY + i < Base::BASE_SIZE
					&& _facilities[gridX - 1]
								  [gridY + i] != NULL
					&& (allowQ == true
						|| _facilities[gridX - 1]
									  [gridY + i]
								->getBuildTime() == 0))

				|| (   gridX + i < Base::BASE_SIZE			// check top
					&& gridY != 0
					&& _facilities[gridX + i]
								  [gridY - 1] != NULL
					&& (allowQ == true
						|| _facilities[gridX + i]
									  [gridY - 1]
								->getBuildTime() == 0))

				|| (   gridX + facSize < Base::BASE_SIZE	// check right
					&& gridY + i < Base::BASE_SIZE
					&& _facilities[gridX + facSize]
								  [gridY + i] != NULL
					&& (allowQ == true
						|| _facilities[gridX + facSize]
									  [gridY + i]
								->getBuildTime() == 0))

				|| (   gridX + i < Base::BASE_SIZE			// check bottom
					&& gridY + facSize < Base::BASE_SIZE
					&& _facilities[gridX + i]
								  [gridY + facSize] != NULL
					&& (allowQ == true
						|| _facilities[gridX + i]
									  [gridY + facSize]
								->getBuildTime() == 0)))
			{
				return true;
			}
		}
	}

	return false;
}

/**
 * Returns if the placed facility is placed in queue or not.
 * @param facRule - pointer to RuleBaseFacility
 * @return, true if queued
 */
bool BaseView::isQueuedBuilding(const RuleBaseFacility* const facRule) const
{
	const size_t
		gridX = static_cast<size_t>(_gridX),
		gridY = static_cast<size_t>(_gridY),
		facSize = facRule->getSize();

	for (size_t
			i = 0;
			i != facSize;
			++i)
	{
		if ((	   gridX != 0							// check left
				&& gridY + i < Base::BASE_SIZE
				&& _facilities[gridX - 1]
							  [gridY + i] != NULL
				&& _facilities[gridX - 1]
							  [gridY + i]
						->getBuildTime() == 0)

			|| (   gridX + i < Base::BASE_SIZE			// check top
				&& gridY != 0
				&& _facilities[gridX + i]
							  [gridY - 1] != NULL
				&& _facilities[gridX + i]
							  [gridY - 1]
						->getBuildTime() == 0)

			|| (   gridX + facSize < Base::BASE_SIZE	// check right
				&& gridY + i < Base::BASE_SIZE
				&& _facilities[gridX + facSize]
							  [gridY + i] != NULL
				&& _facilities[gridX + facSize]
							  [gridY + i]
						->getBuildTime() == 0)

			|| (   gridX + i < Base::BASE_SIZE			// check bottom
				&& gridY + facSize < Base::BASE_SIZE
				&& _facilities[gridX + i]
							  [gridY + facSize] != NULL
				&& _facilities[gridX + i]
							  [gridY + facSize]
						->getBuildTime() == 0))
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
	for (std::vector<BaseFacility*>::const_iterator
			i = _base->getFacilities()->begin();
			i != _base->getFacilities()->end();
			++i)
	{
		if ((*i)->getBuildTime() != 0)
		{
			// set all queued buildings to infinite.
			if ((*i)->getBuildTime() > (*i)->getRules()->getBuildTime())
				(*i)->setBuildTime(std::numeric_limits<int>::max());

			facilities.push_back(*i);
		}
	}

	while (facilities.empty() == false) // applying a simple Dijkstra Algorithm
	{
		std::vector<BaseFacility*>::const_iterator fac = facilities.begin();
		for (std::vector<BaseFacility*>::iterator
				i = facilities.begin();
				i != facilities.end();
				++i)
		{
			if ((*i)->getBuildTime() < (*fac)->getBuildTime())
				fac = i;
		}

		BaseFacility* const facility = *fac;
		facilities.erase(fac);

		const RuleBaseFacility* const facRule = facility->getRules();
		const size_t
			x = static_cast<size_t>(facility->getX()),
			y = static_cast<size_t>(facility->getY()),
			facSize = facRule->getSize();

		for (size_t
				i = 0;
				i != facSize;
				++i)
		{
			if (x != 0)
				updateNeighborFacilityBuildTime(
											facility,
											_facilities[x - 1]
													   [y + i]);

			if (y != 0)
				updateNeighborFacilityBuildTime(
											facility,
											_facilities[x + i]
													   [y - 1]);

			if (x + facSize < Base::BASE_SIZE)
				updateNeighborFacilityBuildTime(
											facility,
											_facilities[x + facSize]
													   [y + i]);

			if (y + facSize < Base::BASE_SIZE)
				updateNeighborFacilityBuildTime(
											facility,
											_facilities[x + i]
													   [y + facSize]);
		}
	}
}

/**
 * Updates the neighborFacility's build time.
 * @note This is for internal use only by reCalcQueuedBuildings().
 * @param facility - pointer to a BaseFacility
 * @param neighbor - pointer to a neighboring BaseFacility
 */
void BaseView::updateNeighborFacilityBuildTime( // private.
		const BaseFacility* const facility,
		BaseFacility* const neighbor)
{
	if (   facility != NULL
		&& neighbor != NULL)
	{
		const int
			facBuild = facility->getBuildTime(),
			borBuild = neighbor->getBuildTime(),
			borTotal = neighbor->getRules()->getBuildTime();

		if (borBuild > borTotal
			&& facBuild + borTotal < borBuild)
		{
			neighbor->setBuildTime(facBuild + borTotal);
		}
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

	if (_selSize != 0)
	{
		SDL_Rect rect;

		if (_blink == true)
		{
			rect.x =
			rect.y = 0;
			rect.w = static_cast<Uint16>(_selector->getWidth());
			rect.h = static_cast<Uint16>(_selector->getHeight());
			_selector->drawRect(&rect, _selectorColor);

			++rect.x;
			++rect.y;
			rect.w -= 2;
			rect.h -= 2;
			_selector->drawRect(&rect, 0);
		}
		else
		{
			rect.x =
			rect.y = 0;
			rect.w = static_cast<Uint16>(_selector->getWidth());
			rect.h = static_cast<Uint16>(_selector->getHeight());
			_selector->drawRect(&rect, 0);
		}
	}
}

/**
 * Draws the view of all the facilities in the base with connectors between
 * them and crafts currently based in hangars.
 * @note This does not draw large facilities that are under construction -
 * that is only the dotted building-outline is shown.
 */
void BaseView::draw()
{
	Surface::draw();

	// draw grid squares
	for (int
			x = 0;
			x != static_cast<int>(Base::BASE_SIZE);
			++x)
	{
		for (int
				y = 0;
				y != static_cast<int>(Base::BASE_SIZE);
				++y)
		{
			Surface* const srfDirt = _texture->getFrame(0);
			srfDirt->setX(x * GRID_SIZE);
			srfDirt->setY(y * GRID_SIZE);
			srfDirt->blit(this);
		}
	}

	// draw facility shape
	for (std::vector<BaseFacility*>::const_iterator
			i = _base->getFacilities()->begin();
			i != _base->getFacilities()->end();
			++i)
	{
		int
			j = 0,
			sprite;
		const int facSize = static_cast<int>((*i)->getRules()->getSize());

		for (int
				y = (*i)->getY();
				y != (*i)->getY() + facSize;
				++y)
		{
			for (int
					x = (*i)->getX();
					x != (*i)->getX() + facSize;
					++x)
			{
				sprite = (*i)->getRules()->getSpriteShape() + j++;
				if ((*i)->getBuildTime() != 0)
					sprite += std::max( // outline
									3,
									facSize * facSize);

				Surface* const srfFac = _texture->getFrame(sprite);
				srfFac->setX(x * GRID_SIZE);
				srfFac->setY(y * GRID_SIZE);
				srfFac->blit(this);
			}
		}
	}

	// draw connectors
	for (std::vector<BaseFacility*>::const_iterator
			i = _base->getFacilities()->begin();
			i != _base->getFacilities()->end();
			++i)
	{
		if ((*i)->getBuildTime() == 0)
		{
			Surface* srfTunnel;
			const size_t
				facX = static_cast<size_t>((*i)->getX()),
				facY = static_cast<size_t>((*i)->getY()),
				facSize = (*i)->getRules()->getSize(),
				x = facX + facSize, // facilities to the right
				y = facY + facSize; // facilities to the bottom

			if (x < Base::BASE_SIZE)
			{
				for (size_t
						y = facY;
						y != facY + facSize;
						++y)
				{
					if (   _facilities[x][y] != NULL
						&& _facilities[x][y]->getBuildTime() == 0)
					{
						srfTunnel = _texture->getFrame(7);
						srfTunnel->setX(static_cast<int>(x) * GRID_SIZE - GRID_SIZE / 2);
						srfTunnel->setY(static_cast<int>(y) * GRID_SIZE);
						srfTunnel->blit(this);
					}
				}
			}

			if (y < Base::BASE_SIZE)
			{
				for (size_t
						x = facX;
						x != facX + facSize;
						++x)
				{
					if (   _facilities[x][y] != NULL
						&& _facilities[x][y]->getBuildTime() == 0)
					{
						srfTunnel = _texture->getFrame(8);
						srfTunnel->setX(static_cast<int>(x) * GRID_SIZE);
						srfTunnel->setY(static_cast<int>(y) * GRID_SIZE - GRID_SIZE / 2);
						srfTunnel->blit(this);
					}
				}
			}
		}
	}

	// draw facility graphic
	for (std::vector<BaseFacility*>::const_iterator
			i = _base->getFacilities()->begin();
			i != _base->getFacilities()->end();
			++i)
	{
		(*i)->setCraft(NULL); // NULL these to prepare hangers for population by Crafts.

		const int facSize = static_cast<int>((*i)->getRules()->getSize());
		int j = 0;

		for (int
				y = (*i)->getY();
				y != (*i)->getY() + facSize;
				++y)
		{
			for (int
					x = (*i)->getX();
					x != (*i)->getX() + facSize;
					++x)
			{
				if (facSize == 1)
				{
					Surface* const srfFac = _texture->getFrame((*i)->getRules()->getSpriteFacility() + j);
					srfFac->setX(x * GRID_SIZE);
					srfFac->setY(y * GRID_SIZE);
					srfFac->blit(this);
				}

				++j;
			}
		}

		// draw time remaining
		if ((*i)->getBuildTime() != 0)
		{
			Text* const text = new Text(
									GRID_SIZE * facSize,
									16,
									0,0);

			text->setPalette(getPalette());
			text->initText(
						_big,
						_small,
						_lang);
			text->setX(((*i)->getX() * GRID_SIZE) - 1);
			text->setY(((*i)->getY() * GRID_SIZE) + (GRID_SIZE * facSize - 16) / 2);
			text->setBig();

			std::wostringstream woststr;
			woststr << (*i)->getBuildTime();
			text->setAlign(ALIGN_CENTER);
			text->setColor(_cellColor);
			text->setText(woststr.str());

			text->blit(this);

			delete text;
		}
	}

	// draw crafts left to right, top row to bottom.
	std::vector<Craft*>::const_iterator i = _base->getCrafts()->begin();
	BaseFacility* fac;
	bool hasDog = (_base->getItems()->getItemQty("STR_DOGE") != 0);
	std::vector<std::pair<int, int> > dogPosition;
	int
		posDog_x,
		posDog_y;

	for (size_t
			y = 0;
			y != Base::BASE_SIZE;
			++y)
	{
		for (size_t
				x = 0;
				x != Base::BASE_SIZE;
				++x)
		{
			fac = _facilities[x][y];
			if (fac != NULL)
			{
				if (i != _base->getCrafts()->end()
					&& fac->getBuildTime() == 0
					&& fac->getRules()->getCrafts() != 0
					&& fac->getCraft() == NULL)
				{
					if ((*i)->getStatus() != "STR_OUT")
					{
						const int facSize = static_cast<int>(fac->getRules()->getSize());

						Surface* const srfCraft = _texture->getFrame((*i)->getRules()->getSprite() + 33);
						srfCraft->setX(fac->getX() * GRID_SIZE + (facSize - 1) * GRID_SIZE / 2 + 2);
						srfCraft->setY(fac->getY() * GRID_SIZE + (facSize - 1) * GRID_SIZE / 2 - 4);
						srfCraft->blit(this);
					}

					fac->setCraft(*i);
					++i;
				}

				if (hasDog == true
					&& (fac->getCraft() == NULL
						|| fac->getCraft()->getStatus() == "STR_OUT"))
				{
					const int facSize = static_cast<int>(fac->getRules()->getSize());

					posDog_x = fac->getX() * GRID_SIZE + facSize * RNG::generate(2,11);
					posDog_y = fac->getY() * GRID_SIZE + facSize * RNG::generate(2,17);
					std::pair<int, int> posDog = std::make_pair(
															posDog_x,
															posDog_y);
					dogPosition.push_back(posDog);
				}
			}
		}
	}

	// draw dog
	if (dogPosition.empty() == false)
	{
		const size_t i = RNG::generate(
									0,
									dogPosition.size() - 1);
		_srfDog->setX(dogPosition[i].first);
		_srfDog->setY(dogPosition[i].second);
		_srfDog->blit(this);
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

	if (   _gridX > -1
		&& _gridX < static_cast<int>(Base::BASE_SIZE)
		&& _gridY > -1
		&& _gridY < static_cast<int>(Base::BASE_SIZE))
	{
		_selFacility = _facilities[static_cast<size_t>(_gridX)][static_cast<size_t>(_gridY)];
		if (_selSize > 0)
		{
			if (   static_cast<size_t>(_gridX) + _selSize < Base::BASE_SIZE + 1
				&& static_cast<size_t>(_gridY) + _selSize < Base::BASE_SIZE + 1)
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
 * Sets the primary color.
 * @param color - primary color
 */
void BaseView::setColor(Uint8 color)
{
	_cellColor = color;
}

/**
 * Sets the secondary color.
 * @param color - secondary color
 */
void BaseView::setSecondaryColor(Uint8 color)
{
	_selectorColor = color;
}

}
