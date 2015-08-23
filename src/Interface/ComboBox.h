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

#ifndef OPENXCOM_COMBOBOX_H
#define OPENXCOM_COMBOBOX_H

//#include <string>
//#include <vector>

#include "../Engine/InteractiveSurface.h"


namespace OpenXcom
{

class Language;
class TextButton;
class TextList;
class Window;


/**
 * Text button with a list dropdown when pressed.
 * Allows selection from multiple available options.
 */
class ComboBox
	:
		public InteractiveSurface
{

private:
	static const int
		MARGIN_HORIZONTAL,
		MARGIN_VERTICAL,
		MAX_ITEMS,
		BUTTON_WIDTH,
		TEXT_HEIGHT;

	bool _toggled;
	size_t _sel;
	Uint8 _color;

	ActionHandler _change;

	Language* _lang;
	State* _state;
	Surface* _arrow;
	TextButton* _button;
	TextList* _list;
	Window* _window;

	///
	void drawArrow();
	///
	void setDropdown(int options);


	public:
		/// Creates a combo box with the specified size and position.
		ComboBox(
				State* state,
				int width,
				int height,
				int x = 0,
				int y = 0);
		/// Cleans up the combo box.
		~ComboBox();

		/// Sets the X position of the surface.
		void setX(int x);
		/// Sets the Y position of the surface.
		void setY(int y);

		/// Sets the palette of the text list.
		void setPalette(
				SDL_Color* colors,
				int firstcolor = 0,
				int ncolors = 256);

		/// Initializes the resources for the text list.
		void initText(
				Font* big,
				Font* small,
				Language* lang);

		/// Sets the background surface.
		void setBackground(Surface* bg);
		/// Sets the color to fill the background.
		void setBackgroundFill(Uint8 color);

		/// Sets the border color.
		void setColor(Uint8 color);
		/// Gets the border color.
		Uint8 getColor() const;

		/// Sets the high contrast color setting.
		void setHighContrast(bool contrast = true);

		/// Sets the arrow color of the text list.
		void setArrowColor(Uint8 color);

		/// Gets the selected option in the list.
		size_t getSelected() const;
		/// Sets the selected option in the list.
		void setSelected(size_t sel);

		/// Sets the list of options.
		void setOptions(const std::vector<std::string>& options);
		/// Sets the list of options.
		void setOptions(const std::vector<std::wstring>& options);

		/// Blits the combo box onto another surface.
		void blit(Surface* surface);

		/// Thinks arrow buttons.
		void think();

		/// Handle arrow buttons.
		void handle(Action* action, State* state);

		/// Toggles the combo box state.
		void toggle(bool init = false);

		/// Hook to an action handler when the content changes.
		void onChange(ActionHandler handler);
};

}

#endif
