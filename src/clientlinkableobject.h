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

#ifndef CLIENTLINKABLEOBJECT_H_
#define CLIENTLINKABLEOBJECT_H_

#include <list>
#include <sstream>
#include <irrlichttypes.h>
#include "clientobject.h"
#include "environment.h"
#include "content_object.h"
#include "utility.h"
#include "log.h"


//this ain't the right place to define this but until cao/sao split
//is decided it'll have to stay here
struct AO_Message_type {
	static const u8 SetPosition      = 0x00;
	static const u8 SetTextureMod    = 0x01;
	static const u8 SetSprite        = 0x02;
	static const u8 Punched          = 0x03;
	static const u8 TakeDamage       = 0x04;
	static const u8 Shoot            = 0x05;
	static const u8 Link             = 0x06;
	static const u8 UnLink           = 0x07;
};


class ClientLinkableObject {
	public:
		ClientLinkableObject();
		~ClientLinkableObject();
		//internal communication between entitys NOT to be used by user
		void link(ClientLinkableObject* entity);
		void unlink(ClientLinkableObject* entity);

		virtual void setPosition(v3f toset, float dtime) = 0;
		virtual void updateLinkState(bool value) = 0;

	protected:
		void stepLinkedObjects(v3f pos,float dtime);

		bool handleLinkUnlinkMessages(u8 cmd,std::istringstream* is,ClientEnvironment *m_env);


		//user driven functions (exported by lua)
		bool linkEntity(v3f offset, ClientLinkableObject* parent);
		bool unlinkEntity();

		bool isLinked();
		v3f  m_linkOffset;


	private:
		ClientLinkableObject* m_Parent;

		std::list<ClientLinkableObject*> m_LinkedObjects;
};


#endif /* CLIENTLINKABLEOBJECT_H_ */
