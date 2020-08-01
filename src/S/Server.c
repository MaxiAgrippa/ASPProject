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

    // TEST:
    int pid = 0;
    int WEXITSTATUS_returnStatus;

    // htonl() converts the unsigned integer hostlong from host byte order to network byte order.
    socketAddress = htonl(INADDR_ANY);

    // Create a socket and assign it to the socketDescriber.
    if ((socketDescriber = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        // Error handle.
        fprintf(stderr, "ERROR: Create socket fail.\n");
        exit(1);
    }

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
        // TEST:
        fprintf(stderr, "accepting...\n");

        // The server gets a socket for an incoming client connection.
        if ((clientDescriber = accept(socketDescriber, &clientAddress, &clientAddressLength)) == -1)
        {
            // Error handle.
            fprintf(stderr, "ERROR: Accept socket fail.\n");
            exit(1);
        }
        fprintf(stdout, "connection accepted, clientDescriber: %d\n", clientDescriber);

        // fork() a child process to handle the connection.
        if ((pid = fork()) != 0)
        {
            // child process, call serviceClient() to handle the connection.
            serviceClient(clientDescriber);
        }

//        // TODO: why close there?
//        // close the connection.
//        if (close(clientDescriber) == -1)
//        {
//            // Error handle.
//            fprintf(stderr, "ERROR: Close socket fail, clientDescriber: %d\n", clientDescriber);
//            exit(1);
//        }
//        fprintf(stdout, "connection closed, clientDescriber: %d\n", clientDescriber);
        // accept children's return, avoid zombie.¬
        waitpid(0, &returnStatus, WNOHANG);
        if ((WEXITSTATUS_returnStatus = WEXITSTATUS(returnStatus)) == 0)
        {
            fprintf(stdout, "Child Process Exit: pid: %d, clientDescriber: %d, returnStatus: %d\n", pid, clientDescriber,
                    WEXITSTATUS_returnStatus);
        }
    }

    // If anything goes wrong, exit with 2.
    exit(2);
}

void serviceClient(int clientDescriber)
{
    char *messageFromClient; // store the message from the client.
    char *command = (char *) malloc(sizeof(char) * 256); // store command from the client.
    char *fileName = (char *) malloc(sizeof(char) * 256); // store file name from the client.
    char *fileContent = "";
    int fileSize = 0;
    char EOT = 4;
    while (1)
    {
        // get the message from client.
        messageFromClient = readALineFrom(clientDescriber);
        // when there is no message from client, skip the loop.
        if (strlen(messageFromClient) == 0)
        {
            continue;
        }

        // print the message from client.
        fprintf(stderr, "messageFromClient: %s\n", messageFromClient);

        // check if message from client is "quit"?
        if (strcmp(messageFromClient, "quit") == 0)
        {
            // if the message is "quit", free the dynamic array and close socket.
            free(messageFromClient);
            free(command);
            free(fileName);
            if (close(clientDescriber) == -1)
            {
                // Error handle.
                fprintf(stderr, "ERROR: Close socket fail, clientDescriber: %d\n", clientDescriber);
                exit(1);
            }
            fprintf(stderr, "clientDescriber: %d closed.\n", clientDescriber);
            exit(0);
        }
            // check if the message contain two string variables.
        else if (sscanf(messageFromClient, "%s %s", command, fileName) == 2)
        {
            // if the command is get, which means read file.
            if (strcmp(command, "get") == 0)
            {
                // check file existence
                if (access(fileName, F_OK) != -1)
                {
                    // open or create, and clean the file sharedFile.bin.
                    int fileDescriptor = open(fileName, O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                    // if something wrong with open(), open() return -1
                    if (fileDescriptor == -1)
                    {
                        // show an error message and exit.
                        write(STDERR_FILENO, "GET ERROR: open file error!\n", 28);
                        exit(1);
                    }
                    // read a file from fileDescriptor
                    fileContent = readAFileFrom(fileDescriptor);
                    // send file content to client.
                    if (write(clientDescriber, fileContent, strlen(fileContent)) == -1)
                    {
                        // error handle
                        write(STDERR_FILENO, "GET ERROR: write file to client error!\n", 39);
                        exit(1);
                    }
                    // send EOT to client.
                    if (write(clientDescriber, &EOT, 1) == -1)
                    {
                        // error handle
                        write(STDERR_FILENO, "GET ERROR: write EOT to client error!\n", 38);
                        exit(1);
                    }
                    // close the opening file descriptor
                    close(fileDescriptor);
                }
                else
                {
                    fprintf(stderr, "GET ERROR: File not exit.");
                }
            }
                // if the command is put, which means write file.
            else if (strcmp(command, "put") == 0)
            {
                // open or create, and clean the file.
                int fileDescriptor = open(fileName, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                // if something wrong with open(), open() return -1
                if (fileDescriptor == -1)
                {
                    // show an error message and exit.
                    write(STDERR_FILENO, "PUT ERROR: open file error!\n", 28);
                    exit(1);
                }
                // read a file from clientDescriber
                fileContent = readAFileFrom(clientDescriber);
                // write file content to local fileDescriptor.
                if (write(fileDescriptor, fileContent, strlen(fileContent)) == -1)
                {
                    // error handle
                    write(STDERR_FILENO, "PUT ERROR: write local file error!\n", 35);
                    exit(1);
                }
                if (read(clientDescriber, messageFromClient, sizeof(messageFromClient)) == -1)
                {
                    // error handle
                    write(STDERR_FILENO, "PUT ERROR: read EOT error!\n", 27);
                    exit(1);
                }
                // if the EOT signal received.
                if (strcmp(messageFromClient, &EOT) == 0)
                {
                    // close the opening file descriptor
                    close(fileDescriptor);
                }
                close(fileDescriptor);
            }
        }
            // exception handle
        else
        {
            fprintf(stderr, "Message format dis-match.");
        }

    }
    free(messageFromClient);
    free(command);
    free(fileName);
    close(clientDescriber);
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