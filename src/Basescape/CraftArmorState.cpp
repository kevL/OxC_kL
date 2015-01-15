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

#include "CraftArmorState.h"

//#include <climits>
//#include <string>

#include "SoldierArmorState.h"
#include "SoldierInfoState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Menu/ErrorMessageState.h"

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
 * @param base	- pointer to the base to get info from
 * @param craft	- ID of the selected craft
 */
CraftArmorState::CraftArmorState(
		Base* base,
		size_t craftID)
	:
		_base(base),
		_craftID(craftID)
{
	_window			= new Window(this, 320, 200, 0, 0);
	_txtTitle		= new Text(300, 17, 11, 10);
	_txtBaseLabel	= new Text(80, 9, 224, 10);

	_txtName		= new Text(90, 9, 16, 31);
	_txtArmor		= new Text(50, 9, 106, 31);
	_txtCraft		= new Text(50, 9, 226, 31);

	_lstSoldiers	= new TextList(293, 129, 8, 42);

	_btnOk			= new TextButton(288, 16, 16, 177);

	setPalette(
			"PAL_BASESCAPE",
			_game->getRuleset()->getInterface("craftArmor")->getElement("palette")->color); //4

	add(_window, "window", "craftArmor");
	add(_txtTitle, "text", "craftArmor");
	add(_txtBaseLabel);
	add(_txtName, "text", "craftArmor");
	add(_txtArmor, "text", "craftArmor");
	add(_txtCraft, "text", "craftArmor");
	add(_lstSoldiers, "list", "craftArmor");
	add(_btnOk, "button", "craftArmor");

	centerAllSurfaces();


//	_window->setColor(Palette::blockOffset(13)+10);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK14.SCR"));

//	_btnOk->setColor(Palette::blockOffset(13)+10);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& CraftArmorState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& CraftArmorState::btnOkClick,
					Options::keyCancel);

//	_txtTitle->setColor(Palette::blockOffset(13)+10);
	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_SELECT_ARMOR"));

	_txtBaseLabel->setColor(Palette::blockOffset(13)+10);
	_txtBaseLabel->setAlign(ALIGN_RIGHT);
	_txtBaseLabel->setText(_base->getName(_game->getLanguage()));

//	_txtName->setColor(Palette::blockOffset(13)+10);
	_txtName->setText(tr("STR_NAME_UC"));

//	_txtCraft->setColor(Palette::blockOffset(13)+10);
	_txtCraft->setText(tr("STR_CRAFT"));

//	_txtArmor->setColor(Palette::blockOffset(13)+10);
	_txtArmor->setText(tr("STR_ARMOR"));

//	_lstSoldiers->setColor(Palette::blockOffset(13)+10);
	_lstSoldiers->setArrowColor(Palette::blockOffset(13)+10);
	_lstSoldiers->setArrowColumn(193, ARROW_VERTICAL);
	_lstSoldiers->setColumns(3, 90, 120, 73);
	_lstSoldiers->setSelectable();
	_lstSoldiers->setBackground(_window);
	_lstSoldiers->setMargin();
	_lstSoldiers->onMousePress((ActionHandler)& CraftArmorState::lstSoldiersPress);
	_lstSoldiers->onLeftArrowClick((ActionHandler)& CraftArmorState::lstLeftArrowClick);
	_lstSoldiers->onRightArrowClick((ActionHandler)& CraftArmorState::lstRightArrowClick);
}

/**
 * dTor.
 */
CraftArmorState::~CraftArmorState()
{}

/**
 * The soldier armors can change after going into other screens.
 */
void CraftArmorState::init()
{
	State::init();

	_lstSoldiers->clearList();

	// in case this is invoked from SoldiersState at a base without any Craft:
	const Craft* craft;
	if (_base->getCrafts()->empty() == false)
		craft = _base->getCrafts()->at(_craftID);
	else
		craft = NULL;

	size_t row = 0;

	for (std::vector<Soldier*>::const_iterator
			i = _base->getSoldiers()->begin();
			i != _base->getSoldiers()->end();
			++i,
				++row)
	{
		_lstSoldiers->addRow(
						3,
						(*i)->getName().c_str(),
						tr((*i)->getArmor()->getType()).c_str(),
						(*i)->getCraftString(_game->getLanguage()).c_str());

		Uint8 color;

		if ((*i)->getCraft() == NULL)
			color = _lstSoldiers->getColor(); //Palette::blockOffset(13)+10;
		else
		{
			if ((*i)->getCraft() == craft)
				color = _lstSoldiers->getSecondaryColor(); //Palette::blockOffset(13);
			else if ((*i)->getCraft() != NULL)
				color = _game->getRuleset()->getInterface("craftArmor")->getElement("otherCraft")->color; //Palette::blockOffset(15)+6;
		}

		_lstSoldiers->setRowColor(row, color);

		if ((*i)->getWoundRecovery() > 0)
		{
			const int woundPct = (*i)->getWoundPercent();
			if (woundPct > 50)
				color = Palette::blockOffset(6);	// orange
			else if (woundPct > 10)
				color = Palette::blockOffset(9);	// yellow
			else
				color = Palette::blockOffset(3);	// green

			_lstSoldiers->setCellColor(
									row,
									2,
									color,
									true);
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
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void CraftArmorState::btnOkClick(Action*)
{
	_base->setCurrentSoldier(_base->getCurrentSoldier());
	_game->popState();
}

/**
 * LMB shows the Select Armor window.
 * RMB shows soldier info.
 * @param action - pointer to an Action
 */
void CraftArmorState::lstSoldiersPress(Action* action)
{
	const double mx = action->getAbsoluteXMouse();
	if (mx >= static_cast<double>(_lstSoldiers->getArrowsLeftEdge())
		&& mx < static_cast<double>(_lstSoldiers->getArrowsRightEdge()))
	{
		return;
	}

	_base->setCurrentSoldier(_lstSoldiers->getScroll());

	const Soldier* const soldier = _base->getSoldiers()->at(_lstSoldiers->getSelectedRow());
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		if (!
			(soldier->getCraft() != NULL
				&& soldier->getCraft()->getStatus() == "STR_OUT"))
		{
			_game->pushState(new SoldierArmorState(
												_base,
												_lstSoldiers->getSelectedRow()));
		}
		else
			_game->pushState(new ErrorMessageState(
												tr("STR_SOLDIER_NOT_AT_BASE"),
												_palette,
												Palette::blockOffset(4)+10,
												"BACK12.SCR",
												6));
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		_base->setCurrentSoldier(_lstSoldiers->getScroll());
		_game->pushState(new SoldierInfoState(
											_base,
											_lstSoldiers->getSelectedRow()));

/*kL: sorry I'll keep SoldierInfoState on RMB; it's easy enough to assign armor.
		SavedGame* _save;
		_save = _game->getSavedGame();
		Armor* a = _game->getRuleset()->getArmor(_save->getLastSelectedArmor());
		if (_game->getSavedGame()->getMonthsPassed() != -1)
		{
			if (_base->getItems()->getItem(a->getStoreItem()) > 0 || a->getStoreItem() == "STR_NONE")
			{
				if (s->getArmor()->getStoreItem() != "STR_NONE")
					_base->getItems()->addItem(s->getArmor()->getStoreItem());
				if (a->getStoreItem() != "STR_NONE")
					_base->getItems()->removeItem(a->getStoreItem());

				s->setArmor(a);
				_lstSoldiers->setCellText(_lstSoldiers->getSelectedRow(), 2, tr(a->getType()));
			}
		}
		else
		{
			s->setArmor(a);
			_lstSoldiers->setCellText(_lstSoldiers->getSelectedRow(), 2, tr(a->getType()));
		} */
	}
}

/**
 * Reorders a soldier up.
 * @param action - pointer to an Action
 */
void CraftArmorState::lstLeftArrowClick(Action* action)
{
	_base->setCurrentSoldier(_lstSoldiers->getScroll());

	const size_t row = _lstSoldiers->getSelectedRow();
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
						static_cast<Uint16>(action->getTopBlackBand() + action->getYMouse() - static_cast<int>(8. * action->getYScale())));
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
	}

	init();
}

/**
 * Reorders a soldier down.
 * @param action - pointer to an Action
 */
void CraftArmorState::lstRightArrowClick(Action* action)
{
	_base->setCurrentSoldier(_lstSoldiers->getScroll());

	const size_t
		numSoldiers = _base->getSoldiers()->size(),
		row = _lstSoldiers->getSelectedRow();

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
						static_cast<Uint16>(action->getTopBlackBand() + action->getYMouse() + static_cast<int>(8. * action->getYScale())));
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
	}

	init();
}

}
