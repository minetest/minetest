/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#ifndef NODEMETADATAFORMSOURCE_H_
#define NODEMETADATAFORMSOURCE_H_

#include "client/clientmap.h"
#include "gui/guiFormSpecMenu.h"
#include "nodemetadata.h"

class NodeMetadataFormSource: public IFormSource
{
public:
	NodeMetadataFormSource(ClientMap *map, v3s16 p):
		m_map(map),
		m_p(p)
	{
	}

	const std::string &getForm() const
	{
		static const std::string empty_string = "";
		NodeMetadata *meta = m_map->getNodeMetadata(m_p);

		if (!meta)
			return empty_string;

		return meta->getString("formspec");
	}

	virtual std::string resolveText(const std::string &str)
	{
		NodeMetadata *meta = m_map->getNodeMetadata(m_p);

		if (!meta)
			return str;

		return meta->resolveString(str);
	}

	ClientMap *m_map;
	v3s16 m_p;
};

#endif // NODEMETADATAFORMSOURCE_H_
