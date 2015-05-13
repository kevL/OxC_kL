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

#ifndef OPENXCOM_OPTIONSBATTLESCAPESTATE_H
#define OPENXCOM_OPTIONSBATTLESCAPESTATE_H

#include "OptionsBaseState.h"


namespace OpenXcom
{

class ComboBox;
class Slider;
class Text;
class TextButton;
class ToggleTextButton;
class Window;


/**
 * Screen that lets the user configure various
 * Battlescape options.
 */
class OptionsBattlescapeState
	:
		public OptionsBaseState
{

private:
	ComboBox
		* _cbxEdgeScroll,
		* _cbxDragScroll;
	Text
		* _txtEdgeScroll,
		* _txtDragScroll,
		* _txtScrollSpeed,
		* _txtFireSpeed,
		* _txtXcomSpeed,
		* _txtAlienSpeed,
		* _txtPathPreview,
		* _txtOptions;
	Slider
		* _slrScrollSpeed,
		* _slrFireSpeed,
		* _slrXcomSpeed,
		* _slrAlienSpeed;
	ToggleTextButton
		* _btnArrows,
		* _btnTuCost,
		* _btnTooltips,
		* _btnDeaths;


	public:
		/// Creates the Battlescape Options state.
		explicit OptionsBattlescapeState(OptionsOrigin origin);
		/// Cleans up the Battlescape Options state.
		~OptionsBattlescapeState();

		/// Handler for changing the Edge Scroll combobox.
		void cbxEdgeScrollChange(Action* action);
		/// Handler for changing the Drag Scroll combobox.
		void cbxDragScrollChange(Action* action);
		/// Handler for changing the scroll speed slider.
		void slrScrollSpeedChange(Action* action);
		/// Handler for changing the fire speed slider.
		void slrFireSpeedChange(Action* action);
		/// Handler for changing the X-COM movement speed slider.
		void slrXcomSpeedChange(Action* action);
		/// Handler for changing the alien movement speed slider.
		void slrAlienSpeedChange(Action* action);
		/// Handler for clicking a Path Preview button.
		void btnPathPreviewClick(Action* action);
		/// Handler for clicking the Tooltips button.
		void btnTooltipsClick(Action* action);
		/// Handler for clicking the Death Notifications button.
		void btnDeathsClick(Action* action);
};

}

#endif
