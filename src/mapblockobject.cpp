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

#include "mapblockobject.h"
#include "mapblock.h"
// Only for ::getNodeBox, TODO: Get rid of this
#include "map.h"

/*
	MapBlockObject
*/

// This is here because it uses the MapBlock
v3f MapBlockObject::getAbsolutePos()
{
	if(m_block == NULL)
		return m_pos;
	
	// getPosRelative gets nodepos relative to map origin
	v3f blockpos = intToFloat(m_block->getPosRelative());
	return blockpos + m_pos;
}

void MapBlockObject::setBlockChanged()
{
	if(m_block)
		m_block->setChangedFlag();
}

/*
	MovingObject
*/
void MovingObject::move(float dtime, v3f acceleration)
{
	DSTACK("%s: typeid=%i, pos=(%f,%f,%f), speed=(%f,%f,%f)"
			", dtime=%f, acc=(%f,%f,%f)",
			__FUNCTION_NAME,
			getTypeId(),
			m_pos.X, m_pos.Y, m_pos.Z,
			m_speed.X, m_speed.Y, m_speed.Z,
			dtime,
			acceleration.X, acceleration.Y, acceleration.Z
			);
	
	v3s16 oldpos_i = floatToInt(m_pos);
	
	if(m_block->isValidPosition(oldpos_i) == false)
	{
		// Should have wrapped, cancelling further movement.
		return;
	}

	// No collisions if there is no collision box
	if(m_collision_box == NULL)
	{
		m_speed += dtime * acceleration;
		m_pos += m_speed * dtime;
		return;
	}
	
	// Set insane speed to zero
	// Otherwise there will be divides by zero and other silly stuff
	if(m_speed.getLength() > 1000.0*BS)
		m_speed = v3f(0,0,0);
		
	// Limit speed to a reasonable value
	float speed_limit = 20.0*BS;
	if(m_speed.getLength() > speed_limit)
		m_speed = m_speed * (speed_limit / m_speed.getLength());

	v3f position = m_pos;
	v3f oldpos = position;

	/*std::cout<<"oldpos_i=("<<oldpos_i.X<<","<<oldpos_i.Y<<","
			<<oldpos_i.Z<<")"<<std::endl;*/

	// Maximum time increment (for collision detection etc)
	// Allow 0.1 blocks per increment
	// time = distance / speed
	// NOTE: In the loop below collisions are detected at 0.15*BS radius
	float speedlength = m_speed.getLength();
	f32 dtime_max_increment;
	if(fabs(speedlength) > 0.001)
		dtime_max_increment = 0.1*BS / speedlength;
	else
		dtime_max_increment = 0.5;
	
	m_touching_ground = false;
		
	u32 loopcount = 0;
	do
	{
		loopcount++;

		f32 dtime_part;
		if(dtime > dtime_max_increment)
			dtime_part = dtime_max_increment;
		else
			dtime_part = dtime;
		dtime -= dtime_part;

		// Begin of dtime limited code
		
		m_speed += acceleration * dtime_part;
		position += m_speed * dtime_part;

		/*
			Collision detection
		*/
		
		v3s16 pos_i = floatToInt(position);
		
		// The loop length is limited to the object moving a distance
		f32 d = (float)BS * 0.15;

		core::aabbox3d<f32> objectbox(
				m_collision_box->MinEdge + position,
				m_collision_box->MaxEdge + position
		);
		
		core::aabbox3d<f32> objectbox_old(
				m_collision_box->MinEdge + oldpos,
				m_collision_box->MaxEdge + oldpos
		);
		
		//TODO: Get these ranges from somewhere
		for(s16 y = oldpos_i.Y - 1; y <= oldpos_i.Y + 2; y++)
		for(s16 z = oldpos_i.Z - 1; z <= oldpos_i.Z + 1; z++)
		for(s16 x = oldpos_i.X - 1; x <= oldpos_i.X + 1; x++)
		{
			try{
				if(content_walkable(m_block->getNodeParent(v3s16(x,y,z)).d)
						== false)
					continue;
			}
			catch(InvalidPositionException &e)
			{
				// Doing nothing here will block the player from
				// walking over map borders
			}

			core::aabbox3d<f32> nodebox = Map::getNodeBox(
					v3s16(x,y,z));
			
			// See if the player is touching ground
			if(
					fabs(nodebox.MaxEdge.Y-objectbox.MinEdge.Y) < d
					&& nodebox.MaxEdge.X-d > objectbox.MinEdge.X
					&& nodebox.MinEdge.X+d < objectbox.MaxEdge.X
					&& nodebox.MaxEdge.Z-d > objectbox.MinEdge.Z
					&& nodebox.MinEdge.Z+d < objectbox.MaxEdge.Z
			){
				m_touching_ground = true;
			}
			
			if(objectbox.intersectsWithBox(nodebox))
			{
					
		v3f dirs[3] = {
			v3f(0,0,1), // back
			v3f(0,1,0), // top
			v3f(1,0,0), // right
		};
		for(u16 i=0; i<3; i++)
		{
			f32 nodemax = nodebox.MaxEdge.dotProduct(dirs[i]);
			f32 nodemin = nodebox.MinEdge.dotProduct(dirs[i]);
			f32 playermax = objectbox.MaxEdge.dotProduct(dirs[i]);
			f32 playermin = objectbox.MinEdge.dotProduct(dirs[i]);
			f32 playermax_old = objectbox_old.MaxEdge.dotProduct(dirs[i]);
			f32 playermin_old = objectbox_old.MinEdge.dotProduct(dirs[i]);

			bool main_edge_collides = 
				((nodemax > playermin && nodemax <= playermin_old + d
					&& m_speed.dotProduct(dirs[i]) < 0)
				||
				(nodemin < playermax && nodemin >= playermax_old - d
					&& m_speed.dotProduct(dirs[i]) > 0));

			bool other_edges_collide = true;
			for(u16 j=0; j<3; j++)
			{
				if(j == i)
					continue;
				f32 nodemax = nodebox.MaxEdge.dotProduct(dirs[j]);
				f32 nodemin = nodebox.MinEdge.dotProduct(dirs[j]);
				f32 playermax = objectbox.MaxEdge.dotProduct(dirs[j]);
				f32 playermin = objectbox.MinEdge.dotProduct(dirs[j]);
				if(!(nodemax - d > playermin && nodemin + d < playermax))
				{
					other_edges_collide = false;
					break;
				}
			}
			
			if(main_edge_collides && other_edges_collide)
			{
				m_speed -= m_speed.dotProduct(dirs[i]) * dirs[i];
				position -= position.dotProduct(dirs[i]) * dirs[i];
				position += oldpos.dotProduct(dirs[i]) * dirs[i];
			}
		
		}
		
			} // if(objectbox.intersectsWithBox(nodebox))
		} // for y

	} // End of dtime limited loop
	while(dtime > 0.001);

	m_pos = position;
}

/*
	MapBlockObjectList
*/

MapBlockObjectList::MapBlockObjectList(MapBlock *block):
	m_block(block)
{
	m_mutex.Init();
}

MapBlockObjectList::~MapBlockObjectList()
{
	clear();
}

/*
	The serialization format:
	[0] u16 number of entries
	[2] entries (id, typeId, parameters)
*/

void MapBlockObjectList::serialize(std::ostream &os, u8 version)
{
	JMutexAutoLock lock(m_mutex);

	u8 buf[2];
	writeU16(buf, m_objects.size());
	os.write((char*)buf, 2);

	for(core::map<s16, MapBlockObject*>::Iterator
			i = m_objects.getIterator();
			i.atEnd() == false; i++)
	{
		i.getNode()->getValue()->serialize(os, version);
	}
}

void MapBlockObjectList::update(std::istream &is, u8 version,
		scene::ISceneManager *smgr)
{
	JMutexAutoLock lock(m_mutex);

	/*
		Collect all existing ids to a set.

		As things are updated, they are removed from this.

		All remaining ones are deleted.
	*/
	core::map<s16, bool> ids_to_delete;
	for(core::map<s16, MapBlockObject*>::Iterator
			i = m_objects.getIterator();
			i.atEnd() == false; i++)
	{
		ids_to_delete.insert(i.getNode()->getKey(), true);
	}
	
	u8 buf[6];
	
	is.read((char*)buf, 2);
	u16 count = readU16(buf);

	for(u16 i=0; i<count; i++)
	{
		// Read id
		is.read((char*)buf, 2);
		s16 id = readS16(buf);
		
		// Read position
		// stored as x1000/BS v3s16
		is.read((char*)buf, 6);
		v3s16 pos_i = readV3S16(buf);
		v3f pos((f32)pos_i.X/1000*BS,
				(f32)pos_i.Y/1000*BS,
				(f32)pos_i.Z/1000*BS);

		// Read typeId
		is.read((char*)buf, 2);
		u16 type_id = readU16(buf);
		
		bool create_new = false;

		// Find an object with the id
		core::map<s16, MapBlockObject*>::Node *n;
		n = m_objects.find(id);
		// If no entry is found for id
		if(n == NULL)
		{
			// Insert dummy pointer node
			m_objects.insert(id, NULL);
			// Get node
			n = m_objects.find(id);
			// A new object will be created at this node
			create_new = true;
		}
		// If type_id differs
		else if(n->getValue()->getTypeId() != type_id)
		{
			// Delete old object
			delete n->getValue();
			// A new object will be created at this node
			create_new = true;
		}

		MapBlockObject *obj = NULL;

		if(create_new)
		{
			/*dstream<<"MapBlockObjectList adding new object"
					" id="<<id
					<<std::endl;*/

			if(type_id == MAPBLOCKOBJECT_TYPE_TEST)
			{
				// The constructors of objects shouldn't need
				// any more parameters than this.
				obj = new TestObject(m_block, id, pos);
			}
			else if(type_id == MAPBLOCKOBJECT_TYPE_TEST2)
			{
				obj = new Test2Object(m_block, id, pos);
			}
			else if(type_id == MAPBLOCKOBJECT_TYPE_SIGN)
			{
				obj = new SignObject(m_block, id, pos);
			}
			else if(type_id == MAPBLOCKOBJECT_TYPE_RAT)
			{
				obj = new RatObject(m_block, id, pos);
			}
			else
			{
				throw SerializationError
				("MapBlockObjectList::update(): Unknown MapBlockObject type");
			}

			if(smgr != NULL)
				obj->addToScene(smgr);

			n->setValue(obj);
		}
		else
		{
			obj = n->getValue();
			obj->updatePos(pos);
		}

		// Now there is an object in obj.
		// Update it.
		
		obj->update(is, version);
		
		// Remove from deletion list
		if(ids_to_delete.find(id) != NULL)
			ids_to_delete.remove(id);
	}

	// Delete all objects whose ids_to_delete remain in ids_to_delete
	for(core::map<s16, bool>::Iterator
			i = ids_to_delete.getIterator();
			i.atEnd() == false; i++)
	{
		s16 id = i.getNode()->getKey();

		/*dstream<<"MapBlockObjectList deleting object"
				" id="<<id
				<<std::endl;*/

		MapBlockObject *obj = m_objects[id];
		obj->removeFromScene();
		delete obj;
		m_objects.remove(id);
	}
}

s16 MapBlockObjectList::getFreeId() throw(ContainerFullException)
{
	s16 id = 0;
	for(;;)
	{
		if(m_objects.find(id) == NULL)
			return id;
		if(id == 32767)
			throw ContainerFullException
					("MapBlockObjectList doesn't fit more objects");
		id++;
	}
}

void MapBlockObjectList::add(MapBlockObject *object)
		throw(ContainerFullException, AlreadyExistsException)
{
	if(object == NULL)
	{
		dstream<<"MapBlockObjectList::add(): NULL object"<<std::endl;
		return;
	}

	JMutexAutoLock lock(m_mutex);

	// Create unique id if id==-1
	if(object->m_id == -1)
	{
		object->m_id = getFreeId();
	}

	if(m_objects.find(object->m_id) != NULL)
	{
		dstream<<"MapBlockObjectList::add(): "
				"object with same id already exists"<<std::endl;
		throw AlreadyExistsException
				("MapBlockObjectList already has given id");
	}
	
	object->m_block = m_block;
	
	/*v3f p = object->m_pos;
	dstream<<"MapBlockObjectList::add(): "
			<<"m_block->getPos()=("
			<<m_block->getPos().X<<","
			<<m_block->getPos().Y<<","
			<<m_block->getPos().Z<<")"
			<<" inserting object with id="<<object->m_id
			<<" pos="
			<<"("<<p.X<<","<<p.Y<<","<<p.Z<<")"
			<<std::endl;*/
	
	m_objects.insert(object->m_id, object);
}

void MapBlockObjectList::clear()
{
	JMutexAutoLock lock(m_mutex);

	for(core::map<s16, MapBlockObject*>::Iterator
			i = m_objects.getIterator();
			i.atEnd() == false; i++)
	{
		MapBlockObject *obj = i.getNode()->getValue();
		//FIXME: This really shouldn't be NULL at any time,
		//       but this condition was added because it was.
		if(obj != NULL)
		{
			obj->removeFromScene();
			delete obj;
		}
	}

	m_objects.clear();
}

void MapBlockObjectList::remove(s16 id)
{
	JMutexAutoLock lock(m_mutex);

	core::map<s16, MapBlockObject*>::Node *n;
	n = m_objects.find(id);
	if(n == NULL)
		return;
	
	n->getValue()->removeFromScene();
	delete n->getValue();
	m_objects.remove(id);
}

MapBlockObject * MapBlockObjectList::get(s16 id)
{
	core::map<s16, MapBlockObject*>::Node *n;
	n = m_objects.find(id);
	if(n == NULL)
		return NULL;
	else
		return n->getValue();
}

void MapBlockObjectList::step(float dtime, bool server)
{
	DSTACK(__FUNCTION_NAME);
	
	JMutexAutoLock lock(m_mutex);
	
	core::map<s16, bool> ids_to_delete;

	{
		DSTACK("%s: stepping objects", __FUNCTION_NAME);

		for(core::map<s16, MapBlockObject*>::Iterator
				i = m_objects.getIterator();
				i.atEnd() == false; i++)
		{
			MapBlockObject *obj = i.getNode()->getValue();
			
			DSTACK("%s: stepping object type %i", __FUNCTION_NAME,
					obj->getTypeId());

			if(server)
			{
				bool to_delete = obj->serverStep(dtime);

				if(to_delete)
					ids_to_delete.insert(obj->m_id, true);
			}
			else
			{
				obj->clientStep(dtime);
			}
		}
	}

	{
		DSTACK("%s: deleting objects", __FUNCTION_NAME);

		// Delete objects in delete queue
		for(core::map<s16, bool>::Iterator
				i = ids_to_delete.getIterator();
				i.atEnd() == false; i++)
		{
			s16 id = i.getNode()->getKey();

			MapBlockObject *obj = m_objects[id];
			obj->removeFromScene();
			delete obj;
			m_objects.remove(id);
		}
	}
	
	/*
		Wrap objects on server
	*/

	if(server == false)
		return;
	
	{
		DSTACK("%s: object wrap loop", __FUNCTION_NAME);

		for(core::map<s16, MapBlockObject*>::Iterator
				i = m_objects.getIterator();
				i.atEnd() == false; i++)
		{
			MapBlockObject *obj = i.getNode()->getValue();

			v3s16 pos_i = floatToInt(obj->m_pos);

			if(m_block->isValidPosition(pos_i))
			{
				// No wrap
				continue;
			}

			bool impossible = wrapObject(obj);

			if(impossible)
			{
				// No wrap
				continue;
			}

			// Restart find
			i = m_objects.getIterator();
		}
	}
}

bool MapBlockObjectList::wrapObject(MapBlockObject *object)
{
	DSTACK(__FUNCTION_NAME);
	
	// No lock here; this is called so that the lock is already locked.
	//JMutexAutoLock lock(m_mutex);

	assert(object->m_block == m_block);
	assert(m_objects.find(object->m_id) != NULL);
	assert(m_objects[object->m_id] == object);

	NodeContainer *parentcontainer = m_block->getParent();
	// This will only work if the parent is the map
	if(parentcontainer->nodeContainerId() != NODECONTAINER_ID_MAP)
	{
		dstream<<"WARNING: Wrapping object not possible: "
				"MapBlock's parent is not map"<<std::endl;
		return true;
	}
	// OK, we have the map!
	Map *map = (Map*)parentcontainer;
	
	// Calculate blockpos on map
	v3s16 oldblock_pos_i_on_map = m_block->getPosRelative();
	v3f pos_f_on_oldblock = object->m_pos;
	v3s16 pos_i_on_oldblock = floatToInt(pos_f_on_oldblock);
	v3s16 pos_i_on_map = pos_i_on_oldblock + oldblock_pos_i_on_map;
	v3s16 pos_blocks_on_map = getNodeBlockPos(pos_i_on_map);

	// Get new block
	MapBlock *newblock;
	try{
		newblock = map->getBlockNoCreate(pos_blocks_on_map);
	}
	catch(InvalidPositionException &e)
	{
		// Couldn't find block -> not wrapping
		/*dstream<<"WARNING: Wrapping object not possible: "
				<<"could not find new block"
				<<"("<<pos_blocks_on_map.X
				<<","<<pos_blocks_on_map.Y
				<<","<<pos_blocks_on_map.Z
				<<")"<<std::endl;*/
		/*dstream<<"pos_f_on_oldblock=("
				<<pos_f_on_oldblock.X<<","
				<<pos_f_on_oldblock.Y<<","
				<<pos_f_on_oldblock.Z<<")"
				<<std::endl;*/
		return true;
	}

	if(newblock == m_block)
	{
		dstream<<"WARNING: Wrapping object not possible: "
				"newblock == oldblock"<<std::endl;
		return true;
	}
	
	// Calculate position on new block
	v3f oldblock_pos_f_on_map = intToFloat(oldblock_pos_i_on_map);
	v3s16 newblock_pos_i_on_map = newblock->getPosRelative();
	v3f newblock_pos_f_on_map = intToFloat(newblock_pos_i_on_map);
	v3f pos_f_on_newblock = pos_f_on_oldblock
			- newblock_pos_f_on_map + oldblock_pos_f_on_map;

	// Remove object from this block
	m_objects.remove(object->m_id);
	
	// Add object to new block
	object->m_pos = pos_f_on_newblock;
	object->m_id = -1;
	object->m_block = NULL;
	newblock->addObject(object);

	//dstream<<"NOTE: Wrapped object"<<std::endl;

	return false;
}

void MapBlockObjectList::getObjects(v3f origin, f32 max_d,
		core::array<DistanceSortedObject> &dest)
{
	for(core::map<s16, MapBlockObject*>::Iterator
			i = m_objects.getIterator();
			i.atEnd() == false; i++)
	{
		MapBlockObject *obj = i.getNode()->getValue();

		f32 d = (obj->m_pos - origin).getLength();

		if(d > max_d)
			continue;

		DistanceSortedObject dso(obj, d);

		dest.push_back(dso);
	}
}

//END
