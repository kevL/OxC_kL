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

#include "ManufactureState.h"

#include <limits>
#include <sstream>

#include "ManufactureInfoState.h"
#include "NewManufactureListState.h"

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
#include "../Engine/Screen.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleManufacture.h"
#include "../Ruleset/Ruleset.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Savegame/Base.h"
#include "../Savegame/Production.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Manufacture screen.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 */
ManufactureState::ManufactureState(
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

	_txtSpace		= new Text(140, 9, 16, 34);
	_txtFunds		= new Text(140, 9, 160, 34);

	_txtItem		= new Text(120, 9, 16, 52);
	_txtEngineers	= new Text(45, 9, 145, 52);
	_txtProduced	= new Text(40, 9, 174, 44);
	_txtCost		= new Text(50, 17, 215, 44);
	_txtTimeLeft	= new Text(25, 17, 271, 44);

	_lstManufacture	= new TextList(285, 96, 16, 70);

	_btnNew			= new TextButton(134, 16, 16, 177);
	_btnOk			= new TextButton(134, 16, 170, 177);


	// back up palette in case we're being called from Geoscape!
	memcpy(
		_oldPalette,
		_game->getScreen()->getPalette(),
		256 * sizeof(SDL_Color));

	_game->setPalette(_game->getResourcePack()->getPalette("PALETTES.DAT_1")->getColors());
	_game->setPalette(
				_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(6)),
				Palette::backPos,
				16);

	add(_window);
	add(_txtTitle);
	add(_txtBaseLabel);
	add(_txtAvailable);
	add(_txtAllocated);
	add(_txtSpace);
	add(_txtFunds);
	add(_txtItem);
	add(_txtEngineers);
	add(_txtProduced);
	add(_txtCost);
	add(_txtTimeLeft);
	add(_lstManufacture);
	add(_btnNew);
	add(_btnOk);

	centerAllSurfaces();


	_window->setColor(Palette::blockOffset(15)+6);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK17.SCR"));

	_btnNew->setColor(Palette::blockOffset(13)+10);
	_btnNew->setText(tr("STR_NEW_PRODUCTION"));
	_btnNew->onMouseClick((ActionHandler)& ManufactureState::btnNewProductionClick);

	_btnOk->setColor(Palette::blockOffset(13)+10);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)&ManufactureState::btnOkClick);
	_btnOk->onKeyboardPress(
						(ActionHandler)& ManufactureState::btnOkClick,
						(SDLKey)Options::getInt("keyCancel"));

	_txtTitle->setColor(Palette::blockOffset(15)+6);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_CURRENT_PRODUCTION"));

	_txtBaseLabel->setColor(Palette::blockOffset(15)+6);
	_txtBaseLabel->setText(_base->getName(_game->getLanguage()));

	_txtAvailable->setColor(Palette::blockOffset(15)+6);
	_txtAvailable->setSecondaryColor(Palette::blockOffset(13));

	_txtAllocated->setColor(Palette::blockOffset(15)+6);
	_txtAllocated->setSecondaryColor(Palette::blockOffset(13));

	_txtSpace->setColor(Palette::blockOffset(15)+6);
	_txtSpace->setSecondaryColor(Palette::blockOffset(13));

	_txtFunds->setColor(Palette::blockOffset(15)+6);
	_txtFunds->setSecondaryColor(Palette::blockOffset(13));
	_txtFunds->setText(tr("STR_CURRENT_FUNDS")
						.arg(Text::formatFunding(_game->getSavedGame()->getFunds())));

	_txtItem->setColor(Palette::blockOffset(15)+1);
	_txtItem->setText(tr("STR_ITEM"));

	_txtEngineers->setColor(Palette::blockOffset(15)+1);
	_txtEngineers->setText(tr("STR_ENGINEERS__ALLOCATED"));
	_txtEngineers->setWordWrap(true);
	_txtEngineers->setVerticalAlign(ALIGN_BOTTOM);

	_txtProduced->setColor(Palette::blockOffset(15)+1);
	_txtProduced->setText(tr("STR_UNITS_PRODUCED"));
	_txtProduced->setWordWrap(true);
	_txtProduced->setVerticalAlign(ALIGN_BOTTOM);

	_txtCost->setColor(Palette::blockOffset(15)+1);
	_txtCost->setText(tr("STR_COST__PER__UNIT"));
	_txtCost->setWordWrap(true);
	_txtCost->setVerticalAlign(ALIGN_BOTTOM);

	_txtTimeLeft->setColor(Palette::blockOffset(15)+1);
	_txtTimeLeft->setText(tr("STR_DAYS_HOURS_LEFT"));
	_txtTimeLeft->setWordWrap(true);
	_txtTimeLeft->setVerticalAlign(ALIGN_BOTTOM);

	_lstManufacture->setColor(Palette::blockOffset(13)+10);
	_lstManufacture->setArrowColor(Palette::blockOffset(15)+9);
	_lstManufacture->setColumns(5, 121, 29, 41, 56, 30);
//kL	_lstManufacture->setAlign(ALIGN_RIGHT);
//kL	_lstManufacture->setAlign(ALIGN_LEFT, 0);
	_lstManufacture->setSelectable(true);
	_lstManufacture->setBackground(_window);
	_lstManufacture->setMargin(8);
	_lstManufacture->onMouseClick((ActionHandler)& ManufactureState::lstManufactureClick);

	fillProductionList();
}

/**
 *
 */
ManufactureState::~ManufactureState()
{
}

/**
 * Updates the production list
 * after going to other screens.
 */
void ManufactureState::init()
{
	fillProductionList();
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void ManufactureState::btnOkClick(Action*)
{
	// restore palette
	_game->setPalette(_oldPalette);

	_game->popState();
}

/**
 * Opens the screen with the list of possible productions.
 * @param action Pointer to an action.
 */
void ManufactureState::btnNewProductionClick(Action*)
{
	_game->pushState(new NewManufactureListState(
												_game,
												_base));
}

/**
 * Fills the list of base productions.
 */
void ManufactureState::fillProductionList()
{
	_lstManufacture->clearList();

	const std::vector<Production*> productions(_base->getProductions());
	for(std::vector<Production*>::const_iterator
			i = productions.begin();
			i != productions.end();
			++i)
	{
		std::wstringstream
			s1,
			s2,
			s3;

		s1 << (*i)->getAssignedEngineers();

		s2 << (*i)->getAmountProduced() << "/";
		if (Options::getBool("allowAutoSellProduction")
			&& (*i)->getAmountTotal() == std::numeric_limits<int>::max())
		{
			s2 << "$$";
		}
		else
			s2 << (*i)->getAmountTotal();

		s3 << Text::formatFunding((*i)->getRules()->getManufactureCost());

		std::wstringstream s4;
		if ((*i)->getAssignedEngineers() > 0)
		{
			int timeLeft;
			if (Options::getBool("allowAutoSellProduction")
				&& (*i)->getAmountTotal() == std::numeric_limits<int>::max())
			{
				timeLeft = ((*i)->getAmountProduced() + 1) * (*i)->getRules()->getManufactureTime()
								- (*i)->getTimeSpent();
			}
			else
			{
				timeLeft = (*i)->getAmountTotal() * (*i)->getRules()->getManufactureTime()
								- (*i)->getTimeSpent();
			}

			timeLeft /= (*i)->getAssignedEngineers();
//kL			float dayLeft = static_cast<float>(timeLeft) / 24.f;	// <- too clever by half.
			int daysLeft = timeLeft / 24;
//kL			int hours = (dayLeft - static_cast<int>(dayLeft)) * 24;	// <- too clever by half.
			int hoursLeft = timeLeft %24;
//kL			s4 << static_cast<int>(daysLeft) << "/" << hoursLeft;	// <- too clever by half.
			s4 << daysLeft << "/" << hoursLeft;
		}
		else
		{
			s4 << L"-";
		}

		_lstManufacture->addRow
							(5,
							tr((*i)->getRules()->getName()).c_str(),
							s1.str().c_str(),
							s2.str().c_str(),
							s3.str().c_str(),
							s4.str().c_str());
	}

	_txtAvailable->setText(tr("STR_ENGINEERS_AVAILABLE")
//kL							.arg(_base->getAvailableEngineers()));
							.arg(_base->getEngineers())); // kL
	_txtAllocated->setText(tr("STR_ENGINEERS_ALLOCATED")
							.arg(_base->getAllocatedEngineers()));
	_txtSpace->setText(tr("STR_WORKSHOP_SPACE_AVAILABLE")
							.arg(_base->getFreeWorkshops()));
}

/**
 * Opens the screen displaying production settings.
 * @param action Pointer to an action.
 */
void ManufactureState::lstManufactureClick(Action*)
{
	const std::vector<Production*> productions(_base->getProductions());
	_game->pushState(new ManufactureInfoState(
											_game,
											_base,
											productions[_lstManufacture->getSelectedRow()]));
}

}
