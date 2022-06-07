/*
 *  message.h
 */

#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#define DATA_BUF_LEN 3000
#define FILE_NAME_LEN 128

typedef struct DATA_MSG_tag
{
	char data[DATA_BUF_LEN]; // filename
}Data_Msg_T;

typedef enum CMD_tag
{
    CMD_LS = 1,
    CMD_SEND = 2,
    CMD_GET = 3,
    CMD_REMOVE = 4,
    CMD_RENAME = 5,
    CMD_SHUTDOWN = 6,
    CMD_QUIT = 7,
    CMD_ACK = 8,
}Cmd_T;

typedef struct CMD_MSG_tag
{
    uint8_t cmd;
    char filename[FILE_NAME_LEN];
    uint32_t size;
    uint16_t port;
	uint16_t error;
    char original_filename[FILE_NAME_LEN];  // # CUSTOM: Added for handling Rename requests
    char expected_filename[FILE_NAME_LEN];  // #
    
    inline CMD_MSG_tag()
    {
        memset(this, 0, sizeof(CMD_MSG_tag));
    }

    inline CMD_MSG_tag toHostByteOrder() const
    {
        CMD_MSG_tag convertedMsg;
        memcpy(&convertedMsg, this, sizeof(CMD_MSG_tag));

        //convertedMsg.cmd     = ntohs(cmd);
        convertedMsg.size    = ntohl(size);
        convertedMsg.port    = ntohs(port);
        convertedMsg.error   = ntohs(error);

        return convertedMsg;
    }

    inline CMD_MSG_tag toNetworkByteOrder() const
    {
        CMD_MSG_tag convertedMsg;
        memcpy(&convertedMsg, this, sizeof(CMD_MSG_tag));

        //convertedMsg.cmd     = htons(cmd);
        convertedMsg.size    = htonl(size);
        convertedMsg.port    = htons(port);
        convertedMsg.error   = htons(error);

        return convertedMsg;
    }
}Cmd_Msg_T;
#endif
