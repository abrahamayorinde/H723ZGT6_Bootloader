/*
 * etx_ota_update.h
 *
 *  Created on: Sep 11, 2024
 *      Author: abrahamayorinde
 */
#include <stdint.h>
#include <stdbool.h>
#ifndef INC_ETX_OTA_UPDATE_H_
#define INC_ETX_OTA_UPDATE_H_

#define ETX_OTA_SOF		0xAA	// Start of frame
#define ETX_OTA_EOF		0xBB	// End of frame
#define ETX_OTA_ACK		0x00	// Acknowledge
#define ETX_OTA_NACK	0x01	// Negative Acknowledge

#define ETX_APP_FLASH_ADDR	0x8040000	//Application Flash address 0x08040000

#define ETX_OTA_DATA_MAX_SIZE	(1024)	//Maximum size
#define ETX_OTA_DATA_OVERHEAD 	(9)		//data overhead
#define ETX_OTA_PACKET_MAX_SIZE	(ETX_OTA_DATA_MAX_SIZE + ETX_OTA_DATA_OVERHEAD)	// Max data transmission
#define ETX_OTA_COMMAND_PACKET_SIZE (10)
#define ETX_OTA_HEADER_PACKET_SIZE (25)
#define PACKET_TYPE_INDEX 	(1)
#define PACKET_CMD_INDEX 	(4)
#define DATA_PACKET_OVERHEAD	(9)
#define ETX_OTA_END_MESSAGE_SIZE	(9)

#define SYSTICK_LOAD (SystemCoreClock/1000000U)
#define SYSTICK_DELAY_CALIB (SYSTICK_LOAD >> 1)

#define DELAY_US(us) \
    do { \
         uint32_t start = SysTick->VAL; \
         uint32_t ticks = (us * SYSTICK_LOAD)-SYSTICK_DELAY_CALIB;  \
         while((start - SysTick->VAL) < ticks); \
    } while (0)

#define DELAY_MS(ms) \
    do { \
        for (uint32_t i = 0; i < ms; ++i) { \
            DELAY_US(1000); \
        } \
    } while (0)
/*
 * Exception codes
 */
typedef enum
{
	ETX_OTA_EX_OK 	= 0,
	ETX_OTA_EX_ERR	= 1,
}ETX_OTA_EX_;

/*
 * OTA process state
 */
typedef enum
{
  ETX_OTA_STATE_IDLE    = 0,
  ETX_OTA_STATE_START   = 1,
  ETX_OTA_STATE_HEADER  = 2,
  ETX_OTA_STATE_DATA    = 3,
  ETX_OTA_STATE_END     = 4,
}ETX_OTA_STATE_;

/*
 * Packet type
 */
typedef enum
{
  ETX_OTA_PACKET_TYPE_CMD       = 0,    // Command
  ETX_OTA_PACKET_TYPE_DATA      = 1,    // Data
  ETX_OTA_PACKET_TYPE_HEADER    = 2,    // Header
  ETX_OTA_PACKET_TYPE_RESPONSE  = 3,    // Response
  ETX_OTA_PACKET_TYPE_RESPONSES = 4,
}ETX_OTA_PACKET_TYPE_;

/*
 * OTA Commands
 */
typedef enum
{
  ETX_OTA_CMD_START = 0,    // OTA Start command
  ETX_OTA_CMD_END   = 1,    // OTA End command
  ETX_OTA_CMD_ABORT = 2,    // OTA Abort command
}ETX_OTA_CMD_;

/*
 * OTA meta info
 */
typedef struct
{
  uint32_t package_size;
  uint32_t package_crc;
  uint32_t reserved1;
  uint32_t reserved2;
}__attribute__((packed)) meta_info;

/*
 * OTA Command format
 *
 * ________________________________________
 * |     | Packet |     |     |     |     |
 * | SOF | Type   | Len | CMD | CRC | EOF |
 * |_____|________|_____|_____|_____|_____|
 *   1B      1B     2B    1B     4B    1B
 */
typedef struct
{
  uint8_t   sof;
  uint8_t   packet_type;
  uint16_t  data_len;
  uint8_t   cmd;
  uint32_t  crc;
  uint8_t   eof;
}__attribute__((packed)) ETX_OTA_COMMAND_;

/*
 * OTA Header format
 *
 * __________________________________________
 * |     | Packet |     | Header |     |     |
 * | SOF | Type   | Len |  Data  | CRC | EOF |
 * |_____|________|_____|________|_____|_____|
 *   1B      1B     2B     16B     4B    1B
 */
typedef struct
{
  uint8_t     sof;
  uint8_t     packet_type;
  uint16_t    data_len;
  meta_info   meta_data;
  uint32_t    crc;
  uint8_t     eof;
}__attribute__((packed)) ETX_OTA_HEADER_;

/*
 * OTA Data format
 *
 * __________________________________________
 * |     | Packet |     |        |     |     |
 * | SOF | Type   | Len |  Data  | CRC | EOF |
 * |_____|________|_____|________|_____|_____|
 *   1B      1B     2B    nBytes   4B    1B
 */
typedef struct
{
  uint8_t sof;
  uint8_t packet_type;
  uint16_t data_len;
  uint8_t *data;
}__attribute__((packed)) ETX_OTA_DATA_;

/*
 * OTA Response format
 *
 * __________________________________________
 * |     | Packet |     |        |     |     |
 * | SOF | Type   | Len | Status | CRC | EOF |
 * |_____|________|_____|________|_____|_____|
 *   1B      1B     2B      1B     4B    1B
 */
typedef struct
{
  uint8_t sof;
  uint8_t packet_type;
  uint16_t data_len;
  uint8_t status;
  uint32_t crc;
  uint8_t eof;
}__attribute__((packed)) ETX_OTA_RESP_;

ETX_OTA_EX_ etx_ota_download_and_flash( void );
/*HAL_StatusTypeDef write_data_to_flash_app( uint8_t *data,
                                        uint16_t data_len, bool is_first_block );*/
void etx_ota_send_resp( uint8_t type );
bool has_download_begun();
void const command_message (void);
void const data_message (void);
void const header_message (void);
void runtask(ETX_OTA_PACKET_TYPE_ parameter);
#endif /* INC_ETX_OTA_UPDATE_H_ */
