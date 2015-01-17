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

#include "CommendationState.h"

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

#include "../Savegame/SoldierDiary.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Medals screen post-mission.
 * @param soldiersMedalled - vector of pointers to Soldier objects
 */
CommendationState::CommendationState(std::vector<Soldier*> soldiersMedalled)
{
	_window			= new Window(this, 320, 200, 0, 0);
	_txtTitle		= new Text(300, 16, 10, 8);
	_lstSoldiers	= new TextList(285, 145, 16, 28);
	_btnOk			= new TextButton(288, 16, 16, 177);

	setPalette("PAL_GEOSCAPE", 0);

//	_game->getResourcePack()->playMusic(OpenXcom::res_MUSIC_TAC_AWARDS); // note: Moved to DebriefingState.

	add(_window);
	add(_txtTitle);
	add(_lstSoldiers);
	add(_btnOk);

	centerAllSurfaces();


	_window->setColor(Palette::blockOffset(15)-1);
	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_txtTitle->setColor(Palette::blockOffset(8)+5);
	_txtTitle->setText(tr("STR_MEDALS"));
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_lstSoldiers->setColor(Palette::blockOffset(8)+10);
	_lstSoldiers->setColumns(2, 200, 77);
	_lstSoldiers->setBackground(_window);
	_lstSoldiers->setSelectable();
	_lstSoldiers->setMargin();

	_btnOk->setColor(Palette::blockOffset(15)-1);
	_btnOk->setText(tr("STR_OK"));
	_btnOk->onMouseClick((ActionHandler)& CommendationState::btnOkClick);
	_btnOk->onKeyboardPress(
					(ActionHandler)& CommendationState::btnOkClick,
					Options::keyOk);
	_btnOk->onKeyboardPress(
					(ActionHandler)& CommendationState::btnOkClick,
					Options::keyCancel);


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

		for (std::vector<Soldier*>::const_iterator
				soldier = soldiersMedalled.begin();
				soldier != soldiersMedalled.end();
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

			_lstSoldiers->setRowColor(titleRow, Palette::blockOffset(15)-1);

			titleChosen = true;
		}

		if (noun == "noNoun")
			++award;
	}
}

/**
 * dTor.
 */
CommendationState::~CommendationState()
{}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void CommendationState::btnOkClick(Action*)
{
	//Log(LOG_INFO) << "Commendation, states = " << _game->getQtyStates();
	if (_game->getQtyStates() == 2) // ie: (1) this, (2) Geoscape
	{
		_game->getResourcePack()->fadeMusic(_game, 863);
		_game->getResourcePack()->playMusic(OpenXcom::res_MUSIC_GEO_GLOBE);
	}

	_game->popState();
}

}
