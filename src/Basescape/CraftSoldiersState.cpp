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

#include "CraftSoldiersState.h"

//#include <climits>
//#include <sstream>
//#include <string>

#include "SoldierInfoState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Logger.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"

#include "../Resource/ResourcePack.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Ruleset/RuleCraft.h"

#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/Soldier.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Craft Soldiers screen.
 * @param base	- pointer to the Base to get info from
 * @param craft	- ID of the selected craft
 */
CraftSoldiersState::CraftSoldiersState(
		Base* base,
		size_t craftID)
	:
		_base(base),
		_craftID(craftID)
{
	_window			= new Window(this, 320, 200, 0, 0);

	_txtTitle		= new Text(300, 17, 16, 8);
	_txtBaseLabel	= new Text(80, 9, 224, 8);

//	_txtUsed		= new Text(110, 9, 16, 24);
//	_txtAvailable	= new Text(110, 9, 160, 24);
	_txtSpace		= new Text(110, 9, 16, 24);
	_txtLoad		= new Text(110, 9, 171, 24);

	_txtName		= new Text(116, 9, 16, 33);
	_txtRank		= new Text(93, 9, 140, 33);
	_txtCraft		= new Text(71, 9, 225, 33);

	_lstSoldiers	= new TextList(285, 129, 16, 42);

	_btnUnload		= new TextButton(134, 16, 16, 177);
	_btnOk			= new TextButton(134, 16, 170, 177);

	setPalette("PAL_BASESCAPE", 2);

	add(_window);
	add(_txtTitle);
	add(_txtBaseLabel);
//	add(_txtUsed);
//	add(_txtAvailable);
	add(_txtSpace);
	add(_txtLoad);
	add(_txtName);
	add(_txtRank);
	add(_txtCraft);
	add(_lstSoldiers);
	add(_btnUnload);
	add(_btnOk);

	centerAllSurfaces();


	_window->setColor(Palette::blockOffset(15)+6);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK02.SCR"));

	_btnOk->setColor(Palette::blockOffset(13)+10);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& CraftSoldiersState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& CraftSoldiersState::btnOkClick,
					Options::keyCancel);

	_btnUnload->setColor(Palette::blockOffset(13)+10);
	_btnUnload->setText(_game->getLanguage()->getString("STR_UNLOAD_CRAFT"));
	_btnUnload->onMouseClick((ActionHandler)& CraftSoldiersState::btnUnloadClick);

	_txtTitle->setColor(Palette::blockOffset(15)+6);
	_txtTitle->setBig();

	_txtBaseLabel->setColor(Palette::blockOffset(15)+6);
	_txtBaseLabel->setAlign(ALIGN_RIGHT);
	_txtBaseLabel->setText(_base->getName(_game->getLanguage()));

	const Craft* const craft = _base->getCrafts()->at(_craftID);
	_txtTitle->setText(tr("STR_SELECT_SQUAD_FOR_CRAFT").arg(craft->getName(_game->getLanguage())));

	_txtName->setColor(Palette::blockOffset(15)+6);
	_txtName->setText(tr("STR_NAME_UC"));

	_txtRank->setColor(Palette::blockOffset(15)+6);
	_txtRank->setText(tr("STR_RANK"));

	_txtCraft->setColor(Palette::blockOffset(15)+6);
	_txtCraft->setText(tr("STR_CRAFT"));

//	_txtUsed->setColor(Palette::blockOffset(15)+6);
//	_txtUsed->setSecondaryColor(Palette::blockOffset(13));

//	_txtAvailable->setColor(Palette::blockOffset(15)+6);
//	_txtAvailable->setSecondaryColor(Palette::blockOffset(13));

	_txtSpace->setColor(Palette::blockOffset(15)+6);
	_txtSpace->setSecondaryColor(Palette::blockOffset(13));

	_txtLoad->setColor(Palette::blockOffset(15)+6);
	_txtLoad->setSecondaryColor(Palette::blockOffset(13));

	_lstSoldiers->setColor(Palette::blockOffset(13)+10);
	_lstSoldiers->setArrowColor(Palette::blockOffset(15)+6);
	_lstSoldiers->setArrowColumn(180, ARROW_VERTICAL);
	_lstSoldiers->setColumns(3, 116, 85, 71);
	_lstSoldiers->setSelectable();
	_lstSoldiers->setBackground(_window);
	_lstSoldiers->setMargin();
//	_lstSoldiers->onMousePress((ActionHandler)& CraftSoldiersState::lstSoldiersMousePress);
	_lstSoldiers->onMousePress((ActionHandler)& CraftSoldiersState::lstSoldiersPress);
	_lstSoldiers->onLeftArrowClick((ActionHandler)& CraftSoldiersState::lstLeftArrowClick);
	_lstSoldiers->onRightArrowClick((ActionHandler)& CraftSoldiersState::lstRightArrowClick);
}

/**
 * dTor.
 */
CraftSoldiersState::~CraftSoldiersState()
{
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void CraftSoldiersState::btnOkClick(Action*)
{
	_base->setCurrentSoldier(_lstSoldiers->getScroll());
	_game->popState();
}

/**
 * Unloads all soldiers from current transport craft.
 * @param action - pointer to an Action
 */
void CraftSoldiersState::btnUnloadClick(Action*)
{
	const Craft* const craft = _base->getCrafts()->at(_craftID);

	for (std::vector<Soldier*>::iterator // iterate over all soldiers at Base
			i = _base->getSoldiers()->begin();
			i != _base->getSoldiers()->end();
			++i)
	{
		if ((*i)->getCraft() == craft) // if soldier is on this Craft, unload them
			(*i)->setCraft(NULL);
	}

	_base->setCurrentSoldier(_lstSoldiers->getScroll());

	init(); // iterate over all listRows and change their stringText and lineColor
}

/**
 * Shows the soldiers in a list at specified offset/scroll.
 */
/* void CraftSoldiersState::initList(size_t scrl)
{
	int row = 0;
	_lstSoldiers->clearList();
	Craft *c = _base->getCrafts()->at(_craft);
	for (std::vector<Soldier*>::iterator i = _base->getSoldiers()->begin(); i != _base->getSoldiers()->end(); ++i)
	{
		_lstSoldiers->addRow(3, (*i)->getName(true, 19).c_str(), tr((*i)->getRankString()).c_str(), (*i)->getCraftString(_game->getLanguage()).c_str());

		Uint8 color;
		if ((*i)->getCraft() == c)
		{
			color = Palette::blockOffset(13);
		}
		else if ((*i)->getCraft() != 0)
		{
			color = Palette::blockOffset(15)+6;
		}
		else
		{
			color = Palette::blockOffset(13)+10;
		}
		_lstSoldiers->setRowColor(row, color);
		row++;
	}
	if (scrl)
		_lstSoldiers->scrollTo(scrl);
	_lstSoldiers->draw();

	_txtAvailable->setText(tr("STR_SPACE_AVAILABLE").arg(c->getSpaceAvailable()));
	_txtUsed->setText(tr("STR_SPACE_USED").arg(c->getSpaceUsed()));
} */

/**
 * Shows the soldiers in a list.
 */
/* void CraftSoldiersState::init()
{
	State::init();
	initList(0);
} */

/**
 * Shows the soldiers in a list.
 */
void CraftSoldiersState::init()
{
	State::init();

	_lstSoldiers->clearList();

	Craft* const craft = _base->getCrafts()->at(_craftID);

	size_t row = 0;

	for (std::vector<Soldier*>::iterator
			i = _base->getSoldiers()->begin();
			i != _base->getSoldiers()->end();
			++i,
				++row)
	{
		_lstSoldiers->addRow(
							3,
							(*i)->getName(true, 19).c_str(),
							tr((*i)->getRankString()).c_str(),
							(*i)->getCraftString(_game->getLanguage()).c_str());

		Uint8 color;

		if ((*i)->getCraft() == craft)
			color = Palette::blockOffset(13);
		else if ((*i)->getCraft() != NULL)
			color = Palette::blockOffset(15)+6;
		else
			color = Palette::blockOffset(13)+10;

		_lstSoldiers->setRowColor(row, color);

		if ((*i)->getWoundRecovery() > 0)
		{
			Uint8 color = Palette::blockOffset(3); // green
			const int woundPct = (*i)->getWoundPercent();
			if (woundPct > 50)
				color = Palette::blockOffset(6); // orange
			else if (woundPct > 10)
				color = Palette::blockOffset(9); // yellow

			_lstSoldiers->setCellColor(
									row,
									2,
									color);
			_lstSoldiers->setCellHighContrast(
									row,
									2);
		}
	}

	_lstSoldiers->scrollTo(_base->getCurrentSoldier());
/*	if (row > 0) // all taken care of in TextList
	{
		if (_lstSoldiers->getScroll() > row
			|| _base->getCurrentSoldier() > row)
		{
			_lstSoldiers->scrollTo(0);
			_base->setCurrentSoldier(0);
		}
		else if (_base->getCurrentSoldier() > 0)
			_lstSoldiers->scrollTo(_base->getCurrentSoldier());
	} */

	_lstSoldiers->draw();

//	_txtAvailable->setText(tr("STR_SPACE_AVAILABLE").arg(craft->getSpaceAvailable()));
//	_txtUsed->setText(tr("STR_SPACE_USED").arg(craft->getSpaceUsed()));
	_txtSpace->setText(tr("STR_SPACE_USED_FREE_")
					.arg(craft->getSpaceUsed())
					.arg(craft->getSpaceAvailable()));
	_txtLoad->setText(tr("STR_LOAD_CAPACITY_FREE_")
					.arg(craft->getLoadCapacity())
					.arg(craft->getLoadCapacity() - craft->getLoadCurrent()));
}

/**
 * Reorders a soldier up.
 * @param action - pointer to an Action
 */
void CraftSoldiersState::lstLeftArrowClick(Action* action)
{
/*	size_t row = _lstSoldiers->getSelectedRow();
	if (row > 0)
	{
		if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
			moveSoldierUp(action, row);
		else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
			moveSoldierUp(action, row, true);
	} */
	_base->setCurrentSoldier(_lstSoldiers->getScroll());

	const size_t row = _lstSoldiers->getSelectedRow();

/*	if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
		_lstSoldiers->scrollUp(false, true);
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
		_lstSoldiers->scrollDown(false, true);
	else */
	if (row > 0)
	{
		Soldier* const soldier = _base->getSoldiers()->at(row);

		if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		{
			_base->getSoldiers()->at(row) = _base->getSoldiers()->at(row - 1);
			_base->getSoldiers()->at(row - 1) = soldier;

			if (row != _lstSoldiers->getScroll())
			{
				SDL_WarpMouse(
						static_cast<Uint16>(action->getLeftBlackBand() + action->getXMouse()),
						static_cast<Uint16>(action->getTopBlackBand() + action->getYMouse() - static_cast<int>(8.0 * action->getYScale())));
			}
			else
			{
				_base->setCurrentSoldier(_lstSoldiers->getScroll() - 1);
				_lstSoldiers->scrollUp(false);
			}
		}
		else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		{
			_base->setCurrentSoldier(_lstSoldiers->getScroll() + 1);

			_base->getSoldiers()->erase(_base->getSoldiers()->begin() + row);
			_base->getSoldiers()->insert(
									_base->getSoldiers()->begin(),
									soldier);
		}

		init();
	}
}

/**
 * Moves a soldier up on the list.
 * @param action - pointer to an Action
 * @param row Selected soldier row.
 * @param max Move the soldier to the top?
 */
/*kL void CraftSoldiersState::moveSoldierUp(Action* action, size_t row, bool max)
{
	Soldier* soldier = _base->getSoldiers()->at(row);
	if (max)
	{
		_base->getSoldiers()->erase(_base->getSoldiers()->begin() + row);
		_base->getSoldiers()->insert(_base->getSoldiers()->begin(), soldier);

		initList(0);
	}
	else
	{
		_base->getSoldiers()->at(row) = _base->getSoldiers()->at(row - 1);
		_base->getSoldiers()->at(row - 1) = soldier;
		if (row != _lstSoldiers->getScroll())
		{
			SDL_WarpMouse(
						action->getLeftBlackBand() + action->getXMouse(),
						action->getTopBlackBand() + action->getYMouse() - static_cast<Uint16>(8 * action->getYScale()));
		}
		else
			_lstSoldiers->scrollUp(false);

		initList(_lstSoldiers->getScroll());
	}
} */

/**
 * Reorders a soldier down.
 * @param action - pointer to an Action
 */
void CraftSoldiersState::lstRightArrowClick(Action* action)
{
/*	size_t row = _lstSoldiers->getSelectedRow();
	size_t numSoldiers = _base->getSoldiers()->size();
	if (0 < numSoldiers && std::numeric_limits<int>::max() >= numSoldiers && row < (int)numSoldiers - 1)
	{
		if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
			moveSoldierDown(action, row);
		else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
			moveSoldierDown(action, row, true);
	} */
	_base->setCurrentSoldier(_lstSoldiers->getScroll());

	const size_t
		numSoldiers = _base->getSoldiers()->size(),
		row = _lstSoldiers->getSelectedRow();

/*	if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP)
		_lstSoldiers->scrollUp(false, true);
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN)
		_lstSoldiers->scrollDown(false, true);
	else */
	if (numSoldiers > 0
		&& numSoldiers <= std::numeric_limits<size_t>::max()
		&& row < numSoldiers - 1)
	{
		Soldier* const soldier = _base->getSoldiers()->at(row);

		if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		{
			_base->getSoldiers()->at(row) = _base->getSoldiers()->at(row + 1);
			_base->getSoldiers()->at(row + 1) = soldier;

			if (row != _lstSoldiers->getVisibleRows() - 1 + _lstSoldiers->getScroll())
			{
				SDL_WarpMouse(
						static_cast<Uint16>(action->getLeftBlackBand() + action->getXMouse()),
						static_cast<Uint16>(action->getTopBlackBand() + action->getYMouse() + static_cast<int>(8.0 * action->getYScale())));
			}
			else
			{
				_base->setCurrentSoldier(_lstSoldiers->getScroll() + 1);
				_lstSoldiers->scrollDown(false);
			}
		}
		else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		{
			_base->getSoldiers()->erase(_base->getSoldiers()->begin() + row);
			_base->getSoldiers()->insert(
									_base->getSoldiers()->end(),
									soldier);
		}

		init();
	}
}

/**
 * Moves a soldier down on the list.
 * @param action	- pointer to an Action
 * @param row		- selected soldier row
 * @param max		- true to move the soldier to the bottom
 */
/*kL void CraftSoldiersState::moveSoldierDown(Action* action, size_t row, bool max)
{
	Soldier* soldier = _base->getSoldiers()->at(row);
	if (max)
	{
		_base->getSoldiers()->erase(_base->getSoldiers()->begin() + row);
		_base->getSoldiers()->insert(_base->getSoldiers()->end(), soldier);

		initList(std::max(0, (int)(_lstSoldiers->getRows() - _lstSoldiers->getVisibleRows())));
	}
	else
	{
		_base->getSoldiers()->at(row) = _base->getSoldiers()->at(row + 1);
		_base->getSoldiers()->at(row + 1) = soldier;
		if (row != _lstSoldiers->getVisibleRows() - 1 + _lstSoldiers->getScroll())
		{
			SDL_WarpMouse(
						action->getLeftBlackBand() + action->getXMouse(),
						action->getTopBlackBand() + action->getYMouse() + static_cast<Uint16>(8 * action->getYScale()));
		}
		else
			_lstSoldiers->scrollDown(false);

		initList(_lstSoldiers->getScroll());
	}
} */

/**
 * LMB assigns and de-assigns soldiers from a craft. RMB shows soldier info.
 * @param action - pointer to an Action
 */
void CraftSoldiersState::lstSoldiersPress(Action* action)
{
	const double mx = action->getAbsoluteXMouse();
	if (mx >= static_cast<double>(_lstSoldiers->getArrowsLeftEdge())
		&& mx < static_cast<double>(_lstSoldiers->getArrowsRightEdge()))
	{
		return;
	}

	const size_t row = _lstSoldiers->getSelectedRow();
	//Log(LOG_INFO) << ". CraftSoldiersState::lstSoldiersClick() row = " << row;

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		Soldier* const soldier = _base->getSoldiers()->at(_lstSoldiers->getSelectedRow());

		if (soldier->getWoundRecovery() > 0)
			return;

		Craft* const craft = _base->getCrafts()->at(_craftID);
		Uint8 color = Palette::blockOffset(13)+10;

		if (soldier->getCraft() == craft
			|| (soldier->getCraft() != NULL
				&& soldier->getCraft()->getStatus() != "STR_OUT"))
		{
			soldier->setCraft(NULL);
			_lstSoldiers->setCellText(row, 2, tr("STR_NONE_UC"));
		}
		else if (soldier->getCraft() == NULL
			&& craft->getSpaceAvailable() > 0
			&& craft->getLoadCapacity() - craft->getLoadCurrent() > 9)
		{
			soldier->setCraft(craft);
			_lstSoldiers->setCellText(row, 2, craft->getName(_game->getLanguage()));
			color = Palette::blockOffset(13);
		}

		_lstSoldiers->setRowColor(row, color);

		_txtSpace->setText(tr("STR_SPACE_USED_FREE_") // (tr("STR_SPACE_AVAILABLE").arg(craft->getSpaceAvailable()))
						.arg(craft->getSpaceUsed())
						.arg(craft->getSpaceAvailable()));
		_txtLoad->setText(tr("STR_LOAD_CAPACITY_FREE_") // (tr("STR_SPACE_USED").arg(craft->getSpaceUsed()))
						.arg(craft->getLoadCapacity())
						.arg(craft->getLoadCapacity() - craft->getLoadCurrent()));
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		_base->setCurrentSoldier(_lstSoldiers->getScroll());

		_game->pushState(new SoldierInfoState(
											_base,
											row));
	}
}

/**
 * Handles the mouse-wheels on the arrow-buttons.
 * @param action - pointer to an Action
 */
/*kL void CraftSoldiersState::lstSoldiersMousePress(Action* action)
{
	if (Options::changeValueByMouseWheel == 0)
		return;

	size_t row = _lstSoldiers->getSelectedRow();
	size_t numSoldiers = _base->getSoldiers()->size();
	if (action->getDetails()->button.button == SDL_BUTTON_WHEELUP
		&& row > 0)
	{
		if (action->getAbsoluteXMouse() >= _lstSoldiers->getArrowsLeftEdge()
			&& action->getAbsoluteXMouse() <= _lstSoldiers->getArrowsRightEdge())
		{
			moveSoldierUp(action, row);
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_WHEELDOWN
		&& 0 < numSoldiers
		&& std::numeric_limits<int>::max() >= numSoldiers
		&& row < (int)numSoldiers - 1)
	{
		if (action->getAbsoluteXMouse() >= _lstSoldiers->getArrowsLeftEdge()
			&& action->getAbsoluteXMouse() <= _lstSoldiers->getArrowsRightEdge())
		{
			moveSoldierDown(action, row);
		}
	}
} */

}
