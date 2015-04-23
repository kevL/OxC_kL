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
	_window			= new Window(this, 320, 200);
	_txtTitle		= new Text(300, 16, 10, 8);
	_lstSoldiers	= new TextList(285, 121, 16, 28);
	_txtMedalInfo	= new Text(280, 25, 20, 150);
	_btnOk			= new TextButton(288, 16, 16, 177);

	setPalette("PAL_GEOSCAPE", 0);

	add(_window);
	add(_txtTitle);
	add(_lstSoldiers);
	add(_txtMedalInfo);
	add(_btnOk);

	centerAllSurfaces();


	_window->setColor(Palette::blockOffset(15)-1); // green
	_window->setBackground(_game->getResourcePack()->getSurface("BACK01.SCR"));

	_txtTitle->setColor(Palette::blockOffset(8)+5); // cyan
	_txtTitle->setText(tr("STR_MEDALS"));
	_txtTitle->setAlign(ALIGN_CENTER);
	_txtTitle->setBig();

	_lstSoldiers->setColor(Palette::blockOffset(8)+10);
	_lstSoldiers->setColumns(2, 200, 77);
	_lstSoldiers->setBackground(_window);
	_lstSoldiers->setSelectable();
	_lstSoldiers->setMargin();
	_lstSoldiers->onMouseOver((ActionHandler)& CommendationState::lstInfoMouseOver);
	_lstSoldiers->onMouseOut((ActionHandler)& CommendationState::lstInfoMouseOut);

	_txtMedalInfo->setColor(Palette::blockOffset(10)); // slate
	_txtMedalInfo->setHighContrast();
	_txtMedalInfo->setWordWrap();

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
			_lstSoldiers->addRow(2, L"", L""); // Blank row, will be filled in later -> unless it's the last row ......
			_titleRows.insert(std::pair<size_t, std::string>(
															row++,
															""));
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
						skip = 0,
						lastInt = -2,
						thisInt = -1;
					size_t j = 0;

					for (std::vector<int>::const_iterator
							i = (*award).second->getCriteria()->begin()->second.begin();
							i != (*award).second->getCriteria()->begin()->second.end();
							++i)
					{
						if (j == (*soldierAward)->getDecorLevelInt() + 1)
							break;

						thisInt = *i;
						if (i != (*award).second->getCriteria()->begin()->second.begin())
						{
							--i;
							lastInt = *i;
							++i;
						}

						if (thisInt == lastInt)
							++skip;

						++j;
					}

					_lstSoldiers->addRow(
									2,
									wss.str().c_str(),
									tr((*soldierAward)->getDecorLevelType(skip)).c_str());
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


			std::string info = (*award).second->getDescriptionGeneral(); // look for Generic Desc first.
			if (info.empty() == false)
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
CommendationState::~CommendationState()
{}

/**
 * Returns to the previous screen.
 * @param action - pointer to an Action
 */
void CommendationState::btnOkClick(Action*)
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
void CommendationState::lstInfoMouseOver(Action*)
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
void CommendationState::lstInfoMouseOut(Action*)
{
	_txtMedalInfo->setText(L"");
}

}
