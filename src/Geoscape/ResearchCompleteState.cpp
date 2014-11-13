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

#include "ResearchCompleteState.h"

#include <algorithm>

#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Logger.h" // kL
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
 * Initializes all the elements in the ResearchComplete screen.
 * @param game		- pointer to the core game
 * @param research	- pointer to the completed research
 * @param bonus		- pointer to bonus unlocked research
 */
ResearchCompleteState::ResearchCompleteState(
		const RuleResearch* research,
		const RuleResearch* bonus)
	:
		_research(research),
		_bonus(bonus)
{
	_screen = false;

	_window			= new Window(this, 230, 140, 45, 30, POPUP_BOTH);
	_txtTitle		= new Text(230, 17, 45, 70);

	_txtResearch	= new Text(230, 17, 45, 96);

	_btnReport		= new TextButton(80, 16, 64, 146);
	_btnOk			= new TextButton(80, 16, 176, 146);

	setPalette("PAL_GEOSCAPE", 0);

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
					Options::keyCancel);

	_btnReport->setColor(Palette::blockOffset(8)+5);
	_btnReport->setText(tr("STR_VIEW_REPORTS"));
	_btnReport->onMouseClick((ActionHandler)& ResearchCompleteState::btnReportClick);
	_btnReport->onKeyboardPress(
						(ActionHandler)& ResearchCompleteState::btnReportClick,
						Options::keyOk);

	_txtTitle->setColor(Palette::blockOffset(15)-1);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_RESEARCH_COMPLETED"));

	_txtResearch->setColor(Palette::blockOffset(8)+10);
	_txtResearch->setAlign(ALIGN_CENTER);
	_txtResearch->setBig();
//kL	_txtResearch->setWordWrap();
	if (research)
		_txtResearch->setText(tr(research->getName()));
}

/**
 * Returns to the previous screen
 * @param action - pointer to an Action
 */
void ResearchCompleteState::btnOkClick(Action*)
{
	_game->popState();
}

/**
 * Opens the Ufopaedia to the entry about the Research.
 * @param action - pointer to an Action
 */
void ResearchCompleteState::btnReportClick(Action*)
{
	//Log(LOG_INFO) << "ResearchCompleteState::btnReportClick()";
	//if (_research) Log(LOG_INFO) << ". There IS _research";

	//Log(LOG_INFO) << ". popState";
	_game->popState();
	//Log(LOG_INFO) << ". popState DONE";

	if (_bonus)
	{
		//Log(LOG_INFO) << ". . in Bonus";

		std::string bonusName;
		if (_bonus->getLookup().empty())
			bonusName = _bonus->getName();
		else
			bonusName = _bonus->getLookup();

		Ufopaedia::openArticle(
							_game,
							bonusName);
	}

	if (_research)
	{
		//Log(LOG_INFO) << ". . in Research";

		std::string name;
		if (_research->getLookup().empty())
			name = _research->getName();
		else
			name = _research->getLookup();

		Ufopaedia::openArticle(
							_game,
							name);
	}
}

}
