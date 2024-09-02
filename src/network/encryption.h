#pragma once

#include "networkconfig.h"
#include "log.h"
#include "util/pointer.h"
#include "util/serialize.h"
#include <openssl/evp.h>
#include <memory>

/*
*  Network Encryption v1
*  
*  A traditional PKI system is not a good fit for a video game where anyone can create a server,
*  but there are still negative privacy and security implications to sending network packets in cleartext,
*  and a best effort should be made to avoid it.
*  
*  
*  
*  High level overview
*   
*   For protocol level >=54 connections a ECDHE key-exchanged is performed in TOSERVER_INIT and TOCLIENT_HELLO,
*   after this point - assuming both the client and the server support the required protocol level - all further *reliable* network packets are encrypted.
*   Unreliable packets are considered low value and are *not* currently encrypted. Server or client identity is not checked at this point, an active attacker
*   could intercept the session.
*   
*   If this is the first session for that user (e.g. registration), then this is likely the best we can do without building a PKI for Minetest.
*   A PKI could be managed by the main server list, or could piggyback of existing HTTPS ACME PKI (e.g. LetsEncrypt).
*   Both are viable options but would seriously increase the complexity of the implementation and/or management burden for sysadmins.
*   
*   However most session involve a user logging into an existing account, in this case the user and server mutually authenticate using SRP,
*   a digest of the shared secret key established in the initial handshake can be added to the state hashed for the evidence messages (M_1 and M_2)
*   
*   To help Lua mods make reasonable decisions with regards to security the encryption and authentication level is available via the API. 
*
*  Implementation details
*
*
* 
*  Initial handshake
* 
*   On initial handshake the client generates an ephemeral Curve25519 [1] keypair and unconditionally sends the public key to the server in TOSERVER_INIT
*   The server checks the client protocol version and if it's >=54, it generates its own key-pair and replies to the client with its own public key.
*   Both sides then derive a shared EC secret using the x25519 ECDH function [1], the output is passed to SHA256 to generate the root network encryption key
*   Key-pairs are completely ephemeral and a new pair is generated for each connection.
* 
*   root_net_key = hkdf_extract_sha256(None, x25519(otherPubKey, otherPrivateKey))
*   
*   The "salt" could be used here for domain separation, but it's not clear how this would improve the security of the construction.
*
*
* 
*  Message encryption.
*
*   HKDF-Expand is used to generate domain-separated per-channel reliable message encryption keys,
*   a different key is generated for the client and server sender:
*
*   channel_client_sender_keys = hkdf_expand(root_net_key, "minetest-client-channel-send-key", 16*max_channel_count())
*   channel_server_sender_keys = hkdf_expand(root_net_key, "minetest-server-channel-send-key", 16*max_channel_count())
*   our_channel_keys = channel_client_sender_keys is minetest_is_client() else channel_server_sender_keys
*   other_channel_keys = channel_server_sender_keys is minetest_is_client() else channel_client_sender_keys
*
*   channel_0_send_key = our_channel_keys[0..16]
*   channel_0_receive_key = other_channel_keys[0..16]
*   channel_1_send_key = our_channel_keys[16..32]
*   ...
*
* 
*   AES-128 in GCM mode is then used to encrypt reliable messages, with the IV generated from 64-bit counter
*   constructed from a 16-bit seq_num counter and a 48-bit seq_num_generation counter, along with the channel ID.
*   In the exceptionally unlikely event this counter overflows the peer is disconnected.
*   This follows the deterministic construction recommended by NIST 800-38D 8.2.1 [3]
*   As GCM is used with a deterministic IV, the maximum number of invocations is 2^32,
*   as the probability of IV reuse is <= 2^-32 (it is zero) [3]
*
*   Key renegotiation is not implemented currently, but could be added in the future,
*   Under conservative assumptions of 1000 packets a seconds per channel, it would take just under 50 days
*   for more than 2^32 invocations to be carried out.
*
*   Key freshness for AES-128-GCM is assured by generating a new key for each session using ECDHE.
*
*   Only the 16-bit seq_num counter is transmitted, but the upper bits can be recovered easily.
*
*   IV = [channel_id] + 3*[0] + (seq_num_gen << 48 | seq_num).to_bytes(64, byteorder="little")
*
*   The authentication hash is truncated to be 12-bytes/96-bits long to reduce overhead while
*   still offering a high level of security.[6]
*
*
*   
*  SRP State Hash
*
*   The cryptosystem as described above only provides confidentiality not authentication,
*   without a PKI there's no way to verify the server is who they claim to be on initial connection.
*
*   If this is not the first login we can modify the SRP [4] login flow to add a digest generated from the root network key
*   to the state hashed to generate the evidence messages M and H (also called M_2 or M_s in some descriptions of the protocol).
*
*   srp_net_key_digest = hkdf_expand(root_net_key, "minetest-handshake-digest-for-srp", 64)
* 
*   M_1 = H(...|srp_net_key_digest)
*   M_2 = H(...|srp_net_key_digest)
*
*   where ... is the standard state that SRP hashes over anyways
*
*   This should have no impact on the security of the SRP login process, no parts of the core
*   protocol are changed, additional state is simply added to the evidence message.
*
*   Hashing the key into the state stops an active attacker from being able to tamper with a connection unless
*   it's the first login for that user or a protocol downgrade attack is carried out.
*
* Partial resistance to downgrade attacks
* 
*   Even if a protocol downgrade attack is carried out, there is no way to convince the server the connection is secure,
*   a server or mod may for instance not allow access to administrative commands or refuse password change requests or
*   apply other custom security policies.
*   In the future as more clients are upgraded to the latest version server administrators may decide to increase the protocol
*   version they require. By making encryption mandatory for all connections in a way that doesn't require any centralized PKI authority
*   unencrypted connections only need to be supported as an accommodation for old clients and servers, under the assumption most servers
*   generally run the latest version or are at most one or two versions behind this would allow for unencrypted connections to be disabled
*   by default in new client releases in the near future.
*
* Adding support for PKI/signed certificates (potential future work)
*
*  A PKI just for minetest could be setup, or existing HTTPS CA infrastructure could be reused,
*  by having Minetest server operators host a file containing the public keys and ports of all
*  minetest instances on a given domain name in the /.well-known/ directory [5] of a HTTPS
*  server serving content for the same domain name.
*
*  In such a case a client could verify their connection to the server was not intercepted even on initial registration,
*  by the server appending a message containing A) its public key certificate B) instructions for how to validate that certificate
*  C) hkdf_expand(root_net_key, "minetest-root-key-digest").
*  Which it then can sign using the servers long term private key.
*
*  If implemented this offers additional security, but does not replace the need for mutual SRP authentication,
*  A server having a signed certificate just shows the domain name owner likely authorized it and that the
*  connection was not intercepted by a third party.
*  It does not demonstrate the server is the one the user *thinks* it is, while knowledge of the SRP verifier,
*  does indicate this is most likely the server the user has an account on.
*   
* 
*  References
*  [1] https://datatracker.ietf.org/doc/html/rfc7748
*  [2] https://datatracker.ietf.org/doc/html/rfc5869
*  [3] https://csrc.nist.gov/pubs/sp/800/38/d/final
*  [4] https://datatracker.ietf.org/doc/html/rfc2945
*  [5] https://datatracker.ietf.org/doc/html/rfc5785
*  [6] https://crypto.stackexchange.com/a/67565
*/

namespace NetworkEncryption
{
	struct ECDHEKeyPair
	{
		static constexpr int key_type = EVP_PKEY_X25519;

		u8 private_key[NET_ECDHE_PRIVATE_KEY_LEN];
		u8 public_key[NET_ECDHE_PUBLIC_KEY_LEN];
	};

	struct ECDHEPublicKey
	{
		u8 key[NET_ECDHE_PUBLIC_KEY_LEN];
	};

	struct AESChannelKeys
	{
		u8 keys[CHANNEL_COUNT][NET_AES_KEY_SIZE];
	};

	struct PacketIV
	{
		u8 iv[NET_AES_IV_SIZE];
	};

	struct HandshakeDigest
	{
		u8 digest[64] = {};
	};

	/*
	* Calculates the shared ECHD secret, hashes it to get a root key (using HKDF-extract-sha256), then derives per-channel keys (HKDF-expand)
	*/
	[[nodiscard]] bool derive_subkeys(const ECDHEKeyPair& our_keys,
		const ECDHEPublicKey& other_pub_key,
		AESChannelKeys&  client_send_keys,
		AESChannelKeys&  server_send_keys,
		HandshakeDigest& srp_signing_key);

	[[nodiscard]] bool get_identity_for_srp(NetworkEncryption::HandshakeDigest handshake_digest,
		std::string_view name,
		std::string& identity_value_out);


	[[nodiscard]] bool ecdh_calculate_shared_secret(const ECDHEKeyPair& our_keys,
		const ECDHEPublicKey& other_pub_key,
		u8(&shared_secret)[NET_ECDHE_SECRET_LEN]);

	[[nodiscard]] bool hmac_sha256(const u8* key, size_t key_length, const u8* msg, size_t msg_length, u8(&output)[32]);

	[[nodiscard]] bool hkdf_extract_sha256(const u8* data, size_t length, u8(&output)[32], const std::string_view& salt = "");
	[[nodiscard]] bool hkdf_expand_sha256(const u8(&input)[32], const std::string_view& info, u8* output, size_t length);

	[[nodiscard]] bool encrypt_aes_128_gcm(const u8(&key)[16], const u8(&iv)[12], const Buffer<u8>& plaintext, u8* encrypted_data, size_t encrypted_buf_length);
	[[nodiscard]] bool decrypt_aes_128_gcm(const u8 (&key)[16], const u8(&iv)[12], Buffer<u8> &encrypted_data);

	[[nodiscard]] inline PacketIV generate_packet_iv(u8 channel_id, u64 packet_id)
	{
		PacketIV packet_iv = {};

		packet_iv.iv[0] = channel_id;
		writeU64(&packet_iv.iv[4], packet_id);

		return packet_iv;
	}

	typedef std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> unique_pkey_t;
	[[nodiscard]] inline unique_pkey_t make_unique_pkey(EVP_PKEY* pkey)
	{
		return { pkey, &EVP_PKEY_free };
	}

	enum class ConnectionSecurityLevel {
		// No encryption or authentication
		None,

		// encryption-only -> peer identity is not verified.
		// The session is protected against passive decryption (perfect forward secrecy) or command injection
		// but an active attacker (MITM/on-path) can intercept the handshake and take over that session
		Passive,

		// Identity of both peers is validated (using shared SRP secret)
		// Session is protected against active on-path attackers, if they do not know the user's password
		// future password disclosure does not allow for decryption of past sessions (perfect forward secrecy)
		// This is strictly a super-set of the protection offered by `ConnectionSecurityLevel::Passive`
		FullyAuthenicated,

	};

	class EphemeralKeyGenerator
	{
	public:

		EphemeralKeyGenerator() :
			m_pctx(EVP_PKEY_CTX_new_id(ECDHEKeyPair::key_type, NULL), &EVP_PKEY_CTX_free)
		{
			if (m_pctx)
			{
				int result = EVP_PKEY_keygen_init(m_pctx.get());

				assert(1 == result);

				if (result < 1)
				{
					errorstream << "NetworkEphemeralKeyGenerator: failed to initialize key generator: " << result << std::endl;
					m_pctx.reset();
				}
			}

		}

		[[nodiscard]] bool generate(ECDHEKeyPair& key_out);

		operator bool() const noexcept
		{
			return m_pctx.operator bool();
		}
	private:

		[[nodiscard]] unique_pkey_t keygen(int& result);

		std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> m_pctx;
	};
}
