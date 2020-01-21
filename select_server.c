#include "networking.h"

void process(char *s);
int cmpfunc(const void * a, const void * b);
void subserver(int from_client, int writepipefd, int readpipefd);

int main() {

  int listen_socket;
  int client_socket;
  int f, i;
  int subserver_count = 0;
  char buffer[BUFFER_SIZE];

  //set of file descriptors to read from
  fd_set read_fds;

  listen_socket = server_setup();

  //pipes to read from
  int rss0[2];
  int rss1[2];
  int rss2[2];
  int rss3[2];

  //pipes to write to
  int wss0[2];
  int wss1[2];
  int wss2[2];
  int wss3[2];

  //array of pipes to read from
  int *server_read_pipes[4] = {
    rss0,
    rss1,
    rss2,
    rss3
  };

  //array of pipes to read from
  int *server_write_pipes[4] = {
    wss0,
    wss1,
    wss2,
    wss3
  };

  //piping the pipes
  pipe(rss0);
  pipe(rss1);
  pipe(rss2);
  pipe(rss3);

  pipe(wss0);
  pipe(wss1);
  pipe(wss2);
  pipe(wss3);

  //buffers to store information received from each subserver
  char rb0[BUFFER_SIZE];
  char rb1[BUFFER_SIZE];
  char rb2[BUFFER_SIZE];
  char rb3[BUFFER_SIZE];

  //buffers to store information to write to each subserver
  char wb0[BUFFER_SIZE];
  char wb1[BUFFER_SIZE];
  char wb2[BUFFER_SIZE];
  char wb3[BUFFER_SIZE];

  //array of buffers for reading from subservers
  char *readbuffers[4] = {
    rb0,
    rb1,
    rb2,
    rb3
  };

  //array of buffers for writing to subservers
  char *writebuffers[4] = {
    wb0,
    wb1,
    wb2,
    wb3
  };

  char positions[4][BUFFER_SIZE] = {
    "1st",
    "2nd",
    "3rd",
    "4th"
  };
  while (1) {

    //select() modifies read_fds
    //we must reset it at each iteration
    // printf("reseting fds\n");
    // FD_ZERO(&read_fds); //0 out fd set
    // FD_SET(STDIN_FILENO, &read_fds); //add stdin to fd set
    // FD_SET(listen_socket, &read_fds); //add socket to fd set
    // for (i = 0; i < subserver_count; i++){
    //   FD_SET(pipes[i][0], &read_fds); //add the read end of the pipe to fd set
    //   printf("set pipe[%d][0]\n", i);
    // }
    // printf("select is happening\n");
    // //select will block until either fd is ready
    // select(listen_socket + 1, &read_fds, NULL, NULL, NULL);
    // printf("after select...\n");
    //if listen_socket triggered select
    // if (FD_ISSET(listen_socket, &read_fds)) {
     client_socket = server_connect(listen_socket);

     f = fork();
     if (f == 0){ //subserver
       //create the connection for pipe allowing the same info going to server
       close(server_read_pipes[subserver_count][0]); //close the read end of pipe for server to read from
       close(server_write_pipes[subserver_count][1]); //close the write end of pipe for server to write to
       printf("subserver[%d] has been initialized \n", subserver_count);
       subserver(client_socket, server_read_pipes[subserver_count][1], server_write_pipes[subserver_count][0]);
       printf("subserver function complete\n");
     }
     else { //main server
       close(server_read_pipes[subserver_count][1]); //close the write end of pipe for server to read from
       close(server_write_pipes[subserver_count][0]); //close read end of pipe for server to write to
       printf("subserver count before ++: %d\n", subserver_count);
       printf("closed the write end in parent\n");
       // FD_SET(pipes[subserver_count][0], &read_fds); //add the read end of the pipe to fd set
       // printf("added the read end of the pipe in parent\n");
       subserver_count++;
       close(client_socket);
     }
    //end listen_socket select
    // printf("end listen socket select\n");
    printf("past the fork, looping through the pipes\n");
    //if any of the pipes triggered select
    for (i = 0; i < subserver_count; i++){
      // if (FD_ISSET(pipes[i][0], &read_fds)) {
        printf("trying to read from server_read_pipes[%d][0]\n", i); //gets stuck here
        read(server_read_pipes[i][0], readbuffers[i], sizeof(readbuffers[i]));
        //read the data into the corresponding buffer
        printf("data received from subserver #%d: %s\n", i, readbuffers[i]);
      // }
    }//end read-end pipes select
    // printf("end read-end pipes select\n");

    int cmpfunc (const void * a, const void * b) {
      return ( *(int*)a - *(int*)b );
    }

    printf("Before sorting the list is: \n");
    for( i = 0 ; i < subserver_count; i++ ) {
      printf("%s\n", readbuffers[i]);
    }

    qsort(readbuffers, subserver_count, BUFFER_SIZE, cmpfunc);

    printf("\nAfter sorting the list is: \n");
    for( i = 0 ; i < subserver_count; i++ ) {
      printf("readbuffers[%d]: %s\n", i, readbuffers[i]);
      strcpy(buffer, strcat(readbuffers[i], positions[i]);
      strcpy(writebuffers[i], buffer);
      printf("writebuffers[%d]: %s\n", i, writebuffers[i]);
    }

    //loop through again to broadcast information
    for (i = 0; i < subserver_count; i++){
      printf("trying to write to server_write_pipes[%d][1]\n", i); //gets stuck here
      write(server_read_pipes[i][1], writebuffers[i], sizeof(writebuffers[i]));
      //read the data into the corresponding buffer
      printf("data sent to subserver #%d: %s\n", i, writebuffers[i]);
    }

    //if stdin triggered select
    // if (FD_ISSET(STDIN_FILENO, &read_fds)) {
    //   //if you don't read from stdin, it will continue to trigger select()
    //   fgets(buffer, sizeof(buffer), stdin);
    //   printf("[server] subserver count: %d\n", subserver_count);
    // }//end stdin select
    // printf("end stdin select\n");
  }
}

void subserver(int client_socket, int writepipefd, int readpipefd) {
  char buffer[BUFFER_SIZE];

  //for testing client select statement
  // strncpy(buffer, "hello client", sizeof(buffer));
  // write(client_socket, buffer, sizeof(buffer));

  while (read(client_socket, buffer, sizeof(buffer))) {

    printf("[subserver %d] received: [%s]\n", getpid(), buffer);
    process(buffer);
    write(client_socket, buffer, sizeof(buffer));
    printf("wrote to client socket\n");
    write(writepipefd, buffer, sizeof(buffer));
    printf("wrote '%s' to write end of pipe\n", buffer);
    read(readpipefd, buffer, sizeof(buffer));
    printf("Information received: %s\n", buffer);
  }//end read loop
  close(client_socket);
  exit(0);
}

void process(char * s) {
  while (*s) {
    if (*s >= 'a' && *s <= 'z')
      *s = ((*s - 'a') + 13) % 26 + 'a';
    else  if (*s >= 'A' && *s <= 'Z')
      *s = ((*s - 'a') + 13) % 26 + 'a';
    s++;
  }
  printf("process complete\n");
}
