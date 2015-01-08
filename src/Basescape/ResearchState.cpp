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


namespace OpenXcom
{

/**
 * Initializes all the elements in the Research screen.
 * @param base - pointer to the base to get info from
 */
ResearchState::ResearchState(
		Base* base)
	:
		_base(base)
{
	_window			= new Window(this, 320, 200, 0, 0);
	_txtTitle		= new Text(300, 17, 16, 10);
	_txtBaseLabel	= new Text(80, 9, 16, 10);

	_txtAllocated	= new Text(140, 9, 16, 26);
	_txtAvailable	= new Text(140, 9, 16, 35);

	_txtSpace		= new Text(100, 9, 160, 26);

	_txtProject		= new Text(110, 9, 16, 48);
	_txtScientists	= new Text(55, 9, 161, 48);
	_txtProgress	= new Text(55, 9, 219, 48);

	_lstResearch	= new TextList(288, 113, 16, 63);

	_btnAliens		= new TextButton(92, 16, 16, 177);
	_btnNew			= new TextButton(92, 16, 114, 177);
	_btnOk			= new TextButton(92, 16, 212, 177);


	setPalette(
			"PAL_BASESCAPE",
			_game->getRuleset()->getInterface("researchMenu")->getElement("palette")->color); //1

	add(_window, "window", "researchMenu");
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


//	_window->setColor(Palette::blockOffset(13)+10);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK05.SCR"));

//	_btnAliens->setColor(Palette::blockOffset(15)+6);
	_btnAliens->setText(tr("STR_ALIENS"));
	_btnAliens->onMouseClick((ActionHandler)& ResearchState::btnAliens);
	_btnAliens->setVisible(false);

//	_btnNew->setColor(Palette::blockOffset(15)+6);
	_btnNew->setText(tr("STR_NEW_PROJECT"));
	_btnNew->onMouseClick((ActionHandler)& ResearchState::btnNewClick);

//	_btnOk->setColor(Palette::blockOffset(15)+6);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& ResearchState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& ResearchState::btnOkClick,
					Options::keyCancel);

//	_txtTitle->setColor(Palette::blockOffset(13)+10);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_CURRENT_RESEARCH"));

//	_txtBaseLabel->setColor(Palette::blockOffset(13)+10);
	_txtBaseLabel->setText(_base->getName(_game->getLanguage()));

//	_txtAvailable->setColor(Palette::blockOffset(13)+10);
//	_txtAvailable->setSecondaryColor(Palette::blockOffset(13));

//	_txtAllocated->setColor(Palette::blockOffset(13)+10);
//	_txtAllocated->setSecondaryColor(Palette::blockOffset(13));

//	_txtSpace->setColor(Palette::blockOffset(13)+10);
//	_txtSpace->setSecondaryColor(Palette::blockOffset(13));

//	_txtProject->setColor(Palette::blockOffset(13)+10);
	_txtProject->setText(tr("STR_RESEARCH_PROJECT"));

//	_txtScientists->setColor(Palette::blockOffset(13)+10);
	_txtScientists->setText(tr("STR_SCIENTISTS_ALLOCATED_UC"));

//	_txtProgress->setColor(Palette::blockOffset(13)+10);
	_txtProgress->setText(tr("STR_PROGRESS"));

//	_lstResearch->setColor(Palette::blockOffset(15)+6);
//	_lstResearch->setArrowColor(Palette::blockOffset(13)+10);
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

	const std::vector<ResearchProject*>& rps (_base->getResearch()); // init.

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

	_game->pushState(new ResearchInfoState(
										_base,
										rps[sel2]));
//										rps[_lstResearch->getSelectedRow()]));
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


		std::wostringstream assigned;
		assigned << (*rp)->getAssigned();

		const std::wstring project = tr((*rp)->getRules()->getName());
		std::wstring wsDaysLeft;

		if ((*rp)->getAssigned() > 0)
		{
			const int daysLeft = static_cast<int>(std::ceil(
									(static_cast<double>((*rp)->getCost() - (*rp)->getSpent()))
									/ static_cast<double>((*rp)->getAssigned())));
			wsDaysLeft = Text::formatNumber(daysLeft, L"", false);
		}
		else
			 wsDaysLeft = L"-";

		_lstResearch->addRow(
							4,
							project.c_str(),
							assigned.str().c_str(),
							tr((*rp)->getResearchProgress()).c_str(),
//							(*rp)->getCostCompleted().c_str());
							wsDaysLeft.c_str());
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
