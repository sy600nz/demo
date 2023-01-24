/*!
*   \file client.c
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "client.h"

/*--- Defines/Macros/Types -------------------------------------------------------------------------*/
typedef struct
{
    int sockFD;
    TClientStatus state;
    char fileName[FILE_NAME_LENGTH_MAX];
} TClientManager;

/*--- Constants ------------------------------------------------------------------------------------*/
static const struct timeval NoneTimeout = {0, 0};
static const struct timeval ReceivingTimeout = {5, 0};                  /* Adjust this to change waiting time between processing next file */
static const char testIpAddree[] = "127.0.0.1";                         /* Change this if testing other place */
static const int testPort = 8080;                                       /* Test port */


/*--- Prototypes -----------------------------------------------------------------------------------*/
static int WriteFile(char * fileName, int sockfd);

/*--- Variables ------------------------------------------------------------------------------------*/


/*--- Local Function Implementation ----------------------------------------------------------------*/
static void * ClientProcessRecieving(void * client){

    TClientManager * manager = (TClientManager *) client;
    char data[FILE_NAME_LENGTH_MAX];
    FILE *fp;

    while(1)
    {
        switch (manager->state)
        {
            case REQUEST_SINGLE_FILE:
            {
                printf(manager->fileName);
                WriteFile(manager->fileName, manager->sockFD);
                manager->state = CLIENT_SATATUS_IDLE;
                break;
            }

            case TRANSFER_ALL_FILES:
            {
                bzero(data, sizeof(data));
                int readCount = recv(manager->sockFD, data, sizeof(data), 0);
                if(readCount > 0)
                {
                    strcpy(manager->fileName, data);
                    printf("receiving file name: %s \r\n", manager->fileName);
                    WriteFile(manager->fileName, manager->sockFD);
                }
                else
                {
                    printf("timeout %d\n", readCount);
                    break;
                }
                break;
            }

            default:
            {
                sleep(1);
                break;
            }
        }
    }
}


static int WriteFile(char * fileName, int sockfd)
{
    int readCount;
    FILE * fp;
    char buffer[DATA_BUFFER_SIZE_MAX];

    fp = fopen(fileName, "w");                          /* Only support text files */
    if(!fp)
    {
        printf("Cannot open file: %s \r\n", fileName);
        return -1;
    }

    /* Change Rx timeout in case waiting too long */
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &ReceivingTimeout, sizeof(ReceivingTimeout)) < 0)
    {
        printf("setsockopt rx timeout failed\r\n");
    }

    /* Receiving file data */
    while (1)
    {
        bzero(buffer, sizeof(buffer));
        readCount = recv(sockfd, buffer, sizeof(buffer), 0);
        if (readCount > 0)
        {
            fprintf(fp, "%s", buffer);          /* Write text stream data */
        }
        else
        {
            printf("timeout %d\r\n", readCount);
            break;
        }
    }
    fclose(fp);
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &NoneTimeout, sizeof(NoneTimeout)) < 0)
    {
        printf("setsockopt reset failed\r\n");
    }

    return 0;
}



int main()
{
    int e;
    int sockfd;
    struct sockaddr_in server_addr;
    TClientManager client;
    pthread_t thread;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0)
    {
        printf("Error in create socket");
        exit(1);
    }
    printf("Client server socket created successfully.\r\n");
    client.sockFD = sockfd;
    client.state = CLIENT_SATATUS_IDLE;

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = testPort;
    server_addr.sin_addr.s_addr = inet_addr(testIpAddree);

    e = connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(e == -1)
    {
        printf("Error when connecting");
        exit(1);
    }
    printf("Connected to Server.\n");

    pthread_create(&thread, NULL, ClientProcessRecieving, (void *) &client );

    while(1)
    {
        char input[FILE_NAME_LENGTH_MAX];

        bzero(input, sizeof(input));
        printf("Please select options: \r\n 'file'--- tranfer one file from the server \r\n 'all'--- transfer all files from the server \r\n 'exit' --- shutdown\r\n");
        scanf("%s",input);

        if(strcmp(input,"file") == 0)
        {
            send(sockfd, input, sizeof(input), 0);          /* Send task file */
            bzero(input, sizeof(input));
            printf("Please enter the file name which is in the server.\r\n");
            scanf("%s",input);
            strncpy(client.fileName, input, strlen(input));
            client.state = REQUEST_SINGLE_FILE;
            send(sockfd, input, sizeof(input), 0);          /* Send file name */
        }
        else if(strcmp(input,"all") == 0)
        {
            client.state = TRANSFER_ALL_FILES;
            send(sockfd, input, sizeof(input), 0);          /* Send task all */
        }
        else if(strcmp(input,"exit") == 0)
        {
            break;
        }
        else
        {
            printf("An invalid option: %s", input);
        }

        printf("----\r\n");
    }
    printf("---END---");

    return 0;
}