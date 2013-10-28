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

#include "CraftArmorState.h"
#include <string>
#include "../Engine/Game.h"
#include "../Resource/ResourcePack.h"
#include "../Engine/Language.h"
#include "../Engine/Palette.h"
#include "../Engine/Options.h"
#include "../Interface/TextButton.h"
#include "../Interface/Window.h"
#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Savegame/Base.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/Craft.h"
#include "../Ruleset/RuleCraft.h"
#include "../Ruleset/Armor.h"
#include "SoldierArmorState.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Craft Armor screen.
 * @param game, Pointer to the core game.
 * @param base, Pointer to the base to get info from.
 * @param craft, ID of the selected craft.
 */
CraftArmorState::CraftArmorState(Game* game, Base* base, size_t craft)
	:
		State(game),
		_base(base),
		_craft(craft)
{
//	Log(LOG_INFO) << "Create CraftArmorState";

	// Create objects
	_window			= new Window(this, 320, 200, 0, 0);

//kL	_txtTitle		= new Text(300, 17, 16, 7);
	_txtTitle		= new Text(300, 17, 11, 10);			// kL

//kL	_txtName		= new Text(114, 9, 16, 32);
//kL	_txtCraft		= new Text(70, 9, 130, 32);
//kL	_txtArmor		= new Text(100, 9, 210, 32);
	_txtName		= new Text(114, 9, 16, 31);				// kL
	_txtArmor		= new Text(76, 9, 133, 31);				// kL
	_txtCraft		= new Text(70, 9, 226, 31);				// kL

//kL	_lstSoldiers	= new TextList(288, 128, 8, 40);
	_lstSoldiers	= new TextList(288, 128, 8, 42);		// kL

//kL	_btnOk			= new TextButton(288, 16, 16, 176);
	_btnOk			= new TextButton(288, 16, 16, 177);		// kL
	// Set palette
	_game->setPalette(_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(4)), Palette::backPos, 16);

	add(_window);
	add(_btnOk);
	add(_txtTitle);
	add(_txtName);
	add(_txtCraft);
	add(_txtArmor);
	add(_lstSoldiers);

	centerAllSurfaces();

	// Set up objects
	_window->setColor(Palette::blockOffset(13)+10);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK14.SCR"));

	_btnOk->setColor(Palette::blockOffset(13)+10);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& CraftArmorState::btnOkClick);
	_btnOk->onKeyboardPress((ActionHandler)& CraftArmorState::btnOkClick, (SDLKey)Options::getInt("keyCancel"));

	_txtTitle->setColor(Palette::blockOffset(13)+10);
	_txtTitle->setBig();
//	_txtTitle->setAlign(ALIGN_CENTER);		// kL
	_txtTitle->setText(tr("STR_SELECT_ARMOR"));

	_txtName->setColor(Palette::blockOffset(13)+10);
	_txtName->setText(tr("STR_NAME_UC"));

	_txtCraft->setColor(Palette::blockOffset(13)+10);
	_txtCraft->setText(tr("STR_CRAFT"));

	_txtArmor->setColor(Palette::blockOffset(13)+10);
	_txtArmor->setText(tr("STR_ARMOR"));

	_lstSoldiers->setColor(Palette::blockOffset(13)+10);
//kL	_lstSoldiers->setColumns(3, 114, 80, 86);
	_lstSoldiers->setArrowColor(Palette::blockOffset(13)+10);	// kL. +6= light brownish
	_lstSoldiers->setArrowColumn(193, ARROW_VERTICAL);			// kL
	_lstSoldiers->setColumns(3, 117, 93, 78);					// kL
	_lstSoldiers->setSelectable(true);
	_lstSoldiers->setBackground(_window);
	_lstSoldiers->setMargin(8);
	_lstSoldiers->onLeftArrowClick((ActionHandler)& CraftArmorState::lstItemsLeftArrowClick_Armor);		// kL
	_lstSoldiers->onRightArrowClick((ActionHandler)& CraftArmorState::lstItemsRightArrowClick_Armor);	// kL
	_lstSoldiers->onMouseClick((ActionHandler)& CraftArmorState::lstSoldiersClick);


//kL	Craft* c = _base->getCrafts()->at(_craft);
	Craft* c;											// kL
	bool hasCraft = _base->getCrafts()->size() > 0;		// kL
	if (hasCraft)										// kL -> KLUDGE!!!
		c = _base->getCrafts()->at(_craft);				// kL

	int row = 0;
	for (std::vector<Soldier* >::iterator i = _base->getSoldiers()->begin(); i != _base->getSoldiers()->end(); ++i)
	{
//		Log(LOG_INFO) << "CraftArmorState::CraftArmorState() iterate soldiers to createList";

//		if (hasCraft)	// kL
//kL			_lstSoldiers->addRow(3, (*i)->getName().c_str(), (*i)->getCraftString(_game->getLanguage()).c_str(), tr((*i)->getArmor()->getType()).c_str());
		_lstSoldiers->addRow(3, (*i)->getName().c_str(), tr((*i)->getArmor()->getType()).c_str(), (*i)->getCraftString(_game->getLanguage()).c_str());
//		else			// kL
//			_lstSoldiers->addRow(3, (*i)->getName().c_str(), tr("STR_NONE_UC"), tr((*i)->getArmor()->getType()).c_str());	// kL

//		Log(LOG_INFO) << ". . add row " << *i;

		Uint8 color;
//		if (_base->getCrafts()->size() > 0)		// kL
//		{
//			Log(LOG_INFO) << ". . . . color, Base has craft";

		if ((*i)->getCraft() == c)		// kL_note: This might CTD.
		{
//			Log(LOG_INFO) << ". . . . color, soldier is on a craft";
			color = Palette::blockOffset(13);
		}
		else if ((*i)->getCraft() != 0)
		{
//			Log(LOG_INFO) << ". . . . color, soldier is NOT on a craft";
			color = Palette::blockOffset(15)+6;
		}
//		}
		else
		{
//			Log(LOG_INFO) << ". . . . color, Base has NO craft";
			color = Palette::blockOffset(13)+10;
		}

		_lstSoldiers->setRowColor(row, color);

		row++;
	}

//	Log(LOG_INFO) << "CraftArmorState::CraftArmorState() EXIT";
}

/**
 *
 */
CraftArmorState::~CraftArmorState()
{
//	Log(LOG_INFO) << "Delete CraftArmorState";
}

/**
 * The soldier armors can change after going into other screens.
 */
void CraftArmorState::init()
{
	_lstSoldiers->clearList();	// kL

	// kL_begin: init Armor list, from cTor
	Craft* c;
	bool hasCraft = _base->getCrafts()->size() > 0;
	if (hasCraft)
		c = _base->getCrafts()->at(_craft);
	// kL_end.

	int row = 0;
	for (std::vector<Soldier* >::iterator i = _base->getSoldiers()->begin(); i != _base->getSoldiers()->end(); ++i)
	{
		_lstSoldiers->addRow(3, (*i)->getName().c_str(), tr((*i)->getArmor()->getType()).c_str(), (*i)->getCraftString(_game->getLanguage()).c_str());	// kL

//kL		_lstSoldiers->setCellText(row, 2, tr((*i)->getArmor()->getType()));
//		_lstSoldiers->setCellText(row, 1, tr((*i)->getArmor()->getType()));		// kL

		// kL_begin: init Armor list, from cTor
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
		// kL_end.

		row++;
	}
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
//kL void CraftArmorState::btnOkClick(Action* )
void CraftArmorState::btnOkClick(Action* action)		// kL
{
	_game->popState();
}

/**
 * Shows the Select Armor window.
 * @param action Pointer to an action.
 */
//kL void CraftArmorState::lstSoldiersClick(Action* )
void CraftArmorState::lstSoldiersClick(Action* action)		// kL
{
	// kL: Taken from CraftSoldiersState::lstSoldiersClick()
	double mx = action->getAbsoluteXMouse();
	if (mx >= _lstSoldiers->getArrowsLeftEdge()
		&& mx < _lstSoldiers->getArrowsRightEdge())
	{
		return;
	} // end_kL.

	Soldier* s = _base->getSoldiers()->at(_lstSoldiers->getSelectedRow());
	if (!(s->getCraft() && s->getCraft()->getStatus() == "STR_OUT"))
	{
		_game->pushState(new SoldierArmorState(_game, _base, _lstSoldiers->getSelectedRow()));
	}
}

/**
 * Reorders a soldier. kL_Taken from CraftSoldiersState.
 * @param action Pointer to an action.
 */
void CraftArmorState::lstItemsLeftArrowClick_Armor(Action* action)
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
void CraftArmorState::lstItemsRightArrowClick_Armor(Action* action)
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
