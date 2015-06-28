#ifndef DEBUG_H
#define DEBUG_H

#define DEBUG_INFO(fmt, args...) app_log(APP_LOG_LEVEL_INFO, __FILE__, __LINE__, fmt, ## args);

#endif /* DEBUG_H */
