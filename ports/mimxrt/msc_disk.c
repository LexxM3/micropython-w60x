/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020-2021 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */
#include "tusb.h"
#if CFG_TUD_MSC
#include "flash.h"
#include BOARD_FLASH_OPS_HEADER_H
#include "stdlib.h"

// This implementation does Not support Flash sector caching.
// MICROPY_FATFS_MAX_SS  must be identical to SECTOR_SIZE_BYTES
#define BLOCK_SIZE          (SECTOR_SIZE_BYTES)
#define BLOCK_COUNT         (MICROPY_HW_FLASH_STORAGE_BYTES / BLOCK_SIZE)
#define FLASH_BASE_ADDR     (MICROPY_HW_FLASH_STORAGE_BASE)

uint8_t tud_msc_state = EJECTED;

static bool msc_enabled = true;

void update_msc_state(void) {
    if (tud_msc_state == TRANSIT) {
        tud_msc_state = EJECTED;
    }
}

void set_msc_enabled(bool state) {
    msc_enabled = state;
}

// Invoked when received SCSI_CMD_INQUIRY
// Application fill vendor id, product id and revision with string up to 8, 16, 4 characters respectively
void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4]) {
    if (msc_enabled) {
        const char vid[] = "Micropy";
        const char pid[] = "Mass Storage";
        const char rev[] = "1.0";

        strncpy((char *)vendor_id,   vid, 8);
        strncpy((char *)product_id,  pid, 16);
        strncpy((char *)product_rev, rev, 4);
        tud_msc_state = MOUNTED;
    }
}

// Invoked when received Test Unit Ready command.
// return true allowing host to read/write this LUN e.g SD card inserted
bool tud_msc_test_unit_ready_cb(uint8_t lun) {
    if (tud_msc_state != MOUNTED || !msc_enabled) {
        tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, 0x3a, 0x00);
        return false;
    }
    return true;
}

// Invoked when received SCSI_CMD_READ_CAPACITY_10 and SCSI_CMD_READ_FORMAT_CAPACITY to determine the disk size
// Application update block count and block size
void tud_msc_capacity_cb(uint8_t lun, uint32_t *block_count, uint16_t *block_size) {
    if (msc_enabled) {
        *block_size = BLOCK_SIZE;
        *block_count = BLOCK_COUNT;
    }
}

// Invoked when received Start Stop Unit command
// - Start = 0 : stopped power mode, if load_eject = 1 : unload disk storage
// - Start = 1 : active mode, if load_eject = 1 : load disk storage
bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject) {
    if (load_eject && msc_enabled) {
        if (start) {
            // load disk storage
            tud_msc_state = MOUNTED;
        } else {
            // unload disk storage
            tud_msc_state = TRANSIT;
        }
    }
    return true;
}

// Callback invoked when received READ10 command.
// Copy disk's data to buffer (up to bufsize) and return number of copied bytes.
int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize) {
    flash_read_block(FLASH_BASE_ADDR + lba * BLOCK_SIZE, buffer, bufsize);
    return bufsize;
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and return number of written bytes
int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize) {
    uint32_t count = bufsize / BLOCK_SIZE;
    // Erase count sectors starting at lba
    for (int n = 0; n < count; n++) {
        flash_erase_sector(FLASH_BASE_ADDR + (lba + n) * BLOCK_SIZE);
    }
    flash_write_block(FLASH_BASE_ADDR + lba * BLOCK_SIZE, buffer, count * BLOCK_SIZE);
    return count * BLOCK_SIZE;
}

// Callback invoked when received an SCSI command not in built-in list below
// - READ_CAPACITY10, READ_FORMAT_CAPACITY, INQUIRY, MODE_SENSE6, REQUEST_SENSE
// - READ10 and WRITE10 has their own callbacks
int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void *buffer, uint16_t bufsize) {
    int32_t resplen = 0;
    switch (scsi_cmd[0]) {
        case SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL:
            // Sync the logical unit if needed.
            break;

        default:
            // Set Sense = Invalid Command Operation
            tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);
            // negative means error -> tinyusb could stall and/or response with failed status
            resplen = -1;
            break;
    }
    return resplen;
}

#endif