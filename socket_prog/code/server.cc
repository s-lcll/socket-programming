#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <vector>
#include <string.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <unistd.h>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sstream>
#include <string>

#include "message.h"
#include "server.h"

using namespace std;

Server_State_T server_state;
string cmd_string[] = {" ", "CMD_LS", "CMD_SEND","CMD_GET","CMD_REMOVE","CMD_RENAME","CMD_SHUTDOWN"};

int main(int argc, char *argv[])
{	
	int msglen;
    unsigned short udp_port = 0;
	
	if ((argc != 1) && (argc != 3))
	{
		cout << "Usage: " << argv[0];
		cout << " [-p <udp_port>]" << endl;
		return 1;
	}
	else
	{
		//system("clear");
		//process input arguments
		for (int i = 1; i < argc; i++)
		{				
			if (strcmp(argv[i], "-p") == 0)
				udp_port = (unsigned short) atoi(argv[++i]);
		    else
		    {
		        cout << "Usage: " << argv[0];
		        cout << " [-p <udp_port>]" << endl;
		        return 1;
		    }
		}
	}
	
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	
	// create socket
	int server_sk = socket(AF_INET, SOCK_DGRAM, 0);
	if (server_sk < 0) {
		cout << "- failed to create UDP socket\n";
		exit(-1);
	}
	
	// setup socket
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(udp_port);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	// bind the socket
	if (bind(server_sk, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) { // testing purpose
		cout << "- failed to bind UDP socket\n";
		exit(-1);
	}
	
	socklen_t serverLen = sizeof(server_addr);
	socklen_t clientLen = sizeof(client_addr);
	// get the port name
	getsockname(server_sk, (struct sockaddr *)&server_addr, &serverLen);
	//cout << "socket has port " << server_addr.sin_port << "\n";
	
	int buf[1024];
	char sdbuf[1024];
	
	Cmd_T ls = CMD_LS;
	Cmd_T send = CMD_SEND;
	Cmd_T get = CMD_GET;
	Cmd_T rm = CMD_REMOVE;
	Cmd_T rn = CMD_RENAME;
	Cmd_T shutd = CMD_SHUTDOWN;
	Cmd_T quit = CMD_QUIT;
	Cmd_T ack = CMD_ACK;
	
	int cCMD;
	Cmd_Msg_T cMsg;

	// # CUSTOM: Server Class
	Server server(server_sk, server_addr, serverLen, client_addr, clientLen, buf, cCMD, cMsg);
	
	bool endloop = false;
    while (!endloop)
    {
		/*cMsg.cmd = cCMD;
		buf[0] = (int)cMsg.cmd;
		cMsg.error = 0;*/
		
        //sleep(1);
		usleep(100);
        
        switch(server_state)
        {
            case WAITING:
            {   
				/*cout << "Waiting UDP command @: " << server_addr.sin_port << "\n";
				if(recvfrom(server_sk, buf, 1024, 0, (struct sockaddr *)&client_addr, &clientLen) != -1) {
					read(server_sk, buf, 1024);
					if(buf[0])
						cout << "CMD_RECEIVED" << endl;
				}*/
				server_state = server.waitSignal();
                break;
            }
            case PROCESS_LS:
            {   
                server_state = WAITING;
				server.processLs();			// # CUSTOM: Server Class
                break;
            }
            case PROCESS_SEND:
            {
				server_state = WAITING;
				server.processSend();		// # CUSTOM: Server Class
                break;
            }
            case PROCESS_REMOVE:
            {               
		        server_state = WAITING;
				server.processRemove();		// # CUSTOM: Server Class
                break;
            }
            case PROCESS_RENAME:
            {                
                server_state = WAITING;
				server.processRename();		// # CUSTOM: Server Class
                break;
            }
            case SHUTDOWN:
            {
				endloop = true;
				server.processShutdown();
				break;
            }
            default:
            {
           		server_state = WAITING;
                break;
            }
        }
    }
	close(server_sk);
    return 0;
}

//this function check if the backup folder exist
int checkDirectory (string dir)
{
	DIR *dp;
	if((dp  = opendir(dir.c_str())) == NULL) {
        //cout << " - error(" << errno << ") opening " << dir << endl;
        if(mkdir(dir.c_str(), S_IRWXU) == 0)
            cout<< " - Note: Folder "<<dir<<" does not exist. Created."<<endl;
        else
            cout<< " - Note: Folder "<<dir<<" does not exist. Cannot created."<<endl;
        return errno;
    }
    closedir(dp);

	return 0;
}


//this function is used to get all the filenames from the
//backup directory
int getDirectory (string dir, vector<string> &files)
{
    DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(dir.c_str())) == NULL) {
        //cout << " - error(" << errno << ") opening " << dir << endl;
        if(mkdir(dir.c_str(), S_IRWXU) == 0)
            cout<< " - Note: Folder "<<dir<<" does not exist. Created."<<endl;
        else
            cout<< " - Note: Folder "<<dir<<" does not exist. Cannot created."<<endl;
        return errno;
    }

    int j=0;
    while ((dirp = readdir(dp)) != NULL) {
    	//do not list the file "." and ".."
        if((string(dirp->d_name)!=".") && (string(dirp->d_name)!=".."))
        	files.push_back(string(dirp->d_name));
    }
    closedir(dp);
    return 0;
}
//this function check if the file exists
bool checkFile(const char *fileName)
{
    ifstream infile(fileName);
    return infile.good();
}

