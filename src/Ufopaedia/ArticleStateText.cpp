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

#include "ArticleStateText.h"

#include "../Engine/Game.h"
//#include "../Engine/LocalizedText.h"
//#include "../Engine/Palette.h"
#include "../Engine/Surface.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/ArticleDefinition.h"


namespace OpenXcom
{

/**
 * cTor.
 * @param defs - pointer to ArticleDefinitionText (ArticleDefinition.h)
 */
ArticleStateText::ArticleStateText(const ArticleDefinitionText* const defs)
	:
		ArticleState(defs->id)
{
	_txtTitle	= new Text(296, 17, 5, 23);
	_txtInfo	= new Text(296, 150, 10, 48);

	setPalette("PAL_UFOPAEDIA");

	ArticleState::initLayout();

	add(_txtTitle);
	add(_txtInfo);

	centerAllSurfaces();


	_game->getResourcePack()->getSurface("BACK10.SCR")->blit(_bg);

	_btnOk->setColor(uPed_VIOLET);
	_btnPrev->setColor(uPed_VIOLET);
	_btnNext->setColor(uPed_VIOLET);

	_txtTitle->setText(tr(defs->title));
	_txtTitle->setColor(uPed_GREEN_SLATE);
	_txtTitle->setBig();

	_txtInfo->setText(tr(defs->text));
	_txtInfo->setColor(uPed_BLUE_SLATE);
	_txtInfo->setWordWrap();
}

/**
 * dTor.
 */
ArticleStateText::~ArticleStateText() // virtual.
{}

}
