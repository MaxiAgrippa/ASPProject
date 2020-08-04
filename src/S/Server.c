//
// Created by Maxi on 7/31/20.
//
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>

void serviceClient(int clientDescriber);

char *readALineFrom(int fromWhat);

char *readAFileFrom(int fromWhat);

void freeCharDynamicArray(char *array);

int main(int argc, const char *argv[])
{
    int socketDescriber; // socket Describer, return by socket()
    int clientDescriber; // client Describer, return by accept(), represent the client
    in_addr_t socketAddress; // internet address
    int portNumber = 55667; // internet port
    int returnStatus; // return status from child process.
    struct sockaddr_in serverAddress; // server socket address
    socklen_t serverAddressLength; // server socket address length

    struct sockaddr clientAddress; // client socket address
    socklen_t clientAddressLength; // client socket address length

    int pid = 0; // store child pid
    int WEXITSTATUS_returnStatus; // store return status

    // htonl() converts the unsigned integer hostlong from host byte order to network byte order.
    socketAddress = htonl(INADDR_ANY);

    // Create a socket and assign it to the socketDescriber.
    if ((socketDescriber = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        // Error handle.
        fprintf(stderr, "ERROR: Create socket fail.\n");
        exit(1);
    }

    // make the port reusable.
    int true = 1;
    setsockopt(socketDescriber, SOL_SOCKET, SO_REUSEADDR, &true, sizeof(int));

    // Clarify a socket address structure
    serverAddress.sin_family = AF_INET; //AF_INET(IPv4) or AF_INET6(IPv6)
    serverAddress.sin_addr.s_addr = socketAddress; // 32-bit IP4 address
    serverAddress.sin_port = htons((uint16_t) portNumber); // 16-bit port number

    // bind() assigns a local protocol address to a socket.
    if (bind(socketDescriber, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) == -1)
    {
        // Error handle.
        fprintf(stderr, "ERROR: Bind socket fail.\n");
        exit(1);
    }

    // listen() is called only by a TCP server to accept connections from client sockets that will issue a connect()
    if (listen(socketDescriber, 5) == -1)
    {
        // Error handle.
        fprintf(stderr, "ERROR: Listen socket fail.\n");
        exit(1);
    }

    // A dead loop to assign local client(child process) to each connection request.
    while (1)
    {
        // Show a message that indicate Server is running.
        fprintf(stdout, "Ready to accept clients...\n");
        // The server gets a socket for an incoming client connection.
        // This will pause the process.
        if ((clientDescriber = accept(socketDescriber, &clientAddress, &clientAddressLength)) == -1)
        {
            // Error handle.
            fprintf(stderr, "ERROR: Accept socket fail.\n");
            exit(1);
        }

        fprintf(stdout, "connection accepted, clientDescriber: %d\n", clientDescriber);

        // fork() a child process to handle the connection.
        if ((pid = fork()) == 0)
        {
            // child process, call serviceClient() to handle the connection.
            serviceClient(clientDescriber);
        }

        // close the connection in main process
        if (close(clientDescriber) == -1)
        {
            // Error handle.
            fprintf(stderr, "ERROR: Close socket fail, clientDescriber: %d\n", clientDescriber);
            exit(1);
        }
        // accept children's return, avoid zombie.
        int p = waitpid(0, &returnStatus, WNOHANG);
        // when a child process exit, print a message.
        if ((WEXITSTATUS_returnStatus = WEXITSTATUS(returnStatus)) == 0 && (p != 0))
        {
            fprintf(stdout, "Child Process Exit: pid: %d, clientDescriber: %d, returnStatus: %d\n", p,
                    clientDescriber,
                    WEXITSTATUS_returnStatus);
        }
    }

    // If anything goes wrong, exit with 2.
    exit(2);
}

void serviceClient(int clientDescriber)
{
    char *messageFromClient = NULL; // store the message from the client.
    char *command = NULL; // store command from the client.
    char *fileName = NULL; // store file name from the client.
    char *fileContent = NULL;
    int fileSize = 0;
    char EOT = 4;
    int condition = 1; // control the dead loop
    while (condition)
    {
        command = (char *) malloc(sizeof(char) * 256); // store command from the client.
        fileName = (char *) malloc(sizeof(char) * 256); // store file name from the client.

        // show a message that indicate running.
        fprintf(stderr, "waiting for command...\n");

        // get the message from client.
//        messageFromClient = readALineFrom(clientDescriber);
        // TEST: using fix size malloc to reduce factors.
        messageFromClient = (char *) malloc(sizeof(char) * 255);
        int i = read(clientDescriber, messageFromClient, 255);

        // -1 indicate something wrong with the connection.
        if (i == -1)
        {
            // free dynamic array
            freeCharDynamicArray(messageFromClient);
            freeCharDynamicArray(command);
            freeCharDynamicArray(fileName);
            exit(1); // child process exit.

        }

        // when there is no message from client, skip the loop.
        if (strlen(messageFromClient) == 0)
        {
            // TODO: fix potential error
            // free dynamic array
            freeCharDynamicArray(messageFromClient);
            freeCharDynamicArray(command);
            freeCharDynamicArray(fileName);
            continue; // go to the next loop.
        }

        // print the message from client.
        fprintf(stderr, "messageFromClient: %s\n", messageFromClient);

        // check if message from client is "quit"?
        if (strcmp(messageFromClient, "quit") == 0)
        {
            // if the message is "quit", free the dynamic array and close socket.
            freeCharDynamicArray(messageFromClient);
            freeCharDynamicArray(command);
            freeCharDynamicArray(fileName);
            fprintf(stderr, "clientDescriber: %d closed by client command.\n\n", clientDescriber);
            exit(0);
        }
            // check if the message contain two string variables.
        else if (sscanf(messageFromClient, "%s %s", command, fileName) == 2)
        {
            // TEST: Show the command and file name.
            fprintf(stderr, "command: %s fileName: %s\n", command, fileName);

            // if the command is get, which means read file.
            if (strcmp(command, "get") == 0)
            {
                // show a message that indicate checking file.
                fprintf(stderr, "Checking file exists....\n");
                // check file existence
                if (access(fileName, F_OK) != -1)
                {
                    // show a message that indicate file exists.
                    fprintf(stderr, "file exists.\n");
                    // open or create, and clean the file sharedFile.bin.
                    int fileDescriptor = open(fileName, O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                    // if something wrong with open(), open() return -1
                    if (fileDescriptor == -1)
                    {
                        // show an error message and exit.
                        fprintf(stderr, "GET ERROR: open file error!\n");
                        // free malloc the char array
                        freeCharDynamicArray(messageFromClient);
                        freeCharDynamicArray(command);
                        freeCharDynamicArray(fileName);
                        // close the opening file descriptor
                        close(fileDescriptor);
                        exit(1); // error exit child thread
                    }
                    // FIXME: Is it necessary?
                    // set file position to the beginning of the file.
                    lseek(fileDescriptor, 0, SEEK_SET);
                    // read a file from fileDescriptor
                    fileContent = readAFileFrom(fileDescriptor);
                    // show a message that indicate the file transfer is begin.
                    fprintf(stderr, "file gonna send.\n");
                    // send file content to client.
                    if (write(clientDescriber, fileContent, strlen(fileContent)) == -1)
                    {
                        // error handle
                        fprintf(stderr, "GET ERROR: write file to client error!\n");
                        // free malloc the char array
                        freeCharDynamicArray(fileContent);
                        freeCharDynamicArray(command);
                        freeCharDynamicArray(fileName);
                        freeCharDynamicArray(messageFromClient);
                        // close the opening file descriptor
                        close(fileDescriptor);
                        exit(1); // error exit child thread
                    }
                    // show a message that indicate the file transfer is successfully finished.
                    fprintf(stderr, "GET SUCCESS: File tranfer finished.\n\n");

                    // FIXME: Is this useful?
                    /**
                    // send EOT to client.
                    if (write(clientDescriber, &EOT, 1) == -1)
                    {
                        // error handle
                        fprintf(stderr, "GET ERROR: write EOT to client error!\n");
                        // free malloc the char array
                        freeCharDynamicArray(fileContent);
                        freeCharDynamicArray(command);
                        freeCharDynamicArray(fileName);
                        freeCharDynamicArray(messageFromClient);
                        // close the opening file descriptor
                        close(fileDescriptor);
                        exit(1); // error exit child thread
                    }
                     **/

                    // free the dynamic char array
                    freeCharDynamicArray(fileContent);
                    freeCharDynamicArray(command);
                    freeCharDynamicArray(fileName);
                    // close the opening file descriptor
                    close(fileDescriptor);
                    // go to the next loop.
                    continue;
                }
                else // if no such file called fileName
                {
                    fprintf(stderr, "GET ERROR: File not exist.\n");
                    // write an error message to the client.
                    if (write(clientDescriber, "No such file.\004", 14) == -1)
                    {
                        // error handle
                        fprintf(stderr, "GET ERROR: write message to client error!\n");
                        exit(1);
                    }
                    // free dynamic arrays.
                    free(messageFromClient);
                    freeCharDynamicArray(command);
                    freeCharDynamicArray(fileName);
                    // go to the next loop.
                    continue;
                }
                // exit if something horrible happend.
                condition = 0;
            }
                // if the command is put, which means write file.
            else if (strcmp(command, "put") == 0)
            {
                // show a message that indicate the file transfer is begin.
                fprintf(stderr, "file gonna receiving.\n");
                // read a file from clientDescriber
                fileContent = readAFileFrom(clientDescriber);

                // check the fileContent from client is empty or not.
                if (strlen(fileContent) == 0)
                {
                    // if it's empty:
                    fprintf(stderr, "PUT ERROR: received empty file error.\n");
                    // free malloc the char array
                    freeCharDynamicArray(fileContent);
                    freeCharDynamicArray(command);
                    freeCharDynamicArray(fileName);
                    freeCharDynamicArray(messageFromClient);
                    // go to the next loop.
                    continue;
                }

                // open or create, and clean the file.
                int fileDescriptor = open(fileName, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                // if something wrong with open(), open() return -1
                if (fileDescriptor == -1)
                {
                    // show an error message and exit.
                    fprintf(stderr, "PUT ERROR: open file error!\n");
                    // free malloc the char array
                    freeCharDynamicArray(fileContent);
                    freeCharDynamicArray(command);
                    freeCharDynamicArray(fileName);
                    freeCharDynamicArray(messageFromClient);
                    exit(1); // error exit child thread
                }

                // write file content to local fileDescriptor.
                if (write(fileDescriptor, fileContent, strlen(fileContent)) == -1)
                {
                    // error handle
                    fprintf(stderr, "PUT ERROR: write local file error!\n");
                    // free malloc the char array
                    freeCharDynamicArray(fileContent);
                    freeCharDynamicArray(command);
                    freeCharDynamicArray(fileName);
                    freeCharDynamicArray(messageFromClient);
                    // close the opening file descriptor
                    close(fileDescriptor);
                    exit(1); // error exit child thread
                }

                /**
                // check for EOT signal.
                if (read(clientDescriber, messageFromClient, sizeof(messageFromClient)) == -1)
                {
                    // error handle
                    fprintf(stderr, "PUT ERROR: read EOT error!\n");
                    // free malloc the char array
                    if (fileContent != NULL)
                    {
                        free(fileContent);
                        fileContent = NULL;
                    }
                    // close the opening file descriptor
                    close(fileDescriptor);
                    exit(1);
                }
                // if the EOT signal received.
                if (strcmp(messageFromClient, &EOT) == 0)
                {
                    // close the opening file descriptor
                    // free malloc the char array
                    if (fileContent != NULL)
                    {
                        free(fileContent);
                        fileContent = NULL;
                    }
                    // close the opening file descriptor
                    close(fileDescriptor);
                }
                 **/

                // show a message that indicate the file transfer is successfully finished.
                fprintf(stderr, "PUT SUCCESS: File tranfer finished.\n\n");

                // free malloc the char array
                freeCharDynamicArray(fileContent);
                freeCharDynamicArray(command);
                freeCharDynamicArray(fileName);
                freeCharDynamicArray(messageFromClient);
                // close the opening file descriptor
                close(fileDescriptor);
                // go to the next loop.
                continue;
            }
            else // exception handle
            {
                // if the command is neither get or put.
                fprintf(stderr, "unknown command.\n");
                // free dynamic arrays.
                freeCharDynamicArray(messageFromClient);
                freeCharDynamicArray(command);
                freeCharDynamicArray(fileName);
                // go to the next loop.
                continue;
            }
            // exit if something horrible happend.
            condition = 0;
        }
        else // exception handle
        {
            // if the message does not contain two string variables.
            fprintf(stderr, "Client message format dis-match.\n");
            // free dynamic arrays.
            freeCharDynamicArray(messageFromClient);
            freeCharDynamicArray(command);
            freeCharDynamicArray(fileName);
            // go to the next loop.
            continue;
        }
        // exit if something horrible happend.
    }
    // out side of the dead loop
    // free dynamic array.
    free(messageFromClient);
    freeCharDynamicArray(command);
    freeCharDynamicArray(fileName);
    exit(0);
}

// read a line from somewhere
char *readALineFrom(int fromWhat)
{
    // temporary store a char
    char tmp;
    // indicate the index used in inputLine.
    int i = 0;
    // store the input:
    char *inputLine = (char *) malloc(64 * sizeof(char));
    // initial inputLine size
    int inputLineSize = 64;

    // when there is next byte got read.
    while (read(fromWhat, &tmp, 1) > 0)
    {
        // if the index used in inputLine is bigger or equal to the inputLine size.
        if (i >= inputLineSize)
        {
            // double the inputLine size
            inputLineSize *= 2;
            // re-allocate the inputLine to double the size.
            inputLine = realloc(inputLine, inputLineSize * sizeof(char));
        }
        // add the char to the end of the inputLine.
        inputLine[i] = tmp;
        // if tmp equal to line end, break the loop.
        if (tmp == '\n')
        { break; }
        // increase i to index on the next using unit.
        i++;
    }
    return inputLine;
}

// read an entire file from somewhere, and return a string of it.
char *readAFileFrom(int fromWhat)
{
    // temporary store a char
    char tmp;
    // indicate the index used in inputFile.
    int i = 0;
    // store the input:
    char *inputFile = (char *) malloc(64 * sizeof(char));
    // initial inputFile size
    int inputFileSize = 64;

    // when there is next byte got read.
    while (read(fromWhat, &tmp, 1) > 0)
    {
        // if the index used in inputFile is bigger or equal to the inputFile size.
        if (i >= inputFileSize)
        {
            // double the inputFile size
            inputFileSize *= 2;
            // re-allocate the inputFile to double the size.
            inputFile = realloc(inputFile, inputFileSize * sizeof(char));
        }
        // add the char to the end of the inputFile.
        inputFile[i] = tmp;
        // increase i to index on the next using unit.
        i++;
    }
    return inputFile;
}

// free char dynamic array.
void freeCharDynamicArray(char *array)
{
    if (array != NULL)
    {
        free(array);
        array = NULL;
    }
}