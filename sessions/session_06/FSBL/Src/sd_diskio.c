#include "sd_diskio.h"

/* SD handler declared in main.c or sd_logger.c */
#include <stdio.h>

extern SD_HandleTypeDef hsd2;
static volatile DSTATUS Stat = STA_NOINIT;

DSTATUS disk_initialize(BYTE pdrv) {
    printf("disk_initialize: starting HAL_SD_Init...\r\n");
    if (pdrv != 0) {
        printf("disk_initialize: wrong pdrv %d\r\n", pdrv);
        return STA_NOINIT;
    }
    
    HAL_StatusTypeDef status = HAL_SD_Init(&hsd2);
    printf("disk_initialize: HAL_SD_Init returned %d\r\n", status);
    
    if (status == HAL_OK) {
        Stat &= ~STA_NOINIT;
        return 0;
    }
    return STA_NOINIT;
}

DSTATUS disk_status(BYTE pdrv) {
    if (pdrv != 0) return STA_NOINIT;
    return Stat;
}

DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT count) {
    if (pdrv != 0 || (Stat & STA_NOINIT)) return RES_NOTRDY;
    
    HAL_StatusTypeDef status = HAL_SD_ReadBlocks(&hsd2, buff, sector, count, 1000);
    
    if (status == HAL_OK) {
        uint32_t timeout = 1000000;
        while (HAL_SD_GetCardState(&hsd2) != HAL_SD_CARD_TRANSFER) {
            timeout--;
            if (timeout == 0) {
                return RES_ERROR;
            }
        }
        return RES_OK;
    }
    return RES_ERROR;
}

DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count) {
    if (pdrv != 0 || (Stat & STA_NOINIT)) return RES_NOTRDY;
    
    HAL_StatusTypeDef status = HAL_SD_WriteBlocks(&hsd2, (uint8_t*)buff, sector, count, 1000);
    
    if (status == HAL_OK) {
        uint32_t timeout = 1000000;
        while (HAL_SD_GetCardState(&hsd2) != HAL_SD_CARD_TRANSFER) {
            timeout--;
            if (timeout == 0) {
                return RES_ERROR;
            }
        }
        return RES_OK;
    }
    return RES_ERROR;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff) {
    DRESULT res = RES_ERROR;
    HAL_SD_CardInfoTypeDef CardInfo;

    if (HAL_SD_GetState(&hsd2) != HAL_SD_STATE_TRANSFER) return RES_NOTRDY;

    switch (cmd) {
        case CTRL_SYNC:
            res = RES_OK;
            break;
        case GET_SECTOR_COUNT:
            HAL_SD_GetCardInfo(&hsd2, &CardInfo);
            *(DWORD*)buff = CardInfo.LogBlockNbr;
            res = RES_OK;
            break;
        case GET_SECTOR_SIZE:
            HAL_SD_GetCardInfo(&hsd2, &CardInfo);
            *(WORD*)buff = CardInfo.LogBlockSize;
            res = RES_OK;
            break;
        case GET_BLOCK_SIZE:
            HAL_SD_GetCardInfo(&hsd2, &CardInfo);
            *(DWORD*)buff = CardInfo.LogBlockSize / 512;
            res = RES_OK;
            break;
        default:
            res = RES_PARERR;
    }
    return res;
}
