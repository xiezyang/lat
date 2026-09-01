#include "qemu/osdep.h"
#include <cxxabi.h>

#ifdef CONFIG_LATX_DEBUG
extern "C"
const char *latx_demangling(char *symbol)
{
    static thread_local char *buffer;
    static thread_local size_t buffer_size;
    int status;

    if (!symbol) {
        return "(null)";
    }
    char *ret = abi::__cxa_demangle(symbol, buffer, &buffer_size, &status);
    if (ret) {
        buffer = ret;
    }
    return (status == 0) ? ret : symbol;
}
#endif
