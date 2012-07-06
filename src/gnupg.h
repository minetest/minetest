#ifndef GNUPG_HEADER
#define GNUPG_HEADER
#ifdef CLIENT
#include "irrlicht.h" // devices/drivers
#endif

#include <stdint.h>
#include <string>

namespace gnupg {
  const uint8_t FINGERPRINT_LENGTH = 40;
  const uint8_t NONCE_LENGTH = 0x40;

  class PendingChallenge {
  public:
    uint8_t solution[NONCE_LENGTH];
    std::string question;
    std::string who;
  PendingChallenge(const std::string& wwho) : who(wwho) {}
    bool matches(const std::string& answer);
  };

  extern std::string mykey;
  extern bool pgpEnabled;

#ifdef CLIENT
    void assureMyKey(const std::string& configPath, 
                     IrrlichtDevice* device, IrrlichtDriver* driver);
#else
    void assureMyKey(const std::string& configPath, void* device, void* bleh);
#endif
  std::string makeResponse(std::string challenge);
  PendingChallenge* makeChallenge(std::string who);
};

/* the idea is: instead of a user/password, the client sends 
   their key fingerprint. Upon receiving it, server passes it to makeChallenge,
   and sends the question to the client, saving the pending challenge for its 
   nonce. The client decrypts and sends the decrypted nonce. The server 
   compares and they match. Everything is now hunky dory.

   No passwords needed.
*/
#endif /* GNUPG_HEADER */
