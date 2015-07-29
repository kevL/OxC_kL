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

#include "InteractiveSurface.h"

#include "Action.h"


namespace OpenXcom
{

const SDLKey InteractiveSurface::SDLK_ANY = (SDLKey) - 1; // using an unused keycode to represent an "any key"


/**
 * Sets up a blank interactive surface with the specified size and position.
 * @param width		- width in pixels
 * @param height	- height in pixels
 * @param x			- X position in pixels (default 0)
 * @param y			- Y position in pixels (default 0)
 */
InteractiveSurface::InteractiveSurface(
		int width,
		int height,
		int x,
		int y)
	:
		Surface(
			width,
			height,
			x,
			y),
		_buttonsPressed(0),
		_in(0),
		_over(0),
		_out(0),
		_isHovered(false),
		_isFocused(true),
		_listButton(false)
{}

/**
 * dTor.
 */
InteractiveSurface::~InteractiveSurface() // virtual
{}

/**
 *
 * @param btn - (default 0)
 */
bool InteractiveSurface::isButtonPressed(Uint8 btn)
{
	if (btn == 0)
		return (_buttonsPressed != 0);
	else
		return (_buttonsPressed & SDL_BUTTON(btn)) != 0;
}

/**
 *
 * @param btn - (default 0)
 */
bool InteractiveSurface::isButtonHandled(Uint8 btn) // virtual
{
	bool handled = (_click.find(0) != _click.end()
				|| _press.find(0) != _press.end()
				|| _release.find(0) != _release.end());

	if (handled == false && btn != 0)
	{
		handled = (_click.find(btn) != _click.end()
				|| _press.find(btn) != _press.end()
				|| _release.find(btn) != _release.end());
	}

	return handled;
}

/**
 *
 * @param btn		-
 * @param pressed	- true if pressed
 */
void InteractiveSurface::setButtonPressed(
		Uint8 btn,
		bool pressed)
{
	if (pressed == true)
		_buttonsPressed = _buttonsPressed | SDL_BUTTON(btn);
	else
		_buttonsPressed = _buttonsPressed & (!SDL_BUTTON(btn));
}

/**
 * Changes the visibility of the surface. A hidden surface
 * isn't blitted nor receives events.
 * @param visible - true if visible (default true)
 */
void InteractiveSurface::setVisible(bool visible)
{
	Surface::setVisible(visible);

	if (_visible == false) // Unpress button if it was hidden
		unpress(NULL);
}

/**
 * Called whenever an action occurs, and processes it to
 * check if it's relevant to the surface and convert it
 * into a meaningful interaction like a "click", calling
 * the respective handlers.
 * @param action - pointer to an Action
 * @param state - state that the action handlers belong to
 */
void InteractiveSurface::handle( // virtual
		Action* action,
		State* state)
{
	if (_visible == false || _hidden == true)
		return;

	action->setSender(this);

	if (action->getDetails()->type == SDL_MOUSEBUTTONUP
		|| action->getDetails()->type == SDL_MOUSEBUTTONDOWN)
	{
		action->setMouseAction(
							action->getDetails()->button.x,
							action->getDetails()->button.y,
							getX(),
							getY());
	}
	else if (action->getDetails()->type == SDL_MOUSEMOTION)
		action->setMouseAction(
							action->getDetails()->motion.x,
							action->getDetails()->motion.y,
							getX(),
							getY());

	if (action->isMouseAction() == true)
	{
		if (   action->getAbsoluteXMouse() >= getX()
			&& action->getAbsoluteXMouse() < getX() + getWidth()
			&& action->getAbsoluteYMouse() >= getY()
			&& action->getAbsoluteYMouse() < getY() + getHeight())
		{
			if (_isHovered == false)
			{
				_isHovered = true;
				mouseIn(action, state);
			}

			if (_listButton == true
				&& action->getDetails()->type == SDL_MOUSEMOTION)
			{
				_buttonsPressed = SDL_GetMouseState(NULL, NULL);
				for (Uint8
						i = 1;
						i <= NUM_BUTTONS;
						++i)
				{
					if (isButtonPressed(i) == true)
					{
						action->getDetails()->button.button = i;
						mousePress(action, state);
					}
				}
			}

			mouseOver(action, state);
		}
		else
		{
			if (_isHovered == true)
			{
				_isHovered = false;
				mouseOut(action, state);

				if (_listButton == true
					&& action->getDetails()->type == SDL_MOUSEMOTION)
				{
					for (Uint8
							i = 1;
							i <= NUM_BUTTONS;
							++i)
					{
						if (isButtonPressed(i) == true)
							setButtonPressed(i, false);

						action->getDetails()->button.button = i;
						mouseRelease(action, state);
					}
				}
			}
		}
	}

	if (action->getDetails()->type == SDL_MOUSEBUTTONDOWN)
	{
		if (_isHovered == true
			&& isButtonPressed(action->getDetails()->button.button) == false)
		{
			setButtonPressed(action->getDetails()->button.button, true);
			mousePress(action, state);
		}
	}
	else if (action->getDetails()->type == SDL_MOUSEBUTTONUP)
	{
		if (isButtonPressed(action->getDetails()->button.button) == true)
		{
			setButtonPressed(action->getDetails()->button.button, false);
			mouseRelease(action, state);
			if (_isHovered == true)
				mouseClick(action, state);
		}
	}

	if (_isFocused == true)
	{
		if (action->getDetails()->type == SDL_KEYDOWN)
			keyboardPress(action, state);
		else if (action->getDetails()->type == SDL_KEYUP)
			keyboardRelease(action, state);
	}
}

/**
 * Changes the surface's focus.
 * Surfaces will only receive keyboard events if focused.
 * @param focus - true if focused
 */
void InteractiveSurface::setFocus(bool focus) // virtual
{
	_isFocused = focus;
}

/**
 * Returns the surface's focus.
 * Surfaces will only receive keyboard events if focused.
 * @return, true if focused
 */
bool InteractiveSurface::isFocused() const
{
	return _isFocused;
}

/**
 * Simulates a "mouse button release". Used in circumstances
 * where the surface is unpressed without user input.
 * @param state Pointer to running state.
 */
void InteractiveSurface::unpress(State* state) // virtual
{
	if (isButtonPressed() == true)
	{
		_buttonsPressed = 0;
		SDL_Event ev;
		ev.type = SDL_MOUSEBUTTONUP;
		ev.button.button = SDL_BUTTON_LEFT;
		Action a = Action(&ev, 0.,0.,0,0);
		mouseRelease(&a, state);
	}
}

/**
 * Called every time there's a mouse press over the surface.
 * Allows the surface to have custom functionality for this action,
 * and can be called externally to simulate the action.
 * @param action - pointer to an Action
 * @param state - state that the action handlers belong to
 */
void InteractiveSurface::mousePress(Action* action, State* state) // virtual
{
	std::map<Uint8, ActionHandler>::const_iterator allHandler = _press.find(0);
	if (allHandler != _press.end())
	{
		ActionHandler handler = allHandler->second;
		(state->*handler)(action);
	}

	std::map<Uint8, ActionHandler>::const_iterator oneHandler = _press.find(action->getDetails()->button.button);
	if (oneHandler != _press.end())
	{
		ActionHandler handler = oneHandler->second;
		(state->*handler)(action);
	}
}

/**
 * Called every time there's a mouse release over the surface.
 * Allows the surface to have custom functionality for this action,
 * and can be called externally to simulate the action.
 * @param action - pointer to an Action
 * @param state - state that the action handlers belong to
 */
void InteractiveSurface::mouseRelease(Action* action, State* state) // virtual
{
	std::map<Uint8, ActionHandler>::const_iterator allHandler = _release.find(0);
	if (allHandler != _release.end())
	{
		ActionHandler handler = allHandler->second;
		(state->*handler)(action);
	}

	std::map<Uint8, ActionHandler>::const_iterator oneHandler = _release.find(action->getDetails()->button.button);
	if (oneHandler != _release.end())
	{
		ActionHandler handler = oneHandler->second;
		(state->*handler)(action);
	}
}

/**
 * Called every time there's a mouse click on the surface.
 * Allows the surface to have custom functionality for this action,
 * and can be called externally to simulate the action.
 * @param action - pointer to an Action
 * @param state - state that the action handlers belong to
 */
void InteractiveSurface::mouseClick(Action* action, State* state) // virtual
{
	std::map<Uint8, ActionHandler>::const_iterator allHandler = _click.find(0);
	if (allHandler != _click.end())
	{
		ActionHandler handler = allHandler->second;
		(state->*handler)(action);
	}

	std::map<Uint8, ActionHandler>::const_iterator oneHandler = _click.find(action->getDetails()->button.button);
	if (oneHandler != _click.end())
	{
		ActionHandler handler = oneHandler->second;
		(state->*handler)(action);
	}
}

/**
 * Called every time the mouse moves into the surface.
 * Allows the surface to have custom functionality for this action,
 * and can be called externally to simulate the action.
 * @param action - pointer to an Action
 * @param state - state that the action handlers belong to
 */
void InteractiveSurface::mouseIn(Action* action, State* state) // virtual
{
	if (_in != 0)
		(state->*_in)(action);
}

/**
 * Called every time the mouse moves over the surface.
 * Allows the surface to have custom functionality for this action,
 * and can be called externally to simulate the action.
 * @param action - pointer to an Action
 * @param state - state that the action handlers belong to
 */
void InteractiveSurface::mouseOver(Action* action, State* state) // virtual
{
	if (_over != 0)
		(state->*_over)(action);
}

/**
 * Called every time the mouse moves out of the surface.
 * Allows the surface to have custom functionality for this action,
 * and can be called externally to simulate the action.
 * @param action - pointer to an Action
 * @param state - state that the action handlers belong to
 */
void InteractiveSurface::mouseOut(Action* action, State* state) // virtual
{
	if (_out != 0)
		(state->*_out)(action);
}

/**
 * Called every time there's a keyboard press when the surface is focused.
 * Allows the surface to have custom functionality for this action,
 * and can be called externally to simulate the action.
 * @param action - pointer to an Action
 * @param state - state that the action handlers belong to
 */
void InteractiveSurface::keyboardPress(Action* action, State* state) // virtual
{
	std::map<SDLKey, ActionHandler>::const_iterator allHandler = _keyPress.find(SDLK_ANY);
	if (allHandler != _keyPress.end())
	{
		ActionHandler handler = allHandler->second;
		(state->*handler)(action);
	}

	// Check if Ctrl, Alt and Shift are pressed
	std::map<SDLKey, ActionHandler>::const_iterator oneHandler = _keyPress.find(action->getDetails()->key.keysym.sym);
	if (oneHandler != _keyPress.end()
		&& (action->getDetails()->key.keysym.mod & (KMOD_CTRL | KMOD_ALT | KMOD_SHIFT)) == 0)
	{
		ActionHandler handler = oneHandler->second;
		(state->*handler)(action);
	}
}

/**
 * Called every time there's a keyboard release over the surface.
 * Allows the surface to have custom functionality for this action,
 * and can be called externally to simulate the action.
 * @param action - pointer to an Action
 * @param state - state that the action handlers belong to
 */
void InteractiveSurface::keyboardRelease(Action* action, State* state) // virtual
{
	std::map<SDLKey, ActionHandler>::const_iterator allHandler = _keyRelease.find(SDLK_ANY);
	if (allHandler != _keyRelease.end())
	{
		ActionHandler handler = allHandler->second;
		(state->*handler)(action);
	}

	// Check if Ctrl, Alt and Shift are pressed
	std::map<SDLKey, ActionHandler>::const_iterator oneHandler = _keyRelease.find(action->getDetails()->key.keysym.sym);
	if (oneHandler != _keyRelease.end()
		&& (action->getDetails()->key.keysym.mod & (KMOD_CTRL | KMOD_ALT | KMOD_SHIFT)) == 0)
	{
		ActionHandler handler = oneHandler->second;
		(state->*handler)(action);
	}
}

/**
 * Sets a function to be called every time the surface is mouse clicked.
 * @param handler	- action handler
 * @param btn		- mouse button to check for; set to 0 for any button (default SDL_BUTTON_LEFT=1)
 */
void InteractiveSurface::onMouseClick(
		ActionHandler handler,
		Uint8 btn)
{
	if (handler != 0)
		_click[btn] = handler;
	else
		_click.erase(btn);
}

/**
 * Sets a function to be called every time the surface is mouse pressed.
 * @param handler	- action handler
 * @param btn		- mouse button to check for; set to 0 for any button (default 0)
 */
void InteractiveSurface::onMousePress(
		ActionHandler handler,
		Uint8 btn)
{
	if (handler != 0)
		_press[btn] = handler;
	else
		_press.erase(btn);
}

/**
 * Sets a function to be called every time the surface is mouse released.
 * @param handler	- action handler
 * @param btn		- mouse button to check for; set to 0 for any button (default 0)
 */
void InteractiveSurface::onMouseRelease(
		ActionHandler handler,
		Uint8 btn)
{
	if (handler != 0)
		_release[btn] = handler;
	else
		_release.erase(btn);
}

/**
 * Sets a function to be called every time the mouse moves into the surface.
 * @param handler Action handler.
 */
void InteractiveSurface::onMouseIn(ActionHandler handler)
{
	_in = handler;
}

/**
 * Sets a function to be called every time the mouse moves over the surface.
 * @param handler Action handler.
 */
void InteractiveSurface::onMouseOver(ActionHandler handler)
{
	_over = handler;
}

/**
 * Sets a function to be called every time the mouse moves out of the surface.
 * @param handler Action handler.
 */
void InteractiveSurface::onMouseOut(ActionHandler handler)
{
	_out = handler;
}

/**
 * Sets a function to be called every time a key is pressed when the surface is focused.
 * @param handler Action handler.
 * @param key Keyboard button to check for (note: ignores key modifiers). Set to 0 for any key. (default SDLK_ANY)
 */
void InteractiveSurface::onKeyboardPress(
		ActionHandler handler,
		SDLKey key)
{
	if (handler != 0)
		_keyPress[key] = handler;
	else
		_keyPress.erase(key);
}

/**
 * Sets a function to be called every time a key is released when the surface is focused.
 * @param handler Action handler.
 * @param key Keyboard button to check for (note: ignores key modifiers). Set to 0 for any key. (default SDLK_ANY)
 */
void InteractiveSurface::onKeyboardRelease(
		ActionHandler handler,
		SDLKey key)
{
	if (handler != 0)
		_keyRelease[key] = handler;
	else
		_keyRelease.erase(key);
}

/**
 * Sets a flag for this button to say "i'm a member of a textList" to true.
 */
void InteractiveSurface::setListButton()
{
	_listButton = true;
}

}
