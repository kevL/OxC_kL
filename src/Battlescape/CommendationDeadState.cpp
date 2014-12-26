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

#include "CommendationDeadState.h"

//#include <sstream>

#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Options.h"
//#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/XcomResourcePack.h"

#include "../Ruleset/RuleCommendations.h"

#include "../Savegame/SoldierDead.h"
#include "../Savegame/SoldierDiary.h"


namespace OpenXcom
{

/**
 * Initializes all the elements for KIA in the Medals screen post-mission.
 * @param soldiersKIA - vector of pointers to SoldierDead objects
 */
CommendationDeadState::CommendationDeadState(std::vector<SoldierDead*> soldiersKIA)
{
	_window			= new Window(this, 320, 200, 0, 0);
	_txtTitle		= new Text(300, 16, 10, 8);
	_lstKIA			= new TextList(285, 9, 16, 28);
	_lstSoldiers	= new TextList(285, 137, 16, 38);
//	_lstSoldiers	= new TextList(285, 145, 16, 28);
	_btnOk			= new TextButton(288, 16, 16, 177);

	setPalette("PAL_GEOSCAPE", 0);

//	_game->getResourcePack()->playMusic(OpenXcom::res_MUSIC_TAC_AWARDS); // note: Moved to DebriefingState.

	add(_window);
	add(_txtTitle);
	add(_lstKIA);
	add(_lstSoldiers);
	add(_btnOk);

	centerAllSurfaces();


	_window->setColor(Palette::blockOffset(15)-1);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_txtTitle->setColor(Palette::blockOffset(8)+5);
	_txtTitle->setText(tr("STR_KIA"));
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_lstKIA->setColor(Palette::blockOffset(9)); // (8)+10
	_lstKIA->setColumns(1, 285);
//	_lstKIA->setSelectable();
//	_lstKIA->setBackground(_window);
	_lstKIA->setMargin();
	_lstKIA->setHighContrast();

	_lstSoldiers->setColor(Palette::blockOffset(9)); // (8)+10
	_lstSoldiers->setColumns(2, 200, 77);
//	_lstSoldiers->setSelectable();
//	_lstSoldiers->setBackground(_window);
	_lstSoldiers->setMargin();
	_lstSoldiers->setHighContrast();

	_btnOk->setColor(Palette::blockOffset(15)-1);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& CommendationDeadState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& CommendationDeadState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& CommendationDeadState::btnOkClick,
					Options::keyCancel);


	size_t rowsKIA = soldiersKIA.size();
	_lstKIA->setHeight(rowsKIA * 8 + 1);
	_lstSoldiers->setY(_lstSoldiers->getY() + (rowsKIA - 1) * 8);
	_lstSoldiers->setHeight(_lstSoldiers->getHeight() - (rowsKIA - 1) * 8);


	for (std::vector<SoldierDead*>::const_iterator
			soldier = soldiersKIA.begin();
			soldier != soldiersKIA.end();
			++soldier)
	{
		_lstKIA->addRow(
					1,
					(*soldier)->getName().c_str());
	}



	std::string noun;
	bool
		titleChosen = true,
		modularAward;
	size_t
		row = 0,
		titleRow = 0;

	std::map<std::string, RuleCommendations*> awardList = _game->getRuleset()->getCommendations();
	for (std::map<std::string, RuleCommendations*>::const_iterator
			award = awardList.begin();
			award != awardList.end();
			)
	{
		modularAward = false;
		noun = "noNoun";

		if (titleChosen == true)
		{
			_lstSoldiers->addRow(2, L"", L""); // Blank row, will be filled in later
			++row;
		}

		titleChosen = false;
		titleRow = row - 1;

		for (std::vector<SoldierDead*>::const_iterator
				soldier = soldiersKIA.begin();
				soldier != soldiersKIA.end();
				++soldier)
		{
			for (std::vector<SoldierCommendations*>::const_iterator
					soldierAward = (*soldier)->getDiary()->getSoldierCommendations()->begin();
					soldierAward != (*soldier)->getDiary()->getSoldierCommendations()->end();
					++soldierAward)
			{
				if ((*soldierAward)->getType() == (*award).first
					&& (*soldierAward)->isNew() == true
					&& noun == "noNoun")
				{
					(*soldierAward)->makeOld();
					++row;

					if ((*soldierAward)->getNoun() != "noNoun")
					{
						noun = (*soldierAward)->getNoun();
						modularAward = true;
					}

					std::wstringstream wss;
					wss << L"  ";
					wss << (*soldier)->getName().c_str();

					int
						skipCounter = 0,
						lastInt = -2,
						thisInt = -1,
						j = 0;

					for (std::vector<int>::const_iterator
							i = (*award).second->getCriteria()->begin()->second.begin();
							i != (*award).second->getCriteria()->begin()->second.end();
							++i)
					{
						if (j == (*soldierAward)->getDecorationLevelInt() + 1)
							break;

						thisInt = *i;
						if (i != (*award).second->getCriteria()->begin()->second.begin())
						{
							--i;
							lastInt = *i;
							++i;
						}

						if (thisInt == lastInt)
							++skipCounter;

						++j;
					}

					_lstSoldiers->addRow(
									2,
									wss.str().c_str(),
									tr((*soldierAward)->getDecorationLevelName(skipCounter)).c_str());
					break;
				}
			}
		}

		if (titleRow != row - 1)
		{
			if (modularAward == true)
				_lstSoldiers->setCellText(
										titleRow,
										0,
										tr((*award).first).arg(tr(noun).c_str()).c_str());
			else
				_lstSoldiers->setCellText(
										titleRow,
										0,
										tr((*award).first).c_str());

			_lstSoldiers->setRowColor(
									titleRow,
									Palette::blockOffset(15)-1);
			titleChosen = true;
		}

		if (noun == "noNoun")
			++award;
	}
}

/**
 * dTor.
 */
CommendationDeadState::~CommendationDeadState()
{}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void CommendationDeadState::btnOkClick(Action*)
{
	//Log(LOG_INFO) << "CommendationDead, states = " << _game->getQtyStates();
	if (_game->getQtyStates() == 2) // ie: (1) this, (2) Geoscape
	{
		_game->getResourcePack()->fadeMusic(_game, 900);
		_game->getResourcePack()->playMusic(OpenXcom::res_MUSIC_GEO_GLOBE);
	}

	_game->popState();
}

}
