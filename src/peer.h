#ifndef PEER_HEADER
#define PEER_HEADER

#include "gamedef.h"
#include "inventorymanager.h"
#include "connection.h"
#include "clientserver.h"

#include "config.h"

#include <string>
#include <vector>

class Peer : public con::PeerHandler, public InventoryManager, public IGameDef {
  // Connection
 protected:
  con::Connection m_con;
  JMutex m_con_mutex;
  u16 m_last_peer_id;
 public:
  Peer();
  virtual ~Peer();
  bool peerProcessData(u16 type, u8 *data, u32 datasize, u16 peer_id);
#ifdef GPGME_EXISTS
  void SendImport(u16 peer_id,
                  std::string fpr);
  void missingKeys(const std::vector<std::string>& fprs);
#endif
};

#endif /* PEER_HEADER */
