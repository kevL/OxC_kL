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

#include "Inventory.h"

//#include <cmath>

#include "PrimeGrenadeState.h"
#include "TileEngine.h"
#include "WarningMessage.h"

#include "../Engine/Action.h"
#include "../Engine/Font.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
#include "../Engine/Logger.h"
#include "../Engine/Options.h"
#include "../Engine/Palette.h"
#include "../Engine/Sound.h"
#include "../Engine/SurfaceSet.h"
#include "../Engine/Timer.h"

#include "../Interface/Text.h"
#include "../Interface/NumberText.h"

#include "../Ruleset/RuleInterface.h"
#include "../Ruleset/RuleInventory.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/Ruleset.h"

#include "../Resource/ResourcePack.h"

#include "../Savegame/BattleItem.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/Tile.h"


namespace OpenXcom
{

/**
 * Sets up an inventory with the specified size and position.
 * @param game		- pointer to core Game
 * @param width		- width in pixels
 * @param height	- height in pixels
 * @param x			- X position in pixels
 * @param y			- Y position in pixels
 * @param base		- true if inventory is accessed from Basescape's CraftEquip screen
 */
Inventory::Inventory(
		Game* game,
		int width,
		int height,
		int x,
		int y,
		bool base)
	:
		InteractiveSurface(
			width,
			height,
			x,
			y),
		_game(game),
		_selUnit(NULL),
		_selItem(NULL),
		_tu(true),
		_base(base),
		_groundOffset(0),
		_fuseFrame(0),
		_grenadeFuses(),	// kL
		_primeGrenade(-1),	// kL
		_tuCost(-1)			// kL
{
	_depth = _game->getSavedGame()->getSavedBattle()->getDepth();

	_grid			= new Surface(width, height, x, y);
	_items			= new Surface(width, height, x, y);
	_selection		= new Surface(
								RuleInventory::HAND_W * RuleInventory::SLOT_W,
								RuleInventory::HAND_H * RuleInventory::SLOT_H,
								x,
								y);
	_warning		= new WarningMessage(224, 24, 48, 176);

	_stackNumber	= new NumberText(15, 15, 0, 0);
	_stackNumber->setBordered(true);

	_warning->initText(
					_game->getResourcePack()->getFont("FONT_BIG"),
					_game->getResourcePack()->getFont("FONT_SMALL"),
					_game->getLanguage());
	_warning->setColor(_game->getRuleset()->getInterface("battlescape")->getElement("warning")->color2); //Palette::blockOffset(2));
	_warning->setTextColor(_game->getRuleset()->getInterface("battlescape")->getElement("warning")->color); //Palette::blockOffset(1)-1);

	_animTimer = new Timer(80);
	_animTimer->onTimer((SurfaceHandler)& Inventory::drawPrimers);
	_animTimer->start();
}

/**
 * Deletes inventory surfaces.
 */
Inventory::~Inventory()
{
	delete _grid;
	delete _items;
	delete _selection;
	delete _warning;
	delete _stackNumber;
	delete _animTimer;
}

/**
 * Replaces a certain amount of colors in the inventory's palette.
 * @param colors		- pointer to the set of colors
 * @param firstcolor	- offset of the first color to replace
 * @param ncolors		- amount of colors to replace
 */
void Inventory::setPalette(
		SDL_Color* colors,
		int firstcolor,
		int ncolors)
{
	Surface::setPalette(colors, firstcolor, ncolors);

	_grid->setPalette(colors, firstcolor, ncolors);
	_items->setPalette(colors, firstcolor, ncolors);
	_selection->setPalette(colors, firstcolor, ncolors);
	_warning->setPalette(colors, firstcolor, ncolors);

	_stackNumber->setPalette(getPalette());
}

/**
 * Changes the inventory's Time Units mode.
 * - when True inventory actions cost soldier time units (for battle);
 * - when False inventory actions don't cost anything (for pre-equip).
 * @param tu - true for Time Units mode; false for pre-battle equip
 */
void Inventory::setTuMode(bool tu)
{
	_tu = tu;
}

/**
 * Changes the unit to display the inventory of.
 * @param unit - pointer to a BattleUnit
 */
void Inventory::setSelectedUnit(BattleUnit* unit)
{
	_selUnit = unit;
	_groundOffset = 999;

	arrangeGround();
}

/**
 * Draws the inventory elements.
 */
void Inventory::draw()
{
	drawGrid();
	drawItems();
}

/**
 * Draws the inventory grid for item placement.
 */
void Inventory::drawGrid()
{
	_grid->clear();

	Text text = Text(16, 9, 0, 0);
	text.setPalette(_grid->getPalette());
	text.setHighContrast();
	text.initText(
				_game->getResourcePack()->getFont("FONT_BIG"),
				_game->getResourcePack()->getFont("FONT_SMALL"),
				_game->getLanguage());

	RuleInterface* rule = _game->getRuleset()->getInterface("inventory");
	text.setColor(rule->getElement("textSlots")->color); // Palette::blockOffset(4)-1

	Uint8 color = rule->getElement("grid")->color; // Palette::blockOffset(0)+8
	bool doLabel;

	for (std::map<std::string, RuleInventory*>::const_iterator
			i = _game->getRuleset()->getInventories()->begin();
			i != _game->getRuleset()->getInventories()->end();
			++i)
	{
		doLabel = true;

		if (i->second->getType() == INV_SLOT) // draw grid
		{
			for (std::vector<RuleSlot>::const_iterator
					j = i->second->getSlots()->begin();
					j != i->second->getSlots()->end();
					++j)
			{
				SDL_Rect r;

				r.x = i->second->getX() + RuleInventory::SLOT_W * j->x;
				r.y = i->second->getY() + RuleInventory::SLOT_H * j->y;
				r.w = RuleInventory::SLOT_W + 1;
				r.h = RuleInventory::SLOT_H + 1;
				_grid->drawRect(&r, color);

				r.x++;
				r.y++;
				r.w -= 2;
				r.h -= 2;
				_grid->drawRect(&r, 0);
			}
		}
		else if (i->second->getType() == INV_HAND) // draw grid
		{
			SDL_Rect r;

			r.x = i->second->getX();
			r.y = i->second->getY();
			r.w = RuleInventory::HAND_W * RuleInventory::SLOT_W;
			r.h = RuleInventory::HAND_H * RuleInventory::SLOT_H;
			_grid->drawRect(&r, color);

			r.x++;
			r.y++;
			r.w -= 2;
			r.h -= 2;
			_grid->drawRect(&r, 0);
		}
		else if (i->second->getType() == INV_GROUND) // draw grid
		{
			doLabel = false;

			for (int
					x = i->second->getX();
					x <= 320;
					x += RuleInventory::SLOT_W)
			{
				for (int
						y = i->second->getY();
						y <= 200;
						y += RuleInventory::SLOT_H)
				{
					SDL_Rect r;

					r.x = x;
					r.y = y;
					r.w = RuleInventory::SLOT_W + 1;
					r.h = RuleInventory::SLOT_H + 1;
					_grid->drawRect(&r, color);

					r.x++;
					r.y++;
					r.w -= 2;
					r.h -= 2;
					_grid->drawRect(&r, 0);
				}
			}
		}

		if (doLabel == true)
		{
			text.setX(i->second->getX()); // draw label
			text.setY(
					i->second->getY()
						- text.getFont()->getHeight()
						- text.getFont()->getSpacing());
			text.setText(_game->getLanguage()->getString(i->second->getId()));
			text.blit(_grid);
		}
	}
}

/**
 * Draws the items contained in the soldier's inventory.
 */
void Inventory::drawItems()
{
	_items->clear();
	_grenadeFuses.clear();

	Uint8 color = _game->getRuleset()->getInterface("inventory")->getElement("numStack")->color;

	if (_selUnit != NULL)
	{
		SurfaceSet* texture = _game->getResourcePack()->getSurfaceSet("BIGOBS.PCK");

		for (std::vector<BattleItem*>::const_iterator // Soldier items
				i = _selUnit->getInventory()->begin();
				i != _selUnit->getInventory()->end();
				++i)
		{
			if (*i == _selItem)
				continue;


			Surface* frame = texture->getFrame((*i)->getRules()->getBigSprite());
			if (frame != NULL) // kL, safety.
			{
				if ((*i)->getSlot()->getType() == INV_SLOT)
				{
					frame->setX((*i)->getSlot()->getX() + (*i)->getSlotX() * RuleInventory::SLOT_W);
					frame->setY((*i)->getSlot()->getY() + (*i)->getSlotY() * RuleInventory::SLOT_H);
				}
				else if ((*i)->getSlot()->getType() == INV_HAND)
				{
					frame->setX(
							(*i)->getSlot()->getX()
									+ (RuleInventory::HAND_W - (*i)->getRules()->getInventoryWidth())
								* RuleInventory::SLOT_W / 2);
					frame->setY(
							(*i)->getSlot()->getY()
									+ (RuleInventory::HAND_H - (*i)->getRules()->getInventoryHeight())
								* RuleInventory::SLOT_H / 2);
				}

				texture->getFrame((*i)->getRules()->getBigSprite())->blit(_items);

				if ((*i)->getFuseTimer() > -1) // grenade primer indicators
					_grenadeFuses.push_back(std::make_pair(
														frame->getX(),
														frame->getY()));
			}
			//else Log(LOG_INFO) << "ERROR : bigob not found #" << (*i)->getRules()->getBigSprite(); // kL
		}

		Surface* stackLayer = new Surface(
										getWidth(),
										getHeight(),
										0,
										0);
		stackLayer->setPalette(getPalette());

		for (std::vector<BattleItem*>::iterator // Ground items
				i = _selUnit->getTile()->getInventory()->begin();
				i != _selUnit->getTile()->getInventory()->end();
				++i)
		{
			// note that you can make items invisible by setting their width
			// or height to 0 (for example used with tank corpse items)
			if (*i == _selItem
				|| (*i)->getSlotX() < _groundOffset
				|| (*i)->getRules()->getInventoryHeight() == 0
				|| (*i)->getRules()->getInventoryWidth() == 0)
			{
				continue;
			}

//			Log(LOG_INFO) << "Inventory::drawItems() bigSprite = " << (*i)->getRules()->getBigSprite();
			Surface* frame = texture->getFrame((*i)->getRules()->getBigSprite());
			if (frame != NULL) // kL, safety.
			{
				frame->setX(
							(*i)->getSlot()->getX()
								+ ((*i)->getSlotX() - _groundOffset) * RuleInventory::SLOT_W);
				frame->setY(
							(*i)->getSlot()->getY()
								+ ((*i)->getSlotY() * RuleInventory::SLOT_H));
				texture->getFrame((*i)->getRules()->getBigSprite())->blit(_items);

				if ((*i)->getFuseTimer() > -1) // grenade primer indicators
					_grenadeFuses.push_back(std::make_pair(
														frame->getX(),
														frame->getY()));
			}
			//else Log(LOG_INFO) << "ERROR : bigob not found #" << (*i)->getRules()->getBigSprite(); // kL

			if (_stackLevel[(*i)->getSlotX()][(*i)->getSlotY()] > 1) // item stacking
			{
				_stackNumber->setX(
								((*i)->getSlot()->getX()
									+ (((*i)->getSlotX() + (*i)->getRules()->getInventoryWidth()) - _groundOffset)
										* RuleInventory::SLOT_W)
									- 4);

				if (_stackLevel[(*i)->getSlotX()][(*i)->getSlotY()] > 9)
				{
					_stackNumber->setX(_stackNumber->getX()-4);
				}

				_stackNumber->setY(
								((*i)->getSlot()->getY()
									+ ((*i)->getSlotY() + (*i)->getRules()->getInventoryHeight())
										* RuleInventory::SLOT_H)
									- 6);
				_stackNumber->setValue(_stackLevel[(*i)->getSlotX()][(*i)->getSlotY()]);
				_stackNumber->draw();
				_stackNumber->setColor(color);
//				_stackNumber->setColor(Palette::blockOffset(4)+2);
				_stackNumber->blit(stackLayer);
			}
		}

		stackLayer->blit(_items);

		delete stackLayer;
	}
}

/**
 * Moves an item to a specified slot in the selected unit's inventory.
 * @param item	- pointer to a battle item
 * @param slot	- pointer to an inventory slot, or NULL if none
 * @param x		- X position in slot (default 0)
 * @param y		- Y position in slot (default 0)
 */
void Inventory::moveItem(
		BattleItem* item,
		RuleInventory* slot,
		int x,
		int y)
{
	if (slot == NULL) // Make items vanish (eg. ammo in weapons)
	{
		if (item->getSlot()->getType() == INV_GROUND)
			_selUnit->getTile()->removeItem(item);
		else
			item->moveToOwner(NULL);
	}
	else
	{
		if (slot != item->getSlot()) // Handle dropping from/to ground.
		{
			if (slot->getType() == INV_GROUND)
			{
				item->moveToOwner(NULL);
				_selUnit->getTile()->addItem(item, item->getSlot());

				if (item->getUnit()
					&& item->getUnit()->getStatus() == STATUS_UNCONSCIOUS)
				{
					item->getUnit()->setPosition(_selUnit->getPosition());
				}
			}
			else if (item->getSlot() == NULL
				|| item->getSlot()->getType() == INV_GROUND)
			{
				item->moveToOwner(_selUnit);
				_selUnit->getTile()->removeItem(item);
//				item->setTurnFlag(false);

				if (item->getUnit()
					&& item->getUnit()->getStatus() == STATUS_UNCONSCIOUS)
				{
					item->getUnit()->setPosition(Position(-1,-1,-1));
				}
			}
		}

		item->setSlot(slot);
		item->setSlotX(x);
		item->setSlotY(y);
	}
}

/**
 * Checks if an item in a certain slot position would overlap with any other inventory item.
 * @param unit	- pointer to current unit
 * @param item	- pointer to battle item
 * @param slot	- pointer to inventory slot, or NULL if none
 * @param x		- X position in slot
 * @param y		- Y position in slot
 * @return, true if there's overlap
 */
bool Inventory::overlapItems(
		BattleUnit* unit,
		BattleItem* item,
		RuleInventory* slot,
		int x,
		int y)
{
	if (slot->getType() != INV_GROUND)
	{
		for (std::vector<BattleItem*>::const_iterator
				i = unit->getInventory()->begin();
				i != unit->getInventory()->end();
				++i)
		{
			if ((*i)->getSlot() == slot
				&& (*i)->occupiesSlot(x, y, item))
			{
				return true;
			}
		}
	}
	else if (unit->getTile() != NULL)
	{
		for (std::vector<BattleItem*>::const_iterator
				i = unit->getTile()->getInventory()->begin();
				i != unit->getTile()->getInventory()->end();
				++i)
		{
			if ((*i)->occupiesSlot(x, y, item))
				return true;
		}
	}

	return false;
}

/**
 * Gets the inventory slot located in the specified mouse position.
 * @param x - pointer to mouse X position; returns the slot's X position
 * @param y - pointer to mouse Y position; returns the slot's Y position
 * @return, pointer to slot rules, or NULL if none
 */
RuleInventory* Inventory::getSlotInPosition(
		int* x,
		int* y) const
{
	for (std::map<std::string, RuleInventory*>::const_iterator
			i = _game->getRuleset()->getInventories()->begin();
			i != _game->getRuleset()->getInventories()->end();
			++i)
	{
		if (i->second->checkSlotInPosition(x, y))
			return i->second;
	}

	return NULL;
}

/**
 * Returns the item currently grabbed by the player.
 * @return, pointer to selected item, or NULL if none
 */
BattleItem* Inventory::getSelectedItem() const
{
	return _selItem;
}

/**
 * Changes the item currently grabbed by the player.
 * @param item - pointer to selected item, or NULL if none
 */
void Inventory::setSelectedItem(BattleItem* item)
{
	_selItem = (item && !item->getRules()->isFixed())? item: NULL;

	if (_selItem == NULL)
		_selection->clear();
	else
	{
		if (_selItem->getSlot()->getType() == INV_GROUND)
			_stackLevel[_selItem->getSlotX()][_selItem->getSlotY()] -= 1;

		_selItem->getRules()->drawHandSprite(
										_game->getResourcePack()->getSurfaceSet("BIGOBS.PCK"),
										_selection);
	}

	drawItems();
}

/**
 * Returns the item currently under mouse cursor.
 * @return, pointer to item, or NULL if none
 */
BattleItem* Inventory::getMouseOverItem() const
{
	return _mouseOverItem;
}

/**
 * Changes the item currently under mouse cursor.
 * @param item - pointer to item, or NULL if none
 */
void Inventory::setMouseOverItem(BattleItem* item)
{
	_mouseOverItem = (item && !item->getRules()->isFixed())? item: NULL;
}

/**
 * Handles WarningMessage timer.
 */
void Inventory::think()
{
	if (_primeGrenade > -1) // kL_begin ->
	{
		std::wstring activated;

		if (_primeGrenade > 0)
			activated = Text::formatNumber(_primeGrenade) + L" ";

		activated += _game->getLanguage()->getString("STR_GRENADE_IS_ACTIVATED");

		if (_primeGrenade > 0)
			activated += L" " + Text::formatNumber(_primeGrenade);

		_warning->showMessage(activated);

		_primeGrenade = -1;
	} // kL_end.

/*	int
		x,
		y;
	SDL_GetMouseState(&x, &y);

	SDL_WarpMouse(x + 1, y);	// send a mouse motion event to refresh any hover actions
	SDL_WarpMouse(x, y); */		// move the mouse back to avoid cursor creep

	_warning->think();
	_animTimer->think(NULL, this);
}

/**
 * Blits the inventory elements.
 * @param surface Pointer to surface to blit onto.
 */
void Inventory::blit(Surface* surface)
{
	clear();

	_grid->blit(this);
	_items->blit(this);
	_selection->blit(this);
	_warning->blit(this);

	Surface::blit(surface);
}

/**
 * kL. Handles the cursor out.
 * @param action - pointer to an Action
 * @param state - state that the action handlers belong to
 */
/* void Inventory::mouseOut(Action* action, State* state) // kL
{
	_tuCost->setVisible(false);
} */

/**
 * Handles the cursor over.
 * @param action - pointer to an Action
 * @param state - state that the action handlers belong to
 */
void Inventory::mouseOver(Action* action, State* state)
{
	_selection->setX(static_cast<int>(
						floor(action->getAbsoluteXMouse())) - (_selection->getWidth() / 2) - getX());
	_selection->setY(static_cast<int>(
						floor(action->getAbsoluteYMouse())) - (_selection->getHeight() / 2) - getY());

	if (_selUnit == NULL)
		return;

	int
		x = static_cast<int>(floor(action->getAbsoluteXMouse())) - getX(),
		y = static_cast<int>(floor(action->getAbsoluteYMouse())) - getY();

	RuleInventory* slot = getSlotInPosition(&x, &y);
	if (slot != NULL)
	{
		std::string warning; // kL_begin:
		if (_tu
			&& _selItem != NULL
			&& fitItem(
					slot,
					_selItem,
					warning,
					true))
		{
			_tuCost = _selItem->getSlot()->getCost(slot);
		}
		else
		{
			_tuCost = -1; // kL_end.

			if (slot->getType() == INV_GROUND)
				x += _groundOffset;

			BattleItem* item = _selUnit->getItem(slot, x, y);
			setMouseOverItem(item);
		}
	}
	else
	{
		_tuCost = -1; // kL
		setMouseOverItem(NULL);
	}

	_selection->setX(static_cast<int>(
						floor(action->getAbsoluteXMouse())) - (_selection->getWidth() / 2) - getX());
	_selection->setY(static_cast<int>(
						floor(action->getAbsoluteYMouse())) - (_selection->getHeight() / 2) - getY());

	InteractiveSurface::mouseOver(action, state);
}

/**
 * Handles the cursor click. Picks up / drops an item.
 * @param action, Pointer to an action.
 * @param state, State that the action handlers belong to.
 */
void Inventory::mouseClick(Action* action, State* state)
{
	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		if (_selUnit == NULL)
			return;

		if (_selItem == NULL) // Pickup item
		{
			int
				x = static_cast<int>(floor(action->getAbsoluteXMouse())) - getX(),
				y = static_cast<int>(floor(action->getAbsoluteYMouse())) - getY();

			RuleInventory* slot = getSlotInPosition(&x, &y);
			if (slot != NULL)
			{
				if (slot->getType() == INV_GROUND)
					x += _groundOffset;

				BattleItem* item = _selUnit->getItem(
													slot,
													x,
													y);
				if (item != NULL)
				{
					if (SDL_GetModState() & KMOD_CTRL)
					{
						bool placed = false;
						bool toGround = true;
						std::string warning;

						RuleInventory* newSlot = NULL;

						if (slot->getType() == INV_HAND
							|| (slot->getType() != INV_GROUND
								&& (_tu == false
									|| _selUnit->getOriginalFaction() != FACTION_PLAYER)))
						{
							newSlot = _game->getRuleset()->getInventory("STR_GROUND");
						}
						else
						{
							if (_selUnit->getItem("STR_RIGHT_HAND") == NULL)
							{
								toGround = false;
								newSlot = _game->getRuleset()->getInventory("STR_RIGHT_HAND");
							}
							else if (_selUnit->getItem("STR_LEFT_HAND") == NULL)
							{
								toGround = false;
								newSlot = _game->getRuleset()->getInventory("STR_LEFT_HAND");
							}
							else if (slot->getType() != INV_GROUND)
								newSlot = _game->getRuleset()->getInventory("STR_GROUND");
						}

						if (newSlot == NULL)
							return;

						if (toGround)
						{
							if (!_tu
								|| _selUnit->spendTimeUnits(item->getSlot()->getCost(newSlot)))
							{
								placed = true;
								moveItem(
										item,
										newSlot);

								arrangeGround(false);

								_game->getResourcePack()->getSoundByDepth(
																		_depth,
																		ResourcePack::ITEM_DROP)
																	->play();
							}
							else
								_warning->showMessage(_game->getLanguage()->getString("STR_NOT_ENOUGH_TIME_UNITS"));
						}
						else
						{
							_stackLevel[item->getSlotX()][item->getSlotY()] -= 1;

							if (fitItem(
										newSlot,
										item,
										warning))
							{
								placed = true;
							}
							else
								_stackLevel[item->getSlotX()][item->getSlotY()] += 1;
						}

						if (placed)
						{
							_mouseOverItem = NULL; // remove cursor info 'cause item is no longer under the cursor.
							mouseOver(action, state);
						}
					}
					else
					{
						setSelectedItem(item);

						int explTurn = item->getFuseTimer();
						if (explTurn > -1)
						{
							std::wstring activated;

							if (explTurn > 0)
								activated = Text::formatNumber(explTurn) + L" ";

							activated += _game->getLanguage()->getString("STR_GRENADE_IS_ACTIVATED");

							if (explTurn > 0)
								activated += L" " + Text::formatNumber(explTurn);

							_warning->showMessage(activated);
						}
					}
				}
			}
		}
		else // Drop item
		{
			int x = _selection->getX()
						+ (RuleInventory::HAND_W - _selItem->getRules()->getInventoryWidth())
							* RuleInventory::SLOT_W / 2
						+ RuleInventory::SLOT_W / 2,
				y = _selection->getY()
						+ (RuleInventory::HAND_H - _selItem->getRules()->getInventoryHeight())
							* RuleInventory::SLOT_H / 2
						+ RuleInventory::SLOT_H / 2;

			RuleInventory* slot = getSlotInPosition(&x, &y);
			if (slot != NULL)
			{
				if (slot->getType() == INV_GROUND)
					x += _groundOffset;

				BattleItem* item = _selUnit->getItem(
													slot,
													x,
													y);

				bool canStack = (slot->getType() == INV_GROUND
									&& canBeStacked(
												item,
												_selItem));

				if (item == NULL // Put item in empty slot, or stack it, if possible.
					|| item == _selItem
					|| canStack)
				{
					if (!overlapItems(_selUnit, _selItem, slot, x, y)
						&& slot->fitItemInSlot(
											_selItem->getRules(),
											x,
											y))
					{
						if (!_tu
							|| _selUnit->spendTimeUnits(_selItem->getSlot()->getCost(slot)))
						{
							_tuCost = -1; // kL

							moveItem(
									_selItem,
									slot,
									x,
									y);

							if (slot->getType() == INV_GROUND)
								_stackLevel[x][y] += 1;

							setSelectedItem(NULL);
							_game->getResourcePack()->getSoundByDepth(
																	_depth,
																	ResourcePack::ITEM_DROP)
																->play();
						}
						else
						{
							_warning->showMessage(_game->getLanguage()->getString("STR_NOT_ENOUGH_TIME_UNITS"));
//							mouseOver(action, state); // kL, refresh tuCost visibility.
//							arrangeGround(false); // kL, refresh tuCost visibility.
						}
					}
					else if (canStack)
					{
						if (!_tu
							|| _selUnit->spendTimeUnits(_selItem->getSlot()->getCost(slot)))
						{
							_tuCost = -1; // kL

							moveItem(
									_selItem,
									slot,
									item->getSlotX(),
									item->getSlotY());
							_stackLevel[item->getSlotX()][item->getSlotY()] += 1;
							setSelectedItem(NULL);
							_game->getResourcePack()->getSoundByDepth(
																	_depth,
																	ResourcePack::ITEM_DROP)
																->play();
						}
						else
						{
							_warning->showMessage(_game->getLanguage()->getString("STR_NOT_ENOUGH_TIME_UNITS"));
//							mouseOver(action, state); // kL, refresh tuCost visibility.
//							arrangeGround(false); // kL, refresh tuCost visibility.
						}
					}
				}
				else if (item->getRules()->getCompatibleAmmo()->empty() == false) // Put item in weapon
				{
					bool wrong = true;
					for (std::vector<std::string>::const_iterator
							i = item->getRules()->getCompatibleAmmo()->begin();
							i != item->getRules()->getCompatibleAmmo()->end();
							++i)
					{
						if ((*i) == _selItem->getRules()->getType())
						{
							wrong = false;

							break;
						}
					}

					if (wrong)
					{
						_warning->showMessage(_game->getLanguage()->getString("STR_WRONG_AMMUNITION_FOR_THIS_WEAPON"));
//						mouseOver(action, state); // kL, refresh tuCost visibility.
//						arrangeGround(false); // kL, refresh tuCost visibility.
					}
					else
					{
						if (item->getAmmoItem() != NULL)
						{
							_warning->showMessage(_game->getLanguage()->getString("STR_WEAPON_IS_ALREADY_LOADED"));
//							mouseOver(action, state); // kL, refresh tuCost visibility.
//							arrangeGround(false); // kL, refresh tuCost visibility.
						}
						else if (!_tu
							|| _selUnit->spendTimeUnits(15))
						{
							_tuCost = -1; // kL

							moveItem(
									_selItem,
									NULL,
									0,
									0);

							item->setAmmoItem(_selItem);
							_selItem->moveToOwner(NULL);
							setSelectedItem(NULL);

							_game->getResourcePack()->getSoundByDepth(
																	_depth,
																	ResourcePack::ITEM_RELOAD)
																->play();

							if (item->getSlot()->getType() == INV_GROUND)
								arrangeGround(false);
						}
						else
						{
							_warning->showMessage(_game->getLanguage()->getString("STR_NOT_ENOUGH_TIME_UNITS"));
//							mouseOver(action, state); // kL, refresh tuCost visibility.
//							arrangeGround(false); // kL, refresh tuCost visibility.
						}
					}
				}
				// else swap the item positions?
			}
			else
			{
				// try again, using the position of the mouse cursor, not the item (slightly more intuitive for stacking)
				x = static_cast<int>(floor(action->getAbsoluteXMouse())) - getX();
				y = static_cast<int>(floor(action->getAbsoluteYMouse())) - getY();

				slot = getSlotInPosition(&x, &y);
				if (slot != NULL
					&& slot->getType() == INV_GROUND)
				{
					x += _groundOffset;

					BattleItem* item = _selUnit->getItem(slot, x, y);
					if (canBeStacked(
									item,
									_selItem))
					{
						if (!_tu
							|| _selUnit->spendTimeUnits(_selItem->getSlot()->getCost(slot)))
						{
							moveItem(
									_selItem,
									slot,
									item->getSlotX(),
									item->getSlotY());
							_stackLevel[item->getSlotX()][item->getSlotY()] += 1;
							setSelectedItem(NULL);

							_game->getResourcePack()->getSoundByDepth(
																	_depth,
																	ResourcePack::ITEM_DROP)
																->play();
						}
						else
						{
							_warning->showMessage(_game->getLanguage()->getString("STR_NOT_ENOUGH_TIME_UNITS"));
//							mouseOver(action, state); // kL, refresh tuCost visibility.
//							arrangeGround(false); // kL, refresh tuCost visibility.
						}
					}
				}
			}
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		_tuCost = -1; // kL

		//Log(LOG_INFO) << "Inventory: SDL_BUTTON_RIGHT";
		if (_selItem == NULL)
		{
			//Log(LOG_INFO) << ". no selected item";
			if (!_base
				|| Options::includePrimeStateInSavedLayout)
			{
				//Log(LOG_INFO) << ". not at base";
				if (!_tu)	// kL_note: ie. TurnUnits have not been instantiated yet:
							// ergo preBattlescape inventory screen is active.
				{
					//Log(LOG_INFO) << ". preBattle screen";
					int
						x = static_cast<int>(floor(action->getAbsoluteXMouse())) - getX(),
						y = static_cast<int>(floor(action->getAbsoluteYMouse())) - getY();

					RuleInventory* slot = getSlotInPosition(&x, &y);
					if (slot != NULL)
					{
						if (slot->getType() == INV_GROUND)
							x += _groundOffset;

						BattleItem* item = _selUnit->getItem(slot, x, y);
						if (item != NULL)
						{
							BattleType itemType = item->getRules()->getBattleType();
							if (BT_GRENADE == itemType
								|| BT_PROXIMITYGRENADE == itemType)
							{
								if (item->getFuseTimer() == -1) // Prime that grenade!
								{
									if (BT_PROXIMITYGRENADE == itemType)
									{
										item->setFuseTimer(0);
										arrangeGround(false);

//										std::wstring activated = Text::formatNumber(0) + L" ";
//										activated += _game->getLanguage()->getString("STR_GRENADE_IS_ACTIVATED");
//										activated += L" " + Text::formatNumber(0);

										const std::wstring activated = _game->getLanguage()->getString("STR_GRENADE_IS_ACTIVATED");
										_warning->showMessage(activated);
									}
									else
										// kL_note: This is where activation warning for nonProxy preBattle grenades goes...
										_game->pushState(new PrimeGrenadeState(
																			NULL,
																			true,
																			item,
																			this)); // kL_add.
								}
								else
								{
									_warning->showMessage(_game->getLanguage()->getString("STR_GRENADE_IS_DEACTIVATED"));
									item->setFuseTimer(-1); // Unprime the grenade
//									drawItems(); // kL, de-vector the Fuse graphic.
									arrangeGround(false);
								}
							}
						}
					}
				}
				else
				{
					//Log(LOG_INFO) << ". in Battle";
					_game->popState(); // Closes the inventory window on right-click (if not in preBattle equip screen!)

/*kL: now done in InventoryState::dTor()
					// but Does NOT applyGravity(), so from InventoryState::btnOkClick()
					SavedBattleGame* battleGame = _game->getSavedGame()->getSavedBattle();
					TileEngine* tileEngine = battleGame->getTileEngine();

					tileEngine->applyGravity(battleGame->getSelectedUnit()->getTile());
					tileEngine->calculateTerrainLighting(); // dropping / picking up flares
					tileEngine->recalculateFOV(); */
/*
					// from BattlescapeGame::dropItem() but can't really use this because I don't know exactly what dropped...
					// could figure it out via what's on Ground but meh.
					if (item->getRules()->getBattleType() == BT_FLARE)
					{
						getTileEngine()->calculateTerrainLighting();
						getTileEngine()->calculateFOV(position);
					} */
				}
			}
			//else Log(LOG_INFO) << ". in CraftEquip";
		}
		else
		{
			//Log(LOG_INFO) << ". drop item to ground";
			if (_selItem->getSlot()->getType() == INV_GROUND)
				_stackLevel[_selItem->getSlotX()][_selItem->getSlotY()] += 1;

			setSelectedItem(NULL); // Return item to original position
		}
	}

	InteractiveSurface::mouseClick(action, state);

/*	int
		x,
		y;
	SDL_GetMouseState(&x, &y);

	SDL_WarpMouse(x + 1, y);	// send a mouse motion event to refresh any hover actions
	SDL_WarpMouse(x, y); */		// move the mouse back to avoid cursor creep
}

/**
 * Unloads the selected weapon, placing the gun
 * on the right hand and the ammo on the left hand.
 * @return, true if the weapon was successfully unloaded
 */
bool Inventory::unload()
{
	if (_selItem == NULL)
		return false;

	if (_selItem->getAmmoItem() == NULL
		&& _selItem->getRules()->getCompatibleAmmo()->empty() == false)
	{
		_warning->showMessage(_game->getLanguage()->getString("STR_NO_AMMUNITION_LOADED"));
	}

	if (_selItem->getAmmoItem() == NULL
		|| _selItem->needsAmmo() == false)
	{
		return false;
	}

	for (std::vector<BattleItem*>::const_iterator
			i = _selUnit->getInventory()->begin();
			i != _selUnit->getInventory()->end();
			++i)
	{
		if ((*i)->getSlot()->getType() == INV_HAND
			&& *i != _selItem)
		{
			_warning->showMessage(_game->getLanguage()->getString("STR_BOTH_HANDS_MUST_BE_EMPTY"));
			return false;
		}
	}

	if (_tu == false
		|| _selUnit->spendTimeUnits(8))
	{
		moveItem(
				_selItem->getAmmoItem(),
				_game->getRuleset()->getInventory("STR_LEFT_HAND"),
				0,
				0);
		_selItem->getAmmoItem()->moveToOwner(_selUnit);
		moveItem(
				_selItem,
				_game->getRuleset()->getInventory("STR_RIGHT_HAND"),
				0,
				0);
		_selItem->moveToOwner(_selUnit);
		_selItem->setAmmoItem(NULL);
		setSelectedItem(NULL);
	}
	else
	{
		_warning->showMessage(_game->getLanguage()->getString("STR_NOT_ENOUGH_TIME_UNITS"));
		return false;
	}

	return true;
}

/**
 * Arranges items on the ground for the inventory display.
 * Since items on the ground aren't assigned to anyone
 * they don't actually have permanent slot positions.
 * @param alterOffset - true to alter the ground offset
 */
void Inventory::arrangeGround(bool alterOffset)
{
	RuleInventory* ground = _game->getRuleset()->getInventory("STR_GROUND");

	bool fit = false;
	int
		slotsX = (320 - ground->getX()) / RuleInventory::SLOT_W,
		slotsY = (200 - ground->getY()) / RuleInventory::SLOT_H,
		x = 0,
		y = 0,
		xMax = 0;

	_stackLevel.clear();

	if (_selUnit != NULL) // kL_note: That should never happen.
	{
		// first move all items out of the way - a big number in X direction
		for (std::vector<BattleItem*>::const_iterator
				i = _selUnit->getTile()->getInventory()->begin();
				i != _selUnit->getTile()->getInventory()->end();
				++i)
		{
			(*i)->setSlot(ground);
			(*i)->setSlotX(1000000);
			(*i)->setSlotY(0);
		}

		// now for each item, find the most topleft position that is not occupied and will fit
		for (std::vector<BattleItem*>::const_iterator
				i = _selUnit->getTile()->getInventory()->begin();
				i != _selUnit->getTile()->getInventory()->end();
				++i)
		{
			x = 0;
			y = 0;

			fit = false;
			while (fit == false)
			{
				fit = true; // assume we can put the item here, if one of the following checks fails, we can't.
				for (int
						xd = 0;
						xd < (*i)->getRules()->getInventoryWidth()
							&& fit;
						++xd)
				{
					if ((x + xd) %slotsX < x %slotsX)
						fit = false;
					else
					{
						for (int
								yd = 0;
								yd < (*i)->getRules()->getInventoryHeight()
									&& fit;
								++yd)
						{
							BattleItem* item = _selUnit->getItem(
																ground,
																x + xd,
																y + yd);
							fit = (item == NULL);

							if (canBeStacked(item, *i))
								fit = true;
						}
					}
				}

				if (fit == true)
				{
					(*i)->setSlotX(x);
					(*i)->setSlotY(y);
					// only increase the stack level if the item is actually visible.
					if ((*i)->getRules()->getInventoryWidth())
						_stackLevel[x][y] += 1;

					xMax = std::max(
								xMax,
								x + (*i)->getRules()->getInventoryWidth());
				}
				else
				{
					y++;
					if (y > slotsY - (*i)->getRules()->getInventoryHeight())
					{
						y = 0;
						x++;
					}
				}
			}
		}
	}

	if (alterOffset == true)
	{
		int itemWidth = 0;
		if (_selItem != NULL)
			itemWidth = _selItem->getRules()->getInventoryWidth();

		if (xMax > _groundOffset + slotsX - itemWidth)
			_groundOffset += slotsX;
		else
			_groundOffset = 0;
	}

	drawItems();
}

/**
 * Attempts to place the item in the inventory slot.
 * @param newSlot	- pointer to where to place the item
 * @param item		- item to be placed
 * @param warning	- warning message if item could not be placed
 * @param test		- true if only doing a test (no move happens)
 * @return, true if the item was successfully placed in the inventory
 */
bool Inventory::fitItem(
		RuleInventory* newSlot,
		BattleItem* item,
		std::string& warning,
		bool test) // kL_add
{
	bool placed = false;

	for (int
			y2 = 0;
			y2 <= newSlot->getY() / RuleInventory::SLOT_H
				&& !placed;
			++y2)
	{
		for (int
				x2 = 0;
				x2 <= newSlot->getX() / RuleInventory::SLOT_W
					&& !placed;
				++x2)
		{
			if (newSlot->fitItemInSlot(
									item->getRules(),
									x2,
									y2))
			{
				if (_tu
					&& test == true)
				{
					placed = true;
				}
				else if (overlapItems(
									_selUnit,
									item,
									newSlot,
									x2,
									y2) == false)
				{
					if (_tu == false
						|| _selUnit->spendTimeUnits(item->getSlot()->getCost(newSlot)))
					{
						placed = true;

						moveItem(
								item,
								newSlot,
								x2,
								y2);

						_game->getResourcePack()->getSoundByDepth(
																_depth,
																ResourcePack::ITEM_DROP)
															->play();
						drawItems();
					}
					else if (test == false)
						_warning->showMessage(_game->getLanguage()->getString("STR_NOT_ENOUGH_TIME_UNITS"));
				}
			}
		}
	}

	return placed;
}

/**
 * Checks if two items can be stacked on one another.
 * @param itemA - first item
 * @param itemB - second item
 * @return, true if the items can be stacked on one another
 */
bool Inventory::canBeStacked(
		BattleItem* itemA,
		BattleItem* itemB)
{
	return (itemA != NULL && itemB != NULL																// both items actually exist
		&& itemA->getRules() == itemB->getRules()														// both items have the same ruleset
		&& ((!itemA->getAmmoItem() && !itemB->getAmmoItem())											// either they both have no ammo
			|| (itemA->getAmmoItem() && itemB->getAmmoItem()											// or they both have ammo
				&& itemA->getAmmoItem()->getRules() == itemB->getAmmoItem()->getRules()					// and the same ammo type
				&& itemA->getAmmoItem()->getAmmoQuantity() == itemB->getAmmoItem()->getAmmoQuantity()))	// and the same ammo quantity
		&& itemA->getFuseTimer() == -1 && itemB->getFuseTimer() == -1									// and neither is set to explode
		&& itemA->getUnit() == NULL && itemB->getUnit() == NULL											// and neither is a corpse or unconscious unit
		&& itemA->getPainKillerQuantity() == itemB->getPainKillerQuantity()								// and if it's a medkit, it has the same number of charges
		&& itemA->getHealQuantity() == itemB->getHealQuantity()
		&& itemA->getStimulantQuantity() == itemB->getStimulantQuantity());
}

/**
 * Shows a warning message.
 * @param msg The message to show.
 */
void Inventory::showWarning(const std::wstring& msg)
{
	_warning->showMessage(msg);
}

/**
 * Shows primer warnings on all live grenades.
 */
void Inventory::drawPrimers()
{
	const int pulse[22] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3};

	if (_fuseFrame == 22)
		_fuseFrame = 0;

	Surface* srf = _game->getResourcePack()->getSurfaceSet("SCANG.DAT")->getFrame(9);
	for (std::vector<std::pair<int, int> >::const_iterator
			i = _grenadeFuses.begin();
			i != _grenadeFuses.end();
			++i)
	{
		srf->blitNShade(
					_items,
					(*i).first,
					(*i).second,
					pulse[_fuseFrame],
					false,
					3); // red
	}

	_fuseFrame++;
}

/**
 * kL. Sets grenade to show a warning in Inventory.
 * @param turn - turns until grenade is to explode
 */
void Inventory::setPrimeGrenade(int turn) // kL
{
	_primeGrenade = turn;
}

/**
 * kL. Gets the TU cost for moving items around.
 * @return, time unit cost
 */
int Inventory::getTUCost() const // kL
{
	return _tuCost;
}

}
