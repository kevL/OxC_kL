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

#include "OptionsBattlescapeState.h"

#include <sstream>

#include "../Engine/Action.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"

#include "../Interface/ComboBox.h"
#include "../Interface/Slider.h"
#include "../Interface/Text.h"
#include "../Interface/TextButton.h"
#include "../Interface/ToggleTextButton.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"


namespace OpenXcom
{

/**
 * Initializes all the elements in the Battlescape Options screen.
 * @param origin - game section that originated this state
 */
OptionsBattlescapeState::OptionsBattlescapeState(OptionsOrigin origin)
	:
		OptionsBaseState(origin)
{
	setCategory(_btnBattlescape);

	_txtEdgeScroll	= new Text(114, 9, 94, 8);
	_cbxEdgeScroll	= new ComboBox(this, 104, 16, 94, 18);

	_txtDragScroll	= new Text(114, 9, 206, 8);
	_cbxDragScroll	= new ComboBox(this, 104, 16, 206, 18);

	_txtScrollSpeed	= new Text(114, 9, 94, 40);
	_slrScrollSpeed	= new Slider(104, 16, 94, 50);

	_txtFireSpeed	= new Text(114, 9, 206, 40);
	_slrFireSpeed	= new Slider(104, 16, 206, 50);

	_txtXcomSpeed	= new Text(114, 9, 94, 72);
	_slrXcomSpeed	= new Slider(104, 16, 94, 82);

	_txtAlienSpeed	= new Text(114, 9, 206, 72);
	_slrAlienSpeed	= new Slider(104, 16, 206, 82);

	_txtPathPreview	= new Text(114, 9, 94, 100);
	_btnArrows		= new ToggleTextButton(104, 16, 94, 110);
	_btnTuCost		= new ToggleTextButton(104, 16, 94, 128);

	_txtOptions		= new Text(114, 9, 206, 100);
	_btnTooltips	= new ToggleTextButton(104, 16, 206, 110);
	_btnDeaths		= new ToggleTextButton(104, 16, 206, 128);

	add(_txtEdgeScroll);
	add(_txtDragScroll);

	add(_txtScrollSpeed);
	add(_slrScrollSpeed);

	add(_txtFireSpeed);
	add(_slrFireSpeed);

	add(_txtXcomSpeed);
	add(_slrXcomSpeed);

	add(_txtAlienSpeed);
	add(_slrAlienSpeed);

	add(_txtPathPreview);
	add(_btnArrows);
	add(_btnTuCost);

	add(_txtOptions);
	add(_btnTooltips);
	add(_btnDeaths);

	add(_cbxEdgeScroll);
	add(_cbxDragScroll);

	centerAllSurfaces();

	_txtEdgeScroll->setColor(Palette::blockOffset(8)+10);
	_txtEdgeScroll->setText(tr("STR_EDGE_SCROLL"));

	std::vector<std::string> edgeScrolls;
	edgeScrolls.push_back("STR_DISABLED");
	edgeScrolls.push_back("STR_TRIGGER_SCROLL");
	edgeScrolls.push_back("STR_AUTO_SCROLL");

	_cbxEdgeScroll->setColor(Palette::blockOffset(15)-1);
	_cbxEdgeScroll->setOptions(edgeScrolls);
	_cbxEdgeScroll->setSelected(Options::battleEdgeScroll);
	_cbxEdgeScroll->onChange((ActionHandler)& OptionsBattlescapeState::cbxEdgeScrollChange);
	_cbxEdgeScroll->setTooltip("STR_EDGE_SCROLL_DESC");
	_cbxEdgeScroll->onMouseIn((ActionHandler)& OptionsBattlescapeState::txtTooltipIn);
	_cbxEdgeScroll->onMouseOut((ActionHandler)& OptionsBattlescapeState::txtTooltipOut);

	_txtDragScroll->setColor(Palette::blockOffset(8)+10);
	_txtDragScroll->setText(tr("STR_DRAG_SCROLL"));

	std::vector<std::string> dragScrolls;
	dragScrolls.push_back("STR_DISABLED");
	dragScrolls.push_back("STR_LEFT_MOUSE_BUTTON");
	dragScrolls.push_back("STR_MIDDLE_MOUSE_BUTTON");
	dragScrolls.push_back("STR_RIGHT_MOUSE_BUTTON");

	_cbxDragScroll->setColor(Palette::blockOffset(15)-1);
	_cbxDragScroll->setOptions(dragScrolls);
	_cbxDragScroll->setSelected(Options::battleDragScrollButton);
	_cbxDragScroll->onChange((ActionHandler)& OptionsBattlescapeState::cbxDragScrollChange);
	_cbxDragScroll->setTooltip("STR_DRAG_SCROLL_DESC");
	_cbxDragScroll->onMouseIn((ActionHandler)& OptionsBattlescapeState::txtTooltipIn);
	_cbxDragScroll->onMouseOut((ActionHandler)& OptionsBattlescapeState::txtTooltipOut);

	_txtScrollSpeed->setColor(Palette::blockOffset(8)+10);
	_txtScrollSpeed->setText(tr("STR_SCROLL_SPEED"));

	_slrScrollSpeed->setColor(Palette::blockOffset(15)-1);
	_slrScrollSpeed->setRange(2, 20);
	_slrScrollSpeed->setValue(Options::battleScrollSpeed);
	_slrScrollSpeed->onChange((ActionHandler)& OptionsBattlescapeState::slrScrollSpeedChange);
	_slrScrollSpeed->setTooltip("STR_SCROLL_SPEED_BATTLE_DESC");
	_slrScrollSpeed->onMouseIn((ActionHandler)& OptionsBattlescapeState::txtTooltipIn);
	_slrScrollSpeed->onMouseOut((ActionHandler)& OptionsBattlescapeState::txtTooltipOut);

	_txtFireSpeed->setColor(Palette::blockOffset(8)+10);
	_txtFireSpeed->setText(tr("STR_FIRE_SPEED"));

	_slrFireSpeed->setColor(Palette::blockOffset(15)-1);
	_slrFireSpeed->setRange(1, 20);
	_slrFireSpeed->setValue(Options::battleFireSpeed);
	_slrFireSpeed->onChange((ActionHandler)& OptionsBattlescapeState::slrFireSpeedChange);
	_slrFireSpeed->setTooltip("STR_FIRE_SPEED_DESC");
	_slrFireSpeed->onMouseIn((ActionHandler)& OptionsBattlescapeState::txtTooltipIn);
	_slrFireSpeed->onMouseOut((ActionHandler)& OptionsBattlescapeState::txtTooltipOut);

	_txtXcomSpeed->setColor(Palette::blockOffset(8)+10);
	_txtXcomSpeed->setText(tr("STR_PLAYER_MOVEMENT_SPEED"));

	_slrXcomSpeed->setColor(Palette::blockOffset(15)-1);
//kL	_slrXcomSpeed->setRange(40, 1);
	_slrXcomSpeed->setRange(80, 1); // kL
	_slrXcomSpeed->setValue(Options::battleXcomSpeed);
	_slrXcomSpeed->onChange((ActionHandler)& OptionsBattlescapeState::slrXcomSpeedChange);
	_slrXcomSpeed->setTooltip("STR_PLAYER_MOVEMENT_SPEED_DESC");
	_slrXcomSpeed->onMouseIn((ActionHandler)& OptionsBattlescapeState::txtTooltipIn);
	_slrXcomSpeed->onMouseOut((ActionHandler)& OptionsBattlescapeState::txtTooltipOut);

	_txtAlienSpeed->setColor(Palette::blockOffset(8)+10);
	_txtAlienSpeed->setText(tr("STR_COMPUTER_MOVEMENT_SPEED"));

	_slrAlienSpeed->setColor(Palette::blockOffset(15)-1);
	_slrAlienSpeed->setRange(40, 1);
	_slrAlienSpeed->setValue(Options::battleAlienSpeed);
	_slrAlienSpeed->onChange((ActionHandler)& OptionsBattlescapeState::slrAlienSpeedChange);
	_slrAlienSpeed->setTooltip("STR_COMPUTER_MOVEMENT_SPEED_DESC");
	_slrAlienSpeed->onMouseIn((ActionHandler)& OptionsBattlescapeState::txtTooltipIn);
	_slrAlienSpeed->onMouseOut((ActionHandler)& OptionsBattlescapeState::txtTooltipOut);

	_txtPathPreview->setColor(Palette::blockOffset(8)+10);
	_txtPathPreview->setText(tr("STR_PATH_PREVIEW"));

	_btnArrows->setColor(Palette::blockOffset(15)-1);
	_btnArrows->setText(tr("STR_PATH_ARROWS"));
	_btnArrows->setPressed((Options::battleNewPreviewPath & PATH_ARROWS) != 0);
	_btnArrows->onMouseClick((ActionHandler)& OptionsBattlescapeState::btnPathPreviewClick);
	_btnArrows->setTooltip("STR_PATH_ARROWS_DESC");
	_btnArrows->onMouseIn((ActionHandler)& OptionsBattlescapeState::txtTooltipIn);
	_btnArrows->onMouseOut((ActionHandler)& OptionsBattlescapeState::txtTooltipOut);

	_btnTuCost->setColor(Palette::blockOffset(15)-1);
	_btnTuCost->setText(tr("STR_PATH_TIME_UNIT_COST"));
	_btnTuCost->setPressed((Options::battleNewPreviewPath & PATH_TU_COST) != 0);
	_btnTuCost->onMouseClick((ActionHandler)& OptionsBattlescapeState::btnPathPreviewClick);
	_btnTuCost->setTooltip("STR_PATH_TIME_UNIT_COST_DESC");
	_btnTuCost->onMouseIn((ActionHandler)& OptionsBattlescapeState::txtTooltipIn);
	_btnTuCost->onMouseOut((ActionHandler)& OptionsBattlescapeState::txtTooltipOut);

	_txtOptions->setColor(Palette::blockOffset(8)+10);
	_txtOptions->setText(tr("STR_USER_INTERFACE_OPTIONS"));

	_btnTooltips->setColor(Palette::blockOffset(15)-1);
	_btnTooltips->setText(tr("STR_TOOLTIPS"));
	_btnTooltips->setPressed(Options::battleTooltips);
	_btnTooltips->onMouseClick((ActionHandler)& OptionsBattlescapeState::btnTooltipsClick);
	_btnTooltips->setTooltip("STR_TOOLTIPS_DESC");
	_btnTooltips->onMouseIn((ActionHandler)& OptionsBattlescapeState::txtTooltipIn);
	_btnTooltips->onMouseOut((ActionHandler)& OptionsBattlescapeState::txtTooltipOut);

	_btnDeaths->setColor(Palette::blockOffset(15)-1);
	_btnDeaths->setText(tr("STR_DEATH_NOTIFICATIONS"));
	_btnDeaths->setPressed(Options::battleNotifyDeath);
	_btnDeaths->onMouseClick((ActionHandler)& OptionsBattlescapeState::btnDeathsClick);
	_btnDeaths->setTooltip("STR_DEATH_NOTIFICATIONS_DESC");
	_btnDeaths->onMouseIn((ActionHandler)& OptionsBattlescapeState::txtTooltipIn);
	_btnDeaths->onMouseOut((ActionHandler)& OptionsBattlescapeState::txtTooltipOut);
}

/**
 * dTor.
 */
OptionsBattlescapeState::~OptionsBattlescapeState()
{

}

/**
 * Changes the Edge Scroll option.
 * @param action - pointer to an action
 */
void OptionsBattlescapeState::cbxEdgeScrollChange(Action*)
{
	Options::battleEdgeScroll = (ScrollType)_cbxEdgeScroll->getSelected();
}

/**
 * Changes the Drag Scroll option.
 * @param action - pointer to an action
 */
void OptionsBattlescapeState::cbxDragScrollChange(Action*)
{
	Options::battleDragScrollButton = _cbxDragScroll->getSelected();
}

/**
 * Updates the scroll speed.
 * @param action - pointer to an action
 */
void OptionsBattlescapeState::slrScrollSpeedChange(Action*)
{
	Options::battleScrollSpeed = _slrScrollSpeed->getValue();
}

/**
 * Updates the fire speed.
 * @param action - pointer to an action
 */
void OptionsBattlescapeState::slrFireSpeedChange(Action*)
{
	Options::battleFireSpeed = _slrFireSpeed->getValue();
}

/**
 * Updates the X-COM movement speed.
 * @param action - pointer to an action
 */
void OptionsBattlescapeState::slrXcomSpeedChange(Action*)
{
	Options::battleXcomSpeed = _slrXcomSpeed->getValue();
}

/**
 * Updates the alien movement speed.
 * @param action - pointer to an action
 */
void OptionsBattlescapeState::slrAlienSpeedChange(Action*)
{
	Options::battleAlienSpeed = _slrAlienSpeed->getValue();
}

/**
 * Updates the path preview options.
 * @param action - pointer to an action
 */
void OptionsBattlescapeState::btnPathPreviewClick(Action*)
{
	int mode = PATH_NONE;

	if (_btnArrows->getPressed())
		mode |= PATH_ARROWS;

	if (_btnTuCost->getPressed())
		mode |= PATH_TU_COST;

	Options::battleNewPreviewPath = (PathPreview)mode;
}

/**
 * Updates the Tooltips option.
 * @param action - pointer to an action
 */
void OptionsBattlescapeState::btnTooltipsClick(Action*)
{
	Options::battleTooltips = _btnTooltips->getPressed();
}

/**
 * Updates the Death Notifications option.
 * @param action - pointer to an action
 */
void OptionsBattlescapeState::btnDeathsClick(Action*)
{
	Options::battleNotifyDeath = _btnDeaths->getPressed();
}

}
