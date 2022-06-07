#ifndef _CLIENT_H
#define _CLIENT_H

#include <sys/types.h>

using namespace std;

typedef enum CLIENT_STATE_tag
{
    WAITING = 0,
    PROCESS_LS = 1,
    PROCESS_SEND = 2,
    PROCESS_GET = 3,
    PROCESS_REMOVE = 4,
    PROCESS_RENAME = 5,
    SHUTDOWN = 6,
    QUIT = 7
}Client_State_T;


// # CUSTOM: Client Class
class Client
{
public:
    Client(Client &&) = delete;
    Client(const Client &) = delete;

    Client(int &client_sk, sockaddr_in &server_addr, socklen_t &serverLen, int (&buf)[1024], int &cCMD, Cmd_Msg_T &cMsg)
    : client_sk(client_sk), server_addr(server_addr), serverLen(serverLen), buf(buf), cCMD(cCMD), cMsg(cMsg) {}

    void processLs()
    {
        //std::cout << "(testing) - processLs()" << std::endl;
        
        Cmd_Msg_T msg;
        msg.cmd = CMD_LS;

        if (!sendMsgViaUDP(msg))
        {
            //std::cout << "(testing) - !sendMsgViaUDP()" << std::endl;
            return;
        }

        if (!recvMsgViaUDP(msg))
        {
            //std::cout << "(testing) - !recvMsgViaUDP()" << std::endl;
            return;
        }

        if (msg.cmd != CMD_LS)
        {
            std::cout << " - command response error." << std::endl;
            return;
        }

        if (msg.size == 0)
            std::cout << " - server backup folder is empty." << std::endl;
        else
        {
            //std::cout << "(testing) - msg.size = " << msg.size << std::endl;
            Data_Msg_T dmsg;
            for (int i = 0; i < msg.size; i++)
            {
                recvMsgViaUDP(dmsg);
                std::cout << " - " << dmsg.data << std::endl;
            }
        }
    }

    void processSend()
    {
        Cmd_Msg_T fileinfo_msg;
        fileinfo_msg.cmd = CMD_SEND;
        fileinfo_msg.error = 0;
        std::cin >> fileinfo_msg.filename;

        class FileAutoClose
        {
        public:
            FileAutoClose(FILE *_file) { m_file = _file; }
            ~FileAutoClose()    { if (m_file != nullptr) fclose(m_file); }
        private:
            FILE *m_file;
        };
        
        FILE *file = fopen(fileinfo_msg.filename, "rb");
        if (file == nullptr)
        {
            std::cout << " - cannot open file: " << fileinfo_msg.filename << std::endl;
            return;
        }
        FileAutoClose _autoClose(file);

        fseek(file, 0, SEEK_END);
        fileinfo_msg.size = ftell(file);

        std::cout << " - filesize: " << fileinfo_msg.size << std::endl;

        sendMsgViaUDP(fileinfo_msg);
        
        Cmd_Msg_T response_msg;
        recvMsgViaUDP(response_msg);

        Cmd_Msg_T tcp_hao123;

        if (response_msg.error == 2)
        {
            char yn;
            std::cout << " - file exists. overwrite? (y/N): ";
            std::cin >> yn;

            if (yn != 'y' && yn != 'Y')
            {
                response_msg.cmd = CMD_SEND;
                response_msg.error = 2;
                sendMsgViaUDP(response_msg);
                return;
            }

            response_msg.cmd = CMD_SEND;
            response_msg.error = 0;
            sendMsgViaUDP(response_msg);

            recvMsgViaUDP(tcp_hao123);
        }
        else
            memcpy(&tcp_hao123, &response_msg, sizeof(tcp_hao123));

        // error
        if (tcp_hao123.port == 0 || tcp_hao123.cmd != CMD_SEND || tcp_hao123.error == 1)
        {
            std::cout << " - error or incorrect response from server." << std::endl;
            return;
        }

        std::cout << " - TCP port: " << tcp_hao123.port << std::endl;

        int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (tcp_socket < 0)
        {
            std::cout << " - failed to create TCP socket." << std::endl;
            return;
        }

        sockaddr_in tcp_server_addr;
        tcp_server_addr.sin_family = AF_INET;
        tcp_server_addr.sin_port = htons(tcp_hao123.port);
        tcp_server_addr.sin_addr.s_addr = server_addr.sin_addr.s_addr;
        if (connect(tcp_socket, reinterpret_cast<sockaddr *>(&tcp_server_addr), sizeof(tcp_server_addr)) < 0)
        {
            std::cout << " - failed to connect server with TCP." << std::endl;
            close(tcp_socket);
            return;
        }

        Data_Msg_T dmsg;
        {
            ssize_t nBytesHandled = 0;
            size_t readResult;
            ssize_t sendResult;

            fseek(file, 0, SEEK_SET);
            int remaining_size;
            while ((remaining_size = fileinfo_msg.size - nBytesHandled) > 0)
            {
                constexpr int dmsgDataSize = sizeof(dmsg.data);
                int read_size = (dmsgDataSize < remaining_size) ? dmsgDataSize : remaining_size;
                readResult = fread(dmsg.data, read_size, 1, file);
                sendResult = send(tcp_socket, dmsg.data, readResult * read_size, 0);
                if (sendResult < 0 || sendResult == 0)
                    break;
                
                nBytesHandled += sendResult;
            }
        }

        close(tcp_socket);

        Cmd_Msg_T ack_msg;
        recvMsgViaUDP(ack_msg);
        if (ack_msg.error == 0)
        {
            std::cout << " - file transmission is completed." << std::endl;
        }
        else
        {
            std::cout << " - file transmission is failed." << std::endl;
        }
    }

    void processRemove()
    {
        Cmd_Msg_T msg;
        msg.cmd = CMD_REMOVE;
        std::cin >> msg.filename;
        sendMsgViaUDP(msg);

        recvMsgViaUDP(msg);
        if (msg.error == 0)
        {
            std::cout << " - file is removed." << std::endl;
        }
        else
        {
            std::cout << " - file doesn't exist." << std::endl;
        }
    }

    void processRename()
    {
        Cmd_Msg_T msg;
        msg.cmd = CMD_RENAME;
        std::cin >> msg.original_filename >> msg.expected_filename;
        sendMsgViaUDP(msg);

        recvMsgViaUDP(msg);
        if (msg.error == 0)
        {
            std::cout << " - file has been renamed." << std::endl;
        }
        else
        {
            std::cout << " - file doesn't exist." << std::endl;
        }
    }

    void shutdownServer()
    {
        Cmd_Msg_T msg;
        msg.cmd = CMD_SHUTDOWN;
        sendMsgViaUDP(msg);
        
        recvMsgViaUDP(msg);
        if (msg.error == 0)
        {
            std::cout << " - server is shutdown." << std::endl;
        }
    }

private:

    void clearBufAndMsg()
    {
        memset(buf,     0, sizeof(buf));
        memset(&cMsg,   0, sizeof(cMsg));
    }

    void copyMsgToBuf()
    {
        constexpr int bufSize = sizeof(buf);
        constexpr int msgSize = sizeof(cMsg);

        memcpy(buf, &cMsg, (bufSize < msgSize) ? bufSize : msgSize);
    }

    bool recvMsgViaUDP(Cmd_Msg_T &msg)
    {
        if (recvBufViaUDP(buf) == -1)
            return false;
        
        constexpr int bufSize = sizeof(buf);
        constexpr int msgSize = sizeof(msg);
        memcpy(&msg, buf, (bufSize < msgSize) ? bufSize : msgSize);
        Cmd_Msg_T hostMsg = msg.toHostByteOrder();   // !!! Converts to Network Byte Order before sending
        memcpy(&msg, &hostMsg, sizeof(Cmd_Msg_T));
        return true;
    }

    bool recvMsgViaUDP(Data_Msg_T &msg)
    {
        if (recvBufViaUDP(buf) == -1)
            return false;
        
        constexpr int bufSize = sizeof(buf);
        constexpr int msgSize = sizeof(msg);
        memcpy(&msg, buf, (bufSize < msgSize) ? bufSize : msgSize);
        return true;
    }

    template<int N>
    int recvBufViaUDP(int (&buf)[N])
    {
        memset(&buf, 0, sizeof(buf));
        return recvfrom(client_sk, buf, sizeof(buf), MSG_WAITALL, (struct sockaddr *)&server_addr, &serverLen);
    }

    bool sendMsgViaUDP(const Cmd_Msg_T &msg)
    {
        constexpr int bufSize = sizeof(buf);
        constexpr int msgSize = sizeof(msg);

        Cmd_Msg_T networkMsg = msg.toNetworkByteOrder();   // !!! Converts to Network Byte Order before sending

        memset(&buf, 0, bufSize);
        memcpy(&buf, &networkMsg, (bufSize < msgSize) ? bufSize : msgSize);

        return sendBufViaUDP(buf) != -1;
    }

    bool sendMsgViaUDP(const Data_Msg_T &msg)
    {
        constexpr int bufSize = sizeof(buf);
        constexpr int msgSize = sizeof(msg);

        memset(&buf, 0, bufSize);
        memcpy(&buf, &msg, (bufSize < msgSize) ? bufSize : msgSize);

        return sendBufViaUDP(buf) != -1;
    }

    template<int N>
    int sendBufViaUDP(int (&buf)[N])
    {
        return sendto(client_sk, buf, sizeof(buf), MSG_CONFIRM, (struct sockaddr *)&server_addr, serverLen);
    }


private:
    int &client_sk;
    sockaddr_in &server_addr;
    socklen_t &serverLen;
    int (&buf)[1024];
    int &cCMD;
    Cmd_Msg_T &cMsg;
};


#endif