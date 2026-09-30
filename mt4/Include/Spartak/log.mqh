// =============================================================================
//  SPARTAK :: Spartak/log.mqh
// =============================================================================
#ifndef SPARTAK_LOG_MQH
#define SPARTAK_LOG_MQH

#include <Spartak/contracts.mqh>

bool   g_log_enabled = true;
string g_log_dir     = "Spartak";
int    g_log_level   = SPK_LOG_INFO;

void log_configure(bool enabled, string dir, int level)
{
    g_log_enabled = enabled;
    g_log_dir     = dir;
    g_log_level   = level;
}

string log_level_name(int level)
{
    if (level == SPK_LOG_ERROR) return "ERROR";
    if (level == SPK_LOG_WARN)  return "WARN";
    if (level == SPK_LOG_INFO)  return "INFO";
    if (level == SPK_LOG_DEBUG) return "DEBUG";
    return "?";
}

void log_write(int level, string msg)
{
    if (!g_log_enabled) return;
    if (level > g_log_level) return;

    string date = TimeToString(TimeCurrent(), TIME_DATE);
    StringReplace(date, ".", "");
    string fname = g_log_dir + "\\log_" + date + ".csv";

    int h = FileOpen(fname, FILE_READ | FILE_WRITE | FILE_CSV | FILE_ANSI, ',');
    if (h == INVALID_HANDLE) return;

    FileSeek(h, 0, SEEK_END);
    FileWrite(h,
              TimeToString(TimeCurrent(), TIME_DATE | TIME_SECONDS),
              log_level_name(level),
              msg);
    FileClose(h);
}

#endif