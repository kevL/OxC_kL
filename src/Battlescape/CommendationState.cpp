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

#include "CommendationState.h"

#include <sstream>

#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"

#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"

#include "../Ruleset/RuleCommendations.h"

#include "../Savegame/Base.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/SoldierDiary.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Medals screen.
 * @param game Pointer to the core game.
 */
CommendationState::CommendationState(std::vector<Soldier*> soldiersMedalled)
{
	_window			= new Window(this, 320, 200, 0, 0);
	_txtTitle		= new Text(300, 16, 10, 8);
	_lstSoldiers	= new TextList(285, 144, 16, 32);
	_btnOk			= new TextButton(288, 16, 16, 177);

	setPalette("PAL_GEOSCAPE", 0);

	add(_window);
	add(_btnOk);
	add(_txtTitle);
	add(_lstSoldiers);

	centerAllSurfaces();

	_window->setColor(Palette::blockOffset(15)-1);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_btnOk->setColor(Palette::blockOffset(15)-1);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& CommendationState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& CommendationState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& CommendationState::btnOkClick,
					Options::keyCancel);

	_txtTitle->setColor(Palette::blockOffset(8)+5);
	_txtTitle->setText(tr("STR_MEDALS"));
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_lstSoldiers->setColor(Palette::blockOffset(8)+10);
	_lstSoldiers->setColumns(2, 200, 77);
	_lstSoldiers->setSelectable(true);
	_lstSoldiers->setBackground(_window);
	_lstSoldiers->setMargin(8);


	std::string noun;
	bool
		titleChosen = true,
		modularCommendation;
	size_t
		row = 0,
		titleRow = 0;
	std::map<std::string, RuleCommendations*> commendationsList = _game->getRuleset()->getCommendation();

	for (std::map<std::string, RuleCommendations*>::const_iterator
			commList = commendationsList.begin();
			commList != commendationsList.end();
			)
	{
		modularCommendation = false;
		noun = "noNoun";

		if (titleChosen)
		{
			_lstSoldiers->addRow(2, L"", L""); // Blank row, will be filled in later

			row++;
		}

		titleChosen = false;
		titleRow = row - 1;

		for (std::vector<Soldier*>::iterator
				s = soldiersMedalled.begin();
				s != soldiersMedalled.end();
				++s)
		{
			for (std::vector<SoldierCommendations*>::const_iterator
					soldierComm = (*s)->getDiary()->getSoldierCommendations()->begin();
					soldierComm != (*s)->getDiary()->getSoldierCommendations()->end();
					++soldierComm)
			{
				if ((*soldierComm)->getType() == (*commList).first
					&& (*soldierComm)->isNew()
					&& noun == "noNoun")
				{
					(*soldierComm)->makeOld();

					row++;

					if ((*soldierComm)->getNoun() != "noNoun")
					{
						noun = (*soldierComm)->getNoun();
						modularCommendation = true;
					}

					std::wstringstream wss; // Soldier name
					wss << "   ";
					wss << (*s)->getName().c_str();

					int // Decoration level name
						skipCounter = 0,
						lastInt = -2,
						thisInt = -1,
						vectorIterator = 0;

					for (std::vector<int>::const_iterator
							k = (*commList).second->getCriteria()->begin()->second.begin();
							k != (*commList).second->getCriteria()->begin()->second.end();
							++k)
					{
						if (vectorIterator == (*soldierComm)->getDecorationLevelInt() + 1)
						{
							break;
						}

						thisInt = *k;
						if (k != (*commList).second->getCriteria()->begin()->second.begin())
						{
							--k;
							lastInt = *k;
							++k;
						}

						if (thisInt == lastInt)
							skipCounter++;

						vectorIterator++;
					}

					_lstSoldiers->addRow(
									2,
									wss.str().c_str(),
									tr((*soldierComm)->getDecorationLevelName(skipCounter)).c_str());
//					_lstSoldiers->setRowColor(row, Palette::blockOffset(8)+10);

					break;
				}
			}
		}

		if (titleRow != row - 1)
		{
			if (modularCommendation) // Medal name
			{
				_lstSoldiers->setCellText(
										titleRow,
										0,
										tr((*commList).first).arg(tr(noun).c_str()).c_str());
			}
			else
			{
				_lstSoldiers->setCellText(
										titleRow,
										0,
										tr((*commList).first).c_str());
			}

			_lstSoldiers->setRowColor(titleRow, Palette::blockOffset(15)-1);

			titleChosen = true;
		}

		if (noun == "noNoun")
			++commList;
	}
}

/**
 * dTor.
 */
CommendationState::~CommendationState()
{
}

/**
 * Returns to the previous screen.
 * @param action Pointer to an action.
 */
void CommendationState::btnOkClick(Action*)
{
	_game->popState();
}

}
