#pragma once

#define BUFFER_SIZE 1024
#define BUFFER_HALF 512

const char* client_cmd = " stty raw -echo && nc localhost ";

const char* usage = "multi -s \"serial device\" [-p PORT ] [ -c CLIENTS ]" \
                     " [ -e SCRIPT ] [-h]\n";
const char* help = "-s PATH_TO_SERIAL -- e.g. /dev/ttyUSB0\n" \
                   "[ -p LISTEN_PORT ] -- default 1234\n" \
                   "[ -c NR_OF_CLIENTS ] -- only when -e is also provided, will" \
                   " be passed to the script as an argument\n" \
                   "[ -e PATH_TO_SCRIPT ]-- will be executed with PORT and " \
                   "NR_OF_CLIENTS as parameters, see the ./scripts/ folder for" \
                   " examples\n";

typedef struct client_con client_con_t;
struct client_con {
  int client_socket;
  int client_id;
  SLIST_ENTRY(client_con) clients;
};

typedef struct connection con_t;
struct connection {
  int serial_fd;
  int server_socket;
  client_con_t* active_client_in;
  client_con_t* active_client_out;
};

SLIST_HEAD(slisthead, client_con) clientsh;

