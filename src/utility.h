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
#include <jthread.h>
#include <jmutex.h>
#include <jmutexautolock.h>

#ifdef _WIN32
	#include <windows.h>
	#define sleep_ms(x) Sleep(x)
#else
	#include <unistd.h>
	#define sleep_ms(x) usleep(x*1000)
#endif

#include "common_irrlicht.h"
#include "debug.h"
#include "strfnd.h"
#include "exceptions.h"

extern const v3s16 g_26dirs[26];

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

// Signed variants of the above

inline void writeS32(u8 *data, s32 i){
	writeU32(data, (u32)i);
}
inline s32 readS32(u8 *data){
	return (s32)readU32(data);
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
	SharedBuffer(unsigned int size)
	{
		m_size = size;
		data = new T[size];
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
		data = new T[size];
		memcpy(data, t, size);
		refcount = new unsigned int;
		(*refcount) = 1;
	}
	/*
		Copies whole buffer
	*/
	SharedBuffer(const Buffer<T> &buffer)
	{
		m_size = buffer.m_size;
		data = new T[buffer.getSize()];
		memcpy(data, *buffer, buffer.getSize());
		refcount = new unsigned int;
		(*refcount) = 1;
	}
	~SharedBuffer()
	{
		drop();
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
	void drop()
	{
		assert((*refcount) > 0);
		(*refcount)--;
		if(*refcount == 0)
		{
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

class IrrlichtWrapper;

class TimeTaker
{
public:
	TimeTaker(const char *name, IrrlichtWrapper *irrlicht, u32 *result=NULL);

	~TimeTaker()
	{
		stop();
	}

	u32 stop(bool quiet=false);

private:
	const char *m_name;
	IrrlichtWrapper *m_irrlicht;
	u32 m_time1;
	bool m_running;
	u32 *m_result;
};

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

inline std::wstring narrow_to_wide(const std::string& mbs)
{
	size_t wcl = mbs.size();
	SharedBuffer<wchar_t> wcs(wcl+1);
	size_t l = mbstowcs(*wcs, mbs.c_str(), wcl);
	wcs[l] = 0;
	return *wcs;
}

inline std::string wide_to_narrow(const std::wstring& wcs)
{
	size_t mbl = wcs.size()*4;
	SharedBuffer<char> mbs(mbl+1);
	size_t l = wcstombs(*mbs, wcs.c_str(), mbl);
	if((int)l == -1)
		mbs[0] = 0;
	else
		mbs[l] = 0;
	return *mbs;
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

inline std::string lowercase(std::string s)
{
	for(size_t i=0; i<s.size(); i++)
	{
		if(s[i] >= 'A' && s[i] <= 'Z')
			s[i] -= 'A' - 'a';
	}
	return s;
}

inline bool is_yes(std::string s)
{
	s = lowercase(trim(s));
	if(s == "y" || s == "yes" || s == "true")
		return true;
	return false;
}

inline s32 stoi(std::string s, s32 min, s32 max)
{
	s32 i = atoi(s.c_str());
	if(i < min)
		i = min;
	if(i > max)
		i = max;
	return i;
}

inline s32 stoi(std::string s)
{
	return atoi(s.c_str());
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
	Config stuff
*/

enum ValueType
{
	VALUETYPE_STRING,
	VALUETYPE_FLAG // Doesn't take any arguments
};

struct ValueSpec
{
	ValueSpec(ValueType a_type, const char *a_help=NULL)
	{
		type = a_type;
		help = a_help;
	}
	ValueType type;
	const char *help;
};

class Settings
{
public:

	// Returns false on EOF
	bool parseConfigObject(std::istream &is)
	{
		if(is.eof())
			return false;
		
		// NOTE: This function will be expanded to allow multi-line settings
		std::string line;
		std::getline(is, line);
		//dstream<<"got line: \""<<line<<"\""<<std::endl;

		std::string trimmedline = trim(line);
		
		// Ignore comments
		if(trimmedline[0] == '#')
			return true;

		//dstream<<"trimmedline=\""<<trimmedline<<"\""<<std::endl;

		Strfnd sf(trim(line));

		std::string name = sf.next("=");
		name = trim(name);

		if(name == "")
			return true;
		
		std::string value = sf.next("\n");
		value = trim(value);

		dstream<<"Config name=\""<<name<<"\" value=\""
				<<value<<"\""<<std::endl;
		
		m_settings[name] = value;
		
		return true;
	}

	/*
		Read configuration file

		Returns true on success
	*/
	bool readConfigFile(const char *filename)
	{
		std::ifstream is(filename);
		if(is.good() == false)
		{
			dstream<<"Error opening configuration file \""
					<<filename<<"\""<<std::endl;
			return false;
		}

		dstream<<"Parsing configuration file: \""
				<<filename<<"\""<<std::endl;
				
		while(parseConfigObject(is));
		
		return true;
	}

	/*
		Reads a configuration object from stream (usually a single line)
		and adds it to dst.
		
		Preserves comments and empty lines.

		Settings that were added to dst are also added to updated.
		key of updated is setting name, value of updated is dummy.

		Returns false on EOF
	*/
	bool getUpdatedConfigObject(std::istream &is,
			core::list<std::string> &dst,
			core::map<std::string, bool> &updated)
	{
		if(is.eof())
			return false;
		
		// NOTE: This function will be expanded to allow multi-line settings
		std::string line;
		std::getline(is, line);

		std::string trimmedline = trim(line);

		std::string line_end = "";
		if(is.eof() == false)
			line_end = "\n";
		
		// Ignore comments
		if(trimmedline[0] == '#')
		{
			dst.push_back(line+line_end);
			return true;
		}

		Strfnd sf(trim(line));

		std::string name = sf.next("=");
		name = trim(name);

		if(name == "")
		{
			dst.push_back(line+line_end);
			return true;
		}
		
		std::string value = sf.next("\n");
		value = trim(value);
		
		if(m_settings.find(name))
		{
			std::string newvalue = m_settings[name];
			
			if(newvalue != value)
			{
				dstream<<"Changing value of \""<<name<<"\" = \""
						<<value<<"\" -> \""<<newvalue<<"\""
						<<std::endl;
			}

			dst.push_back(name + " = " + newvalue + line_end);

			updated[name] = true;
		}
		
		return true;
	}

	/*
		Updates configuration file

		Returns true on success
	*/
	bool updateConfigFile(const char *filename)
	{
		dstream<<"Updating configuration file: \""
				<<filename<<"\""<<std::endl;
		
		core::list<std::string> objects;
		core::map<std::string, bool> updated;
		
		// Read and modify stuff
		{
			std::ifstream is(filename);
			if(is.good() == false)
			{
				dstream<<"Error opening configuration file"
						" for reading: \""
						<<filename<<"\""<<std::endl;
				return false;
			}

			while(getUpdatedConfigObject(is, objects, updated));
		}
		
		// Write stuff back
		{
			std::ofstream os(filename);
			if(os.good() == false)
			{
				dstream<<"Error opening configuration file"
						" for writing: \""
						<<filename<<"\""<<std::endl;
				return false;
			}
			
			/*
				Write updated stuff
			*/
			for(core::list<std::string>::Iterator
					i = objects.begin();
					i != objects.end(); i++)
			{
				os<<(*i);
			}

			/*
				Write stuff that was not already in the file
			*/
			for(core::map<std::string, std::string>::Iterator
					i = m_settings.getIterator();
					i.atEnd() == false; i++)
			{
				if(updated.find(i.getNode()->getKey()))
					continue;
				std::string name = i.getNode()->getKey();
				std::string value = i.getNode()->getValue();
				dstream<<"Adding \""<<name<<"\" = \""<<value<<"\""
						<<std::endl;
				os<<name<<" = "<<value<<"\n";
			}
		}
		
		return true;
	}

	/*
		NOTE: Types of allowed_options are ignored

		returns true on success
	*/
	bool parseCommandLine(int argc, char *argv[],
			core::map<std::string, ValueSpec> &allowed_options)
	{
		int i=1;
		for(;;)
		{
			if(i >= argc)
				break;
			std::string argname = argv[i];
			if(argname.substr(0, 2) != "--")
			{
				dstream<<"Invalid command-line parameter \""
						<<argname<<"\": --<option> expected."<<std::endl;
				return false;
			}
			i++;

			std::string name = argname.substr(2);

			core::map<std::string, ValueSpec>::Node *n;
			n = allowed_options.find(name);
			if(n == NULL)
			{
				dstream<<"Unknown command-line parameter \""
						<<argname<<"\""<<std::endl;
				return false;
			}

			ValueType type = n->getValue().type;

			std::string value = "";
			
			if(type == VALUETYPE_FLAG)
			{
				value = "true";
			}
			else
			{
				if(i >= argc)
				{
					dstream<<"Invalid command-line parameter \""
							<<name<<"\": missing value"<<std::endl;
					return false;
				}
				value = argv[i];
				i++;
			}
			

			dstream<<"Valid command-line parameter: \""
					<<name<<"\" = \""<<value<<"\""
					<<std::endl;
			set(name, value);
		}

		return true;
	}

	void set(std::string name, std::string value)
	{
		m_settings[name] = value;
	}

	void setDefault(std::string name, std::string value)
	{
		m_defaults[name] = value;
	}

	bool exists(std::string name)
	{
		return (m_settings.find(name) || m_defaults.find(name));
	}

	std::string get(std::string name)
	{
		core::map<std::string, std::string>::Node *n;
		n = m_settings.find(name);
		if(n == NULL)
		{
			n = m_defaults.find(name);
			if(n == NULL)
			{
				throw SettingNotFoundException("Setting not found");
			}
		}

		return n->getValue();
	}

	bool getBool(std::string name)
	{
		return is_yes(get(name));
	}
	
	bool getFlag(std::string name)
	{
		try
		{
			return getBool(name);
		}
		catch(SettingNotFoundException &e)
		{
			return false;
		}
	}

	// Asks if empty
	bool getBoolAsk(std::string name, std::string question, bool def)
	{
		std::string s = get(name);
		if(s != "")
			return is_yes(s);
		
		char templine[10];
		std::cout<<question<<" [y/N]: ";
		std::cin.getline(templine, 10);
		s = templine;

		if(s == "")
			return def;

		return is_yes(s);
	}

	float getFloat(std::string name)
	{
		float f;
		std::istringstream vis(get(name));
		vis>>f;
		return f;
	}

	u16 getU16(std::string name)
	{
		return stoi(get(name), 0, 65535);
	}

	u16 getU16Ask(std::string name, std::string question, u16 def)
	{
		std::string s = get(name);
		if(s != "")
			return stoi(s, 0, 65535);
		
		char templine[10];
		std::cout<<question<<" ["<<def<<"]: ";
		std::cin.getline(templine, 10);
		s = templine;

		if(s == "")
			return def;

		return stoi(s, 0, 65535);
	}

	s16 getS16(std::string name)
	{
		return stoi(get(name), -32768, 32767);
	}

	s32 getS32(std::string name)
	{
		return stoi(get(name));
	}

private:
	core::map<std::string, std::string> m_settings;
	core::map<std::string, std::string> m_defaults;
};

/*
	A thread-safe queue
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

/*
	Quickhands for typical request-result queues.
	Used for distributing work between threads.
*/

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

#endif

