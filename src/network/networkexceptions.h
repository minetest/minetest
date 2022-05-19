/*
Minetest
Copyright (C) 2013-2017 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "exceptions.h"

namespace con
{
/*
	Exceptions
*/
DEFINE_EXCEPTION(NotFoundException);
DEFINE_EXCEPTION(PeerNotFoundException);
DEFINE_EXCEPTION(ConnectionException);
DEFINE_EXCEPTION(ConnectionBindFailed);
DEFINE_EXCEPTION(InvalidIncomingDataException);
DEFINE_EXCEPTION(NoIncomingDataException);
DEFINE_EXCEPTION(ProcessedSilentlyException);
DEFINE_EXCEPTION(ProcessedQueued);
DEFINE_EXCEPTION(IncomingDataCorruption);
} // namespace con

DEFINE_EXCEPTION(SocketException);
DEFINE_EXCEPTION(ResolveError);
DEFINE_EXCEPTION(SendFailedException);
