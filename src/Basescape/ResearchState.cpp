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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ResearchState.h"

#include <sstream>

#include "ManageAlienContainmentState.h"
#include "NewResearchListState.h"
#include "ResearchInfoState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
#include "../Engine/Screen.h"

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
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 */
ResearchState::ResearchState(
		Game* game,
		Base* base)
	:
		State(game),
		_base(base)
{
	_window			= new Window(this, 320, 200, 0, 0);
	_txtTitle		= new Text(300, 17, 16, 9);
	_txtBaseLabel	= new Text(80, 9, 16, 9);

	_txtAllocated	= new Text(140, 9, 16, 25);
	_txtAvailable	= new Text(140, 9, 160, 25);

	_txtSpace		= new Text(100, 9, 16, 34);

	_txtProject		= new Text(110, 9, 16, 47);
	_txtScientists	= new Text(55, 9, 173, 47);
	_txtProgress	= new Text(55, 9, 232, 47);

	_lstResearch	= new TextList(285, 112, 16, 62);

	_btnAliens		= new TextButton(92, 16, 16, 177); // kL
	_btnNew			= new TextButton(92, 16, 114, 177);
	_btnOk			= new TextButton(92, 16, 212, 177);

	// back up palette in case we're being called from Geoscape!
	memcpy(
		_oldPalette,
		_game->getScreen()->getPalette(),
		256 * sizeof(SDL_Color));


	_game->setPalette(_game->getResourcePack()->getPalette("PALETTES.DAT_1")->getColors());
	_game->setPalette(
				_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(1)),
				Palette::backPos,
				16);

	add(_window);
	add(_txtTitle);
	add(_txtBaseLabel);
	add(_txtAvailable);
	add(_txtAllocated);
	add(_txtSpace);
	add(_txtProject);
	add(_txtScientists);
	add(_txtProgress);
	add(_lstResearch);
	add(_btnAliens); // kL
	add(_btnNew);
	add(_btnOk);

	centerAllSurfaces();


	_window->setColor(Palette::blockOffset(13)+10);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK05.SCR"));

	_btnAliens->setColor(Palette::blockOffset(15)+6);
	_btnAliens->setText(tr("STR_ALIENS"));
	_btnAliens->onMouseClick((ActionHandler)& ResearchState::btnAliens);

	_btnNew->setColor(Palette::blockOffset(15)+6);
	_btnNew->setText(tr("STR_NEW_PROJECT"));
	_btnNew->onMouseClick((ActionHandler)& ResearchState::btnNewClick);

	_btnOk->setColor(Palette::blockOffset(15)+6);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& ResearchState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& ResearchState::btnOkClick,
					(SDLKey)Options::getInt("keyCancel"));

	_txtTitle->setColor(Palette::blockOffset(13)+10);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_CURRENT_RESEARCH"));

	_txtBaseLabel->setColor(Palette::blockOffset(13)+10);
	_txtBaseLabel->setText(_base->getName(_game->getLanguage()));

	_txtAvailable->setColor(Palette::blockOffset(13)+10);
	_txtAvailable->setSecondaryColor(Palette::blockOffset(13));

	_txtAllocated->setColor(Palette::blockOffset(13)+10);
	_txtAllocated->setSecondaryColor(Palette::blockOffset(13));

	_txtSpace->setColor(Palette::blockOffset(13)+10);
	_txtSpace->setSecondaryColor(Palette::blockOffset(13));

	_txtProject->setColor(Palette::blockOffset(13)+10);
	_txtProject->setWordWrap(true);
	_txtProject->setText(tr("STR_RESEARCH_PROJECT"));

	_txtScientists->setColor(Palette::blockOffset(13)+10);
	_txtScientists->setWordWrap(true);
	_txtScientists->setText(tr("STR_SCIENTISTS_ALLOCATED_UC"));

	_txtProgress->setColor(Palette::blockOffset(13)+10);
	_txtProgress->setText(tr("STR_PROGRESS"));

	_lstResearch->setColor(Palette::blockOffset(15)+6);
	_lstResearch->setArrowColor(Palette::blockOffset(13)+10);
	_lstResearch->setColumns(4, 149, 59, 48, 17);
	_lstResearch->setSelectable(true);
	_lstResearch->setBackground(_window);
	_lstResearch->setMargin(8);
	_lstResearch->onMouseClick((ActionHandler)& ResearchState::onSelectProject);

	init();
}

/**
 *
 */
ResearchState::~ResearchState()
{
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void ResearchState::btnOkClick(Action*)
{
	_game->setPalette(_oldPalette);

	_game->popState();
}

/**
 * Pops up the NewResearchListState screen.
 * @param action Pointer to an action.
 */
void ResearchState::btnNewClick(Action*)
{
	_game->pushState(new NewResearchListState(
											_game,
											_base));
}

/**
 * Goes to the Manage Alien Containment screen.
 * @param action Pointer to an action.
 */
void ResearchState::btnAliens(Action*) // kL
{
	_game->pushState(new ManageAlienContainmentState(
													_game,
													_base,
													OPT_GEOSCAPE));
}

/**
 * Displays the list of possible ResearchProjects.
 * @param action Pointer to an action.
 */
void ResearchState::onSelectProject(Action*)
{
	const std::vector<ResearchProject*>& baseProjects(_base->getResearch());

	_game->pushState(new ResearchInfoState(
										_game,
										_base,
										baseProjects[_lstResearch->getSelectedRow()]));
}

/**
 * Updates the research list after going to other screens.
 */
void ResearchState::init()
{
	_lstResearch->clearList();

	const std::vector<ResearchProject*>& baseProjects(_base->getResearch());
	for (std::vector<ResearchProject*>::const_iterator
			i = baseProjects.begin();
			i != baseProjects.end();
			++i)
	{
		std::wstringstream sstr;
		sstr << (*i)->getAssigned();
		const RuleResearch* r = (*i)->getRules();

		std::wstring wstr = tr(r->getName());
		_lstResearch->addRow(
							4,
							wstr.c_str(),
							sstr.str().c_str(),
							tr((*i)->getResearchProgress()).c_str(),
							(*i)->getCostCompleted().c_str());
	}

	_txtAvailable->setText(tr("STR_SCIENTISTS_AVAILABLE")
							.arg(_base->getScientists()));
	_txtAllocated->setText(tr("STR_SCIENTISTS_ALLOCATED")
							.arg(_base->getAllocatedScientists()));
	_txtSpace->setText(tr("STR_LABORATORY_SPACE_AVAILABLE")
							.arg(_base->getFreeLaboratories()));

	// kL_begin:
	_btnAliens->setVisible(false);
	for (std::vector<BaseFacility*>::iterator
			i = _base->getFacilities()->begin();
			i != _base->getFacilities()->end();
			++i)
	{
		if ((*i)->getRules()->getAliens() > 0)
		{
			_btnAliens->setVisible(true);

			break;
		}
	} // kL_end.
}

}
