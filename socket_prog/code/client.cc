#include <sys/types.h>
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
#include <netdb.h>
#include <algorithm>

#include "message.h"
#include "client.h"

using namespace std;

int main(int argc, char *argv[])
{
    unsigned short udp_port = 0;
    const char* server_host = "127.0.0.1";
	//const char* server_host = "172.16.5.11";
    //process input arguments
	if ((argc != 3) && (argc != 5))
	{
		cout << "Usage: " << argv[0];
		cout << " [-s <server_host>] -p <udp_port>" << endl;
		return 1;
	}
	else
	{
		//system("clear");
		for (int i = 1; i < argc; i++)
		{				
			if (strcmp(argv[i], "-p") == 0)
				udp_port = (unsigned short) atoi(argv[++i]);
			else if (strcmp(argv[i], "-s") == 0)
			{
				server_host = argv[++i];
				if (argc == 3)
				{
				    cout << "Usage: " << argv[0];
		            cout << " [-s <server_host>] -p <udp_port>" << endl;
		            return 1;
				}
		    }
	        else
	        {
	            cout << "Usage: " << argv[0];
		        cout << " [-s <server_host>] -p <udp_port>" << endl;
		        return 1;
	        }
		}
	}
	
	
	
	Client_State_T client_state = WAITING;
	string in_cmd;
	
	int client_sk = socket(AF_INET, SOCK_DGRAM, 0);
	if (client_sk < 0) {
		cout << "- failed to create UDP socket\n";
		exit(-1);
	}
	
	// setup socket
	struct sockaddr_in server_addr;
	server_addr.sin_port = htons(udp_port);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(argv[2]);
	socklen_t serverLen = sizeof(server_addr);

	struct hostent *ser = gethostbyname(argv[2]);
	if (!ser) { // for testing
		cout << "No this host!\n";
		exit(-1);
	}
	
	//recv(client_sk, DATA_BUF_LEN, DATA_BUF_LEN, 0);
	/*if(connect(client_sk, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		cout << "- failed to connect the socket\n"; // testing purpose
		exit(-1);
	}
	else {
		cout << "connected ...\n"; // testing purpose
	}*/
	
	// declare array to store command
	int buf[1024];
	// centralise the command that can be converted to integer type
	Cmd_T ls = CMD_LS;
	Cmd_T send = CMD_SEND;
	Cmd_T get = CMD_GET;
	Cmd_T rm = CMD_REMOVE;
	Cmd_T rn = CMD_RENAME;
	Cmd_T shutd = CMD_SHUTDOWN;
	Cmd_T quit = CMD_QUIT;
	Cmd_T ack = CMD_ACK;
	// declare variable to be assigned with command tag
	int cCMD;
	// content to be stored in the array
	Cmd_Msg_T cMsg;
	
	// # CUSTOM: Client Class
	Client client(client_sk, server_addr, serverLen, buf, cCMD, cMsg);
	
	bool endloop = false;
	while (!endloop)
	{
		cMsg.cmd = cCMD;
		buf[0] = (int)cMsg.cmd;
		cMsg.error = 0;
	    usleep(100);
		
	    switch(client_state)
	    {
	        case WAITING:
	        {
	            cout<<"$ ";
	            cin>>in_cmd;
	            
	            if(in_cmd == "ls")
	            {
					cCMD = ls;
	                client_state = PROCESS_LS;
	            }
	            else if(in_cmd == "send")
	            {
					cCMD = send;
	                client_state = PROCESS_SEND;
	            }
	            else if(in_cmd == "remove")
	            {
					cCMD = rm;
	                client_state = PROCESS_REMOVE;
	            }
	            else if(in_cmd == "rename")
                {
					cCMD = rn;
                    client_state = PROCESS_RENAME;
                }
	            else if(in_cmd == "shutdown")
	            {
					cCMD = shutd;
	                client_state = SHUTDOWN;
	            }
	            else if(in_cmd == "quit")
	            {
					cCMD = quit;
	                client_state = QUIT;
	            }
	            else
	            {
	                cout<<" - wrong command."<<endl;
	                client_state = WAITING;
	            }
	            break;
	        }
	        case PROCESS_LS:
	        {
				//cout << buf[0] << " " << (int)cMsg.error << endl;
		        client_state = WAITING;
				client.processLs();			// # CUSTOM: Client Class
	            break;
	        }
	        case PROCESS_SEND:
	        {
				//cout << (int)cMsg.cmd << endl;
		        client_state = WAITING;
				client.processSend();		// # CUSTOM: Client Class
		        break;
	        }
	        case PROCESS_REMOVE:
	        {	           
	            client_state = WAITING;
				client.processRemove();		// # CUSTOM: Client Class
	            break;
	        }
	        case PROCESS_RENAME:
	        {	           
	            client_state = WAITING;
				client.processRename();		// # CUSTOM: Client Class
	            break;
	        }	
	        case SHUTDOWN:
	        {	
				//buf[0] = (int)cMsg.cmd;
				/*cout << buf[0] << endl;
				if (sendto(client_sk, buf, sizeof(buf), 0, (struct sockaddr *)&server_addr, serverLen) != -1) {
					cout << "Sent shutdown cmd\n"; // testing purpose
				}*/
				client_state = WAITING; // testing purpose
				client.shutdownServer();	// # CUSTOM: Client Class
				//endloop = true;
	            break;	            
	        }
	        case QUIT:
	        {
				endloop = true;
				break;
	        }
	        default:
	        {
	        	client_state = WAITING;
	            break;
	        }    
	    }
	}
	
	close(client_sk);
    return 0;
}
