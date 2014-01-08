/*
 * Copyright 2010-2013 OpenXcom Developers.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OPENXCOM_RESEARCHSTATE_H
#define OPENXCOM_RESEARCHSTATE_H

#include <SDL.h>

#include "../Engine/State.h"


namespace OpenXcom
{

class Base;
class Text;
class TextButton;
class TextList;
class Window;


/**
 * Research screen that lets the player manage
 * all the researching operations of a base.
 */
class ResearchState
	:
		public State
{

private:
	Base* _base;
	Text
		* _txtAllocated,
		* _txtAvailable,
		* _txtBaseLabel,
		* _txtProgress,
		* _txtProject,
		* _txtScientists,
		* _txtSpace,
		* _txtTitle;
	TextButton
		* _btnNew,
		* _btnOk;
	TextList* _lstResearch;
	SDL_Color _oldPalette[256];
	Window* _window;


	public:
		/// Creates the Research state.
		ResearchState(
				Game* game,
				Base* base);
		/// Cleans up the Research state.
		~ResearchState();

		/// Handler for clicking the OK button.
		void btnOkClick(Action* action);
		/// Handler for clicking the New Research button.
		void btnNewClick(Action* action);
		/// Handler for clicking the ResearchProject list.
		void onSelectProject(Action* action);

		/// Updates the research list.
		void init();
};

}

#endif
