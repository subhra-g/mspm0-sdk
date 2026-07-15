/*
 * DFU Application Interface
 *
 * This header provides the interface for DFU operations including
 * initialization, deinitialization, and data transfer functions.
 */

#ifndef DFU_APP_H_
#define DFU_APP_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  bool initialized;
  uint32_t current_address;
  uint32_t read_address;
  uint32_t total_bytes_received;
  uint32_t total_bytes_sent;
  uint32_t last_erased_sector;
} dfu_app_state_t;

dfu_app_state_t dfu_state;

//--------------------------------------------------------------------+
// Application Configuration
//--------------------------------------------------------------------+

// Flash memory configuration for firmware update
#define DFU_FLASH_START_ADDR    0x00010000  // Start address for firmware storage
#define DFU_FLASH_END_ADDR      0x00014000  // 192KB available for firmware
#define DFU_FLASH_SECTOR_SIZE   1024        // Flash sector size in bytes
#define DFU_FLASH_READ_END_ADDR 0x000130FC
//--------------------------------------------------------------------+
// Application API
//--------------------------------------------------------------------+

/**
 * @brief Initialize DFU application
 *
 * Initializes flash driver, buffers, and prepares system for DFU operations.
 * Must be called before any DFU operations.
 *
 * @return true if initialization successful, false otherwise
 */
bool dfu_app_init(void);

/**
 * @brief Deinitialize DFU application
 *
 * Cleans up resources, closes flash driver, and resets DFU state.
 * Should be called when DFU operations are complete.
 *
 * @return true if deinitialization successful, false otherwise
 */
bool dfu_app_deinit(void);

/**
 * @brief Receive firmware data from host
 *
 * Receives firmware data block from USB host and writes to flash memory.
 * Performs address validation and flash write operations.
 *
 * @param block_num Block number (sequential counter)
 * @param data Pointer to received data buffer
 * @param length Length of data in bytes
 * @return true if data received and written successfully, false on error
 */
bool dfu_app_receive(uint16_t block_num, uint8_t const *data, uint16_t length);

/**
 * @brief Send firmware data to host
 *
 * Reads firmware data from flash memory and sends to USB host.
 * Used for upload/readback operations.
 *
 * @param block_num Block number (sequential counter)
 * @param data Pointer to buffer for data to send
 * @param length Maximum length to read
 * @return Number of bytes actually read and ready to send, 0 on error
 */
uint16_t dfu_app_send(uint8_t *data, uint16_t length);

/**
 * @brief Erase flash memory for firmware update
 *
 * Erases the designated flash region to prepare for new firmware.
 *
 * @return true if erase successful, false otherwise
 */
bool dfu_app_erase_flash(void);


#ifdef __cplusplus
}
#endif

#endif /* DFU_APP_H_ */