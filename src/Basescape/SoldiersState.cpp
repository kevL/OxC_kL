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
#include <string>
#include "../Engine/Game.h"
#include "../Resource/ResourcePack.h"
#include "../Engine/Language.h"
#include "../Engine/Palette.h"
#include "../Engine/Options.h"
#include "../Geoscape/AllocatePsiTrainingState.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Savegame/Base.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/Craft.h"
#include "../Ruleset/RuleCraft.h"
#include "SoldierInfoState.h"
#include "SoldierMemorialState.h"
// kL_begin: taken from CraftSoldiersState...
#include <sstream>
#include <climits>
#include "../Engine/Action.h"
#include "../Engine/LocalizedText.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Soldiers screen.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to get info from.
 */
SoldiersState::SoldiersState(Game* game, Base* base)
//SoldiersState::SoldiersState(Game* game, Base* base, size_t craft)		// kL
	:
		State(game),
		_base(base)
//		_craft(craft)	// kL
{
	bool isPsiBtnVisible = Options::getBool("anytimePsiTraining") && _base->getAvailablePsiLabs() > 0;

	// Create objects
	_window			= new Window(this, 320, 200, 0, 0);

	if (isPsiBtnVisible)
	{
		_btnOk			= new TextButton(96, 16, 216, 176);
		_btnPsiTraining	= new TextButton(96, 16, 112, 176);
		_btnMemorial	= new TextButton(96, 16, 8, 176);
	}
	else
	{
		_btnOk			= new TextButton(148, 16, 164, 176);
		_btnPsiTraining	= new TextButton(148, 16, 164, 176);
		_btnMemorial	= new TextButton(148, 16, 8, 176);
	}

//kL	_txtTitle		= new Text(310, 16, 5, 8);
	_txtTitle		= new Text(300, 16, 10, 8);		// kL

//kL	_txtName		= new Text(114, 9, 16, 32);
//kL	_txtRank		= new Text(102, 9, 130, 32);
//kL	_txtCraft		= new Text(82, 9, 222, 32);
	_txtName		= new Text(114, 9, 16, 31);		// kL
	_txtRank		= new Text(102, 9, 133, 31);	// kL
	_txtCraft		= new Text(82, 9, 226, 31);		// kL

//kL	_lstSoldiers	= new TextList(288, 128, 8, 40);
	_lstSoldiers	= new TextList(288, 128, 8, 42);

//kL	_btnPsiTrain	= new TextButton(148, 16, 8, 176);
	_btnPsiTrain	= new TextButton(94, 16, 16, 177);		// kL
//kL	_btnOk			= new TextButton(isPsiBtnVisible? 148:288, 16, isPsiBtnVisible? 164:16, 176);
	_btnArmor		= new TextButton(94, 16, 113, 177);		// kL
	_btnOk			= new TextButton(94, 16, 210, 177);		// kL

	// Set palette
	_game->setPalette(_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(2)), Palette::backPos, 16);

	add(_window);
	add(_btnPsiTrain);
	add(_btnArmor);		// kL
	add(_btnOk);
	add(_btnMemorial);
	add(_txtTitle);
	add(_txtName);
	add(_txtRank);
	add(_txtCraft);
	add(_lstSoldiers);

	centerAllSurfaces();

	// Set up objects
	_window->setColor(Palette::blockOffset(15)+1);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK02.SCR"));

	_btnPsiTrain->setColor(Palette::blockOffset(13)+10);
	_btnPsiTrain->setText(tr("STR_PSIONIC_TRAINING"));
	_btnPsiTrain->onMouseClick((ActionHandler)& SoldiersState::btnPsiTrainingClick);
	_btnPsiTrain->setVisible(isPsiBtnVisible);

	// kL_begin: setup Armor from Soldiers screen.
	_btnArmor->setColor(Palette::blockOffset(13)+10);
	_btnArmor->setText(tr("STR_ARMOR"));
	_btnArmor->onMouseClick((ActionHandler)& SoldiersState::btnArmorClick_Soldier);
	// kL_end.

	_btnOk->setColor(Palette::blockOffset(13)+10);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& SoldiersState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)& SoldiersState::btnOkClick, (SDLKey)Options::getInt("keyCancel"));

	_btnMemorial->setColor(Palette::blockOffset(13)+10);
	_btnMemorial->setText(tr("STR_MEMORIAL"));
	_btnMemorial->onMouseClick((ActionHandler)&SoldiersState::btnMemorialClick);

	_txtTitle->setColor(Palette::blockOffset(13)+10);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_SOLDIER_LIST"));

	_txtName->setColor(Palette::blockOffset(15)+1);
	_txtName->setText(tr("STR_NAME_UC"));

	_txtRank->setColor(Palette::blockOffset(15)+1);
	_txtRank->setText(tr("STR_RANK"));

	_txtCraft->setColor(Palette::blockOffset(15)+1);
	_txtCraft->setText(tr("STR_CRAFT"));

	_lstSoldiers->setColor(Palette::blockOffset(13)+10);
//kL	_lstSoldiers->setArrowColor(Palette::blockOffset(15)+1);
	_lstSoldiers->setArrowColor(Palette::blockOffset(15)+6);	// kL
	_lstSoldiers->setArrowColumn(193, ARROW_VERTICAL);			// kL
//kL	_lstSoldiers->setColumns(3, 114, 92, 74);				// =280
	_lstSoldiers->setColumns(3, 117, 93, 71);					// kL
	_lstSoldiers->setSelectable(true);
	_lstSoldiers->setBackground(_window);
	_lstSoldiers->setMargin(8);
	_lstSoldiers->onLeftArrowClick((ActionHandler)& SoldiersState::lstItemsLeftArrowClick_Soldier);		// kL
	_lstSoldiers->onRightArrowClick((ActionHandler)& SoldiersState::lstItemsRightArrowClick_Soldier);	// kL
	_lstSoldiers->onMouseClick((ActionHandler)& SoldiersState::lstSoldiersClick);

	// kL_note: this is the CraftSoldiersState list:
/*	_lstSoldiers->setColor(Palette::blockOffset(13)+10);
	_lstSoldiers->setArrowColor(Palette::blockOffset(15)+6);
	_lstSoldiers->setArrowColumn(192, ARROW_VERTICAL);
	_lstSoldiers->setColumns(3, 106, 102, 72);
	_lstSoldiers->setSelectable(true);
	_lstSoldiers->setBackground(_window);
	_lstSoldiers->setMargin(8);
	_lstSoldiers->onLeftArrowClick((ActionHandler) &CraftSoldiersState::lstItemsLeftArrowClick);
	_lstSoldiers->onRightArrowClick((ActionHandler) &CraftSoldiersState::lstItemsRightArrowClick);
	_lstSoldiers->onMouseClick((ActionHandler) &CraftSoldiersState::lstSoldiersClick); */
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
	for (std::vector<Soldier* >::iterator i = _base->getSoldiers()->begin(); i != _base->getSoldiers()->end(); ++i)
	{
		_lstSoldiers->addRow(3, (*i)->getName().c_str(), tr((*i)->getRankString()).c_str(), (*i)->getCraftString(_game->getLanguage()).c_str());

		if ((*i)->getCraft() == 0)
		{
			_lstSoldiers->setRowColor(row, Palette::blockOffset(15)+6);
		}

		row++;
	}
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void SoldiersState::btnOkClick(Action* )
{
	_game->popState();
}

/**
 * Opens the Psionic Training screen.
 * @param action Pointer to an action.
 */
void SoldiersState::btnPsiTrainingClick(Action* )
{
	_game->pushState(new AllocatePsiTrainingState(_game, _base));
}

/**
 * Goes to the Select Armor screen.
 * kL. Taken from CraftInfoState.
 * @param action Pointer to an action.
 */
void SoldiersState::btnArmorClick_Soldier(Action* )
{
	_game->pushState(new CraftArmorState(_game, _base, (size_t)0));
}

/**
 * Opens the Memorial screen.
 * @param action Pointer to an action.
 */
void SoldiersState::btnMemorialClick(Action* )
{
	_game->pushState(new SoldierMemorialState(_game));
}

/**
/**
 * Shows the selected soldier's info.
 * @param action Pointer to an action.
 */
//kL void SoldiersState::lstSoldiersClick(Action* )
void SoldiersState::lstSoldiersClick(Action* action)		// kL
{
	// kL: Taken from CraftSoldiersState::lstSoldiersClick()
	double mx = action->getAbsoluteXMouse();
	if (mx >= _lstSoldiers->getArrowsLeftEdge()
		&& mx < _lstSoldiers->getArrowsRightEdge())
	{
		return;
	} // end_kL.

	_game->pushState(new SoldierInfoState(_game, _base, _lstSoldiers->getSelectedRow()));
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
					SDL_WarpMouse(action->getXMouse(), action->getYMouse() - static_cast<Uint16>(8 * action->getYScale()));
				}
				else
				{
					_lstSoldiers->scrollUp(false);
				}
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
			&& row < (int)numSoldiers - 1)
		{
			Soldier* s = _base->getSoldiers()->at(row);

			if (SDL_BUTTON_LEFT == action->getDetails()->button.button)
			{
				_base->getSoldiers()->at(row) = _base->getSoldiers()->at(row + 1);
				_base->getSoldiers()->at(row + 1) = s;

				if (row != 15 + _lstSoldiers->getScroll())
				{
					SDL_WarpMouse(action->getXMouse(), action->getYMouse() + static_cast<Uint16>(8 * action->getYScale()));
				}
				else
				{
					_lstSoldiers->scrollDown(false);
				}
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
