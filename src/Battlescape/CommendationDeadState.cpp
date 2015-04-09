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
	_window			= new Window(this, 320, 200);
	_txtTitle		= new Text(300, 16, 10, 8);
	_lstKIA			= new TextList(285, 9, 16, 26);
	_lstSoldiers	= new TextList(285, 113, 16, 36);
	_txtMedalInfo	= new Text(280, 25, 20, 150);
	_btnOk			= new TextButton(288, 16, 16, 177);

	setPalette("PAL_GEOSCAPE", 0);

	add(_window);
	add(_txtTitle);
	add(_lstKIA);
	add(_lstSoldiers);
	add(_txtMedalInfo);
	add(_btnOk);

	centerAllSurfaces();


	_window->setColor(Palette::blockOffset(15)-1); // green
	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_txtTitle->setColor(Palette::blockOffset(8)+5); // cyan
	_txtTitle->setText(tr("STR_KIA"));
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_lstKIA->setColor(Palette::blockOffset(8)+5);
	_lstKIA->setColumns(1, 285);
	_lstKIA->setBackground(_window);
	_lstKIA->setSelectable();
	_lstKIA->setMargin();

	_lstSoldiers->setColor(Palette::blockOffset(15)-1);
	_lstSoldiers->setColumns(2, 200, 77);
	_lstSoldiers->setBackground(_window);
	_lstSoldiers->setSelectable();
	_lstSoldiers->setMargin();
	_lstSoldiers->onMouseOver((ActionHandler)& CommendationDeadState::lstInfoMouseOver);
	_lstSoldiers->onMouseOut((ActionHandler)& CommendationDeadState::lstInfoMouseOut);

	_txtMedalInfo->setColor(Palette::blockOffset(10)); // slate
	_txtMedalInfo->setHighContrast();
	_txtMedalInfo->setWordWrap();

	_btnOk->setColor(Palette::blockOffset(15)-1);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& CommendationDeadState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& CommendationDeadState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& CommendationDeadState::btnOkClick,
					Options::keyCancel);


	const int rowsKIAList = std::min(	// the soldiersKIA list has maximum 8 rows
								8,		// to leave room below for the awards List
								static_cast<int>(soldiersKIA.size()));
	_lstKIA->setHeight(rowsKIAList * 8 + 1);

	_lstSoldiers->setY(_lstSoldiers->getY() + (rowsKIAList - 1) * 8);
	_lstSoldiers->setHeight(_lstSoldiers->getHeight() - (rowsKIAList - 1) * 8);


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
			_lstSoldiers->addRow(2, L"", L""); // Blank row, will be filled in later -> unless it's the last row ......
			_titleRows.insert(std::pair<size_t, std::string>(
															row++,
															""));
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
								Palette::blockOffset(9), // brown
								true);


			std::string info = (*award).second->getDescriptionGeneral(); // look for Generic Desc first.
			if (info != "")
				_titleRows[titleRow] = info;
			else
				_titleRows[titleRow] = (*award).second->getDescription();


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
	if (_game->getQtyStates() == 2) // ie: (1) this, (2) Geoscape
	{
		_game->getResourcePack()->fadeMusic(_game, 863);
		_game->getResourcePack()->playMusic(OpenXcom::res_MUSIC_GEO_GLOBE);
	}

	_game->popState();
}

/**
 * Shows the Medal description.
 */
void CommendationDeadState::lstInfoMouseOver(Action*)
{
	const size_t row = _lstSoldiers->getSelectedRow();
	if (_titleRows.find(row) != _titleRows.end())
		_txtMedalInfo->setText(tr(_titleRows[row].c_str()));
	else
		_txtMedalInfo->setText(L"");
}

/**
 * Clears the Medal description.
 */
void CommendationDeadState::lstInfoMouseOut(Action*)
{
	_txtMedalInfo->setText(L"");
}

}
