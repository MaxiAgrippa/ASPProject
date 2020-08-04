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

char *concatenateMessage(const char *s1, const char *s2);

void freeCharDynamicArray(char *array);

int main(int argc, const char *argv[])
{
    int socketDescriber; // socket Describer, return by socket()
    in_addr_t socketAddress; // internet address
    int portNumber = 55667; // internet port
    int returnStatus; // return status from child process.
    struct sockaddr_in serverAddress; // server socket address
    socklen_t serverAddressLength; // server socket address length

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

    // tranfer string IPv4 address to binary IPv4 address and give it to sin_addr.
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
        char *command = (char *) malloc(sizeof(char) * 256); // Store command
        char *fileName = (char *) malloc(sizeof(char) * 256); // Store file name
        char EOT = 4; // end of transmit.
        // print a line of hint to screen.
        fprintf(stdout, "Please enter a command to send.\n");
        // get the command from console
        scanf("%s", command);
        // is the command equal to "quit"?
        if (strcmp(command, "quit") == 0)
        {
            // if it's equal to "quit", send "quit" to the server
            if (write(socketDescriber, command, strlen(command)) == -1)
            {
                // error handle
                fprintf(stderr, "ERROR: Send command quit error!\n");
                exit(1);
            }
            // close the connection.
            close(socketDescriber);
            // free dynamic array.
            freeCharDynamicArray(command);
            freeCharDynamicArray(fileName);
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
                // format the message to server.
                char *message = concatenateMessage(command, fileName);

                // TEST: command gonna send:
                fprintf(stderr, "Sending Command: %s\n", message);

                // send message to server.
                if (write(socketDescriber, message, strlen(message)) == -1)
                {
                    // error handle
                    fprintf(stderr, "ERROR: get command, send message to server error!\n");
                    // free dynamic array
                    freeCharDynamicArray(message);
                    freeCharDynamicArray(command);
                    freeCharDynamicArray(fileName);
                    continue; // go to the next loop
                }
                // Show a message represent working.
                fprintf(stderr, "getting file...\n");
                // read the file content from the socket connection.
                char *content = readAFileFrom(socketDescriber);
                // check if there is the file content or not.
                if (strlen(content) == 0)
                {
                    // ERROR: Something wrong from here.
                    fprintf(stderr, "Get File Error: %s can not access.\n", fileName);
                    // free dynamic arrays
                    freeCharDynamicArray(content);
                    freeCharDynamicArray(message);
                    freeCharDynamicArray(command);
                    freeCharDynamicArray(fileName);
                    continue; // go to the next loop
                }
                // check the message from server is "No such file." or not.
                if (strcmp(content, "No such file.") == 0)
                {
                    // show an error message.
                    fprintf(stderr, "Get File Error: %s, no such file.\n", fileName);
                    // free dynamic arrays
                    freeCharDynamicArray(content);
                    freeCharDynamicArray(message);
                    freeCharDynamicArray(command);
                    freeCharDynamicArray(fileName);
                    continue; // go to the next loop
                }
                // open or create, and clean the file to write.
                int fileDescriptor = open(fileName, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                // if something wrong with open(), open() return -1
                if (fileDescriptor == -1)
                {
                    // show an error message and exit.
                    fprintf(stderr, "PUT ERROR: open file error!\n");
                    // free dynamic arrays
                    freeCharDynamicArray(content);
                    freeCharDynamicArray(message);
                    freeCharDynamicArray(command);
                    freeCharDynamicArray(fileName);
                    continue; // go to the next loop
                }
                // write the file content to the file.
                if (write(fileDescriptor, content, strlen(content)) == -1)
                {
                    // error handle
                    fprintf(stderr, "ERROR: get command, write to file error!\n");
                    // free dynamic arrays
                    freeCharDynamicArray(content);
                    freeCharDynamicArray(message);
                    freeCharDynamicArray(command);
                    freeCharDynamicArray(fileName);
                    // close fileDescriptor
                    close(fileDescriptor);
                    continue; // go to the next loop
                }
                // close file descriptor
                close(fileDescriptor);
                // free dynamic arrays
                freeCharDynamicArray(content);
                freeCharDynamicArray(message);
                freeCharDynamicArray(command);
                freeCharDynamicArray(fileName);
                // Show a success message.
                fprintf(stderr, "SUCCESS: get %s success.\n\n", fileName);
                // get file transfer finished
                continue; // go to the next loop
            }
            else
            {
                // input exception handle.
                fprintf(stdout, "Please enter with a file name.\n");
                // free dynamic arrays
                freeCharDynamicArray(command);
                freeCharDynamicArray(fileName);
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
                    // format message to server.
                    char *message = concatenateMessage(command, fileName);
                    // send message to server.
                    if (write(socketDescriber, message, strlen(message)) == -1)
                    {
                        // error handle
                        fprintf(stderr, "ERROR: put command, send message to server error!\n");
                        // free dynamic arrays.
                        freeCharDynamicArray(message);
                        freeCharDynamicArray(command);
                        freeCharDynamicArray(fileName);
                        continue; // go to the next loop
                    }
                    // open or create, and clean the file.
                    int fileDescriptor = open(fileName, O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                    // set file position to the beginning of the file.
                    lseek(fileDescriptor, 0, SEEK_SET);
                    // get file content from local fileDescriptor
                    char *content = readAFileFrom(fileDescriptor);
                    // ERROR: SERVER ERROR!!!SERVER ERROR!!!SERVER ERROR!!!SERVER ERROR!!!SERVER ERROR!!!SERVER ERROR!!!
                    // write file content to server
                    if (write(socketDescriber, content, strlen(content)) == -1)
                    {
                        // error handle
                        fprintf(stderr, "ERROR: put command, write to socket error!\n");
                        // free dynamic arrays
                        freeCharDynamicArray(content);
                        freeCharDynamicArray(message);
                        freeCharDynamicArray(command);
                        freeCharDynamicArray(fileName);
                        continue; // go to the next loop
                    }
                    // FIXME: Is this useful?
                    // send EOT to client.
                    if (write(socketDescriber, &EOT, 1) == -1)
                    {
                        // error handle
                        fprintf(stderr, "GET ERROR: write EOT to client error!\n");
                        // free dynamic arrays
                        freeCharDynamicArray(content);
                        freeCharDynamicArray(message);
                        freeCharDynamicArray(command);
                        freeCharDynamicArray(fileName);
                        continue; // go to the next loop
                    }
                    // close file descriptor
                    close(fileDescriptor);
                    // free dynamic arrays
                    freeCharDynamicArray(content);
                    freeCharDynamicArray(message);
                    freeCharDynamicArray(command);
                    freeCharDynamicArray(fileName);
                    // Show a success message.
                    fprintf(stderr, "SUCCESS: put %s success.\n\n", fileName);
                    // put file transfer finished
                    continue; // go to the next loop
                }
                else // if file is not exit.
                {
                    // input exception handle.
                    fprintf(stdout, "No such file called: %s\n", fileName);
                    // free dynamic arrays
                    freeCharDynamicArray(command);
                    freeCharDynamicArray(fileName);
                    continue;
                }
            }
            else // if file name is empty.
            {
                // input exception handle.
                fprintf(stdout, "Please enter a valid file name.\n");
                // free dynamic arrays
                freeCharDynamicArray(command);
                freeCharDynamicArray(fileName);
                continue;
            }
        }
        else
        {
            // input exception handle.
            fprintf(stdout, "Please enter with a file name.\n");
            fprintf(stdout, "<command> [file name]\n");
            // free dynamic arrays
            freeCharDynamicArray(command);
            freeCharDynamicArray(fileName);
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

char *concatenateMessage(const char *s1, const char *s2)
{
    const size_t len1 = strlen(s1); // string s1 length
    const size_t len2 = strlen(s2); // string s2 length
    char *result = (char *) malloc((len1 + len2 + 1 + 1) * sizeof(char)); // +1 for " ", +1 for the null-terminator
    memcpy(result, s1, len1); // put s1 in result
    memcpy(result + len1, " ", 1); // put " " in result
    memcpy(result + len1 + 1, s2, len2); // put s2 in result
    memcpy(result + len1 + 1 + len2, "\0", len2 + 1 + 1); // +1 to copy the null-terminator
    return result;
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