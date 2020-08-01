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

char *readALineFrom(int fromWhat);

char *readAFileFrom(int fromWhat);

int main(int argc, const char *argv[])
{
    int socketDescriber; // socket Describer, return by socket()
    in_addr_t socketAddress; // internet address
    int portNumber = 55667; // internet port
    int returnStatus; // return status from child process.
    struct sockaddr_in serverAddress; // server socket address
    socklen_t serverAddressLength; // server socket address length

    // TEST:
    int pid = 0;

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

    if (inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr) < 0)
    {
        // Error handle
        fprintf(stderr, "ERROR: inet_pton() failed.\n");
        exit(1);
    }

    // Connect to the server.
    if (connect(socketDescriber, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0)
    {
        // Error handle.
        fprintf(stderr, "ERROR: Connect socket fail.\n");
        exit(1);
    }

    // inside a dead loop
    while (1)
    {
        char command[256]; // Store command
        char fileName[256]; // Store file name
        // print a line of hint to screen.
        fprintf(stdout, "Please enter a command to send.\n");
        // get the command from console
        scanf("%s", command);
        // is the command equal to "quit"?
        if (strcmp(command, "quit") == 0)
        {
            // if it's equal to "quit", send "quit" to the server
            if (write(socketDescriber, "quit", 4) == -1)
            {
                // error handle
                fprintf(stderr, "ERROR: Send command quit error!\n");
                continue;
            }
            // close the connection.
            close(socketDescriber);
            exit(0);
        }
            // if the command is "get"
        else if (strcmp(command, "get") == 0)
        {
            // get the file name from console
            scanf("%s", fileName);
            // check is there a file name exist
            if (strlen(fileName) != 0)
            {
                // open or create, and clean the file.
                int fileDescriptor = open(fileName, O_RDWR | O_CREAT | O_TRUNC,
                                          S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                // read the file content from the socket connection.
                char *content = readAFileFrom(socketDescriber);
                // write the file content to the file.
                if (write(fileDescriptor, content, sizeof(content)) == -1)
                {
                    // error handle
                    fprintf(stderr, "ERROR: get command, write to file error!\n");
                    continue;
                }
                // close file descriptor
                close(fileDescriptor);
                // free dynamic array
                free(content);
                // continue for the next command.
                continue;
            }
            else
            {
                // input exception handle.
                fprintf(stdout, "Please enter a valid file name.\n");
                continue;
            }
        }
            // if the command is "put"
        else if (strcmp(command, "put") == 0)
        {
            // get the file name from console
            scanf("%s", fileName);
            // check is there a file name exist
            if (strlen(fileName) != 0)
            {
                // check the existence of the file
                if (access(fileName, F_OK) != -1)
                {
                    // open or create, and clean the file.
                    int fileDescriptor = open(fileName, O_RDWR | O_CREAT | O_TRUNC,
                                              S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                    char *content = readAFileFrom(fileDescriptor);
                    if (write(socketDescriber, content, sizeof(content)) == -1)
                    {
                        // error handle
                        fprintf(stderr, "ERROR: put command, write to socket error!\n");
                        continue;
                    }
                    // close file descriptor
                    close(fileDescriptor);
                    // free dynamic array
                    free(content);
                    // continue for the next command.
                    continue;
                }
                else
                {
                    // input exception handle.
                    fprintf(stdout, "No such file called: %s\n", fileName);
                }
            }
            else
            {
                // input exception handle.
                fprintf(stdout, "Please enter a valid file name.\n");
                continue;
            }
        }
        else
        {
            // input exception handle.
            fprintf(stdout, "Please enter a valid command.\n");
            fprintf(stdout, "<command> [file name]\n");
            continue;
        }
    }
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