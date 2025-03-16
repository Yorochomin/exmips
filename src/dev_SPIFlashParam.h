#ifndef __DEV_SPIFLASHPARAM_H__ 
#define __DEV_SPIFLASHPARAM_H__ 

#include <stdint.h>

struct stSPIFlashParam {
    uint32_t capacity;
    uint32_t block_size;
    uint32_t sector_size;
    uint32_t page_size;
    uint32_t device_id;
    uint8_t manufacturer_id;
    uint8_t devicetype_id;
    uint8_t capacity_id;
    const uint8_t *sfdp;
    uint32_t sfdp_size;
};

extern const struct stSPIFlashParam spiflashparam_s25fl164k;
extern const struct stSPIFlashParam spiflashparam_mx66u2g45g;

#endif
