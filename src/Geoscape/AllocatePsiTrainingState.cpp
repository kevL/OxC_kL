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

#include "AllocatePsiTrainingState.h"

#include <climits>
#include <sstream>

#include "GeoscapeState.h"
#include "PsiTrainingState.h"

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

#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleSoldier.h"

#include "../Savegame/Base.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Psi Training screen.
 * @param game Pointer to the core game.
 * @param base Pointer to the base to handle.
 */
AllocatePsiTrainingState::AllocatePsiTrainingState(
		Game* game,
		Base* base)
	:
		State(game),
		_sel(0)
{
	_base = base;

	_window			= new Window(this, 320, 200, 0, 0);
	_txtTitle		= new Text(300, 17, 10, 8);

	_txtRemaining	= new Text(100, 8, 12, 20);

	_txtName		= new Text(114, 8, 16, 31);
	_txtPsiStrength	= new Text(48, 16, 134, 23);
	_txtPsiSkill	= new Text(48, 16, 182, 23);
	_txtTraining	= new Text(34, 16, 260, 23);

	_lstSoldiers	= new TextList(293, 128, 8, 42);

	_btnOk			= new TextButton(288, 16, 16, 177);


	_game->setPalette(_game->getResourcePack()->getPalette("PALETTES.DAT_1")->getColors());
	_game->setPalette(
				_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(7)),
				Palette::backPos,
				16);

	add(_window);
	add(_btnOk);
	add(_txtName);
	add(_txtTitle);
	add(_txtRemaining);
	add(_txtPsiStrength);
	add(_txtPsiSkill);
	add(_txtTraining);
	add(_lstSoldiers);

	centerAllSurfaces();


	_window->setColor(Palette::blockOffset(13)+10);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_btnOk->setColor(Palette::blockOffset(13)+10);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& AllocatePsiTrainingState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& AllocatePsiTrainingState::btnOkClick,
					(SDLKey)Options::getInt("keyCancel"));

	_txtTitle->setColor(Palette::blockOffset(13)+10);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_PSIONIC_TRAINING"));

	_labSpace = base->getAvailablePsiLabs() - base->getUsedPsiLabs();
	_txtRemaining->setColor(Palette::blockOffset(13)+10);
	_txtRemaining->setSecondaryColor(Palette::blockOffset(13));
	_txtRemaining->setText(tr("STR_REMAINING_PSI_LAB_CAPACITY").arg(_labSpace));

	_txtName->setColor(Palette::blockOffset(13)+10);
	_txtName->setText(tr("STR_NAME"));

	_txtPsiStrength->setColor(Palette::blockOffset(13)+10);
	_txtPsiStrength->setText(tr("STR_PSIONIC__STRENGTH"));

	_txtPsiSkill->setColor(Palette::blockOffset(13)+10);
	_txtPsiSkill->setText(tr("STR_PSIONIC_SKILL_IMPROVEMENT"));

	_txtTraining->setColor(Palette::blockOffset(13)+10);
	_txtTraining->setText(tr("STR_IN_TRAINING"));

	_lstSoldiers->setColor(Palette::blockOffset(13)+10);
	_lstSoldiers->setArrowColor(Palette::blockOffset(13)+10);
//kL	_lstSoldiers->setArrowColumn(-1, ARROW_VERTICAL);
	_lstSoldiers->setArrowColumn(193, ARROW_VERTICAL); // kL
	_lstSoldiers->setColumns(4, 118, 48, 78, 34);
	_lstSoldiers->setSelectable(true);
	_lstSoldiers->setBackground(_window);
	_lstSoldiers->setMargin(8);
	_lstSoldiers->onLeftArrowClick((ActionHandler)& AllocatePsiTrainingState::lstItemsLeftArrowClick_Psi);
	_lstSoldiers->onRightArrowClick((ActionHandler)& AllocatePsiTrainingState::lstItemsRightArrowClick_Psi);
	_lstSoldiers->onMouseClick((ActionHandler)& AllocatePsiTrainingState::lstSoldiersClick);

	init(); // kL
/*kL	int row = 0;
	for (std::vector<Soldier*>::const_iterator
			s = base->getSoldiers()->begin();
			s != base->getSoldiers()->end();
			++s)
	{
		_soldiers.push_back(*s);

		std::wstringstream ssStr, ssSkl;

		if ((*s)->getCurrentStats()->psiSkill > 0
			|| (Options::getBool("psiStrengthEval")
				&& _game->getSavedGame()->isResearched(_game->getRuleset()->getPsiRequirements())))
		{
			ssStr << ((*s)->getCurrentStats()->psiStrength);
		}
		else
		{
			ssStr << tr("STR_UNKNOWN").c_str();
		}

		if ((*s)->getCurrentStats()->psiSkill > 0)
		{
			ssSkl << (*s)->getCurrentStats()->psiSkill; //kL << "/+" << (*s)->getImprovement();
		}
		else
		{
//kL			ssSkl << "0/+0";
			ssSkl << "0";
		}

		if ((*s)->isInPsiTraining())
		{
			_lstSoldiers->addRow(
								4,
								(*s)->getName().c_str(),
								ssStr.str().c_str(),
								ssSkl.str().c_str(),
								tr("STR_YES").c_str());
			_lstSoldiers->setRowColor(row, Palette::blockOffset(13)+5);
		}
		else
		{
			_lstSoldiers->addRow(
								4,
								(*s)->getName().c_str(),
								ssStr.str().c_str(),
								ssSkl.str().c_str(),
								tr("STR_NO").c_str());
			_lstSoldiers->setRowColor(row, Palette::blockOffset(15)+6);
		}

		row++;
	} */
}

/**
 *
 */
AllocatePsiTrainingState::~AllocatePsiTrainingState()
{
}

/**
 * Resets the palette.
 */
void AllocatePsiTrainingState::init()
{
	_game->setPalette(
				_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(Palette::blockOffset(7)),
				Palette::backPos,
				16);

	_lstSoldiers->clearList();

	int row = 0;
	for (std::vector<Soldier*>::const_iterator
			s = _base->getSoldiers()->begin();
			s != _base->getSoldiers()->end();
			++s)
	{
		_soldiers.push_back(*s);

		std::wstringstream
			ssStr,
			ssSkl;


		int minPsi = (*s)->getRules()->getMinStats().psiSkill; // kL

//kL		if ((*s)->getCurrentStats()->psiSkill > 0
		if ((*s)->getCurrentStats()->psiSkill >= minPsi // kL
			|| (Options::getBool("psiStrengthEval")
				&& _game->getSavedGame()->isResearched(_game->getRuleset()->getPsiRequirements())))
		{
			ssStr << ((*s)->getCurrentStats()->psiStrength);
		}
		else
		{
			ssStr << tr("STR_UNKNOWN").c_str();
		}

//kL		if ((*s)->getCurrentStats()->psiSkill > 0)
		if ((*s)->getCurrentStats()->psiSkill >= minPsi) // kL
		{
			ssSkl << (*s)->getCurrentStats()->psiSkill; //kL << "/+" << (*s)->getImprovement();
		}
		else
		{
//kL			ssSkl << "0/+0";
//			ssSkl << "0"; // kL
			ssSkl << tr("STR_UNKNOWN").c_str(); // kL
		}

		if ((*s)->isInPsiTraining())
		{
			_lstSoldiers->addRow(
								4,
								(*s)->getName().c_str(),
								ssStr.str().c_str(),
								ssSkl.str().c_str(),
								tr("STR_YES").c_str());
			_lstSoldiers->setRowColor(row, Palette::blockOffset(13)+5);
		}
		else
		{
			_lstSoldiers->addRow(
								4,
								(*s)->getName().c_str(),
								ssStr.str().c_str(),
								ssSkl.str().c_str(),
								tr("STR_NO").c_str());
			_lstSoldiers->setRowColor(row, Palette::blockOffset(15)+6);
		}

		row++;
	}
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void AllocatePsiTrainingState::btnOkClick(Action*)
{
	_game->popState();
}

/**
 *
 */
/*kL void AllocatePsiTrainingState::lstSoldiersPress(Action*)
{
} */

/**
 *
 */
/*kL void AllocatePsiTrainingState::lstSoldiersRelease(Action*)
{
} */

/**
 *
 */
void AllocatePsiTrainingState::lstSoldiersClick(Action* action)
{
	// kL: Taken from CraftSoldiersState::lstSoldiersClick()
	double mx = action->getAbsoluteXMouse();
	if (mx >= static_cast<double>(_lstSoldiers->getArrowsLeftEdge())
		&& mx < static_cast<double>(_lstSoldiers->getArrowsRightEdge()))
	{
		return;
	} // end_kL.

	_sel = _lstSoldiers->getSelectedRow();

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		if (!_base->getSoldiers()->at(_sel)->isInPsiTraining())
		{
			if (_base->getUsedPsiLabs() < _base->getAvailablePsiLabs())
			{
				_lstSoldiers->setCellText(_sel, 3, tr("STR_YES").c_str());
				_lstSoldiers->setRowColor(_sel, Palette::blockOffset(13)+5);

				_labSpace--;
				_txtRemaining->setText(tr("STR_REMAINING_PSI_LAB_CAPACITY").arg(_labSpace));

				_base->getSoldiers()->at(_sel)->setPsiTraining();
			}
		}
		else
		{
			_lstSoldiers->setCellText(_sel, 3, tr("STR_NO").c_str());
			_lstSoldiers->setRowColor(_sel, Palette::blockOffset(15)+6);
			_labSpace++;
			_txtRemaining->setText(tr("STR_REMAINING_PSI_LAB_CAPACITY").arg(_labSpace));
			_base->getSoldiers()->at(_sel)->setPsiTraining();
		}
	}
}

/**
 * Reorders a soldier. kL_Taken from CraftSoldiersState.
 * @param action Pointer to an action.
 */
void AllocatePsiTrainingState::lstItemsLeftArrowClick_Psi(Action* action)
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
void AllocatePsiTrainingState::lstItemsRightArrowClick_Psi(Action* action)
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
