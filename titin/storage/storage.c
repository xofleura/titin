#include <stdint.h>
#include "storage.h"
#include "../filesystem/tfs.h"

#define ATA_DATA 0x1F0
#define ATA_ERROR 0x1F1
#define ATA_SECTOR_COUNT 0x1F2
#define ATA_LBA_LOW 0x1F3
#define ATA_LBA_MID 0x1F4
#define ATA_LBA_HIGH 0x1F5
#define ATA_DRIVE 0x1F6
#define ATA_STATUS 0x1F7
#define ATA_COMMAND 0x1F7

#define ATA_STATUS_ERROR 0x01
#define ATA_STATUS_DRQ 0x08
#define ATA_STATUS_READY 0x40
#define ATA_STATUS_BUSY 0x80

#define ATA_COMMAND_READ 0x20
#define ATA_COMMAND_WRITE 0x30
#define ATA_COMMAND_IDENTIFY 0xEC

#define STORAGE_SECTORS \
    TFS_STORAGE_SECTOR_COUNT

static uint8_t storage_buffer[
    TFS_STORAGE_SIZE
];

static inline void io_out8(
    uint16_t port,
    uint8_t value
)
{
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}

static inline uint8_t io_in8(
    uint16_t port
)
{
    uint8_t value;

    __asm__ volatile (
        "inb %1, %0"
        : "=a"(value)
        : "Nd"(port)
    );

    return value;
}

static inline void io_out16(
    uint16_t port,
    uint16_t value
)
{
    __asm__ volatile (
        "outw %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}

static inline uint16_t io_in16(
    uint16_t port
)
{
    uint16_t value;

    __asm__ volatile (
        "inw %1, %0"
        : "=a"(value)
        : "Nd"(port)
    );

    return value;
}

static void ata_wait_400ns(void)
{
    io_in8(ATA_STATUS);
    io_in8(ATA_STATUS);
    io_in8(ATA_STATUS);
    io_in8(ATA_STATUS);
}

static int ata_wait_ready(void)
{
    for (uint64_t i = 0;
         i < 1000000;
         i++) {
        uint8_t status =
            io_in8(ATA_STATUS);

        if (status &
            ATA_STATUS_ERROR) {
            return 0;
        }

        if (!(status &
              ATA_STATUS_BUSY) &&
            (status &
             ATA_STATUS_READY)) {
            return 1;
        }
    }

    return 0;
}

static int ata_wait_drq(void)
{
    for (uint64_t i = 0;
         i < 1000000;
         i++) {
        uint8_t status =
            io_in8(ATA_STATUS);

        if (status &
            ATA_STATUS_ERROR) {
            return 0;
        }

        if (status &
            ATA_STATUS_DRQ) {
            return 1;
        }

        if (!(status &
              ATA_STATUS_BUSY) &&
            !(status &
              ATA_STATUS_READY)) {
            return 0;
        }
    }

    return 0;
}

static int ata_identify(void)
{
    io_out8(
        ATA_DRIVE,
        0xA0
    );

    io_out8(
        ATA_SECTOR_COUNT,
        0
    );

    io_out8(
        ATA_LBA_LOW,
        0
    );

    io_out8(
        ATA_LBA_MID,
        0
    );

    io_out8(
        ATA_LBA_HIGH,
        0
    );

    io_out8(
        ATA_COMMAND,
        ATA_COMMAND_IDENTIFY
    );

    uint8_t status =
        io_in8(ATA_STATUS);

    if (status == 0) {
        return 0;
    }

    for (uint64_t i = 0;
         i < 1000000;
         i++) {
        status =
            io_in8(ATA_STATUS);

        if (status &
            ATA_STATUS_ERROR) {
            return 0;
        }

        if (status &
            ATA_STATUS_DRQ) {
            break;
        }

        if (status == 0) {
            return 0;
        }
    }

    if (!(status &
          ATA_STATUS_DRQ)) {
        return 0;
    }

    for (uint64_t i = 0;
         i < 256;
         i++) {
        io_in16(ATA_DATA);
    }

    return 1;
}

static int ata_read_sector(
    uint32_t lba,
    uint8_t *buffer
)
{
    if (!ata_wait_ready()) {
        return 0;
    }

    io_out8(
        ATA_DRIVE,
        0xE0 |
        ((lba >> 24) & 0x0F)
    );

    io_out8(
        ATA_SECTOR_COUNT,
        1
    );

    io_out8(
        ATA_LBA_LOW,
        lba & 0xFF
    );

    io_out8(
        ATA_LBA_MID,
        (lba >> 8) & 0xFF
    );

    io_out8(
        ATA_LBA_HIGH,
        (lba >> 16) & 0xFF
    );

    io_out8(
        ATA_COMMAND,
        ATA_COMMAND_READ
    );

    if (!ata_wait_drq()) {
        return 0;
    }

    for (uint64_t i = 0;
         i < 256;
         i++) {
        uint16_t value =
            io_in16(ATA_DATA);

        buffer[i * 2] =
            value & 0xFF;

        buffer[i * 2 + 1] =
            value >> 8;
    }

    ata_wait_400ns();

    return 1;
}

static int ata_write_sector(
    uint32_t lba,
    const uint8_t *buffer
)
{
    if (!ata_wait_ready()) {
        return 0;
    }

    io_out8(
        ATA_DRIVE,
        0xE0 |
        ((lba >> 24) & 0x0F)
    );

    io_out8(
        ATA_SECTOR_COUNT,
        1
    );

    io_out8(
        ATA_LBA_LOW,
        lba & 0xFF
    );

    io_out8(
        ATA_LBA_MID,
        (lba >> 8) & 0xFF
    );

    io_out8(
        ATA_LBA_HIGH,
        (lba >> 16) & 0xFF
    );

    io_out8(
        ATA_COMMAND,
        ATA_COMMAND_WRITE
    );

    if (!ata_wait_drq()) {
        return 0;
    }

    for (uint64_t i = 0;
         i < 256;
         i++) {
        uint16_t value =
            buffer[i * 2] |
            ((uint16_t)buffer[i * 2 + 1] << 8);

        io_out16(
            ATA_DATA,
            value
        );
    }

    ata_wait_400ns();

    return ata_wait_ready();
}

int storage_start(void)
{
    return ata_identify();
}

int storage_load(void)
{
    if (!storage_start()) {
        return 0;
    }

    for (uint64_t sector = 0;
         sector < STORAGE_SECTORS;
         sector++) {
        if (!ata_read_sector(
                sector,
                storage_buffer +
                sector *
                TFS_STORAGE_SECTOR_SIZE
            )) {
            return 0;
        }
    }

    if (!tfs_storage_import(
            storage_buffer,
            TFS_STORAGE_SIZE
        )) {
        return 0;
    }

    return 1;
}

int storage_save(void)
{
    if (!storage_start()) {
        return 0;
    }

    if (!tfs_storage_export(
            storage_buffer,
            TFS_STORAGE_SIZE
        )) {
        return 0;
    }

    for (uint64_t sector = 0;
         sector < STORAGE_SECTORS;
         sector++) {
        if (!ata_write_sector(
                sector,
                storage_buffer +
                sector *
                TFS_STORAGE_SECTOR_SIZE
            )) {
            return 0;
        }
    }

    return 1;
}
