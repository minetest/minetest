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

#include "serverlinkableobject.h"


ServerLinkableObject::ServerLinkableObject() {
	this->m_Linked = false;
}

ServerLinkableObject::~ServerLinkableObject() {}

bool ServerLinkableObject::linkEntity(ServerActiveObject* parent,v3f offset) {
	//check if entity is in correct state
	if (this->m_Linked == true) {
		errorstream<<"ServerLinkableObject: link but object already linked!"<<std::endl;
		return false;
	}
	this->m_Linked = true;

	errorstream<<"ServerLinkableObject: try to send link message!"<<std::endl;
	return sendLinkMsg(parent,offset);
}

bool ServerLinkableObject::unlinkEntity() {
	//check if entity is in correct state
	if (this->m_Linked == false) {
		errorstream<<"ServerLinkableObject: unlink but object not linked!"<<std::endl;
		return false;
	}

	this->m_Linked = false;

	errorstream<<"ServerLinkableObject: try to send unlink message!"<<std::endl;
	return sendUnlinkMsg();
}
