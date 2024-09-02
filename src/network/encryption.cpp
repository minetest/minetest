/*
Minetest
Copyright (C) 2024 red-001 <red-001@outlook.ie>

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

#include "encryption.h"
#include "settings.h"
#include "util/base64.h"
#include "porting.h"
#include <openssl/evp.h>
#include "Hacl_HMAC.h"
#include "Hacl_HKDF.h"
#include "EverCrypt_Curve25519.h"
#include "EverCrypt_Chacha20Poly1305.h"
#include <optional>

namespace NetworkEncryption
{
	bool derive_subkeys(const ECDHEKeyPair& our_keys,
		const ECDHEPublicKey& other_pub_key,
		AESChannelKeys&  client_send_keys,
		AESChannelKeys&  server_send_keys,
		HandshakeDigest& handshake_digest)
	{
		u8 shared_secret[NET_ECDHE_SECRET_LEN] = {};
		if (!ecdh_calculate_shared_secret(our_keys, other_pub_key, shared_secret))
		{
			errorstream << "derive_subkeys(): calculate shared secret" << std::endl;
			return false;
		}

		u8 root_key[32] = {};
		hkdf_extract_sha256(shared_secret, sizeof(shared_secret), root_key);

		hkdf_expand_sha256(
			root_key,
			"minetest-client-channel-send-key",
			&client_send_keys.keys[0][0],
			CHANNEL_COUNT * NET_AES_KEY_SIZE);

		hkdf_expand_sha256(
			root_key,
			"minetest-server-channel-send-key",
			&server_send_keys.keys[0][0],
			CHANNEL_COUNT * NET_AES_KEY_SIZE);

		hkdf_expand_sha256(
			root_key,
			"minetest-handshake-digest-for-srp",
			handshake_digest.digest,
			sizeof(handshake_digest.digest));

#ifndef NDEBUG
		if (g_settings->getBool("secure.dump_network_encryption_key"))
		{

			infostream << "network root key: ";
			for (u8 byte : root_key)
				infostream << std::hex << u32(byte) << " ";
			infostream << std::endl;

			infostream << "Handshake digest: ";
			for (u8 byte : handshake_digest.digest)
				infostream << std::hex << u32(byte) << " ";
			infostream << std::endl;

			infostream << "client keys" << std::endl;
			for (auto& client_key : client_send_keys.keys)
			{
				infostream << "key: ";
				for (u8 byte : client_key)
					infostream << std::hex << u32(byte) << " ";
				infostream << std::endl;
			}

			infostream << "server keys" << std::endl;
			for (auto& server_key : server_send_keys.keys)
			{
				infostream << "key: ";
				for (u8 byte : server_key)
					infostream << std::hex << u32(byte) << " ";
				infostream << std::endl;
			}
		}
#endif

		memset(root_key, 0xFF, sizeof(root_key));

		return true;
	}

	void get_identity_for_srp(NetworkEncryption::HandshakeDigest handshake_digest,
		std::string_view name,
		std::string& identity_value_out) {
		u8 hmac_output[32] = {};
		hmac_sha256(
			handshake_digest.digest,
			sizeof(handshake_digest.digest),
			reinterpret_cast<const u8*>(name.data()), name.size(),
			hmac_output);

		std::string_view hmac_output_view = std::string_view(reinterpret_cast<char*>(hmac_output), sizeof(hmac_output));

		std::stringstream ss;
		ss << name;
		ss << ":";
		ss << base64_encode(hmac_output_view);

		identity_value_out = ss.str();
	}

	/*
	* Everything below this point is code for interfacing with HACL*
	*/

#pragma region HACL_IMPL
	void hmac_sha256(const u8* key, size_t key_length, const u8* msg, size_t msg_length, u8(&output)[32]) {
		Hacl_HMAC_compute_sha2_256(output, const_cast<u8*>(key), key_length, const_cast<u8*>(msg), msg_length);
	}

	void hkdf_extract_sha256(const u8* data, size_t length, u8(&output)[32], const std::string_view& salt) {
		Hacl_HKDF_extract_sha2_256(const_cast<u8*>(data), (u8*)salt.data(), salt.size(), output, sizeof(output));
	}

	void hkdf_expand_sha256(const u8(&input)[32], const std::string_view& info, u8* output, size_t length) {
		Hacl_HKDF_expand_sha2_256(output, const_cast<u8*>(input), sizeof(input), (u8*)info.data(), info.size(), length);
	}


	[[nodiscard]] bool ecdh_calculate_shared_secret(const ECDHEKeyPair& our_keys,
		const ECDHEPublicKey& other_pub_key,
		u8(&shared_secret)[NET_ECDHE_SECRET_LEN])
	{
		if (!EverCrypt_Curve25519_ecdh(shared_secret, const_cast<u8*>(our_keys.private_key), const_cast<u8*>(other_pub_key.key))) {
			memset(shared_secret, 0, sizeof(shared_secret));
			return false;
		}
		return true;
	}

	bool encrypt_aes_128_gcm(const u8(&key)[16], const u8(&iv)[12], const Buffer<u8>& plaintext, u8 *encrypted_data, size_t encrypted_buf_length)
	{
		assert(encrypted_buf_length == size_t(plaintext.getSize()) + NET_AAED_TAG_SIZE);
		assert(plaintext.getSize() < std::numeric_limits<int>::max());

		if (encrypted_buf_length < NET_AAED_TAG_SIZE ||
			encrypted_buf_length < size_t(plaintext.getSize()) + NET_AAED_TAG_SIZE ||
			plaintext.getSize() >= std::numeric_limits<int>::max())
		{
			errorstream << "encrypt_aes_128_gcm(): invalid paramters!" << std::endl;
			return false;
		}

		std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)> ctx = { EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free };

		if (!ctx || 1 != EVP_EncryptInit_ex(ctx.get(), EVP_aes_128_gcm(), NULL, key, iv))
		{
			errorstream << "encrypt_aes_128_gcm(): failed to setup encryption ctx!" << std::endl;
			return false;
		}

		int encrypted_len = encrypted_buf_length - NET_AAED_TAG_SIZE;
		if (1 != EVP_EncryptUpdate(ctx.get(), encrypted_data, &encrypted_len, &plaintext[0], plaintext.getSize()) ||
			static_cast<int>(plaintext.getSize()) != encrypted_len)
		{
			errorstream << "encrypt_aes_128_gcm(): failed encrypt data!" << std::endl;
			return false;
		}

		int final_len = {};
		if (1 != EVP_EncryptFinal_ex(ctx.get(), &encrypted_data[encrypted_len], &final_len) ||
			final_len != 0)
		{
			errorstream << "encrypt_aes_128_gcm(): failed finalize data encryption!" << std::endl;
			return false;
		}

		if (1 != EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_GET_TAG, NET_AAED_TAG_SIZE, &encrypted_data[encrypted_len]))
		{
			errorstream << "encrypt_aes_128_gcm(): get AD tag!" << std::endl;
			return false;
		}

		return true;
	}

	bool decrypt_aes_128_gcm(const u8(&key)[16], const u8(&iv)[12], Buffer<u8> &encrypted_data)
	{
		std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)> ctx = { EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free };

		if (!ctx || 1 != EVP_DecryptInit_ex(ctx.get(), EVP_aes_128_gcm(), NULL, key, iv))
		{
			errorstream << "decrypt_aes_128_gcm(): failed to setup encryption ctx!" << std::endl;
			return false;
		}

		size_t total_length = encrypted_data.getSize();

		if (total_length <= NET_AAED_TAG_SIZE)
		{
			errorstream << "decrypt_aes_128_gcm(): data is too short to even fit the tag!" << std::endl;
			return false;
		}

		size_t length_without_tag = total_length - NET_AAED_TAG_SIZE;

		u8 AAD[NET_AAED_TAG_SIZE] = {};
		memcpy(&AAD[0], &encrypted_data[total_length - NET_AAED_TAG_SIZE], NET_AAED_TAG_SIZE);

		int plaintext_length{};
		if (1 != EVP_DecryptUpdate(ctx.get(), &encrypted_data[0], &plaintext_length, &encrypted_data[0], length_without_tag) || plaintext_length < 0)
		{
			// wipe buffer as it might have invalid data (prevent misuse)
			memset(&encrypted_data[0], 0xDE, length_without_tag);
			errorstream << "decrypt_aes_128_gcm(): decryption failed!" << std::endl;
			return false;
		}

		if (1 != EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_TAG, NET_AAED_TAG_SIZE, AAD))
		{
			// wipe buffer as it might have invalid data (prevent misuse)
			memset(&encrypted_data[0], 0xDE, length_without_tag);
			errorstream << "decrypt_aes_128_gcm(): setting expected AAED tag failed!" << std::endl;
			return false;
		}

		int final_length = {};
		if (1 != EVP_DecryptFinal_ex(ctx.get(), &encrypted_data[plaintext_length], &final_length) ||
			static_cast<size_t>(final_length) + static_cast<size_t>(plaintext_length) != length_without_tag)
		{
			// wipe buffer as it might have invalid data (prevent misuse)
			memset(&encrypted_data[0], 0xDE, length_without_tag);
			errorstream << "decrypt_aes_128_gcm(): validation failed!" << std::endl;
			return false;
		}

		encrypted_data.shrinkSize(length_without_tag);

		return true;

	}

	bool generate_ephemeral_key_pair(ECDHEKeyPair& output) {
		static_assert(sizeof(output.private_key) == 32);
		static_assert(sizeof(output.public_key) == 32);

		u8 random_data[32] = {};
		if (!porting::secure_rand_fill_buf(random_data, sizeof(random_data)))
			return false;
		// from https://cr.yp.to/ecdh.html
		random_data[0] &= 248;
		random_data[31] &= 127;
		random_data[31] |= 64;

		memcpy(output.private_key, random_data, sizeof(output.private_key));
		EverCrypt_Curve25519_secret_to_public(random_data, output.public_key);

		return true;
	}

#pragma endregion
}
