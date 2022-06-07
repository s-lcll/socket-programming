***message.h***
This header file includes the necessary variables that will be used in other source file

***server.h***
This header file includes all of the main functions that will be used in the server.cc file

***server.cc***
This file is the main file to be run as a server side, mainly calling the function in the server.h to process

***client.h***
This header file includes all of the main functions that will be used in the client.cc file

***client.cc***
This file is the main file to be run as a client side, mainly calling the function in the client.h to process

***makefile***
Used to compile the source code. The compile version is c++11.


To compile the file, firstly change the mode of the file to be executable, then type the command "make"
in the command window

Please use the latest gcc (8.x.x or 9.x.x) with later c++ standards to compile as much as possible