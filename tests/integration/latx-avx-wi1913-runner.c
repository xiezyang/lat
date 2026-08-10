#include <stdint.h>
#include <stdio.h>

#ifndef WI1913_SYMBOL
#error "WI1913_SYMBOL is required"
#endif

#ifndef WI1913_BYTES
#error "WI1913_BYTES is required"
#endif

extern void WI1913_SYMBOL(uint8_t *output);

int main(void)
{
    uint8_t output[2048] = { 0 };

    WI1913_SYMBOL(output);
    return fwrite(output, 1, WI1913_BYTES, stdout) == WI1913_BYTES ? 0 : 1;
}
