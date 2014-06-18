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

#include "CraftArmorState.h"

#include <climits>
#include <string>

#include "SoldierArmorState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/Armor.h"
#include "../Ruleset/RuleCraft.h"

#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/Soldier.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Craft Armor screen.
 * @param game, Pointer to the core game.
 * @param base, Pointer to the base to get info from.
 * @param craft, ID of the selected craft.
 */
CraftArmorState::CraftArmorState(
		Game* game,
		Base* base,
		size_t craft)
	:
		State(game),
		_base(base),
		_craft(craft)
{
	//Log(LOG_INFO) << "Create CraftArmorState";

	_window			= new Window(this, 320, 200, 0, 0);
	_txtTitle		= new Text(300, 17, 11, 10);
	_txtBaseLabel	= new Text(80, 9, 224, 10);

	_txtName		= new Text(114, 9, 16, 31);
	_txtArmor		= new Text(76, 9, 133, 31);
	_txtCraft		= new Text(70, 9, 226, 31);

	_lstSoldiers	= new TextList(293, 128, 8, 42);

	_btnOk			= new TextButton(288, 16, 16, 177);

	setPalette("PAL_BASESCAPE", 4);

	add(_window);
	add(_txtTitle);
	add(_txtBaseLabel);
	add(_txtName);
	add(_txtArmor);
	add(_txtCraft);
	add(_lstSoldiers);
	add(_btnOk);

	centerAllSurfaces();


	_window->setColor(Palette::blockOffset(13)+10);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK14.SCR"));

	_btnOk->setColor(Palette::blockOffset(13)+10);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& CraftArmorState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& CraftArmorState::btnOkClick,
					Options::keyCancel);

	_txtTitle->setColor(Palette::blockOffset(13)+10);
	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_SELECT_ARMOR"));

	_txtBaseLabel->setColor(Palette::blockOffset(13)+10);
	_txtBaseLabel->setAlign(ALIGN_RIGHT);
	_txtBaseLabel->setText(_base->getName(_game->getLanguage()));

	_txtName->setColor(Palette::blockOffset(13)+10);
	_txtName->setText(tr("STR_NAME_UC"));

	_txtCraft->setColor(Palette::blockOffset(13)+10);
	_txtCraft->setText(tr("STR_CRAFT"));

	_txtArmor->setColor(Palette::blockOffset(13)+10);
	_txtArmor->setText(tr("STR_ARMOR"));

	_lstSoldiers->setColor(Palette::blockOffset(13)+10);
	_lstSoldiers->setArrowColor(Palette::blockOffset(13)+10);
	_lstSoldiers->setArrowColumn(193, ARROW_VERTICAL);
	_lstSoldiers->setColumns(3, 117, 93, 73);
	_lstSoldiers->setSelectable(true);
	_lstSoldiers->setBackground(_window);
	_lstSoldiers->setMargin(8);
	_lstSoldiers->onLeftArrowClick((ActionHandler)& CraftArmorState::lstLeftArrowClick);
	_lstSoldiers->onRightArrowClick((ActionHandler)& CraftArmorState::lstRightArrowClick);
//kL	_lstSoldiers->onMouseClick((ActionHandler)& CraftArmorState::lstSoldiersClick, 0);
	_lstSoldiers->onMouseClick((ActionHandler)& CraftArmorState::lstSoldiersClick); // kL


//kL	Craft* c = _base->getCrafts()->at(_craft);
	Craft* c = 0;										// kL
	bool hasCraft = _base->getCrafts()->size() > 0;		// kL
	if (hasCraft)										// kL -> KLUDGE!!!
		c = _base->getCrafts()->at(_craft);				// kL: This is always 0 (1st craft) when coming from SoldiersState.

	int row = 0;
	for (std::vector<Soldier*>::iterator
			i = _base->getSoldiers()->begin();
			i != _base->getSoldiers()->end();
			++i)
	{
		//Log(LOG_INFO) << "CraftArmorState::CraftArmorState() iterate soldiers to createList";
		_lstSoldiers->addRow(
							3,
							(*i)->getName(true).c_str(),
							tr((*i)->getArmor()->getType()).c_str(),
							(*i)->getCraftString(_game->getLanguage()).c_str());

		//Log(LOG_INFO) << ". . added row " << *i;

		Uint8 color;
		if (!hasCraft)
			//Log(LOG_INFO) << ". . . . color, Base has NO craft";
			color = Palette::blockOffset(13)+10;
		else if ((*i)->getCraft() == c)
			//Log(LOG_INFO) << ". . . . color, soldier is on 'this' craft";
			color = Palette::blockOffset(13);
		else if ((*i)->getCraft() != 0)
			//Log(LOG_INFO) << ". . . . color, soldier is on another craft";
			color = Palette::blockOffset(15)+6;
		else // craft==0
			//Log(LOG_INFO) << ". . . . color, soldier is not on a craft";
			color = Palette::blockOffset(13)+10;

		_lstSoldiers->setRowColor(row, color);

		row++;
	}
	//Log(LOG_INFO) << "CraftArmorState::CraftArmorState() EXIT";
}

/**
 *
 */
CraftArmorState::~CraftArmorState()
{
	//Log(LOG_INFO) << "Delete CraftArmorState";
}

/**
 * The soldier armors can change after going into other screens.
 */
void CraftArmorState::init()
{
	State::init();

	_lstSoldiers->clearList(); // kL

	// kL_begin: init Armor list, from cTor
	Craft* c = 0;
	bool hasCraft = _base->getCrafts()->size() > 0;
	if (hasCraft)
		c = _base->getCrafts()->at(_craft);
	// kL_end.

	int row = 0;
	for (std::vector<Soldier*>::iterator
			i = _base->getSoldiers()->begin();
			i != _base->getSoldiers()->end();
			++i)
	{
		_lstSoldiers->addRow(
							3,
							(*i)->getName().c_str(),
							tr((*i)->getArmor()->getType()).c_str(),
							(*i)->getCraftString(_game->getLanguage()).c_str());

		// kL_begin: init Armor list, from cTor
		Uint8 color;
		if (!hasCraft)
			//Log(LOG_INFO) << ". . . . color, Base has NO craft";
			color = Palette::blockOffset(13)+10;
		else if ((*i)->getCraft() == c)
			//Log(LOG_INFO) << ". . . . color, soldier is on 'this' craft";
			color = Palette::blockOffset(13);
		else if ((*i)->getCraft() != 0)
			//Log(LOG_INFO) << ". . . . color, soldier is on another craft";
			color = Palette::blockOffset(15)+6;
		else // craft==0
			//Log(LOG_INFO) << ". . . . color, soldier is not on a craft";
			color = Palette::blockOffset(13)+10;

		_lstSoldiers->setRowColor(row, color);
		// kL_end.

		row++;
	}

	_lstSoldiers->draw();
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void CraftArmorState::btnOkClick(Action*)
{
	_game->popState();
}

/**
 * Shows the Select Armor window.
 * @param action Pointer to an action.
 */
// void CraftArmorState::lstSoldiersClick(Action*)
void CraftArmorState::lstSoldiersClick(Action* action) // kL
{
	// kL_begin:
	double mx = action->getAbsoluteXMouse();
	if (mx >= static_cast<double>(_lstSoldiers->getArrowsLeftEdge())
		&& mx < static_cast<double>(_lstSoldiers->getArrowsRightEdge()))
	{
		return;
	} // kL_end.

	Soldier* soldier = _base->getSoldiers()->at(_lstSoldiers->getSelectedRow());
	if (!
		(soldier->getCraft()
			&& soldier->getCraft()->getStatus() == "STR_OUT"))
	{
		_game->pushState(new SoldierArmorState(
											_game,
											_base,
											_lstSoldiers->getSelectedRow()));
	}
}

/**
 * kL. Reorders a soldier up.
 * @param action Pointer to an action.
 */
void CraftArmorState::lstLeftArrowClick(Action* action) // kL
{
	int row = _lstSoldiers->getSelectedRow();
	if (row > 0)
	{
		Soldier* soldier = _base->getSoldiers()->at(row);

		if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		{
			_base->getSoldiers()->at(row) = _base->getSoldiers()->at(row - 1);
			_base->getSoldiers()->at(row - 1) = soldier;

			if (row != static_cast<int>(_lstSoldiers->getScroll()))
			{
				SDL_WarpMouse(
						static_cast<Uint16>(action->getLeftBlackBand() + action->getXMouse()),
						static_cast<Uint16>(action->getTopBlackBand() + action->getYMouse() - static_cast<int>(8.0 * action->getYScale())));
			}
			else
				_lstSoldiers->scrollUp(false);
		}
		else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		{
			_base->getSoldiers()->erase(_base->getSoldiers()->begin() + row);
			_base->getSoldiers()->insert(
									_base->getSoldiers()->begin(),
									soldier);
		}
	}

	init();
}

/**
 * kL. Reorders a soldier down.
 * @param action Pointer to an action.
 */
void CraftArmorState::lstRightArrowClick(Action* action) // kL
{
	int row = _lstSoldiers->getSelectedRow();
	size_t numSoldiers = _base->getSoldiers()->size();
	if (numSoldiers > 0
		&& numSoldiers <= INT_MAX
		&& row < static_cast<int>(numSoldiers) - 1)
	{
		Soldier* soldier = _base->getSoldiers()->at(row);

		if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		{
			_base->getSoldiers()->at(row) = _base->getSoldiers()->at(row + 1);
			_base->getSoldiers()->at(row + 1) = soldier;

			if (row != static_cast<int>(_lstSoldiers->getVisibleRows() - 1 + _lstSoldiers->getScroll()))
			{
				SDL_WarpMouse(
						static_cast<Uint16>(action->getLeftBlackBand() + action->getXMouse()),
						static_cast<Uint16>(action->getTopBlackBand() + action->getYMouse() + static_cast<int>(8.0 * action->getYScale())));
			}
			else
				_lstSoldiers->scrollDown(false);
		}
		else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		{
			_base->getSoldiers()->erase(_base->getSoldiers()->begin() + row);
			_base->getSoldiers()->insert(
									_base->getSoldiers()->end(),
									soldier);
		}
	}

	init();
}

}
