#include "networking.h"

void process(char *s);
void subserver(int from_client, int writepipefd);

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
  int ss0[2];
  int ss1[2];
  int ss2[2];
  int ss3[2];

  //array of pipes
  int *pipes[4] = {
    ss0,
    ss1,
    ss2,
    ss3
  };

  //piping the pipes
  pipe(ss0);
  pipe(ss1);
  pipe(ss2);
  pipe(ss3);

  //buffers to store information received from each subserver
  char buffer0[BUFFER_SIZE];
  char buffer1[BUFFER_SIZE];
  char buffer2[BUFFER_SIZE];
  char buffer3[BUFFER_SIZE];

  //array of buffers
  char *readbuffers[4] = {
    buffer0,
    buffer1,
    buffer2,
    buffer3
  };

  while (1) {

    //select() modifies read_fds
    //we must reset it at each iteration
    FD_ZERO(&read_fds); //0 out fd set
    FD_SET(STDIN_FILENO, &read_fds); //add stdin to fd set
    FD_SET(listen_socket, &read_fds); //add socket to fd set
    for (i = 0; i < subserver_count; i++){
      FD_SET(pipes[i][0], &read_fds); //add the read end of the pipe to fd set
    }

    //select will block until either fd is ready
    select(listen_socket + 1, &read_fds, NULL, NULL, NULL);

    //if listen_socket triggered select
    if (FD_ISSET(listen_socket, &read_fds)) {
     client_socket = server_connect(listen_socket);

     f = fork();
     if (f == 0){ //subserver
       //create the connection for pipe allowing the same info going to server
       close(pipes[subserver_count][0]); //close the read end
       printf("subserver[%d] has been initialized \n", subserver_count);
       subserver(client_socket, pipes[subserver_count][1]);
     }
     else { //main server
       close(pipes[subserver_count][1]); //close the write end
       FD_SET(pipes[subserver_count][0], &read_fds); //add the read end of the pipe to fd set
       subserver_count++;
       close(client_socket);
     }
    }//end listen_socket select

    //if any of the pipes triggered select
    for (i = 0; i < subserver_count; i++){
      if (FD_ISSET(pipes[i][0], &read_fds)) {
        read(pipes[i][0], &readbuffers[i], sizeof(&readbuffers[i]));
        //read the data into the corresponding buffer
        printf("data received by subserver #%d: %s\n", i, readbuffers[i]);
      }
    }//end read-end pipes select

    //if stdin triggered select
    if (FD_ISSET(STDIN_FILENO, &read_fds)) {
      //if you don't read from stdin, it will continue to trigger select()
      fgets(buffer, sizeof(buffer), stdin);
      printf("[server] subserver count: %d\n", subserver_count);
    }//end stdin select

  }
}

void subserver(int client_socket, int writepipefd) {
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
    printf("wrote to write end of pipe\n");
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
}
