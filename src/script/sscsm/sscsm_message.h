#pragma once

#include "irrlichttypes.h"
#include "util/Optional.h"
#include <stdio.h>
#include <vector>

enum class SSCSMMsgType {
	// controller -> script
	C2S_RUN_STEP,
	C2S_RUN_LOAD_MODS,
	C2S_GET_NODE,

	// script -> controller
	S2C_DONE,
	S2C_GET_NODE,
};

struct SSCSMRecvMsg {
	SSCSMMsgType type;
	std::vector<u8> data;
};

bool sscsm_send_msg(FILE *f, SSCSMMsgType type, size_t size, const void *data);

Optional<SSCSMRecvMsg> sscsm_recv_msg(FILE *f);

void sscsm_send_msg_ex(FILE *f, SSCSMMsgType type, size_t size, const void *data);

SSCSMRecvMsg sscsm_recv_msg_ex(FILE *f);
