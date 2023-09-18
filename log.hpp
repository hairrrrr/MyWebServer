#pragma once

#include <cstdio>
#include <iostream>
#include <string>
#include <cstring>
#include <cstdarg>

#define DEBUG   0
#define INFO    1
#define WARNING 2
#define ERROR   3
#define FATAL   4

#define DEBUG_MODE   1
#define RELEASE_MODE 0

#define LOG_FILE "./.log"

#define LOGFILE_SHOW 1

#define LOG(level, debug_mode, error_number)\
	log(__LINE__, __FILE__, #level, debug_mode, error_number)

#define LOGMESSAGE(level, debug_mode, format, args...)\
	logMessage(__LINE__, __FILE__, #level, debug_mode, format, ##args)

   
static inline 
void log(int line, const std::string file, const std::string level, 
         int debugMode, int error_number)
{
    if( debugMode == RELEASE_MODE ) return;

    std::string str;
    str = "[" + level + "] " + "{" + file + ":" 
          + std::to_string(line) + "} " + "(" + strerror(error_number) + ")";

    std::cout << str << "\n";
#ifdef LOGFILE_SHOW 
    FILE* fp = fopen(LOG_FILE, "a");
    fprintf(fp, "%s\n", str.c_str());
#endif
}

static inline 
void logMessage(int line, const char* file, const char* level, 
                int debugMode, const char* format, ...)
{
    if( debugMode == RELEASE_MODE ) return;

    char stdBuffer[1024] = {0};
    snprintf(stdBuffer, sizeof stdBuffer, "[%s] {%d:%s}", level, line, file);

    char logBuffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(logBuffer, sizeof logBuffer, format, args);
    va_end(args);

#ifdef LOGFILE_SHOW
    FILE* fp = fopen(LOG_FILE, "a");
    fprintf(fp, "%s %s\n", stdBuffer, logBuffer);
#endif
    
    printf("%s %s\n", stdBuffer, logBuffer);
}

inline 
void log_quit(int error_code, int debug_mode, int error_number, int exit_number)
{
    LOG(error_code, debug_mode, error_number);
    exit(exit_number);   
}
