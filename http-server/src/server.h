#ifndef _SERVER_H_
#define _SERVER_H_

#include <pthread.h>

enum thread_task_status
{
  done = 0,
  active = 1
};

typedef struct
{
  int descriptor;
  enum thread_task_status task_status;
} thread_task;

typedef struct
{
  pthread_mutex_t mutex;
  pthread_cond_t condition_variable;
  int current_len;
  int max_len;
  thread_task *tasks;
  int head;
  int tail;
} pool_thread;

// POOL STUFF
void pool_init(pool_thread *pool, int max_len);
void pool_destroy(pool_thread *pool);
void *pool_worker(void *arg);
int pool_add_task(pool_thread *pool, thread_task *task);

typedef struct
{
  char method[16];
  char path[1024];
  char version[16];
  char host[256];
  char query[1024];
  char *body;
  char content_type[256];
} http_header;

typedef struct
{
  int port;
} server_instance;

// SERVER STUFF
server_instance init_instance(int port);
int start_server(server_instance *instance);
void init_http_header(http_header *header);
void parse_request(const char *request, http_header *header);
const char *get_mime_type(const char *path);
void parse_multipart_body(const char *body, const char *boundary);

#endif
