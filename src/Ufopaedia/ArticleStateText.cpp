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

#include "ArticleStateText.h"

#include "Ufopaedia.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Palette.h"
#include "../Engine/Surface.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/ArticleDefinition.h"


namespace OpenXcom
{

ArticleStateText::ArticleStateText(
		Game* game,
		ArticleDefinitionText* defs)
	:
		ArticleState(game, defs->id)
{
	_txtTitle	= new Text(296, 17, 5, 23);
	_txtInfo	= new Text(296, 150, 10, 48);

	setPalette("PAL_UFOPAEDIA");

	ArticleState::initLayout();

	add(_txtTitle);
	add(_txtInfo);

	centerAllSurfaces();

	_game->getResourcePack()->getSurface("BACK10.SCR")->blit(_bg);
	_btnOk->setColor(Palette::blockOffset(5));
	_btnPrev->setColor(Palette::blockOffset(5));
	_btnNext->setColor(Palette::blockOffset(5));

	_txtTitle->setColor(Palette::blockOffset(15)+4);
	_txtTitle->setBig();
	_txtTitle->setText(tr(defs->title));

	_txtInfo->setColor(Palette::blockOffset(15)-1);
	_txtInfo->setWordWrap(true);
	_txtInfo->setText(tr(defs->text));
}

ArticleStateText::~ArticleStateText()
{
}

}
