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

#include "NewPossibleResearchState.h"

//#include <algorithm>

#include "../Basescape/ResearchState.h"

#include "../Engine/Game.h"
//#include "../Engine/LocalizedText.h"
//#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleResearch.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/SavedGame.h"


namespace OpenXcom
{
/**
 * Initializes all the elements in the EndResearch screen.
 * @param base					- pointer to the Base to get info from
 * @param possibilities			- reference the vector of pointers to RuleResearch projects
 * @param showResearchButton	- true to show new research button
 */
NewPossibleResearchState::NewPossibleResearchState(
		Base* base,
		const std::vector<RuleResearch*>& possibilities,
		bool showResearchButton) // myk002_add.
	:
		_base(base)
{
	_screen = false;

	_window				= new Window(this, 288, 180, 16, 10);
	_txtTitle			= new Text(288, 40, 16, 20);

	_lstPossibilities	= new TextList(253, 81, 24, 56);

	_btnOk				= new TextButton(160, 14, 80, 149);
	_btnResearch		= new TextButton(160, 14, 80, 165);

	setInterface("geoResearch");

	add(_window,			"window",	"geoResearch");
	add(_txtTitle,			"text1",	"geoResearch");
	add(_lstPossibilities,	"text2",	"geoResearch");
	add(_btnOk,				"button",	"geoResearch");
	add(_btnResearch,		"button",	"geoResearch");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK05.SCR"));

//myk002	_btnOk->setText(tr("STR_OK"));
	_btnOk->setText(tr(showResearchButton? "STR_OK": "STR_MORE")); // myk002
	_btnOk->onMouseClick((ActionHandler)& NewPossibleResearchState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& NewPossibleResearchState::btnOkClick,
					Options::keyCancel);

	_btnResearch->setText(tr("STR_ALLOCATE_RESEARCH"));
	_btnResearch->setVisible(showResearchButton); // myk002
	_btnResearch->onMouseClick((ActionHandler)& NewPossibleResearchState::btnResearchClick);
	_btnResearch->onKeyboardPress(
					(ActionHandler)& NewPossibleResearchState::btnResearchClick,
					Options::keyOk);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);

	_lstPossibilities->setColumns(1, 288);
	_lstPossibilities->setBig();
	_lstPossibilities->setAlign(ALIGN_CENTER);

	size_t tally (0); // init.

	for (std::vector<RuleResearch *>::const_iterator
			i = possibilities.begin();
			i != possibilities.end();
			++i)
	{
		if (_game->getSavedGame()->wasResearchPopped(*i) == false
			&& (*i)->getRequirements().empty() == true
			&& _game->getRuleset()->getUnit((*i)->getName()) == NULL)
		{
			_game->getSavedGame()->addPoppedResearch((*i));
			_lstPossibilities->addRow(
									1,
									tr((*i)->getName ()).c_str());
		}
		else
			++tally;
	}

	if (!
		(tally == possibilities.size()
			|| possibilities.empty()))
	{
		_txtTitle->setText(tr("STR_WE_CAN_NOW_RESEARCH"));
	}
}

/**
 * return to the previous screen
 * @param action - pointer to an Action
 */
void NewPossibleResearchState::btnOkClick(Action*)
{
	_game->popState();
}

/**
 * Open the ResearchState so the player can dispatch available scientist.
 * @param action - pointer to an Action
 */
void NewPossibleResearchState::btnResearchClick(Action*)
{
	_game->popState();
	_game->pushState(new ResearchState(_base));
}

}
