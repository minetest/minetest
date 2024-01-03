/*
Minetest
Copyright (C) 2022 DS
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
OpenAL support based on work by:
Copyright (C) 2011 Sebastian 'Bahamada' Rühl
Copyright (C) 2011 Cyriaque 'Cisoun' Skrapits <cysoun@gmail.com>
Copyright (C) 2011 Giuseppe Bilotta <giuseppe.bilotta@gmail.com>

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

#include "al_helpers.h"

namespace sound {

/*
 * RAIIALSoundBuffer
 */

RAIIALSoundBuffer &RAIIALSoundBuffer::operator=(RAIIALSoundBuffer &&other) noexcept
{
	if (&other != this)
		reset(other.release());
	return *this;
}

void RAIIALSoundBuffer::reset(ALuint buf) noexcept
{
	if (m_buffer != 0) {
		alDeleteBuffers(1, &m_buffer);
		warn_if_al_error("Failed to free sound buffer");
	}

	m_buffer = buf;
}

RAIIALSoundBuffer RAIIALSoundBuffer::generate() noexcept
{
	ALuint buf;
	alGenBuffers(1, &buf);
	return RAIIALSoundBuffer(buf);
}

} // namespace sound
