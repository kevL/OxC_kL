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

#include "Inventory.h"

//#include <cmath>

#include "PrimeGrenadeState.h"
#include "TileEngine.h"
#include "WarningMessage.h"

#include "../Engine/Action.h"
//#include "../Engine/Font.h"
#include "../Engine/Game.h"
#include "../Engine/Language.h"
//#include "../Engine/Options.h"
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

#include "../Ufopaedia/Ufopaedia.h"


namespace OpenXcom
{

/**
 * Sets up an inventory with the specified size and position.
 * @param game		- pointer to core Game
 * @param width		- width in pixels
 * @param height	- height in pixels
 * @param x			- X position in pixels (default 0)
 * @param y			- Y position in pixels (default 0)
 * @param atBase	- true if inventory is accessed from Basescape (default false)
 */
Inventory::Inventory(
		Game* game,
		int width,
		int height,
		int x,
		int y,
		bool atBase)
	:
		InteractiveSurface(
			width,
			height,
			x,y),
		_game(game),
		_selUnit(NULL),
		_selItem(NULL),
		_mouseOverItem(NULL),
		_tuMode(true),
		_atBase(atBase),
		_groundOffset(0),
		_fuseFrame(0),
		_prime(-1),
		_tuCost(-1)
{
	_grid			= new Surface(width, height, x,y);
	_items			= new Surface(width, height, x,y);
	_selection		= new Surface(
								RuleInventory::HAND_W * RuleInventory::SLOT_W,
								RuleInventory::HAND_H * RuleInventory::SLOT_H,
								x,y);
	_warning		= new WarningMessage(224, 24, 48, 176);
	_stackNumber	= new NumberText(15, 15);
	_animTimer		= new Timer(80);

	_warning->initText(
					_game->getResourcePack()->getFont("FONT_BIG"),
					_game->getResourcePack()->getFont("FONT_SMALL"),
					_game->getLanguage());
	_warning->setTextColor(static_cast<Uint8>(_game->getRuleset()->getInterface("battlescape")->getElement("warning")->color));
	_warning->setColor(static_cast<Uint8>(_game->getRuleset()->getInterface("battlescape")->getElement("warning")->color2));

	_stackNumber->setBordered();

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
 * @param firstcolor	- offset of the first color to replace (default 0)
 * @param ncolors		- amount of colors to replace (default 256)
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
	_tuMode = tu;
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

	Text text = Text(16,9);
	text.setPalette(_grid->getPalette());
	text.setHighContrast();
	text.initText(
				_game->getResourcePack()->getFont("FONT_BIG"),
				_game->getResourcePack()->getFont("FONT_SMALL"),
				_game->getLanguage());

	const RuleInterface* const rule = _game->getRuleset()->getInterface("inventory");
	text.setColor(static_cast<Uint8>(rule->getElement("textSlots")->color));

	const Uint8 color = static_cast<Uint8>(rule->getElement("grid")->color);
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
				SDL_Rect rect;

				rect.x = static_cast<Sint16>(i->second->getX() + RuleInventory::SLOT_W * j->x);
				rect.y = static_cast<Sint16>(i->second->getY() + RuleInventory::SLOT_H * j->y);
				rect.w = static_cast<Uint16>(RuleInventory::SLOT_W + 1);
				rect.h = static_cast<Uint16>(RuleInventory::SLOT_H + 1);
				_grid->drawRect(&rect, color);

				++rect.x;
				++rect.y;
				rect.w -= 2;
				rect.h -= 2;
				_grid->drawRect(&rect, 0);
			}
		}
		else if (i->second->getType() == INV_HAND) // draw grid
		{
			SDL_Rect rect;

			rect.x = static_cast<Sint16>(i->second->getX());
			rect.y = static_cast<Sint16>(i->second->getY());
			rect.w = static_cast<Uint16>(RuleInventory::HAND_W * RuleInventory::SLOT_W);
			rect.h = static_cast<Uint16>(RuleInventory::HAND_H * RuleInventory::SLOT_H);
			_grid->drawRect(&rect, color);

			++rect.x;
			++rect.y;
			rect.w -= 2;
			rect.h -= 2;
			_grid->drawRect(&rect, 0);
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
					SDL_Rect rect;

					rect.x = static_cast<Sint16>(x);
					rect.y = static_cast<Sint16>(y);
					rect.w = static_cast<Uint16>(RuleInventory::SLOT_W + 1);
					rect.h = static_cast<Uint16>(RuleInventory::SLOT_H + 1);
					_grid->drawRect(&rect, color);

					++rect.x;
					++rect.y;
					rect.w -= 2;
					rect.h -= 2;
					_grid->drawRect(&rect, 0);
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
 * Draws the items contained in the unit's inventory.
 */
void Inventory::drawItems()
{
	_items->clear();
	_grenadeFuses.clear();

	if (_selUnit != NULL)
	{
		const Uint8 color = static_cast<Uint8>(_game->getRuleset()->getInterface("inventory")->getElement("numStack")->color);

		SurfaceSet* const srt = _game->getResourcePack()->getSurfaceSet("BIGOBS.PCK");
		Surface* srf;

		for (std::vector<BattleItem*>::const_iterator // Soldier items
				i = _selUnit->getInventory()->begin();
				i != _selUnit->getInventory()->end();
				++i)
		{
			if (*i != _selItem)
			{
				srf = srt->getFrame((*i)->getRules()->getBigSprite());
				if (srf != NULL) // safety.
				{
					if ((*i)->getSlot()->getType() == INV_SLOT)
					{
						srf->setX((*i)->getSlot()->getX() + (*i)->getSlotX() * RuleInventory::SLOT_W);
						srf->setY((*i)->getSlot()->getY() + (*i)->getSlotY() * RuleInventory::SLOT_H);
					}
					else if ((*i)->getSlot()->getType() == INV_HAND)
					{
						srf->setX(
								(*i)->getSlot()->getX()
								+ (RuleInventory::HAND_W - (*i)->getRules()->getInventoryWidth())
									* RuleInventory::SLOT_W / 2);
						srf->setY(
								(*i)->getSlot()->getY()
								+ (RuleInventory::HAND_H - (*i)->getRules()->getInventoryHeight())
									* RuleInventory::SLOT_H / 2);
					}

//					srt->getFrame((*i)->getRules()->getBigSprite())->blit(_items);
					srf->blit(_items);

					if ((*i)->getFuse() > -1) // grenade primer indicators
						_grenadeFuses.push_back(std::make_pair(
															srf->getX(),
															srf->getY()));
				}
				//else Log(LOG_INFO) << "ERROR: bigob not found #" << (*i)->getRules()->getBigSprite();
			}
		}

		Surface* const stackLayer = new Surface(
											getWidth(),
											getHeight(),
											0,0);
		stackLayer->setPalette(getPalette());

		for (std::vector<BattleItem*>::const_iterator // Ground items
				i = _selUnit->getTile()->getInventory()->begin();
				i != _selUnit->getTile()->getInventory()->end();
				++i)
		{
			// Note that items can be made invisible by setting their
			// width or height to 0 - eg. used with tank corpse items.
			if (*i != _selItem
				&& (*i)->getSlotX() >= _groundOffset
				&& (*i)->getRules()->getInventoryHeight() != 0
				&& (*i)->getRules()->getInventoryWidth() != 0)
			{
				srf = srt->getFrame((*i)->getRules()->getBigSprite());
				if (srf != NULL) // safety.
				{
					srf->setX(
							(*i)->getSlot()->getX()
							+ ((*i)->getSlotX() - _groundOffset) * RuleInventory::SLOT_W);
					srf->setY(
							(*i)->getSlot()->getY()
							+ ((*i)->getSlotY() * RuleInventory::SLOT_H));

//					srt->getFrame((*i)->getRules()->getBigSprite())->blit(_items);
					srf->blit(_items);

					if ((*i)->getFuse() > -1) // grenade primer indicators
						_grenadeFuses.push_back(std::make_pair(
															srf->getX(),
															srf->getY()));
				}
				//else Log(LOG_INFO) << "ERROR : bigob not found #" << (*i)->getRules()->getBigSprite();

				if (_stackLevel[(*i)->getSlotX()]
							   [(*i)->getSlotY()] > 1) // item stacking
				{
					_stackNumber->setX(
									((*i)->getSlot()->getX()
										+ (((*i)->getSlotX() + (*i)->getRules()->getInventoryWidth()) - _groundOffset)
											* RuleInventory::SLOT_W)
									- 4);

					if (_stackLevel[(*i)->getSlotX()]
								   [(*i)->getSlotY()] > 9)
						_stackNumber->setX(_stackNumber->getX() - 4);

					_stackNumber->setY(
									((*i)->getSlot()->getY()
										+ ((*i)->getSlotY() + (*i)->getRules()->getInventoryHeight())
											* RuleInventory::SLOT_H)
									- 6);
					_stackNumber->setValue(_stackLevel[(*i)->getSlotX()]
													  [(*i)->getSlotY()]);
					_stackNumber->draw();
					_stackNumber->setColor(color); // Palette::blockOffset(4)+2
					_stackNumber->blit(stackLayer);
				}
			}
		}

		stackLayer->blit(_items);
		delete stackLayer;
	}
}

/**
 * Moves an item to a specified slot in the selected unit's inventory.
 * @param item	- pointer to a BattleItem
 * @param slot	- pointer to RuleInventory slot or NULL if none
 * @param x		- X position in slot (default 0)
 * @param y		- Y position in slot (default 0)
 */
void Inventory::moveItem( // private.
		BattleItem* const item,
		RuleInventory* const slot,
		int x,
		int y)
{
	if (slot == NULL) // Make items vanish (eg. ammo in weapons)
	{
		if (item->getSlot()->getType() == INV_GROUND)
			_selUnit->getTile()->removeItem(item);
		else
			item->moveToOwner(NULL);

		item->setSlot(NULL);
	}
	else
	{
		if (slot != item->getSlot()) // Handle dropping from/to ground.
		{
			if (slot->getType() == INV_GROUND) // set to Ground
			{
				item->moveToOwner(NULL);
				_selUnit->getTile()->addItem(
											item,
											item->getSlot());

				if (item->getUnit() != NULL
					&& item->getUnit()->getUnitStatus() == STATUS_UNCONSCIOUS)
				{
					item->getUnit()->setPosition(_selUnit->getPosition());
				}
			}
			else if (item->getSlot() == NULL					// unload a weapon clip to left hand
				|| item->getSlot()->getType() == INV_GROUND)	// or pick up item.
			{
				if (_tuMode == true									// To prevents units from picking up large objects and running around with
					&& item->getSlot()->getType() == INV_GROUND)	// nearly full TU on the same turn its weight becomes an extra tu-burden
				{
					_selUnit->setTimeUnits(std::max(
												0,
												_selUnit->getTimeUnits() - item->getRules()->getWeight()));
				}

				item->moveToOwner(_selUnit);
				_selUnit->getTile()->removeItem(item);

				if (item->getUnit() != NULL
					&& item->getUnit()->getUnitStatus() == STATUS_UNCONSCIOUS)
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
 * @param x		- X position in slot (default 0)
 * @param y		- Y position in slot (default 0)
 * @return, true if there's overlap
 */
bool Inventory::overlapItems( // static.
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
				&& (*i)->occupiesSlot(x,y, item))
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
			if ((*i)->occupiesSlot(x,y, item))
				return true;
		}
	}

	return false;
}

/**
 * Gets the inventory slot located in the specified mouse position.
 * @param x - pointer to mouse X position; returns the slot's X position
 * @param y - pointer to mouse Y position; returns the slot's Y position
 * @return, pointer to slot rules or NULL if none
 */
RuleInventory* Inventory::getSlotInPosition( // private.
		int* x,
		int* y) const
{
	for (std::map<std::string, RuleInventory*>::const_iterator
			i = _game->getRuleset()->getInventories()->begin();
			i != _game->getRuleset()->getInventories()->end();
			++i)
	{
		if (i->second->checkSlotInPosition(x,y))
			return i->second;
	}

	return NULL;
}

/**
 * Returns the item currently grabbed by the player.
 * @return, pointer to selected BattleItem or NULL if none
 */
BattleItem* Inventory::getSelectedItem() const
{
	return _selItem;
}

/**
 * Changes the item currently grabbed by the player.
 * @param item - pointer to selected BattleItem or NULL if none
 */
void Inventory::setSelectedItem(BattleItem* const item)
{
	if (item != NULL
		&& item->getRules()->isFixed() == false)
	{
		_selItem = item;

		if (_selItem->getSlot()->getType() == INV_GROUND)
			_stackLevel[static_cast<size_t>(_selItem->getSlotX())]
					   [static_cast<size_t>(_selItem->getSlotY())] -= 1;

		_selItem->getRules()->drawHandSprite(
										_game->getResourcePack()->getSurfaceSet("BIGOBS.PCK"),
										_selection);
	}
	else
	{
		_selItem = NULL;
		_selection->clear();
	}

	drawItems();
}

/**
 * Returns the item currently under mouse cursor.
 * @return, pointer to a BattleItem or NULL if none
 */
BattleItem* Inventory::getMouseOverItem() const
{
	return _mouseOverItem;
}

/**
 * Changes the item currently under mouse cursor.
 * @param item - pointer to a BattleItem or NULL if none
 */
void Inventory::setMouseOverItem(BattleItem* const item)
{
	if (item != NULL
		&& item->getRules()->isFixed() == false)
	{
		_mouseOverItem = item;
	}
	else
		_mouseOverItem = NULL;
}

/**
 * Handles WarningMessage timer.
 */
void Inventory::think()
{
	if (_prime > -1)
	{
		std::wstring activated;

		if (_prime > 0)
			activated = Text::formatNumber(_prime) + L" ";

		activated += _game->getLanguage()->getString("STR_GRENADE_IS_ACTIVATED");

		if (_prime > 0)
			activated += L" " + Text::formatNumber(_prime);

		_warning->showMessage(activated);

		_prime = -1;
	}

/*	int x,y;
	SDL_GetMouseState(&x, &y);
	SDL_WarpMouse(x + 1, y);	// send a mouse motion event to refresh any hover actions
	SDL_WarpMouse(x, y);		// move the mouse back to avoid cursor creep */

	_warning->think();
	_animTimer->think(NULL, this);
}

/**
 * Blits the inventory elements.
 * @param surface - pointer to Surface to blit onto
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
 * Handles the cursor over.
 * @param action	- pointer to an Action
 * @param state		- State that the action handlers belong to
 */
void Inventory::mouseOver(Action* action, State* state)
{
	_selection->setX(static_cast<int>(std::floor(
					action->getAbsoluteXMouse())) - (_selection->getWidth() / 2) - getX());
	_selection->setY(static_cast<int>(std::floor(
					action->getAbsoluteYMouse())) - (_selection->getHeight() / 2) - getY());

	if (_selUnit == NULL)
		return;

	int
		x = static_cast<int>(std::floor(action->getAbsoluteXMouse())) - getX(),
		y = static_cast<int>(std::floor(action->getAbsoluteYMouse())) - getY();

	RuleInventory* const slot = getSlotInPosition(&x,&y);
	if (slot != NULL)
	{
		if (_tuMode == true
			&& _selItem != NULL
			&& fitItem(
					slot,
					_selItem,
					true) == true)
		{
			_tuCost = _selItem->getSlot()->getCost(slot);
		}
		else
		{
			_tuCost = -1;

			if (slot->getType() == INV_GROUND)
				x += _groundOffset;

			BattleItem* const item = _selUnit->getItem(slot, x,y);
			setMouseOverItem(item);
		}
	}
	else
	{
		_tuCost = -1;
		setMouseOverItem(NULL);
	}

	_selection->setX(static_cast<int>(std::floor(
					action->getAbsoluteXMouse())) - (_selection->getWidth() / 2) - getX());
	_selection->setY(static_cast<int>(std::floor(
					action->getAbsoluteYMouse())) - (_selection->getHeight() / 2) - getY());

	InteractiveSurface::mouseOver(action, state);
}

/**
 * Handles the cursor click. Picks up / drops an item.
 * @param action	- pointer to an Action
 * @param state		- State that the action handlers belong to
 */
void Inventory::mouseClick(Action* action, State* state)
{
	int sound = -1;

	if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
	{
		if (_selUnit == NULL)
			return;

		if (_selItem == NULL) // Pickup or Move item.
		{
			int
				x = static_cast<int>(std::floor(action->getAbsoluteXMouse())) - getX(),
				y = static_cast<int>(std::floor(action->getAbsoluteYMouse())) - getY();

			RuleInventory* const slot = getSlotInPosition(&x,&y);
			if (slot != NULL)
			{
				if (slot->getType() == INV_GROUND)
					x += _groundOffset;

				BattleItem* const item = _selUnit->getItem(
														slot,
														x,y);
				if (item != NULL)
				{
					if ((SDL_GetModState() & KMOD_CTRL) != 0) // Move item.
					{
						bool
							placed = false,
							toGround = true;

						RuleInventory* toSlot = NULL;

						if (slot->getType() == INV_HAND
							|| (slot->getType() != INV_GROUND
								&& (_tuMode == false
									|| _selUnit->getOriginalFaction() != FACTION_PLAYER))) // aLien units drop-to-ground on Ctrl+LMB
						{
							toSlot = _game->getRuleset()->getInventory("STR_GROUND");
						}
						else
						{
							if (_selUnit->getItem("STR_RIGHT_HAND") == NULL)
							{
								toGround = false;
								toSlot = _game->getRuleset()->getInventory("STR_RIGHT_HAND");
							}
							else if (_selUnit->getItem("STR_LEFT_HAND") == NULL)
							{
								toGround = false;
								toSlot = _game->getRuleset()->getInventory("STR_LEFT_HAND");
							}
							else if (slot->getType() != INV_GROUND)
								toSlot = _game->getRuleset()->getInventory("STR_GROUND");
						}

						if (toSlot != NULL)
						{
							if (toGround == true)
							{
								if (_tuMode == false
									|| _selUnit->spendTimeUnits(item->getSlot()->getCost(toSlot)) == true)
								{
									placed = true;
									moveItem(
											item,
											toSlot);
									arrangeGround(false);

									sound = ResourcePack::ITEM_DROP;
								}
								else
									_warning->showMessage(_game->getLanguage()->getString("STR_NOT_ENOUGH_TIME_UNITS"));
							}
							else
							{
								_stackLevel[static_cast<size_t>(item->getSlotX())]
										   [static_cast<size_t>(item->getSlotY())] -= 1;

								if (fitItem(
										toSlot,
										item) == true)
								{
									placed = true;
								}
								else
									_stackLevel[static_cast<size_t>(item->getSlotX())]
											   [static_cast<size_t>(item->getSlotY())] += 1;
							}

							if (placed == true)
							{
								_mouseOverItem = NULL; // remove cursor info 'cause item is no longer under the cursor
								mouseOver(action, state);
							}
						}
					}
					else // Pickup item.
					{
						setSelectedItem(item);

						const int explTurn = item->getFuse();
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
		else // Drop item or Load weapon.
		{
			int
				x = _selection->getX()
						+ (RuleInventory::HAND_W - _selItem->getRules()->getInventoryWidth())
							* RuleInventory::SLOT_W / 2
						+ RuleInventory::SLOT_W / 2,
				y = _selection->getY()
						+ (RuleInventory::HAND_H - _selItem->getRules()->getInventoryHeight())
							* RuleInventory::SLOT_H / 2
						+ RuleInventory::SLOT_H / 2;

			RuleInventory* slot = getSlotInPosition(&x,&y);

			if (slot != NULL)
			{
				if (slot->getType() == INV_GROUND)
					x += _groundOffset;

				BattleItem* const item = _selUnit->getItem(
														slot,
														x,y);

				const bool canStack = slot->getType() == INV_GROUND
								   && canBeStacked(
												item,
												_selItem) == true;

				if (item == NULL // Put item in empty slot or stack it if possible.
					|| item == _selItem
					|| canStack == true)
				{
					if (overlapItems(
								_selUnit,
								_selItem,
								slot,
								x,y) == false
						&& slot->fitItemInSlot(
											_selItem->getRules(),
											x,y) == true)
					{
						if (_tuMode == false
							|| _selUnit->spendTimeUnits(_selItem->getSlot()->getCost(slot)) == true)
						{
							_tuCost = -1;

							moveItem(
									_selItem,
									slot,
									x,y);

							if (slot->getType() == INV_GROUND)
								_stackLevel[static_cast<size_t>(x)]
										   [static_cast<size_t>(y)] += 1;
							setSelectedItem(NULL);

							sound = ResourcePack::ITEM_DROP;
						}
						else
							_warning->showMessage(_game->getLanguage()->getString("STR_NOT_ENOUGH_TIME_UNITS"));
					}
					else if (canStack == true)
					{
						if (_tuMode == false
							|| _selUnit->spendTimeUnits(_selItem->getSlot()->getCost(slot)) == true)
						{
							_tuCost = -1;

							moveItem(
									_selItem,
									slot,
									item->getSlotX(),
									item->getSlotY());
							_stackLevel[static_cast<size_t>(item->getSlotX())]
									   [static_cast<size_t>(item->getSlotY())] += 1;
							setSelectedItem(NULL);

							sound = ResourcePack::ITEM_DROP;
						}
						else
							_warning->showMessage(_game->getLanguage()->getString("STR_NOT_ENOUGH_TIME_UNITS"));
					}
				}
				else if (item->getRules()->getCompatibleAmmo()->empty() == false) // Put item in weapon.
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

					if (wrong == true)
						_warning->showMessage(_game->getLanguage()->getString("STR_WRONG_AMMUNITION_FOR_THIS_WEAPON"));
					else
					{
						if (item->getAmmoItem() != NULL)
							_warning->showMessage(_game->getLanguage()->getString("STR_WEAPON_IS_ALREADY_LOADED"));
						else if (_tuMode == false
							|| _selUnit->spendTimeUnits(item->getRules()->getTUReload()) == true)
						{
							_tuCost = -1;

							moveItem(
								_selItem,
								NULL);

							item->setAmmoItem(_selItem);
							_selItem->moveToOwner(NULL);
							setSelectedItem(NULL);

							sound = ResourcePack::ITEM_RELOAD;

							if (item->getSlot()->getType() == INV_GROUND)
								arrangeGround(false);
						}
						else
							_warning->showMessage(_game->getLanguage()->getString("STR_NOT_ENOUGH_TIME_UNITS"));
					}
				}
				// else swap the item positions ...
			}
			else
			{
				// try again using the position of the mouse cursor not the item (slightly more intuitive for stacking)
				x = static_cast<int>(std::floor(action->getAbsoluteXMouse())) - getX();
				y = static_cast<int>(std::floor(action->getAbsoluteYMouse())) - getY();

				slot = getSlotInPosition(&x,&y);
				if (slot != NULL
					&& slot->getType() == INV_GROUND)
				{
					x += _groundOffset;

					BattleItem* const item = _selUnit->getItem(slot, x,y);
					if (canBeStacked(
									item,
									_selItem) == true)
					{
						if (_tuMode == false
							|| _selUnit->spendTimeUnits(_selItem->getSlot()->getCost(slot)) == true)
						{
							moveItem(
									_selItem,
									slot,
									item->getSlotX(),
									item->getSlotY());
							_stackLevel[static_cast<size_t>(item->getSlotX())]
									   [static_cast<size_t>(item->getSlotY())] += 1;
							setSelectedItem(NULL);

							sound = ResourcePack::ITEM_DROP;
						}
						else
							_warning->showMessage(_game->getLanguage()->getString("STR_NOT_ENOUGH_TIME_UNITS"));
					}
				}
			}
		}
	}
	else if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		_tuCost = -1;

		if (_selItem == NULL)
		{
			if ((SDL_GetModState() & KMOD_CTRL) == 0)
			{
				if (Options::includePrimeStateInSavedLayout == true
					|| _atBase == false) // Priming is allowed only on the field or in preBattle, or if fuse-state can save to Layouts.
				{
					if (_tuMode == false)
					{
						int
							x = static_cast<int>(std::floor(action->getAbsoluteXMouse())) - getX(),
							y = static_cast<int>(std::floor(action->getAbsoluteYMouse())) - getY();

						const RuleInventory* const slot = getSlotInPosition(&x,&y);
						if (slot != NULL)
						{
							if (slot->getType() == INV_GROUND)
								x += _groundOffset;

							BattleItem* const item = _selUnit->getItem(
																	slot,
																	x,y);
							if (item != NULL)
							{
								const RuleItem* const itRule = item->getRules();
								if (itRule->isGrenade() == true)
								{
									if (item->getFuse() == -1) // Prime that grenade!
									{
										if (itRule->getBattleType() == BT_PROXYGRENADE)
										{
											item->setFuse(0);
											arrangeGround(false);

											const std::wstring activated = _game->getLanguage()->getString("STR_GRENADE_IS_ACTIVATED");
											_warning->showMessage(activated);
										}
										else // This is where activation warning for nonProxy preBattle grenades goes.
											_game->pushState(new PrimeGrenadeState(
																				NULL,
																				true,
																				item,
																				this));
									}
									else // deFuse grenade
									{
										_warning->showMessage(_game->getLanguage()->getString("STR_GRENADE_IS_DEACTIVATED"));
										item->setFuse(-1);
										arrangeGround(false);
									}
								}
								else if (slot->getType() != INV_GROUND) // move item to Ground
								{
									moveItem(
											item,
											_game->getRuleset()->getInventory("STR_GROUND"));

									arrangeGround(false);
									sound = ResourcePack::ITEM_DROP;

									_mouseOverItem = NULL; // remove cursor info 'cause item is no longer under the cursor
									mouseOver(action, state);
								}
							}
						}
					}
					else
						_game->popState(); // Close the inventory window on right-click if not in preBattle equip screen!
				}
			}
			else // Open Ufopaedia article.
			{
				int
					x = static_cast<int>(std::floor(action->getAbsoluteXMouse())) - getX(),
					y = static_cast<int>(std::floor(action->getAbsoluteYMouse())) - getY();

				RuleInventory* const slot = getSlotInPosition(&x,&y);
				if (slot != NULL)
				{
					if (slot->getType() == INV_GROUND)
						x += _groundOffset;

					BattleItem* const item = _selUnit->getItem(
															slot,
															x,y);
					if (item != NULL)
					{
						std::string article = item->getRules()->getType(); // strip const. yay,
						Ufopaedia::openArticle(
											_game,
											article);
					}
				}
			}
		}
		else // RMB w/ item on cursor
		{
			if (_selItem->getSlot()->getType() == INV_GROUND)
				_stackLevel[static_cast<size_t>(_selItem->getSlotX())]
						   [static_cast<size_t>(_selItem->getSlotY())] += 1;

			setSelectedItem(NULL); // Return item to original position.
			sound = ResourcePack::ITEM_DROP;
		}
	}

	if (sound != -1)
		_game->getResourcePack()->getSound(
										"BATTLE.CAT",
										sound)
									->play();

	InteractiveSurface::mouseClick(action, state);

/*	int x,y;
	SDL_GetMouseState(&x,&y);
	SDL_WarpMouse(x + 1,y);	// send a mouse motion event to refresh any hover actions
	SDL_WarpMouse(x,y);		// move the mouse back to avoid cursor creep */
}

/**
 * Unloads the selected weapon placing the gun on the right hand and the ammo
 * on the left hand - unless tuMode is false then drop its ammo to the ground.
 * @return, true if a weapon is successfully unloaded
 */
bool Inventory::unload()
{
	if (_selItem == NULL
		|| _selItem->selfPowered() == true
		|| _game->getSavedGame()->isResearched(_selItem->getRules()->getRequirements()) == false)
	{
		return false;
	}

	BattleItem* const ammo = _selItem->getAmmoItem();
	if (ammo == NULL)
	{
		if (_selItem->getRules()->getCompatibleAmmo()->empty() == false)
			_warning->showMessage(_game->getLanguage()->getString("STR_NO_AMMUNITION_LOADED"));

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


	if (_tuMode == false
		|| _selUnit->spendTimeUnits(_selItem->getRules()->getTUUnload()) == true)
	{
		RuleInventory* slotRule;
		BattleUnit* toOwner;

		if (_tuMode == false)
		{
			slotRule = _game->getRuleset()->getInventory("STR_GROUND");
			toOwner = NULL;
		}
		else
		{
			slotRule = _game->getRuleset()->getInventory("STR_LEFT_HAND");
			toOwner = _selUnit;;
		}

		moveItem(
				_selItem,
				_game->getRuleset()->getInventory("STR_RIGHT_HAND"));
		_selItem->moveToOwner(_selUnit);

		_selItem->setAmmoItem(NULL);
		setSelectedItem(NULL);

		moveItem(
				ammo,
				slotRule);
		ammo->moveToOwner(toOwner);

		if (toOwner == NULL)
			arrangeGround(false);
		else
			drawItems();
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
 * @note Since items on the ground aren't assigned to anyone they don't actually
 * have permanent slot positions.
 * @param alterOffset - true to alter the ground offset (default true)
 */
void Inventory::arrangeGround(bool alterOffset)
{
	RuleInventory* const ground = _game->getRuleset()->getInventory("STR_GROUND");

	bool fit;
	const int
		slotsX = (320 - ground->getX()) / RuleInventory::SLOT_W,
		slotsY = (200 - ground->getY()) / RuleInventory::SLOT_H;
	int
		x,y,
		xMax = 0;

	_stackLevel.clear();

	if (_selUnit != NULL) // kL_note: That should never happen.
	{
		// first move all items out of the way -> a big number in X direction to right
		for (std::vector<BattleItem*>::const_iterator
				i = _selUnit->getTile()->getInventory()->begin();
				i != _selUnit->getTile()->getInventory()->end();
				++i)
		{
			(*i)->setSlot(ground);
			(*i)->setSlotX(1000000);
			(*i)->setSlotY(0);
		}

		// for each item find the most topleft position that is not occupied and will fit
		for (std::vector<BattleItem*>::const_iterator
				i = _selUnit->getTile()->getInventory()->begin();
				i != _selUnit->getTile()->getInventory()->end();
				++i)
		{
			x =
			y = 0;

			fit = false;
			while (fit == false)
			{
				fit = true; // assume the item can be put here, if one of the following checks fails it can't
				for (int
						xd = 0;
						xd < (*i)->getRules()->getInventoryWidth()
							&& fit;
						++xd)
				{
					if ((x + xd) % slotsX < x % slotsX)
						fit = false;
					else
					{
						for (int
								yd = 0;
								yd < (*i)->getRules()->getInventoryHeight()
									&& fit;
								++yd)
						{
							BattleItem* const item = _selUnit->getItem(
																	ground,
																	x + xd,
																	y + yd);
							fit = (item == NULL);

							if (canBeStacked(item, *i) == true)
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
					++y;
					if (y > slotsY - (*i)->getRules()->getInventoryHeight())
					{
						y = 0;
						++x;
					}
				}
			}
		}
	}

	if (alterOffset == true)
	{
		int itemWidth;
		if (_selItem != NULL)
			itemWidth = _selItem->getRules()->getInventoryWidth();
		else
			itemWidth = 0;

		if (xMax > _groundOffset + slotsX - itemWidth)
			_groundOffset += slotsX;
		else
			_groundOffset = 0;
	}

	drawItems();
}

/**
 * Attempts to place the item in the inventory slot.
 * @param slot	- pointer to where to place the item
 * @param item	- pointer to item to be placed
 * @param test	- true if only doing a test (no move happens) (default false)
 * @return, true if the item was successfully placed in the inventory
 */
bool Inventory::fitItem(
		RuleInventory* slot,
		BattleItem* item,
		bool test)
{
	bool placed = false;

	for (int
			y2 = 0;
			y2 <= slot->getY() / RuleInventory::SLOT_H
				&& placed == false;
			++y2)
	{
		for (int
				x2 = 0;
				x2 <= slot->getX() / RuleInventory::SLOT_W
					&& placed == false;
				++x2)
		{
			if (slot->fitItemInSlot(
								item->getRules(),
								x2,y2))
			{
				if (_tuMode == true
					&& test == true)
				{
					placed = true;
				}
				else if (overlapItems(
								_selUnit,
								item,
								slot,
								x2,y2) == false)
				{
					if (_tuMode == false
						|| _selUnit->spendTimeUnits(item->getSlot()->getCost(slot)) == true)
					{
						placed = true;

						moveItem(
							item,
							slot,
							x2,y2);

						_game->getResourcePack()->getSound(
														"BATTLE.CAT",
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
		const BattleItem* const itemA,
		const BattleItem* const itemB)
{
	return (itemA != NULL && itemB != NULL																// both items actually exist
		&& itemA->getRules() == itemB->getRules()														// both items have the same ruleset
		&& ((itemA->getAmmoItem() == NULL && itemB->getAmmoItem() == NULL)								// either they both have no ammo
			|| (itemA->getAmmoItem() && itemB->getAmmoItem()											// or they both have ammo
				&& itemA->getAmmoItem()->getRules() == itemB->getAmmoItem()->getRules()					// and the same ammo type
				&& itemA->getAmmoItem()->getAmmoQuantity() == itemB->getAmmoItem()->getAmmoQuantity()))	// and the same ammo quantity
		&& itemA->getFuse() == -1 && itemB->getFuse() == -1												// and neither is set to explode
		&& itemA->getUnit() == NULL && itemB->getUnit() == NULL											// and neither is a corpse or unconscious unit
		&& itemA->getPainKillerQuantity() == itemB->getPainKillerQuantity()								// and if it's a medkit, it has the same number of charges
		&& itemA->getHealQuantity() == itemB->getHealQuantity()
		&& itemA->getStimulantQuantity() == itemB->getStimulantQuantity());
}

/**
 * Shows a warning message.
 * @param msg - reference a message to show
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
	const int pulse[22] = { 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,
						   13,12,11,10, 9, 8, 7, 6, 5, 4, 3 };

	if (_fuseFrame == 22)
		_fuseFrame = 0;

	Surface* const srf = _game->getResourcePack()->getSurfaceSet("SCANG.DAT")->getFrame(9);
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

	++_fuseFrame;
}

/**
 * Sets grenade to show a warning in Inventory.
 * @param turn - turns until grenade is to explode
 */
void Inventory::setPrimeGrenade(int turn)
{
	_prime = turn;
}

/**
 * Gets the TU cost for moving items around.
 * @return, time unit cost
 */
int Inventory::getTuCostInventory() const
{
	int wt;
	if (_tuCost > 0
		&& _selItem->getSlot()->getId() == "STR_GROUND")
	{
		wt = _selItem->getRules()->getWeight();
	}
	else
		wt = 0;

	return (_tuCost + wt);
}

}
