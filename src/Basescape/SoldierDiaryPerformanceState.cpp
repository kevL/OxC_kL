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
#include "../Engine/Language.h"
//#include "../Engine/Palette.h"
//#include "../Engine/Options.h"
#include "../Engine/SurfaceSet.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleCommendations.h"

#include "../Savegame/Base.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/SoldierDead.h"
#include "../Savegame/SoldierDiary.h"


namespace OpenXcom
{

/**
 * Initializes all the elements for the Performance screens.
 * @param base						- pointer to the Base to get info from
 * @param soldierId					- ID of the selected soldier
 * @param soldierDiaryOverviewState	- pointer to SoldierDiaryOverviewState
 * @param display					- 0 displays Kills stats
									  1 displays Missions stats
									  2 displays Awards stats
 */
SoldierDiaryPerformanceState::SoldierDiaryPerformanceState(
		Base* const base,
		const size_t soldierID,
		SoldierDiaryOverviewState* const soldierDiaryOverviewState,
		const int display)
	:
		_base(base),
		_soldierID(soldierID),
		_soldierDiaryOverviewState(soldierDiaryOverviewState),
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
		_displayMissions = false;
		_displayAwards = false;
	}
	else if (display == 1)
	{
		_displayKills = false;
		_displayMissions = true;
		_displayAwards = false;
	}
	else // display == 2
	{
		_displayKills = false;
		_displayMissions = false;
		_displayAwards = true;
	}

	_window				= new Window(this, 320, 200, 0, 0);
	_txtTitle			= new Text(310, 16, 5, 8);
	_txtBaseLabel		= new Text(310, 9, 5, 25);

	_btnPrev			= new TextButton(28, 14, 8, 8);
	_btnNext			= new TextButton(28, 14, 284, 8);

	_btnMissions		= new TextButton(70, 16, 8, 177);
	_btnKills			= new TextButton(70, 16, 86, 177);
	_btnAwards			= new TextButton(70, 16, 164, 177);
	_btnOk				= new TextButton(70, 16, 242, 177);

	// Mission stats
	_txtLocation		= new Text(92, 16, 16, 36);
	_txtType			= new Text(114, 16, 108, 36);
	_txtUFO				= new Text(92, 16, 222, 36);
	_lstLocation		= new TextList(92, 113, 16, 52);
	_lstType			= new TextList(114, 113, 108, 52);
	_lstUFO				= new TextList(92, 113, 222, 52);
	_lstMissionTotals	= new TextList(288, 9, 18, 166);

	// Kill stats
	_txtRace			= new Text(98, 16, 16, 36);
	_txtRank			= new Text(98, 16, 114, 36);
	_txtWeapon			= new Text(98, 16, 212, 36);
	_lstRace			= new TextList(98, 113, 16, 52);
	_lstRank			= new TextList(98, 113, 114, 52);
	_lstWeapon			= new TextList(98, 113, 212, 52);
	_lstKillTotals		= new TextList(210, 9, 18, 166);

	// Award stats
	_txtMedalName		= new Text(90, 9, 16, 36);
	_txtMedalLevel		= new Text(52, 9, 196, 36);
	_txtMedalClass		= new Text(40, 9, 248, 36);
	_lstAwards			= new TextList(240, 97, 48, 49);
	_txtMedalInfo		= new Text(280, 25, 20, 150);


	// Award sprites
	_sstSprite = _game->getResourcePack()->getSurfaceSet("Commendations");
	_sstDecor = _game->getResourcePack()->getSurfaceSet("CommendationDecorations");
	for (int
			i = 0;
			i != LIST_ROWS;
			++i)
	{
		_srfSprite.push_back(new Surface(31, 7, 16, LIST_SPRITES_y + 8 * i));
		_srfDecor.push_back(new Surface(31, 7, 16, LIST_SPRITES_y + 8 * i));
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
	for (int
			i = 0;
			i != LIST_ROWS;
			++i)
	{
		add(_srfSprite[i]);
		add(_srfDecor[i]);
	}

	centerAllSurfaces();


	_window->setColor(Palette::blockOffset(15)+1);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK02.SCR"));

	_txtTitle->setColor(Palette::blockOffset(13)+10);
	_txtTitle->setBig();
	_txtTitle->setAlign(ALIGN_CENTER);


	_btnNext->setColor(Palette::blockOffset(15)+6);
	_btnNext->setText(L">");

	_btnPrev->setColor(Palette::blockOffset(15)+6);
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
		_txtBaseLabel->setColor(Palette::blockOffset(13)+10);
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


	_btnMissions->setColor(Palette::blockOffset(13)+10);
	_btnMissions->setText(tr("STR_MISSIONS_UC"));
	_btnMissions->onMouseClick((ActionHandler)& SoldierDiaryPerformanceState::btnMissionsToggle);

	_btnKills->setColor(Palette::blockOffset(13)+10);
	_btnKills->setText(tr("STR_KILLS_UC"));
	_btnKills->onMouseClick((ActionHandler)& SoldierDiaryPerformanceState::btnKillsToggle);

	_btnAwards->setColor(Palette::blockOffset(13)+10);
	_btnAwards->setText(tr("STR_AWARDS_UC"));
	_btnAwards->onMouseClick((ActionHandler)& SoldierDiaryPerformanceState::btnCommendationsToggle);

	_btnOk->setColor(Palette::blockOffset(13)+10);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& SoldierDiaryPerformanceState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& SoldierDiaryPerformanceState::btnOkClick,
					Options::keyCancel);


	// Mission stats ->
	_txtLocation->setColor(Palette::blockOffset(15)+1);
	_txtLocation->setText(tr("STR_MISSIONS_BY_LOCATION"));

	_txtType->setColor(Palette::blockOffset(15)+1);
	_txtType->setText(tr("STR_MISSIONS_BY_TYPE"));

	_txtUFO->setColor(Palette::blockOffset(15)+1);
	_txtUFO->setText(tr("STR_MISSIONS_BY_UFO"));

	_lstLocation->setColor(Palette::blockOffset(13));
	_lstLocation->setArrowColor(Palette::blockOffset(15)+1);
	_lstLocation->setColumns(2, 80, 12);
//	_lstLocation->setBackground(_window);

	_lstType->setColor(Palette::blockOffset(13));
	_lstType->setArrowColor(Palette::blockOffset(15)+1);
	_lstType->setColumns(2, 100, 14);
//	_lstType->setBackground(_window);

	_lstUFO->setColor(Palette::blockOffset(13));
	_lstUFO->setArrowColor(Palette::blockOffset(15)+1);
	_lstUFO->setColumns(2, 80, 12);
//	_lstUFO->setBackground(_window);

	_lstMissionTotals->setColor(Palette::blockOffset(13)+5);
	_lstMissionTotals->setSecondaryColor(Palette::blockOffset(13));
	_lstMissionTotals->setColumns(4, 70, 70, 70, 78);
//	_lstMissionTotals->setBackground(_window);


	// Kill stats ->
	_txtRace->setColor(Palette::blockOffset(15)+1);
	_txtRace->setText(tr("STR_KILLS_BY_RACE"));

	_txtRank->setColor(Palette::blockOffset(15)+1);
	_txtRank->setText(tr("STR_KILLS_BY_RANK"));

	_txtWeapon->setColor(Palette::blockOffset(15)+1);
	_txtWeapon->setText(tr("STR_KILLS_BY_WEAPON"));

	_lstRace->setColor(Palette::blockOffset(13));
	_lstRace->setArrowColor(Palette::blockOffset(15)+1);
	_lstRace->setColumns(2, 80, 18);
//	_lstRace->setBackground(_window);

	_lstRank->setColor(Palette::blockOffset(13));
	_lstRank->setArrowColor(Palette::blockOffset(15)+1);
	_lstRank->setColumns(2, 80, 18);
//	_lstRank->setBackground(_window);

	_lstWeapon->setColor(Palette::blockOffset(13));
	_lstWeapon->setArrowColor(Palette::blockOffset(15)+1);
	_lstWeapon->setColumns(2, 80, 18);
//	_lstWeapon->setBackground(_window);

	_lstKillTotals->setColor(Palette::blockOffset(13)+5);
	_lstKillTotals->setSecondaryColor(Palette::blockOffset(13));
	_lstKillTotals->setColumns(3, 70, 70, 70);
//	_lstKillTotals->setBackground(_window);


	// Award stats ->
	_txtMedalName->setColor(Palette::blockOffset(15)+1);
	_txtMedalName->setText(tr("STR_MEDAL_NAME"));

	_txtMedalLevel->setColor(Palette::blockOffset(15)+1);
	_txtMedalLevel->setText(tr("STR_MEDAL_DECOR_LEVEL"));

	_txtMedalClass->setColor(Palette::blockOffset(15)+1);
	_txtMedalClass->setText(tr("STR_MEDAL_DECOR_CLASS"));

	_lstAwards->setColor(Palette::blockOffset(13));
	_lstAwards->setArrowColor(Palette::blockOffset(15)+1);
	_lstAwards->setColumns(3, 148, 52, 40);
	_lstAwards->setSelectable();
	_lstAwards->setBackground(_window);
	_lstAwards->onMouseOver((ActionHandler)& SoldierDiaryPerformanceState::lstInfoMouseOver);
	_lstAwards->onMouseOut((ActionHandler)& SoldierDiaryPerformanceState::lstInfoMouseOut);
	_lstAwards->onMousePress((ActionHandler)& SoldierDiaryPerformanceState::handle);

	_txtMedalInfo->setColor(Palette::blockOffset(13)+10);
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

	for (int // clear sprites
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

	if (_game->getRuleset()->getCommendations().empty() == true) // set visibility for Award stats
		_displayAwards = false;

	_txtMedalName->setVisible(_displayAwards);
	_txtMedalLevel->setVisible(_displayAwards);
	_txtMedalClass->setVisible(_displayAwards);
	_lstAwards->setVisible(_displayAwards);
	_txtMedalInfo->setVisible(_displayAwards);

	if (_displayKills == true)
		_btnKills->setColor(Palette::blockOffset(13)+5);
	else
		_btnKills->setColor(Palette::blockOffset(13)+10);

	if (_displayMissions == true)
		_btnMissions->setColor(Palette::blockOffset(13)+5);
	else
		_btnMissions->setColor(Palette::blockOffset(13)+10);

	if (_displayAwards == true)
		_btnAwards->setColor(Palette::blockOffset(13)+5);
	else
		_btnAwards->setColor(Palette::blockOffset(13)+10);


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
/*		if (_listDead->empty() == true)
		{
			_game->popState();
			return;
		} */ // should never happen. Btn won't be visible if listDead is empty.

		if (_soldierID >= _listDead->size())
			_soldierID = 0;

		const SoldierDead* const deadSoldier = _listDead->at(_soldierID);
		_diary = deadSoldier->getDiary();

		_txtTitle->setText(deadSoldier->getName());
	}
	else
	{
/*		if (_list->empty() == true)
		{
			_game->popState();
			return;
		} */ // should never happen. Btn won't be visible unless viewing at least one soldier.

		if (_soldierID >= _list->size())
			_soldierID = 0;

		const Soldier* const soldier = _list->at(_soldierID);
		_diary = soldier->getDiary();

		_txtTitle->setText(soldier->getName());
	}


	if (_diary == NULL) // safety.
		return;


	std::wstring
		ws1,
		ws2,
		ws3,
		ws4;

	if (_diary->getMissionTotal() != 0) // Mission stats ->
		ws1 = tr("STR_MISSIONS").arg(_diary->getMissionTotal());
	if (_diary->getWinTotal() != 0)
		ws2 = tr("STR_WINS").arg(_diary->getWinTotal());
	if (_diary->getScoreTotal() != 0)
		ws3 = tr("STR_SCORE_VALUE").arg(_diary->getScoreTotal());
	if (_diary->getDaysWoundedTotal() != 0)
		ws4 = tr("STR_DAYS_WOUNDED").arg(_diary->getDaysWoundedTotal()).arg(L" dy");

	_lstMissionTotals->addRow(
						4,
						ws1.c_str(),
						ws2.c_str(),
						ws3.c_str(),
						ws4.c_str());

	ws1 =
	ws2 =
	ws3 = L"";

	if (_diary->getKillTotal() != 0) // Kill stats ->
		ws1 = tr("STR_KILLS").arg(_diary->getKillTotal());
	if (_diary->getStunTotal() != 0)
		ws2 = tr("STR_STUNS").arg(_diary->getStunTotal());
	if (_diary->getScorePoints() != 0)
		ws3 = tr("STR_SCORE_VALUE").arg(_diary->getScorePoints());

	_lstKillTotals->addRow(
						3,
						ws1.c_str(),
						ws2.c_str(),
						ws3.c_str());

	TextList* const lstArray[6] = // Kill & Mission stats ->
	{
		_lstRace,
		_lstRank,
		_lstWeapon,
		_lstLocation,
		_lstType,
		_lstUFO
	};

	const std::map<std::string, int> mapArray[6] =
	{
		_diary->getAlienRaceTotal(),
		_diary->getAlienRankTotal(),
		_diary->getWeaponTotal(),
		_diary->getRegionTotal(),
		_diary->getTypeTotal(),
		_diary->getUFOTotal()
	};

	for (int
			i = 0;
			i != 6;
			++i)
	{
		size_t row = 0;

		for (std::map<std::string, int>::const_iterator
				j = mapArray[i].begin();
				j != mapArray[i].end();
				++j)
		{
			if ((*j).first == "NO_UFO")
				continue;

			std::wstringstream
				ss1,
				ss2;

			ss1 << tr((*j).first.c_str());
			ss2 << (*j).second;

			lstArray[i]->addRow(
							2,
							ss1.str().c_str(),
							ss2.str().c_str());
			lstArray[i]->setCellColor(row++, 0, Palette::blockOffset(13)+5);
		}
	}


	for (std::vector<SoldierCommendations*>::const_iterator // Award stats ->
			i = _diary->getSoldierCommendations()->begin();
			i != _diary->getSoldierCommendations()->end();
			++i)
	{
		if (_game->getRuleset()->getCommendations().empty() == true)
			break;

		const RuleCommendations* const rule = _game->getRuleset()->getCommendations()[(*i)->getType()];

		std::wstringstream
			ss1,
			ss2,
			ss3,
			ss4;

		if ((*i)->getNoun() != "noNoun")
		{
			ss1 << tr((*i)->getType().c_str()).arg(tr((*i)->getNoun()).c_str());
			ss4 << tr(rule->getDescription().c_str()).arg(tr((*i)->getNoun()).c_str());
		}
		else
		{
			ss1 << tr((*i)->getType().c_str());
			ss4 << tr(rule->getDescription().c_str());
		}

		ss2 << tr((*i)->getDecorationDescription().c_str());
		ss3 << tr((*i)->getDecorationClass().c_str());

		_lstAwards->addRow(
						3,
						ss1.str().c_str(),
						ss2.str().c_str(),
						ss3.str().c_str());

		_awardsListEntry.push_back(ss4.str().c_str());

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

	for (int
			i = 0;
			i != LIST_ROWS;
			++i)
	{
		_srfSprite[i]->clear();
		_srfDecor[i]->clear();
	}

	const int scroll = _lstAwards->getScroll();
	int j = 0;

	for (std::vector<SoldierCommendations*>::const_iterator
			i = _diary->getSoldierCommendations()->begin();
			i != _diary->getSoldierCommendations()->end();
			++i,
				++j)
	{
		const RuleCommendations* const rule = _game->getRuleset()->getCommendations()[(*i)->getType()];

		if (j < scroll
			|| j - scroll >= static_cast<int>(_srfSprite.size()))
		{
			continue; // skip awards that are not visible on the list
		}

		const int sprite = rule->getSprite(); // handle award sprites
		_sstSprite->getFrame(sprite)->setX(0);
		_sstSprite->getFrame(sprite)->setY(0);
		_sstSprite->getFrame(sprite)->blit(_srfSprite[j - scroll]);

		const int decor = (*i)->getDecorationLevelInt(); // handle award decoration sprites
		if (decor != 0)
		{
			_sstDecor->getFrame(decor)->setX(0);
			_sstDecor->getFrame(decor)->setY(0);
			_sstDecor->getFrame(decor)->blit(_srfDecor[j - scroll]);
		}
	}
}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void SoldierDiaryPerformanceState::btnOkClick(Action*)
{
	_soldierDiaryOverviewState->setSoldierID(_soldierID);
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

	if (_soldierID == 0)
		_soldierID = rows - 1;
	else
		--_soldierID;

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

	++_soldierID;
	if (_soldierID >= rows)
		_soldierID = 0;

	init();
}

/**
 * Display Kills totals.
 */
void SoldierDiaryPerformanceState::btnKillsToggle(Action*)
{
	_displayKills = true;
	_displayMissions = false;
	_displayAwards = false;

	init();
}

/**
 * Display Missions totals.
 */
void SoldierDiaryPerformanceState::btnMissionsToggle(Action*)
{
	_displayKills = false;
	_displayMissions = true;
	_displayAwards = false;

	init();
}

/**
 * Display Commendations.
 */
void SoldierDiaryPerformanceState::btnCommendationsToggle(Action*)
{
	_displayKills = false;
	_displayMissions = false;
	_displayAwards = true;

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
 * Used to update sprite vector.
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
