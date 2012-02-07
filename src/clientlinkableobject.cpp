/*
Minetest-c55
Copyright (C) 2010-2012 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2012 sapier sapier at gmx dot net

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

#include "clientlinkableobject.h"

ClientLinkableObject::ClientLinkableObject() {

	this->m_Parent = NULL;
}

ClientLinkableObject::~ClientLinkableObject() {
	if (this->isLinked())
		this->unlink(this);
}


void ClientLinkableObject::link(ClientLinkableObject* entity) {
	//TODO check if entity is already linkt (shouldn't be the case but just to be sure)
	this->m_LinkedObjects.push_back(entity);
}

void ClientLinkableObject::unlink(ClientLinkableObject* entity) {
	this->m_LinkedObjects.remove(entity);
}


void ClientLinkableObject::stepLinkedObjects(v3f pos,float dtime) {
	for(std::list<ClientLinkableObject*>::iterator i = this->m_LinkedObjects.begin();
			i != this->m_LinkedObjects.end(); i++) {
			(*i)->setPosition(pos,dtime);
	}
}

bool ClientLinkableObject::handleLinkUnlinkMessages(u8 cmd,std::istringstream* is,ClientEnvironment *m_env) {
	if(cmd == 3) // Link entity
		{
			//Object to link entity to
			u16 object_id = readU16(*is);
			//offset against linked object
			v3f offset    = readV3F1000(*is);

			ClientActiveObject* parent_cao = m_env->getActiveObject(object_id);

			ClientLinkableObject* parent = dynamic_cast<ClientLinkableObject*>(parent_cao);

			if (parent != NULL) {
				this->linkEntity(offset,parent);
			}
			else {
				errorstream << "Invalid object to link to!" << std::endl;
			}
			return true;

		}
	else if(cmd == 4) // UnLink entity
		{
			if (this->m_Parent == NULL) {
				errorstream << "Unlinking object not linked!" << std::endl;
			}

			this->unlinkEntity();
			return true;
		}

	return false;
}


bool ClientLinkableObject::linkEntity(v3f offset, ClientLinkableObject* parent) {
	//already linked unlink first
	if (this->m_Parent != NULL) {
		return false;
	}

	//TODO add linkchain support
	if (this->m_LinkedObjects.size() > 0) {
		return false;
	}

	parent->link(this);
	updateLinkState(true);
	this->m_linkOffset 	= offset;
	this->m_Parent 	 	= parent;
	return true;
}


bool ClientLinkableObject::unlinkEntity() {
	if (this->m_Parent != NULL) {

			updateLinkState(false);
			this->m_Parent->unlink(this);
			this->m_Parent = NULL;
			return true;

		}

		return false;
}

bool ClientLinkableObject::isLinked() {
	if (this->m_Parent != NULL)
		return true;
	else
		return false;
}
