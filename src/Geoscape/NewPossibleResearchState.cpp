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
		Base* const base,
		const std::vector<RuleResearch*>& possibilities,
		bool showResearchButton)
	:
		_base(base)
{
	_screen = false;

	_window				= new Window(this, 288, 180, 16, 10);
	_txtTitle			= new Text(288, 17, 16, 20);

	_lstPossibilities	= new TextList(253, 81, 24, 56);

	_btnResearch		= new TextButton(160, 14, 80, 149);
	_btnOk				= new TextButton(160, 14, 80, 165);

	setInterface("geoResearch");

	add(_window,			"window",	"geoResearch");
	add(_txtTitle,			"text1",	"geoResearch");
	add(_lstPossibilities,	"text2",	"geoResearch");
	add(_btnResearch,		"button",	"geoResearch");
	add(_btnOk,				"button",	"geoResearch");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK05.SCR"));

	_btnOk->setText(tr(showResearchButton ? "STR_OK" : "STR_MORE"));
	_btnOk->onMouseClick((ActionHandler)& NewPossibleResearchState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& NewPossibleResearchState::btnOkClick,
					Options::keyCancel);

	_btnResearch->setText(tr("STR_ALLOCATE_RESEARCH"));
	_btnResearch->setVisible(showResearchButton);
	_btnResearch->onMouseClick((ActionHandler)& NewPossibleResearchState::btnResearchClick);
	_btnResearch->onKeyboardPress(
					(ActionHandler)& NewPossibleResearchState::btnResearchClick,
					Options::keyOk);

	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_lstPossibilities->setColumns(1, 261);
	_lstPossibilities->setAlign(ALIGN_CENTER);
	_lstPossibilities->setBig();

	size_t tally (0);
	for (std::vector<RuleResearch *>::const_iterator
			i = possibilities.begin();
			i != possibilities.end();
			++i)
	{
		if (_game->getSavedGame()->wasResearchPopped(*i) == false
			&& (*i)->getRequirements().empty() == true
			&& ((*i)->needsItem() == false || _game->getRuleset()->getUnit((*i)->getType()) == NULL)) // not liveAlien.
		{
			_game->getSavedGame()->addPoppedResearch((*i));
			_lstPossibilities->addRow(1, tr((*i)->getType ()).c_str());
		}
		else
			++tally;
	}

	if (possibilities.empty() == false
		&& possibilities.size() != tally)
	{
		_txtTitle->setText(tr("STR_WE_CAN_NOW_RESEARCH"));
	}
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void NewPossibleResearchState::btnOkClick(Action*)
{
	_game->popState();
}

/**
 * Opens the ResearchState so the player can dispatch available scientists.
 * @param action - pointer to an Action
 */
void NewPossibleResearchState::btnResearchClick(Action*)
{
	_game->popState();
	_game->pushState(new ResearchState(_base));
}

}
