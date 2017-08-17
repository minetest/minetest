/*
Minetest
Copyright (C) 2015, 2016 est31 <MTest31@outlook.com>

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

/// Gets the base64 encoded legacy password db entry.
std::string translate_password(const std::string &name,
	const std::string &password);

/// Creates a verification key with given salt and password.
std::string generate_srp_verifier(const std::string &name,
	const std::string &password, const std::string &salt);

/// Creates a verification key and salt with given password.
void generate_srp_verifier_and_salt(const std::string &name,
	const std::string &password, std::string *verifier,
	std::string *salt);

/// Gets an SRP verifier, generating a salt,
/// and encodes it as DB-ready string.
std::string get_encoded_srp_verifier(const std::string &name,
	const std::string &password);

/// Converts the passed SRP verifier into a DB-ready format.
std::string encode_srp_verifier(const std::string &verifier,
	const std::string &salt);

/// Reads the DB-formatted SRP verifier and gets the verifier
/// and salt components.
bool decode_srp_verifier_and_salt(const std::string &encoded,
	std::string *verifier, std::string *salt);
