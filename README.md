# demo
demo transferring files over a network from the server to the client

This c program starts a TCP/IP socket server which can be connected with multiple clients.
Any connected client can request a specific file from the server or request all files at
the server specific folder.

# How to build
The server.c can be compiled to a server program and the client.c can generate a client exe.
The build environment is under cygwin using gcc compiler.
    gcc server.c -o server.exe
    gcc client.c -o client.exe

# How to run
Before running the server.exe, the "dataFileFolder" must be created at server.exe directory. The test
text files like: *.txt, *.c, *.mot ... should be placed here. After running the server.exe,
many client.exe can be run anywhere can connect to the server.exe. after a client connected to
the server, there are two options for user:
    "file" ---- user enter "file" will trigger the server transferring a single file mode.
                user must enter a specific file name which must be in the server "dataFileFolder".
                The file name will be sent to the server from the client, the server will open
                this file and send all file data to the client. The client will save the data
                as a file using the specific file name.
    "all" ----  user enter "all" will put the server into transferring all files in the "dataFileFolder"
                to the client.
    "exit" ---  Shutdown the program.


# Design and Implementation
1. To support multiple concurrent connections, server uses multiple threads to handle multiple client connections.
2. Choose SOCK_STREAM socket type, TCP/IP layer which can provide sequenced, reliable, two-way communication to meet
    -- 4.The network link may have limited bandwidth and some latency requirement-- Setup and adjusting timeout can
    also improve limited bandwidth and some latency issues.
3. Select text format type files to demo. The text format file has been serialised and is reliable to transfer on
    the network. To support binary format files, a binary file can be serialised and be converted to a text stream
    format file. After transferred to the client, it can be converted back to the original binary format. This part
    has not been implemented in this demo.
