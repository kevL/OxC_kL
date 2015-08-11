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

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
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
	"STR_HEAD",			// 0
	"STR_TORSO",		// 1
	"STR_RIGHT_ARM",	// 2
	"STR_LEFT_ARM",		// 3
	"STR_RIGHT_LEG",	// 4
	"STR_LEFT_LEG"		// 5
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
		_selectedPart(BattleUnit::PARTS_BODY),
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
	for (size_t
			i = 0;
			i != BattleUnit::PARTS_BODY; //static_cast<int>(srt->getTotalFrames());
			++i)
	{
		if (_unit->getFatalWound(i) != 0)
			color = _game->getRuleset()->getInterface("medikit")->getElement("body")->color2;
		else
			color = _game->getRuleset()->getInterface("medikit")->getElement("body")->color;

		Surface* const srf = srt->getFrame(static_cast<int>(i));
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


	if (_selectedPart != BattleUnit::PARTS_BODY)
	{
		_txtPart->setText(_game->getLanguage()->getString(BODY_PARTS[_selectedPart]));

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
	SurfaceSet* const srt = _game->getResourcePack()->getSurfaceSet("MEDIBITS.DAT");
	const Surface* srf;

	const int
		x = static_cast<int>(action->getRelativeXMouse() / action->getXScale()),
		y = static_cast<int>(action->getRelativeYMouse() / action->getYScale());

	for (size_t
			i = 0;
			i != BattleUnit::PARTS_BODY; //static_cast<int>(srt->getTotalFrames());
			++i)
	{
		srf = srt->getFrame(static_cast<int>(i));
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
size_t MedikitView::getSelectedPart() const
{
	return _selectedPart;
}

/**
 * Automatically selects a wounded body part.
 */
void MedikitView::autoSelectPart()
{
	for (size_t
			i = 0;
			i != BattleUnit::PARTS_BODY;
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
