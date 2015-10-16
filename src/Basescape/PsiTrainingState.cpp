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

#include "PsiTrainingState.h"

//#include <climits>
//#include <sstream>

#include "../Basescape/SoldierInfoState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
//#include "../Engine/LocalizedText.h"
//#include "../Engine/Options.h"
#include "../Engine/Sound.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/XcomResourcePack.h"

//#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleSoldier.h"

#include "../Savegame/Base.h"
#include "../Savegame/SavedGame.h"
//#include "../Savegame/Soldier.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Psi Training screen.
 * @param base - pointer to the Base to get info from
 */
PsiTrainingState::PsiTrainingState(Base* base)
	:
		_base(base),
		_sel(0)
{
	_window			= new Window(this, 320, 200);

	_txtTitle		= new Text(300, 17,  10, 8);
	_txtBaseLabel	= new Text( 80,  9, 230, 8);

	_txtRemaining	= new Text(100, 9, 12, 20);

	_txtName		= new Text(114, 9,  16, 31);
	_txtPsiStrength	= new Text( 48, 9, 134, 31);
	_txtPsiSkill	= new Text( 48, 9, 182, 31);
	_txtTraining	= new Text( 34, 9, 260, 31);

	_lstSoldiers	= new TextList(293, 129, 8, 42);

	_btnOk			= new TextButton(288, 16, 16, 177);

	setInterface("allocatePsi");

	add(_window,			"window",	"allocatePsi");
	add(_txtTitle,			"text",		"allocatePsi");
	add(_txtBaseLabel,		"text",		"allocatePsi");
	add(_txtRemaining,		"text",		"allocatePsi");
	add(_txtName,			"text",		"allocatePsi");
	add(_txtPsiStrength,	"text",		"allocatePsi");
	add(_txtPsiSkill,		"text",		"allocatePsi");
	add(_txtTraining,		"text",		"allocatePsi");
	add(_lstSoldiers,		"list",		"allocatePsi");
	add(_btnOk,				"button",	"allocatePsi");

	centerAllSurfaces();


	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& PsiTrainingState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& PsiTrainingState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& PsiTrainingState::btnOkClick,
					Options::keyOkKeypad);
	_btnOk->onKeyboardPress(
					(ActionHandler)& PsiTrainingState::btnOkClick,
					Options::keyCancel);

	_txtTitle->setText(tr("STR_PSIONIC_TRAINING"));
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_txtBaseLabel->setText(base->getName(_game->getLanguage()));
	_txtBaseLabel->setAlign(ALIGN_RIGHT);

	_labSpace = base->getAvailablePsiLabs() - base->getUsedPsiLabs();
	_txtRemaining->setText(tr("STR_REMAINING_PSI_LAB_CAPACITY").arg(_labSpace));
	_txtRemaining->setSecondaryColor(Palette::blockOffset(13));

	_txtName->setText(tr("STR_NAME"));
	_txtPsiStrength->setText(tr("STR_PSIONIC_STRENGTH_HEADER"));
	_txtPsiSkill->setText(tr("STR_PSIONIC_SKILL_HEADER"));
	_txtTraining->setText(tr("STR_IN_TRAINING"));

	_lstSoldiers->setColumns(4, 118,48,78,34);
	_lstSoldiers->setArrowColumn(193, ARROW_VERTICAL);
	_lstSoldiers->setBackground(_window);
	_lstSoldiers->setSelectable();
	_lstSoldiers->onMousePress((ActionHandler)& PsiTrainingState::lstSoldiersPress);
	_lstSoldiers->onLeftArrowClick((ActionHandler)& PsiTrainingState::lstLeftArrowClick);
	_lstSoldiers->onRightArrowClick((ActionHandler)& PsiTrainingState::lstRightArrowClick);
}

/**
 * dTor.
 */
PsiTrainingState::~PsiTrainingState()
{}

/**
 * Resets the palette. uh, not really.
 */
void PsiTrainingState::init()
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
			woststr1, // strength
			woststr2; // skill

		if ((*i)->getCurrentStats()->psiSkill != 0)
		{
			woststr1 << (*i)->getCurrentStats()->psiStrength;
			woststr2 << (*i)->getCurrentStats()->psiSkill;
		}
		else
		{
			woststr2 << tr("STR_UNKNOWN").c_str();
			woststr1 << tr("STR_UNKNOWN").c_str();
		}

		std::wstring wst;
		Uint8 color;
		if ((*i)->inPsiTraining() == true)
		{
			wst = tr("STR_YES");
			color = _lstSoldiers->getSecondaryColor();
		}
		else
		{
			wst = tr("STR_NO");
			color = _lstSoldiers->getColor();
		}

		_lstSoldiers->addRow(
						4,
						(*i)->getName().c_str(),
						woststr1.str().c_str(),
						woststr2.str().c_str(),
						wst.c_str());
		_lstSoldiers->setRowColor(row, color);
	}

	_lstSoldiers->scrollTo(_base->getRecallRow(REC_SOLDIER));
	_lstSoldiers->draw();
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void PsiTrainingState::btnOkClick(Action*)
{
	_base->setRecallRow(
					REC_SOLDIER,
					_lstSoldiers->getScroll());
	_game->popState();
}

/**
 * LMB assigns & removes a soldier from Psi Training.
 * RMB shows soldier info.
 * @param action - pointer to an Action
 */
void PsiTrainingState::lstSoldiersPress(Action* action)
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
		if (_base->getSoldiers()->at(_sel)->inPsiTraining() == false)
		{
			if (_base->getUsedPsiLabs() < _base->getAvailablePsiLabs())
			{
				_lstSoldiers->setCellText(
										_sel,
										3,
										tr("STR_YES").c_str());
				_lstSoldiers->setRowColor(
										_sel,
										_lstSoldiers->getSecondaryColor());

				--_labSpace;
				_txtRemaining->setText(tr("STR_REMAINING_PSI_LAB_CAPACITY").arg(_labSpace));

				_base->getSoldiers()->at(_sel)->togglePsiTraining();
			}
		}
		else
		{
			_lstSoldiers->setCellText(
									_sel,
									3,
									tr("STR_NO").c_str());
			_lstSoldiers->setRowColor(
									_sel,
									_lstSoldiers->getColor());
			++_labSpace;
			_txtRemaining->setText(tr("STR_REMAINING_PSI_LAB_CAPACITY").arg(_labSpace));
			_base->getSoldiers()->at(_sel)->togglePsiTraining();
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		_base->setRecallRow(
						REC_SOLDIER,
						_lstSoldiers->getScroll());
		_game->pushState(new SoldierInfoState(_base, _sel));
		kL_soundPop->play(Mix_GroupAvailable(0));
	}
}

/**
 * Reorders a soldier up.
 * @param action - pointer to an Action
 */
void PsiTrainingState::lstLeftArrowClick(Action* action)
{
	_base->setRecallRow(
					REC_SOLDIER,
					_lstSoldiers->getScroll());

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
						static_cast<Uint16>(action->getTopBlackBand() + action->getYMouse()
												- static_cast<int>(8. * action->getYScale())));
			}
			else
			{
				_base->setRecallRow(
								REC_SOLDIER,
								_lstSoldiers->getScroll() - 1);
				_lstSoldiers->scrollUp();
			}
		}
		else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
		{
			_base->setRecallRow(
							REC_SOLDIER,
							_lstSoldiers->getScroll() + 1);

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
void PsiTrainingState::lstRightArrowClick(Action* action)
{
	_base->setRecallRow(
					REC_SOLDIER,
					_lstSoldiers->getScroll());

	const size_t
		qtySoldiers = _base->getSoldiers()->size(),
		row = _lstSoldiers->getSelectedRow();

	if (qtySoldiers > 0
		&& row < qtySoldiers - 1)
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
						static_cast<Uint16>(action->getTopBlackBand() + action->getYMouse()
												+ static_cast<int>(8. * action->getYScale())));
			}
			else
			{
				_base->setRecallRow(
								REC_SOLDIER,
								_lstSoldiers->getScroll() + 1);
				_lstSoldiers->scrollDown();
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
