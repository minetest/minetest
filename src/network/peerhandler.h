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

#pragma once

#include "networkprotocol.h"

namespace con
{

class IPeer;

class PeerHandler
{
public:
	PeerHandler() = default;
	virtual ~PeerHandler() = default;

	// Note: all functions are called from within a Receive() call on the same thread.

	/*
		This is called after the Peer has been inserted into the
		Connection's peer container.
	*/
	virtual void peerAdded(IPeer *peer) = 0;

	/*
		This is called before the Peer has been removed from the
		Connection's peer container.
	*/
	virtual void deletingPeer(IPeer *peer, bool timeout) = 0;
};

}
