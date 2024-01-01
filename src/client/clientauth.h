/*
Minetest
Copyright (C) 2024 SFENCE, <sfence.software@gmail.com>

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

#include <string>
#include "network/networkprotocol.h"
#include "util/basic_macros.h"

class ClientAuth
{
public:
	ClientAuth();
	ClientAuth(const std::string &player_name, const std::string &password);

	~ClientAuth();
	DISABLE_CLASS_COPY(ClientAuth);

	void moveFrom(ClientAuth& other);

	void applyPassword(const std::string &player_name, const std::string &password);
	
	bool getIsEmpty() const { return m_is_empty; }
	const std::string &getSrpVerifier() const { return m_srp_verifier; }
	const std::string &getSrpSalt() const { return m_srp_salt; }
	void * getLegacyAuthData() const { return m_legacy_auth_data; }
	void * getSrpAuthData() const { return m_srp_auth_data; }
	void * getAuthData(AuthMechanism chosen_auth_mech) const;
	
	void deleteAuthData();
private:
	bool m_is_empty;

	std::string m_srp_verifier;
	std::string m_srp_salt;
	
	void *m_legacy_auth_data = nullptr;
	void *m_srp_auth_data = nullptr;
};
