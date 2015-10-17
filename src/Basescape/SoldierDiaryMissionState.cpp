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

#include "SoldierDiaryMissionState.h"

//#include <sstream>

#include "../Engine/Game.h"
//#include "../Engine/LocalizedText.h"
//#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/Base.h"
#include "../Savegame/SavedGame.h"
//#include "../Savegame/Soldier.h"
#include "../Savegame/SoldierDead.h"
#include "../Savegame/SoldierDiary.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Soldier Diary Mission-description window.
 * @param base		- pointer to the Base to get info from
 * @param soldierId	- ID of the selected soldier
 * @param entry		- listrow to get mission info from
 */
SoldierDiaryMissionState::SoldierDiaryMissionState(
		Base* const base,
		size_t soldierId,
		size_t entry)
{
	_screen = false;

	_window			= new Window(this, 252, 141, 34, 34, POPUP_BOTH);
	_txtTitle		= new Text(242, 17, 39, 42);

	_txtMissionType	= new Text(144, 9, 46, 60);
	_txtUFO			= new Text(144, 9, 46, 69);
	_txtScore		= new Text(144, 9, 46, 78);
	_txtDaysWounded	= new Text(144, 9, 46, 87);

	_txtRace		= new Text(80, 9, 188, 60);
	_txtDaylight	= new Text(80, 9, 188, 69);
	_txtKills		= new Text(80, 9, 188, 78);
	_txtPoints		= new Text(80, 9, 188, 87);

	_srfLine		= new Surface(120, 1, 100, 99);
	_lstKills		= new TextList(217, 49, 50, 101);

	_btnOk			= new TextButton(180, 16, 70, 152);

	setPalette("PAL_BASESCAPE");

	add(_window);
	add(_txtTitle);
	add(_txtMissionType);
	add(_txtUFO);
	add(_txtScore);
	add(_txtPoints);
	add(_txtRace);
	add(_txtDaylight);
	add(_txtKills);
	add(_txtDaysWounded);
	add(_srfLine);
	add(_lstKills);
	add(_btnOk);

	centerAllSurfaces();


	const std::vector<MissionStatistics*>* const stats (_game->getSavedGame()->getMissionStatistics());
	size_t missionId;
	int daysWounded;

	SoldierDiary* diary;

	if (base == NULL)
	{
		const std::vector<SoldierDead*>* const deadList (_game->getSavedGame()->getDeadSoldiers());

		if (soldierId >= deadList->size())
			soldierId = 0;

		const SoldierDead* const solDead (deadList->at(soldierId));
		diary = solDead->getDiary();

		missionId = diary->getMissionIdList().at(entry);
		if (missionId > stats->size())
			missionId = 0;

		daysWounded = stats->at(missionId)->injuryList[solDead->getId()];
	}
	else
	{
		const std::vector<Soldier*>* const liveList (base->getSoldiers());

		if (soldierId >= liveList->size())
			soldierId = 0;

		const Soldier* const sol (liveList->at(soldierId));
		diary = sol->getDiary();

		missionId = diary->getMissionIdList().at(entry);
		if (missionId > stats->size())
			missionId = 0;

		daysWounded = stats->at(missionId)->injuryList[sol->getId()];
	}


	_window->setColor(BLUE);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK14.SCR"));

	_btnOk->setColor(YELLOW);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& SoldierDiaryMissionState::btnOkClick);
	_btnOk->onKeyboardPress(
			(ActionHandler)& SoldierDiaryMissionState::btnOkClick,
			Options::keyCancel);
	_btnOk->onKeyboardPress(
			(ActionHandler)& SoldierDiaryMissionState::btnOkClick,
			Options::keyOk);
	_btnOk->onKeyboardPress(
			(ActionHandler)& SoldierDiaryMissionState::btnOkClick,
			Options::keyOkKeypad);

	_txtTitle->setColor(YELLOW);
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();
	_txtTitle->setText(tr("STR_MISSION_DETAILS"));

	_txtScore->setColor(YELLOW);
	_txtScore->setSecondaryColor(WHITE);
	_txtScore->setText(tr("STR_SCORE_VALUE").arg(stats->at(missionId)->score));

	_txtMissionType->setColor(YELLOW);
	_txtMissionType->setSecondaryColor(WHITE);
	_txtMissionType->setText(tr("STR_MISSION_TYPE").arg(tr(stats->at(missionId)->getMissionTypeLowerCase())));

	_txtUFO->setColor(YELLOW);
	_txtUFO->setSecondaryColor(WHITE);
	_txtUFO->setText(tr("STR_UFO_TYPE").arg(tr(stats->at(missionId)->ufo)));
	if (stats->at(missionId)->ufo == "NUL_UFO")
		_txtUFO->setVisible(false);

	_txtRace->setColor(YELLOW);
	_txtRace->setSecondaryColor(WHITE);
	_txtRace->setText(tr("STR_RACE_TYPE").arg(tr(stats->at(missionId)->alienRace)));
	if (stats->at(missionId)->alienRace == "STR_UNKNOWN")
		_txtRace->setVisible(false);

	if (stats->at(missionId)->type == "STR_BASE_DEFENSE"
		|| stats->at(missionId)->type == "STR_ALIEN_BASE_ASSAULT")
	{
		_txtDaylight->setVisible(false);
	}
	else
	{
		_txtDaylight->setColor(YELLOW);
		_txtDaylight->setSecondaryColor(WHITE);
		if (stats->at(missionId)->shade < 9)
			_txtDaylight->setText(tr("STR_DAYLIGHT_TYPE").arg(tr("STR_DAY")));
		else
			_txtDaylight->setText(tr("STR_DAYLIGHT_TYPE").arg(tr("STR_NIGHT")));
	}

	if (daysWounded == 0)
		_txtDaysWounded->setVisible(false);
	else
	{
		_txtDaysWounded->setColor(YELLOW);
		_txtDaysWounded->setSecondaryColor(WHITE);
		if (daysWounded == -1)
			_txtDaysWounded->setText(tr("STR_DAYS_WOUNDED").arg(tr("STR_KIA")).arg(L""));
		else if (daysWounded == -2)
			_txtDaysWounded->setText(tr("STR_DAYS_WOUNDED").arg(tr("STR_MIA")).arg(L""));
		else
			_txtDaysWounded->setText(tr("STR_DAYS_WOUNDED").arg(daysWounded).arg(L" dy"));
	}

	_srfLine->drawLine(0,0, 120,0, YELLOW + 1);

	_lstKills->setColor(WHITE);
	_lstKills->setArrowColor(YELLOW);
	_lstKills->setColumns(3, 27,96,94);
	_lstKills->setMargin();

	int
		killQty (0),
		points (0);
	size_t row (0);
//	bool stunOrKill = false;

	for (std::vector<BattleUnitKill*>::const_iterator
			i = diary->getKills().begin();
			i != diary->getKills().end();
			++i)
	{
		if ((*i)->_mission == static_cast<int>(missionId))
		{
			std::wostringstream
				strRace,
				strRank,
				strWeapon,
				strUnit,
				strStatus;
//				strAmmo;

			strRace << tr((*i)->_race.c_str());
			strRank << tr((*i)->_rank.c_str());
			strWeapon << tr((*i)->_weapon.c_str());
			strUnit << strRace.str().c_str() << " " << strRank.str().c_str();

			if ((*i)->getUnitStatusString() == "STATUS_DEAD")
			{
				strStatus << tr("STR_KILLED").c_str();
//				++killQty; // below_ Count both kills & stuns.
//				stunOrKill = true;
			}
			else
			{
				strStatus << tr("STR_STUNNED").c_str();
//				stunOrKill = true;
			}

			++killQty;
			points += (*i)->_points;

			_lstKills->addRow(
							3,
							strStatus.str().c_str(),
							strUnit.str().c_str(),
							strWeapon.str().c_str());
			_lstKills->setCellColor(row++, 0, YELLOW);
		}
	}

//	if (stunOrKill == false)
//	{
//		std::wostringstream wossKills;
//		wossKills << tr("STR_NO_KILLS");
//		_lstKills->addRow(1, wossKills.str().c_str()); // should change to/add shots-connected ...
//	}

	if (killQty != 0)
	{
		_txtKills->setColor(YELLOW);
		_txtKills->setSecondaryColor(WHITE);
		_txtKills->setText(tr("STR_MARKS").arg(killQty));
	}
	else
		_txtKills->setVisible(false);

	if (points != 0)
	{
		_txtPoints->setColor(YELLOW);
		_txtPoints->setSecondaryColor(WHITE);
		_txtPoints->setText(tr("STR_POINTS_VALUE").arg(points));
	}
	else
		_txtPoints->setVisible(false);
}

/**
 * dTor.
 */
SoldierDiaryMissionState::~SoldierDiaryMissionState()
{}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void SoldierDiaryMissionState::btnOkClick(Action*)
{
	_game->popState();
}

}
