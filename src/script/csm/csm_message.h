#pragma once

#include "irrlichttypes.h"
#include "util/Optional.h"
#include <stdio.h>
#include <vector>

enum class CSMMsgType {
	// controller -> script
	C2S_RUN_STEP,
	C2S_RUN_LOAD_MODS,
	C2S_GET_NODE,

	// script -> controller
	S2C_DONE,
	S2C_GET_NODE,
};

struct CSMRecvMsg {
	CSMMsgType type;
	std::vector<u8> data;
};

bool csm_send_msg(FILE *f, CSMMsgType type, size_t size, const void *data);

Optional<CSMRecvMsg> csm_recv_msg(FILE *f);

void csm_send_msg_ex(FILE *f, CSMMsgType type, size_t size, const void *data);

CSMRecvMsg csm_recv_msg_ex(FILE *f);
