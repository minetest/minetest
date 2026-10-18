// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#include "activeobject.h"
#include "util/serialize.h"
#include "itemstackmetadata.h"
#include "network/networkprotocol.h"

void ActiveObjectMessage::appendTo(std::string &data) const
{
	appendTo(data, LATEST_PROTOCOL_VERSION);
}

void ActiveObjectMessage::appendTo(std::string &data, u16 protocol_version) const
{
	const std::string *payload = &datastring;
	if (m_legacy_datastring && !nodeModifiersSupportForProtocol(protocol_version))
		payload = &*m_legacy_datastring;

	char idbuf[2];
	writeU16((u8*) idbuf, id);
	data.append(idbuf, sizeof(idbuf));
	data.append(serializeString16(*payload));
}
