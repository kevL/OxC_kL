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

#include "ResearchState.h"

//#include <sstream>

#include "AlienContainmentState.h"
#include "BasescapeState.h"
#include "MiniBaseView.h"
#include "NewResearchListState.h"
#include "ResearchInfoState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
//#include "../Engine/Screen.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleBaseFacility.h"
#include "../Ruleset/RuleResearch.h"

#include "../Savegame/Base.h"
#include "../Savegame/BaseFacility.h"
#include "../Savegame/ResearchProject.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Research screen.
 * @param base	- pointer to the base to get info from
 * @param state	- pointer to the BasescapeState (default NULL when geoscape-invoked)
 */
ResearchState::ResearchState(
		Base* base,
		BasescapeState* state)
	:
		_base(base),
		_state(state)
{
	_window			= new Window(this, 320, 200, 0, 0);
	_mini			= new MiniBaseView(128, 16, 180, 26, MBV_RESEARCH);

	_txtTitle		= new Text(300, 17, 16, 9);
	_txtBaseLabel	= new Text(80, 9, 16, 9);

	_txtAllocated	= new Text(60, 9, 16, 25);
	_txtAvailable	= new Text(60, 9, 16, 34);

	_txtSpace		= new Text(100, 9, 80, 25);

	_txtProject		= new Text(110, 9, 16, 48);
	_txtScientists	= new Text(55, 9, 161, 48);
	_txtProgress	= new Text(55, 9, 219, 48);

	_lstResearch	= new TextList(288, 113, 16, 63);

	_btnAliens		= new TextButton(92, 16, 16, 177);
	_btnNew			= new TextButton(92, 16, 114, 177);
	_btnOk			= new TextButton(92, 16, 212, 177);


	setPalette(
			"PAL_BASESCAPE",
			_game->getRuleset()->getInterface("researchMenu")->getElement("palette")->color);

	add(_window, "window", "researchMenu");
	add(_mini, "miniBase", "basescape");
	add(_txtTitle, "text", "researchMenu");
	add(_txtBaseLabel, "text", "researchMenu");
	add(_txtAvailable, "text", "researchMenu");
	add(_txtAllocated, "text", "researchMenu");
	add(_txtSpace, "text", "researchMenu");
	add(_txtProject, "text", "researchMenu");
	add(_txtScientists, "text", "researchMenu");
	add(_txtProgress, "text", "researchMenu");
	add(_lstResearch, "list", "researchMenu");
	add(_btnAliens, "button", "researchMenu");
	add(_btnNew, "button", "researchMenu");
	add(_btnOk, "button", "researchMenu");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK05.SCR"));

	_mini->setTexture(_game->getResourcePack()->getSurfaceSet("BASEBITS.PCK"));
	_mini->setBases(_game->getSavedGame()->getBases());
	for (size_t
			i = 0;
			i != _game->getSavedGame()->getBases()->size();
			++i)
	{
		if (_game->getSavedGame()->getBases()->at(i) == base)
		{
			_mini->setSelectedBase(i);
			break;
		}
	}
	_mini->onMouseClick(
					(ActionHandler)& ResearchState::miniClick,
					SDL_BUTTON_LEFT);

	_btnAliens->setText(tr("STR_ALIENS"));
	_btnAliens->onMouseClick((ActionHandler)& ResearchState::btnAliens);
	_btnAliens->setVisible(false);

	_btnNew->setText(tr("STR_NEW_PROJECT"));
	_btnNew->onMouseClick((ActionHandler)& ResearchState::btnNewClick);

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& ResearchState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& ResearchState::btnOkClick,
					Options::keyCancel);

	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_CURRENT_RESEARCH"));

	_txtBaseLabel->setText(_base->getName(_game->getLanguage()));

	_txtProject->setText(tr("STR_RESEARCH_PROJECT"));

	_txtScientists->setText(tr("STR_SCIENTISTS_ALLOCATED_UC"));

	_txtProgress->setText(tr("STR_PROGRESS"));

	_lstResearch->setBackground(_window);
	_lstResearch->setColumns(4, 137, 58, 48, 34);
	_lstResearch->setSelectable();
	_lstResearch->setMargin();
	_lstResearch->onMouseClick((ActionHandler)& ResearchState::onSelectProject);
}

/**
 * dTor.
 */
ResearchState::~ResearchState()
{}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void ResearchState::btnOkClick(Action*)
{
	_game->popState();
}

/**
 * Pops up the NewResearchListState screen.
 * @param action - pointer to an Action
 */
void ResearchState::btnNewClick(Action*)
{
	_game->pushState(new NewResearchListState(_base));
}

/**
 * Goes to the Manage Alien Containment screen.
 * @param action - pointer to an Action
 */
void ResearchState::btnAliens(Action*)
{
	_game->pushState(new AlienContainmentState(
											_base,
											OPT_GEOSCAPE));
}

/**
 * Selects a ResearchProject to begin research.
 * @param action - pointer to an Action
 */
void ResearchState::onSelectProject(Action*)
{
	size_t
		sel = _lstResearch->getSelectedRow(),	// original listclick
		sel2 = sel,								// entry of RP vector
		entry = 0;								// to break when vector.at[entry] meets sel

	for (std::vector<bool>::const_iterator
			i = _online.begin();
			i != _online.end();
			++i)
	{
		if (*i == false)
			++sel2;

		if (entry == sel2)
			break;

		++entry;
	}

	const std::vector<ResearchProject*>& rps (_base->getResearch()); // init.
	_game->pushState(new ResearchInfoState(
										_base,
										rps[sel2]));
//										rps[_lstResearch->getSelectedRow()]));
}

/**
 * Selects a new base to display.
 * @param action - pointer to an Action
 */
void ResearchState::miniClick(Action*)
{
	if (_state != NULL) // cannot switch bases if coming from geoscape.
	{
		const size_t baseID = _mini->getHoveredBase();
		if (baseID < _game->getSavedGame()->getBases()->size())
		{
			Base* const base = _game->getSavedGame()->getBases()->at(baseID);

			if (base != _base
				&& base->hasResearch() == true)
			{
				_base = base;
				_mini->setSelectedBase(baseID);
				_state->setBase(_base);

				init();
			}
		}
	}
}

/**
 * Updates the research list after going to other screens.
 */
void ResearchState::init()
{
	State::init();

	_online.clear();
	_lstResearch->clearList();

	const std::vector<ResearchProject*>& rps (_base->getResearch()); // init.
	for (std::vector<ResearchProject*>::const_iterator
			rp = rps.begin();
			rp != rps.end();
			++rp)
	{
		if ((*rp)->getOffline() == true)
		{
			_online.push_back(false);
			continue;
		}
		_online.push_back(true);


		const std::wstring wstProject = tr((*rp)->getRules()->getName());

		std::wostringstream wostsAssigned;
		wostsAssigned << (*rp)->getAssigned();

		std::wstring wstDaysLeft;

		if ((*rp)->getAssigned() > 0)
		{
			const int daysLeft = static_cast<int>(std::ceil(
								(static_cast<double>((*rp)->getCost() - (*rp)->getSpent()))
							   / static_cast<double>((*rp)->getAssigned())));
			wstDaysLeft = Text::formatNumber(daysLeft);
		}
		else
			 wstDaysLeft = L"-";

		_lstResearch->addRow(
							4,
							wstProject.c_str(),
							wostsAssigned.str().c_str(),
							tr((*rp)->getResearchProgress()).c_str(),
//							(*rp)->getCostCompleted().c_str());
							wstDaysLeft.c_str());
	}

	_txtAvailable->setText(tr("STR_SCIENTISTS_AVAILABLE")
							.arg(_base->getScientists()));
	_txtAllocated->setText(tr("STR_SCIENTISTS_ALLOCATED")
							.arg(_base->getAllocatedScientists()));
	_txtSpace->setText(tr("STR_LABORATORY_SPACE_AVAILABLE")
							.arg(_base->getFreeLaboratories()));


	if (_base->getAvailableContainment() > 0)
		_btnAliens->setVisible();
}

}
