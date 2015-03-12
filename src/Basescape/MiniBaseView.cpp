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
 * @param mode		- MiniBaseViewType (default MBV_STANDARD)
 */
MiniBaseView::MiniBaseView(
		int width,
		int height,
		int x,
		int y,
		MiniBaseViewType mode)
	:
		InteractiveSurface(
			width,
			height,
			x,
			y),
		_mode(mode),
		_texture(NULL),
		_baseID(0),
		_hoverBase(0),
		_blink(false),
		_green(0),
		_red(0)
{
	if (_mode == MBV_STANDARD)
	{
		_timer = new Timer(250);
		_timer->onTimer((SurfaceHandler)& MiniBaseView::blink);
		_timer->start();
	}
}

/**
 * dTor.
 */
MiniBaseView::~MiniBaseView()
{
	if (_mode == MBV_STANDARD)
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
	_baseID =
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
	int
		x,y;
	Uint8 color;

	for (size_t
			i = 0;
			i < MAX_BASES;
			++i)
	{
		if (i == _baseID) // Draw base squares
		{
			SDL_Rect rect;
			rect.x = static_cast<Sint16>(static_cast<int>(i) * (MINI_SIZE + 2));
			rect.y = 0;
			rect.w =
			rect.h = static_cast<Uint16>(MINI_SIZE + 2);
			drawRect(&rect, 1);
		}

		_texture->getFrame(41)->setX(static_cast<int>(i) * (MINI_SIZE + 2));
		_texture->getFrame(41)->setY(0);
		_texture->getFrame(41)->blit(this);


		if (i < _bases->size()) // Draw facilities
		{
			base = _bases->at(i);

			if (_mode == MBV_STANDARD
				|| (_mode == MBV_RESEARCH
					&& base->hasResearch() == true)
				|| (_mode == MBV_PRODUCTION
					&& base->hasProduction() == true))
			{
				lock();
				SDL_Rect rect;

				for (std::vector<BaseFacility*>::const_iterator
						fac = base->getFacilities()->begin();
						fac != base->getFacilities()->end();
						++fac)
				{
					if ((*fac)->getBuildTime() == 0)
						color = _green;
					else
						color = _red;

					rect.x = static_cast<Sint16>(static_cast<int>(i) * (MINI_SIZE + 2) + 2 + (*fac)->getX() * 2);
					rect.y = static_cast<Sint16>(2 + (*fac)->getY() * 2);
					rect.w =
					rect.h = static_cast<Uint16>((*fac)->getRules()->getSize() * 2);
					drawRect(&rect, color+3);

					++rect.x;
					++rect.y;
					--rect.w;
					--rect.h;
					drawRect(&rect, color+5);

					--rect.x;
					--rect.y;
					drawRect(&rect, color+2);

					++rect.x;
					++rect.y;
					--rect.w;
					--rect.h;
					drawRect(&rect, color+3);

					--rect.x;
					--rect.y;
					setPixelColor(
								rect.x,
								rect.y,
								color+1);
				}
				unlock();


				// Dot Marks for various base-status indicators.
				if (_mode == MBV_STANDARD)
				{
					x = static_cast<int>(i) * (MINI_SIZE + 2);

					if (base->getTransfers()->empty() == false) // incoming Transfers
						setPixelColor(
									x + 2,
									21,
									Palette::blockOffset(4)+5); // lavender

					if (base->getCrafts()->empty() == false)
					{
						const RuleCraft* craftRule;
						y = 17;

						for (std::vector<Craft*>::const_iterator
								craft = base->getCrafts()->begin();
								craft != base->getCrafts()->end();
								++craft)
						{
							if ((*craft)->getWarning() != CW_NONE)
							{
								if ((*craft)->getWarning() == CW_CANTREFUEL)
									color = Palette::blockOffset(6);	// yellow
								else if ((*craft)->getWarning() == CW_CANTREARM)
									color = Palette::blockOffset(6)+2;	// orange
								else if ((*craft)->getWarning() == CW_CANTREPAIR) // should not happen without a repair mechanic!
									color = Palette::blockOffset(2)+5;	// red

								setPixelColor( // Craft needs materiels
											x + 2,
											19,
											color);
							}

							if ((*craft)->getStatus() == "STR_READY")
								setPixelColor(
											x + 14,
											y,
											Palette::blockOffset(3)+2);


							craftRule = (*craft)->getRules();

							if (craftRule->getRefuelItem().empty() == false)
								color = Palette::blockOffset(9)+1;	// yellow
							else
								color = Palette::blockOffset(9)+9;	// brown

							setPixelColor(
										x + 12,
										y,
										color);

							if (craftRule->getWeapons() > 0
								&& craftRule->getWeapons() == (*craft)->getNumWeapons())
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
 *
 */
void MiniBaseView::setColor(Uint8 color)
{
	_green = color;
}

/**
 *
 */
void MiniBaseView::setSecondaryColor(Uint8 color)
{
	_red = color;
}

/**
 * Handles the blink() timer.
 */
void MiniBaseView::think()
{
	if (_mode == MBV_STANDARD)
		_timer->think(NULL, this);
}

/**
 * Blinks the craft status indicators.
 */
void MiniBaseView::blink()
{
	_blink = !_blink;

	Base* base;
	std::string stat;
	int
		x,
		y;
	Uint8 color;

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
			if (_blink == true)
				color = Palette::blockOffset(2)+1; // red
			else
				color = 0;

			setPixelColor(
						x + 2,
						17,
						color);
		}

		if (base->getCrafts()->empty() == false)
		{
			const RuleCraft* craftRule;
			y = 17;

			for (std::vector<Craft*>::const_iterator
					craft = base->getCrafts()->begin();
					craft != base->getCrafts()->end();
					++craft)
			{
				stat = (*craft)->getStatus();
				if (stat != "STR_READY")
				{
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
					else
						color = 0;

					setPixelColor(
								x + 14,
								y,
								color);
				}

				craftRule = (*craft)->getRules();
				if (craftRule->getWeapons() > 0 // craft needs Weapons mounted.
					&& craftRule->getWeapons() != (*craft)->getNumWeapons())
				{
					if (_blink == true)
						color = Palette::blockOffset(8)+2;	// blue
					else
						color = 0;

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
