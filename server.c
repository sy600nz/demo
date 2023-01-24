/*!
*   \file server.c
*
*   Purpose: Server transfers files over a network to the client.
*
*   Requirement:
*       1. The client should connect to the server, specify the file to transfer, and receive the file to the local filesystem.
*       2. The server needs to support multiple concurrent connections.
*       3. The content is not sensitive, there is no need to restrict access or encrypt content. The files available to be transferred will all be in a single directory on the server.
*       4. The network link may have limited bandwidth and some latency.
*       5. The code needs to minimize external dependencies, please use only normal system libraries.
*
*   Implementation:
*       1. Use multiple threads to handle multiple client connections.
*       2. Choose SOCK_STREAM socket type, TCP/IP layer which can provide sequenced, reliable, two-way communication to meet--The network link may have limited bandwidth
*           and some latency requirement 4. Setup and adjusting timeout can also improve limited bandwidth and some latency inssues.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <dirent.h>
#include "client.h"                 /* Get some shared define */

/*--- Defines/Macros/Types -------------------------------------------------------------------------*/
#define CLIENT_NUMBER_MAX                           128

typedef struct
{
    int index;
    int socketFD;
    struct sockaddr_in clientAddr;
    int len;
    TClientStatus state;
} TClient;

/*--- Constants ------------------------------------------------------------------------------------*/
static const char serverDataFileFolder[] = "dataFileFolder/";           /* Where are the server data file stored */
static const char testIpAddree[] = "127.0.0.1";                         /* Change this if testing other place */
static const int testPort = 8080;                                       /* Test port */

/*--- Prototypes -----------------------------------------------------------------------------------*/
static int SendFile(char * fileName, int sockfd);

/*--- Variables ------------------------------------------------------------------------------------*/



/*--- Local Function Implementation ----------------------------------------------------------------*/
static void * ProcessClientConnection(void * client)
{
    int count;
    FILE *fp;
    char data[DATA_BUFFER_SIZE_MAX];
    char fileNameWithPath[FILE_NAME_LENGTH_MAX];
    TClient * clientDetail = (TClient *)client;
    int index = clientDetail->index;
    int clientSocket = clientDetail->socketFD;

    printf("Client %d connected.\r\n",index + 1);

    while(1)
    {
        bzero(data, sizeof(data));
        count = recv(clientSocket, data, sizeof(data), 0);      /* Receive a task from client */
        if (count > 0)
        {
            if(strcmp(data,"file") == 0)
            {
                clientDetail->state = REQUEST_SINGLE_FILE;
            }
            else if(strcmp(data,"all") == 0)
            {
                clientDetail->state = TRANSFER_ALL_FILES;
            }
            else
            {
                printf("Don't support this task: %s \r\n", data);
                clientDetail->state = CLIENT_SATATUS_IDLE;
            }
        }
        switch (clientDetail->state)
        {
            case REQUEST_SINGLE_FILE:
            {
                while(1)
                {
                    /* Receive Client specified file transfer request */
                    bzero(data, sizeof(data));
                    bzero(fileNameWithPath, sizeof(fileNameWithPath));
                    count = recv(clientSocket, data, sizeof(data), 0);      /* Receive the file name from client */
                    if (count > 0)
                    {
                        strcpy(fileNameWithPath, serverDataFileFolder);
                        strcat(fileNameWithPath, data);
                        printf("Received file name: %s", fileNameWithPath);
                        if (SendFile(fileNameWithPath, clientSocket) == 0)
                        {
                            printf("File data sent successfully.\r\n");
                        }
                        else
                        {
                            clientDetail->state = CLIENT_SATATUS_IDLE;          /* Reset state and retry */
                        }
                    }
                    break;
                }
                break;
            }
            case TRANSFER_ALL_FILES:
            {
                DIR *d;
                struct dirent *dir;
                d = opendir(serverDataFileFolder);
                if (d)
                {
                    while ((dir = readdir(d)) != NULL)
                    {
                        printf("%s\n", dir->d_name);
                        if ((strcmp(dir->d_name, ".") == 0) || (strcmp(dir->d_name, "..") == 0))        /* These are not our files, skip them */
                        {
                            continue;
                        }
                        bzero(fileNameWithPath, sizeof(fileNameWithPath));
                        strcpy(fileNameWithPath, serverDataFileFolder);
                        strcat(fileNameWithPath, dir->d_name);
                        /* Send file name to client first */
                        if (send(clientSocket, dir->d_name, sizeof(dir->d_name), 0) == -1)
                        {
                            printf("Error in sending file name.");
                            clientDetail->state = CLIENT_SATATUS_IDLE;          /* Reset state and retry */
                            break;
                        }
                        sleep(1);       /* Wait for the client to change state and receive file data */
                        SendFile(fileNameWithPath, clientSocket);
                        printf("File data sent successfully.\r\n");
                        sleep(7);       /* Wait for the client to change state and receive file name */
                    }
                    closedir(d);
                }
                else
                {
                    printf("Cannot open directory: %d \r\n", serverDataFileFolder);
                }
                break;
            }

            default:
            {
                break;
            }
        }
        sleep(1);       /* Give other threads more chances */
    }
}


static int SendFile(char * fileName, int sockfd)
{
    FILE *fp;
    int readBytes;
    char data[DATA_BUFFER_SIZE_MAX];

    fp = fopen(fileName, "r");                /* Only support text files */
    if (fp == NULL)
    {
        printf("Error: cannot open the file: %s \r\n", fileName);
        return -1;
    }

    /* Send file data to client */
    bzero(data, sizeof(data));
    while(fgets(data, sizeof(data), fp) != NULL)            /* Read text file data */
    {
        if (send(sockfd, data, sizeof(data), 0) == -1)      /* Send file data to client */
        {
            printf("Error in sending file.");
            fclose(fp);
            return -1;
        }
        bzero(data, sizeof(data));
    }

    fclose(fp);
    return 0;
}


int main()
{
    int sockfd;
    struct sockaddr_in server_addr;
    int clientCount;
    TClient clients[CLIENT_NUMBER_MAX];                 /* Support multiple client connections */
    pthread_t thread[CLIENT_NUMBER_MAX];

    clientCount = 0;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);           /* Need to use TCP/IP protocol for reliable connection */
    if (sockfd < 0)
    {
        printf("Socket error.\r\n");
        exit(1);
    }
    printf("Server socket created successfully.\r\n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = testPort;
    server_addr.sin_addr.s_addr = inet_addr(testIpAddree);

    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("Binding error.\r\n");
        exit(1);
    }
    printf("Binding successfull.\r\n");

    if (listen(sockfd, CLIENT_NUMBER_MAX) == 0)
    {
        printf("Listening....\r\n");
    }
    else
    {
        printf("Error in listening\r\n");
        exit(1);
    }

    while(1)
    {
        clients[clientCount].socketFD = accept(sockfd, (struct sockaddr*) &clients[clientCount].clientAddr, &clients[clientCount].len);
        clients[clientCount].index = clientCount;
        clients[clientCount].state = CLIENT_SATATUS_IDLE;
        /* Create threads to support multiple client connections */
        pthread_create(&thread[clientCount], NULL, ProcessClientConnection, (void *) &clients[clientCount]);
        clientCount++;
    }

    for(int i = 0 ; i < clientCount ; i ++)
    {
        pthread_join(thread[i], NULL);              /* Clean up threads */
    }

    return 0;
}
