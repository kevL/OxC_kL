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

#include "MonthlyCostsState.h"

#include <sstream>

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h"
#include "../Savegame/SavedGame.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Monthly Costs screen.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 */
MonthlyCostsState::MonthlyCostsState(
		Game* game,
		Base* base)
	:
		State(game),
		_base(base)
{
	_window			= new Window(this, 320, 200, 0, 0);

	_txtTitle		= new Text(300, 17, 11, 10);
//	_txtBaseLabel	= new Text(80, 9, 224, 10);

	_txtCost		= new Text(80, 9, 141, 31);
	_txtQuantity	= new Text(55, 9, 211, 31);
	_txtTotal		= new Text(60, 9, 255, 31);

	_txtRental		= new Text(150, 9, 16, 46);
	_lstCrafts		= new TextList(293, 40, 16, 57);

	_txtSalaries	= new Text(150, 9, 16, 100);
	_lstSalaries	= new TextList(293, 27, 16, 111);

	_lstMaintenance	= new TextList(293, 9, 16, 142);

	_txtIncome		= new Text(150, 9, 16, 159);
	_lstTotal		= new TextList(98, 9, 206, 159);

	_btnOk			= new TextButton(288, 16, 16, 177);


	_game->setPalette(
				_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(6)),
				Palette::backPos,
				16);

	add(_window);
	add(_txtTitle);
//	add(_txtBaseLabel);
	add(_txtCost);
	add(_txtQuantity);
	add(_txtTotal);
	add(_txtRental);
	add(_lstCrafts);
	add(_txtSalaries);
	add(_lstSalaries);
	add(_txtIncome);
	add(_lstMaintenance);
	add(_lstTotal);
	add(_btnOk);

	centerAllSurfaces();


	_window->setColor(Palette::blockOffset(15)+1);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK13.SCR"));

	_btnOk->setColor(Palette::blockOffset(15)+1);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& MonthlyCostsState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& MonthlyCostsState::btnOkClick,
					(SDLKey)Options::getInt("keyOk"));
	_btnOk->onKeyboardPress(
					(ActionHandler)& MonthlyCostsState::btnOkClick,
					(SDLKey)Options::getInt("keyCancel"));

	_txtTitle->setColor(Palette::blockOffset(15)+1);
	_txtTitle->setBig();
//	_txtTitle->setText(tr("STR_MONTHLY_COSTS"));
	_txtTitle->setText(tr("STR_MONTHLY_COSTS_").arg(_base->getName()));

//	_txtBaseLabel->setColor(Palette::blockOffset(15)+1);
//	_txtBaseLabel->setAlign(ALIGN_RIGHT);
//	_txtBaseLabel->setText(_base->getName(_game->getLanguage()));

	_txtCost->setColor(Palette::blockOffset(15)+1);
	_txtCost->setText(tr("STR_COST_PER_UNIT"));

	_txtQuantity->setColor(Palette::blockOffset(15)+1);
	_txtQuantity->setText(tr("STR_QUANTITY"));

	_txtTotal->setColor(Palette::blockOffset(15)+1);
	_txtTotal->setText(tr("STR_TOTAL"));

	_txtRental->setColor(Palette::blockOffset(15)+1);
	_txtRental->setText(tr("STR_CRAFT_RENTAL"));

	_txtSalaries->setColor(Palette::blockOffset(15)+1);
	_txtSalaries->setText(tr("STR_SALARIES"));

	_txtIncome->setColor(Palette::blockOffset(13)+10);
	std::wstringstream ss;
	ss << tr("STR_INCOME") << L" " << Text::formatFunding(_game->getSavedGame()->getCountryFunding());
	_txtIncome->setText(ss.str());

	_lstCrafts->setColor(Palette::blockOffset(13)+10);
	_lstCrafts->setColumns(4, 125, 70, 44, 54);
	_lstCrafts->setDot(true);

	const std::vector<std::string>& crafts = _game->getRuleset()->getCraftsList();
	for (std::vector<std::string>::const_iterator
			i = crafts.begin();
			i != crafts.end();
			++i)
	{
		RuleCraft* craft = _game->getRuleset()->getCraft(*i);
		if (craft->getRentCost() > 0
			&& _game->getSavedGame()->isResearched(craft->getRequirements()))
		{
			std::wstringstream ss2;
			ss2 << _base->getCraftCount((*i));
			_lstCrafts->addRow(
							4,
							tr(*i).c_str(),
							Text::formatFunding(craft->getRentCost()).c_str(),
							ss2.str().c_str(),
							Text::formatFunding(_base->getCraftCount(*i)
												* craft->getRentCost()).c_str());
		}
	}

	_lstSalaries->setColor(Palette::blockOffset(13)+10);
	_lstSalaries->setColumns(4, 125, 70, 44, 54);
	_lstSalaries->setDot(true);

	std::wstringstream
		ss4,
		ss5,
		ss6;

	ss4 << _base->getSoldiers()->size();
	_lstSalaries->addRow(
						4,
						tr("STR_SOLDIERS").c_str(),
						Text::formatFunding(_game->getRuleset()->getSoldierCost()).c_str(),
						ss4.str().c_str(),
						Text::formatFunding(_base->getSoldiers()->size()
											* _game->getRuleset()->getSoldierCost()).c_str());

	ss5 << _base->getTotalEngineers();
	_lstSalaries->addRow(
						4,
						tr("STR_ENGINEERS").c_str(),
						Text::formatFunding(_game->getRuleset()->getEngineerCost()).c_str(),
						ss5.str().c_str(),
						Text::formatFunding(_base->getTotalEngineers()
											* _game->getRuleset()->getEngineerCost()).c_str());

	ss6 << _base->getTotalScientists();
	_lstSalaries->addRow(
						4,
						tr("STR_SCIENTISTS").c_str(),
						Text::formatFunding(_game->getRuleset()->getScientistCost()).c_str(),
						ss6.str().c_str(),
						Text::formatFunding(_base->getTotalScientists()
											* _game->getRuleset()->getScientistCost()).c_str());

	_lstMaintenance->setColor(Palette::blockOffset(13)+10);
	_lstMaintenance->setColumns(2, 239, 54);
	_lstMaintenance->setDot(true);
	_lstMaintenance->addRow(
						2,
						tr("STR_BASE_MAINTENANCE").c_str(),
						Text::formatFunding(_base->getFacilityMaintenance()).c_str());
	_lstMaintenance->setCellColor(0, 0, Palette::blockOffset(15)+1);

	_lstTotal->setColor(Palette::blockOffset(13));
	_lstTotal->setColumns(2, 44, 55);
	_lstTotal->setDot(true);
	_lstTotal->addRow(
					2,
					tr("STR_TOTAL").c_str(),
					Text::formatFunding(_base->getMonthlyMaintenace()).c_str());
}

/**
 *
 */
MonthlyCostsState::~MonthlyCostsState()
{
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void MonthlyCostsState::btnOkClick(Action*)
{
	_game->popState();
}

}
