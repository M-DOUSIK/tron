#ifndef INC_DEBUG_LOG_H_
#define INC_DEBUG_LOG_H_

#include <stdio.h>

/* Set this to 1 to enable debug logging, or 0 to disable it globally */
#define ENABLE_DEBUG_LOG 1

#if ENABLE_DEBUG_LOG
    #define DEBUG_LOG(...) printf(__VA_ARGS__)
#else
    #define DEBUG_LOG(...) do {} while(0)
#endif

#endif /* INC_DEBUG_LOG_H_ */
