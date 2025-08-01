#include "system.h"
#include <esp_system.h>


void bsp_system_reset(void) {
    esp_restart();
}
