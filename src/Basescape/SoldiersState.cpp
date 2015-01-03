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

#include "SoldiersState.h"

//#include <sstream>
//#include <string>
//#include <climits>

#include "CraftArmorState.h"
#include "SoldierInfoState.h"
#include "SoldierMemorialState.h"

#include "../Battlescape/BattlescapeGenerator.h"
#include "../Battlescape/InventoryState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Logger.h"
//#include "../Engine/LocalizedText.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"
//#include "../Engine/Screen.h"

#include "../Geoscape/AllocatePsiTrainingState.h"

#include "../Interface/Cursor.h"
#include "../Interface/FpsCounter.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/Base.h"
#include "../Savegame/Craft.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"

#include "../Ruleset/RuleCraft.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the SoldiersState screen.
 * @param base - pointer to the Base to get info from
 */
SoldiersState::SoldiersState(Base* base)
	:
		_base(base)
{
	_window			= new Window(this, 320, 200, 0, 0);
	_txtTitle		= new Text(300, 17, 10, 11);
	_txtBaseLabel	= new Text(80, 9, 16, 11);
	_txtSoldiers	= new Text(20, 9, 284, 11);

	_txtName		= new Text(117, 9, 16, 31);
	_txtRank		= new Text(92, 9, 133, 31);
	_txtCraft		= new Text(82, 9, 226, 31);

	_lstSoldiers	= new TextList(293, 129, 8, 42);

	_btnMemorial	= new TextButton(56, 16, 10, 177);
	_btnPsi			= new TextButton(56, 16, 71, 177);
	_btnArmor		= new TextButton(56, 16, 132, 177);
	_btnEquip		= new TextButton(56, 16, 193, 177);
	_btnOk			= new TextButton(56, 16, 254, 177);

	setPalette("PAL_BASESCAPE", _game->getRuleset()->getInterface("soldierList")->getElement("palette")->color); //2

	add(_window, "window", "soldierList");
	add(_txtTitle, "text1", "soldierList");
	add(_txtBaseLabel, "text2", "soldierList");
	add(_txtSoldiers, "text2", "soldierList");
	add(_txtName, "text2", "soldierList");
	add(_txtRank, "text2", "soldierList");
	add(_txtCraft, "text2", "soldierList");
	add(_lstSoldiers, "list", "soldierList");
	add(_btnMemorial, "button", "soldierList");
	add(_btnPsi, "button", "soldierList");
	add(_btnArmor, "button", "soldierList");
	add(_btnEquip, "button", "soldierList");
	add(_btnOk, "button", "soldierList");

	centerAllSurfaces();


//	_window->setColor(Palette::blockOffset(15)+1);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK02.SCR"));

//	_txtTitle->setColor(Palette::blockOffset(13)+10);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_SOLDIER_LIST"));

//	_txtBaseLabel->setColor(Palette::blockOffset(13)+10);
	_txtBaseLabel->setText(_base->getName(_game->getLanguage()));

//	_txtSoldiers->setColor(Palette::blockOffset(13)+10);
	_txtSoldiers->setAlign(ALIGN_RIGHT);

//	_btnMemorial->setColor(Palette::blockOffset(13)+10);
	_btnMemorial->setText(tr("STR_MEMORIAL"));
	_btnMemorial->onMouseClick((ActionHandler)& SoldiersState::btnMemorialClick);
	_btnMemorial->setVisible(_game->getSavedGame()->getDeadSoldiers()->empty() == false);

//	_btnPsi->setColor(Palette::blockOffset(13)+10);
	_btnPsi->setText(tr("STR_PSIONIC_TRAINING"));
	_btnPsi->onMouseClick((ActionHandler)& SoldiersState::btnPsiTrainingClick);
	_btnPsi->setVisible(
					Options::anytimePsiTraining
					&& _base->getAvailablePsiLabs() > 0);

//	_btnArmor->setColor(Palette::blockOffset(13)+10);
	_btnArmor->setText(tr("STR_ARMOR"));
	_btnArmor->onMouseClick((ActionHandler)& SoldiersState::btnArmorClick);

//	_btnEquip->setColor(Palette::blockOffset(13)+10);
	_btnEquip->setText(tr("STR_INVENTORY"));
	_btnEquip->onMouseClick((ActionHandler)& SoldiersState::btnEquipClick);
	_btnEquip->setVisible(_base->getAvailableSoldiers(true) > 0);

//	_btnOk->setColor(Palette::blockOffset(13)+10);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& SoldiersState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& SoldiersState::btnOkClick,
					Options::keyCancel);

//	_txtName->setColor(Palette::blockOffset(15)+1);
	_txtName->setText(tr("STR_NAME_UC"));

//	_txtRank->setColor(Palette::blockOffset(15)+1);
	_txtRank->setText(tr("STR_RANK"));

//	_txtCraft->setColor(Palette::blockOffset(15)+1);
	_txtCraft->setText(tr("STR_CRAFT"));

//	_lstSoldiers->setColor(Palette::blockOffset(13)+10);
//	_lstSoldiers->setArrowColor(Palette::blockOffset(15)+6);
	_lstSoldiers->setBackground(_window);
	_lstSoldiers->setArrowColumn(193, ARROW_VERTICAL);
	_lstSoldiers->setColumns(3, 117, 93, 71);
	_lstSoldiers->setSelectable();
	_lstSoldiers->setMargin();
	_lstSoldiers->onMousePress((ActionHandler)& SoldiersState::lstSoldiersPress);
	_lstSoldiers->onLeftArrowClick((ActionHandler)& SoldiersState::lstLeftArrowClick);
	_lstSoldiers->onRightArrowClick((ActionHandler)& SoldiersState::lstRightArrowClick);
}

/**
 * dTor.
 */
SoldiersState::~SoldiersState()
{}

/**
 * Updates the soldiers list after going to other screens.
 */
void SoldiersState::init()
{
	//Log(LOG_INFO) << "SoldiersState::init()";
	State::init();

	// return from Inventory Equipment Layout screen (pre-battle equip)
	_game->getSavedGame()->setBattleGame(NULL);
	_base->setInBattlescape(false);

//	_game->getCursor()->setColor(Palette::blockOffset(15)+12);
//	_game->getFpsCounter()->setColor(Palette::blockOffset(15)+12);
	// end pre-battle Equip.

	std::wostringstream ss; // in case soldier is told to GTFO.
	ss << _base->getTotalSoldiers();
	_txtSoldiers->setText(ss.str());


	_lstSoldiers->clearList();

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
							tr((*i)->getRankString()).c_str(),
							(*i)->getCraftString(_game->getLanguage()).c_str());

		if ((*i)->getCraft() == NULL)
		{
			_lstSoldiers->setRowColor(row, _lstSoldiers->getSecondaryColor()); //Palette::blockOffset(15)+6);

			if ((*i)->getWoundRecovery() > 0)
			{
				Uint8 color;
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
void SoldiersState::btnOkClick(Action*)
{
	_base->setCurrentSoldier(_lstSoldiers->getScroll());
	_game->popState();
}

/**
 * Opens the Psionic Training screen.
 * @param action - pointer to an Action
 */
void SoldiersState::btnPsiTrainingClick(Action*)
{
	_base->setCurrentSoldier(_lstSoldiers->getScroll());
	_game->pushState(new AllocatePsiTrainingState(_base));
}

/**
 * Goes to the Select Armor screen.
 * @param action - pointer to an Action
 */
void SoldiersState::btnArmorClick(Action*)
{
	_base->setCurrentSoldier(_lstSoldiers->getScroll());
	_game->pushState(new CraftArmorState(
										_base,
										0));
}

/**
 * Opens the Memorial screen.
 * @param action - pointer to an Action
 */
void SoldiersState::btnMemorialClick(Action*)
{
	_game->getResourcePack()->fadeMusic(_game, 900);

	_base->setCurrentSoldier(_lstSoldiers->getScroll());
	_game->pushState(new SoldierMemorialState());
}

/**
 * Shows the selected soldier's info.
 * @param action - pointer to an Action
 */
void SoldiersState::lstSoldiersPress(Action* action)
{
	const double mx = action->getAbsoluteXMouse();
	if (mx >= _lstSoldiers->getArrowsLeftEdge()
		&& mx < _lstSoldiers->getArrowsRightEdge())
	{
		return;
	}

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT
		|| action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		_base->setCurrentSoldier(_lstSoldiers->getScroll());
		_game->pushState(new SoldierInfoState(
											_base,
											_lstSoldiers->getSelectedRow()));
	}
}

/**
 * Reorders a soldier up.
 * @param action - pointer to an Action
 */
void SoldiersState::lstLeftArrowClick(Action* action)
{
	_base->setCurrentSoldier(_lstSoldiers->getScroll());

	const size_t row = _lstSoldiers->getSelectedRow();
	if (row > 0)
	{
		Soldier* soldier = _base->getSoldiers()->at(row);

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
void SoldiersState::lstRightArrowClick(Action* action)
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

/**
* Displays the inventory screen for the soldiers at Base.
* @param action - pointer to an Action
*/
void SoldiersState::btnEquipClick(Action*)
{
	_base->setCurrentSoldier(_lstSoldiers->getScroll());

	SavedBattleGame* battle = new SavedBattleGame();
	_game->getSavedGame()->setBattleGame(battle);
	BattlescapeGenerator bgen = BattlescapeGenerator(_game);

	bgen.runInventory(NULL, _base);

	// Set system colors
//	_game->getCursor()->setColor(Palette::blockOffset(9));
//	_game->getFpsCounter()->setColor(Palette::blockOffset(9));

	_game->getScreen()->clear();
	_game->pushState(new InventoryState(
									false,
									NULL));
}

}
