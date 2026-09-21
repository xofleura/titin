#include "info.h"

const char *system_info_version(void)
{
    return "0.1";
}

const char *system_info_architecture(void)
{
#if defined(__x86_64__)
    return "x86_64";
#elif defined(__i386__)
    return "x86";
#elif defined(__aarch64__)
    return "aarch64";
#elif defined(__arm__)
    return "arm";
#else
    return "unknown";
#endif
}
