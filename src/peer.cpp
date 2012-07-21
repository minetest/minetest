#include "peer.h"

#ifdef GPGME_EXISTS
#include "gnupg.h"
#endif
#include <iostream>

Peer::Peer() : m_con(PROTOCOL_ID, 512, CONNECTION_TIMEOUT, this) {}
Peer::~Peer() {
  //JMutexAutoLock conlock(m_con_mutex); //bulk comment-out
  m_con.Disconnect();
}


#ifdef GPGME_EXISTS

void Peer::SendImport(u16 peer_id,
                      std::string fpr) {
  std::cerr << "FPR size " << fpr.size() << std::endl;
  SharedBuffer<u8> data(fpr.size()+2);
  writeU16(&data[0], TOPEER_EXPORT);
  memcpy(&data[2],fpr.c_str(),fpr.size());
  m_con.Send(peer_id, 0, data, true);
}
void Peer::missingKeys(const std::vector<std::string>& fprs) {
  for(std::vector<std::string>::const_iterator i = fprs.begin();
      i != fprs.end();
      ++i) {
    SendImport(m_last_peer_id, *i);
    sleep(1);
  }
}
#endif /* GPGME_EXISTS */

bool Peer::peerProcessData(u16 command, u8 *data, u32 datasize, u16 peer_id) {
#ifdef GPGME_EXISTS
  if(command == TOPEER_EXPORT) {
    if(datasize != 40+2) return false;
    std::string key = gnupg::exportKey(&data[2]);
    SharedBuffer<u8> reply(key.size()+2);
    writeU16(&reply[0],TOPEER_IMPORT);
    memcpy(&reply[2],key.c_str(),key.size());
    m_con.Send(peer_id, 1, reply, true);
    return true;
  } 
  if(command == TOPEER_IMPORT) {
    // need a delay in here somehow...
    if(datasize > 0x10000) return false;
    gnupg::importKey(&data[2],datasize-2);
    return true;
  }
#endif
  return false;
}
