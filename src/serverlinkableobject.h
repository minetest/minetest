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

#ifndef SERVERLINKABLEOBJECT_H_
#define SERVERLINKABLEOBJECT_H_

#include <sstream>
#include <irrlichttypes.h>
#include "serverobject.h"
#include "content_object.h"
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

class ServerLinkableObject {
	public:
		ServerLinkableObject();
		~ServerLinkableObject();

		bool linkEntity(ServerActiveObject* parent,v3f offset);
		bool unlinkEntity();

		virtual bool sendLinkMsg(ServerActiveObject* parent,v3f offset) = 0;
		virtual bool sendUnlinkMsg() = 0;

	protected:
		inline bool isLinked() { return m_Linked; }

	private:
		bool m_Linked;

};

#endif /* SERVERLINKABLEOBJECT_H_ */
