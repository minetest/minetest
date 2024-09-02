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

#include "test.h"
#include "network/encryption.h"

class TestEncyption : public TestBase {
public:
	TestEncyption() { TestManager::registerTestModule(this); }
	const char* getName() { return "TestEncyption"; }

	void runTests(IGameDef* gamedef);

	void testHKDF();
	void testAES_128_gcm_encrypt();
	void testECKeyGen();
	void testX25519();

	void testAESSingle(
		const u8(&key)[16],
		const u8(&iv)[12],
		const u8(&aaed_tag_expected)[NET_AAED_TAG_SIZE],
		const std::vector<u8> plaintext,
		const std::vector<u8> expected_cryptotext
	);
};

static TestEncyption g_test_instance;

void TestEncyption::runTests(IGameDef* gamedef)
{
	TEST(testHKDF);
	TEST(testAES_128_gcm_encrypt);
	TEST(testECKeyGen);
	TEST(testX25519);
}

static std::vector<u8> rfc_string_to_cpp(std::string_view rfc)
{
	SANITY_CHECK(rfc.length() % 2 == 0);

	size_t start_offset = 0;
	if (rfc.length() > 2 && rfc[0] == '0' && rfc[1] == 'x')
	{
		start_offset = 2;
	}

	std::vector<u8> binary_output;
	binary_output.reserve(rfc.length() / 2);

	for (size_t i = start_offset; (i + 1) < rfc.length(); i += 2)
	{
		char str[3] = {};
		str[0] = rfc[i];
		str[1] = rfc[i + 1];

		unsigned long value = strtoul(str, nullptr, 16);
		SANITY_CHECK(value < 256);

		binary_output.push_back(static_cast<u8>(value));
	}

	return binary_output;
}
template<size_t len>
static void rfc_string_to_c(u8(&key_out)[len], std::string_view rfc)
{
	std::vector<u8> cpp_value = rfc_string_to_cpp(rfc);
	SANITY_CHECK(cpp_value.size() == len);
	memcpy(key_out, cpp_value.data(), len);
}


void TestEncyption::testHKDF()
{

	// from https://datatracker.ietf.org/doc/html/rfc5869
	// test 1
	{
		const std::vector<u8> IKM = rfc_string_to_cpp("0x0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b");
		const std::vector<u8> salt = rfc_string_to_cpp("0x000102030405060708090a0b0c");
		const std::vector<u8> info = rfc_string_to_cpp("0xf0f1f2f3f4f5f6f7f8f9");

		const std::vector<u8> PRK = rfc_string_to_cpp("0x077709362c2e32df0ddc3f0dc47bba6390b6c73bb50f9c3122ec844ad7c2b3e5");
		const std::vector<u8> OKM = rfc_string_to_cpp("0x3cb25f25faacd57a90434f64d0362f2a2d2d0a90cf1a5a4c5db02d56ecc4c5bf34007208d5b887185865");

		const std::string_view salt_sw = std::string_view((char*)salt.data(), salt.size());
		const std::string_view info_sw = std::string_view((char*)info.data(), info.size());

		SANITY_CHECK(PRK.size() == 32);

		u8 prk_actual[32] = {};
		UASSERT(NetworkEncryption::hkdf_extract_sha256(IKM.data(), IKM.size(), prk_actual, salt_sw));
		UASSERT(memcmp(prk_actual, PRK.data(), 32) == 0);

		std::vector<u8> OKM_actual;
		OKM_actual.resize(OKM.size());

		UASSERT(NetworkEncryption::hkdf_expand_sha256(prk_actual, info_sw, OKM_actual.data(), OKM_actual.size()));
		UASSERT(OKM_actual == OKM);
	}
	// test 2
	{
		const std::vector<u8> IKM = rfc_string_to_cpp(
			"0x000102030405060708090a0b0c0d0e0f"
			"101112131415161718191a1b1c1d1e1f"
			"202122232425262728292a2b2c2d2e2f"
			"303132333435363738393a3b3c3d3e3f"
			"404142434445464748494a4b4c4d4e4f");
		const std::vector<u8> salt = rfc_string_to_cpp(
			"0x606162636465666768696a6b6c6d6e6f"
			"707172737475767778797a7b7c7d7e7f"
			"808182838485868788898a8b8c8d8e8f"
			"909192939495969798999a9b9c9d9e9f"
			"a0a1a2a3a4a5a6a7a8a9aaabacadaeaf"
		);
		const std::vector<u8> info = rfc_string_to_cpp(
			"0xb0b1b2b3b4b5b6b7b8b9babbbcbdbebf"
			"c0c1c2c3c4c5c6c7c8c9cacbcccdcecf"
			"d0d1d2d3d4d5d6d7d8d9dadbdcdddedf"
			"e0e1e2e3e4e5e6e7e8e9eaebecedeeef"
			"f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff"
		);

		const std::vector<u8> PRK = rfc_string_to_cpp(
			"0x06a6b88c5853361a06104c9ceb35b45c"
			"ef760014904671014a193f40c15fc244"
		);
		const std::vector<u8> OKM = rfc_string_to_cpp(
			"0xb11e398dc80327a1c8e7f78c596a4934"
			"4f012eda2d4efad8a050cc4c19afa97c"
			"59045a99cac7827271cb41c65e590e09"
			"da3275600c2f09b8367793a9aca3db71"
			"cc30c58179ec3e87c14c01d5c1f3434f"
			"1d87"
		);

		const std::string_view salt_sw = std::string_view((char*)salt.data(), salt.size());
		const std::string_view info_sw = std::string_view((char*)info.data(), info.size());

		SANITY_CHECK(PRK.size() == 32);

		u8 prk_actual[32] = {};
		UASSERT(NetworkEncryption::hkdf_extract_sha256(IKM.data(), IKM.size(), prk_actual, salt_sw));
		UASSERT(memcmp(prk_actual, PRK.data(), 32) == 0);

		std::vector<u8> OKM_actual;
		OKM_actual.resize(OKM.size());

		UASSERT(NetworkEncryption::hkdf_expand_sha256(prk_actual, info_sw, OKM_actual.data(), OKM_actual.size()));
		UASSERT(OKM_actual == OKM);
	}
	// test 3
	{
		const std::vector<u8> IKM = rfc_string_to_cpp("0x0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b");
		const std::vector<u8> PRK = rfc_string_to_cpp(
			"0x19ef24a32c717b167f33a91d6f648bdf"
			"96596776afdb6377ac434c1c293ccb04"
		);
		const std::vector<u8> OKM = rfc_string_to_cpp(
			"0x8da4e775a563c18f715f802a063c5a31"
			"b8a11f5c5ee1879ec3454e5f3c738d2d"
			"9d201395faa4b61a96c8"
		);

		SANITY_CHECK(PRK.size() == 32);

		u8 prk_actual[32] = {};
		UASSERT(NetworkEncryption::hkdf_extract_sha256(IKM.data(), IKM.size(), prk_actual));
		UASSERT(memcmp(prk_actual, PRK.data(), 32) == 0);

		std::vector<u8> OKM_actual;
		OKM_actual.resize(OKM.size());

		UASSERT(NetworkEncryption::hkdf_expand_sha256(prk_actual, "", OKM_actual.data(), OKM_actual.size()));
		UASSERT(OKM_actual == OKM);
	}
}

void TestEncyption::testAESSingle(
	const u8(&key)[16],
	const u8(&iv)[12],
	const u8(&aaed_tag_expected)[NET_AAED_TAG_SIZE],
	const std::vector<u8> plaintext,
	const std::vector<u8> expected_cryptotext
)
{
	std::vector<u8> actual_cryptotext;
	actual_cryptotext.resize(expected_cryptotext.size() + sizeof(aaed_tag_expected));

	UASSERT(NetworkEncryption::encrypt_aes_128_gcm(key, iv, Buffer<u8>(plaintext.data(), plaintext.size()), actual_cryptotext.data(), actual_cryptotext.size()));

	UASSERT(memcmp(expected_cryptotext.data(), actual_cryptotext.data(), expected_cryptotext.size()) == 0);
	UASSERT(memcmp(aaed_tag_expected, &actual_cryptotext[expected_cryptotext.size()], sizeof(aaed_tag_expected)) == 0);
}

void TestEncyption::testAES_128_gcm_encrypt()
{
	// some test vectors from csrc.nist.gov/groups/STM/cavp/documents/mac/gcmtestvectors.zip

	{
		/*
		*
		*
		[Keylen = 128]
		[IVlen = 96]
		[PTlen = 128]
		[AADlen = 0]
		[Taglen = 96]
		*/

		// count 0

		u8 key[16] = {};
		u8 iv[12] = {};
		u8 aaed_tag_expected[12] = {};

		rfc_string_to_c(key, "f00fdd018c02e03576008b516ea971ad");
		rfc_string_to_c(iv, "3b3e276f9e98b1ecb7ce6d28");
		rfc_string_to_c(aaed_tag_expected, "cba06bb4f6e097199250b0d1");

		const std::vector<u8> plaintext = rfc_string_to_cpp("2853e66b7b1b3e1fa3d1f37279ac82be");
		const std::vector<u8> expected_cryptotext = rfc_string_to_cpp("55d2da7a3fb773b8a073db499e24bf62");

		testAESSingle(key, iv, aaed_tag_expected, plaintext, expected_cryptotext);
	}
	{
		/*
		*
		*
		[Keylen = 128]
		[IVlen = 96]
		[PTlen = 128]
		[AADlen = 0]
		[Taglen = 96]
		*/

		// count 1

		u8 key[16] = {};
		u8 iv[12] = {};
		u8 aaed_tag_expected[12] = {};

		rfc_string_to_c(key, "bc8fb606bc51571912ad8732ca4ee7af");
		rfc_string_to_c(iv, "fd4c8432015c5a5def1561c5");
		rfc_string_to_c(aaed_tag_expected, "c90cd06a5fffa615291c2f3b");

		const std::vector<u8> plaintext = rfc_string_to_cpp("bcf430dc33aa27c6b31c377c2d6b0133");
		const std::vector<u8> expected_cryptotext = rfc_string_to_cpp("3b864d7c12e8dca51a65b0be202cb8d0");

		testAESSingle(key, iv, aaed_tag_expected, plaintext, expected_cryptotext);
	}
	// another test
	{
		/*
		*
		*
		[Keylen = 128]
		[IVlen = 96]
		[PTlen = 408]
		[AADlen = 0]
		[Taglen = 96]
		*/

		u8 key[16] = {};
		u8 iv[12] = {};
		u8 aaed_tag_expected[12] = {};

		rfc_string_to_c(key, "9aa701eaf1146ae9a8aa14f36294e8e0");
		rfc_string_to_c(iv, "fd78280e023ff4cdcaab5e67");
		rfc_string_to_c(aaed_tag_expected, "87b981bdd2c37fcc6ff734a9");

		const std::vector<u8> plaintext = rfc_string_to_cpp("806f21e96bcd6c8ec1b7f688978c0ffd24492cd38eb62361fd73eeffbee4d9f9d7ad32d408ffc6706647bc723c620c83020f06");
		const std::vector<u8> expected_cryptotext = rfc_string_to_cpp("010428fc5b03162f7e001fd2f4f2d1a8ab13ce97063c82cfe62e7cd5b26551b03a55358857159959ab021e7015f370b6fc1f16");

		testAESSingle(key, iv, aaed_tag_expected, plaintext, expected_cryptotext);
	}

}

void TestEncyption::testECKeyGen()
{
	NetworkEncryption::EphemeralKeyGenerator keygen;
	for (int i = 0; i < 2000; i++)
	{
		NetworkEncryption::ECDHEKeyPair keypair = {};

		UASSERT(keygen.generate(keypair));

		// check if the private key is valid
		UASSERT((keypair.private_key[0] & ~248u) == 0);
		UASSERT((keypair.private_key[31] & ~127u) == 0);
		UASSERT((keypair.private_key[31] & 64u) == 64u);
	}
}

void TestEncyption::testX25519()
{
	u8 a_private_key[32];
	u8 a_public_key[32];

	rfc_string_to_c(a_private_key, "77076d0a7318a57d3c16c17251b26645df4c2f87ebc0992ab177fba51db92c2a");
	rfc_string_to_c(a_public_key, "8520f0098930a754748b7ddcb43ef75a0dbf3a0d26381af4eba4a98eaa9b4e6a");

	u8 b_private_key[32];
	u8 b_public_key[32];

	rfc_string_to_c(b_private_key, "5dab087e624a8a4b79e17f8b83800ee66f3bb1292618b6fd1c2f8b27ff88e0eb");
	rfc_string_to_c(b_public_key, "de9edb7d7b7dc1b4d35b61c2ece435373f8343c85b78674dadfc7e146f882b4f");

	NetworkEncryption::ECDHEKeyPair a_keypair = {};
	NetworkEncryption::ECDHEPublicKey a_pubkey_net = {};
	memcpy(&a_keypair.private_key, a_private_key, sizeof(a_private_key));
	memcpy(&a_keypair.public_key, a_public_key, sizeof(a_public_key));
	memcpy(&a_pubkey_net.key, a_public_key, sizeof(a_public_key));

	NetworkEncryption::ECDHEKeyPair b_keypair = {};
	NetworkEncryption::ECDHEPublicKey b_pubkey_net = {};
	memcpy(&b_keypair.private_key, b_private_key, sizeof(b_private_key));
	memcpy(&b_keypair.public_key, b_public_key, sizeof(b_public_key));
	memcpy(&b_pubkey_net.key, b_public_key, sizeof(b_public_key));

	u8 expected_shared_secret[NET_ECDHE_SECRET_LEN] = {};
	rfc_string_to_c(expected_shared_secret, "4a5d9d5ba4ce2de1728e3bf480350f25e07e21c947d19e3376f09b3c1e161742");

	{
		u8 calculated_shared_secret_a[NET_ECDHE_SECRET_LEN] = {};
		UASSERT(NetworkEncryption::ecdh_calculate_shared_secret(a_keypair, b_pubkey_net, calculated_shared_secret_a));
		UASSERT(memcmp(calculated_shared_secret_a, expected_shared_secret, NET_ECDHE_SECRET_LEN) == 0);
	}

	{
		u8 calculated_shared_secret_b[NET_ECDHE_SECRET_LEN] = {};
		UASSERT(NetworkEncryption::ecdh_calculate_shared_secret(b_keypair, a_pubkey_net, calculated_shared_secret_b));
		UASSERT(memcmp(calculated_shared_secret_b, expected_shared_secret, NET_ECDHE_SECRET_LEN) == 0);
	}

}
