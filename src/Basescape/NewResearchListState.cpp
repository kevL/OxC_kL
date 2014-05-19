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

#include "NewResearchListState.h"

#include <algorithm>

#include "ResearchInfoState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleResearch.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h"
#include "../Savegame/ItemContainer.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{
/**
 * Initializes all the elements in the Research list screen.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 */
NewResearchListState::NewResearchListState(
		Game* game,
		Base* base)
	:
		State(game),
		_base(base)
{
	_screen = false;

	_window			= new Window(this, 230, 136, 45, 32, POPUP_BOTH);
	_txtTitle		= new Text(214, 16, 53, 40);
	_lstResearch	= new TextList(198, 88, 53, 52);
	_btnCancel		= new TextButton(214, 16, 53, 144);

	setPalette("PAL_BASESCAPE", 1);

	add(_window);
	add(_btnCancel);
	add(_txtTitle);
	add(_lstResearch);

	centerAllSurfaces();

	_window->setColor(Palette::blockOffset(13)+10);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK05.SCR"));

	_btnCancel->setColor(Palette::blockOffset(15)+6);
	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)& NewResearchListState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& NewResearchListState::btnCancelClick,
					Options::keyCancel);

	_txtTitle->setColor(Palette::blockOffset(13)+10);
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_NEW_RESEARCH_PROJECTS"));

	_lstResearch->setColor(Palette::blockOffset(13));
	_lstResearch->setColumns(1, 198);
	_lstResearch->setSelectable(true);
	_lstResearch->setBackground(_window);
	_lstResearch->setMargin(8);
	_lstResearch->setAlign(ALIGN_CENTER);
	_lstResearch->setArrowColor(Palette::blockOffset(13)+10);
	_lstResearch->onMouseClick((ActionHandler)& NewResearchListState::onSelectProject);
}

/**
 * Initializes the screen (fills the list).
 */
void NewResearchListState::init()
{
	State::init();

	fillProjectList();
}

/**
 * Selects the RuleResearch to work on.
 * @param action Pointer to an action.
 */
void NewResearchListState::onSelectProject(Action*)
{
	_game->pushState(new ResearchInfoState(
										_game,
										_base,
										_projects[_lstResearch->getSelectedRow()]));
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void NewResearchListState::btnCancelClick(Action*)
{
	_game->popState();
}

/**
 * Fills the list with possible ResearchProjects.
 */
void NewResearchListState::fillProjectList()
{
	_projects.clear();
	_lstResearch->clearList();

	_game->getSavedGame()->getAvailableResearchProjects(
													_projects,
													_game->getRuleset(),
													_base);

	std::vector<RuleResearch*>::iterator it = _projects.begin();
	while (it != _projects.end())
	{
		if ((*it)->getRequirements().empty())
		{
			_lstResearch->addRow(
								1,
								tr((*it)->getName()).c_str());

			++it;
		}
		else
		{
			it = _projects.erase(it);
		}
	}
}

}
