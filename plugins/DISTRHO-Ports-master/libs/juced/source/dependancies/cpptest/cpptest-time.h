// ---
//
// $Id: cpptest-time.h,v 1.3 2005/06/08 08:08:06 nilu Exp $
//
// CppTest - A C++ Unit Testing Framework
// Copyright (c) 2003 Niklas Lundell
//
// ---
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the
// Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA 02111-1307, USA.
//
// ---

/** \file */

#ifndef CPPTEST_TIME_H
#define CPPTEST_TIME_H

namespace Test
{
	/// \brief %Time representation.
	///
	/// Encapsulates a time value with microsecond resolution. It is possible
	/// to retrieve the current time, add and subtract time values, and output
	/// the time to an output stream.
	///
	class Time
	{
	public:
		Time();
		Time(unsigned int sec, unsigned int usec);
		
		unsigned int seconds() const;
		unsigned int microseconds() const;
		
		static Time current();
		
		friend Time operator+(const Time& t1, const Time& t2);
		friend Time operator-(const Time& t1, const Time& t2);
		
		friend std::ostream& operator<<(std::ostream& os, const Time& t);
		
	private:
		unsigned int _sec;
		unsigned int _usec;
	};
	
} // namespace Test

#endif // #ifndef CPPTEST_TIME_H
