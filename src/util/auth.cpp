/*
Minetest
Copyright (C) 2015 est31 <MTest31@outlook.com>

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

#include <algorithm>
#include <string>
#include "auth.h"
#include "base64.h"
#include "sha1.h"
#include "srp.h"
#include "string.h"
#include "debug.h"

// Get an sha-1 hash of the player's name combined with
// the password entered. That's what the server uses as
// their password. (Exception : if the password field is
// blank, we send a blank password - this is for backwards
// compatibility with password-less players).
std::string translatePassword(const std::string &name,
	const std::string &password)
{
	if (password.length() == 0)
		return "";

	std::string slt = name + password;
	SHA1 sha1;
	sha1.addBytes(slt.c_str(), slt.length());
	unsigned char *digest = sha1.getDigest();
	std::string pwd = base64_encode(digest, 20);
	free(digest);
	return pwd;
}

void getSRPVerifier(const std::string &name,
	const std::string &password, char **salt, size_t *salt_len,
	char **bytes_v, size_t *len_v)
{
	std::string n_name = lowercase(name);
	SRP_Result res = srp_create_salted_verification_key(SRP_SHA256, SRP_NG_2048,
		n_name.c_str(), (const unsigned char *)password.c_str(),
		password.size(), (unsigned char **)salt, salt_len,
		(unsigned char **)bytes_v, len_v, NULL, NULL);
	FATAL_ERROR_IF(res != SRP_OK, "Couldn't create salted SRP verifier");
}

// Get a db-ready SRP verifier
// If the salt param is NULL, one is automatically generated.
// Please free() it afterwards. You shouldn't use it for other purposes,
// as you will need the contents of salt_len too.
inline static std::string getSRPVerifier(const std::string &name,
	const std::string &password, char ** salt, size_t salt_len)
{
	char * bytes_v = NULL;
	size_t len_v;
	getSRPVerifier(name, password, salt, &salt_len,
		&bytes_v, &len_v);
	assert(*salt); // usually, srp_create_salted_verification_key promises us to return SRP_ERR when *salt == NULL
	std::string ret_val = encodeSRPVerifier(std::string(bytes_v, len_v),
		std::string(*salt, salt_len));
	free(bytes_v);
	return ret_val;
}

// Get a db-ready SRP verifier
std::string getSRPVerifier(const std::string &name,
	const std::string &password)
{
	char * salt = NULL;
	std::string ret_val = getSRPVerifier(name,
		password, &salt, 0);
	free(salt);
	return ret_val;
}

// Get a db-ready SRP verifier
std::string getSRPVerifier(const std::string &name,
	const std::string &password, const std::string &salt)
{
	// The implementation won't change the salt if its set,
	// therefore we can cast.
	char *salt_cstr = (char *)salt.c_str();
	return getSRPVerifier(name, password,
		&salt_cstr, salt.size());
}

// Make a SRP verifier db-ready
std::string encodeSRPVerifier(const std::string &verifier,
	const std::string &salt)
{
	std::ostringstream ret_str;
	ret_str << "#1#"
		<< base64_encode((unsigned char*) salt.c_str(), salt.size()) << "#"
		<< base64_encode((unsigned char*) verifier.c_str(), verifier.size());
	return ret_str.str();
}

bool decodeSRPVerifier(const std::string &enc_pwd,
	std::string *salt, std::string *bytes_v)
{
	std::vector<std::string> pwd_components = str_split(enc_pwd, '#');

	if ((pwd_components.size() != 4)
			|| (pwd_components[1] != "1") // 1 means srp
			|| !base64_is_valid(pwd_components[2])
			|| !base64_is_valid(pwd_components[3]))
		return false;

	std::string salt_str = base64_decode(pwd_components[2]);
	std::string bytes_v_str = base64_decode(pwd_components[3]);
	*salt = salt_str;
	*bytes_v = bytes_v_str;
	return true;

}
