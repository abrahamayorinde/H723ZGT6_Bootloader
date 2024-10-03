# Bootloader
A UART bootloader.

Bootloader: A critical piece of software that provides an interface for the user to load an operating system and applications.

The bootloader therefore exists in one memory location while the other application exists in another.

The bootloader typically has the responsibility of verifying the hardware through a series of tests before it allows the other applications (operating system etc) to execute.
The bootloader's other responsibility is to update the application. 

The implications are that the bootloader has to have input/output (IO) capabililties, particularly the ability to communicate in order to receive the data that will be the updated application.  This also implies that the bootloader must be able to write to memory.  

In order to satisfy the communication requirement, this bootloader will use the USART2 communcation peripheral as the communication channel.  The STM32CubeMX configuration software is used to configure the peripheral, and it also provides some Hardware Abstraction Layer (HAL) functions to interact with it.  Additionally, the configuration software provides some HAL functions to be able to read from and write to the flash memory.

The UART2 channel is configured in interrupt mode.  This means the CPU will be interrupted once the UART has received a specific number of bytes.  It is up to the user to specify the number of bytes that should be received. The HAL function to enable the interrupt is; HAL_UART_Receive_IT(&uart_channel, receive_buffer, bytes_to_trigger_interrupt).  The function requires a pointer to the UART channel handler, the receive buffer as the second parameter, and the specific number of bytes as the last parameter.  

Once the buffer receives the specified number of bytes the interrupt handler callback is called.  This is where the received data will be examined to determine the CPU response.  

Communication begins with the Host tool sending a starting frame.  The target STM32 receives the frame and responds with an acknowledge frame.  The Host tool then sends the header frame.

When the target receives the header frame it extracts and saves the package size (data payload) from the meta data field. The package size is the complete data payload for all of the following data frames.  This information makes it straightforward to determine how many bytes the target should expect for the subsequent interrupts.  With the total data payload in memory, as well as the size of the buffer (1024 bytes), the target keep track of how many bytes remain to be downloaded and can call the HAL_UART_Receive_IT function with the appropriate amount of bytes to trigger the next interrupt.  Reception of a valid header frame by the target is followed the target sending an acknowledge frame.

Each subsequent data frame is followed by an acknolwedge frame.  Once the Host has sent all the package data via data frames and received the acknowledge frame, the Host will send a final end frame.  The target will also respond to the valid end frame with an acknolwedge frame.

Start/Initial Frame
| Start of Frame (SOF) | Packet Type | Data Length | Command | CRC | End of Frame (EOF)
| -------- | ------- |------- |------- |------- |------- |
| 1 byte | 1 byte | 2 bytes | 1 byte | 4 bytes | 1 byte |
| 0xAA | 0x00 | 0x01 | 0x00 | 0x00 | 0xBB |


Header Frame
| Start of Frame (SOF) | Packet Type | Data Length | Meta Data | CRC | End of Frame (EOF)
| -------- | ------- |------- |------- |------- |------- |
| 1 byte | 1 byte | 2 bytes | 16 bytes | 4 bytes | 1 byte |
| 0xAA | 0x02 | 0x0010 | 0x?? 0x?? 0x?? 0x??| 0x00 | 0xBB|

The 'Meta Data' field is made up of four parts: 
The Package size field (4 bytes, The full size of the application to be upload. A maximum of 4294967296 bytes),
Package CRC (4 bytes, currently always 0x00000000),
Reserved1 (4 bytes, unused),
Reserved2 (4 bytes, unused)

Data Frame
| Start of Frame (SOF) | Packet Type | Data Length | Data | CRC | End of Frame (EOF)
| -------- | ------- |------- |------- |------- |------- |
| 1 byte | 1 byte | 2 bytes | n bytes | 4 bytes | 1 byte |
| 0xAA | 0x01 | 0x???? | (Data bytes) | 0x00 | 0xBB |

The 'Data Length' field of the Data Frame is a maximum of 1024 bytes.

End Frame
| Start of Frame (SOF) | Packet Type | Data Length | Command | CRC | End of Frame (EOF)
| -------- | ------- |------- |------- |------- |------- |
| 1 byte | 1 byte | 2 bytes | 1 byte | 4 bytes | 1 byte |
| 0xAA | 0x00 | 0x01 | 0x01 | 0x00 | 0xBB |

Target Response/Acknowledge Frame
| Start of Frame (SOF) | Packet Type | Data Length | Status | CRC | End of Frame (EOF)
| -------- | ------- |------- |------- |------- |------- |
| 0xAA | 0x03 | 0x01 | (0x00 or 0x01) | 0x00 | 0xBB |
