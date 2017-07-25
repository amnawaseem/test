#define _GNU_SOURCE
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/stat.h>
#include <ctype.h>
#include <sys/queue.h>
#include <stdint.h>
#include "multi.h"


/* Configures the serial device specified by "path". Currently a default baud
 * rate of 115200 is used. If opened successfully, the file descriptor of the
 * serial device is returned, else "-1" is returned
*/
int configure_serial(char* path) {
  struct termios term;

  int fd = open(path,O_RDWR | O_NONBLOCK | O_SYNC);
  if (fd < 0) {
    fprintf(stderr,"Error opening serial device %s\n",path);
    return -1;
  }
  tcgetattr(fd, &term);
  cfsetospeed(&term, B115200);
  cfsetispeed(&term, B115200);
  cfmakeraw(&term);
  term.c_cflag &= ~(CSTOPB);
  term.c_cflag |= CS8;
  term.c_cflag &= ~ECHO;
  tcsetattr(fd, TCSANOW, &term);

  return fd;
}

/* Changes the active channel of the serial line. */
void update_client(client_con_t* client,con_t* con) {
  char cc = 0xff;
  write(con->serial_fd,&cc,1);
  write(con->serial_fd,&client->client_id,1);
  con->active_client_out = client;
}

/* Create a listening TCP on port "port". If successfully, the file descriptor
 * of that socket is returned, else "-1" is returned.
 */
int create_server_socket(int port) {
  int server_socket;
  struct sockaddr_in addr;
  int setopt = 1;

  if ((server_socket = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0)) == -1) {
    fprintf(stderr, "socket() failed: %s\n", strerror(errno));
    return -1;
  }

  setsockopt(server_socket,SOL_SOCKET,SO_REUSEADDR, (char*)&setopt
      ,sizeof(setopt));

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = port;

  if (bind(server_socket, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
    fprintf(stderr, "bind() failed: %s\n", strerror(errno));
    return -1;
  }

  if (listen(server_socket, 1) == -1) {
    fprintf(stderr, "listen() failed: %s\n", strerror(errno));
    return -1;
  }

  return server_socket;
}

/* Accepts a new client on the server socket. Should only be called when the
 * server socket is ready, i.e. a new client is pending.
 * Returns the file descriptor of the new client if successful, else "-1" is
 * returned.
 */
int accept_client(int server_socket) {
  int client_socket;
  struct sockaddr addr;
  socklen_t len;

  memset(&addr, 0, sizeof(addr));
  len = sizeof(addr);
  if ((client_socket = accept4(server_socket, &addr,&len,SOCK_CLOEXEC |
          SOCK_NONBLOCK)) == -1) {
    fprintf(stderr, "accept() failed: %s\n", strerror(errno));
    return -1;
  }

  return client_socket;
}

/* Updates the fdset "rfds" by adding every valid file descriptor from the
 * current connection specified by "con". Returns the new highest fd number.
 */
int update_fdset(fd_set* rfds,con_t* con) {
  int max_fd = 0;
  client_con_t* client;
  FD_ZERO(rfds);
  SLIST_FOREACH(client,&clientsh,clients) {
    if(client->client_socket != -1) {
      FD_SET(client->client_socket,rfds); 
      max_fd = client->client_socket > max_fd ? client->client_socket : max_fd;
    }
  }
  if(con->server_socket != -1) {
    FD_SET(con->server_socket,rfds);
    max_fd = con->server_socket > max_fd ? con->server_socket: max_fd;
  } 

  if(con->serial_fd != -1) {
    FD_SET(con->serial_fd,rfds);
    max_fd = con->serial_fd > max_fd ? con->serial_fd: max_fd;
  }

  return max_fd;
}

/* Initializes the list of clients */
void initialize_clients() {
  client_con_t* first_client;
  SLIST_INIT(&clientsh);

  first_client = malloc(sizeof(client_con_t));
  first_client->client_id = 0; // Upstream connection to phidias on core 0
  first_client->client_socket = -1;
  SLIST_INSERT_HEAD(&clientsh,first_client,clients);
}

/* This function does one of the following:
 * If no client id is provided (id == -1):
 * 
 * Accept a new client connection and update an already existing client.
 * OR
 * Create a new client object in case no unused client existed.
 * 
 * 
 * If the id is provided (id >= 0):
 * 
 * Find an existing client that matches the client id.
 * OR
 * Use an existing client.
 * OR
 * Create a new client.
 */
client_con_t* get_client(int id,con_t* con) {
  client_con_t* client;
  client_con_t* unused_client = NULL;

  if(id == -1) {
    // Accept a new client
    SLIST_FOREACH(client,&clientsh,clients) {
      if(client->client_socket == -1) {
        // already allocated client without connection
        client->client_socket = accept_client(con->server_socket);
        return client;
      }
    }

    // no allocated client -> create a new one
    client = (client_con_t*)malloc(sizeof(client_con_t));
    client->client_socket = accept_client(con->server_socket);
    client->client_id = -1;
    SLIST_INSERT_HEAD(&clientsh,client,clients);
    return client;


  } else {
    // Try to find an existing client
    SLIST_FOREACH(client,&clientsh,clients) {
      if(id < 8 && client->client_id == 0) {
        //        printf("Existing client with core merging: 0x0\n");
        return client;
      }
      if(id == client->client_id) {
        //        printf("Existing guest client: 0x%x\n",client->client_id);
        return client; // Found it
      }
      if(client->client_id == -1)
        unused_client = client;

    }

  }
  // No matching client found, so either create a new one, or use an unused one
  if (unused_client == NULL) {
    unused_client = (client_con_t*)malloc(sizeof(client_con_t));
    unused_client->client_socket = -1;
    SLIST_INSERT_HEAD(&clientsh,unused_client,clients);
  } 
  unused_client->client_id = id;

  return unused_client;
}

/* Spawn the client script provided by the "-e" parameter */
void spawn_client_script(char* script,short port,int clients) {
  int child_pid;
  char port_s[6] = {0};
  char client_s[2] = {0};
  char* newargv[] = {script, port_s, client_s,NULL};
  snprintf(port_s,6,"%d",port);
  snprintf(client_s,2,"%d",clients);

  child_pid = fork();
  if(child_pid == 0) {
    execv(script,newargv);
    printf("Error, failed to execute script: %s\n",script);
    exit(-1);

  } 
}

/* Prepare buffer to be send over serial line */
int prepare_out_buffer(uint8_t* in_buffer,uint8_t* out_buffer,int read_bytes) {
  // We need to check for occurrences of the special character "0xff" and add an
  // additional 0xff for each found char.
  uint8_t i,a;
  a = 0;
  for(i = 0;i<read_bytes;i++) {
    if(in_buffer[i] == 0xff) {
      out_buffer[a] = 0xff;
      a++;
    }
    out_buffer[a] = in_buffer[i];
    a++;

  };

  return a;
}

/* Prepare buffer to be send to clients */
void process_in_buffer(uint8_t* in_buffer,int read_bytes,con_t* con,
    int* client_change_imminent) {

  size_t tmp = 0;
  uint8_t out_buffer[BUFFER_SIZE];
  int a = 0;
  for(int i = 0;i<read_bytes;i++) {
    if(*client_change_imminent && (in_buffer[i] == 0xff)) {
      *client_change_imminent = 0;
      out_buffer[a] = 0xff;
      a++;
    } else if(in_buffer[i] == 0xff) {
      /* Possible client change, ignore the first 0xff */
      *client_change_imminent = 1;
    } else if (*client_change_imminent) {
      /*Flush bytes read so far to current client */
      while((tmp += write(con->active_client_in->client_socket,out_buffer,a)) <a);
      /* Change to new client */
      con->active_client_in = get_client(in_buffer[i],con);
      /* reset "a" */
      a = 0;
      *client_change_imminent = 0;
    } else {
      out_buffer[a] = in_buffer[i];
      a++;
    }
  }
  /* Flush remaining bytes to active client */
  tmp = 0;
  if (a) {
    while((tmp += write(con->active_client_in->client_socket,out_buffer,a)) <a); 
  }

}

int main(int argc,char* argv[]) {
  fd_set rfds;
  int max_fd = 0;
  int retval;
  int opt;
  char* portname = NULL;
  uint8_t in_buffer[BUFFER_SIZE];
  uint8_t out_buffer[BUFFER_SIZE];
  int read_bytes;
  short port = 1234;
  int client_change_imminent = 0;
  int count_client = 0;
  char* mode = NULL;
  con_t connection;
  client_con_t* client;

  //Parse options
  while ((opt = getopt(argc,argv,"p:s:e:c:h")) != -1) {
    switch(opt) {
      case 'p':
        port = atoi(optarg);
        break;
      case 's':
        portname = optarg;
        break;
      case 'e':
        mode = optarg;
        break;
      case 'c':
        count_client = atoi(optarg);
        break;
      case 'h':
        printf("%s",help);
        exit(0);
      case '?':
        printf("%s",usage);
        exit(-1);
    }

  }

  if(count_client && !mode) {
    printf("-c is only meaningful when used with -e\n");
    printf("%s",usage);
    exit(-1);
  }

  if(!portname) {
    printf("%s",usage);
    exit(-1);
  }

  initialize_clients();



  connection.active_client_out = SLIST_FIRST(&clientsh);
  connection.active_client_in = SLIST_FIRST(&clientsh);
  connection.server_socket = create_server_socket(htons(port));

  connection.serial_fd = configure_serial(portname);

  if(count_client && mode)
    spawn_client_script(mode,port,count_client);


  max_fd = update_fdset(&rfds,&connection);

  do {
    if((retval = select(max_fd + 1,&rfds,NULL,NULL,NULL)) == -1) {
      perror("select()");
      exit(-1);
    } else if (retval > 0) {
      /* If select returns with retval > 0, either
       * a) A listening server socket is ready for accept()
       * b) An already accepted client socket is ready for read()
       * c) A remote client closed the connection
       */
      /* New client */
      if (FD_ISSET(connection.server_socket,&rfds)) {
        client = get_client(-1,&connection);
      }
      SLIST_FOREACH(client,&clientsh,clients) {
        if(FD_ISSET(client->client_socket,&rfds)) {
          if((read_bytes = read(client->client_socket,&in_buffer,BUFFER_HALF)) > 0) {
            /* Receive at most BUFFER_SIZE/2 bytes from client */
            if(client != connection.active_client_out)
              update_client(client,&connection);
            int a = prepare_out_buffer(in_buffer,out_buffer,read_bytes);
            write(connection.serial_fd,&out_buffer,a);
          } else {
            printf("ERROR, remote end closed the connection\n");
            close(client->client_socket);
            client->client_socket = -1;
          }
        }
      }
      /* Serial line has new data?*/
      int tmp = 0;
      if(FD_ISSET(connection.serial_fd,&rfds)) {
        if((tmp = read(connection.serial_fd,in_buffer,BUFFER_SIZE)) > 0 ) {
          process_in_buffer(in_buffer,tmp,&connection,&client_change_imminent);
        } else {
          close(connection.serial_fd);
          connection.serial_fd = -1;
        }
      }
    } 
    /* If the serial is gone, try to re-open it */
    if(connection.serial_fd == -1) {
      printf("Re-opening the serial device\n");
      connection.serial_fd = configure_serial(portname);
    }
    FD_ZERO(&rfds);
    max_fd = update_fdset(&rfds,&connection);
  } while(1);

}

// vim:ts=2:sw=2:expandtab:textwidth=79
