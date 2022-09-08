#ifndef RE_PHY
#define RE_PHY

#include <stdint.h>

void rfc_init();
void phy_init();
void phy_hw_set_channel(uint16_t freq);

#endif
