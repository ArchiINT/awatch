#include <stdint.h>
#define BATTERY

typedef struct{
    uint16_t IconWidth;
    uint16_t IconHeight;
    const uint16_t *data;
} IconDef_t;

#ifdef BATTERY
extern IconDef_t Battery;
#endif