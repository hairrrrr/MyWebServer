#pragma once

#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <cstring>
#include <cstdlib>
#include <cstdio>

#include "../log.hpp"
#include "error_number.h"

#define LOG_MODE DEBUG_MODE