/*
 * etx_ota_update.c
 *
 *  Created on: Sep 11, 2024
 *      Author: abrahamayorinde
 */

#include <stdio.h>
#include "etx_ota_update.h"
#include "main.h"
#include <string.h>
#include <stdbool.h>

/* Firmware Size that we have received */
static uint32_t ota_fw_received_size;


extern UART_HandleTypeDef huart2;
extern uint8_t RxBuffer[1033];
//UART_HandleTypeDef huart3;

static uint32_t received_data = 0;
static uint32_t remaining_data = 0;
static uint16_t next_data_packet_size = 0;
static int upload_complete = 0;
HAL_StatusTypeDef ex = HAL_ERROR;

void etx_ota_send_resp( uint8_t type )
{
  ETX_OTA_RESP_ rsp =
  {
    .sof         = ETX_OTA_SOF,
    .packet_type = ETX_OTA_PACKET_TYPE_RESPONSE,
    .data_len    = 1u,
    .status      = type,
    .crc         = 0u,                //TODO: Add CRC
    .eof         = ETX_OTA_EOF
  };

  //DELAY_MS(5);
  //send response
  HAL_UART_Transmit(&huart2, (uint8_t *)&rsp, sizeof(ETX_OTA_RESP_), HAL_MAX_DELAY);
}

/**
  * @brief Write data to the Application's actual flash location.
  * @param data data to be written
  * @param data_len data length
  * @is_first_block true - if this is first block, false - not first block
  * @retval HAL_StatusTypeDef
  */
/*static*/ HAL_StatusTypeDef write_data_to_flash_app( uint8_t *data,
                                        uint16_t data_len, bool is_first_block )
{
  HAL_StatusTypeDef ret = HAL_ERROR;

  do
  {
	ret = HAL_FLASH_Unlock();

    if( ret != HAL_OK )
    {
      break;
    }

	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGSERR | FLASH_FLAG_WRPERR);
    //No need to erase every time. Erase only the first time.

    if( is_first_block )
    {
    	printf("Erasing the Flash memory...\r\n");

    	FLASH_Erase_Sector( FLASH_SECTOR_2, FLASH_BANK_1, FLASH_VOLTAGE_RANGE_3);

    	FLASH_WaitForLastOperation( HAL_MAX_DELAY, FLASH_BANK_1 );
    	//Erase the Flash
    }

    for(int i = 0; i < data_len;)
    {

    	ret = HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD,
                               (uint32_t)(ETX_APP_FLASH_ADDR + ota_fw_received_size),
                               (uint32_t)&data[i]);

    	FLASH_WaitForLastOperation( HAL_MAX_DELAY, FLASH_BANK_1 );

    	if( ret == HAL_OK )
    	{
    		//update the data count
    		ota_fw_received_size += 32;
    	}
    	else
    	{
    		printf("Flash Write Error\r\n");
    		break;
    	}
    	i+=32;
    }

    if( ret != HAL_OK )
    {
      break;
    }

    ret = HAL_FLASH_Lock();

  }while( false );

  return ret;
}

/*
 * Keeps track of whether the download has begun by returning the state of the local static variable v
 * this function
 */
bool has_download_begun()
{
	if(ota_fw_received_size == 0)
	{
		return true;
	}

	return false;
}

typedef struct
{
	ETX_OTA_PACKET_TYPE_ type;/* How often to call the task */
	void const (*proc)(void);	/* pointer to function returning void */
} DOWNLOAD_STATE;

static const DOWNLOAD_STATE timed_task[] =
{
    {ETX_OTA_PACKET_TYPE_CMD, command_message },
    {ETX_OTA_PACKET_TYPE_DATA, data_message },
    {ETX_OTA_PACKET_TYPE_HEADER, header_message },
    { 0, NULL },
};

void runtask(ETX_OTA_PACKET_TYPE_ parameter)
{
	if((parameter >= ETX_OTA_PACKET_TYPE_CMD) && (parameter < ETX_OTA_PACKET_TYPE_RESPONSES))
	{
		if(parameter == timed_task[parameter].type)
		{
			timed_task[parameter].proc();
			DELAY_MS(200);
		    etx_ota_send_resp( ETX_OTA_ACK );
		}
	}
	else
	{
	    etx_ota_send_resp( ETX_OTA_NACK );
	}
}

void const command_message (void)
{
	if(RxBuffer[PACKET_CMD_INDEX] == ETX_OTA_CMD_START)
	{
		printf("Start packet transmission received.\n");
		HAL_UART_Receive_IT(&huart2, RxBuffer, ETX_OTA_HEADER_PACKET_SIZE);
	}
	else if(RxBuffer[PACKET_CMD_INDEX] == ETX_OTA_CMD_END)
	{
		printf("End of transmission packet received.\n");
		upload_complete = 1;
		remaining_data = 0;
		received_data = 0;
		next_data_packet_size = 0;
	}
}

void const data_message (void)
{
	printf("Data packet received.\n");
    received_data = (RxBuffer[3]<<8) | (RxBuffer[2]);
    //function to save received buffer data to flash//
    ex = write_data_to_flash_app( &(RxBuffer[4]), received_data, has_download_begun() );

    if(received_data == next_data_packet_size)
    {
        printf("Received the expected packet size.\n");
        remaining_data -= next_data_packet_size;
    }
    else
    {
        printf("Did not receive the expected packet size.\n");
        remaining_data -= received_data;
    }

    if(remaining_data > ETX_OTA_DATA_MAX_SIZE)
    {
        next_data_packet_size = ETX_OTA_DATA_MAX_SIZE;
    }
    else
    {
        next_data_packet_size = remaining_data;
    }

    if(remaining_data == 0)
    {
        HAL_UART_Receive_IT(&huart2, RxBuffer, ETX_OTA_COMMAND_PACKET_SIZE);

    }
    else
    {
        HAL_UART_Receive_IT(&huart2, RxBuffer, next_data_packet_size + DATA_PACKET_OVERHEAD);
    }

}

void const header_message (void)
{
	printf("Header packet received.\n");

    uint32_t total_data_size = ( (RxBuffer[7]<<24) | (RxBuffer[6]<<16) | (RxBuffer[5]<<8) | (RxBuffer[4]) );
    remaining_data = total_data_size;

    if(remaining_data > ETX_OTA_DATA_MAX_SIZE)
    {
        next_data_packet_size = ETX_OTA_DATA_MAX_SIZE;
    }
    else
    {
        next_data_packet_size = remaining_data;
    }

    HAL_UART_Receive_IT(&huart2, RxBuffer, next_data_packet_size + DATA_PACKET_OVERHEAD);
}
