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

#include "State.h"

#include "Game.h"
#include "InteractiveSurface.h"
#include "Language.h"
//#include "LocalizedText.h"
//#include "Palette.h"
//#include "Screen.h"
#include "Surface.h"

#include "../Interface/ArrowButton.h"
#include "../Interface/BattlescapeButton.h"
#include "../Interface/ComboBox.h"
#include "../Interface/Cursor.h"
#include "../Interface/FpsCounter.h"
#include "../Interface/TextButton.h"
#include "../Interface/TextEdit.h"
#include "../Interface/TextList.h"
#include "../Interface/Slider.h"
#include "../Interface/Window.h"

#include "../Resource/ResourcePack.h"


namespace OpenXcom
{

/// Initializes static member
Game* State::_game = NULL;


/**
 * Initializes a brand new state with no child elements.
 * @note States are full-screen by default.
 */
State::State()
	:
		_screen(true),
		_modal(NULL),
		_ruleInterface(NULL),
		_ruleInterfaceParent(NULL)
{
	std::memset( // initialize palette to all black
			_palette,
			0,
			sizeof(_palette));

	_cursorColor = _game->getCursor()->getColor();
}

/**
 * Deletes all the child elements contained in the state.
 */
State::~State()
{
	for (std::vector<Surface*>::const_iterator
			i = _surfaces.begin();
			i != _surfaces.end();
			++i)
	{
		delete *i;
	}
}

/**
 * Sets interface data from the ruleset - also sets the palette for the state.
 * @param category		- reference the name of the interface from the Interfaces ruleset
 * @param alterPal		- true to swap out the backpal colors (default false)
 * @param battle	- true to use battlescape palette (applies only to options screens) (default false)
 */
void State::setInterface(
		const std::string& category,
		bool alterPal,
		bool battle)
{
	int backPal = -1;
	std::string pal = "PAL_GEOSCAPE";

	_ruleInterface = _game->getRuleset()->getInterface(category);
	if (_ruleInterface != NULL)
	{
		pal = _ruleInterface->getPalette();
		const Element* element = _ruleInterface->getElement("palette");

		_ruleInterfaceParent = _game->getRuleset()->getInterface(_ruleInterface->getParent());
		if (_ruleInterfaceParent != NULL)
		{
			if (element == NULL)
				element = _ruleInterfaceParent->getElement("palette");

			if (pal.empty() == true)
				pal = _ruleInterfaceParent->getPalette();
		}

		if (element != NULL
			&& battle == false)
		{
			int color;
			if (alterPal == true)
				color = element->color2;
			else
				color = element->color;

			if (color != std::numeric_limits<int>::max())
				backPal = color;
		}
	}

	if (battle == true)
		pal = "PAL_BATTLESCAPE";
	else if (pal.empty() == true)
		pal = "PAL_GEOSCAPE";

	setPalette(
			pal,
			backPal);
}

/**
 * Adds a new child surface for the state to take care of, giving it the game's
 * display palette. Once associated, the state handles all of the surface's
 * behaviour and management automatically.
 * @note Since visible elements can overlap one another they have to
 * be added in ascending Z-Order to be blitted correctly onto the screen.
 * @param surface - child surface
 */
void State::add(Surface* surface)
{
	surface->setPalette(_palette);

	if (_game->getLanguage() != NULL
		&& _game->getResourcePack() != NULL)
	{
		surface->initText(
					_game->getResourcePack()->getFont("FONT_BIG"),
					_game->getResourcePack()->getFont("FONT_SMALL"),
					_game->getLanguage());
	}

	_surfaces.push_back(surface);
}

/**
 * As above except this adds a surface based on an interface element defined in the ruleset.
 * @note that this function REQUIRES the ruleset to have been loaded prior to use.
 * @note if no parent is defined the element will not be moved.
 * @param surface	- pointer to child Surface
 * @param id		- reference the ID of the element defined in the ruleset if any
 * @param category	- reference the category of elements this interface is associated with
 * @param parent	- pointer to the Surface to base the coordinates of this element off (default NULL)
 */
void State::add(
		Surface* surface,
		const std::string& id,
		const std::string& category,
		Surface* parent)
{
	surface->setPalette(_palette);

	BattlescapeButton* const tacBtn = dynamic_cast<BattlescapeButton*>(surface);

	if (_game->getRuleset()->getInterface(category) != NULL)
	{
		const Element* const element = _game->getRuleset()->getInterface(category)->getElement(id);
		if (element != NULL)
		{
			if (parent != NULL)
			{
				if (element->x != std::numeric_limits<int>::max()
					&& element->y != std::numeric_limits<int>::max())
				{
					surface->setX(parent->getX() + element->x);
					surface->setY(parent->getY() + element->y);
				}

				if (element->w != -1
					&& element->h != -1)
				{
					surface->setWidth(element->w);
					surface->setHeight(element->h);
				}
			}

//			if (tacBtn != NULL)
//				tacBtn->setTftdMode(element->TFTDMode);
//			surface->setTFTDMode(element->TFTDMode);

			if (element->color != -1)
				surface->setColor(static_cast<Uint8>(element->color));

			if (element->color2 != -1)
				surface->setSecondaryColor(static_cast<Uint8>(element->color2));

			if (element->border != -1)
				surface->setBorderColor(static_cast<Uint8>(element->border));
		}
	}

	if (tacBtn != NULL)
	{
		tacBtn->copy(parent);
		tacBtn->initSurfaces();
	}

	if (   _game->getLanguage() != NULL
		&& _game->getResourcePack() != NULL)
	{
		surface->initText(
					_game->getResourcePack()->getFont("FONT_BIG"),
					_game->getResourcePack()->getFont("FONT_SMALL"),
					_game->getLanguage());
	}

	_surfaces.push_back(surface);
}

/**
 * Returns whether this is a full-screen state.
 * @note This is used to optimize the state machine since full-screen states
 * automatically cover the whole screen (whether they actually use it all or
 * not) so states behind them can be safely ignored since they'd be covered up.
 * @return, true if full-screen
 */
bool State::isScreen() const
{
	return _screen;
}

/**
 * Toggles the full-screen flag.
 * @note Used by windows to keep the previous screen in display while the window
 * is still "popping up".
 */
void State::toggleScreen()
{
	_screen = !_screen;
}

/**
 * Initializes the state and its child elements.
 * @note This is used for settings that have to be reset every time the state is
 * returned to focus (eg. palettes) so can't just be put in the constructor.
 * There's a stack of states so they can be created once but then repeatedly
 * switched in and out of focus.
 */
void State::init() // virtual
{
	_game->getScreen()->setPalette(_palette);

	_game->getCursor()->setPalette(_palette);
	_game->getCursor()->setColor(_cursorColor);
	_game->getCursor()->draw();

	_game->getFpsCounter()->setPalette(_palette);
	_game->getFpsCounter()->setColor(_cursorColor);
	_game->getFpsCounter()->draw();

	if (_game->getResourcePack() != NULL)
		_game->getResourcePack()->setPalette(_palette);
}

/**
 * Runs any code the state needs to keep updating every game cycle, like timers
 * and other real-time elements.
 */
void State::think() // virtual
{
	for (std::vector<Surface*>::const_iterator
			i = _surfaces.begin();
			i != _surfaces.end();
			++i)
	{
		(*i)->think();
	}
}

/**
 * Blits all the visible Surface child elements onto the display screen by order
 * of addition.
 */
void State::blit() // virtual
{
	for (std::vector<Surface*>::const_iterator
			i = _surfaces.begin();
			i != _surfaces.end();
			++i)
	{
		(*i)->blit(_game->getScreen()->getSurface());
	}
}

/**
 * Takes care of any events from the core game engine, and passes them on to its
 * InteractiveSurface child elements.
 * @param action - pointer to an Action
 */
void State::handle(Action* action) // virtual
{
	if (_modal == NULL)
	{
		for (std::vector<Surface*>::const_reverse_iterator
				i = _surfaces.rbegin();
				i != _surfaces.rend();
				++i)
		{
			InteractiveSurface* const srf = dynamic_cast<InteractiveSurface*>(*i);
			if (srf != NULL)
				srf->handle(action, this);
		}
	}
	else
		_modal->handle(action, this);
}

/**
 * Hides all the Surface child elements on display.
 */
void State::hideAll()
{
	for (std::vector<Surface*>::const_iterator
			i = _surfaces.begin();
			i != _surfaces.end();
			++i)
	{
		(*i)->setHidden(true);
	}
}

/**
 * Shows all the hidden Surface child elements.
 */
void State::showAll()
{
	for (std::vector<Surface*>::const_iterator
			i = _surfaces.begin();
			i != _surfaces.end();
			++i)
	{
		(*i)->setHidden(false);
	}
}

/**
 * Resets the status of all the Surface child elements like unpressing buttons.
 */
void State::resetAll()
{
	for (std::vector<Surface*>::const_iterator
			i = _surfaces.begin();
			i != _surfaces.end();
			++i)
	{
		InteractiveSurface* const srf = dynamic_cast<InteractiveSurface*>(*i);
		if (srf != NULL)
		{
			srf->unpress(this);
//			srf->setFocus(false);
		}
	}
}

/**
 * Gets the LocalizedText for dictionary key @a id.
 * @note This function forwards the call to Language::getString(const std::string&).
 * @param id - reference the dictionary key to search for
 * @return, a reference to the LocalizedText
 */
const LocalizedText& State::tr(const std::string& id) const
{
	return _game->getLanguage()->getString(id);
}

/**
 * Gets a modifiable copy of the LocalizedText for dictionary key @a id.
 * @note This function forwards the call to Language::getString(const std::string&, unsigned).
 * @param id	- reference the dictionary key to search for
 * @param n		- the number to use for the proper version
 * @return, a copy of the LocalizedLext
 */
LocalizedText State::tr(
		const std::string& id,
		unsigned n) const
{
	return _game->getLanguage()->getString(id, n);
}

/**
 * Centers all the surfaces on the screen.
 */
void State::centerAllSurfaces()
{
	for (std::vector<Surface*>::const_iterator
			i = _surfaces.begin();
			i != _surfaces.end();
			++i)
	{
		(*i)->setX((*i)->getX() + _game->getScreen()->getDX());
		(*i)->setY((*i)->getY() + _game->getScreen()->getDY());
	}
}

/**
 * Drops all the surfaces by half the screen height.
 */
void State::lowerAllSurfaces()
{
	for (std::vector<Surface*>::const_iterator
			i = _surfaces.begin();
			i != _surfaces.end();
			++i)
	{
		(*i)->setY((*i)->getY() + _game->getScreen()->getDY() / 2);
	}
}

/**
 * Switches all the colors to something a little more battlescape appropriate.
 */
void State::applyBattlescapeTheme()
{
	const Element* const element = _game->getRuleset()->getInterface("mainMenu")->getElement("battlescapeTheme");

	for (std::vector<Surface*>::const_iterator
			i = _surfaces.begin();
			i != _surfaces.end();
			++i)
	{
		Window* const window = dynamic_cast<Window*>(*i);
		if (window != NULL)
		{
			window->setColor(static_cast<Uint8>(element->color));
			window->setHighContrast();
			window->setBackground(_game->getResourcePack()->getSurface("TAC00.SCR"));

			continue;
		}

		Text* const text = dynamic_cast<Text*>(*i);
		if (text != NULL)
		{
			text->setColor(static_cast<Uint8>(element->color));
			text->setHighContrast();

			continue;
		}

		TextButton* const btn = dynamic_cast<TextButton*>(*i);
		if (btn != NULL)
		{
			btn->setColor(static_cast<Uint8>(element->color));
			btn->setHighContrast();

			continue;
		}

		TextEdit* const edit = dynamic_cast<TextEdit*>(*i);
		if (edit != NULL)
		{
			edit->setColor(static_cast<Uint8>(element->color));
			edit->setHighContrast();

			continue;
		}

		TextList* const textList = dynamic_cast<TextList*>(*i);
		if (textList != NULL)
		{
			textList->setColor(static_cast<Uint8>(element->color));
			textList->setArrowColor(static_cast<Uint8>(element->border));
			textList->setHighContrast();

			continue;
		}

		ArrowButton* const arrow = dynamic_cast<ArrowButton*>(*i);
		if (arrow != NULL)
		{
			arrow->setColor(static_cast<Uint8>(element->border));

			continue;
		}

		Slider* const slider = dynamic_cast<Slider*>(*i);
		if (slider != NULL)
		{
			slider->setColor(static_cast<Uint8>(element->color));
			slider->setHighContrast();

			continue;
		}

		ComboBox* const combo = dynamic_cast<ComboBox*>(*i);
		if (combo != NULL)
		{
			combo->setColor(static_cast<Uint8>(element->color));
			combo->setArrowColor(static_cast<Uint8>(element->border));
			combo->setHighContrast();
		}
	}
}

/**
 * Redraws all the text-type surfaces.
 */
void State::redrawText()
{
	for (std::vector<Surface*>::const_iterator
			i = _surfaces.begin();
			i != _surfaces.end();
			++i)
	{
		const Text* const text = dynamic_cast<Text*>(*i);
		const TextButton* const btn = dynamic_cast<TextButton*>(*i);
		const TextEdit* const edit = dynamic_cast<TextEdit*>(*i);
		const TextList* const textList = dynamic_cast<TextList*>(*i);

		if (text != NULL
			|| btn != NULL
			|| edit != NULL
			|| textList != NULL)
		{
			(*i)->draw();
		}
	}
}

/**
 * Changes the current modal surface.
 * @note If a surface is modal then only that surface can receive events. This
 * is used when an element needs to take priority over everything else, eg. focus.
 * @param surface - pointer to modal surface; NULL for no modal
 */
void State::setModal(InteractiveSurface* surface)
{
	_modal = surface;
}

/**
 * Replaces a certain amount of colors in the state's palette.
 * @param colors		- pointer to the set of colors
 * @param firstcolor	- offset of the first color to replace (default 0)
 * @param ncolors		- amount of colors to replace (default 256)
 * @param immediately	- apply changes immediately, otherwise wait in case of multiple setPalettes (default true)
 */
void State::setPalette(
		SDL_Color* colors,
		int firstcolor,
		int ncolors,
		bool immediately)
{
	if (colors != NULL)
		std::memcpy(
				_palette + firstcolor,
				colors,
				ncolors * sizeof(SDL_Color));

	if (immediately == true)
	{
		_game->getCursor()->setPalette(_palette);
		_game->getCursor()->draw();

		_game->getFpsCounter()->setPalette(_palette);
		_game->getFpsCounter()->draw();

		if (_game->getResourcePack() != NULL)
			_game->getResourcePack()->setPalette(_palette);
	}
}

/**
 * Loads palettes from the game resources into the state.
 * @param palette	- reference the string ID of the palette to load
 * @param backpal	- BACKPALS.DAT offset to use (default -1)
 */
void State::setPalette(
		const std::string& palette,
		int backpal)
{
	setPalette(
			_game->getResourcePack()->getPalette(palette)->getColors(),
			0,
			256,
			false);

	if (palette == "PAL_GEOSCAPE")
		_cursorColor = static_cast<Uint8>(ResourcePack::GEOSCAPE_CURSOR);
	else if (palette == "PAL_BASESCAPE")
		_cursorColor = static_cast<Uint8>(ResourcePack::BASESCAPE_CURSOR);
	else if (palette == "PAL_UFOPAEDIA")
		_cursorColor = static_cast<Uint8>(ResourcePack::UFOPAEDIA_CURSOR);
	else if (palette == "PAL_GRAPHS")
		_cursorColor = static_cast<Uint8>(ResourcePack::GRAPHS_CURSOR);
	else
		_cursorColor = static_cast<Uint8>(ResourcePack::BATTLESCAPE_CURSOR);

	if (backpal != -1)
		setPalette(
				_game->getResourcePack()->getPalette("BACKPALS.DAT")->getColors(static_cast<int>(Palette::blockOffset(static_cast<Uint8>(backpal)))),
				Palette::PAL_bgID,
				16,
				false);

	setPalette(NULL); // delay actual update to the end
}

/**
 * Returns the state's 8bpp palette.
 * @return, pointer to the palette's colors
 */
SDL_Color* const State::getPalette()
{
	return _palette;
}

/**
 * Each state will probably need its own resize handling so this space
 * intentionally left blank.
 * @param dX - reference the delta of X
 * @param dY - reference the delta of Y
 */
void State::resize(
		int& dX,
		int& dY)
{
	recenter(dX, dY);
}

/**
 * Re-orients all the surfaces in the state.
 * @param dX - delta of X
 * @param dY - delta of Y
 */
void State::recenter(
		int dX,
		int dY)
{
	for (std::vector<Surface*>::const_iterator
			i = _surfaces.begin();
			i != _surfaces.end();
			++i)
	{
		(*i)->setX((*i)->getX() + dX / 2);
		(*i)->setY((*i)->getY() + dY / 2);
	}
}

/**
 * Sets a pointer to the Game object.
 * @note This pointer can be used universally by all child-States.
 * @param game - THE pointer to Game
 */
void State::setGamePtr(Game* game)
{
	_game = game;
}

}
