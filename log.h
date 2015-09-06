#ifndef __LOG_H
#define __LOG_H

#define str(s)  #s
#define XSTR(s) str(s)
#define LOGPREF __FILE__":"xstr(__LINE__)

#ifdef dprintf
#undef dprintf
#endif

#define SYSLOG(fmt, args...) \
        syslog(LOG_USER | LOG_NDELAY | LOG_INFO, fmt, ##args)

#define dprintf(format, args...) do {                   \
        fprintf(stderr, format, ##args);                \
} while (0)

/**
 * @brief print and die
 */
#define FATAL(format, ...) do {                 \
        fprintf(stderr, format, ##__VA_ARGS__); \
        exit(EXIT_FAILURE);                     \
} while (0)



#endif /* __LOG_H */
