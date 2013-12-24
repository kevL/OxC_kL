/*
 * Copyright 2010-2013 OpenXcom Developers.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ResearchCompleteState.h"

#include <algorithm>

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/ArticleDefinition.h"
#include "../Ruleset/RuleResearch.h"
#include "../Ruleset/Ruleset.h"

#include "../Ufopaedia/Ufopaedia.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the EndResearch screen.
 * @param game Pointer to the core game.
 * @param research Pointer to the completed research.
 * @param bonus Pointer to bonus unlocked research.
 */
ResearchCompleteState::ResearchCompleteState(
		Game* game,
		const RuleResearch* research,
		const RuleResearch* bonus)
	:
		State(game),
		_research(research),
		_bonus(bonus)
{
	_screen = false;


	_window			= new Window(this, 230, 140, 45, 30, POPUP_BOTH);
	_txtTitle		= new Text(230, 17, 45, 70);

	_txtResearch	= new Text(230, 17, 45, 96);

	_btnReport		= new TextButton(80, 16, 64, 146);
	_btnOk			= new TextButton(80, 16, 176, 146);


	_game->setPalette(
				_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(0)),
				Palette::backPos,
				16);

	add(_window);
	add(_txtTitle);
	add(_txtResearch);
	add(_btnReport);
	add(_btnOk);

	centerAllSurfaces();


	_window->setColor(Palette::blockOffset(15)-1);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK05.SCR"));

	_btnOk->setColor(Palette::blockOffset(8)+5);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& ResearchCompleteState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& ResearchCompleteState::btnOkClick,
					(SDLKey)Options::getInt("keyCancel"));

	_btnReport->setColor(Palette::blockOffset(8)+5);
	_btnReport->setText(tr("STR_VIEW_REPORTS"));
	_btnReport->onMouseClick((ActionHandler)& ResearchCompleteState::btnReportClick);
	_btnReport->onKeyboardPress(
						(ActionHandler)& ResearchCompleteState::btnReportClick,
						(SDLKey)Options::getInt("keyOk"));

	_txtTitle->setColor(Palette::blockOffset(15)-1);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_RESEARCH_COMPLETED"));

	_txtResearch->setColor(Palette::blockOffset(8)+10);
	_txtResearch->setAlign(ALIGN_CENTER);
	_txtResearch->setBig();
//kL	_txtResearch->setWordWrap(true);
	if (research)
	{
		_txtResearch->setText(tr(research->getName()));
	}
}

/**
 * Resets the palette.
 */
void ResearchCompleteState::init()
{
	_game->setPalette(
				_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(0)),
				Palette::backPos,
				16);
}

/**
 * return to the previous screen
 * @param action Pointer to an action.
 */
void ResearchCompleteState::btnOkClick(Action*)
{
	_game->popState();
}

/**
 * open the Ufopaedia to the entry about the Research.
 * @param action Pointer to an action.
 */
void ResearchCompleteState::btnReportClick(Action*)
{
	_game->popState();

	if (_bonus)
	{
		std::string bonusName;
		if (_bonus->getLookup() == "")
			bonusName = _bonus->getName();
		else
			bonusName = _bonus->getLookup();

		Ufopaedia::openArticle(_game, bonusName);
	}

	if (_research)
	{
		std::string name;
		if (_research->getLookup() == "")
			name = _research->getName();
		else
			name = _research->getLookup();

		Ufopaedia::openArticle(_game, name);
	}
}

}
