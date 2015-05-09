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

#include "SurfaceSet.h"

//#include <fstream>
//#include <SDL_endian.h>

#include "Surface.h"
//#include "Exception.h"


namespace OpenXcom
{

/**
 * Sets up a new empty SurfaceSet for frames of the specified size.
 * @param width		- frame width in pixels
 * @param height	- frame height in pixels
 */
SurfaceSet::SurfaceSet(
		int width,
		int height)
	:
		_width(width),
		_height(height)
{}

/**
 * Performs a deep copy of an existing SurfaceSet.
 * @param other - reference to a SurfaceSet to copy from
 */
SurfaceSet::SurfaceSet(const SurfaceSet& other)
{
	_width = other._width;
	_height = other._height;

	for (std::map<int, Surface*>::const_iterator
			f = other._frames.begin();
			f != other._frames.end();
			++f)
	{
		_frames[f->first] = new Surface(*f->second);
	}
}

/**
 * Deletes the images from memory.
 */
SurfaceSet::~SurfaceSet()
{
	for (std::map<int, Surface*>::iterator
			i = _frames.begin();
			i != _frames.end();
			++i)
	{
		delete i->second;
	}
}

/**
 * Loads the contents of an X-Com set of PCK/TAB image files into this SurfaceSet.
 * The PCK file contains an RLE compressed image while the TAB file contains
 * the offsets to each frame in the image.
 * @param pck - reference to filename of the PCK image
 * @param tab - reference to filename of the TAB offsets (default "")
 * @sa http://www.ufopaedia.org/index.php?title=Image_Formats#PCK
 */
void SurfaceSet::loadPck(
		const std::string& pck,
		const std::string& tab)
{
	//Log(LOG_INFO) << "SurfaceSet::loadPck() " << pck;
	size_t nframes = 0;

	// Load TAB and get image offsets
	if (tab.empty() == false)
	{
		std::ifstream offsetFile (tab.c_str(), std::ios::in | std::ios::binary); // init.
		if (offsetFile.fail() == true)
		{
			throw Exception(tab + " not found");
		}

		std::streampos
			start,
			stop;

		start = offsetFile.tellg();

		int offset;
		offsetFile.read(
					(char*)&offset,
					sizeof(offset));
		offsetFile.seekg(
					0,
					std::ios::end);

		stop = offsetFile.tellg();

		const size_t tabSize = static_cast<size_t>(stop - start);

		if (offset != 0)
			nframes = tabSize / 2; // 16-bit offsets
		else
			nframes = tabSize / 4; // 32-bit offsets

		offsetFile.close();

		for (size_t
				frame = 0;
				frame != nframes;
				++frame)
		{
			_frames[frame] = new Surface(
									_width,
									_height);
		}
	}
	else
	{
		nframes = 1;
		_frames[0] = new Surface(
							_width,
							_height);
	}

	// Load PCK and put pixels in surfaces
	std::ifstream imgFile (pck.c_str(), std::ios::in | std::ios::binary); // init.
	if (imgFile.fail() == true)
	{
		throw Exception(pck + " not found");
	}

	Uint8 value;
	int
		x,y;

	for (size_t
			frame = 0;
			frame != nframes;
			++frame)
	{
		x =
		y = 0;

		_frames[frame]->lock();
		imgFile.read(
				(char*)&value,
				1);
		for (Uint8
				i = 0;
				i != value;
				++i)
		{
			for (int
					j = 0;
					j != _width;
					++j)
			{
				_frames[frame]->setPixelIterative(&x,&y, 0);
			}
		}

		while (imgFile.read(
						(char*)&value,
						1)
			&& value != 255)
		{
			if (value == 254)
			{
				imgFile.read(
						(char*)&value,
						1);
				for (Uint8
						i = 0;
						i != value;
						++i)
				{
					_frames[frame]->setPixelIterative(&x,&y, 0);
				}
			}
			else
				_frames[frame]->setPixelIterative(&x,&y, value);
		}
		_frames[frame]->unlock();
	}

	imgFile.close();
}

/**
 * Loads the contents of an X-Com DAT image file into this SurfaceSet.
 * Unlike the PCK, a DAT file is an uncompressed image with no
 * offsets so these have to be figured out manually, usually
 * by splitting the image into equal portions.
 * @param filename - reference the filename of the DAT image
 * @sa http://www.ufopaedia.org/index.php?title=Image_Formats#SCR_.26_DAT
 */
void SurfaceSet::loadDat(const std::string& filename)
{
	size_t nframes = 0;

	// Load file and put pixels in surface
	std::ifstream imgFile (filename.c_str(), std::ios::in | std::ios::binary); // init.
	if (imgFile.fail() == true)
	{
		throw Exception(filename + " not found");
	}

	imgFile.seekg(
				0,
				std::ios::end);
	std::streamoff datSize = imgFile.tellg();
	imgFile.seekg(
				0,
				std::ios::beg);

	nframes = static_cast<size_t>(static_cast<int>(datSize) / (_width * _height));

	for (size_t
			i = 0;
			i != nframes;
			++i)
	{
		Surface* surface = new Surface(
									_width,
									_height);
		_frames[i] = surface;
	}

	Uint8 value;
	int
		x = 0,
		y = 0;
	size_t frame = 0;

	_frames[frame]->lock();
	while (imgFile.read(
					(char*)&value,
					1))
	{
		_frames[frame]->setPixelIterative(&x,&y, value);

		if (y >= _height)
		{
			_frames[frame]->unlock();

			++frame;
			x =
			y = 0;

			if (frame >= nframes)
				break;
			else
				_frames[frame]->lock();
		}
	}

	imgFile.close();
}

/**
 * Returns a particular frame from this SurfaceSet.
 * @param i - frame number in the set
 * @return, pointer to the respective Surface
 */
Surface* SurfaceSet::getFrame(const int i)
{
	if (_frames.find(i) != _frames.end())
		return _frames[i];

	return NULL;
}

/**
 * Creates and returns a particular frame in this SurfaceSet.
 * @param i - frame number in the set
 * @return, pointer to the respective surface
 */
Surface* SurfaceSet::addFrame(const int i)
{
	_frames[i] = new Surface(
						_width,
						_height);

	return _frames[i];
}

/**
 * Returns the full width of a frame in this SurfaceSet.
 * @return, width in pixels
 */
int SurfaceSet::getWidth() const
{
	return _width;
}

/**
 * Returns the full height of a frame in this SurfaceSet.
 * @return, height in pixels
 */
int SurfaceSet::getHeight() const
{
	return _height;
}

/**
 * Returns the total amount of frames currently stored in this SurfaceSet.
 * @return, number of frames
 */
size_t SurfaceSet::getTotalFrames() const
{
	return _frames.size();
}

/**
 * Replaces a certain amount of colors in all of the frames.
 * @param colors		- pointer to the set of colors
 * @param firstcolor	- offset of the first color to replace (default 0)
 * @param ncolors		- amount of colors to replace (default 256)
 */
void SurfaceSet::setPalette(
		SDL_Color* colors,
		int firstcolor,
		int ncolors)
{
	for (std::map<int, Surface*>::iterator
			i = _frames.begin();
			i != _frames.end();
			++i)
	{
		(*i).second->setPalette(
							colors,
							firstcolor,
							ncolors);
	}
}

/**
 * Gets a map of the frames in this SurfaceSet.
 * @return, pointer to a map of ints w/ pointers to Surface corresponding to the frames
 */
std::map<int, Surface*>* SurfaceSet::getFrames()
{
	return &_frames;
}

}
