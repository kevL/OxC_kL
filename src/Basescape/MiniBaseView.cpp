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

#include "MiniBaseView.h"

//#include <cmath>

#include "../Engine/Action.h"
//#include "../Engine/Palette.h"
#include "../Engine/SurfaceSet.h"
#include "../Engine/Timer.h"

#include "../Geoscape/GeoscapeState.h"

#include "../Ruleset/RuleBaseFacility.h"
#include "../Ruleset/RuleCraft.h"

#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/Craft.h"


namespace OpenXcom
{

/**
 * Sets up a MiniBaseView with the specified size and position.
 * @param width		- width in pixels
 * @param height	- height in pixels
 * @param x			- X position in pixels (default 0)
 * @param y			- Y position in pixels (default 0)
 */
MiniBaseView::MiniBaseView(
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
		_texture(NULL),
		_base(0),
		_hoverBase(0),
		_blink(false)
{
	_timer = new Timer(250);
	_timer->onTimer((SurfaceHandler)& MiniBaseView::blink);
	_timer->start();
}

/**
 * dTor.
 */
MiniBaseView::~MiniBaseView()
{
	delete _timer;
}

/**
 * Changes the current list of bases to display.
 * @param bases - pointer to a vector of pointers to Base
 */
void MiniBaseView::setBases(std::vector<Base*>* bases)
{
	_bases = bases;
	_redraw = true;
}

/**
 * Changes the texture to use for drawing the various base elements.
 * @param texture - pointer to a SurfaceSet to use
 */
void MiniBaseView::setTexture(SurfaceSet* texture)
{
	_texture = texture;
}

/**
 * Returns the base the mouse cursor is currently over.
 * @return, ID of the base
 */
size_t MiniBaseView::getHoveredBase() const
{
	return _hoverBase;
}

/**
 * Changes the base that is currently selected on the mini base view.
 * @param base - ID of the base
 */
void MiniBaseView::setSelectedBase(size_t base)
{
	_base = base;

	kL_currentBase = base;

	_redraw = true;
}

/**
 * Draws the view of all the bases with facilities in varying colors.
 */
void MiniBaseView::draw()
{
	Surface::draw();

	Base* base = NULL;
	std::string stat;
	int
		x,
		y,
		color;

	for (size_t
			i = 0;
			i < MAX_BASES;
			++i)
	{
		if (i == _base) // Draw base squares
		{
			SDL_Rect rect;
			rect.x = static_cast<Sint16>(static_cast<int>(i) * (MINI_SIZE + 2));
			rect.y = 0;
			rect.w = static_cast<Uint16>(MINI_SIZE + 2);
			rect.h = static_cast<Uint16>(MINI_SIZE + 2);
			drawRect(&rect, 1);
		}

		_texture->getFrame(41)->setX(static_cast<int>(i) * (MINI_SIZE + 2));
		_texture->getFrame(41)->setY(0);
		_texture->getFrame(41)->blit(this);


		if (i < _bases->size()) // Draw facilities
		{
			base = _bases->at(i);

			lock();
			SDL_Rect rect;

			for (std::vector<BaseFacility*>::const_iterator
					fac = base->getFacilities()->begin();
					fac != base->getFacilities()->end();
					++fac)
			{
				Uint8 pal = 2;
				if ((*fac)->getBuildTime() == 0)
					pal = 3;

				rect.x = static_cast<Sint16>(static_cast<int>(i) * (MINI_SIZE + 2) + 2 + (*fac)->getX() * 2);
				rect.y = static_cast<Sint16>(2 + (*fac)->getY() * 2);
				rect.w = static_cast<Uint16>((*fac)->getRules()->getSize() * 2);
				rect.h = static_cast<Uint16>((*fac)->getRules()->getSize() * 2);
				drawRect(&rect, Palette::blockOffset(pal)+3);

				++rect.x;
				++rect.y;
				--rect.w;
				--rect.h;
				drawRect(&rect, Palette::blockOffset(pal)+5);

				--rect.x;
				--rect.y;
				drawRect(&rect, Palette::blockOffset(pal)+2);

				++rect.x;
				++rect.y;
				--rect.w;
				--rect.h;
				drawRect(&rect, Palette::blockOffset(pal)+3);

				--rect.x;
				--rect.y;
				setPixelColor(
							rect.x,
							rect.y,
							Palette::blockOffset(pal)+1);
			}
			unlock();


			// Dot Marks for various base-status indicators.
			x = i * (MINI_SIZE + 2);

			if (base->getTransfers()->empty() == false) // incoming Transfers
				setPixelColor(
							x + 2,
							21,
							Palette::blockOffset(4)+5); // lavender

			if (base->getCrafts()->empty() == false)
			{
				const RuleCraft* craftRule = NULL;
				y = 17;

				for (std::vector<Craft*>::const_iterator
						c = base->getCrafts()->begin();
						c != base->getCrafts()->end();
						++c)
				{
					if ((*c)->getWarning() != CW_NONE)
					{
						if ((*c)->getWarning() == CW_CANTREFUEL)
							color = Palette::blockOffset(6);	// yellow
						else if ((*c)->getWarning() == CW_CANTREARM)
							color = Palette::blockOffset(6)+2;	// orange

						setPixelColor( // Craft needs materiels
									x + 2,
									19,
									color);
					}

					stat = (*c)->getStatus();
					if (stat == "STR_READY")
						setPixelColor(
									x + 14,
									y,
									Palette::blockOffset(3)+2);


					craftRule = (*c)->getRules();

					color = Palette::blockOffset(9)+9;		// brown
					if (craftRule->getRefuelItem().empty() == false)
						color = Palette::blockOffset(9)+1;	// yellow

					setPixelColor(
								x + 12,
								y,
								color);

					if (craftRule->getWeapons() > 0
						&& craftRule->getWeapons() == (*c)->getNumWeapons())
					{
						setPixelColor(
									x + 10,
									y,
									Palette::blockOffset(8)+2);		// blue
					}

					if (craftRule->getSoldiers() > 0)
						setPixelColor(
									x + 8,
									y,
									Palette::blockOffset(10)+1);	// brown

					if (craftRule->getVehicles() > 0)
						setPixelColor(
									x + 6,
									y,
									Palette::blockOffset(4)+8);		// lavender

					y += 2;
				}
			}
		}
	}
}

/**
 * Selects the base the mouse is over.
 * @param action	- pointer to an Action
 * @param state		- state that the action handlers belong to
 */
void MiniBaseView::mouseOver(Action* action, State* state)
{
	_hoverBase = static_cast<size_t>(std::floor(
					action->getRelativeXMouse()) / (static_cast<double>(MINI_SIZE + 2) * action->getXScale()));

	if (_hoverBase < 0) _hoverBase = 0;
	if (_hoverBase > 8) _hoverBase = 8;

	InteractiveSurface::mouseOver(action, state);
}

/**
 * Handles the blink() timer.
 */
void MiniBaseView::think()
{
	_timer->think(NULL, this);
}

/**
 * Blinks the craft status indicators.
 */
void MiniBaseView::blink()
{
	_blink = !_blink;

	Base* base = NULL;
	std::string stat;
	int
		x,
		y,
		color;

	for (size_t
			i = 0;
			i != _bases->size(); // < MAX_BASES;
			++i)
	{
		base = _bases->at(i);

		x = i * (MINI_SIZE + 2);

		if (base->getScientists() > 0 // unused Scientists &/or Engineers
			|| base->getEngineers() > 0)
		{
			color = 0;
			if (_blink == true)
				color = Palette::blockOffset(2)+1; // red

			setPixelColor(
						x + 2,
						17,
						color);
		}

		if (base->getCrafts()->empty() == false)
		{
			const RuleCraft* craftRule = NULL;
			y = 17;

			for (std::vector<Craft*>::const_iterator
					craft = base->getCrafts()->begin();
					craft != base->getCrafts()->end();
					++craft)
			{
				stat = (*craft)->getStatus();
				if (stat != "STR_READY")
				{
					color = 0;

					if (_blink == true)
					{
						if (stat == "STR_OUT")
							color = Palette::blockOffset(3)+2;	// green
						else if (stat == "STR_REFUELLING")
							color = Palette::blockOffset(6);	// yellow
						else if (stat == "STR_REARMING")
							color = Palette::blockOffset(6)+2;	// orange
						else if (stat == "STR_REPAIRS")
							color = Palette::blockOffset(2)+5;	// red
					}

					setPixelColor(
								x + 14,
								y,
								color);
				}

				craftRule = (*craft)->getRules();
				if (craftRule->getWeapons() > 0 // craft needs Weapons mounted.
					&& craftRule->getWeapons() != (*craft)->getNumWeapons())
				{
					color = 0;								// transparent
					if (_blink == true)
						color = Palette::blockOffset(8)+2;	// blue

					setPixelColor(
								x + 10,
								y,
								color);
				}

				y += 2;
			}
		}
	}
}

}
