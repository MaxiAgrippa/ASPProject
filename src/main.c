//
// Created by Maxi on 7/31/20.
//
#include <zconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

char *readALineFrom(int fromWhat);

int main()
{
    char c[256];
    char f[256];
    char *m;
    m = readALineFrom(STDOUT_FILENO);
    printf("%s\n", m);
    sscanf("get test.txt","%s %s", c, f);
    printf("%s\n%s", c, f);
    free(m);
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
