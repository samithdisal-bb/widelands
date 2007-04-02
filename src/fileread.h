/*
 * Copyright (C) 2006-2007 by the Widelands Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef FILEREAD_H
#define FILEREAD_H

#include "machdep.h"
#include <limits>
#include <cassert>
#include <string>

struct FileSystem;

/**
 * FileRead can be used to read a file.
 * It works quite naively by reading the entire file into memory.
 * Convenience functions are available for endian-safe access of common data types
 */
struct FileRead {
	typedef size_t Pos;
	static Pos NoPos() throw () {return std::numeric_limits<size_t>::max();}

	struct FileRead_Exception {};
	struct File_Boundary_Exceeded : public FileRead_Exception {};
	struct Buffer_Overflow        : public FileRead_Exception {};

	FileRead (); /// Create the object with nothing to read.
	~FileRead(); /// Close the file if open.

	/**
	 * Loads a file into memory.
	 * Reserves one additional byte which is zeroed, so that text files can
	 * be handled like a normal C string.
	 * Throws an exception if the file couldn't be loaded for whatever reason.
	 */
	void    Open(FileSystem &, const char * const filename);

	/// Works just like Open, but returns false when the load fails.
	bool TryOpen(FileSystem &, const char * const filename);

	void Close(); /// Frees allocated memory.

	size_t GetSize() const throw () {return length;}
	bool IsEOF() const throw () {return length <= filepos;}

	/**
	 * Set the file pointer to the given location.
	 * Raises File_Boundary_Exceeded when the pointer is out of bound.
	 */
	void SetFilePos(const Pos pos);

	/**
	 * Get the position that will be read from in the next read operation that
	 * does not specify a position.
	 */
	Pos GetPos() const throw () {return filepos;}

	/**
	 * Get the position that was read from in the previous read operation that
	 * did not specify a position.
	 */
	Pos GetPrevPos() const throw () {return prevpos;}

	Sint8    Signed8 (const Pos pos = NoPos())
	{return                                       Deref8    (Data(1, pos));}
	Uint8  Unsigned8 (const Pos pos = NoPos())
	{return static_cast<const Uint8>             (Deref8    (Data(1, pos)));}
	Sint16   Signed16(const Pos pos = NoPos())
	{return                           Little16   (Deref16   (Data(2, pos)));}
	Uint16 Unsigned16(const Pos pos = NoPos())
	{return static_cast<const Uint16>(Little16   (Deref16   (Data(2, pos))));}
	Sint32   Signed32(const Pos pos = NoPos())
	{return                           Little32   (Deref32   (Data(4, pos)));}
	Uint32 Unsigned32(const Pos pos = NoPos())
	{return static_cast<const Uint32>(Little32   (Deref32   (Data(4, pos))));}
	float       Float(const Pos pos = NoPos())
	{return                           LittleFloat(DerefFloat(Data(4, pos)));}
	char * CString(const Pos pos = NoPos()); /// Read a zero-terminated string.

	/**
	 * This function copies characters from the file to the memory starting at
	 * buf until it encounters a newline. The newline is not copied. The
	 * characters including the newline are consumed.
	 *
	 * This function will not write to the memory at buf_end or beyond.
	 *
	 *  Requirement: buf < buf_end
	 */
	bool ReadLine(char *buf, const char * const buf_end);

	void * Data(const uint bytes, const Pos pos = NoPos()) {
			assert(data);

			Pos i = pos;
			if (pos == NoPos()) {
				prevpos = i = filepos;
				filepos += bytes;
			}
		if (i+bytes > length) throw File_Boundary_Exceeded();

			return static_cast<char * const>(data) + i;
		}

	void * data;
private:
	Pos    filepos;
	Pos    prevpos;
	size_t length;

	FileRead & operator=(const FileRead &);
	FileRead            (const FileRead &);
};

#endif
