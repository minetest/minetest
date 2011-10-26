/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef UTILITY_HEADER
#define UTILITY_HEADER

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <jthread.h>
#include <jmutex.h>
#include <jmutexautolock.h>
#include <cstring>

#include "common_irrlicht.h"
#include "debug.h"
#include "strfnd.h"
#include "exceptions.h"
#include "porting.h"

extern const v3s16 g_6dirs[6];

extern const v3s16 g_26dirs[26];

// 26th is (0,0,0)
extern const v3s16 g_27dirs[27];

inline void writeU64(u8 *data, u64 i)
{
	data[0] = ((i>>56)&0xff);
	data[1] = ((i>>48)&0xff);
	data[2] = ((i>>40)&0xff);
	data[3] = ((i>>32)&0xff);
	data[4] = ((i>>24)&0xff);
	data[5] = ((i>>16)&0xff);
	data[6] = ((i>> 8)&0xff);
	data[7] = ((i>> 0)&0xff);
}

inline void writeU32(u8 *data, u32 i)
{
	data[0] = ((i>>24)&0xff);
	data[1] = ((i>>16)&0xff);
	data[2] = ((i>> 8)&0xff);
	data[3] = ((i>> 0)&0xff);
}

inline void writeU16(u8 *data, u16 i)
{
	data[0] = ((i>> 8)&0xff);
	data[1] = ((i>> 0)&0xff);
}

inline void writeU8(u8 *data, u8 i)
{
	data[0] = ((i>> 0)&0xff);
}

inline u64 readU64(u8 *data)
{
	return ((u64)data[0]<<56) | ((u64)data[1]<<48)
		| ((u64)data[2]<<40) | ((u64)data[3]<<32)
		| ((u64)data[4]<<24) | ((u64)data[5]<<16)
		| ((u64)data[6]<<8) | ((u64)data[7]<<0);
}

inline u32 readU32(u8 *data)
{
	return (data[0]<<24) | (data[1]<<16) | (data[2]<<8) | (data[3]<<0);
}

inline u16 readU16(u8 *data)
{
	return (data[0]<<8) | (data[1]<<0);
}

inline u8 readU8(u8 *data)
{
	return (data[0]<<0);
}

inline void writeS32(u8 *data, s32 i){
	writeU32(data, (u32)i);
}
inline s32 readS32(u8 *data){
	return (s32)readU32(data);
}

inline void writeF1000(u8 *data, f32 i){
	writeS32(data, i*1000);
}
inline f32 readF1000(u8 *data){
	return (f32)readS32(data)/1000.;
}

inline void writeS16(u8 *data, s16 i){
	writeU16(data, (u16)i);
}
inline s16 readS16(u8 *data){
	return (s16)readU16(data);
}

inline void writeV3S32(u8 *data, v3s32 p)
{
	writeS32(&data[0], p.X);
	writeS32(&data[4], p.Y);
	writeS32(&data[8], p.Z);
}
inline v3s32 readV3S32(u8 *data)
{
	v3s32 p;
	p.X = readS32(&data[0]);
	p.Y = readS32(&data[4]);
	p.Z = readS32(&data[8]);
	return p;
}

inline void writeV3F1000(u8 *data, v3f p)
{
	writeF1000(&data[0], p.X);
	writeF1000(&data[4], p.Y);
	writeF1000(&data[8], p.Z);
}
inline v3f readV3F1000(u8 *data)
{
	v3f p;
	p.X = (float)readF1000(&data[0]);
	p.Y = (float)readF1000(&data[4]);
	p.Z = (float)readF1000(&data[8]);
	return p;
}

inline void writeV2S16(u8 *data, v2s16 p)
{
	writeS16(&data[0], p.X);
	writeS16(&data[2], p.Y);
}

inline v2s16 readV2S16(u8 *data)
{
	v2s16 p;
	p.X = readS16(&data[0]);
	p.Y = readS16(&data[2]);
	return p;
}

inline void writeV2S32(u8 *data, v2s32 p)
{
	writeS32(&data[0], p.X);
	writeS32(&data[2], p.Y);
}

inline v2s32 readV2S32(u8 *data)
{
	v2s32 p;
	p.X = readS32(&data[0]);
	p.Y = readS32(&data[2]);
	return p;
}

inline void writeV3S16(u8 *data, v3s16 p)
{
	writeS16(&data[0], p.X);
	writeS16(&data[2], p.Y);
	writeS16(&data[4], p.Z);
}

inline v3s16 readV3S16(u8 *data)
{
	v3s16 p;
	p.X = readS16(&data[0]);
	p.Y = readS16(&data[2]);
	p.Z = readS16(&data[4]);
	return p;
}

/*
	The above stuff directly interfaced to iostream
*/

inline void writeU8(std::ostream &os, u8 p)
{
	char buf[1];
	writeU8((u8*)buf, p);
	os.write(buf, 1);
}
inline u8 readU8(std::istream &is)
{
	char buf[1];
	is.read(buf, 1);
	return readU8((u8*)buf);
}

inline void writeU16(std::ostream &os, u16 p)
{
	char buf[2];
	writeU16((u8*)buf, p);
	os.write(buf, 2);
}
inline u16 readU16(std::istream &is)
{
	char buf[2];
	is.read(buf, 2);
	return readU16((u8*)buf);
}

inline void writeU32(std::ostream &os, u16 p)
{
	char buf[4];
	writeU16((u8*)buf, p);
	os.write(buf, 4);
}
inline u16 readU32(std::istream &is)
{
	char buf[4];
	is.read(buf, 4);
	return readU32((u8*)buf);
}

inline void writeF1000(std::ostream &os, f32 p)
{
	char buf[4];
	writeF1000((u8*)buf, p);
	os.write(buf, 4);
}
inline f32 readF1000(std::istream &is)
{
	char buf[4];
	is.read(buf, 4);
	return readF1000((u8*)buf);
}

inline void writeV3F1000(std::ostream &os, v3f p)
{
	char buf[12];
	writeV3F1000((u8*)buf, p);
	os.write(buf, 12);
}
inline v3f readV3F1000(std::istream &is)
{
	char buf[12];
	is.read(buf, 12);
	return readV3F1000((u8*)buf);
}

/*
	None of these are used at the moment
*/

template <typename T>
class SharedPtr
{
public:
	SharedPtr(T *t=NULL)
	{
		refcount = new int;
		*refcount = 1;
		ptr = t;
	}
	SharedPtr(SharedPtr<T> &t)
	{
		//*this = t;
		drop();
		refcount = t.refcount;
		(*refcount)++;
		ptr = t.ptr;
	}
	~SharedPtr()
	{
		drop();
	}
	SharedPtr<T> & operator=(T *t)
	{
		drop();
		refcount = new int;
		*refcount = 1;
		ptr = t;
		return *this;
	}
	SharedPtr<T> & operator=(SharedPtr<T> &t)
	{
		drop();
		refcount = t.refcount;
		(*refcount)++;
		ptr = t.ptr;
		return *this;
	}
	T* operator->()
	{
		return ptr;
	}
	T & operator*()
	{
		return *ptr;
	}
	bool operator!=(T *t)
	{
		return ptr != t;
	}
	bool operator==(T *t)
	{
		return ptr == t;
	}
	T & operator[](unsigned int i)
	{
		return ptr[i];
	}
private:
	void drop()
	{
		assert((*refcount) > 0);
		(*refcount)--;
		if(*refcount == 0)
		{
			delete refcount;
			if(ptr != NULL)
				delete ptr;
		}
	}
	T *ptr;
	int *refcount;
};

template <typename T>
class Buffer
{
public:
	Buffer(unsigned int size)
	{
		m_size = size;
		data = new T[size];
	}
	Buffer(const Buffer &buffer)
	{
		m_size = buffer.m_size;
		data = new T[buffer.m_size];
		memcpy(data, buffer.data, buffer.m_size);
	}
	Buffer(T *t, unsigned int size)
	{
		m_size = size;
		data = new T[size];
		memcpy(data, t, size);
	}
	~Buffer()
	{
		delete[] data;
	}
	T & operator[](unsigned int i) const
	{
		return data[i];
	}
	T * operator*() const
	{
		return data;
	}
	unsigned int getSize() const
	{
		return m_size;
	}
private:
	T *data;
	unsigned int m_size;
};

template <typename T>
class SharedBuffer
{
public:
	SharedBuffer()
	{
		m_size = 0;
		data = NULL;
		refcount = new unsigned int;
		(*refcount) = 1;
	}
	SharedBuffer(unsigned int size)
	{
		m_size = size;
		if(m_size != 0)
			data = new T[m_size];
		else
			data = NULL;
		refcount = new unsigned int;
		(*refcount) = 1;
	}
	SharedBuffer(const SharedBuffer &buffer)
	{
		//std::cout<<"SharedBuffer(const SharedBuffer &buffer)"<<std::endl;
		m_size = buffer.m_size;
		data = buffer.data;
		refcount = buffer.refcount;
		(*refcount)++;
	}
	SharedBuffer & operator=(const SharedBuffer & buffer)
	{
		//std::cout<<"SharedBuffer & operator=(const SharedBuffer & buffer)"<<std::endl;
		if(this == &buffer)
			return *this;
		drop();
		m_size = buffer.m_size;
		data = buffer.data;
		refcount = buffer.refcount;
		(*refcount)++;
		return *this;
	}
	/*
		Copies whole buffer
	*/
	SharedBuffer(T *t, unsigned int size)
	{
		m_size = size;
		if(m_size != 0)
		{
			data = new T[m_size];
			memcpy(data, t, m_size);
		}
		else
			data = NULL;
		refcount = new unsigned int;
		(*refcount) = 1;
	}
	/*
		Copies whole buffer
	*/
	SharedBuffer(const Buffer<T> &buffer)
	{
		m_size = buffer.getSize();
		if(m_size != 0)
		{
			data = new T[m_size];
			memcpy(data, *buffer, buffer.getSize());
		}
		else
			data = NULL;
		refcount = new unsigned int;
		(*refcount) = 1;
	}
	~SharedBuffer()
	{
		drop();
	}
	T & operator[](unsigned int i) const
	{
		//assert(i < m_size)
		return data[i];
	}
	T * operator*() const
	{
		return data;
	}
	unsigned int getSize() const
	{
		return m_size;
	}
private:
	void drop()
	{
		assert((*refcount) > 0);
		(*refcount)--;
		if(*refcount == 0)
		{
			if(data)
				delete[] data;
			delete refcount;
		}
	}
	T *data;
	unsigned int m_size;
	unsigned int *refcount;
};

inline SharedBuffer<u8> SharedBufferFromString(const char *string)
{
	SharedBuffer<u8> b((u8*)string, strlen(string)+1);
	return b;
}

template<typename T>
class MutexedVariable
{
public:
	MutexedVariable(T value):
		m_value(value)
	{
		m_mutex.Init();
	}

	T get()
	{
		JMutexAutoLock lock(m_mutex);
		return m_value;
	}

	void set(T value)
	{
		JMutexAutoLock lock(m_mutex);
		m_value = value;
	}
	
	// You'll want to grab this in a SharedPtr
	JMutexAutoLock * getLock()
	{
		return new JMutexAutoLock(m_mutex);
	}
	
	// You pretty surely want to grab the lock when accessing this
	T m_value;

private:
	JMutex m_mutex;
};

/*
	TimeTaker
*/

class TimeTaker
{
public:
	TimeTaker(const char *name, u32 *result=NULL);

	~TimeTaker()
	{
		stop();
	}

	u32 stop(bool quiet=false);

	u32 getTime();

private:
	const char *m_name;
	u32 m_time1;
	bool m_running;
	u32 *m_result;
};

#ifndef SERVER
// Sets the color of all vertices in the mesh
void setMeshVerticesColor(scene::IMesh* mesh, video::SColor& color);
#endif

// Calculates the borders of a "d-radius" cube
inline void getFacePositions(core::list<v3s16> &list, u16 d)
{
	if(d == 0)
	{
		list.push_back(v3s16(0,0,0));
		return;
	}
	if(d == 1)
	{
		/*
			This is an optimized sequence of coordinates.
		*/
		list.push_back(v3s16( 0, 1, 0)); // top
		list.push_back(v3s16( 0, 0, 1)); // back
		list.push_back(v3s16(-1, 0, 0)); // left
		list.push_back(v3s16( 1, 0, 0)); // right
		list.push_back(v3s16( 0, 0,-1)); // front
		list.push_back(v3s16( 0,-1, 0)); // bottom
		// 6
		list.push_back(v3s16(-1, 0, 1)); // back left
		list.push_back(v3s16( 1, 0, 1)); // back right
		list.push_back(v3s16(-1, 0,-1)); // front left
		list.push_back(v3s16( 1, 0,-1)); // front right
		list.push_back(v3s16(-1,-1, 0)); // bottom left
		list.push_back(v3s16( 1,-1, 0)); // bottom right
		list.push_back(v3s16( 0,-1, 1)); // bottom back
		list.push_back(v3s16( 0,-1,-1)); // bottom front
		list.push_back(v3s16(-1, 1, 0)); // top left
		list.push_back(v3s16( 1, 1, 0)); // top right
		list.push_back(v3s16( 0, 1, 1)); // top back
		list.push_back(v3s16( 0, 1,-1)); // top front
		// 18
		list.push_back(v3s16(-1, 1, 1)); // top back-left
		list.push_back(v3s16( 1, 1, 1)); // top back-right
		list.push_back(v3s16(-1, 1,-1)); // top front-left
		list.push_back(v3s16( 1, 1,-1)); // top front-right
		list.push_back(v3s16(-1,-1, 1)); // bottom back-left
		list.push_back(v3s16( 1,-1, 1)); // bottom back-right
		list.push_back(v3s16(-1,-1,-1)); // bottom front-left
		list.push_back(v3s16( 1,-1,-1)); // bottom front-right
		// 26
		return;
	}

	// Take blocks in all sides, starting from y=0 and going +-y
	for(s16 y=0; y<=d-1; y++)
	{
		// Left and right side, including borders
		for(s16 z=-d; z<=d; z++)
		{
			list.push_back(v3s16(d,y,z));
			list.push_back(v3s16(-d,y,z));
			if(y != 0)
			{
				list.push_back(v3s16(d,-y,z));
				list.push_back(v3s16(-d,-y,z));
			}
		}
		// Back and front side, excluding borders
		for(s16 x=-d+1; x<=d-1; x++)
		{
			list.push_back(v3s16(x,y,d));
			list.push_back(v3s16(x,y,-d));
			if(y != 0)
			{
				list.push_back(v3s16(x,-y,d));
				list.push_back(v3s16(x,-y,-d));
			}
		}
	}

	// Take the bottom and top face with borders
	// -d<x<d, y=+-d, -d<z<d
	for(s16 x=-d; x<=d; x++)
	for(s16 z=-d; z<=d; z++)
	{
		list.push_back(v3s16(x,-d,z));
		list.push_back(v3s16(x,d,z));
	}
}

class IndentationRaiser
{
public:
	IndentationRaiser(u16 *indentation)
	{
		m_indentation = indentation;
		(*m_indentation)++;
	}
	~IndentationRaiser()
	{
		(*m_indentation)--;
	}
private:
	u16 *m_indentation;
};

inline s16 getContainerPos(s16 p, s16 d)
{
	return (p>=0 ? p : p-d+1) / d;
}

inline v2s16 getContainerPos(v2s16 p, s16 d)
{
	return v2s16(
		getContainerPos(p.X, d),
		getContainerPos(p.Y, d)
	);
}

inline v3s16 getContainerPos(v3s16 p, s16 d)
{
	return v3s16(
		getContainerPos(p.X, d),
		getContainerPos(p.Y, d),
		getContainerPos(p.Z, d)
	);
}

inline v2s16 getContainerPos(v2s16 p, v2s16 d)
{
	return v2s16(
		getContainerPos(p.X, d.X),
		getContainerPos(p.Y, d.Y)
	);
}

inline v3s16 getContainerPos(v3s16 p, v3s16 d)
{
	return v3s16(
		getContainerPos(p.X, d.X),
		getContainerPos(p.Y, d.Y),
		getContainerPos(p.Z, d.Z)
	);
}

inline bool isInArea(v3s16 p, s16 d)
{
	return (
		p.X >= 0 && p.X < d &&
		p.Y >= 0 && p.Y < d &&
		p.Z >= 0 && p.Z < d
	);
}

inline bool isInArea(v2s16 p, s16 d)
{
	return (
		p.X >= 0 && p.X < d &&
		p.Y >= 0 && p.Y < d
	);
}

inline bool isInArea(v3s16 p, v3s16 d)
{
	return (
		p.X >= 0 && p.X < d.X &&
		p.Y >= 0 && p.Y < d.Y &&
		p.Z >= 0 && p.Z < d.Z
	);
}

inline s16 rangelim(s16 i, s16 max)
{
	if(i < 0)
		return 0;
	if(i > max)
		return max;
	return i;
}

#define rangelim(d, min, max) ((d) < (min) ? (min) : ((d)>(max)?(max):(d)))

inline v3s16 arealim(v3s16 p, s16 d)
{
	if(p.X < 0)
		p.X = 0;
	if(p.Y < 0)
		p.Y = 0;
	if(p.Z < 0)
		p.Z = 0;
	if(p.X > d-1)
		p.X = d-1;
	if(p.Y > d-1)
		p.Y = d-1;
	if(p.Z > d-1)
		p.Z = d-1;
	return p;
}

inline std::wstring narrow_to_wide(const std::string& mbs)
{
	size_t wcl = mbs.size();
	Buffer<wchar_t> wcs(wcl+1);
	size_t l = mbstowcs(*wcs, mbs.c_str(), wcl);
	if(l == (size_t)(-1))
		return L"<invalid multibyte string>";
	wcs[l] = 0;
	return *wcs;
}

inline std::string wide_to_narrow(const std::wstring& wcs)
{
	size_t mbl = wcs.size()*4;
	SharedBuffer<char> mbs(mbl+1);
	size_t l = wcstombs(*mbs, wcs.c_str(), mbl);
	if(l == (size_t)(-1))
		mbs[0] = 0;
	else
		mbs[l] = 0;
	return *mbs;
}

// Split a string using the given delimiter. Returns a vector containing
// the component parts.
inline std::vector<std::wstring> str_split(const std::wstring &str, wchar_t delimiter)
{
	std::vector<std::wstring> parts;
	std::wstringstream sstr(str);
	std::wstring part;
	while(std::getline(sstr, part, delimiter))
		parts.push_back(part);
	return parts;
}


/*
	See test.cpp for example cases.
	wraps degrees to the range of -360...360
	NOTE: Wrapping to 0...360 is not used because pitch needs negative values.
*/
inline float wrapDegrees(float f)
{
	// Take examples of f=10, f=720.5, f=-0.5, f=-360.5
	// This results in
	// 10, 720, -1, -361
	int i = floor(f);
	// 0, 2, 0, -1
	int l = i / 360;
	// NOTE: This would be used for wrapping to 0...360
	// 0, 2, -1, -2
	/*if(i < 0)
		l -= 1;*/
	// 0, 720, 0, -360
	int k = l * 360;
	// 10, 0.5, -0.5, -0.5
	f -= float(k);
	return f;
}

/* Wrap to 0...360 */
inline float wrapDegrees_0_360(float f)
{
	// Take examples of f=10, f=720.5, f=-0.5, f=-360.5
	// This results in
	// 10, 720, -1, -361
	int i = floor(f);
	// 0, 2, 0, -1
	int l = i / 360;
	// Wrap to 0...360
	// 0, 2, -1, -2
	if(i < 0)
		l -= 1;
	// 0, 720, 0, -360
	int k = l * 360;
	// 10, 0.5, -0.5, -0.5
	f -= float(k);
	return f;
}

/* Wrap to -180...180 */
inline float wrapDegrees_180(float f)
{
	f += 180;
	f = wrapDegrees_0_360(f);
	f -= 180;
	return f;
}

inline std::string lowercase(const std::string &s)
{
	std::string s2;
	for(size_t i=0; i<s.size(); i++)
	{
		char c = s[i];
		if(c >= 'A' && c <= 'Z')
			c -= 'A' - 'a';
		s2 += c;
	}
	return s2;
}

inline bool is_yes(const std::string &s)
{
	std::string s2 = lowercase(trim(s));
	if(s2 == "y" || s2 == "yes" || s2 == "true" || s2 == "1")
		return true;
	return false;
}

inline s32 stoi(const std::string &s, s32 min, s32 max)
{
	s32 i = atoi(s.c_str());
	if(i < min)
		i = min;
	if(i > max)
		i = max;
	return i;
}


// MSVC2010 includes it's own versions of these
#if !defined(_MSC_VER) || _MSC_VER < 1600

inline s32 stoi(std::string s)
{
	return atoi(s.c_str());
}

inline s32 stoi(std::wstring s)
{
	return atoi(wide_to_narrow(s).c_str());
}

inline float stof(std::string s)
{
	float f;
	std::istringstream ss(s);
	ss>>f;
	return f;
}

#endif

inline std::string itos(s32 i)
{
	std::ostringstream o;
	o<<i;
	return o.str();
}

inline std::string ftos(float f)
{
	std::ostringstream o;
	o<<f;
	return o.str();
}

inline void str_replace(std::string & str, std::string const & pattern,
		std::string const & replacement)
{
	std::string::size_type start = str.find(pattern, 0);
	while(start != str.npos)
	{
		str.replace(start, pattern.size(), replacement);
		start = str.find(pattern, start+replacement.size());
	}
}

inline void str_replace_char(std::string & str, char from, char to)
{
	for(unsigned int i=0; i<str.size(); i++)
	{
		if(str[i] == from)
			str[i] = to;
	}
}

/*
	A base class for simple background thread implementation
*/

class SimpleThread : public JThread
{
	bool run;
	JMutex run_mutex;

public:

	SimpleThread():
		JThread(),
		run(true)
	{
		run_mutex.Init();
	}

	virtual ~SimpleThread()
	{}

	virtual void * Thread() = 0;

	bool getRun()
	{
		JMutexAutoLock lock(run_mutex);
		return run;
	}
	void setRun(bool a_run)
	{
		JMutexAutoLock lock(run_mutex);
		run = a_run;
	}

	void stop()
	{
		setRun(false);
		while(IsRunning())
			sleep_ms(100);
	}
};

/*
	FIFO queue (well, actually a FILO also)
*/
template<typename T>
class Queue
{
public:
	void push_back(T t)
	{
		m_list.push_back(t);
	}
	
	T pop_front()
	{
		if(m_list.size() == 0)
			throw ItemNotFoundException("Queue: queue is empty");

		typename core::list<T>::Iterator begin = m_list.begin();
		T t = *begin;
		m_list.erase(begin);
		return t;
	}
	T pop_back()
	{
		if(m_list.size() == 0)
			throw ItemNotFoundException("Queue: queue is empty");

		typename core::list<T>::Iterator last = m_list.getLast();
		T t = *last;
		m_list.erase(last);
		return t;
	}

	u32 size()
	{
		return m_list.size();
	}

protected:
	core::list<T> m_list;
};

/*
	Thread-safe FIFO queue (well, actually a FILO also)
*/

template<typename T>
class MutexedQueue
{
public:
	MutexedQueue()
	{
		m_mutex.Init();
	}
	u32 size()
	{
		JMutexAutoLock lock(m_mutex);
		return m_list.size();
	}
	void push_back(T t)
	{
		JMutexAutoLock lock(m_mutex);
		m_list.push_back(t);
	}
	T pop_front(u32 wait_time_max_ms=0)
	{
		u32 wait_time_ms = 0;

		for(;;)
		{
			{
				JMutexAutoLock lock(m_mutex);

				if(m_list.size() > 0)
				{
					typename core::list<T>::Iterator begin = m_list.begin();
					T t = *begin;
					m_list.erase(begin);
					return t;
				}

				if(wait_time_ms >= wait_time_max_ms)
					throw ItemNotFoundException("MutexedQueue: queue is empty");
			}

			// Wait a while before trying again
			sleep_ms(10);
			wait_time_ms += 10;
		}
	}
	T pop_back(u32 wait_time_max_ms=0)
	{
		u32 wait_time_ms = 0;

		for(;;)
		{
			{
				JMutexAutoLock lock(m_mutex);

				if(m_list.size() > 0)
				{
					typename core::list<T>::Iterator last = m_list.getLast();
					T t = *last;
					m_list.erase(last);
					return t;
				}

				if(wait_time_ms >= wait_time_max_ms)
					throw ItemNotFoundException("MutexedQueue: queue is empty");
			}

			// Wait a while before trying again
			sleep_ms(10);
			wait_time_ms += 10;
		}
	}

	JMutex & getMutex()
	{
		return m_mutex;
	}

	core::list<T> & getList()
	{
		return m_list;
	}

protected:
	JMutex m_mutex;
	core::list<T> m_list;
};

/*
	A single worker thread - multiple client threads queue framework.
*/

template<typename Caller, typename Data>
class CallerInfo
{
public:
	Caller caller;
	Data data;
};

template<typename Key, typename T, typename Caller, typename CallerData>
class GetResult
{
public:
	Key key;
	T item;
	core::list<CallerInfo<Caller, CallerData> > callers;
};

template<typename Key, typename T, typename Caller, typename CallerData>
class ResultQueue: public MutexedQueue< GetResult<Key, T, Caller, CallerData> >
{
};

template<typename Key, typename T, typename Caller, typename CallerData>
class GetRequest
{
public:
	GetRequest()
	{
		dest = NULL;
	}
	GetRequest(ResultQueue<Key,T, Caller, CallerData> *a_dest)
	{
		dest = a_dest;
	}
	GetRequest(ResultQueue<Key,T, Caller, CallerData> *a_dest,
			Key a_key)
	{
		dest = a_dest;
		key = a_key;
	}
	~GetRequest()
	{
	}
	
	Key key;
	ResultQueue<Key, T, Caller, CallerData> *dest;
	core::list<CallerInfo<Caller, CallerData> > callers;
};

template<typename Key, typename T, typename Caller, typename CallerData>
class RequestQueue
{
public:
	u32 size()
	{
		return m_queue.size();
	}

	void add(Key key, Caller caller, CallerData callerdata,
			ResultQueue<Key, T, Caller, CallerData> *dest)
	{
		JMutexAutoLock lock(m_queue.getMutex());
		
		/*
			If the caller is already on the list, only update CallerData
		*/
		for(typename core::list< GetRequest<Key, T, Caller, CallerData> >::Iterator
				i = m_queue.getList().begin();
				i != m_queue.getList().end(); i++)
		{
			GetRequest<Key, T, Caller, CallerData> &request = *i;

			if(request.key == key)
			{
				for(typename core::list< CallerInfo<Caller, CallerData> >::Iterator
						i = request.callers.begin();
						i != request.callers.end(); i++)
				{
					CallerInfo<Caller, CallerData> &ca = *i;
					if(ca.caller == caller)
					{
						ca.data = callerdata;
						return;
					}
				}
				CallerInfo<Caller, CallerData> ca;
				ca.caller = caller;
				ca.data = callerdata;
				request.callers.push_back(ca);
				return;
			}
		}

		/*
			Else add a new request to the queue
		*/

		GetRequest<Key, T, Caller, CallerData> request;
		request.key = key;
		CallerInfo<Caller, CallerData> ca;
		ca.caller = caller;
		ca.data = callerdata;
		request.callers.push_back(ca);
		request.dest = dest;
		
		m_queue.getList().push_back(request);
	}

	GetRequest<Key, T, Caller, CallerData> pop(bool wait_if_empty=false)
	{
		return m_queue.pop_front(wait_if_empty);
	}

private:
	MutexedQueue< GetRequest<Key, T, Caller, CallerData> > m_queue;
};

/*
	Pseudo-random (VC++ rand() sucks)
*/
int myrand(void);
void mysrand(unsigned seed);
#define MYRAND_MAX 32767

int myrand_range(int min, int max);

/*
	Miscellaneous functions
*/

bool isBlockInSight(v3s16 blockpos_b, v3f camera_pos, v3f camera_dir,
		f32 camera_fov, f32 range, f32 *distance_ptr=NULL);

/*
	Queue with unique values with fast checking of value existence
*/

template<typename Value>
class UniqueQueue
{
public:
	
	/*
		Does nothing if value is already queued.
		Return value:
			true: value added
			false: value already exists
	*/
	bool push_back(Value value)
	{
		// Check if already exists
		if(m_map.find(value) != NULL)
			return false;

		// Add
		m_map.insert(value, 0);
		m_list.push_back(value);
		
		return true;
	}

	Value pop_front()
	{
		typename core::list<Value>::Iterator i = m_list.begin();
		Value value = *i;
		m_map.remove(value);
		m_list.erase(i);
		return value;
	}

	u32 size()
	{
		assert(m_list.size() == m_map.size());
		return m_list.size();
	}

private:
	core::map<Value, u8> m_map;
	core::list<Value> m_list;
};

#if 1
template<typename Key, typename Value>
class MutexedMap
{
public:
	MutexedMap()
	{
		m_mutex.Init();
		assert(m_mutex.IsInitialized());
	}
	
	void set(const Key &name, const Value &value)
	{
		JMutexAutoLock lock(m_mutex);

		m_values[name] = value;
	}
	
	bool get(const Key &name, Value *result)
	{
		JMutexAutoLock lock(m_mutex);

		typename core::map<Key, Value>::Node *n;
		n = m_values.find(name);

		if(n == NULL)
			return false;
		
		if(result != NULL)
			*result = n->getValue();
			
		return true;
	}

private:
	core::map<Key, Value> m_values;
	JMutex m_mutex;
};
#endif

/*
	Generates ids for comparable values.
	Id=0 is reserved for "no value".

	Is fast at:
	- Returning value by id (very fast)
	- Returning id by value
	- Generating a new id for a value

	Is not able to:
	- Remove an id/value pair (is possible to implement but slow)
*/
template<typename T>
class MutexedIdGenerator
{
public:
	MutexedIdGenerator()
	{
		m_mutex.Init();
		assert(m_mutex.IsInitialized());
	}
	
	// Returns true if found
	bool getValue(u32 id, T &value)
	{
		if(id == 0)
			return false;
		JMutexAutoLock lock(m_mutex);
		if(m_id_to_value.size() < id)
			return false;
		value = m_id_to_value[id-1];
		return true;
	}
	
	// If id exists for value, returns the id.
	// Otherwise generates an id for the value.
	u32 getId(const T &value)
	{
		JMutexAutoLock lock(m_mutex);
		typename core::map<T, u32>::Node *n;
		n = m_value_to_id.find(value);
		if(n != NULL)
			return n->getValue();
		m_id_to_value.push_back(value);
		u32 new_id = m_id_to_value.size();
		m_value_to_id.insert(value, new_id);
		return new_id;
	}

private:
	JMutex m_mutex;
	// Values are stored here at id-1 position (id 1 = [0])
	core::array<T> m_id_to_value;
	core::map<T, u32> m_value_to_id;
};

/*
	Checks if a string contains only supplied characters
*/
inline bool string_allowed(const std::string &s, const std::string &allowed_chars)
{
	for(u32 i=0; i<s.size(); i++)
	{
		bool confirmed = false;
		for(u32 j=0; j<allowed_chars.size(); j++)
		{
			if(s[i] == allowed_chars[j])
			{
				confirmed = true;
				break;
			}
		}
		if(confirmed == false)
			return false;
	}
	return true;
}

/*
	Forcefully wraps string into rows using \n
	(no word wrap, used for showing paths in gui)
*/
inline std::string wrap_rows(const std::string &from, u32 rowlen)
{
	std::string to;
	for(u32 i=0; i<from.size(); i++)
	{
		if(i != 0 && i%rowlen == 0)
			to += '\n';
		to += from[i];
	}
	return to;
}

/*
	Some helper stuff
*/
#define MYMIN(a,b) ((a)<(b)?(a):(b))
#define MYMAX(a,b) ((a)>(b)?(a):(b))

/*
	Returns integer position of node in given floating point position
*/
inline v3s16 floatToInt(v3f p, f32 d)
{
	v3s16 p2(
		(p.X + (p.X>0 ? d/2 : -d/2))/d,
		(p.Y + (p.Y>0 ? d/2 : -d/2))/d,
		(p.Z + (p.Z>0 ? d/2 : -d/2))/d);
	return p2;
}

/*
	Returns floating point position of node in given integer position
*/
inline v3f intToFloat(v3s16 p, f32 d)
{
	v3f p2(
		(f32)p.X * d,
		(f32)p.Y * d,
		(f32)p.Z * d
	);
	return p2;
}

/*
	More serialization stuff
*/

// Creates a string with the length as the first two bytes
inline std::string serializeString(const std::string &plain)
{
	//assert(plain.size() <= 65535);
	if(plain.size() > 65535)
		throw SerializationError("String too long for serializeString");
	char buf[2];
	writeU16((u8*)&buf[0], plain.size());
	std::string s;
	s.append(buf, 2);
	s.append(plain);
	return s;
}

// Creates a string with the length as the first two bytes from wide string
inline std::string serializeWideString(const std::wstring &plain)
{
	//assert(plain.size() <= 65535);
	if(plain.size() > 65535)
		throw SerializationError("String too long for serializeString");
	char buf[2];
	writeU16((u8*)buf, plain.size());
	std::string s;
	s.append(buf, 2);
	for(u32 i=0; i<plain.size(); i++)
	{
		writeU16((u8*)buf, plain[i]);
		s.append(buf, 2);
	}
	return s;
}

// Reads a string with the length as the first two bytes
inline std::string deSerializeString(std::istream &is)
{
	char buf[2];
	is.read(buf, 2);
	if(is.gcount() != 2)
		throw SerializationError("deSerializeString: size not read");
	u16 s_size = readU16((u8*)buf);
	if(s_size == 0)
		return "";
	Buffer<char> buf2(s_size);
	is.read(&buf2[0], s_size);
	std::string s;
	s.reserve(s_size);
	s.append(&buf2[0], s_size);
	return s;
}

// Reads a wide string with the length as the first two bytes
inline std::wstring deSerializeWideString(std::istream &is)
{
	char buf[2];
	is.read(buf, 2);
	if(is.gcount() != 2)
		throw SerializationError("deSerializeString: size not read");
	u16 s_size = readU16((u8*)buf);
	if(s_size == 0)
		return L"";
	std::wstring s;
	s.reserve(s_size);
	for(u32 i=0; i<s_size; i++)
	{
		is.read(&buf[0], 2);
		wchar_t c16 = readU16((u8*)buf);
		s.append(&c16, 1);
	}
	return s;
}

// Creates a string with the length as the first four bytes
inline std::string serializeLongString(const std::string &plain)
{
	char buf[4];
	writeU32((u8*)&buf[0], plain.size());
	std::string s;
	s.append(buf, 4);
	s.append(plain);
	return s;
}

// Reads a string with the length as the first four bytes
inline std::string deSerializeLongString(std::istream &is)
{
	char buf[4];
	is.read(buf, 4);
	if(is.gcount() != 4)
		throw SerializationError("deSerializeLongString: size not read");
	u32 s_size = readU32((u8*)buf);
	if(s_size == 0)
		return "";
	Buffer<char> buf2(s_size);
	is.read(&buf2[0], s_size);
	std::string s;
	s.reserve(s_size);
	s.append(&buf2[0], s_size);
	return s;
}

//

inline u32 time_to_daynight_ratio(u32 time_of_day)
{
	const s32 daylength = 16;
	const s32 nightlength = 6;
	const s32 daytimelength = 8;
	s32 d = daylength;
	s32 t = (((time_of_day)%24000)/(24000/d));
	if(t < nightlength/2 || t >= d - nightlength/2)
		//return 300;
		return 350;
	else if(t >= d/2 - daytimelength/2 && t < d/2 + daytimelength/2)
		return 1000;
	else
		return 750;
}

// Random helper. Usually d=BS
inline core::aabbox3d<f32> getNodeBox(v3s16 p, float d)
{
	return core::aabbox3d<f32>(
		(float)p.X * d - 0.5*d,
		(float)p.Y * d - 0.5*d,
		(float)p.Z * d - 0.5*d,
		(float)p.X * d + 0.5*d,
		(float)p.Y * d + 0.5*d,
		(float)p.Z * d + 0.5*d
	);
}
	
class IntervalLimiter
{
public:
	IntervalLimiter():
		m_accumulator(0)
	{
	}
	/*
		dtime: time from last call to this method
		wanted_interval: interval wanted
		return value:
			true: action should be skipped
			false: action should be done
	*/
	bool step(float dtime, float wanted_interval)
	{
		m_accumulator += dtime;
		if(m_accumulator < wanted_interval)
			return false;
		m_accumulator -= wanted_interval;
		return true;
	}
protected:
	float m_accumulator;
};

std::string translatePassword(std::string playername, std::wstring password);

#endif

