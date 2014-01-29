/*
 * Copyright 2010-2013 OpenXcom Developers.
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

#include "SoldiersState.h"

#include <sstream>
#include <string>
#include <climits>

#include "CraftArmorState.h"
#include "SoldierInfoState.h"
#include "SoldierMemorialState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/LocalizedText.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"

#include "../Geoscape/AllocatePsiTrainingState.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/Soldier.h"

#include "../Ruleset/RuleCraft.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Soldiers screen.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 */
SoldiersState::SoldiersState(
		Game* game,
		Base* base)
	:
		State(game),
		_base(base)
{
	_window			= new Window(this, 320, 200, 0, 0);
	_txtTitle		= new Text(300, 17, 10, 11);
	_txtBaseLabel	= new Text(80, 9, 16, 11);
	_txtSoldiers	= new Text(20, 9, 284, 11);

	_txtName		= new Text(114, 9, 16, 31);
	_txtRank		= new Text(102, 9, 133, 31);
	_txtCraft		= new Text(82, 9, 226, 31);

	_lstSoldiers	= new TextList(293, 128, 8, 42);

	_btnMemorial	= new TextButton(72, 16, 11, 177);
	_btnPsiTrain	= new TextButton(71, 16, 87, 177);
	_btnArmor		= new TextButton(71, 16, 162, 177);
	_btnOk			= new TextButton(72, 16, 237, 177);


	_game->setPalette(
				_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(2)),
				Palette::backPos,
				16);

	add(_window);
	add(_txtTitle);
	add(_txtBaseLabel);
	add(_txtSoldiers);
	add(_txtName);
	add(_txtRank);
	add(_txtCraft);
	add(_lstSoldiers);
	add(_btnMemorial);
	add(_btnPsiTrain);
	add(_btnArmor);
	add(_btnOk);

	centerAllSurfaces();


	_window->setColor(Palette::blockOffset(15)+1);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK02.SCR"));

	_txtTitle->setColor(Palette::blockOffset(13)+10);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_SOLDIER_LIST"));

	_txtBaseLabel->setColor(Palette::blockOffset(13)+10);
	_txtBaseLabel->setText(_base->getName(_game->getLanguage()));

	_txtSoldiers->setColor(Palette::blockOffset(13)+10);
	_txtSoldiers->setAlign(ALIGN_RIGHT);
	std::wstringstream ss;
	ss << _base->getTotalSoldiers();
	_txtSoldiers->setText(ss.str());

	_btnPsiTrain->setColor(Palette::blockOffset(13)+10);
	_btnPsiTrain->setText(tr("STR_PSIONIC_TRAINING"));
	_btnPsiTrain->onMouseClick((ActionHandler)& SoldiersState::btnPsiTrainingClick);
	_btnPsiTrain->setVisible(
						Options::getBool("anytimePsiTraining")
						&& _base->getAvailablePsiLabs() > 0);

	_btnArmor->setColor(Palette::blockOffset(13)+10);
	_btnArmor->setText(tr("STR_ARMOR"));
	_btnArmor->onMouseClick((ActionHandler)& SoldiersState::btnArmorClick_Soldier);

	_btnOk->setColor(Palette::blockOffset(13)+10);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& SoldiersState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& SoldiersState::btnOkClick,
					(SDLKey)Options::getInt("keyCancel"));

	_btnMemorial->setColor(Palette::blockOffset(13)+10);
	_btnMemorial->setText(tr("STR_MEMORIAL"));
	_btnMemorial->onMouseClick((ActionHandler)& SoldiersState::btnMemorialClick);

	_txtName->setColor(Palette::blockOffset(15)+1);
	_txtName->setText(tr("STR_NAME_UC"));

	_txtRank->setColor(Palette::blockOffset(15)+1);
	_txtRank->setText(tr("STR_RANK"));

	_txtCraft->setColor(Palette::blockOffset(15)+1);
	_txtCraft->setText(tr("STR_CRAFT"));

	_lstSoldiers->setColor(Palette::blockOffset(13)+10);
	_lstSoldiers->setArrowColor(Palette::blockOffset(15)+6);
	_lstSoldiers->setArrowColumn(193, ARROW_VERTICAL);
	_lstSoldiers->setColumns(3, 117, 93, 71);
	_lstSoldiers->setSelectable(true);
	_lstSoldiers->setBackground(_window);
	_lstSoldiers->setMargin(8);
	_lstSoldiers->onLeftArrowClick((ActionHandler)& SoldiersState::lstItemsLeftArrowClick_Soldier);
	_lstSoldiers->onRightArrowClick((ActionHandler)& SoldiersState::lstItemsRightArrowClick_Soldier);
	_lstSoldiers->onMouseClick((ActionHandler)& SoldiersState::lstSoldiersClick);
}

/**
 *
 */
SoldiersState::~SoldiersState()
{
}

/**
 * Updates the soldiers list
 * after going to other screens.
 */
void SoldiersState::init()
{
	_lstSoldiers->clearList();

	int row = 0;
	for (std::vector<Soldier*>::iterator
			i = _base->getSoldiers()->begin();
			i != _base->getSoldiers()->end();
			++i)
	{
		_lstSoldiers->addRow(
							3,
							(*i)->getName().c_str(),
							tr((*i)->getRankString()).c_str(),
							(*i)->getCraftString(_game->getLanguage()).c_str());

		if ((*i)->getCraft() == 0)
		{
			_lstSoldiers->setRowColor(row, Palette::blockOffset(15)+6);
		}

		row++;
	}

	if (row > 0
		&& _lstSoldiers->getScroll() >= row)
	{
		_lstSoldiers->setScroll(0);
	}
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void SoldiersState::btnOkClick(Action*)
{
	_game->popState();
}

/**
 * Opens the Psionic Training screen.
 * @param action Pointer to an action.
 */
void SoldiersState::btnPsiTrainingClick(Action*)
{
	_game->pushState(new AllocatePsiTrainingState(
												_game,
												_base));
}

/**
 * Goes to the Select Armor screen.
 * kL. Taken from CraftInfoState.
 * @param action Pointer to an action.
 */
void SoldiersState::btnArmorClick_Soldier(Action*)
{
	_game->pushState(new CraftArmorState(
										_game,
										_base,
										static_cast<size_t>(0)));
}

/**
 * Opens the Memorial screen.
 * @param action Pointer to an action.
 */
void SoldiersState::btnMemorialClick(Action*)
{
	_game->pushState(new SoldierMemorialState(_game));
}

/**
/**
 * Shows the selected soldier's info.
 * @param action Pointer to an action.
 */
void SoldiersState::lstSoldiersClick(Action* action)
{
	double mx = action->getAbsoluteXMouse();
	if (mx >= _lstSoldiers->getArrowsLeftEdge()
		&& mx < _lstSoldiers->getArrowsRightEdge())
	{
		return;
	}

	_game->pushState(new SoldierInfoState(
										_game,
										_base,
										_lstSoldiers->getSelectedRow()));
}

/**
 * Reorders a soldier. kL_Taken from CraftSoldiersState.
 * @param action Pointer to an action.
 */
void SoldiersState::lstItemsLeftArrowClick_Soldier(Action* action)
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
				{
					SDL_WarpMouse(
							static_cast<Uint16>(action->getXMouse()),
							static_cast<Uint16>(action->getYMouse() - static_cast<int>(8.0 * action->getYScale())));
				}
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
 * Reorders a soldier. kL_Taken from CraftSoldiersState.
 * @param action Pointer to an action.
 */
void SoldiersState::lstItemsRightArrowClick_Soldier(Action* action)
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
				{
					SDL_WarpMouse(
							static_cast<Uint16>(action->getXMouse()),
							static_cast<Uint16>(action->getYMouse() + static_cast<int>(8.0 * action->getYScale())));
				}
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

}
