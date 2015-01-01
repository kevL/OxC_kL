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

#include "AllocatePsiTrainingState.h"

//#include <climits>
//#include <sstream>

#include "GeoscapeState.h"
#include "PsiTrainingState.h"

#include "../Basescape/SoldierInfoState.h"

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
 * @param base - pointer to the Base to get info from
 */
AllocatePsiTrainingState::AllocatePsiTrainingState(Base* base)
	:
		_base(base),
		_sel(0)
{
	_window			= new Window(this, 320, 200, 0, 0);
	_txtTitle		= new Text(300, 17, 10, 8);
	_txtBaseLabel	= new Text(80, 9, 230, 8);

	_txtRemaining	= new Text(100, 9, 12, 20);

	_txtName		= new Text(114, 9, 16, 31);
	_txtPsiStrength	= new Text(48, 9, 134, 31);
	_txtPsiSkill	= new Text(48, 9, 182, 31);
	_txtTraining	= new Text(34, 9, 260, 31);

	_lstSoldiers	= new TextList(293, 129, 8, 42);

	_btnOk			= new TextButton(288, 16, 16, 177);

	setPalette("PAL_BASESCAPE", 7);

	add(_window);
	add(_txtTitle);
	add(_txtBaseLabel);
	add(_txtRemaining);
	add(_txtName);
	add(_txtPsiStrength);
	add(_txtPsiSkill);
	add(_txtTraining);
	add(_lstSoldiers);
	add(_btnOk);

	centerAllSurfaces();


	_window->setColor(Palette::blockOffset(13)+10);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_btnOk->setColor(Palette::blockOffset(13)+10);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& AllocatePsiTrainingState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& AllocatePsiTrainingState::btnOkClick,
					Options::keyCancel);

	_txtTitle->setColor(Palette::blockOffset(13)+10);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setText(tr("STR_PSIONIC_TRAINING"));

	_txtBaseLabel->setColor(Palette::blockOffset(13)+10);
	_txtBaseLabel->setAlign(ALIGN_RIGHT);
	_txtBaseLabel->setText(base->getName(_game->getLanguage()));

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
	_lstSoldiers->setArrowColumn(193, ARROW_VERTICAL);
//kL	_lstSoldiers->setAlign(ALIGN_RIGHT, 3);
	_lstSoldiers->setColumns(4, 118, 48, 78, 34);
	_lstSoldiers->setSelectable();
	_lstSoldiers->setBackground(_window);
	_lstSoldiers->setMargin();
	_lstSoldiers->onMousePress((ActionHandler)& AllocatePsiTrainingState::lstSoldiersPress);
	_lstSoldiers->onLeftArrowClick((ActionHandler)& AllocatePsiTrainingState::lstLeftArrowClick);
	_lstSoldiers->onRightArrowClick((ActionHandler)& AllocatePsiTrainingState::lstRightArrowClick);
}

/**
 * dTor.
 */
AllocatePsiTrainingState::~AllocatePsiTrainingState()
{
}

/**
 * Resets the palette. uh, not really.
 */
void AllocatePsiTrainingState::init()
{
	State::init();

	_lstSoldiers->clearList();

	size_t row = 0;

	for (std::vector<Soldier*>::const_iterator
			i = _base->getSoldiers()->begin();
			i != _base->getSoldiers()->end();
			++i,
				++row)
	{
		_soldiers.push_back(*i);

		std::wostringstream
			ssStr,
			ssSkl;

		int minPsi = (*i)->getRules()->getMinStats().psiSkill;

		if ((*i)->getCurrentStats()->psiSkill >= minPsi
			|| (Options::psiStrengthEval
				&& _game->getSavedGame()->isResearched(_game->getRuleset()->getPsiRequirements())))
		{
			ssStr << ((*i)->getCurrentStats()->psiStrength);
		}
		else
			ssStr << tr("STR_UNKNOWN").c_str();

		if ((*i)->getCurrentStats()->psiSkill >= minPsi)
			ssSkl << (*i)->getCurrentStats()->psiSkill; // << "/+" << (*i)->getImprovement();
		else
		{
//			ssSkl << "0/+0";
			ssSkl << tr("STR_UNKNOWN").c_str();
		}

		if ((*i)->isInPsiTraining())
		{
			_lstSoldiers->addRow(
								4,
								(*i)->getName().c_str(),
								ssStr.str().c_str(),
								ssSkl.str().c_str(),
								tr("STR_YES").c_str());
			_lstSoldiers->setRowColor(row, Palette::blockOffset(13)+5);
		}
		else
		{
			_lstSoldiers->addRow(
								4,
								(*i)->getName().c_str(),
								ssStr.str().c_str(),
								ssSkl.str().c_str(),
								tr("STR_NO").c_str());
			_lstSoldiers->setRowColor(row, Palette::blockOffset(15)+6);
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
void AllocatePsiTrainingState::btnOkClick(Action*)
{
	_base->setCurrentSoldier(_lstSoldiers->getScroll());
	_game->popState();
}

/**
 * LMB assigns & removes a soldier from Psi Training. RMB shows soldier info.
 * @param action - pointer to an Action
 */
void AllocatePsiTrainingState::lstSoldiersPress(Action* action)
{
	const double mx = action->getAbsoluteXMouse();
	if (mx >= static_cast<double>(_lstSoldiers->getArrowsLeftEdge())
		&& mx < static_cast<double>(_lstSoldiers->getArrowsRightEdge()))
	{
		return;
	}

	_sel = _lstSoldiers->getSelectedRow();

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		if (_base->getSoldiers()->at(_sel)->isInPsiTraining() == false)
		{
			if (_base->getUsedPsiLabs() < _base->getAvailablePsiLabs())
			{
				_lstSoldiers->setCellText(_sel, 3, tr("STR_YES").c_str());
				_lstSoldiers->setRowColor(_sel, Palette::blockOffset(13)+5);

				--_labSpace;
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
	else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		_base->setCurrentSoldier(_lstSoldiers->getScroll());
		_game->pushState(new SoldierInfoState(
											_base,
											_sel));
	}
}

/**
 * Reorders a soldier up.
 * @param action - pointer to an Action
 */
void AllocatePsiTrainingState::lstLeftArrowClick(Action* action)
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
void AllocatePsiTrainingState::lstRightArrowClick(Action* action)
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
