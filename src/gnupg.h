#ifndef GNUPG_HEADER
#define GNUPG_HEADER
#include "config.h"

#include "peer.h"

#include "irrlichttypes.h"

#ifdef BUILD_CLIENT
#include "irrlicht.h" // driver
#include "irrlichttypes_extrabloated.h" // device
#endif

#include <stdint.h>
#include <string>

namespace gnupg {
  const static u8 FINGERPRINT_LENGTH = 40;
  const static u8 NONCE_LENGTH = 0x40;

  class PendingChallenge {
  public:
    u8 solution[NONCE_LENGTH];
    std::string question;
    std::string who;
  PendingChallenge(const std::string& wwho) : who(wwho) {}
    bool matches(u8* answer, size_t alen);
  };

  class PassPhraseGetter {
  public:
    bool gotIt;
    PassPhraseGetter() : gotIt(false) {}

    void onPassPhrase(char* passphrase);
  };

  extern std::string mykey;
  extern bool pgpEnabled;

  void start();

#ifdef BUILD_CLIENT
    void assureMyKey(const std::string& configPath, 
                     IrrlichtDevice* device, 
                     video::IVideoDriver* driver);
#else
    void assureMyKey(const std::string& configPath, void* device, void* bleh);
#endif
  std::string exportKey(u8* fpr);
  void importKey(u8* keyData,size_t size);
  PendingChallenge* makeChallenge(std::string who, Peer* const self);
  std::string makeResponse(std::string challenge, Peer* const self);
};

/* the idea is: instead of a user/password, the client sends 
   their key fingerprint. Upon receiving it, server passes it to makeChallenge,
   and sends the question to the client, saving the pending challenge for its 
   nonce. The client decrypts and sends the decrypted nonce. The server 
   compares and they match. Everything is now hunky dory.

   No passwords needed.
*/
#endif /* GNUPG_HEADER */
