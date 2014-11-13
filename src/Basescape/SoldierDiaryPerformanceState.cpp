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

#include "SoldierDiaryPerformanceState.h"

#include <string>

#include "SoldierDiaryOverviewState.h"
#include "SoldierInfoState.h"
#include "SoldierInfoDeadState.h" // kL

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Logger.h" // kL
#include "../Engine/Palette.h"
#include "../Engine/Options.h"
#include "../Engine/SurfaceSet.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleCommendations.h"

#include "../Savegame/Base.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/SoldierDead.h" // kL
#include "../Savegame/SoldierDiary.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Soldiers screen.
 * @param base						- pointer to the base to get info from
 * @param soldierId					- ID of the selected soldier
 * @param soldierDiaryOverviewState	- pointer to said
 * @param display					- subscreen to display
 */
SoldierDiaryPerformanceState::SoldierDiaryPerformanceState(
		Base* base,
		size_t soldierID,
		SoldierDiaryOverviewState* soldierDiaryOverviewState,
		int display)
	:
		_base(base),
		_soldierID(soldierID),
		_soldierDiaryOverviewState(soldierDiaryOverviewState),
		_display(display),
		_lastScrollPos(0)
{
	//Log(LOG_INFO) << "Create SoldierDiaryPerformanceState";
	if (_base == NULL) // kL
	{
		_listDead = _game->getSavedGame()->getDeadSoldiers();
		_list = NULL;
	}
	else
	{
		_list = _base->getSoldiers();
		_listDead = NULL;
	}

	if (_display == 0)
	{
		_displayKills = true;
		_displayMissions = false;
		_displayAwards = false;
	}
	else if (_display == 1)
	{
		_displayKills = false;
		_displayMissions = true;
		_displayAwards = false;
	}
	else // _display == 2
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

	// Kill stats
	_txtRace			= new Text(98, 16, 16, 36);
	_txtRank			= new Text(98, 16, 114, 36);
	_txtWeapon			= new Text(98, 16, 212, 36);
	_lstRace			= new TextList(98, 113, 16, 52);
	_lstRank			= new TextList(98, 113, 114, 52);
	_lstWeapon			= new TextList(98, 113, 212, 52);
	_lstKillTotals		= new TextList(100, 9, 18, 166);

	// Mission stats
	_txtLocation		= new Text(92, 16, 16, 36);
	_txtType			= new Text(114, 16, 108, 36);
	_txtUFO				= new Text(92, 16, 222, 36);
	_lstLocation		= new TextList(92, 113, 16, 52);
	_lstType			= new TextList(114, 113, 108, 52);
	_lstUFO				= new TextList(92, 113, 222, 52);
	_lstMissionTotals	= new TextList(288, 9, 18, 166);

	// Award stats
	_txtMedalName		= new Text(90, 9, 16, 36);
	_txtMedalLevel		= new Text(90, 9, 206, 36);
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
		_btnPrev->onMouseClick((ActionHandler)& SoldierDiaryPerformanceState::btnNextClick);
		_btnPrev->onKeyboardPress(
						(ActionHandler)& SoldierDiaryPerformanceState::btnNextClick,
						Options::keyBattlePrevUnit);
	}
	else
	{
		_btnPrev->onMouseClick((ActionHandler)& SoldierDiaryPerformanceState::btnPrevClick);
		_btnPrev->onKeyboardPress(
						(ActionHandler)& SoldierDiaryPerformanceState::btnPrevClick,
						Options::keyBattlePrevUnit);
	}

	_btnNext->setColor(Palette::blockOffset(15)+6);
	_btnNext->setText(L">");
	if (_base == NULL)
	{
		_btnNext->onMouseClick((ActionHandler)& SoldierDiaryPerformanceState::btnPrevClick);
		_btnNext->onKeyboardPress(
						(ActionHandler)& SoldierDiaryPerformanceState::btnPrevClick,
						Options::keyBattleNextUnit);
	}
	else
	{
		_btnNext->onMouseClick((ActionHandler)& SoldierDiaryPerformanceState::btnNextClick);
		_btnNext->onKeyboardPress(
						(ActionHandler)& SoldierDiaryPerformanceState::btnNextClick,
						Options::keyBattleNextUnit);
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


	// Kill stats
	_txtRace->setColor(Palette::blockOffset(15)+1);
	_txtRace->setText(tr("STR_KILLS_BY_RACE"));
	_txtRace->setWordWrap();

	_txtRank->setColor(Palette::blockOffset(15)+1);
	_txtRank->setText(tr("STR_KILLS_BY_RANK"));
	_txtRank->setWordWrap();

	_txtWeapon->setColor(Palette::blockOffset(15)+1);
	_txtWeapon->setText(tr("STR_KILLS_BY_WEAPON"));
	_txtWeapon->setWordWrap();

	_lstRace->setColor(Palette::blockOffset(13));
	_lstRace->setArrowColor(Palette::blockOffset(15)+1);
	_lstRace->setColumns(2, 80, 18);
	_lstRace->setBackground(_window);
//	_lstRace->setDot();

	_lstRank->setColor(Palette::blockOffset(13));
	_lstRank->setArrowColor(Palette::blockOffset(15)+1);
	_lstRank->setColumns(2, 80, 18);
	_lstRank->setBackground(_window);
//	_lstRank->setDot();

	_lstWeapon->setColor(Palette::blockOffset(13));
	_lstWeapon->setArrowColor(Palette::blockOffset(15)+1);
	_lstWeapon->setColumns(2, 80, 18);
	_lstWeapon->setBackground(_window);
//	_lstWeapon->setDot();

	_lstKillTotals->setColor(Palette::blockOffset(13)+5);
	_lstKillTotals->setSecondaryColor(Palette::blockOffset(13));
	_lstKillTotals->setColumns(2, 50, 50);
//	_lstKillTotals->setMargin(); // TODO: need to fix this.
	_lstKillTotals->setBackground(_window);

	// Mission stats
	_txtLocation->setColor(Palette::blockOffset(15)+1);
	_txtLocation->setText(tr("STR_MISSIONS_BY_LOCATION"));
	_txtLocation->setWordWrap();

	_txtType->setColor(Palette::blockOffset(15)+1);
	_txtType->setText(tr("STR_MISSIONS_BY_TYPE"));
	_txtType->setWordWrap();

	_txtUFO->setColor(Palette::blockOffset(15)+1);
	_txtUFO->setText(tr("STR_MISSIONS_BY_UFO"));
	_txtUFO->setWordWrap();

	_lstLocation->setColor(Palette::blockOffset(13));
	_lstLocation->setArrowColor(Palette::blockOffset(15)+1);
	_lstLocation->setColumns(2, 80, 12);
	_lstLocation->setBackground(_window);
//	_lstLocation->setDot();

	_lstType->setColor(Palette::blockOffset(13));
	_lstType->setArrowColor(Palette::blockOffset(15)+1);
	_lstType->setColumns(2, 100, 14);
	_lstType->setBackground(_window);
//	_lstType->setDot();

	_lstUFO->setColor(Palette::blockOffset(13));
	_lstUFO->setArrowColor(Palette::blockOffset(15)+1);
	_lstUFO->setColumns(2, 80, 12);
	_lstUFO->setBackground(_window);
//	_lstUFO->setDot();

	_lstMissionTotals->setColor(Palette::blockOffset(13)+5);
	_lstMissionTotals->setSecondaryColor(Palette::blockOffset(13));
	_lstMissionTotals->setColumns(4, 68, 68, 68, 84);
//	_lstMissionTotals->setMargin(); // TODO: need to fix this.
	_lstMissionTotals->setBackground(_window);

	// Award stats
	_txtMedalName->setColor(Palette::blockOffset(15)+1);
	_txtMedalName->setText(tr("STR_MEDAL_NAME"));
	_txtMedalName->setWordWrap();

	_txtMedalLevel->setColor(Palette::blockOffset(15)+1);
	_txtMedalLevel->setText(tr("STR_MEDAL_DECOR_LEVEL"));
	_txtMedalLevel->setWordWrap();

	_lstAwards->setColor(Palette::blockOffset(13));
	_lstAwards->setArrowColor(Palette::blockOffset(15)+1);
	_lstAwards->setColumns(2, 158, 80);
	_lstAwards->setSelectable();
	_lstAwards->setBackground(_window);
	_lstAwards->onMouseOver((ActionHandler)& SoldierDiaryPerformanceState::lstInfoMouseOver);
	_lstAwards->onMouseOut((ActionHandler)& SoldierDiaryPerformanceState::lstInfoMouseOut);
	_lstAwards->onMousePress((ActionHandler)& SoldierDiaryPerformanceState::handle);

	_txtMedalInfo->setColor(Palette::blockOffset(13)+10);
	_txtMedalInfo->setWordWrap();

//	init(); // Populate the list
	//Log(LOG_INFO) << "Create SoldierDiaryPerformanceState EXIT";
}

/**
 * dTor.
 */
SoldierDiaryPerformanceState::~SoldierDiaryPerformanceState()
{
}

/**
 *  Clears all the variables and reinitializes the list of kills or missions for the soldier.
 */
void SoldierDiaryPerformanceState::init()
{
	//Log(LOG_INFO) << "SoldierDiaryPerformanceState::init";
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

	_txtRace->setVisible(_displayKills); // set visibility for kills
	_txtRank->setVisible(_displayKills);
	_txtWeapon->setVisible(_displayKills);
	_lstRace->setVisible(_displayKills);
	_lstRank->setVisible(_displayKills);
	_lstWeapon->setVisible(_displayKills);
	_lstKillTotals->setVisible(_displayKills);

	_txtLocation->setVisible(_displayMissions); // set visibility for missions
	_txtType->setVisible(_displayMissions);
	_txtUFO->setVisible(_displayMissions);
	_lstLocation->setVisible(_displayMissions);
	_lstType->setVisible(_displayMissions);
	_lstUFO->setVisible(_displayMissions);
	_lstMissionTotals->setVisible(_displayMissions);

	if (_game->getRuleset()->getCommendation().empty()) // set visibility for awards
		_displayAwards = false;

	_txtMedalName->setVisible(_displayAwards);
	_txtMedalLevel->setVisible(_displayAwards);
	_txtMedalInfo->setVisible(_displayAwards);
	_lstAwards->setVisible(_displayAwards);

	// kL_begin:
	if (_displayKills)
		_btnKills->setColor(Palette::blockOffset(13)+5);
	else
		_btnKills->setColor(Palette::blockOffset(13)+10);

	if (_displayMissions)
		_btnMissions->setColor(Palette::blockOffset(13)+5);
	else
		_btnMissions->setColor(Palette::blockOffset(13)+10);
	// kL_end.
//	_btnKills->setVisible(!_displayKills); // set visibility for buttons
//	_btnMissions->setVisible(!_displayMissions);

//	if (_game->getRuleset()->getCommendation().empty())
//		_btnAwards->setVisible(false);
//	else
//	{
	// kL_begin:
	if (_displayAwards)
		_btnAwards->setColor(Palette::blockOffset(13)+5);
	else
		_btnAwards->setColor(Palette::blockOffset(13)+10);
	// kL_end.
//		_btnAwards->setVisible(!_displayAwards);
//	}


	_commendationsListEntry.clear();
	_lstKillTotals->clearList();
	_lstMissionTotals->clearList();

	_lstRace->clearList();
	_lstRank->clearList();
	_lstWeapon->clearList();
	_lstLocation->clearList();
	_lstType->clearList();
	_lstUFO->clearList();
	_lstAwards->clearList();

	if (_base == NULL) // kL
	{
		if (_listDead->empty())
		{
			_game->popState();
			return;
		}

		if (_soldierID >= _listDead->size())
			_soldierID = 0;

		_soldierDead = _listDead->at(_soldierID);

		if (_soldierDead->getDiary() != NULL) // kL
		{
			_lstKillTotals->addRow(
								2,
								tr("STR_KILLS").arg(_soldierDead->getDiary()->getKillTotal()).c_str(),
								tr("STR_STUNS").arg(_soldierDead->getDiary()->getStunTotal()).c_str());
			_lstMissionTotals->addRow(
								4,
								tr("STR_MISSIONS").arg(_soldierDead->getDiary()->getMissionTotal()).c_str(),
								tr("STR_WINS").arg(_soldierDead->getDiary()->getWinTotal()).c_str(),
								tr("STR_SCORE_VALUE").arg(_soldierDead->getDiary()->getScoreTotal()).c_str(),
								tr("STR_DAYS_WOUNDED").arg(_soldierDead->getDiary()->getDaysWoundedTotal()).c_str());
		}

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

		_lstKillTotals->addRow(
							2,
							tr("STR_KILLS").arg(_soldier->getDiary()->getKillTotal()).c_str(),
							tr("STR_STUNS").arg(_soldier->getDiary()->getStunTotal()).c_str());
		_lstMissionTotals->addRow(
							4,
							tr("STR_MISSIONS").arg(_soldier->getDiary()->getMissionTotal()).c_str(),
							tr("STR_WINS").arg(_soldier->getDiary()->getWinTotal()).c_str(),
							tr("STR_SCORE_VALUE").arg(_soldier->getDiary()->getScoreTotal()).c_str(),
							tr("STR_DAYS_WOUNDED").arg(_soldier->getDiary()->getDaysWoundedTotal()).c_str());

		_txtTitle->setText(_soldier->getName());
	}

	TextList* lstArray[6] =
	{
		_lstRace,
		_lstRank,
		_lstWeapon,
		_lstLocation,
		_lstType,
		_lstUFO
	};

	size_t row = 0;

	if (_base == NULL) // kL
	{
		if (_soldierDead->getDiary() != NULL) // kL
		{
			std::map<std::string, int> mapArray[6] =
			{
				_soldierDead->getDiary()->getAlienRaceTotal(),
				_soldierDead->getDiary()->getAlienRankTotal(),
				_soldierDead->getDiary()->getWeaponTotal(),
				_soldierDead->getDiary()->getRegionTotal(),
				_soldierDead->getDiary()->getTypeTotal(),
				_soldierDead->getDiary()->getUFOTotal()
			};

			for (int
					i = 0;
					i != 6;
					++i)
			{
				row = 0;

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

					lstArray[i]->setCellColor(row, 0, Palette::blockOffset(13)+5);

					row++;
				}
			}
		}
	}
	else
	{
		std::map<std::string, int> mapArray[6] =
		{
			_soldier->getDiary()->getAlienRaceTotal(),
			_soldier->getDiary()->getAlienRankTotal(),
			_soldier->getDiary()->getWeaponTotal(),
			_soldier->getDiary()->getRegionTotal(),
			_soldier->getDiary()->getTypeTotal(),
			_soldier->getDiary()->getUFOTotal()
		};

		for (int
				i = 0;
				i != 6;
				++i)
		{
			row = 0;

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

				lstArray[i]->setCellColor(row, 0, Palette::blockOffset(13)+5);

				row++;
			}
		}
	}

	if (_base == NULL) // kL
	{
		if (_soldierDead->getDiary() != NULL) // kL
		{
			for (std::vector<SoldierCommendations*>::const_iterator
					i = _soldierDead->getDiary()->getSoldierCommendations()->begin();
					i != _soldierDead->getDiary()->getSoldierCommendations()->end();
					++i)
			{
				if (_game->getRuleset()->getCommendation().empty())
					break;

				RuleCommendations* rule = _game->getRuleset()->getCommendation()[(*i)->getType()];

				std::wstringstream
					ss1,
					ss2,
					ss3;

				if ((*i)->getNoun() != "noNoun")
				{
					ss1 << tr((*i)->getType().c_str()).arg(tr((*i)->getNoun()).c_str());
					ss3 << tr(rule->getDescription().c_str()).arg(tr((*i)->getNoun()).c_str());
				}
				else
				{
					ss1 << tr((*i)->getType().c_str());
					ss3 << tr(rule->getDescription().c_str());
				}

				ss2 << tr((*i)->getDecorationDescription().c_str());

				_lstAwards->addRow(
								2,
								ss1.str().c_str(),
								ss2.str().c_str());

				_commendationsListEntry.push_back(ss3.str().c_str());

				drawSprites();
			}
		}
	}
	else
	{
		for (std::vector<SoldierCommendations*>::const_iterator
				i = _soldier->getDiary()->getSoldierCommendations()->begin();
				i != _soldier->getDiary()->getSoldierCommendations()->end();
				++i)
		{
			if (_game->getRuleset()->getCommendation().empty())
				break;

			RuleCommendations* rule = _game->getRuleset()->getCommendation()[(*i)->getType()];

			std::wstringstream
				ss1,
				ss2,
				ss3;

			if ((*i)->getNoun() != "noNoun")
			{
				ss1 << tr((*i)->getType().c_str()).arg(tr((*i)->getNoun()).c_str());
				ss3 << tr(rule->getDescription().c_str()).arg(tr((*i)->getNoun()).c_str());
			}
			else
			{
				ss1 << tr((*i)->getType().c_str());
				ss3 << tr(rule->getDescription().c_str());
			}

			ss2 << tr((*i)->getDecorationDescription().c_str());

			_lstAwards->addRow(
							2,
							ss1.str().c_str(),
							ss2.str().c_str());

			_commendationsListEntry.push_back(ss3.str().c_str());

			drawSprites();
		}
	}
	//Log(LOG_INFO) << "SoldierDiaryPerformanceState::init EXIT";
}

/**
 * Draws sprites.
 */
void SoldierDiaryPerformanceState::drawSprites()
{
	if (_displayAwards == false)
		return;

	for (int // clear sprites
			i = 0;
			i != LIST_ROWS;
			++i)
	{
		_srfSprite[i]->clear();
		_srfDecor[i]->clear();
	}

	int j = 0; // current location in the vector
	int scroll = _lstAwards->getScroll(); // start here

	if (_base == NULL) // kL
	{
		if (_listDead->at(_soldierID)->getDiary() != NULL) // kL
		{
			for (std::vector<SoldierCommendations*>::const_iterator
					i = _listDead->at(_soldierID)->getDiary()->getSoldierCommendations()->begin();
					i != _listDead->at(_soldierID)->getDiary()->getSoldierCommendations()->end();
					++i)
			{
				RuleCommendations* rule = _game->getRuleset()->getCommendation()[(*i)->getType()];

				if (j < scroll // skip awards that are not visible in the textlist
					|| j - scroll >= static_cast<int>(_srfSprite.size()))
				{
					j++;

					continue;
				}

				int sprite = rule->getSprite();
				_sstSprite->getFrame(sprite)->setX(0); // handle award sprites
				_sstSprite->getFrame(sprite)->setY(0);
				_sstSprite->getFrame(sprite)->blit(_srfSprite[j - scroll]);

				int decor = (*i)->getDecorationLevelInt();
				if (decor != 0) // handle award decoration sprites
				{
					_sstDecor->getFrame(decor)->setX(0);
					_sstDecor->getFrame(decor)->setY(0);
					_sstDecor->getFrame(decor)->blit(_srfDecor[j - scroll]);
				}

				j++;
			}
		}
	}
	else
	{
		for (std::vector<SoldierCommendations*>::const_iterator
				i = _list->at(_soldierID)->getDiary()->getSoldierCommendations()->begin();
				i != _list->at(_soldierID)->getDiary()->getSoldierCommendations()->end();
				++i)
		{
			RuleCommendations* rule = _game->getRuleset()->getCommendation()[(*i)->getType()];

			if (j < scroll // skip awards that are not visible in the textlist
				|| j - scroll >= static_cast<int>(_srfSprite.size()))
			{
				j++;

				continue;
			}

			int sprite = rule->getSprite();
			_sstSprite->getFrame(sprite)->setX(0); // handle award sprites
			_sstSprite->getFrame(sprite)->setY(0);
			_sstSprite->getFrame(sprite)->blit(_srfSprite[j - scroll]);

			int decor = (*i)->getDecorationLevelInt();
			if (decor != 0) // handle award decoration sprites
			{
				_sstDecor->getFrame(decor)->setX(0);
				_sstDecor->getFrame(decor)->setY(0);
				_sstDecor->getFrame(decor)->blit(_srfDecor[j - scroll]);
			}

			j++;
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
		_soldierID--;

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

	_soldierID++;
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
	size_t _sel = _lstAwards->getSelectedRow();

	if (_commendationsListEntry.empty()
		|| _sel > _commendationsListEntry.size() - 1)
	{
		_txtMedalInfo->setText(L"");
	}
	else
		_txtMedalInfo->setText(_commendationsListEntry[_sel]);
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
