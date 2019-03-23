#include "PasswordManager.h"
#include "../cmake_config.h"

#ifdef USE_LIBSECRET
	extern "C" {
		#include <libsecret/secret.h>
	}

	const SecretSchema MinetestSecretSchema = {
			"minetest_server_passwords",
			SECRET_SCHEMA_NONE,
			{
					{ "server", SECRET_SCHEMA_ATTRIBUTE_STRING },
					{ "port", SECRET_SCHEMA_ATTRIBUTE_INTEGER },
					{ "username", SECRET_SCHEMA_ATTRIBUTE_STRING },
					{ nullptr, SECRET_SCHEMA_ATTRIBUTE_STRING }
			}
	};

	bool gnome_get_password(const std::string &server, u16 port, const std::string &username, std::string &password)
	{
		gchar *gpassword = secret_password_lookup_sync(&MinetestSecretSchema, NULL, NULL,
				"server", server.c_str(),
				"port", (int)port,
				"username", username.c_str(), NULL);

		password = gpassword;
		return true;
	}

	bool gnome_store_password(const std::string &server, u16 port, const std::string &username, const std::string &password)
	{
		return secret_password_store_sync(&MinetestSecretSchema, NULL,
									"minetest_login",
									password.c_str(),
									NULL,
									NULL,
										  "server", server.c_str(),
										  "port", (int)port,
										  "username", username.c_str(), NULL) > 0;
	}
#endif


bool PasswordManager::get(const std::string &server, u16 port, const std::string &username, std::string &password) {
#ifdef USE_LIBSECRET
	return gnome_get_password(server, port, username, password);
#endif

	return false;
}

bool PasswordManager::store(const std::string &server, u16 port, const std::string &username,
						  const std::string &password) {
#ifdef USE_LIBSECRET
	return gnome_store_password(server, port, username, password);
#endif

	return false;
}
