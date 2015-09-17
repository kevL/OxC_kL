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

#include "NewResearchListState.h"

#include "ResearchInfoState.h"

#include "../Engine/Game.h"
//#include "../Engine/LocalizedText.h"
//#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleResearch.h"

#include "../Savegame/Base.h"
#include "../Savegame/ResearchProject.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{
/**
 * Initializes all the elements in the research list screen.
 * @param base - pointer to the Base to get info from
 */
NewResearchListState::NewResearchListState(
		Base* base)
	:
		_base(base),
		_cutoff(-1),
		_scroll(0)
{
	_screen = false;

	_window			= new Window(this, 230, 150, 45, 25, POPUP_BOTH);
	_txtTitle		= new Text(214, 16, 53, 33);
	_lstResearch	= new TextList(190, 105, 61, 44);
	_btnCancel		= new TextButton(214, 16, 53, 152);

	setInterface("selectNewResearch");

	add(_window,		"window",	"selectNewResearch");
	add(_txtTitle,		"text",		"selectNewResearch");
	add(_lstResearch,	"list",		"selectNewResearch");
	add(_btnCancel,		"button",	"selectNewResearch");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK05.SCR"));

	_txtTitle->setText(tr("STR_NEW_RESEARCH_PROJECTS"));
	_txtTitle->setAlign(ALIGN_CENTER);

	_lstResearch->setColumns(1, 180);
	_lstResearch->setBackground(_window);
	_lstResearch->setSelectable();
	_lstResearch->setMargin();
	_lstResearch->setAlign(ALIGN_CENTER);
	_lstResearch->onMouseClick((ActionHandler)& NewResearchListState::onSelectProject);

	_btnCancel->setText(tr("STR_CANCEL"));
	_btnCancel->onMouseClick((ActionHandler)& NewResearchListState::btnCancelClick);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& NewResearchListState::btnCancelClick,
					Options::keyCancel);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& NewResearchListState::btnCancelClick,
					Options::keyOk);
	_btnCancel->onKeyboardPress(
					(ActionHandler)& NewResearchListState::btnCancelClick,
					Options::keyOkKeypad);
}

/**
 * Initializes the screen and fills the list with ResearchProjects.
 */
void NewResearchListState::init()
{
	State::init();

	fillProjectList();
	_lstResearch->scrollTo(_scroll);
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void NewResearchListState::btnCancelClick(Action*)
{
	_game->popState();
}

/**
 * Selects the RuleResearch to work on.
 * @note If an offline ResearchProject is selected the spent & cost values are preserved.
 * @param action - pointer to an Action
 */
void NewResearchListState::onSelectProject(Action*) // private.
{
	_scroll = _lstResearch->getScroll();

	if (static_cast<int>(_lstResearch->getSelectedRow()) > _cutoff)
		_game->pushState(new ResearchInfoState( // brand new project
											_base,
											_projects[static_cast<size_t>(static_cast<int>(_lstResearch->getSelectedRow()) - (_cutoff + 1))]));
	else
		_game->pushState(new ResearchInfoState( // offline project reactivation.
											_base,
											_offlines[_lstResearch->getSelectedRow()]));
//	_game->popState();
}

/**
 * Fills the list with ResearchProjects.
 */
void NewResearchListState::fillProjectList() // private.
{
	_cutoff = -1;
	_offlines.clear();
	_projects.clear();
	_lstResearch->clearList();

	size_t row = 0;
	const Uint8 color = _lstResearch->getSecondaryColor();

	const std::vector<ResearchProject*>& currentProjects (_base->getResearch()); // init.
	for (std::vector<ResearchProject*>::const_iterator
			i = currentProjects.begin();
			i != currentProjects.end();
			++i)
	{
		// If cancelled projects are not marked 'offline' they'd lose spent
		// research time; if they are not marked 'offline' even though no
		// spent time has accumulated, they still need to be tracked so player
		// can't exploit the system by forcing a recalculation of totalCost ....
		if ((*i)->getOffline() == true)
		{
			std::wstring wst = tr((*i)->getRules()->getName());
			if ((*i)->getSpent() != 0)
				wst += L" (" + Text::formatNumber((*i)->getSpent()) + L")";

			_lstResearch->addRow(1, wst.c_str());
			// Try to blend these in but doesn't really work due to ordering:
//			if ((*i)->getSpent() > 0)
			_lstResearch->setRowColor(row++, color, true);

			_offlines.push_back(*i);
			++_cutoff;
		}
	}


	_game->getSavedGame()->getAvailableResearchProjects(
													_projects,
													_game->getRuleset(),
													_base);

	std::vector<RuleResearch*>::const_iterator i = _projects.begin();
	while (i != _projects.end())
	{
		if ((*i)->getRequirements().empty() == true)
		{
			_lstResearch->addRow(1, tr((*i)->getName()).c_str());
			++i;
		}
		else
			i = _projects.erase(i);
	}
}

}
