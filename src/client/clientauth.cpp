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

#include "clientauth.h"
#include "util/auth.h"
#include "util/srp.h"

ClientAuth::ClientAuth() :
		m_is_empty(true),
		m_srp_verifier(""),
		m_srp_salt("")
{
}

ClientAuth::ClientAuth(const std::string &player_name, const std::string &password)
{
	applyPassword(player_name, password);
}

ClientAuth::~ClientAuth()
{
	clear();
}

ClientAuth &ClientAuth::operator=(ClientAuth &&other)
{
	clear();

	m_is_empty = other.m_is_empty;

	m_srp_verifier = other.m_srp_verifier;
	m_srp_salt = other.m_srp_salt;

	m_legacy_auth_data = other.m_legacy_auth_data;
	m_srp_auth_data = other.m_srp_auth_data;

	other.m_legacy_auth_data = nullptr;
	other.m_srp_auth_data = nullptr;

	other.clear();

	return *this;
}

void ClientAuth::applyPassword(const std::string &player_name, const std::string &password)
{
	clear();
	// AUTH_MECHANISM_FIRST_SRP
	generate_srp_verifier_and_salt(player_name, password, &m_srp_verifier, &m_srp_salt);
	m_is_empty = password.empty();
	
	std::string player_name_u = lowercase(player_name);
	// AUTH_MECHANISM_SRP
	m_srp_auth_data = srp_user_new(SRP_SHA256, SRP_NG_2048,
			player_name.c_str(), player_name_u.c_str(),
			reinterpret_cast<const unsigned char *>(password.c_str()),
			password.length(), NULL, NULL);
	// AUTH_MECHANISM_LEGACY_PASSWORD
	std::string translated = translate_password(player_name, password);
	m_legacy_auth_data = srp_user_new(SRP_SHA256, SRP_NG_2048,
			player_name.c_str(), player_name_u.c_str(),
			reinterpret_cast<const unsigned char *>(translated.c_str()),
			translated.length(), NULL, NULL);
}

void * ClientAuth::getAuthData(AuthMechanism chosen_auth_mech) const
{
	switch (chosen_auth_mech) {
		case AUTH_MECHANISM_LEGACY_PASSWORD:
			return m_legacy_auth_data;
		case AUTH_MECHANISM_SRP:
			return m_srp_auth_data;
		default:
			return nullptr;
	}
}

void ClientAuth::clear()
{
	if (m_legacy_auth_data != nullptr) {
		srp_user_delete(m_legacy_auth_data);
		m_legacy_auth_data = nullptr;
	}
	if (m_srp_auth_data != nullptr) {
		srp_user_delete(m_srp_auth_data);
		m_srp_auth_data = nullptr;
	}
	m_srp_verifier.clear();
	m_srp_salt.clear();
}
