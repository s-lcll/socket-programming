#ifndef _SERVER_H
#define _SERVER_H

#include <vector>
#include <string>

using namespace std;

typedef enum SERVER_STATE_tag
{
    WAITING = 0,
    PROCESS_LS = 1,
    PROCESS_SEND = 2,
    PROCESS_GET = 3,
    PROCESS_REMOVE = 4,
    PROCESS_RENAME = 5,
    SHUTDOWN = 6
}Server_State_T;

bool checkFile(const char *fileName);
int checkDirectory (string dir);
int getDirectory (string dir, vector<string> &files);


// # CUSTOM: Server Class
class Server
{
public:
    Server(Server &&) = delete;
    Server(const Server &) = delete;

    Server(int &server_sk, sockaddr_in &server_addr, socklen_t &serverLen, sockaddr_in &client_addr, socklen_t &clientLen, int (&buf)[1024], int &cCMD, Cmd_Msg_T &cMsg)
    : server_sk(server_sk), server_addr(server_addr), serverLen(serverLen), client_addr(client_addr), clientLen(clientLen), buf(buf), cCMD(cCMD), cMsg(cMsg) {}


    Server_State_T waitSignal()
    {
        cout << "Waiting UDP command @: " << ntohs(server_addr.sin_port) << "\n"; // < can see this
        clearBufAndMsg();
        if (recvMsgViaUDP(cMsg)) {
            //read(server_sk, buf, 1024);
            //cCMD = buf[0];

            //cCMD = msg.cmd;

            //std::cout << "msg.cmd = " << (int)msg.cmd << std::endl;

            switch (cMsg.cmd)
            {
            case CMD_LS:
                std::cout << "[CMD_RECEIVED]: CMD_LS" << std::endl;
                return PROCESS_LS;
                break;
            case CMD_SEND:
                std::cout << "[CMD_RECEIVED]: CMD_SEND" << std::endl;
                return PROCESS_SEND;
                break;
            case CMD_GET:
                std::cout << "[CMD_RECEIVED]: CMD_GET" << std::endl;
                return PROCESS_GET;
                break;
            case CMD_REMOVE:
                std::cout << "[CMD_RECEIVED]: CMD_REMOVE" << std::endl;
                return PROCESS_REMOVE;
                break;
            case CMD_RENAME:
                std::cout << "[CMD_RECEIVED]: CMD_RENAME" << std::endl;
                return PROCESS_RENAME;
                break;
            case CMD_SHUTDOWN:
                std::cout << "[CMD_RECEIVED]: CMD_SHUTDOWN" << std::endl;
                return SHUTDOWN;
                break;
            //case CMD_QUIT:
            //    break;
            //case CMD_ACK:
            //    break;
            default:
                break;
            }
        }

        return WAITING;
    }

    void processLs()
    {
        //std::cout << "(testing) - processLs()" << std::endl;

        Cmd_Msg_T msg;
        msg.cmd = CMD_LS;
        
        if (checkDirectory(ROOT_DIR) != 0)
        {
            std::cout << " - server backup folder is empty" << std::endl;

            msg.size = 0;   // If there is no files in the folder or the folder does not exist, the server will send this message with “size = 0”.
            sendMsgViaUDP(msg);
            return;
        }

        std::vector<std::string> files;
        if (getDirectory(ROOT_DIR, files) != 0 || files.size() == 0)
        {
            std::cout << " - server backup folder is empty." << std::endl;

            msg.size = 0;   // If there is no files in the folder or the folder does not exist, the server will send this message with “size = 0”.
            sendMsgViaUDP(msg);
            return;
        }

        msg.size = files.size();
        sendMsgViaUDP(msg);

        Data_Msg_T dmsg;
        for (auto &filename : files)
        {
            std::cout << " - " << filename << std::endl;
            strncpy(dmsg.data, filename.c_str(), sizeof(dmsg.data));
            sendMsgViaUDP(dmsg);
        }
    }

    void processSend()
    {
        Cmd_Msg_T &fileinfo_msg = cMsg;

        std::string filename;
        fileinfo_msg.filename[sizeof(fileinfo_msg.filename) - 1] = '\0';
        filename = ROOT_DIR + fileinfo_msg.filename;

        Cmd_Msg_T response_msg;
        response_msg.cmd = CMD_SEND;

        if (checkFile(filename.c_str()))
        {
            std::cout << " - file " << fileinfo_msg.filename << " exists; overwrite?" << std::endl;

            response_msg.error = 2;
            sendMsgViaUDP(response_msg);

            recvMsgViaUDP(response_msg);
            if (response_msg.error == 2)
                return;

            // 'y' or 'Y'
            // if (response_msg.error == 0)
        }

        class FileAutoClose
        {
        public:
            FileAutoClose(FILE *_file) { m_file = _file; }
            ~FileAutoClose()    { if (m_file != nullptr) fclose(m_file); }
        private:
            FILE *m_file;
        };
        
        FILE *file = fopen(filename.c_str(), "wb");
        if (file == nullptr)
        {
            Cmd_Msg_T fopen_error_msg;
            fopen_error_msg.cmd = CMD_SEND;
            fopen_error_msg.port = 0;
            fopen_error_msg.error = 1;
            sendMsgViaUDP(fopen_error_msg);
            std::cout << " - open file " << cMsg.filename << " error." << std::endl;
            return;
        }
        FileAutoClose _autoClose(file);

        std::cout << " - filename: " << cMsg.filename << std::endl;
        std::cout << " - filesize: " << cMsg.size << std::endl;

        
        int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (tcp_socket < 0)
        {
            std::cout << " - failed to create TCP socket." << std::endl;
            return;
        }

        sockaddr_in tcp_server_addr;
        tcp_server_addr.sin_family = AF_INET;
        tcp_server_addr.sin_port = htons(0);
        tcp_server_addr.sin_addr.s_addr = server_addr.sin_addr.s_addr;
        
        if (bind(tcp_socket, reinterpret_cast<sockaddr *>(&tcp_server_addr), sizeof(tcp_server_addr)) < 0)
        {
            std::cout << " - failed to bind TCP socket." << std::endl;
            close(tcp_socket);
            return;
        }
        
        listen(tcp_socket, 1);

        socklen_t tcp_addrlen = sizeof(tcp_server_addr);
        getsockname(tcp_socket, reinterpret_cast<sockaddr *>(&tcp_server_addr), &tcp_addrlen);

        uint16_t tcp_port = ntohs(tcp_server_addr.sin_port);
        std::cout << " - listen @: " << tcp_port << std::endl;

        Cmd_Msg_T tcp_hao123;
        tcp_hao123.cmd = CMD_SEND;
        tcp_hao123.port = tcp_port;
        tcp_hao123.error = 0;
        sendMsgViaUDP(tcp_hao123);
        
        int tcp_client_socket;
        if ((tcp_client_socket = accept(tcp_socket, reinterpret_cast<sockaddr *>(&tcp_server_addr), &tcp_addrlen)) < 0)
        {
            std::cout << " - failed to accept TCP connection." << std::endl;
            close(tcp_socket);
            return;
        }

        std::cout << " - connected with client." << std::endl;

        Data_Msg_T dmsg;
        {
            ssize_t nBytesHandled = 0;
            ssize_t result;

            fseek(file, 0, SEEK_SET);
            while (nBytesHandled < cMsg.size)
            {
                constexpr int dmsgDataSize = sizeof(dmsg.data);
                result = read(tcp_client_socket, dmsg.data, (dmsgDataSize < fileinfo_msg.size) ? dmsgDataSize : fileinfo_msg.size);
                if (result < 0 || result == 0)
                {
                    std::cout << " - message reception error. " << std::endl;
                    Cmd_Msg_T ack_msg;
                    ack_msg.cmd = CMD_ACK;
                    ack_msg.error = 1;
                    sendMsgViaUDP(ack_msg);
                    std::cout << " - sent acknowledgement." << std::endl;
                    close(tcp_socket);
                    return;
                }
                fwrite(dmsg.data, result, 1, file);
                nBytesHandled += result;

                std::cout << result << std::endl;
                std::cout << " - total bytes received: " << nBytesHandled << std::endl;
            }
        }

        close(tcp_socket);

        Cmd_Msg_T ack_msg;
        ack_msg.cmd = CMD_ACK;
        ack_msg.error = 0;
        sendMsgViaUDP(ack_msg);
        std::cout << " - sent acknowledgement." << std::endl;
    }

    void processRemove()
    {
        std::string filename;
        cMsg.filename[sizeof(cMsg.filename) - 1] = '\0';
        filename = cMsg.filename;

        if (!checkFile((ROOT_DIR + filename).c_str()))
        {
            std::cout << " - file doesn't exist." << std::endl;
            Cmd_Msg_T ack_msg;
            ack_msg.cmd = CMD_ACK;
            ack_msg.error = 1;
            sendMsgViaUDP(ack_msg);
            std::cout << " - sent acknowledgement." << std::endl;
            return;
        }

        system((std::string("rm ") + ROOT_DIR + filename).c_str());
        std::cout << " - " << (ROOT_DIR + filename) << " is removed." << std::endl;

        Cmd_Msg_T ack_msg;
        ack_msg.cmd = CMD_ACK;
        ack_msg.error = 0;
        sendMsgViaUDP(ack_msg);
        std::cout << " - sent acknowledgement." << std::endl;
    }

    void processRename()
    {
        cMsg.original_filename[sizeof(cMsg.original_filename) - 1] = '\0';
        cMsg.expected_filename[sizeof(cMsg.expected_filename) - 1] = '\0';

        std::string original_filename, expected_filename;
        original_filename = cMsg.original_filename;
        expected_filename = cMsg.expected_filename;

        if (!checkFile((ROOT_DIR + original_filename).c_str())/* || checkFile((ROOT_DIR + expected_filename).c_str())*/)
        {
            std::cout << " - file doesn't exist." << std::endl;
            Cmd_Msg_T ack_msg;
            ack_msg.cmd = CMD_ACK;
            ack_msg.error = 1;
            sendMsgViaUDP(ack_msg);
            std::cout << " - sent acknowledgement." << std::endl;
            return;
        }

        system((std::string("mv ") + ROOT_DIR + original_filename + " " + ROOT_DIR + expected_filename).c_str());
        std::cout << " - " << "the file has been renamed to " << expected_filename << "." << std::endl;

        Cmd_Msg_T ack_msg;
        ack_msg.cmd = CMD_ACK;
        ack_msg.error = 0;
        sendMsgViaUDP(ack_msg);
        std::cout << " - sent acknowledgement." << std::endl;
    }

    void processShutdown()
    {
        Cmd_Msg_T ack_msg;
        ack_msg.cmd = CMD_ACK;
        ack_msg.error = 0;
        sendMsgViaUDP(ack_msg);
        std::cout << " - sent acknowledgement." << std::endl;
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
        if (recvBufViaUDP(buf) < 0)
        {
            memset(&buf, 0, sizeof(buf));
            memset(&msg, 0, sizeof(msg));
            return false;
        }
        
        constexpr int bufSize = sizeof(buf);
        constexpr int msgSize = sizeof(msg);
        memcpy(&msg, buf, (bufSize < msgSize) ? bufSize : msgSize);
        //std::cout << "buf = " << (uint32_t)(((uint8_t *)buf)[0]) << std::endl;
        Cmd_Msg_T hostMsg = msg.toHostByteOrder();   // !!! Converts to Network Byte Order before sending
        memcpy(&msg, &hostMsg, sizeof(Cmd_Msg_T));
        return true;
    }

    bool recvMsgViaUDP(Data_Msg_T &msg)
    {
        if (recvBufViaUDP(buf) < 0)
        {
            memset(&buf, 0, sizeof(buf));
            memset(&msg, 0, sizeof(msg));
            return false;
        }
        
        constexpr int bufSize = sizeof(buf);
        constexpr int msgSize = sizeof(msg);
        memcpy(&msg, buf, (bufSize < msgSize) ? bufSize : msgSize);
        //std::cout << "buf = " << (uint32_t)(((uint8_t *)buf)[0]) << std::endl;
        return true;
    }

    template<int N>
    int recvBufViaUDP(int (&buf2)[N])
    {
        memset(&client_addr, 0, sizeof(client_addr));
        //memcpy(&client_addr, &server_addr, sizeof(client_addr));
        //clientLen = serverLen;
        return recvfrom(server_sk, buf, sizeof(buf), MSG_WAITALL, (struct sockaddr *)&client_addr, &clientLen);
    }

    bool sendMsgViaUDP(const Cmd_Msg_T &msg)
    {
        constexpr int bufSize = sizeof(buf);
        constexpr int msgSize = sizeof(msg);

        Cmd_Msg_T networkMsg = msg.toNetworkByteOrder();   // !!! Converts to Network Byte Order before sending

        memset(&buf, 0, bufSize);
        memcpy(&buf, &networkMsg, (bufSize < msgSize) ? bufSize : msgSize);

        return sendBufViaUDP(buf) > 0;
    }

    bool sendMsgViaUDP(const Data_Msg_T &msg)
    {
        constexpr int bufSize = sizeof(buf);
        constexpr int msgSize = sizeof(msg);

        memset(&buf, 0, bufSize);
        memcpy(&buf, &msg, (bufSize < msgSize) ? bufSize : msgSize);

        return sendBufViaUDP(buf) > 0;
    }

    template<int N>
    int sendBufViaUDP(int (&buf2)[N])
    {
        //memcpy(&client_addr, &server_addr, sizeof(client_addr));
        //clientLen = serverLen;
        return sendto(server_sk, buf, sizeof(buf), MSG_CONFIRM, (struct sockaddr *)&client_addr, clientLen);
    }


private:
    int &server_sk;
    sockaddr_in &server_addr;
    socklen_t &serverLen;
    sockaddr_in &client_addr;
    socklen_t &clientLen;
    int (&buf)[1024];
    int &cCMD;
    Cmd_Msg_T &cMsg;
    
    const std::string ROOT_DIR = "./backup/";
};


#endif