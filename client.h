/*!
*   \file client.h
*
*   Purpose: Client asks the Server to transfer files to it.
*
*   Requirement:
*       1. The client should connect to the server, specify the file to transfer, and receive the file to the local filesystem.
*       2. The server needs to support multiple concurrent connections.
*       3. The content is not sensitive, there is no need to restrict access or encrypt content. The files available to be transferred will all be in a single directory on the server.
*       4. The network link may have limited bandwidth and some latency.
*       5. The code needs to minimize external dependencies, please use only normal system libraries.
*
*   Implementation:
*       * User can have options to transfer files
*           1. Enter a file name which should be in the server file list and request the server to tranfer.
*           2. Request the server to transfer all files in the server file folder.
*
*       * Choose SOCK_STREAM socket type, TCP/IP layer which can provide sequenced, reliable, two-way communication to meet--The network link may have limited bandwidth
*           and some latency requirement 4. Setup and adjusting timeout can also improve limited bandwidth and some latency inssues. Changing timeout can solve requirement 4.
*/

#define FILE_NAME_LENGTH_MAX                        128
#define DATA_BUFFER_SIZE_MAX                        1024

typedef enum
{
    CLIENT_SATATUS_IDLE = 0,
    REQUEST_SINGLE_FILE,
    TRANSFER_ALL_FILES,
    REPEAT_TRANFER_FILE
} TClientStatus;