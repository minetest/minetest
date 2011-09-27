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

#ifndef CONTENT_NODEMETA_HEADER
#define CONTENT_NODEMETA_HEADER

#include "nodemetadata.h"

class Inventory;

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
	virtual std::string getInventoryDrawSpecString();
	
private:
	Inventory *m_inventory;
};

class LockingChestNodeMetadata : public NodeMetadata
{
public:
	LockingChestNodeMetadata();
	~LockingChestNodeMetadata();

	virtual u16 typeId() const;
	static NodeMetadata* create(std::istream &is);
	virtual NodeMetadata* clone();
	virtual void serializeBody(std::ostream &os);
	virtual std::string infoText();
	virtual Inventory* getInventory() {return m_inventory;}
	virtual bool nodeRemovalDisabled();
	virtual std::string getInventoryDrawSpecString();

	virtual std::string getOwner(){ return m_text; }
	virtual void setOwner(std::string t){ m_text = t; }

private:
	Inventory *m_inventory;
	std::string m_text;
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
	virtual bool nodeRemovalDisabled();
	virtual std::string getInventoryDrawSpecString();

private:
	Inventory *m_inventory;
	float m_step_accumulator;
	float m_fuel_totaltime;
	float m_fuel_time;
	float m_src_totaltime;
	float m_src_time;
};


#endif

