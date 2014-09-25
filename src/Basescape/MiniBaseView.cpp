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

#include <cmath>

#include "../Engine/Action.h"
#include "../Engine/Palette.h"
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
 * Sets up a mini base view with the specified size and position.
 * @param width Width in pixels.
 * @param height Height in pixels.
 * @param x X position in pixels.
 * @param y Y position in pixels.
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
 * @param bases Pointer to base list to display.
 */
void MiniBaseView::setBases(std::vector<Base*>* bases)
{
	_bases = bases;
	_redraw = true;
}

/**
 * Changes the texture to use for drawing the various base elements.
 * @param texture Pointer to SurfaceSet to use.
 */
void MiniBaseView::setTexture(SurfaceSet* texture)
{
	_texture = texture;
}

/**
 * Returns the base the mouse cursor is currently over.
 * @return ID of the base.
 */
size_t MiniBaseView::getHoveredBase() const
{
	return _hoverBase;
}

/**
 * Changes the base that is currently selected on the mini base view.
 * @param base ID of base.
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

	for (size_t
			i = 0;
			i < MAX_BASES;
			++i)
	{
		if (i == _base) // Draw base squares
		{
			SDL_Rect r;
			r.x = static_cast<Sint16>(static_cast<int>(i) * (MINI_SIZE + 2));
			r.y = 0;
			r.w = static_cast<Uint16>(MINI_SIZE + 2);
			r.h = static_cast<Uint16>(MINI_SIZE + 2);
			drawRect(&r, 1);
		}

		_texture->getFrame(41)->setX(static_cast<int>(i) * (MINI_SIZE + 2));
		_texture->getFrame(41)->setY(0);
		_texture->getFrame(41)->blit(this);


		if (i < _bases->size()) // Draw facilities
		{
			base = _bases->at(i);

			lock();
			SDL_Rect r;

			for (std::vector<BaseFacility*>::iterator
					fac = base->getFacilities()->begin();
					fac != base->getFacilities()->end();
					++fac)
			{
				int pal;
				if ((*fac)->getBuildTime() == 0)
					pal = 3;
				else
					pal = 2;

				r.x = static_cast<Sint16>(static_cast<int>(i) * (MINI_SIZE + 2) + 2 + (*fac)->getX() * 2);
				r.y = static_cast<Sint16>(2 + (*fac)->getY() * 2);
				r.w = static_cast<Uint16>((*fac)->getRules()->getSize() * 2);
				r.h = static_cast<Uint16>((*fac)->getRules()->getSize() * 2);
				drawRect(&r, Palette::blockOffset(pal)+3);

				r.x++;
				r.y++;
				r.w--;
				r.h--;
				drawRect(&r, Palette::blockOffset(pal)+5);

				r.x--;
				r.y--;
				drawRect(&r, Palette::blockOffset(pal)+2);

				r.x++;
				r.y++;
				r.w--;
				r.h--;
				drawRect(&r, Palette::blockOffset(pal)+3);

				r.x--;
				r.y--;
				setPixelColor(r.x, r.y, Palette::blockOffset(pal)+1);
			}
			unlock();

			// kL_begin: Dot Marks for various base-status indicators.
			int offset_x = i * (MINI_SIZE + 2);

			if (base->getScientists() > 0 // red for unused Scientists & Engineers
				|| base->getEngineers() > 0)
			{
				setPixelColor(offset_x + 2, 17, Palette::blockOffset(2)+1); // red
			}

			if (base->getTransfers()->empty() == false) // white for incoming Transfers
				setPixelColor(offset_x + 2, 21, Palette::blockOffset(4)+5); // lavender

			if (base->getCrafts()->empty() == false)
			{
				int pixel_y = 17;
				RuleCraft* craftRule = NULL;

				for (std::vector<Craft*>::iterator
						craft = base->getCrafts()->begin();
						craft != base->getCrafts()->end();
						++craft)
				{
					craftRule = (*craft)->getRules();

					std::string stat = (*craft)->getStatus();
					if (stat == "STR_READY")
						setPixelColor(offset_x + 14, pixel_y, Palette::blockOffset(3)+2);

					if (craftRule->getRefuelItem() == "STR_ELERIUM_115")
						setPixelColor(offset_x + 12, pixel_y, Palette::blockOffset(9)+1); // yellow
					else
						setPixelColor(offset_x + 12, pixel_y, Palette::blockOffset(9)+9);

					if (craftRule->getWeapons() > 0
						&& craftRule->getWeapons() == (*craft)->getNumWeapons())
					{
						setPixelColor(offset_x + 10, pixel_y, Palette::blockOffset(8)+2); // blue
					}
//					else setPixelColor(offset_x + 12, pixel_y, Palette::blockOffset(8)+10);

					if (craftRule->getSoldiers() > 0)
						setPixelColor(offset_x + 8, pixel_y, Palette::blockOffset(10)+1); // brown
//					else setPixelColor(offset_x + 10, pixel_y, Palette::blockOffset(10)+10);

					if (craftRule->getVehicles() > 0)
						setPixelColor(offset_x + 6, pixel_y, Palette::blockOffset(4)+8); // lavender
//					else setPixelColor(offset_x + 8, pixel_y, Palette::blockOffset(4)+10);

					pixel_y += 2;
				}
			} // kL_end.
		}
	}
}

/**
 * Selects the base the mouse is over.
 * @param action, Pointer to an action.
 * @param state, State that the action handlers belong to.
 */
void MiniBaseView::mouseOver(Action* action, State* state)
{
	_hoverBase = static_cast<size_t>(
					floor(action->getRelativeXMouse()) / (static_cast<double>(MINI_SIZE + 2) * action->getXScale()));

	if (_hoverBase < 0) _hoverBase = 0; // kL
	if (_hoverBase > 8) _hoverBase = 8; // kL

	InteractiveSurface::mouseOver(action, state);
}

/**
 * kL. Handles the blink() timer.
 */
void MiniBaseView::think()
{
	_timer->think(NULL, this);
}

/**
 * kL. Blinks the craft status indicators.
 */
void MiniBaseView::blink() // kL
{
	//Log(LOG_INFO) << "MiniBaseView::blink() = " << (int)_blink;
	_blink = !_blink;

	Base* base = NULL;

	for (size_t
			i = 0;
			i < MAX_BASES;
			++i)
	{
		if (i < _bases->size())
		{
			base = _bases->at(i);

			int offset_x = i * (MINI_SIZE + 2);

			if (base->getCrafts()->empty() == false)
			{
				int pixel_y = 17;

				for (std::vector<Craft*>::iterator
						craft = base->getCrafts()->begin();
						craft != base->getCrafts()->end();
						++craft)
				{
					std::string stat = (*craft)->getStatus();

					if (stat != "STR_READY") // done in draw() above^; Craft is not status_Ready.
					{
						if (_blink)
						{
							if (stat == "STR_OUT")
								setPixelColor(offset_x + 14, pixel_y, Palette::blockOffset(3)+2); // green
							else if (stat == "STR_REFUELLING")
								setPixelColor(offset_x + 14, pixel_y, Palette::blockOffset(6)); // orange
							else if (stat == "STR_REARMING")
								setPixelColor(offset_x + 14, pixel_y, Palette::blockOffset(6)+2);
							else if (stat == "STR_REPAIRS")
								setPixelColor(offset_x + 14, pixel_y, Palette::blockOffset(2)+5); // red
						}
						else
							setPixelColor(offset_x + 14, pixel_y, 0); // transparent
					}

					if ((*craft)->getRules()->getWeapons() > 0								// done in draw() above^
						&& (*craft)->getRules()->getWeapons() != (*craft)->getNumWeapons())	// craft needs Weapons mounted.
					{
						if (_blink)
							setPixelColor(offset_x + 10, pixel_y, Palette::blockOffset(8)+2); // blue
						else
							setPixelColor(offset_x + 10, pixel_y, 0); // transparent
					}

					pixel_y += 2;
				}
			}
		}
	}
}

}
