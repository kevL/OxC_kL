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

#include <climits>
#include <sstream>
#include <string>

#include "SoldierInfoState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
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
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 * @param craft ID of the selected craft.
 */
CraftSoldiersState::CraftSoldiersState(
		Game* game,
		Base* base,
		size_t craft)
	:
		State(game),
		_base(base),
		_craft(craft)
{
	//Log(LOG_INFO) << "Create CraftSoldiersState";

	_window			= new Window(this, 320, 200, 0, 0);

	_txtTitle		= new Text(300, 17, 16, 8);
	_txtBaseLabel	= new Text(80, 9, 224, 8);

	_txtUsed		= new Text(110, 9, 16, 24);
	_txtAvailable	= new Text(110, 9, 160, 24);

	_txtName		= new Text(116, 9, 16, 33);
	_txtRank		= new Text(93, 9, 140, 33);
	_txtCraft		= new Text(71, 9, 225, 33);

	_lstSoldiers	= new TextList(285, 128, 16, 42);

	_btnUnload		= new TextButton(134, 16, 16, 177);
	_btnOk			= new TextButton(134, 16, 170, 177);

	setPalette("PAL_BASESCAPE", 2);

	add(_window);
	add(_txtTitle);
	add(_txtBaseLabel);
	add(_txtAvailable);
	add(_txtUsed);
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
	_btnUnload->setText(_game->getLanguage()->getString("STR_UNLOAD"));
	_btnUnload->onMouseClick((ActionHandler)& CraftSoldiersState::btnUnloadClick);

	_txtTitle->setColor(Palette::blockOffset(15)+6);
	_txtTitle->setBig();

	_txtBaseLabel->setColor(Palette::blockOffset(15)+6);
	_txtBaseLabel->setAlign(ALIGN_RIGHT);
	_txtBaseLabel->setText(_base->getName(_game->getLanguage()));

	Craft *c = _base->getCrafts()->at(_craft);
	_txtTitle->setText(tr("STR_SELECT_SQUAD_FOR_CRAFT").arg(c->getName(_game->getLanguage())));

	_txtName->setColor(Palette::blockOffset(15)+6);
	_txtName->setText(tr("STR_NAME_UC"));

	_txtRank->setColor(Palette::blockOffset(15)+6);
	_txtRank->setText(tr("STR_RANK"));

	_txtCraft->setColor(Palette::blockOffset(15)+6);
	_txtCraft->setText(tr("STR_CRAFT"));

	_txtAvailable->setColor(Palette::blockOffset(15)+6);
	_txtAvailable->setSecondaryColor(Palette::blockOffset(13));
	_txtAvailable->setText(tr("STR_SPACE_AVAILABLE").arg(c->getSpaceAvailable()));

	_txtUsed->setColor(Palette::blockOffset(15)+6);
	_txtUsed->setSecondaryColor(Palette::blockOffset(13));
	_txtUsed->setText(tr("STR_SPACE_USED").arg(c->getSpaceUsed()));

	_lstSoldiers->setColor(Palette::blockOffset(13)+10);
	_lstSoldiers->setArrowColor(Palette::blockOffset(15)+6);
	_lstSoldiers->setArrowColumn(180, ARROW_VERTICAL);
	_lstSoldiers->setColumns(3, 116, 85, 71);
	_lstSoldiers->setSelectable(true);
	_lstSoldiers->setBackground(_window);
	_lstSoldiers->setMargin(8);
	_lstSoldiers->onLeftArrowClick((ActionHandler)& CraftSoldiersState::lstItemsLeftArrowClick);
	_lstSoldiers->onRightArrowClick((ActionHandler)& CraftSoldiersState::lstItemsRightArrowClick);
	_lstSoldiers->onMouseClick((ActionHandler)& CraftSoldiersState::lstSoldiersClick, 0);
}

/**
 *
 */
CraftSoldiersState::~CraftSoldiersState()
{
	//Log(LOG_INFO) << "Delete CraftSoldiersState";
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void CraftSoldiersState::btnOkClick(Action*)
{
	_game->popState();
}

/**
 * kL. Unloads all soldiers from current transport craft.
 * NB: This relies on no two transport craft having the same name!!!!!
 * See, void CraftInfoState::edtCraftKeyPress(Action* action) etc.
 *
 * @param action, Pointer to an action.
 */
void CraftSoldiersState::btnUnloadClick(Action*) // kL
{
	Craft* c = _base->getCrafts()->at(_craft);
	std::wstring craft1 = c->getName(_game->getLanguage());

	// iterate over all soldiers at Base
	for (std::vector<Soldier*>::iterator
			i = _base->getSoldiers()->begin();
			i != _base->getSoldiers()->end();
			++i)
	{
		if ((*i)->getCraft() == c) // if soldier is on this Craft, unload them
			(*i)->setCraft(0);
	}

	// iterate over all listRows and change their stringText and lineColor
	Uint8 color = Palette::blockOffset(13)+10;
	std::wstring craft2;

	for (unsigned
			r = 0;
			r < _base->getSoldiers()->size();
			++r)
	{
		craft2 = _lstSoldiers->getCellText(r, 2);
		if (craft2 == craft1) // if row pertains to this craft
		{
			_lstSoldiers->setCellText(r, 2, _game->getLanguage()->getString("STR_NONE_UC"));
			_lstSoldiers->setRowColor(r, color);
		}
	}

	_txtAvailable->setText(tr("STR_SPACE_AVAILABLE").arg(c->getSpaceAvailable()));
	_txtUsed->setText(tr("STR_SPACE_USED").arg(c->getSpaceUsed()));
}

/**
 * Shows the soldiers in a list.
 */
void CraftSoldiersState::init()
{
	State::init();

	Craft* c = _base->getCrafts()->at(_craft);
	_lstSoldiers->clearList();

	int row = 0;
	for (std::vector<Soldier*>::iterator
			i = _base->getSoldiers()->begin();
			i != _base->getSoldiers()->end();
			++i)
	{
		_lstSoldiers->addRow(
							3,
							(*i)->getName(true, 19).c_str(),
							tr((*i)->getRankString()).c_str(),
							(*i)->getCraftString(_game->getLanguage()).c_str());

		Uint8 color;

		if ((*i)->getCraft() == c)
			color = Palette::blockOffset(13);
		else if ((*i)->getCraft() != 0)
			color = Palette::blockOffset(15)+6;
		else
			color = Palette::blockOffset(13)+10;

		_lstSoldiers->setRowColor(row, color);

		row++;
	}

	_lstSoldiers->draw();
}

/**
 * Reorders a soldier.
 * @param action Pointer to an action.
 */
void CraftSoldiersState::lstItemsLeftArrowClick(Action* action)
{
	if (SDL_BUTTON_LEFT == action->getDetails()->button.button
		|| SDL_BUTTON_RIGHT == action->getDetails()->button.button)
	{
		int row = _lstSoldiers->getSelectedRow();
		if (row > 0)
		{
			Soldier* s = _base->getSoldiers()->at(row);

			if (SDL_BUTTON_LEFT == action->getDetails()->button.button)
			{
				_base->getSoldiers()->at(row) = _base->getSoldiers()->at(row - 1);
				_base->getSoldiers()->at(row - 1) = s;

				if (row != _lstSoldiers->getScroll())
					SDL_WarpMouse(
							static_cast<Uint16>(action->getLeftBlackBand() + action->getXMouse()),
							static_cast<Uint16>(action->getTopBlackBand() + action->getYMouse() - static_cast<int>(8.0 * action->getYScale())));
				else
					_lstSoldiers->scrollUp(false);
			}
			else
			{
				_base->getSoldiers()->erase(_base->getSoldiers()->begin() + row);
				_base->getSoldiers()->insert(_base->getSoldiers()->begin(), s);
			}
		}

		init();
	}
}

/**
 * Reorders a soldier.
 * @param action Pointer to an action.
 */
void CraftSoldiersState::lstItemsRightArrowClick(Action* action)
{
	if (SDL_BUTTON_LEFT == action->getDetails()->button.button
		|| SDL_BUTTON_RIGHT == action->getDetails()->button.button)
	{
		int row = _lstSoldiers->getSelectedRow();
		size_t numSoldiers = _base->getSoldiers()->size();

		if (0 < numSoldiers
			&& INT_MAX >= numSoldiers
			&& row < static_cast<int>(numSoldiers) - 1)
		{
			Soldier* s = _base->getSoldiers()->at(row);

			if (SDL_BUTTON_LEFT == action->getDetails()->button.button)
			{
				_base->getSoldiers()->at(row) = _base->getSoldiers()->at(row + 1);
				_base->getSoldiers()->at(row + 1) = s;

				if (row != 15 + _lstSoldiers->getScroll())
					SDL_WarpMouse(
							static_cast<Uint16>(action->getLeftBlackBand() + action->getXMouse()),
							static_cast<Uint16>(action->getTopBlackBand() + action->getYMouse() + static_cast<int>(8.0 * action->getYScale())));
				else
					_lstSoldiers->scrollDown(false);
			}
			else
			{
				_base->getSoldiers()->erase(_base->getSoldiers()->begin() + row);
				_base->getSoldiers()->insert(_base->getSoldiers()->end(), s);
			}
		}

		init();
	}
}

/**
 * Assigns and de-assigns soldiers from a craft.
 * @param action Pointer to an action.
 */
void CraftSoldiersState::lstSoldiersClick(Action* action)
{
	double mx = action->getAbsoluteXMouse();
	if (mx >= static_cast<double>(_lstSoldiers->getArrowsLeftEdge())
		&& mx < static_cast<double>(_lstSoldiers->getArrowsRightEdge()))
	{
		return;
	}

	int row = _lstSoldiers->getSelectedRow();

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		Craft* c = _base->getCrafts()->at(_craft);
		Soldier* s = _base->getSoldiers()->at(_lstSoldiers->getSelectedRow());

		Uint8 color = Palette::blockOffset(13)+10;

		if (s->getCraft() == c)
		{
			s->setCraft(0);
			_lstSoldiers->setCellText(row, 2, tr("STR_NONE_UC"));
			color = Palette::blockOffset(13)+10;
		}
		else if (s->getCraft()
			&& s->getCraft()->getStatus() == "STR_OUT")
		{
			color = Palette::blockOffset(15)+6;
		}
		else if (c->getSpaceAvailable() > 0
			&& s->getWoundRecovery() == 0)
		{
			s->setCraft(c);
			_lstSoldiers->setCellText(row, 2, c->getName(_game->getLanguage()));
			color = Palette::blockOffset(13);
		}

		_lstSoldiers->setRowColor(row, color);

		_txtAvailable->setText(tr("STR_SPACE_AVAILABLE").arg(c->getSpaceAvailable()));
		_txtUsed->setText(tr("STR_SPACE_USED").arg(c->getSpaceUsed()));
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		_game->pushState(new SoldierInfoState(
											_game,
											_base,
											row));
}

}
