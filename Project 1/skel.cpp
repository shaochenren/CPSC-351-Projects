#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace std;

/* The pipe for parent-to-child communications */
int parentToChildPipe[2];

/* The pipe for the child-to-parent communication */
int childToParentPipe[2];

/* The read end of the pipe */
#define READ_END 0

/* The write end of the pipe */
#define WRITE_END 1

/* The maximum size of the array of hash programs */
#define HASH_PROG_ARRAY_SIZE 6

/* The maximum length of the hash value */
#define HASH_VALUE_LENGTH 1000

/* The maximum length of the file name */
#define MAX_FILE_NAME_LENGTH 1000

/* The array of names of hash programs */
const string hashProgs[] = {"md5sum",    "sha1sum",   "sha224sum",
                            "sha256sum", "sha384sum", "sha512sum"};

/**
 * The function called by a child
 * @param hashProgName - the name of the hash program
 */
void computeHash(const string &hashProgName) {
  /* The hash value buffer */
  char hashValue[HASH_VALUE_LENGTH];

  /* The received file name string */
  char fileNameRecv[MAX_FILE_NAME_LENGTH];

  /* Fill the buffer with 0's */
  memset(fileNameRecv, (char)NULL, MAX_FILE_NAME_LENGTH);

  /** TODO: Now, lets read a message from the parent */
  /** Now, lets read a message from the parent **/
  // READING
  if (read(parentToChildPipe[READ_END], fileNameRecv, MAX_FILE_NAME_LENGTH) <
      0) {
    perror("read");
    exit(-1);
  }

  /* Glue together a command line <PROGRAM NAME>.
   * For example, sha512sum fileName.
   */
  string cmdLine(hashProgName);
  cmdLine += " ";
  cmdLine += fileNameRecv;

  /** TODO: Open the pipe to the program (specified in cmdLine)
   * using popen() and save the ouput into hashValue. See popen.cpp
   * for examples using popen. */
  FILE *progOutput = popen(cmdLine.c_str(), "r");

  /* Make sure that popen succeeded */
  if (!progOutput) {
    perror("popen");
    exit(-1);
  }

  /* Reset the value buffer */
  memset(hashValue, (char)NULL, HASH_VALUE_LENGTH);

  /* Read the program output into the buffer */
  if (fread(hashValue, sizeof(char), sizeof(char) * HASH_VALUE_LENGTH,
            progOutput) < 0) {
    perror("fread");
    exit(-1);
  }

  /* Close the file pointer representing the program output */
  if (pclose(progOutput) < 0) {
    perror("perror");
    exit(-1);
  }

  fprintf(stdout, "Program output: %s\n", hashValue);
  fflush(stdout);

  /** TODO: Send a string to the parent **/
  /* Send a string to the parent */
  // WRITING
  if (write(childToParentPipe[WRITE_END], hashValue, HASH_VALUE_LENGTH) < 0) {
    perror("write");
    exit(-1);
  }

  /* Close write end of the child-to-parent pipe; this will
   * cause the parent's read() function to unblock. */
  // Set to parent to READ
  if (close(childToParentPipe[WRITE_END]) < 0) {
    perror("close");
    exit(-1);
  }

  /* The child terminates */
  exit(0);
}

int main(int argc, char **argv) {
  /* Check for errors */
  if (argc < 2) {
    fprintf(stderr, "USAGE: %s <FILE NAME>\n", argv[0]);
    exit(-1);
  }

  /* Save the name of the file */
  string fileName(argv[1]);

  /* The process id */
  pid_t pid;

  /* Run a program for each type of hashing algorithm hash algorithm */
  for (int hashAlgNum = 0; hashAlgNum < HASH_PROG_ARRAY_SIZE; ++hashAlgNum) {
    /** TODO: create two pipes **/
    /* Create a parent-to-child pipe */
    if (pipe(parentToChildPipe) < 0) {
      perror("pipe");
      exit(-1);
    }

    /* Create a child-to-parent pipe */
    if (pipe(childToParentPipe) < 0) {
      perror("pipe");
      exit(-1);
    }

    /* Fork a child process and save the id */
    if ((pid = fork()) < 0) {
      perror("fork");
      exit(-1);
    }
    /* I am a child */
    else if (pid == 0) {
      /** TODO: close the unused ends of two pipes */
      /* Close the write-end of the parent-to-child pipe */
      // Set to READ
      if (close(parentToChildPipe[WRITE_END]) < 0) {
        perror("close");
        exit(-1);
      }

      /* Close the read-end of the child-to-parent pipe */
      // Set to WRITE
      if (close(childToParentPipe[READ_END]) < 0) {
        perror("close");
        exit(-1);
      }

      /* Compute the hash */
      computeHash(hashProgs[hashAlgNum]);
    }
    /* I am the parent */

    /** TODO: close the unused ends of two pipes. */
    /* Close the read-end of the child-to-parent pipe */
    // Seto to WRITE
    if (close(parentToChildPipe[READ_END]) < 0) {
      perror("close");
      exit(-1);
    }

    /* Close the write-end of the child-to-parent pipe */
    // Set to READ
    if (close(childToParentPipe[WRITE_END]) < 0) {
      perror("close");
      exit(-1);
    }

    /* The buffer to hold the string received from the child */
    char hashValue[HASH_VALUE_LENGTH];

    /* Reset the hash buffer */
    memset(hashValue, (char)NULL, HASH_VALUE_LENGTH);

    /** TODO: Send the string to the child */
    /* Send the string to the child */
    // WRITING
    if (write(parentToChildPipe[WRITE_END], fileName.c_str(),
              MAX_FILE_NAME_LENGTH) < 0) {
      perror("write");
      exit(-1);
    }

    /** TODO: Read the string sent by the child */
    /* Read the string sent by the child */
    // READING
    if (read(childToParentPipe[READ_END], hashValue, MAX_FILE_NAME_LENGTH) <
        0) {
      perror("read");
      exit(-1);
    }

    /* Print the hash value */
    fprintf(stdout, "%s HASH VALUE: %s\n", hashProgs[hashAlgNum].c_str(),
            hashValue);
    fflush(stdout);

    /* Wait for the program to terminate */
    if (wait(NULL) < 0) {
      perror("wait");
      exit(-1);
    }
  }

  cout << "Program Ending...\n";
  return 0;
}
