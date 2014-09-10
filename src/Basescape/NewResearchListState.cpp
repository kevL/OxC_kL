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
//#include "../Engine/Logger.h"
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
#include "../Savegame/ResearchProject.h" // kL
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{
/**
 * Initializes all the elements in the Research list screen.
 * @param base Pointer to the base to get info from.
 */
NewResearchListState::NewResearchListState(
		Base* base)
	:
		_base(base),
		_offlines(), // kL
		_projects(), // kL
		_cutoff(-1) // kL
{
	_screen = false;

	_window			= new Window(this, 230, 136, 45, 32, POPUP_BOTH);
	_txtTitle		= new Text(214, 16, 53, 40);
	_lstResearch	= new TextList(190, 88, 61, 52);
	_btnCancel		= new TextButton(214, 16, 53, 144);

	setPalette("PAL_BASESCAPE", 1);

	add(_window);
	add(_txtTitle);
	add(_lstResearch);
	add(_btnCancel);

	centerAllSurfaces();

	_window->setColor(Palette::blockOffset(13)+10);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK05.SCR"));

	_txtTitle->setColor(Palette::blockOffset(13)+10);
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_NEW_RESEARCH_PROJECTS"));

	_lstResearch->setColor(Palette::blockOffset(13));
	_lstResearch->setColumns(1, 180);
	_lstResearch->setSelectable();
	_lstResearch->setBackground(_window);
	_lstResearch->setMargin();
	_lstResearch->setAlign(ALIGN_CENTER);
	_lstResearch->setArrowColor(Palette::blockOffset(13)+10);
	_lstResearch->onMouseClick((ActionHandler)& NewResearchListState::onSelectProject);

	_btnCancel->setColor(Palette::blockOffset(15)+6);
	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)& NewResearchListState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& NewResearchListState::btnCancelClick,
					Options::keyCancel);
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
 * kL: Or restarts an old ResearchProject; spent & cost values are preserved.
 * @param action Pointer to an action.
 */
void NewResearchListState::onSelectProject(Action*)
{
	if (static_cast<int>(_lstResearch->getSelectedRow()) > _cutoff) // kL
	{
		//Log(LOG_INFO) << ". new project";
		_game->pushState(new ResearchInfoState( // brand new project
											_base,
											_projects[static_cast<size_t>(static_cast<int>(_lstResearch->getSelectedRow()) - (_cutoff + 1))]));
	}
	else
	{
		//Log(LOG_INFO) << ". reactivate";
		_game->pushState(new ResearchInfoState( // kL, offline project reactivation.
											_base,
											_offlines[_lstResearch->getSelectedRow()]));
	}

//	_game->popState(); // kL
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
	//Log(LOG_INFO) << "NewResearchListState::fillProjectList()";
	_cutoff = -1; // kL
	_offlines.clear(); // kL

	_projects.clear();
	_lstResearch->clearList();

	// kL_begin:
	size_t row = 0;

	const std::vector<ResearchProject*>& baseProjects(_base->getResearch());
	for (std::vector<ResearchProject*>::const_iterator
			i = baseProjects.begin();
			i != baseProjects.end();
			++i)
	{
		if ((*i)->getOffline())
		{
			//Log(LOG_INFO) << ". offline: " << (*i)->getRules()->getName();
			_lstResearch->addRow(
								1,
								tr((*i)->getRules()->getName()).c_str());

			_lstResearch->setRowColor(row, Palette::blockOffset(10));

			_offlines.push_back(*i);

			row++;
			_cutoff++;
		}
		//else Log(LOG_INFO) << ". already online: " << (*i)->getRules()->getName();
	} // kL_end.


	_game->getSavedGame()->getAvailableResearchProjects(
													_projects,
													_game->getRuleset(),
													_base);

	std::vector<RuleResearch*>::iterator i = _projects.begin();
	while (i != _projects.end())
	{
		//Log(LOG_INFO) << ". rule: " << (*i)->getName();
		if ((*i)->getRequirements().empty())
		{
			//Log(LOG_INFO) << ". rule: add";
			_lstResearch->addRow(
								1,
								tr((*i)->getName()).c_str());

			_lstResearch->setRowColor(row, Palette::blockOffset(13)); // kL

			row++; // kL

			++i;
		}
		else
		{
			//Log(LOG_INFO) << ". rule: erase";
			i = _projects.erase(i);
		}
	}
}

}
