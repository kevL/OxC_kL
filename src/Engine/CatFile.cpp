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

#include "CatFile.h"

#include <SDL.h>


namespace OpenXcom
{

/**
 * Creates a CAT file stream. A CAT file starts with an index of the
 * offset and size of every file contained within. Each file consists
 * of a filename followed by its contents.
 * @param path - full path to CAT file
 */
CatFile::CatFile(const char* path)
	:
		std::ifstream(
			path,
			std::ios::in | std::ios::binary),
		_amount(0),
		_offset(0),
		_size(0)
{
	// Get amount of files
	read(
		(char*)&_amount,
		sizeof(_amount));

	_amount = static_cast<unsigned>(SDL_SwapLE32(_amount));
	_amount /= 2 * sizeof(_amount);

	// Get object offsets
	seekg(
		0,
		std::ios::beg);

	_offset = new unsigned int[_amount];
	_size = new unsigned int[_amount];

	for (unsigned
			i = 0;
			i < _amount;
			++i)
	{
		read(
			(char*)&_offset[i],
			sizeof(*_offset));
		_offset[i] = static_cast<unsigned>(SDL_SwapLE32(_offset[i]));

		read(
			(char*)&_size[i],
			sizeof(*_size));
		_size[i] = static_cast<unsigned>(SDL_SwapLE32(_size[i]));
	}
}

/**
 * Frees associated memory.
 */
CatFile::~CatFile()
{
	delete[] _offset;
	delete[] _size;

	close();
}

/**
 * Loads an object into memory.
 * @param i		- Object number to load.
 * @param name	- True to preserve internal file name.
 * @return, pointer to the loaded object
 */
char* CatFile::load(
		unsigned i,
		bool name)
{
	if (i >= _amount)
		return 0;

	seekg(
		_offset[i],
		std::ios::beg);

	unsigned char namesize = peek();

	if (namesize <= 56)
	{
		if (name == false)
		{
			seekg(
				namesize + 1,
				std::ios::cur);
		}
		else
			_size[i] += namesize + 1;
	}

	char* object = new char[_size[i]];
	read(
		object,
		_size[i]);

	return object;
}

}
