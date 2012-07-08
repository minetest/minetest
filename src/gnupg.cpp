// note: gpgme keeps all private key access in a separate process
// because for HUGE SECURITY network capable programs should never
// access or decrypt private keys directly.

#include "gnupg.h"
#include "log.h"
#include "debug.h"
#include "main.h"
#include "settings.h"

#include "util/numeric.h"

#include "jthread/jmutex.h"
#include "jthread/jmutexautolock.h"
#include "jthread/jthread.h"

#include "config.h" // for largefile stuff

#define _FILE_OFFSET_BITS 64
#define LARGEFILE_SOURCE 1

#ifdef BUILD_CLIENT
#ifndef SERVER
#include "guiPassPhraseGetter.h"
#include "mainmenumanager.h"
#endif
#endif

#include "porting.h"
#include "filesys.h"

extern "C" {
#include <locale.h>
#include <gpgme.h>
#include <unistd.h>
#include <string.h> // memcmp
#include <termios.h> // err
#include <sys/stat.h> // mkdir 
#include <sys/types.h>
#include <errno.h> // errno
}

#include <vector>
#include <string>
#include <deque>
#include <stdexcept>

class Value {
public:
  virtual ~Value() {}
};

class Future {
  JMutex waiter;
  bool isSet;
  Value* value;
public:
  Future() : isSet(false), value(NULL) {}
  Value* get() {
    JMutexAutoLock derp(waiter);
    while(!isSet) {
      waiter.Unlock();
      // why no pthread_cond_t T_T
      sleep(1);
      waiter.Lock();
    }
    // XXX: this is not how futures work >:(
    isSet = false;

    return value;
  }
  void set(Value* val) {
    JMutexAutoLock derp(waiter);
    if(isSet) return;
    this->value = val;
    isSet = true;
  }
};

class Responsible : virtual public Value {
public:
  Future onResponse;
};

class KeyNeeder : public virtual Value {
public:
  std::vector<std::string> needKeys;
};

class EncodeResponse : public virtual KeyNeeder {
public:
  gpgme_data_t cipher;
};

class EncodeRequest : public virtual Responsible {
public:
  gpgme_data_t plain;
  std::vector<std::string> signers;
  std::vector<std::string> encrypters;
};

class DecodeResponse : public virtual KeyNeeder {
public:
  gpgme_data_t plain;
  std::vector<std::string> signers;
};

class DecodeRequest : public virtual Responsible {
public:
  gpgme_data_t cipher;
};

class ImportRequest : public virtual Responsible {
  // even if it responds with nothing
public:
  gpgme_data_t keyData;
};

class ExportResponse : public virtual Value {
public:
  gpgme_data_t keyData;
};

class ExportRequest : public virtual Responsible {
public:
  u8* fpr;
};

class GenerateKeyResponse : public virtual Value {
public:
  std::string keyId;
};

class GenerateKeyRequest : public virtual Responsible {
public:
  char* passphrase;
};

class MainThread : public JThread {
public:
  std::deque<Value*> queue;
  JMutex queueLock;
  gpgme_ctx_t ctx;

  void enqueue(Value* what) {
    JMutexAutoLock derp(queueLock);    
    queue.push_front(what);
  }

  void* Thread() {
    ThreadStarted();
    log_register_thread("PGP stuff");
    
    DSTACK(__FUNCTION_NAME);
    
    BEGIN_DEBUG_EXCEPTION_HANDLER      

      ;

    assert(GPG_ERR_NO_ERROR==gpgme_new(&ctx));

    std::string location(porting::path_user.c_str());
    location = location + DIR_DELIM + "keyring";
    assert(0==mkdir(location.c_str(),0700) ||
           errno == EEXIST);

    assert(GPG_ERR_NO_ERROR==
           gpgme_ctx_set_engine_info(ctx,GPGME_PROTOCOL_OpenPGP,NULL,location.c_str()));

    // note: since jthread makes them detached
    // no need to explicitly stop the thread
    // unless cleanup is needed
    // that exiting the process won't do automatically.
    // (which is generally a BAD idea)
    for(;;) {      
      Value* what;
      { JMutexAutoLock derp(queueLock);
        while(queue.size()==0) {
          //std::cerr << "Queue empty" << std::endl;
          queueLock.Unlock();
          sleep(1);
          // my kingdom for thread conditions T_T
          queueLock.Lock();
        }
        std::cerr << "YOY" << std::endl;
        what = queue.back();
        queue.pop_back();
      }      

      GenerateKeyRequest* derpGenkeyRequest = dynamic_cast<GenerateKeyRequest*>(what);
      if(derpGenkeyRequest) {
        GenerateKeyRequest& keyRequest = *(derpGenkeyRequest);
        char params[0x1000];
        snprintf(params,0x1000,
                 " <GnupgKeyParms format=\"internal\">\n"
                 "Key-Type: DSA\n"
                 "Key-Length: 1024\n"
                 "Subkey-Type: ELG-E\n"
                 "Subkey-Length: 1024\n"
                 "Name-Real: Minetest\n"
                 "Name-Comment: generated by minetest\n"
                 "Expire-Date: 0\n"
                 "Passphrase: %s\n"
                 "</GnupgKeyParms>\n",
                 keyRequest.passphrase);
        memset(keyRequest.passphrase,0,strlen(keyRequest.passphrase));
        gpg_error_t res =
          gpgme_op_genkey(ctx,params,NULL,NULL);
        memset(params,0,strlen(params));

        switch(res) {          
        case GPG_ERR_NO_ERROR:
          break;
        default:
          std::cerr << "Um genkey error " << gpgme_strerror(res) <<
            ": " << gpgme_strsource(res) << std::endl;
          throw std::runtime_error("shit genkey failed");
        }
        gpgme_genkey_result_t result = gpgme_op_genkey_result (ctx);
        GenerateKeyResponse *response = new GenerateKeyResponse();
        response->keyId = result->fpr;
        keyRequest.onResponse.set(response);
        std::cerr << "whee!" << std::endl;
        continue;
      }
               
      EncodeRequest* derpEncodeRequest = dynamic_cast<EncodeRequest*>(what);
      if(derpEncodeRequest) {
        EncodeRequest& encodeRequest = *derpEncodeRequest;
        std::cerr << "Encoding something" << std::endl;
        for(std::vector<std::string>::iterator signer = encodeRequest.signers.begin();
            signer != encodeRequest.signers.end();
            ++signer) {
          gpgme_key_t key;
          gpgme_get_key(ctx,signer->c_str(),&key,1);
          if(key)
            gpgme_signers_add(ctx,key);
          gpgme_key_unref(key);
          key = NULL;
        }
        gpgme_key_t* ekeys = NULL;

        EncodeResponse* response = new EncodeResponse();
        if(encodeRequest.encrypters.size()>0) {
          ekeys = (gpgme_key_t*) alloca(encodeRequest.encrypters.size()*sizeof(gpgme_key_t*)+1);
          ekeys[encodeRequest.encrypters.size()] = NULL;
       
          int i = 0;
          std::vector<std::string>::iterator encrypter = encodeRequest.encrypters.begin();
          bool derpError = false;
          for(;encrypter != encodeRequest.encrypters.end();
              ++i,++encrypter) {
            gpg_error_t err = gpgme_get_key(ctx,encrypter->c_str(),ekeys+i,0);
            gpg_err_code_t ec = gpgme_err_code(err);
            switch(ec) {
            case GPG_ERR_NO_ERROR:
              continue;
            case GPG_ERR_EOF:
              std::cerr << "We don't have the key for " << *encrypter << std::endl;
              response->needKeys.push_back(*encrypter);
              continue;
            default:
              std::cerr << "get key error " << err << ": " << gpgme_strerror(err) << std::endl;
              derpError = true;
            };                          
          }
          if(derpError) {
            delete response;
            continue;
          }
        }

        if(response->needKeys.size()==0) {
          assert(GPG_ERR_NO_ERROR ==
                 gpgme_data_new(&response->cipher));

          assert(GPG_ERR_NO_ERROR == 
                 gpgme_op_encrypt(ctx,ekeys,GPGME_ENCRYPT_ALWAYS_TRUST,
                                  encodeRequest.plain,response->cipher));
        }
        gpgme_signers_clear(ctx);
        encodeRequest.onResponse.set(response);
        continue;
      }

      DecodeRequest* derpDecodeRequest = dynamic_cast<DecodeRequest*>(what);
      if(derpDecodeRequest) {
        DecodeRequest& decodeRequest = *derpDecodeRequest;
        DecodeResponse* response = new DecodeResponse();
        assert(GPG_ERR_NO_ERROR==
               gpgme_data_new(&response->plain));
      
        assert(GPG_ERR_NO_ERROR==
               gpgme_op_decrypt_verify(ctx,decodeRequest.cipher,response->plain));

        // signing no data is meaningless so GPG_ERR_NO_DATA should still fail
        // gpgme_decrypt_result_t dres = gpgme_op_decrypt_result(ctx);
        gpgme_verify_result_t vres = gpgme_op_verify_result(ctx);
      
        for(gpgme_signature_t sig = vres->signatures; sig; sig = sig->next) {
          switch(sig->status) {
          case GPG_ERR_NO_PUBKEY:
            response->needKeys.push_back(sig->fpr);
          case GPG_ERR_NO_ERROR:
            continue;
          default:
            throw "Doubleplus ungood signature found!";
          };        
        }
        if(response->needKeys.size()==0) {
          for(gpgme_signature_t sig = vres->signatures; sig; sig = sig->next) {
            response->signers.push_back(sig->fpr);
          }
        } else {
          // better than freeing this outside the thread.
          gpgme_data_release(response->plain);
        }
        decodeRequest.onResponse.set(response);
        continue;
      }

      ExportRequest* derpER = dynamic_cast<ExportRequest*>(what);
      if(derpER) {
        ExportRequest& exportRequest = *derpER;
        ExportResponse* response = new ExportResponse();
        gpgme_data_new(&response->keyData);
        exportRequest.fpr[41] = 0;
        if(GPG_ERR_NO_ERROR!=
           gpgme_op_export(ctx,(const char*)exportRequest.fpr,0,response->keyData))
          std::cerr << "Warning: failed to export " << exportRequest.fpr << std::endl;
        exportRequest.onResponse.set(response);
        continue;
      }
      ImportRequest* derpIM = dynamic_cast<ImportRequest*>(what);
      if(derpIM) {
        ImportRequest& importRequest = *derpIM;
        if(GPG_ERR_NO_ERROR!=
           gpgme_op_import(ctx,importRequest.keyData))
          std::cerr << "Warning: failed to import a key." << std::endl;
        importRequest.onResponse.set(new Value()); // yeah sure, whatever
        gpgme_data_release(importRequest.keyData);
        continue;
      }
    }
    END_DEBUG_EXCEPTION_HANDLER(std::cerr) ;
  }

  MainThread() {
    /* Initialize the locale environment.  */
    setlocale (LC_ALL, "");
    gpgme_check_version (NULL);
    gpgme_set_locale (NULL, LC_CTYPE, setlocale (LC_CTYPE, NULL));
#ifdef LC_MESSAGES
    gpgme_set_locale (NULL, LC_MESSAGES, setlocale (LC_MESSAGES, NULL));
#endif

    gnupg::pgpEnabled = GPG_ERR_NO_ERROR==gpgme_engine_check_version(GPGME_PROTOCOL_OpenPGP);    
  }
};

MainThread mainThread;

namespace gnupg {
  bool PendingChallenge::matches(u8* answer, size_t size) {
    return size==NONCE_LENGTH && (0 == memcmp(solution,answer,NONCE_LENGTH));
  }

  void start() {
    if(pgpEnabled)
      mainThread.Start();
  }


  std::string mykey;
  bool pgpEnabled = false;

  void PassPhraseGetter::onPassPhrase(char* passphrase) {
    GenerateKeyRequest* request = new GenerateKeyRequest();
    request->passphrase = passphrase;
    mainThread.enqueue(request);
    GenerateKeyResponse* response = dynamic_cast<GenerateKeyResponse*>(request->onResponse.get());
    assert(response);
    delete request;
    mykey = response->keyId;
    gotIt = true;
  }

  //XXX: Jesus Christ what the fuck is this

#ifdef BUILD_CLIENT
#ifndef SERVER
  void assureMyKey(const std::string& configPath, 
                   IrrlichtDevice* device, 
                   video::IVideoDriver* driver)
#else
  void assureMyKey(const std::string& configPath, void* device, void* bleh)
#endif
#else
  void assureMyKey(const std::string& configPath, void* device, void* bleh)
#endif
  {
    if(pgpEnabled == false) {
      std::cerr << "Can't have a key since gpgme doesn't seem to exist here." << std::endl;
      return;
    }
    std::cerr << "Assuring my key exists." << std::endl;
    if(configPath.size() != 0) {
      std::cerr << "cp " << configPath << std::endl;
      std::ifstream in((configPath+".keyId").c_str());
      if(!in.fail()) {
        char buf[FINGERPRINT_LENGTH];
        in.read(buf,FINGERPRINT_LENGTH);
        if(!in.fail()) {
          mykey = std::string(buf,FINGERPRINT_LENGTH);
          in.close();
          return;
        }          
      }
      in.close();        
    }
    
    PassPhraseGetter ppg;
#ifdef BUILD_CLIENT
#ifndef SERVER
    if(device) {
      GUIPassPhraseGetter* menu = new GUIPassPhraseGetter(&ppg, guienv, guienv->getRootGUIElement(), -1, &g_menumgr);
      menu->drop();
        
      bool &kill = *porting::signal_handler_killstatus();
      while(device->run() && !kill)
        {
          if(ppg.gotIt)
            break;
            
          //driver->beginScene(true, true, video::SColor(255,0,0,0));
          driver->beginScene(true, true, video::SColor(255,255,128,128));
            
          // drawMenuBackground(driver);
            
          guienv->drawAll();
            
          driver->endScene();
            
          // On some computers framerate doesn't seem to be
          // automatically limited
          sleep_ms(25);
        }
      assert(ppg.gotIt);
      if(configPath.size() != 0) {
        std::ofstream out((configPath+".keyId").c_str());
        out.write(mykey.c_str(),FINGERPRINT_LENGTH);
        out.close();
      }
      std::cerr << "My key: " << mykey << std::endl;
      return;
    }
#endif
#endif
    struct termios t;
    tcgetattr(0, &t);
    t.c_lflag &= ~ECHO;
    tcsetattr(0, TCSANOW, &t);
    std::cout << "Please type in a passphrase to generate the server key: ";
    char passphrase[0x1000];
    std::cin.getline(passphrase,0x1000);      
    t.c_lflag |= ECHO;
    tcsetattr(0, TCSANOW, &t);
    ppg.onPassPhrase(passphrase);
    assert(ppg.gotIt);
    if(configPath.size() != 0) {
      std::ofstream out((configPath+".keyId").c_str());
      out.write(mykey.c_str(),FINGERPRINT_LENGTH);
      out.close();
    }

    std::cerr << "My key: " << mykey << std::endl;
  }
  

  std::string makeResponse(std::string challenge, Peer* const self) {
    assert(pgpEnabled);
    DecodeRequest* request = new DecodeRequest();
    assert(GPG_ERR_NO_ERROR==
           gpgme_data_new_from_mem(&request->cipher,challenge.c_str(),challenge.size(),0));
    DecodeResponse* response = NULL;
    for(int i = 0;i < 10;++i) {
      mainThread.enqueue(request);
      response = dynamic_cast<DecodeResponse*>(request->onResponse.get());
      if(response->needKeys.size()==0) break;
      self->missingKeys(response->needKeys);
      delete response;
    }
    gpgme_data_release(request->cipher);
    delete request;
    if(response->needKeys.size()>0)
      throw std::runtime_error("Couldn't import key of signature of response");
    size_t size;
    char* resp = gpgme_data_release_and_get_mem(response->plain,&size);
    std::string derp(resp,size);
    gpgme_free(resp);
    delete response;
    return derp;
  }
  PendingChallenge* makeChallenge(std::string who, Peer* const self) {
    PendingChallenge* pc = new PendingChallenge(who);
    for(int i = 0; i < NONCE_LENGTH; ++i) {
      pc->solution[i] = myrand() % 0x100;
    }
    // note, do NOT have this on the stack
    // if this thread dies, the gpgme thread will be unable to tell
    // the pointer has gone bad!
    EncodeRequest* request = new EncodeRequest();
    assert(GPG_ERR_NO_ERROR==
           gpgme_data_new_from_mem(&request->plain,
                                   (const char*)pc->solution,NONCE_LENGTH,0));

    request->encrypters.push_back(who);
    EncodeResponse* response = NULL;
    mainThread.enqueue(request);
    response = dynamic_cast<EncodeResponse*>(request->onResponse.get());
    assert(response);
    if(response->needKeys.size()>0) {
      // don't loop when making a challenge if keys are missing
      // make an entirely fresh challenge l8r
      self->missingKeys(response->needKeys);
      // response->cipher not allocated if keys are needed
      delete response;
      return NULL;
    }
    gpgme_data_release(request->plain);
    delete request;
    size_t size;
    char* resp = gpgme_data_release_and_get_mem(response->cipher,&size);
    pc->question = std::string(resp,size);
    gpgme_free(resp);
    delete response;
    return pc;
  }

  std::string exportKey(u8* fpr) {
    std::cerr << "Export a key yay " << std::string((const char*)fpr,FINGERPRINT_LENGTH) << std::endl;
    ExportRequest* request = new ExportRequest();
    request->fpr = fpr;
    mainThread.enqueue(request);
    ExportResponse* response = dynamic_cast<ExportResponse*>(request->onResponse.get());
    assert(response);
    delete request;
    size_t size;
    char* resp = gpgme_data_release_and_get_mem(response->keyData,&size);
    std::string derp(resp,size);
    gpgme_free(resp);
    delete response;
    return derp;
  }

  void importKey(u8* keyData, size_t size) {
    std::cerr << "Import a key yay" << std::endl;
    ImportRequest* request = new ImportRequest();
    gpgme_data_new_from_mem(&request->keyData,
                            (const char*)keyData,size,0);
    mainThread.enqueue(request);
    Value* response = request->onResponse.get();
    assert(response);
    delete request;
    delete response;
  }

};

/* the idea is: instead of a user/password, the client sends 
   their key fingerprint. Upon receiving it, server passes it to makeChallenge,
   and sends the question to the client, saving the pending challenge for its 
   nonce. The client decrypts and sends the decrypted nonce. The server 
   compares and they match. Everything is now hunky dory.

   No passwords needed.
*/
