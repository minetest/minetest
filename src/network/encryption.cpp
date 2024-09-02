#include "encryption.h"
#include "settings.h"
#include "util/base64.h"
#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <openssl/hmac.h>
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
		if (!hkdf_extract_sha256(shared_secret, sizeof(shared_secret), root_key))
		{
			errorstream << "derive_subkeys(): failed to derive root key" << std::endl;
			return false;
		}

		if (!hkdf_expand_sha256(
			root_key,
			"minetest-client-channel-send-key",
			&client_send_keys.keys[0][0],
			CHANNEL_COUNT * NET_AES_KEY_SIZE))
		{
			errorstream << "derive_subkeys(): failed to derive client sender keys" << std::endl;
			return false;
		}

		if (!hkdf_expand_sha256(
			root_key,
			"minetest-server-channel-send-key",
			&server_send_keys.keys[0][0],
			CHANNEL_COUNT * NET_AES_KEY_SIZE))
		{
			errorstream << "derive_subkeys(): failed to derive server sender keys" << std::endl;
			return false;
		}

		if (!hkdf_expand_sha256(
			root_key,
			"minetest-handshake-digest-for-srp",
			handshake_digest.digest,
			sizeof(handshake_digest.digest)))
		{
			errorstream << "derive_subkeys(): failed to derive handshake digest for SRP" << std::endl;
			return false;
		}

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

	[[nodiscard]] bool get_identity_for_srp(NetworkEncryption::HandshakeDigest handshake_digest,
		std::string_view name,
		std::string& identity_value_out) {
		u8 hmac_output[32] = {};
		if (!hmac_sha256(
			handshake_digest.digest,
			sizeof(handshake_digest.digest),
			reinterpret_cast<const u8*>(name.data()), name.size(),
			hmac_output))
			return false;

		std::string_view hmac_output_view = std::string_view(reinterpret_cast<char*>(hmac_output), sizeof(hmac_output));

		std::stringstream ss;
		ss << name;
		ss << ":";
		ss << base64_encode(hmac_output_view);

		identity_value_out = ss.str();

		return true;
	}

	/*
	* Everything below this point is code for interfacing with openssl
	*/

#pragma region OPENSSL_IMPL

	[[nodiscard]] bool ecdh_calculate_shared_secret(const ECDHEKeyPair& our_keys,
		const ECDHEPublicKey& other_pub_key,
		u8(&shared_secret)[NET_ECDHE_SECRET_LEN])
	{
		unique_pkey_t pkey_peer_pub = make_unique_pkey(EVP_PKEY_new_raw_public_key(ECDHEKeyPair::key_type, NULL, &other_pub_key.key[0], NET_ECDHE_PUBLIC_KEY_LEN));

		if (!pkey_peer_pub)
		{
			errorstream << "ecdh_calculate_shared_secret(): failed to load other peer's public key!" << std::endl;
			return false;
		}

		unique_pkey_t pkey_our_private_key = make_unique_pkey(EVP_PKEY_new_raw_private_key(ECDHEKeyPair::key_type, NULL, &our_keys.private_key[0], NET_ECDHE_PRIVATE_KEY_LEN));

		if (!pkey_our_private_key)
		{
			errorstream << "ecdh_calculate_shared_secret(): failed to load our peer's private key!" << std::endl;
			return false;
		}

		std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> pctx = {
			EVP_PKEY_CTX_new(pkey_our_private_key.get(), NULL),
			&EVP_PKEY_CTX_free };

		if (1 != EVP_PKEY_derive_init(pctx.get()))
		{
			errorstream << "ecdh_calculate_shared_secret(): failed init derive ctx!" << std::endl;
			return false;
		}

		if (1 != EVP_PKEY_derive_set_peer(pctx.get(), pkey_peer_pub.get()))
		{
			errorstream << "ecdh_calculate_shared_secret(): failed set derive peer!" << std::endl;
			return false;
		}

		size_t echd_secret_length = {};
		if (1 != EVP_PKEY_derive(pctx.get(), NULL, &echd_secret_length) || echd_secret_length == 0)
		{
			errorstream << "ecdh_calculate_shared_secret(): failed get secret length!" << std::endl;
			return false;
		}

		assert(NET_ECDHE_SECRET_LEN == echd_secret_length);
		if (echd_secret_length != NET_ECDHE_SECRET_LEN)
		{
			errorstream << "ecdh_calculate_shared_secret(): secret length does not match expected length! got:"
				<< echd_secret_length
				<< " expected:" << NET_ECDHE_SECRET_LEN << std::endl;
			return false;
		}

		if (1 != EVP_PKEY_derive(pctx.get(), shared_secret, &echd_secret_length) || NET_ECDHE_SECRET_LEN != echd_secret_length)
		{
			errorstream << "ecdh_calculate_shared_secret(): failed to calculate secret! len=" << echd_secret_length << std::endl;
			return false;
		}

		return true;
	}

	bool hmac_sha256(const u8* key, size_t key_length, const u8* msg, size_t msg_length, u8(&output)[32]) {
		static_assert(EVP_MAX_MD_SIZE >= 32);

		u8 md_out[EVP_MAX_MD_SIZE];
		unsigned int md_out_len = {};
		if (!HMAC(EVP_sha256(), key, key_length, msg, msg_length, md_out, &md_out_len)
			|| md_out_len != 32) {
			return false;
		}

		memcpy(output, md_out, sizeof(output));
		return true;
	}

#if OPENSSL_VERSION_NUMBER >= 0x30000000L
	typedef std::unique_ptr<EVP_KDF_CTX, decltype(&EVP_KDF_CTX_free)> kdf_context_t;

	static bool hkdf_sha256_derive_internal(const u8* input, size_t input_length, u8* output, size_t output_length,
		bool is_expand, const std::string_view &info, const std::string_view& salt)
	{
		std::unique_ptr<EVP_KDF, decltype(&EVP_KDF_free)> kdf = { EVP_KDF_fetch(NULL, "hkdf", NULL), EVP_KDF_free };
		if (kdf == NULL)
		{
			errorstream << "hkdf_sha256_derive_internal(): failed to fetch HKDF" << std::endl;
			return false;
		}

		std::unique_ptr<EVP_KDF_CTX, decltype(&EVP_KDF_CTX_free)> kctx = { EVP_KDF_CTX_new(kdf.get()), EVP_KDF_CTX_free };
		if (!kctx)
		{
			errorstream << "hkdf_sha256_derive_internal(): failed to create HKDF context" << std::endl;
			return false;
		}

		// makes clang happier
		char digest_type[] = "sha256";

		OSSL_PARAM params[6] = {}, * p = params;
		*p++ = OSSL_PARAM_construct_utf8_string("digest", digest_type, (size_t)7);
		// openssl doesn't do strong typing
		// this is safe
		*p++ = OSSL_PARAM_construct_octet_string("key", const_cast<u8*>(input), input_length);
		int mode = is_expand ? EVP_KDF_HKDF_MODE_EXPAND_ONLY : EVP_KDF_HKDF_MODE_EXTRACT_ONLY;
		*p++ = OSSL_PARAM_construct_int("mode", &mode);
		if (info.length() != 0 && is_expand)
		{
			*p++ = OSSL_PARAM_construct_octet_string("info", const_cast<char*>(info.data()), info.length());
		}

		if (salt.length() != 0 && !is_expand)
		{
			*p++ = OSSL_PARAM_construct_octet_string("salt", const_cast<char*>(salt.data()), salt.length());
		}

		if (1 != EVP_KDF_derive(kctx.get(), output, output_length, params))
		{
			errorstream << "hkdf_sha256_derive_internal(): failed to derive output key" << std::endl;
			return false;
		}

		return true;
	}

	bool hkdf_extract_sha256(const u8* data, size_t length, u8(&output)[32], const std::string_view& salt)
	{
		return hkdf_sha256_derive_internal(data, length, output, sizeof(output), false, "", salt);
	}

	bool hkdf_expand_sha256(const u8(&input)[32], const std::string_view& info, u8* output, size_t length)
	{
		return hkdf_sha256_derive_internal(input, sizeof(input), output, length, true, info, "");
	}
#else

	bool hkdf_extract_sha256(const u8* data, size_t length, u8(&output)[32], const std::string_view& salt)
	{
		const u8* salt_ptr = reinterpret_cast<const u8 *>(salt.data());
		size_t salt_length = salt.size();
		if (salt_length == 0)
		{
			static constexpr u8 static_salt[32] = {};

			salt_ptr = static_salt;
			salt_length = sizeof(static_salt);
		}

		return hmac_sha256(salt_ptr, salt_length, data, length, output);
	}

	bool hkdf_expand_sha256(const u8(&prk)[32], const std::string_view& info, u8* output, size_t length)
	{
		// fast return if no bytes were requested
		// prints to warning stream as this is a misuse of the API
		if (length == 0)
		{
			warningstream << "hkdf_expand_sha256(): got called with zero bytes of output requested, this is unexpected" << std::endl;
			return true;
		}

		std::vector<u8> hmac_message;
		hmac_message.reserve(32 + info.size() + 1);

		u8 i = 0;
		size_t bytes_generated = 0;
		u8 last_hmac_output[32] = {};

		while (bytes_generated < length)
		{
			if (i == std::numeric_limits<u8>::max())
			{
				// wipe output as it's incomplete
				memset(output, 0, length);

				errorstream << "hkdf_expand_sha256(): Too many bytes requested, unable to comply" << std::endl;
				return false;
			}

			i++;
			/* build message for HMAC */
			if (i != 1)
			{
				hmac_message.insert(std::end(hmac_message), std::begin(last_hmac_output), std::end(last_hmac_output));
			}
			hmac_message.insert(std::end(hmac_message), info.begin(), info.end());
			SANITY_CHECK(i != 0); // should never be zero, as we check for overflow at the start of this block.
			hmac_message.push_back(i);

			if (!hmac_sha256(prk, sizeof(prk), hmac_message.data(), hmac_message.size(), last_hmac_output))
			{
				// wipe output as it's incomplete
				memset(output, 0, length);

				errorstream << "hkdf_expand_sha256(): call to hmac_sha256 failed, unable to continue";

				return false;
			}

			size_t length_left = length - bytes_generated;

			memcpy(&output[bytes_generated], last_hmac_output, std::min(length_left, sizeof(last_hmac_output)));

			bytes_generated += sizeof(last_hmac_output);
		}

		return true;
	}
#endif

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

	bool EphemeralKeyGenerator::generate(ECDHEKeyPair& key_out)
	{
		if (!m_pctx.get())
			return false;

		bool success = true;

		int key_gen_result;
		unique_pkey_t pkey = keygen(key_gen_result);

		if (key_gen_result == 1 && pkey)
		{
			size_t actual_private_key_length = NET_ECDHE_PRIVATE_KEY_LEN;
			size_t actual_public_key_length = NET_ECDHE_PUBLIC_KEY_LEN;

			int get_private_key_result = EVP_PKEY_get_raw_private_key(pkey.get(), key_out.private_key, &actual_private_key_length);
			if (1 != get_private_key_result || NET_ECDHE_PRIVATE_KEY_LEN != actual_private_key_length)
			{
				errorstream << "NetworkEphemeralKeyGenerator: failed to get raw private key: " << get_private_key_result << std::endl;
				success = false;
			}

			int get_public_key_result = EVP_PKEY_get_raw_public_key(pkey.get(), key_out.public_key, &actual_public_key_length);
			if (1 != get_public_key_result || NET_ECDHE_PUBLIC_KEY_LEN != actual_public_key_length)
			{
				errorstream << "NetworkEphemeralKeyGenerator: failed to get raw public key: " << get_public_key_result << std::endl;
				success = false;
			}

		}
		else {
			errorstream << "NetworkEphemeralKeyGenerator: failed to generate key: " << key_gen_result << std::endl;
			success = false;
		}


		if (!success)
		{
			// clear output keys if generation failed
			key_out = {};
		}

		return success;
	}

	unique_pkey_t EphemeralKeyGenerator::keygen(int& result)
	{
		EVP_PKEY* pkey = nullptr;
		result = EVP_PKEY_keygen(m_pctx.get(), &pkey);

		return make_unique_pkey(pkey);
	}
#pragma endregion
}
