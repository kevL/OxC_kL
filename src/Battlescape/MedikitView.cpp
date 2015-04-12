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
 * A array of strings that define body parts.
 */
const std::string MedikitView::BODY_PARTS[] =
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
 * @param w		- the MedikitView width
 * @param h		- the MedikitView height
 * @param x		- the MedikitView x origin
 * @param y		- the MedikitView y origin
 * @param game	- pointer to the core Game
 * @param unit	- pointer to the wounded BattleUnit
 * @param part	- pointer to Text of the selected body part
 * @param wound	- pointer to Text of fatal woundage
 */
MedikitView::MedikitView(
		int w,
		int h,
		int x,
		int y,
		Game* game,
		BattleUnit* unit,
		Text* part,
		Text* wound)
	:
		InteractiveSurface(
			w,h,
			x,y),
		_game(game),
		_selectedPart(-1),
		_unit(unit),
		_txtPart(part),
		_txtWound(wound)
{
	autoSelectPart();
	_redraw = true;
}

/**
 * Draws the medikit view.
 */
void MedikitView::draw()
{
	SurfaceSet* const srt = _game->getResourcePack()->getSurfaceSet("MEDIBITS.DAT");
	int color;

	this->lock();
	for (int
			i = 0;
			i != BODYPARTS; //static_cast<int>(srt->getTotalFrames());
			++i)
	{
		if (_unit->getFatalWound(i) != 0)
			color = _game->getRuleset()->getInterface("medikit")->getElement("body")->color2;
		else
			color = _game->getRuleset()->getInterface("medikit")->getElement("body")->color;

		Surface* const srf = srt->getFrame(i);
		srf->blitNShade(
					this,
					Surface::getX(),
					Surface::getY(),
					0,
					false,
					color);
	}
	this->unlock();

	_redraw = false;


	if (_selectedPart != -1)
	{
		_txtPart->setText(_game->getLanguage()->getString(BODY_PARTS[static_cast<size_t>(_selectedPart)]));

		std::wostringstream woststr;
		woststr << _unit->getFatalWound(_selectedPart);
		_txtWound->setText(woststr.str());
	}
}

/**
 * Handles clicks on the medikit view.
 * @param action - pointer to an Action
 * @param state - state that the action handlers belong to
 */
void MedikitView::mouseClick(Action* action, State*)
{
	SurfaceSet* const sst = _game->getResourcePack()->getSurfaceSet("MEDIBITS.DAT");

	const int
		x = static_cast<int>(action->getRelativeXMouse() / action->getXScale()),
		y = static_cast<int>(action->getRelativeYMouse() / action->getYScale());

	const Surface* srf;
	for (int
			i = 0;
			i != BODYPARTS; //static_cast<int>(sst->getTotalFrames());
			++i)
	{
		srf = sst->getFrame(i);
		if (srf->getPixelColor(x,y) != 0)
		{
			_selectedPart = i;
			_redraw = true;

			break;
		}
	}
}

/**
 * Gets the selected body part.
 * @return, the selected body part
 */
int MedikitView::getSelectedPart() const
{
	return _selectedPart;
}

/**
 * Automatically selects a wounded body part.
 */
void MedikitView::autoSelectPart()
{
	for (int
			i = 0;
			i != BODYPARTS;
			++i)
	{
		if (_unit->getFatalWound(i) != 0)
		{
			_selectedPart = i;
			return;
		}
	}

	_txtPart->setVisible(false);
	_txtWound->setVisible(false);
}

}
