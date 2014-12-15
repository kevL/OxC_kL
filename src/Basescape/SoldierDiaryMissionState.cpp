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

#include "SoldierDiaryMissionState.h"

//#include <sstream>

#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/Base.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/SoldierDead.h"
#include "../Savegame/SoldierDiary.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Soldier Diary Mission-description window.
 * @param base		- pointer to the Base to get info from
 * @param soldierId	- ID of the selected soldier
 * @param rowEntry	- listrow to get mission info from
 */
SoldierDiaryMissionState::SoldierDiaryMissionState(
		Base* const base,
		const size_t soldierId,
		const size_t rowEntry)
	:
		_base(base),
		_soldierId(soldierId),
		_rowEntry(rowEntry)
{
	_screen = false;

	_window			= new Window(this, 252, 133, 34, 36, POPUP_BOTH);
	_txtTitle		= new Text(242, 17, 39, 44);

	_txtMissionType	= new Text(144, 9, 46, 62);
	_txtUFO			= new Text(144, 9, 46, 71);
	_txtScore		= new Text(144, 9, 46, 80);

	_txtRace		= new Text(80, 9, 188, 62);
	_txtDaylight	= new Text(80, 9, 188, 71);
	_txtKills		= new Text(80, 9, 188, 80);

	_txtDaysWounded	= new Text(100, 9, 100, 89);

	_lstKills		= new TextList(217, 41, 50, 101);

	_btnOk			= new TextButton(180, 16, 70, 146);

	setPalette("PAL_BASESCAPE");

	add(_window);
	add(_btnOk);
	add(_txtTitle);
	add(_txtScore);
	add(_txtKills);
	add(_txtMissionType);
	add(_txtUFO);
	add(_txtRace);
	add(_txtDaylight);
	add(_txtDaysWounded);
	add(_lstKills);

	centerAllSurfaces();


	const std::vector<MissionStatistics*>* const missionStatistics = _game->getSavedGame()->getMissionStatistics();
	size_t missionId;
	int daysWounded;

	SoldierDiary* diary;

	if (_base == NULL)
	{
		const std::vector<SoldierDead*>* const deadList = _game->getSavedGame()->getDeadSoldiers();

/*		if (deadList->empty() == true)
		{
			_game->popState();
			return;
		} */ // should never happen. Btn won't be visible if listDead is empty.

		if (_soldierId >= deadList->size())
			_soldierId = 0;

		const SoldierDead* const deadSoldier = deadList->at(_soldierId);
		diary = deadSoldier->getDiary();

		missionId = diary->getMissionIdList().at(_rowEntry);
		if (missionId > static_cast<int>(missionStatistics->size()))
			missionId = 0;

		daysWounded = missionStatistics->at(missionId)->injuryList[deadSoldier->getId()];
	}
	else
	{
		const std::vector<Soldier*>* const liveList = _base->getSoldiers();

/*		if (liveList->empty() == true)
		{
			_game->popState();
			return;
		} */ // should never happen. Btn won't be visible unless viewing at least one soldier.

		if (_soldierId >= liveList->size())
			_soldierId = 0;

		const Soldier* const soldier = liveList->at(_soldierId);
		diary = soldier->getDiary();

		missionId = diary->getMissionIdList().at(_rowEntry);
		if (missionId > static_cast<int>(missionStatistics->size()))
			missionId = 0;

		daysWounded = missionStatistics->at(missionId)->injuryList[soldier->getId()];
	}


	_window->setColor(Palette::blockOffset(13)+10);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK14.SCR"));

	_btnOk->setColor(Palette::blockOffset(13)+5);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& SoldierDiaryMissionState::btnOkClick);
	_btnOk->onKeyboardPress(
			(ActionHandler)& SoldierDiaryMissionState::btnOkClick,
			Options::keyCancel);

	_txtTitle->setColor(Palette::blockOffset(13)+5);
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_MISSION_DETAILS"));

	_txtScore->setColor(Palette::blockOffset(13)+5);
	_txtScore->setSecondaryColor(Palette::blockOffset(13));
	_txtScore->setAlign(ALIGN_LEFT);
	_txtScore->setText(tr("STR_SCORE_VALUE").arg(missionStatistics->at(missionId)->score));

	_txtMissionType->setColor(Palette::blockOffset(13)+5);
	_txtMissionType->setSecondaryColor(Palette::blockOffset(13));
	_txtMissionType->setText(tr("STR_MISSION_TYPE").arg(tr(missionStatistics->at(missionId)->getMissionTypeLowerCase())));

	_txtUFO->setColor(Palette::blockOffset(13)+5);
	_txtUFO->setSecondaryColor(Palette::blockOffset(13));
	_txtUFO->setText(tr("STR_UFO_TYPE").arg(tr(missionStatistics->at(missionId)->ufo)));
//	_txtUFO->setVisible();
	if (missionStatistics->at(missionId)->ufo == "NO_UFO")
		_txtUFO->setVisible(false);

	_txtRace->setColor(Palette::blockOffset(13)+5);
	_txtRace->setSecondaryColor(Palette::blockOffset(13));
	_txtRace->setText(tr("STR_RACE_TYPE").arg(tr(missionStatistics->at(missionId)->alienRace)));
//	_txtRace->setVisible();
	if (missionStatistics->at(missionId)->alienRace == "STR_UNKNOWN")
		_txtRace->setVisible(false);

	_txtDaylight->setColor(Palette::blockOffset(13)+5);
	_txtDaylight->setSecondaryColor(Palette::blockOffset(13));
	if (missionStatistics->at(missionId)->shade < 9)
		_txtDaylight->setText(tr("STR_DAYLIGHT_TYPE").arg(tr("STR_DAY")));
	else
		_txtDaylight->setText(tr("STR_DAYLIGHT_TYPE").arg(tr("STR_NIGHT")));

	if (daysWounded == 0)
		_txtDaysWounded->setVisible(false);
	else
	{
		_txtDaysWounded->setColor(Palette::blockOffset(13)+5);
		_txtDaysWounded->setSecondaryColor(Palette::blockOffset(13));
		_txtDaysWounded->setAlign(ALIGN_CENTER);
		_txtDaysWounded->setText(tr("STR_DAYS_WOUNDED").arg(daysWounded));
	}

	_lstKills->setColor(Palette::blockOffset(13));
	_lstKills->setArrowColor(Palette::blockOffset(13)+5);
	_lstKills->setColumns(3, 27, 96, 94); // 217 total
	_lstKills->setSelectable(false);
	_lstKills->setBackground(_window);
//	_lstKills->setMargin();

	int killQty = 0;
	size_t row = 0;
//	bool stunOrKill = false;

	for (std::vector<BattleUnitKills*>::const_iterator
			j = diary->getKills().begin();
			j != diary->getKills().end();
			++j)
	{
		if ((*j)->_mission == missionId)
		{
			std::wostringstream
				wossRace,
				wossRank,
				wossWeapon,
				wossUnit,
				wossStatus;
//				wossAmmo;

			wossRace << tr((*j)->_race.c_str());
			wossRank << tr((*j)->_rank.c_str());
			wossWeapon << tr((*j)->_weapon.c_str());
			wossUnit << wossRace.str().c_str() << " " << wossRank.str().c_str();

			if ((*j)->getUnitStatusString() == "STATUS_DEAD")
			{
				wossStatus << tr("STR_KILLED").c_str();
				++killQty;
//				stunOrKill = true;
			}
			else
			{
				wossStatus << tr("STR_STUNNED").c_str();
//				stunOrKill = true;
			}

			_lstKills->addRow(
							3,
							wossStatus.str().c_str(),
							wossUnit.str().c_str(),
							wossWeapon.str().c_str());
			_lstKills->setCellColor(row++, 0, Palette::blockOffset(13)+5);
		}
	}

//	if (stunOrKill == false)
//	{
//		std::wostringstream wossKills;
//		wossKills << tr("STR_NO_KILLS");
//		_lstKills->addRow(1, wossKills.str().c_str()); // should change to/add shots-connected ...
//	}

	_txtKills->setColor(Palette::blockOffset(13)+5);
	_txtKills->setSecondaryColor(Palette::blockOffset(13));
	_txtKills->setAlign(ALIGN_LEFT);
	_txtKills->setText(tr("STR_KILLS").arg(killQty));
}

/**
 * dTor.
 */
SoldierDiaryMissionState::~SoldierDiaryMissionState()
{
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void SoldierDiaryMissionState::btnOkClick(Action*)
{
	_game->popState();
}

}
