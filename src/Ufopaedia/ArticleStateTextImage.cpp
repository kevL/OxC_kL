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

#include "ArticleStateTextImage.h"

#include "ExtraAlienInfoState.h"
#include "Ufopaedia.h"

#include "../Engine/Game.h"
//#include "../Engine/LocalizedText.h"
//#include "../Engine/Palette.h"
#include "../Engine/Surface.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/ArticleDefinition.h"

#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * cTor.
 * @param defs - pointer to ArticleDefinitionTextImage (ArticleDefinition.h)
 */
ArticleStateTextImage::ArticleStateTextImage(const ArticleDefinitionTextImage* const defs)
	:
		ArticleState(defs->id),
		_defs(defs)
{
	_txtTitle = new Text(defs->text_width, 48, 5, 22);

	setPalette("PAL_UFOPAEDIA");

	ArticleState::initLayout();

	add(_txtTitle);

	_game->getResourcePack()->getSurface(defs->image_id)->blit(_bg);

	_btnOk->setColor(uPed_VIOLET);
	_btnPrev->setColor(uPed_VIOLET);
	_btnNext->setColor(uPed_VIOLET);

	_txtTitle->setText(tr(defs->title));
	_txtTitle->setColor(uPed_GREEN_SLATE);
	_txtTitle->setBig();
	_txtTitle->setWordWrap();

	const int text_height = _txtTitle->getTextHeight();
	_txtInfo = new Text(
					defs->text_width,
					162,
					5,
					23 + text_height);
	add(_txtInfo);
	_txtInfo->setText(tr(defs->text));
	_txtInfo->setColor(uPed_BLUE_SLATE);
	_txtInfo->setWordWrap();

	_btnExtraInfo = new TextButton(50, 16, 0, 184);
	add(_btnExtraInfo);
	_btnExtraInfo->setText(L"info");
	_btnExtraInfo->setColor(uPed_GREEN_SLATE);
	_btnExtraInfo->setVisible(defs->section == UFOPAEDIA_ALIEN_LIFE_FORMS
							&& showInfoBtn() == true);
	_btnExtraInfo->onMouseClick((ActionHandler)& ArticleStateTextImage::btnInfo);

	centerAllSurfaces();
}

/**
 * dTor.
 */
ArticleStateTextImage::~ArticleStateTextImage() // virtual.
{}

/**
 * Opens the ExtraAlienInfoState screen.
 * @param action - pointer to an Action
 */
void ArticleStateTextImage::btnInfo(Action*) // private.
{
	_game->pushState(new ExtraAlienInfoState(_defs));
}

/**
 * Checks if necessary research has been done before showing the ExtraInfo button.
 * @note Player needs to have both the alien and its autopsy researched.
 * @return, true if the researches have been done
 */
bool ArticleStateTextImage::showInfoBtn() // private.
{
	if (_defs->id.find("_AUTOPSY") != std::string::npos)
	{
		const std::string alienId = _defs->id.substr(0, _defs->id.length() - 8);
		if (_game->getSavedGame()->isResearched(alienId) == true)
			return true;
	}
	else if (_game->getSavedGame()->isResearched(_defs->id + "_AUTOPSY") == true)
		return true;

	return false;
}

}
