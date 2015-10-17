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

#include "SoldierDiaryPerformanceState.h"

//#include <string>

#include "SoldierDiaryOverviewState.h"
#include "SoldierInfoState.h"
#include "SoldierInfoDeadState.h"

#include "../Engine/Action.h"
#include "../Engine/Game.h"
//#include "../Engine/LocalizedText.h"
//#include "../Engine/Options.h"
#include "../Engine/SurfaceSet.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleAward.h"
#include "../Ruleset/Ruleset.h"

#include "../Savegame/Base.h"
//#include "../Savegame/Soldier.h"
#include "../Savegame/SoldierDead.h"
#include "../Savegame/SoldierDiary.h"


namespace OpenXcom
{

/**
 * Initializes all the elements for the Performance screens.
 * @param base		- pointer to the Base to get info from
 * @param soldierId	- ID of the selected soldier
 * @param overview	- pointer to SoldierDiaryOverviewState
 * @param display	- 0 displays Kills stats
					  1 displays Missions stats
					  2 displays Awards stats
 */
SoldierDiaryPerformanceState::SoldierDiaryPerformanceState(
		Base* const base,
		const size_t soldierId,
		SoldierDiaryOverviewState* const overview,
		const int display)
	:
		_base(base),
		_soldierId(soldierId),
		_overview(overview),
		_lastScrollPos(0),
		_diary(NULL)
{
	if (_base == NULL)
	{
		_listDead = _game->getSavedGame()->getDeadSoldiers();
		_list = NULL;
	}
	else
	{
		_list = _base->getSoldiers();
		_listDead = NULL;
	}

	if (display == 0)
	{
		_displayKills = true;
		_displayMissions =
		_displayAwards = false;
	}
	else if (display == 1)
	{
		_displayMissions = true;
		_displayKills =
		_displayAwards = false;
	}
	else // display == 2
	{
		_displayAwards = true;
		_displayKills =
		_displayMissions = false;
	}

	_window				= new Window(this, 320, 200);

	_txtTitle			= new Text(310, 16, 5,  8);
	_txtBaseLabel		= new Text(310,  9, 5, 25);

	_btnPrev			= new TextButton(28, 14,   8, 8);
	_btnNext			= new TextButton(28, 14, 284, 8);

	_btnMissions		= new TextButton(70, 16,   8, 177);
	_btnKills			= new TextButton(70, 16,  86, 177);
	_btnAwards			= new TextButton(70, 16, 164, 177);
	_btnOk				= new TextButton(70, 16, 242, 177);

	// Mission stats
	_txtLocation		= new Text( 92, 16,  16, 36);
	_txtType			= new Text(114, 16, 108, 36);
	_txtUFO				= new Text( 92, 16, 222, 36);
	_lstLocation		= new TextList( 92, 113,  16,  52);
	_lstType			= new TextList(114, 113, 108,  52);
	_lstUFO				= new TextList( 92, 113, 222,  52);
	_lstMissionTotals	= new TextList(288,   9,  18, 166);

	// Kill stats
	_txtRace			= new Text(98, 16,  16, 36);
	_txtRank			= new Text(98, 16, 114, 36);
	_txtWeapon			= new Text(98, 16, 212, 36);
	_lstRace			= new TextList( 98, 113,  16,  52);
	_lstRank			= new TextList( 98, 113, 114,  52);
	_lstWeapon			= new TextList( 98, 113, 212,  52);
	_lstKillTotals		= new TextList(210,   9,  18, 166);

	// Award stats
	_txtMedalName		= new Text(90, 9,  16, 36);
	_txtMedalLevel		= new Text(52, 9, 196, 36);
	_txtMedalClass		= new Text(40, 9, 248, 36);
	_lstAwards			= new TextList(240, 97, 48, 49);
	_txtMedalInfo		= new Text(280, 25, 20, 150);


	// Award sprites
	_srtSprite = _game->getResourcePack()->getSurfaceSet("Awards");
	_srtDecor = _game->getResourcePack()->getSurfaceSet("AwardDecorations");
	for (int
			i = 0;
			i != static_cast<int>(LIST_ROWS);
			++i)
	{
		_srfSprite.push_back(new Surface(31, 7, 16, LIST_SPRITES_y + (i * 8)));
		_srfDecor.push_back(new Surface(31, 7, 16, LIST_SPRITES_y + (i * 8)));
	}

	setPalette("PAL_BASESCAPE");

	add(_window);
	add(_txtTitle);
	add(_txtBaseLabel);

	add(_btnPrev);
	add(_btnNext);

	add(_btnMissions);
	add(_btnKills);
	add(_btnAwards);

	add(_btnOk);

	// Kill stats
	add(_txtRace);
	add(_txtRank);
	add(_txtWeapon);
	add(_lstRace);
	add(_lstRank);
	add(_lstWeapon);
	add(_lstKillTotals);

	// Mission stats
	add(_txtLocation);
	add(_txtType);
	add(_txtUFO);
	add(_lstLocation);
	add(_lstType);
	add(_lstUFO);
	add(_lstMissionTotals);

	// Award stats
	add(_txtMedalName);
	add(_txtMedalLevel);
	add(_txtMedalClass);
	add(_lstAwards);
	add(_txtMedalInfo);

	// Award sprites
	for (size_t
			i = 0;
			i != LIST_ROWS;
			++i)
	{
		add(_srfSprite[i]);
		add(_srfDecor[i]);
	}

	centerAllSurfaces();


	_window->setColor(PINK);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK02.SCR"));

	_txtTitle->setColor(BLUE);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);


	_btnNext->setColor(PURPLE);
	_btnNext->setText(L">");

	_btnPrev->setColor(PURPLE);
	_btnPrev->setText(L"<");

	if (_base == NULL)
	{
		_txtBaseLabel->setVisible(false);

		_btnNext->onMouseClick((ActionHandler)& SoldierDiaryPerformanceState::btnPrevClick); // list is reversed in Memorial.
		_btnNext->onKeyboardPress(
						(ActionHandler)& SoldierDiaryPerformanceState::btnPrevClick,
						Options::keyBattleNextUnit);

		_btnPrev->onMouseClick((ActionHandler)& SoldierDiaryPerformanceState::btnNextClick);
		_btnPrev->onKeyboardPress(
						(ActionHandler)& SoldierDiaryPerformanceState::btnNextClick,
						Options::keyBattlePrevUnit);
	}
	else
	{
		_txtBaseLabel->setColor(BLUE);
		_txtBaseLabel->setAlign(ALIGN_CENTER);
		_txtBaseLabel->setText(_base->getName(_game->getLanguage()));

		_btnNext->onMouseClick((ActionHandler)& SoldierDiaryPerformanceState::btnNextClick);
		_btnNext->onKeyboardPress(
						(ActionHandler)& SoldierDiaryPerformanceState::btnNextClick,
						Options::keyBattleNextUnit);

		_btnPrev->onMouseClick((ActionHandler)& SoldierDiaryPerformanceState::btnPrevClick);
		_btnPrev->onKeyboardPress(
						(ActionHandler)& SoldierDiaryPerformanceState::btnPrevClick,
						Options::keyBattlePrevUnit);
	}


	_btnMissions->setColor(BLUE);
	_btnMissions->setText(tr("STR_MISSIONS_UC"));
	_btnMissions->onMouseClick((ActionHandler)& SoldierDiaryPerformanceState::btnMissionsToggle);

	_btnKills->setColor(BLUE);
	_btnKills->setText(tr("STR_KILLS_UC"));
	_btnKills->onMouseClick((ActionHandler)& SoldierDiaryPerformanceState::btnKillsToggle);

	_btnAwards->setColor(BLUE);
	_btnAwards->setText(tr("STR_AWARDS_UC"));
	_btnAwards->onMouseClick((ActionHandler)& SoldierDiaryPerformanceState::btnCommendationsToggle);

	_btnOk->setColor(BLUE);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& SoldierDiaryPerformanceState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& SoldierDiaryPerformanceState::btnOkClick,
					Options::keyCancel);


	// Mission stats ->
	_txtLocation->setColor(PINK);
	_txtLocation->setText(tr("STR_MISSIONS_BY_LOCATION"));

	_txtType->setColor(PINK);
	_txtType->setText(tr("STR_MISSIONS_BY_TYPE"));

	_txtUFO->setColor(PINK);
	_txtUFO->setText(tr("STR_MISSIONS_BY_UFO"));

	_lstLocation->setColor(WHITE);
	_lstLocation->setArrowColor(PINK);
	_lstLocation->setColumns(2, 80,12);
	_lstLocation->setMargin();

	_lstType->setColor(WHITE);
	_lstType->setArrowColor(PINK);
	_lstType->setColumns(2, 100,14);
	_lstType->setMargin();

	_lstUFO->setColor(WHITE);
	_lstUFO->setArrowColor(PINK);
	_lstUFO->setColumns(2, 80,12);
	_lstUFO->setMargin();

	_lstMissionTotals->setColor(YELLOW);
	_lstMissionTotals->setSecondaryColor(WHITE);
	_lstMissionTotals->setColumns(4, 70,70,70,78);
	_lstMissionTotals->setMargin();


	// Kill stats ->
	_txtRace->setColor(PINK);
	_txtRace->setText(tr("STR_KILLS_BY_RACE"));

	_txtRank->setColor(PINK);
	_txtRank->setText(tr("STR_KILLS_BY_RANK"));

	_txtWeapon->setColor(PINK);
	_txtWeapon->setText(tr("STR_KILLS_BY_WEAPON"));

	_lstRace->setColor(WHITE);
	_lstRace->setArrowColor(PINK);
	_lstRace->setColumns(2, 80,18);
	_lstRace->setMargin();

	_lstRank->setColor(WHITE);
	_lstRank->setArrowColor(PINK);
	_lstRank->setColumns(2, 80,18);
	_lstRank->setMargin();

	_lstWeapon->setColor(WHITE);
	_lstWeapon->setArrowColor(PINK);
	_lstWeapon->setColumns(2, 80,18);
	_lstWeapon->setMargin();

	_lstKillTotals->setColor(YELLOW);
	_lstKillTotals->setSecondaryColor(WHITE);
	_lstKillTotals->setColumns(3, 70,70,70);
	_lstKillTotals->setMargin();


	// Award stats ->
	_txtMedalName->setColor(PINK);
	_txtMedalName->setText(tr("STR_MEDAL_NAME"));

	_txtMedalLevel->setColor(PINK);
	_txtMedalLevel->setText(tr("STR_MEDAL_DECOR_LEVEL"));

	_txtMedalClass->setColor(PINK);
	_txtMedalClass->setText(tr("STR_MEDAL_DECOR_CLASS"));

	_lstAwards->setColor(WHITE);
	_lstAwards->setArrowColor(PINK);
	_lstAwards->setColumns(3, 148,52,40);
	_lstAwards->setBackground(_window);
	_lstAwards->setSelectable();
	_lstAwards->setMargin();
	_lstAwards->onMouseOver((ActionHandler)& SoldierDiaryPerformanceState::lstInfoMouseOver);
	_lstAwards->onMouseOut((ActionHandler)& SoldierDiaryPerformanceState::lstInfoMouseOut);
	_lstAwards->onMousePress((ActionHandler)& SoldierDiaryPerformanceState::handle);

	_txtMedalInfo->setColor(BROWN);
	_txtMedalInfo->setHighContrast();
	_txtMedalInfo->setWordWrap();
}

/**
 * dTor.
 */
SoldierDiaryPerformanceState::~SoldierDiaryPerformanceState()
{}

/**
 *  Clears all the variables and reinitializes the list of kills or missions for the soldier.
 */
void SoldierDiaryPerformanceState::init()
{
	State::init();

	for (size_t // clear sprites
			i = 0;
			i != LIST_ROWS;
			++i)
	{
		_srfSprite[i]->clear();
		_srfDecor[i]->clear();
	}

	_lstRank->scrollTo(0); // reset scroll depth for lists
	_lstRace->scrollTo(0);
	_lstWeapon->scrollTo(0);
	_lstKillTotals->scrollTo(0);
	_lstLocation->scrollTo(0);
	_lstType->scrollTo(0);
	_lstUFO->scrollTo(0);
	_lstMissionTotals->scrollTo(0);
	_lstAwards->scrollTo(0);
	_lastScrollPos = 0;

	_txtRace->setVisible(_displayKills); // set visibility for Kill stats
	_txtRank->setVisible(_displayKills);
	_txtWeapon->setVisible(_displayKills);
	_lstRace->setVisible(_displayKills);
	_lstRank->setVisible(_displayKills);
	_lstWeapon->setVisible(_displayKills);
	_lstKillTotals->setVisible(_displayKills);

	_txtLocation->setVisible(_displayMissions); // set visibility for Mission stats
	_txtType->setVisible(_displayMissions);
	_txtUFO->setVisible(_displayMissions);
	_lstLocation->setVisible(_displayMissions);
	_lstType->setVisible(_displayMissions);
	_lstUFO->setVisible(_displayMissions);
	_lstMissionTotals->setVisible(_displayMissions);

	if (_game->getRuleset()->getAwardsList().empty() == true) // set visibility for Award stats
		_displayAwards = false;

	_txtMedalName->setVisible(_displayAwards);
	_txtMedalLevel->setVisible(_displayAwards);
	_txtMedalClass->setVisible(_displayAwards);
	_lstAwards->setVisible(_displayAwards);
	_txtMedalInfo->setVisible(_displayAwards);

	if (_displayKills == true)
		_btnKills->setColor(YELLOW);
	else
		_btnKills->setColor(BLUE);

	if (_displayMissions == true)
		_btnMissions->setColor(YELLOW);
	else
		_btnMissions->setColor(BLUE);

	if (_displayAwards == true)
		_btnAwards->setColor(YELLOW);
	else
		_btnAwards->setColor(BLUE);


	_awardsListEntry.clear();
	_lstKillTotals->clearList();
	_lstMissionTotals->clearList();

	_lstRace->clearList();
	_lstRank->clearList();
	_lstWeapon->clearList();
	_lstLocation->clearList();
	_lstType->clearList();
	_lstUFO->clearList();
	_lstAwards->clearList();

	if (_base == NULL)
	{
		if (_soldierId >= _listDead->size())
			_soldierId = 0;

		const SoldierDead* const deadSoldier = _listDead->at(_soldierId);
		_diary = deadSoldier->getDiary();

		_txtTitle->setText(deadSoldier->getName());
	}
	else
	{
		if (_soldierId >= _list->size())
			_soldierId = 0;

		const Soldier* const soldier = _list->at(_soldierId);
		_diary = soldier->getDiary();

		_txtTitle->setText(soldier->getName());
	}


	if (_diary == NULL) // safety.
		return;


	std::wstring
		wst1,
		wst2,
		wst3,
		wst4;

	if (_diary->getMissionTotal() != 0) // Mission stats ->
		wst1 = tr("STR_MISSIONS").arg(_diary->getMissionTotal());
	if (_diary->getWinTotal() != 0)
		wst2 = tr("STR_WINS").arg(_diary->getWinTotal());
	if (_diary->getScoreTotal() != 0)
		wst3 = tr("STR_SCORE_VALUE").arg(_diary->getScoreTotal());
	if (_diary->getDaysWoundedTotal() != 0)
		wst4 = tr("STR_DAYS_WOUNDED").arg(_diary->getDaysWoundedTotal()).arg(L" dy");

	_lstMissionTotals->addRow(
						4,
						wst1.c_str(),
						wst2.c_str(),
						wst3.c_str(),
						wst4.c_str());

	wst1 =
	wst2 =
	wst3 = L"";

	if (_diary->getKillTotal() != 0) // Kill stats ->
		wst1 = tr("STR_KILLS").arg(_diary->getKillTotal());
	if (_diary->getStunTotal() != 0)
		wst2 = tr("STR_STUNS").arg(_diary->getStunTotal());
	if (_diary->getScorePoints() != 0)
		wst3 = tr("STR_SCORE_VALUE").arg(_diary->getScorePoints());

	_lstKillTotals->addRow(
						3,
						wst1.c_str(),
						wst2.c_str(),
						wst3.c_str());


	const size_t lstRows (6);
	TextList* const lstArray[lstRows] = // Kill & Mission stats ->
	{
		_lstRace,
		_lstRank,
		_lstWeapon,
		_lstLocation,
		_lstType,
		_lstUFO
	};

	const std::map<std::string, int> mapArray[lstRows] =
	{
		_diary->getAlienRaceTotal(),
		_diary->getAlienRankTotal(),
		_diary->getWeaponTotal(),
		_diary->getRegionTotal(),
		_diary->getTypeTotal(),
		_diary->getUfoTotal()
	};

	for (size_t
			i = 0;
			i != lstRows;
			++i)
	{
		size_t row (0);

		for (std::map<std::string, int>::const_iterator
				j = mapArray[i].begin();
				j != mapArray[i].end();
				++j)
		{
			if ((*j).first != "NUL_UFO")
			{
				std::wstringstream
					woststr1,
					woststr2;

				woststr1 << tr((*j).first.c_str());
				woststr2 << (*j).second;

				lstArray[i]->addRow(
								2,
								woststr1.str().c_str(),
								woststr2.str().c_str());
				lstArray[i]->setCellColor(row++, 0, YELLOW);
			}
		}
	}


	for (std::vector<SoldierAward*>::const_iterator // Award stats ->
			i = _diary->getSoldierAwards()->begin();
			i != _diary->getSoldierAwards()->end();
			++i)
	{
		if (_game->getRuleset()->getAwardsList().empty() == true)
			break;

		const RuleAward* const awardRule (_game->getRuleset()->getAwardsList()[(*i)->getType()]);
		std::wstringstream
			woststr1,
			woststr2,
			woststr3,
			woststr4;

		if ((*i)->getQualifier() != "noQual")
		{
			woststr1 << tr((*i)->getType().c_str()).arg(tr((*i)->getQualifier()).c_str());
			woststr4 << tr(awardRule->getDescription().c_str()).arg(tr((*i)->getQualifier()).c_str());
		}
		else
		{
			woststr1 << tr((*i)->getType().c_str());
			woststr4 << tr(awardRule->getDescription().c_str());
		}

		woststr2 << tr((*i)->getClassDescription().c_str());
		woststr3 << tr((*i)->getClassDegree().c_str());

		_lstAwards->addRow(
						3,
						woststr1.str().c_str(),
						woststr2.str().c_str(),
						woststr3.str().c_str());

		_awardsListEntry.push_back(woststr4.str().c_str());

		drawSprites();
	}
}

/**
 * Draws sprites.
 */
void SoldierDiaryPerformanceState::drawSprites()
{
	if (_displayAwards == false)
		return;

	for (size_t
			i = 0;
			i != LIST_ROWS;
			++i)
	{
		_srfSprite[i]->clear();
		_srfDecor[i]->clear();
	}

	const RuleAward* awardRule;
	int sprite;
	const size_t scroll = _lstAwards->getScroll();
	size_t j = 0;

	for (std::vector<SoldierAward*>::const_iterator
			i = _diary->getSoldierAwards()->begin();
			i != _diary->getSoldierAwards()->end();
			++i,
				++j)
	{
		if (j >= scroll // show awards that are visible on the list
			&& j - scroll < _srfSprite.size())
		{
			awardRule = _game->getRuleset()->getAwardsList()[(*i)->getType()]; // handle award sprites
			sprite = awardRule->getSprite();
			_srtSprite->getFrame(sprite)->blit(_srfSprite[j - scroll]);

			sprite = static_cast<int>((*i)->getClassLevel()); // handle award decoration sprites
			if (sprite != 0)
				_srtDecor->getFrame(sprite)->blit(_srfDecor[j - scroll]);
		}
	}
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void SoldierDiaryPerformanceState::btnOkClick(Action*)
{
	_overview->setSoldierId(_soldierId);
	_game->popState();
}

/**
 * Goes to the previous soldier.
 * @param action - pointer to an Action
 */
void SoldierDiaryPerformanceState::btnPrevClick(Action*)
{
	size_t rows;
	if (_base == NULL)
		rows = _listDead->size();
	else
		rows = _list->size();

	if (_soldierId == 0)
		_soldierId = rows - 1;
	else
		--_soldierId;

	init();
}

/**
 * Goes to the next soldier.
 * @param action - pointer to an Action
 */
void SoldierDiaryPerformanceState::btnNextClick(Action*)
{
	size_t rows;
	if (_base == NULL)
		rows = _listDead->size();
	else
		rows = _list->size();

	if (++_soldierId >= rows)
		_soldierId = 0;

	init();
}

/**
 * Display Kills totals.
 */
void SoldierDiaryPerformanceState::btnKillsToggle(Action*)
{
	_displayKills = true;
	_displayMissions =
	_displayAwards = false;

	init();
}

/**
 * Display Missions totals.
 */
void SoldierDiaryPerformanceState::btnMissionsToggle(Action*)
{
	_displayMissions = true;
	_displayKills =
	_displayAwards = false;

	init();
}

/**
 * Display Commendations.
 */
void SoldierDiaryPerformanceState::btnCommendationsToggle(Action*)
{
	_displayAwards = true;
	_displayKills =
	_displayMissions = false;

	init();
}

/**
 *
 */
void SoldierDiaryPerformanceState::lstInfoMouseOver(Action*)
{
	const size_t row = _lstAwards->getSelectedRow();

	if (_awardsListEntry.empty() == true
		|| row > _awardsListEntry.size() - 1)
	{
		_txtMedalInfo->setText(L"");
	}
	else
		_txtMedalInfo->setText(_awardsListEntry[row]);
}

/**
 * Clears the Medal information.
 */
void SoldierDiaryPerformanceState::lstInfoMouseOut(Action*)
{
	_txtMedalInfo->setText(L"");
}

/**
 * Runs state functionality every cycle.
 * @note Used to update sprite vector.
 */
void SoldierDiaryPerformanceState::think()
{
	State::think();

	if (_lastScrollPos != _lstAwards->getScroll())
	{
		_lastScrollPos = _lstAwards->getScroll();
		drawSprites();
	}
}

}
