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

#include "MedikitView.h"

//#include <iostream>

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Palette.h"
#include "../Engine/SurfaceSet.h"

#include "../Interface/Text.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleInterface.h"

#include "../Savegame/BattleUnit.h"


namespace OpenXcom
{

/**
 * User interface string identifier of body parts.
 */
const std::string PARTS_STRING[6] =
{
	"STR_HEAD",
	"STR_TORSO",
	"STR_RIGHT_ARM",
	"STR_LEFT_ARM",
	"STR_RIGHT_LEG",
	"STR_LEFT_LEG"
};


/**
 * Initializes the Medikit view.
 * @param w			- the MinikitView width
 * @param h			- the MinikitView height
 * @param x			- the MinikitView x origin
 * @param y			- the MinikitView y origin
 * @param game		- pointer to the core Game
 * @param unit		- pointer to the wounded BattleUnit
 * @param partTxt	- pointer to a Text; will be updated with the selected body part
 * @param woundTxt	- pointer to a Text; will be updated with the amount of fatal woundage
 */
MedikitView::MedikitView(
		int w,
		int h,
		int x,
		int y,
		Game* game,
		BattleUnit* unit,
		Text* partTxt,
		Text* woundTxt)
	:
		InteractiveSurface(w,h,x,y),
		_game(game),
		_selectedPart(0),
		_unit(unit),
		_partTxt(partTxt),
		_woundTxt(woundTxt)
{
	updateSelectedPart();
	_redraw = true;
}

/**
 * Draws the medikit view.
 */
void MedikitView::draw()
{
	SurfaceSet* const srt = _game->getResourcePack()->getSurfaceSet("MEDIBITS.DAT");
	Uint8 color;

	this->lock();
	for (int
			i = 0;
			i != srt->getTotalFrames();
			++i)
	{
		if (_unit->getFatalWound(i) != 0)
			color = static_cast<Uint8>(_game->getRuleset()->getInterface("medikit")->getElement("body")->color2); // red
		else
			color = static_cast<Uint8>(_game->getRuleset()->getInterface("medikit")->getElement("body")->color); // green

		Surface* const surface = srt->getFrame(i);
		surface->blitNShade(
						this,
						Surface::getX(),
						Surface::getY(),
						0,
						false,
						color);
	}
	this->unlock();

	_redraw = false;


	if (_selectedPart == -1)
		return;


	std::wostringstream
		ss1,
		ss2;

	ss1 << _game->getLanguage()->getString(PARTS_STRING[_selectedPart]);
	_partTxt->setText(ss1.str());

	ss2 << _unit->getFatalWound(_selectedPart);
	_woundTxt->setText(ss2.str());
}

/**
 * Handles clicks on the medikit view.
 * @param action - pointer to an Action
 * @param state - state that the action handlers belong to
 */
void MedikitView::mouseClick(Action* action, State*)
{
	SurfaceSet* const srt = _game->getResourcePack()->getSurfaceSet("MEDIBITS.DAT");

	const int
		x = static_cast<int>(action->getRelativeXMouse() / action->getXScale()),
		y = static_cast<int>(action->getRelativeYMouse() / action->getYScale());

	for (size_t
			i = 0;
			i < srt->getTotalFrames();
			++i)
	{
		const Surface* const surface = srt->getFrame(i);
		if (surface->getPixelColor(x, y))
		{
			_selectedPart = i;
			_redraw = true;

			break;
		}
	}
}

/**
 * Gets the selected body part.
 * @return The selected body part.
 */
int MedikitView::getSelectedPart() const
{
	return _selectedPart;
}

/**
 * Updates the selected body part.
 * If there is a wounded body part, selects that.
 * Otherwise does not change the selected part.
 */
void MedikitView::updateSelectedPart()
{
	for (int
			i = 0;
			i < 6;
			++i)
	{
		if (_unit->getFatalWound(i) != 0)
		{
			_selectedPart = i;
			break;
		}
	}
}

}
