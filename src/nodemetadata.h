/*
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef NODEMETADATA_HEADER
#define NODEMETADATA_HEADER

#include "common_irrlicht.h"
#include <string>
#include <iostream>

/*
	Used for storing:

	Oven:
		- Item that is being burned
		- Burning time
		- Item stack that is being heated
		- Result item stack
	
	Sign:
		- Text
*/

class Inventory;

class NodeMetadata
{
public:
	typedef NodeMetadata* (*Factory)(std::istream&);

	NodeMetadata();
	virtual ~NodeMetadata();
	
	static NodeMetadata* deSerialize(std::istream &is);
	void serialize(std::ostream &os);
	
	// This usually is the CONTENT_ value
	virtual u16 typeId() const = 0;
	virtual NodeMetadata* clone() = 0;
	virtual void serializeBody(std::ostream &os) = 0;
	virtual std::string infoText() {return "<todo: remove this text>";}
	virtual Inventory* getInventory() {return NULL;}
	// This is called always after the inventory is modified, before
	// the changes are copied elsewhere
	virtual void inventoryModified(){}
	// A step in time. Returns true if metadata changed.
	virtual bool step(float dtime) {return false;}
	virtual bool nodeRemovalDisabled(){return false;}

protected:
	static void registerType(u16 id, Factory f);
private:
	static core::map<u16, Factory> m_types;
};

class SignNodeMetadata : public NodeMetadata
{
public:
	SignNodeMetadata(std::string text);
	//~SignNodeMetadata();
	
	virtual u16 typeId() const;
	static NodeMetadata* create(std::istream &is);
	virtual NodeMetadata* clone();
	virtual void serializeBody(std::ostream &os);
	virtual std::string infoText();

	std::string getText(){ return m_text; }
	void setText(std::string t){ m_text = t; }

private:
	std::string m_text;
};

class ChestNodeMetadata : public NodeMetadata
{
public:
	ChestNodeMetadata();
	~ChestNodeMetadata();
	
	virtual u16 typeId() const;
	static NodeMetadata* create(std::istream &is);
	virtual NodeMetadata* clone();
	virtual void serializeBody(std::ostream &os);
	virtual std::string infoText();
	virtual Inventory* getInventory() {return m_inventory;}

	virtual bool nodeRemovalDisabled();
	
private:
	Inventory *m_inventory;
};

class FurnaceNodeMetadata : public NodeMetadata
{
public:
	FurnaceNodeMetadata();
	~FurnaceNodeMetadata();
	
	virtual u16 typeId() const;
	virtual NodeMetadata* clone();
	static NodeMetadata* create(std::istream &is);
	virtual void serializeBody(std::ostream &os);
	virtual std::string infoText();
	virtual Inventory* getInventory() {return m_inventory;}
	virtual void inventoryModified();
	virtual bool step(float dtime);

private:
	Inventory *m_inventory;
	float m_step_accumulator;
	float m_fuel_totaltime;
	float m_fuel_time;
	float m_src_totaltime;
	float m_src_time;
};

/*
	List of metadata of all the nodes of a block
*/

class InventoryManager;

class NodeMetadataList
{
public:
	~NodeMetadataList();

	void serialize(std::ostream &os);
	void deSerialize(std::istream &is);
	
	// Get pointer to data
	NodeMetadata* get(v3s16 p);
	// Deletes data
	void remove(v3s16 p);
	// Deletes old data and sets a new one
	void set(v3s16 p, NodeMetadata *d);
	
	// A step in time. Returns true if something changed.
	bool step(float dtime);

private:
	core::map<v3s16, NodeMetadata*> m_data;
};

#endif

