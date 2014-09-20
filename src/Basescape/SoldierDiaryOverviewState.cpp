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

#include "SoldierDiaryOverviewState.h"

#include <string>

#include "SoldierDiaryMissionState.h"
#include "SoldierDiaryPerformanceState.h"
#include "SoldierInfoState.h"
#include "SoldierInfoDeadState.h" // kL

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/Base.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/SoldierDead.h" // kL
#include "../Savegame/SoldierDiary.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Soldiers screen.
 * @param base					- pointer to the base to get info from
 * @param soldierId				- ID of the selected soldier
 * @param soldierInfoState		-
 * @param soldierInfoDeadState	-
 */
SoldierDiaryOverviewState::SoldierDiaryOverviewState(
		Base* base,
		size_t soldierID,
		SoldierInfoState* soldierInfoState,
		SoldierInfoDeadState* soldierInfoDeadState) // kL
	:
		_base(base),
		_soldierID(soldierID),
		_soldierInfoState(soldierInfoState),
		_soldierInfoDeadState(soldierInfoDeadState),
		_soldier(NULL),
		_soldierDead(NULL),
		_curRow(0)
{
	if (_base == NULL)
	{
		_listDead = _game->getSavedGame()->getDeadSoldiers(); // kL
		_list = NULL;
	}
	else
	{
		_list = _base->getSoldiers();
		_listDead = NULL;
	}

	_window			= new Window(this, 320, 200, 0, 0);
	_txtTitle		= new Text(310, 16, 5, 8);
	_txtBaseLabel	= new Text(310, 9, 5, 25);

	_btnPrev		= new TextButton(28, 14, 8, 8);
	_btnNext		= new TextButton(28, 14, 284, 8);

	_txtLocation	= new Text(94, 9, 16, 36);
	_txtStatus		= new Text(108, 9, 110, 36);
	_txtDate		= new Text(90, 9, 218, 36);

	_lstDiary		= new TextList(288, 121, 16, 44);

	_btnKills		= new TextButton(70, 16, 8, 177);
	_btnMissions	= new TextButton(70, 16, 86, 177);
	_btnAwards		= new TextButton(70, 16, 164, 177);
	_btnOk			= new TextButton(70, 16, 242, 177);

	setPalette("PAL_BASESCAPE");

	add(_window);
	add(_txtTitle);
	add(_txtBaseLabel);

	add(_btnPrev);
	add(_btnNext);

	add(_txtLocation);
	add(_txtStatus);
	add(_txtDate);

	add(_lstDiary);

	add(_btnKills);
	add(_btnMissions);
	add(_btnAwards);
	add(_btnOk);

	centerAllSurfaces();

	_window->setColor(Palette::blockOffset(15)+1);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK02.SCR"));

	_txtTitle->setColor(Palette::blockOffset(13)+10);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);

	if (_base != NULL)
	{
		_txtBaseLabel->setColor(Palette::blockOffset(13)+10);
		_txtBaseLabel->setAlign(ALIGN_CENTER);
		_txtBaseLabel->setText(_base->getName(_game->getLanguage()));
	}
	else
		_txtBaseLabel->setVisible(false);

	_btnPrev->setColor(Palette::blockOffset(15)+6);
	_btnPrev->setText(L"<");
	if (_base == NULL)
	{
		_btnPrev->onMouseClick((ActionHandler)& SoldierDiaryOverviewState::btnNextClick);
		_btnPrev->onKeyboardPress(
							(ActionHandler)& SoldierDiaryOverviewState::btnNextClick,
							Options::keyBattlePrevUnit);
	}
	else
	{
		_btnPrev->onMouseClick((ActionHandler)& SoldierDiaryOverviewState::btnPrevClick);
		_btnPrev->onKeyboardPress(
						(ActionHandler)& SoldierDiaryOverviewState::btnPrevClick,
						Options::keyBattlePrevUnit);
	}

	_btnNext->setColor(Palette::blockOffset(15)+6);
	_btnNext->setText(L">");
	if (_base == NULL)
	{
		_btnNext->onMouseClick((ActionHandler)& SoldierDiaryOverviewState::btnPrevClick);
		_btnNext->onKeyboardPress(
						(ActionHandler)& SoldierDiaryOverviewState::btnPrevClick,
						Options::keyBattleNextUnit);
	}
	else
	{
		_btnNext->onMouseClick((ActionHandler)& SoldierDiaryOverviewState::btnNextClick);
		_btnNext->onKeyboardPress(
						(ActionHandler)& SoldierDiaryOverviewState::btnNextClick,
						Options::keyBattleNextUnit);
	}

	_txtLocation->setColor(Palette::blockOffset(15)+1);
	_txtLocation->setText(tr("STR_LOCATION"));

	_txtStatus->setColor(Palette::blockOffset(15)+1);
	_txtStatus->setText(tr("STR_STATUS"));

	_txtDate->setColor(Palette::blockOffset(15)+1);
	_txtDate->setText(tr("STR_DATE_UC"));

	_lstDiary->setColor(Palette::blockOffset(13));
	_lstDiary->setArrowColor(Palette::blockOffset(15)+1);
	_lstDiary->setColumns(5, 94, 108, 26, 23, 29);
	_lstDiary->setSelectable();
	_lstDiary->setBackground(_window);
	_lstDiary->setMargin();
	_lstDiary->onMouseClick((ActionHandler)& SoldierDiaryOverviewState::lstDiaryInfoClick);

	_btnKills->setColor(Palette::blockOffset(13)+10);
	_btnKills->setText(tr("STR_KILLS_UC"));
	_btnKills->onMouseClick((ActionHandler)& SoldierDiaryOverviewState::btnKillsClick);

	_btnMissions->setColor(Palette::blockOffset(13)+10);
	_btnMissions->setText(tr("STR_MISSIONS_UC"));
	_btnMissions->onMouseClick((ActionHandler)& SoldierDiaryOverviewState::btnMissionsClick);

	_btnAwards->setColor(Palette::blockOffset(13)+10);
	_btnAwards->setText(tr("STR_AWARDS_UC"));
	if (!_game->getRuleset()->getCommendation().empty())
	{
		_btnAwards->onMouseClick((ActionHandler)& SoldierDiaryOverviewState::btnCommendationsClick);
		_btnAwards->setVisible();
	}
	else
		_btnAwards->setVisible(false);

	_btnOk->setColor(Palette::blockOffset(13)+10);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& SoldierDiaryOverviewState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& SoldierDiaryOverviewState::btnOkClick,
					Options::keyCancel);

	init(); // Populate the list
}

/**
 * dTor.
 */
SoldierDiaryOverviewState::~SoldierDiaryOverviewState()
{
}

/**
 *  Clears all the variables and reinitializes the list of medals for the soldier.
 *
 */
void SoldierDiaryOverviewState::init()
{
	State::init();

	if (_base == NULL)
	{
		if (_listDead->empty())
		{
			_game->popState();

			return;
		}

		if (_soldierID >= _listDead->size())
			_soldierID = 0;

		_soldierDead = _listDead->at(_soldierID);
		_txtTitle->setText(_soldierDead->getName());
	}
	else
	{
		if (_list->empty())
		{
			_game->popState();

			return;
		}

		if (_soldierID >= _list->size())
			_soldierID = 0;

		_soldier = _list->at(_soldierID);
		_txtTitle->setText(_soldier->getName());
	}


	_lstDiary->clearList();

	std::vector<MissionStatistics*>* missionStatistics = _game->getSavedGame()->getMissionStatistics();
	size_t row = 0;

	for (std::vector<MissionStatistics*>::reverse_iterator
			j = missionStatistics->rbegin();
			j != missionStatistics->rend();
			++j)
	{
		int missionId = (*j)->id;
		bool wasOnMission = false;

		if (_base == NULL)
		{
			if (_soldierDead->getDiary() != NULL) // kL: dead soldiers are initialized w/ NULL diaries ...
			{
				for (std::vector<int>::const_iterator
						k = _soldierDead->getDiary()->getMissionIdList().begin();
						k != _soldierDead->getDiary()->getMissionIdList().end();
						++k)
				{
					if (missionId == *k)
					{
						wasOnMission = true;

						break;
					}
				}
			}
		}
		else
		{
			for (std::vector<int>::const_iterator
					k = _soldier->getDiary()->getMissionIdList().begin();
					k != _soldier->getDiary()->getMissionIdList().end();
					++k)
			{
				if (missionId == *k)
				{
					wasOnMission = true;

					break;
				}
			}
		}

		if (!wasOnMission)
			continue;

		// See if this mission is part of the soldier's vector of missions
		std::wstringstream
			wssSuccess,
			wssRating,
			wssScore,

			wssRegion,
			wssCountry,

			wssHour,
			wssMinute,
			wssDay,
			wssMonth,
			wssYear;

		if ((*j)->success)
			wssSuccess << tr("STR_MISSION_WIN");
		else
			wssSuccess << tr("STR_MISSION_LOSS");

		wssRating << tr((*j)->rating.c_str());
		wssScore << (*j)->score;

		wssRegion << tr((*j)->region.c_str());
		wssCountry << tr((*j)->country.c_str());

		wssMonth << tr((*j)->time.getMonthString().c_str());
		wssDay << (*j)->time.getDayString(_game->getLanguage());
		wssYear << (*j)->time.getYear();

		std::wstringstream
			wssStatus,
			wssLocation;

		if ((*j)->country == "STR_UNKNOWN")
			wssLocation << wssRegion.str().c_str();
		else
			wssLocation << wssCountry.str().c_str();

		wssStatus << wssSuccess.str().c_str() << " - " << wssRating.str().c_str();

		_lstDiary->addRow(
						5,
						wssLocation.str().c_str(),
						wssStatus.str().c_str(),
						wssDay.str().c_str(),
						wssMonth.str().c_str(),
						wssYear.str().c_str());

		row++;
	}

	if (row > 0
		&& _lstDiary->getScroll() >= row)
	{
		_lstDiary->scrollTo(0);
	}
	else if (_curRow > 0)
		_lstDiary->scrollTo(_curRow);
}

/**
 * Set the soldier's ID.
 */
void SoldierDiaryOverviewState::setSoldierID(size_t soldierID)
{
	_soldierID = soldierID;
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void SoldierDiaryOverviewState::btnOkClick(Action*)
{
	if (_base == NULL)
		_soldierInfoDeadState->setSoldierID(_soldierID);
	else
		_soldierInfoState->setSoldierID(_soldierID);

	_game->popState();
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void SoldierDiaryOverviewState::btnKillsClick(Action*)
{
	_curRow = _lstDiary->getScroll();

	int _display = 0;
	_game->pushState(new SoldierDiaryPerformanceState(
												_base,
												_soldierID,
												this,
												_display));
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void SoldierDiaryOverviewState::btnMissionsClick(Action*)
{
	_curRow = _lstDiary->getScroll();

	int _display = 1;
	_game->pushState(new SoldierDiaryPerformanceState(
												_base,
												_soldierID,
												this,
												_display));
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void SoldierDiaryOverviewState::btnCommendationsClick(Action*)
{
	_curRow = _lstDiary->getScroll();

	int _display = 2;
	_game->pushState(new SoldierDiaryPerformanceState(
												_base,
												_soldierID,
												this,
												_display));
}

/**
 * Goes to the previous soldier.
 * @param action Pointer to an action.
 */
void SoldierDiaryOverviewState::btnPrevClick(Action*)
{
	_curRow = 0;

	size_t rows;
	if (_base == NULL)
		rows = _listDead->size();
	else
		rows = _list->size();

	if (_soldierID == 0)
		_soldierID = rows - 1;
	else
		_soldierID--;

	init();
}

/**
 * Goes to the next soldier.
 * @param action Pointer to an action.
 */
void SoldierDiaryOverviewState::btnNextClick(Action*)
{
	_curRow = 0;

	size_t rows;
	if (_base == NULL)
		rows = _listDead->size();
	else
		rows = _list->size();

	_soldierID++;
	if (_soldierID >= rows)
		_soldierID = 0;

	init();
}

/**
 * Shows the selected soldier's info.
 * @param action Pointer to an action.
 */
void SoldierDiaryOverviewState::lstDiaryInfoClick(Action*)
{
//	if (_lstDiary->getRows() > 0)
//	{
	_curRow = _lstDiary->getScroll();

	size_t row = _lstDiary->getRows() - _lstDiary->getSelectedRow() - 1;
	_game->pushState(new SoldierDiaryMissionState(
												_base,
												_soldierID,
												row));
//	}
}

}
