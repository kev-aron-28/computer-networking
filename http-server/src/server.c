#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "server.h"
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>

void init_http_header(http_header *header)
{
  memset(header->method, 0, sizeof(header->method));

  memset(header->path, 0, sizeof(header->path));

  memset(header->version, 0, sizeof(header->version));

  memset(header->host, 0, sizeof(header->host));
}

void parse_request(const char *request, http_header *header)
{
  char *requestCopy = strdup(request);
  if (!requestCopy)
  {
    fprintf(stderr, "Memory allocation failed\n");
    return;
  }

  char *requestLine = strtok(requestCopy, "\r\n");
  if (!requestLine)
  {
    fprintf(stderr, "Invalid request: missing request line\n");
    free(requestCopy);
    return;
  }  
  sscanf(requestLine, "%s %s %s", header->method, header->path, header->version);

  char *queryStart = strchr(header->path, '?');
  if (queryStart)
  {
    *queryStart = '\0';
    strcpy(header->query, queryStart + 1);
  }
  else
  {
    header->query[0] = '\0';
  }

  char *line;
  int contentLength = 0;
  int headersEnd = 0;

  char *lastLine = NULL;

  while ((line = strtok(NULL, "\r\n")) != NULL)
  {

    lastLine = line;

    if (strncmp(line, "Host:", 5) == 0)
    {
      sscanf(line, "Host: %255s", header->host);
    }
    else if (strncmp(line, "Content-Length:", 15) == 0)
    {
      sscanf(line, "Content-Length: %d", &contentLength);
    }
  }

  if (contentLength > 0)
  {
    strncpy(header->body, lastLine, contentLength);

    header->body[contentLength] = '\0';
  }
  else
  {
    header->body[0] = '\0';
  }

  free(requestCopy);
}

const char *get_mime_type(const char *path)
{
  if (strstr(path, ".html"))
    return "text/html";
  if (strstr(path, ".css"))
    return "text/css";
  if (strstr(path, ".js"))
    return "application/javascript";
  if (strstr(path, ".jpg"))
    return "image/jpeg";
  if (strstr(path, ".png"))
    return "image/png";
  return "application/octet-stream";
}

void pool_init(pool_thread *pool, int max_len)
{
  pool->tasks = malloc(max_len * sizeof(thread_task));
  pool->current_len = 0;
  pool->max_len = max_len;
  pool->head = 0;
  pool->tail = 0;
  pthread_mutex_init(&pool->mutex, NULL);
  pthread_cond_init(&pool->condition_variable, NULL);
}

void pool_destroy(pool_thread *pool)
{
  free(pool->tasks);
  pthread_mutex_destroy(&pool->mutex);
  pthread_cond_destroy(&pool->condition_variable);
}

void *pool_worker(void *arg)
{
  pool_thread *pool = (pool_thread *)arg;

  while (1)
  {
    pthread_mutex_lock(&pool->mutex);

    while (pool->current_len == 0)
    {
      pthread_cond_wait(&pool->condition_variable, &pool->mutex);
    }

    thread_task task = pool->tasks[pool->head];

    pool->head = (pool->head + 1) % pool->max_len;

    pool->current_len--;

    char buffer[8192] = {0};

    pthread_mutex_unlock(&pool->mutex);

    ssize_t bytesRead = read(task.descriptor, buffer, sizeof(buffer));

    if (bytesRead == 0)
    {
      perror("FAILED TO READ FROM SOCKET \n");

      pthread_mutex_unlock(&pool->mutex);

      close(task.descriptor);

      return NULL;
    }

    buffer[8191] = '\n';

    http_header header;

    init_http_header(&header);

    printf("BUFFER: %s/n", buffer);

    parse_request(buffer, &header);

    char file_path[1024] = "public";

    if (strcmp(header.path, "/") == 0)
    {
      strcat(file_path, "/index.html");
    }
    else
    {
      strcat(file_path, header.path);
    }

    int file_descriptor = open(file_path, O_RDONLY);

    if (file_descriptor < 0)
    {
      const char *not_found =
          "HTTP/1.1 404 Not Found\r\n"
          "Content-Type: text/html\r\n"
          "Content-Length: 22\r\n\r\n"
          "<h1>404 Not Found</h1>";

      write(task.descriptor, not_found, strlen(not_found));
    }
    else
    {
      if (strcmp(header.method, "POST") == 0)
      {
        printf("Received POST request with body: %s\n", header.body);
      }

      if(strcmp(header.method, "DELETE") == 0) {
        printf("Received DELETE request");
      }

      if(strcmp(header.method, "PUT") == 0) {
        printf("Received PUT request with body: %s\n", header.body);
      }

      if(strcmp(header.method, "HEAD") == 0) {
        printf("Received HEAD request \n");
      }

      if (strlen(header.query) > 0)
      {
        printf("Query parameters: %s\n", header.query);
      }

      struct stat file_stat;

      fstat(file_descriptor, &file_stat);

      size_t file_size = file_stat.st_size;

      char header[256];

      snprintf(header, sizeof(header),
               "HTTP/1.1 200 OK\r\n"
               "Content-Length: %zu\r\n"
               "Content-Type: %s\r\n\r\n",
               file_size, get_mime_type(file_path));

      write(task.descriptor, header, strlen(header));

      char file_buffer[1024];

      ssize_t bytes;

      while ((bytes = read(file_descriptor, file_buffer, sizeof(file_buffer))) > 0)
      {
        write(task.descriptor, file_buffer, bytes);
      }

      close(file_descriptor);
    }

    close(task.descriptor);
  }

  return NULL;
}

int pool_add_task(pool_thread *pool, thread_task *task)
{
  pthread_mutex_lock(&pool->mutex);

  if (pool->current_len == pool->max_len)
  {
    pthread_mutex_unlock(&pool->mutex);

    return -1;
  }

  pool->tasks[pool->tail] = *task;

  pool->tail = (pool->tail + 1) % pool->max_len;

  pool->current_len++;

  pthread_cond_signal(&pool->condition_variable);

  pthread_mutex_unlock(&pool->mutex);

  return 0;
}

server_instance init_instance(int port)
{
  server_instance instance = {.port = port};

  return instance;
}

int start_server(server_instance *instance)
{
  int socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);

  if (socket_descriptor < 0)
  {
    perror("FAILED TO CREATE THE SOCKET \n");

    return -1;
  }

  struct sockaddr_in server_address;

  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(instance->port);

  if (bind(socket_descriptor, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
  {
    perror("FAILED TO BIND THE SOCKET \n");

    close(socket_descriptor);
    return -1;
  }

  if (listen(socket_descriptor, 5) < 0)
  {
    perror("FAILED TO LISTEN ON ADDRESS \n");

    close(socket_descriptor);
  }

  printf("SERVER IS LISTENING ON PORT %d\n", instance->port);

  pool_thread pool;

  pool_init(&pool, 5);

  pthread_t workers[5];

  for (size_t i = 0; i < 5; i++)
  {
    pthread_create(&workers[i], NULL, pool_worker, &pool);
  }

  while (1)
  {
    struct sockaddr_in connection_address;

    socklen_t connection_address_len = sizeof(connection_address);

    int connection_descriptor = accept(socket_descriptor, (struct sockaddr *)&connection_address, &connection_address_len);

    if (connection_descriptor < 0)
    {
      perror("FAILED TO ACCEPT CONNECTION \n");

      continue;
    }

    thread_task task = { .task_status = active, .descriptor = connection_descriptor};

    if (pool_add_task(&pool, &task) != 0)
    {
      printf("TASK QUEUE FULL");

      close(connection_descriptor);
    }
  }

  pool_destroy(&pool);

  close(socket_descriptor);
}

int main(int argc, char const *argv[])
{

  server_instance http = init_instance(3000);

  start_server(&http);

  return 0;
}
