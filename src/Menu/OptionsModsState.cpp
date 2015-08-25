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

#include "OptionsModsState.h"

//#include <algorithm>

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Options.h"

#include "../Interface/Text.h"
#include "../Interface/TextList.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Mod Options window.
 * @param origin Game section that originated this state.
 */
OptionsModsState::OptionsModsState(OptionsOrigin origin)
	:
		OptionsBaseState(origin)
{
	setCategory(_btnMods);

	_lstMods = new TextList(200, 137, 94, 8);
	add(_lstMods, "optionLists", "modsMenu");

	centerAllSurfaces();

	// how much room do we need for YES/NO
	Text text = Text(100, 9);
	text.initText(
				_game->getResourcePack()->getFont("FONT_BIG"),
				_game->getResourcePack()->getFont("FONT_SMALL"),
				_game->getLanguage());
	text.setText(tr("STR_YES"));
	int yes = text.getTextWidth();
	text.setText(tr("STR_NO"));
	int no = text.getTextWidth();

	int
		rightcol = std::max(yes, no) + 2,
		leftcol = _lstMods->getWidth() - rightcol;

	_lstMods->setAlign(ALIGN_RIGHT, 1);
	_lstMods->setColumns(2, leftcol, rightcol);
	_lstMods->setWordWrap();
	_lstMods->setSelectable();
	_lstMods->setBackground(_window);
	_lstMods->onMouseClick((ActionHandler)& OptionsModsState::lstModsClick);
//	_lstMods->setTooltip("STR_MODS_DESC");
//	_lstMods->onMouseIn((ActionHandler)& OptionsModsState::txtTooltipIn);
//	_lstMods->onMouseOut((ActionHandler)& OptionsModsState::txtTooltipOut);

	std::vector<std::string> rulesets = CrossPlatform::getDataContents("Ruleset/");
	for (std::vector<std::string>::iterator
			i = rulesets.begin();
			i != rulesets.end();
			++i)
	{
		std::string file = *i;
		std::transform(
					file.begin(),
					file.end(),
					file.begin(),
					tolower);

		if ((file.length() > 4
				&& file.substr(file.length() - 4, 4) == ".rul")
			|| CrossPlatform::getDataContents("Ruleset/" + *i, "rul").empty() == false)
		{
			std::string mod = CrossPlatform::noExt(*i);
			std::wstring modName = Language::fsToWstr(mod);
			Language::replace(
							modName,
							L"_",
							L" ");

			if (mod != "Xcom1Ruleset") // ignore default ruleset
			{
				bool modEnabled = (std::find(
										Options::rulesets.begin(),
										Options::rulesets.end(),
										mod) != Options::rulesets.end());
				_lstMods->addRow(
							2,
							modName.c_str(),
							modEnabled ? tr("STR_YES").c_str() : tr("STR_NO").c_str());

				_mods.push_back(mod);
			}
		}
	}
}

/**
 * dTor.
 */
OptionsModsState::~OptionsModsState()
{}

void OptionsModsState::lstModsClick(Action*)
{
	std::string selectedMod = _mods[_lstMods->getSelectedRow()];
	std::vector<std::string>::iterator i = std::find(
													Options::rulesets.begin(),
													Options::rulesets.end(),
													selectedMod);
	if (i != Options::rulesets.end())
	{
		_lstMods->setCellText(_lstMods->getSelectedRow(), 1, tr("STR_NO").c_str());
		Options::rulesets.erase(i);
	}
	else
	{
		_lstMods->setCellText(_lstMods->getSelectedRow(), 1, tr("STR_YES").c_str());
		Options::rulesets.push_back(selectedMod);
	}

	Options::reload = true;
}

}
