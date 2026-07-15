/*
 * DFU Application Implementation
 *
 * Implements DFU operations including flash read/write
 *  and TinyUSB DFU callbacks.
 */

#include <string.h>
#include <third_party/tinyusb/src/tusb.h>
#include "dfu_app.h"
#include "driverlib/dl_flashctl.h"

//--------------------------------------------------------------------+
// Flash Memory Operations (Platform Specific - Template)
//--------------------------------------------------------------------+

static bool flash_read(uint32_t addr, uint8_t *data, uint32_t length)
{
    /* 1) address must be inside the flash window */
    if (addr < DFU_FLASH_START_ADDR || (addr + length) > DFU_FLASH_END_ADDR) {
        return false;
    }

    /* 2) the flash controller on the MSPM0+ can only be accessed on
     *    32bit boundaries when the core clock is < flash clock.
     *    Enforce the alignment of the caller (DFU upload) already guarantees
     *    that the start address is word aligned. */
    if ((addr & 0x3U) != 0U) {
        return false;
    }

    /* 3) simple memcpy works because flash is memory mapped. */
    memcpy(data, (const void *)addr, length);
    return true;
}

//--------------------------------------------------------------------+
// Application API Implementation
//--------------------------------------------------------------------+

bool dfu_app_init(void)
{
    /* init device stack on configured roothub port */
    tusb_rhport_init_t dev_init = {
      .role = TUSB_ROLE_DEVICE,
      .speed = TUSB_SPEED_AUTO
    };

    // Initialize TinyUSB
    tusb_init(BOARD_TUD_RHPORT, &dev_init);

    if (dfu_state.initialized) {
    return true;  // Already initialized
    }

    // Initialize state
    memset(&dfu_state, 0, sizeof(dfu_app_state_t));

    dfu_state.current_address = DFU_FLASH_START_ADDR;
    dfu_state.read_address = DFU_FLASH_START_ADDR;
    dfu_state.initialized = true;

    return true;
}

bool dfu_app_deinit(void)
{
  if (!dfu_state.initialized) {
    return true;  // Already deinitialized
  }

  // Clean up resources
  memset(&dfu_state, 0, sizeof(dfu_app_state_t));

  return true;
}

bool dfu_app_receive(uint16_t block_num, uint8_t const *data, uint16_t length)
{
    if (!dfu_state.initialized) return false;

    if (block_num == 0) {
        dfu_state.current_address      = DFU_FLASH_START_ADDR;
        dfu_state.total_bytes_received = 0;
        dfu_state.last_erased_sector   = 0xFFFFFFFF; // invalidate

        //Pre-erase entire DFU flash region upfront
        uint32_t erase_addr = DFU_FLASH_START_ADDR;
        while (erase_addr < DFU_FLASH_END_ADDR) {
            DL_FlashCTL_eraseMemoryFromRAM(FLASHCTL, erase_addr,
                                           DL_FLASHCTL_COMMAND_SIZE_SECTOR);
            erase_addr += DFU_FLASH_SECTOR_SIZE;
        }
    }

    uint32_t target_addr = dfu_state.current_address;

    if (target_addr < DFU_FLASH_START_ADDR ||
        (target_addr + length) > DFU_FLASH_END_ADDR) {
        return false;
    }

    // Erase every sector this block touches
    uint32_t erase_addr = (target_addr / DFU_FLASH_SECTOR_SIZE) * DFU_FLASH_SECTOR_SIZE;
    uint32_t end_addr   = target_addr + length;

    while (erase_addr < end_addr) {
        if (erase_addr != dfu_state.last_erased_sector) {
            DL_FlashCTL_eraseMemoryFromRAM(FLASHCTL, erase_addr,
                                           DL_FLASHCTL_COMMAND_SIZE_SECTOR);
            dfu_state.last_erased_sector = erase_addr;
        }
        erase_addr += DFU_FLASH_SECTOR_SIZE;
    }

    // Program in 64-bit chunks
    if (!DL_FlashCTL_programMemoryBlocking64WithECCGenerated(
            FLASHCTL,
            target_addr,
            (uint32_t*)data,
            length / 4,
            DL_FLASHCTL_REGION_SELECT_MAIN)) {
        return false;
    }

    dfu_state.current_address      = target_addr + length;
    dfu_state.total_bytes_received += length;
    return true;
}

uint16_t dfu_app_send(uint8_t *data, uint16_t length)
{

    if (!dfu_state.initialized) {
    return 0;
    }

    /* --------------------------------------------------------------
    *  Compute the absolute source address.
    * -------------------------------------------------------------- */
    uint32_t src = dfu_state.read_address;

    /* --------------------------------------------------------------
    *  If the host asks for a zero length block, it is the termination
    *  request.
    * -------------------------------------------------------------- */
    if (length == 0U) {
      dfu_state.read_address = DFU_FLASH_START_ADDR;
      return 0U;
    }

    /* --------------------------------------------------------------
    *  Clip the request to the end of the flash region.  Upload always
    *  reads from DFU_FLASH_START_ADDR to DFU_FLASH_END_ADDR regardless
    *  of how much was downloaded in the current session.  The host
    *  controls how many bytes it wants via the -U file size; a short
    *  (or zero-length) response on the final block signals EOF.
    * -------------------------------------------------------------- */
    uint32_t remaining = (DFU_FLASH_READ_END_ADDR > src)
                         ? (DFU_FLASH_READ_END_ADDR - src)
                         : 0U;
    uint16_t copy_len = (remaining < length) ? (uint16_t)remaining : length;

    /* --------------------------------------------------------------
    *  Perform the read.  If anything goes wrong we return 0 which makes
    *  TinyUSB set the DFU status to errADDRESS (0x03).
    * -------------------------------------------------------------- */
    if (!flash_read(src, data, copy_len)) {
      dfu_state.read_address = DFU_FLASH_START_ADDR;
      return 0U;                       // error â†’ dfuERROR
    }


    /* --------------------------------------------------------------
    *  Update our own bookkeeping (purely for diagnostics).
    * -------------------------------------------------------------- */
    dfu_state.total_bytes_sent += copy_len;
    dfu_state.read_address += copy_len;


    if (copy_len < length) {
      dfu_state.read_address = DFU_FLASH_START_ADDR;
    }

    return copy_len;
}

//--------------------------------------------------------------------+
// TinyUSB DFU Callbacks
//--------------------------------------------------------------------+

// Invoked right before tud_dfu_download_cb() or tud_dfu_manifest_cb()
// Return timeout in milliseconds for the next operation
uint32_t tud_dfu_get_timeout_cb(uint8_t alt, uint8_t state)
{
  (void)alt;
  if (state == DFU_DNBUSY) {
    // During download/busy state, provide timeout for next operation
    // For flash-only device, use consistent timeout
    return 1;
  } else if (state == DFU_MANIFEST) {
    // No timeout needed during manifest phase
    return 0;
  } else {
    // Default timeout for other states
    return 0;
  }
}

// Invoked when received DFU_DNLOAD (wLength>0) following by DFU_GETSTATUS
// This callback is called after the DFU state machine transitions to DFU_DNBUSY
void tud_dfu_download_cb(uint8_t alt, uint16_t block_num, uint8_t const *data, uint16_t length)
{
  (void) alt;  // alt can be used for multiple partitions

  // Receive and write firmware data
  bool success = dfu_app_receive(block_num, data, length);

  // Notify TinyUSB of completion
  if (success) {
    tud_dfu_finish_flashing(DFU_STATUS_OK);
  } else {
    tud_dfu_finish_flashing(DFU_STATUS_ERR_WRITE);
  }
}

// Invoked when download process is complete (received DFU_DNLOAD with wLength=0)
// This is the manifest phase where we can verify the firmware
void tud_dfu_manifest_cb(uint8_t alt)
{
  (void) alt;

  tud_dfu_finish_flashing(DFU_STATUS_OK);
}

// Invoked when received DFU_UPLOAD request
// Application fills buffer with up to length bytes and returns actual number of bytes
uint16_t tud_dfu_upload_cb(uint8_t alt, uint16_t block_num, uint8_t* data, uint16_t length)
{
    (void)alt;

    // Reset read pointer at the start of every new upload session
    if (block_num == 0) {
        dfu_state.read_address    = DFU_FLASH_START_ADDR;
        dfu_state.total_bytes_sent = 0;
    }

    return dfu_app_send(data, length);
}


// Invoked when the host has terminated a download or upload transfer
void tud_dfu_abort_cb(uint8_t alt)
{
    (void)alt;
    /* Reset our own counters */
    dfu_state.read_address      = DFU_FLASH_START_ADDR;
    dfu_state.total_bytes_sent     = 0;
    dfu_state.total_bytes_received = 0;
    // current address not resetting

}
