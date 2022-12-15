#pragma once

#include <stdio.h>

class NodeDefManager;

#define SSCSM_SCRIPT_READ_FD 3
#define SSCSM_SCRIPT_WRITE_FD 4

extern FILE *g_sscsm_from_controller;
extern FILE *g_sscsm_to_controller;

extern NodeDefManager *g_sscsm_nodedef;

int sscsm_script_main(int argc, char *argv[]);
