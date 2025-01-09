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
#define MAX_BODY_SIZE 30000

void init_http_header(http_header *header)
{
  memset(header->method, 0, sizeof(header->method));

  memset(header->path, 0, sizeof(header->path));

  memset(header->version, 0, sizeof(header->version));

  memset(header->host, 0, sizeof(header->host));

  memset(header->content_type, 0, sizeof(header->content_type));
}

void parse_request(const char *request, http_header *header)
{
  printf("%s\n", request);
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
    strncpy(header->query, queryStart + 1, sizeof(header->query) - 1);
  }
  else
  {
    header->query[0] = '\0';
  }

  char *line;
  size_t contentLength = 0;

  while ((line = strtok(NULL, "\r\n")) != NULL)
  {
    if (strncmp(line, "Host:", 5) == 0)
    {
      sscanf(line, "Host: %255s", header->host);
    }
    else if (strncmp(line, "Content-Length:", 15) == 0)
    {
      sscanf(line, "Content-Length: %ld", &contentLength);
    }
    else if (strncmp(line, "Content-Type:", 13) == 0)
    {
      sscanf(line, "Content-Type: %255s", header->content_type);
    }
    else if (strlen(line) == 0) // End of headers
    {
      break;
    }
  }

  size_t request_len = strlen(request);

  header->body = (char *)malloc(sizeof(char) * MAX_BODY_SIZE);

  printf("request_len: %zu, contentLength: %zu\n", request_len, contentLength);
  if (contentLength > 0 && contentLength <= request_len)
  {
    const char *bodyStart = request + (request_len - contentLength);
    printf("%s \n", bodyStart);

    if (contentLength < MAX_BODY_SIZE) // Define un tamaño máximo para evitar overflows.
    {
      strncpy(header->body, bodyStart, contentLength);
      header->body[contentLength] = '\0';
    }
    else
    {
      printf("Body too large, truncating.\n");
      strncpy(header->body, bodyStart, MAX_BODY_SIZE - 1);
      header->body[MAX_BODY_SIZE - 1] = '\0';
    }
  }
  else
  {
    header->body[0] = '\0';
  }

  printf("body %s \n", header->body);

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

    buffer[10000] = '\n';

    http_header header;

    init_http_header(&header);

    parse_request(buffer, &header);

    if (strcmp(header.method, "GET") == 0)
    {
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
    else if (strcmp(header.method, "POST") == 0)
    {
      printf("Received POST request with body: %s\n", header.body);

      size_t body_length = strlen(header.body);

      char response_header[256];
      snprintf(response_header, sizeof(response_header),
               "HTTP/1.1 200 OK\r\n"
               "Content-Length: %zu\r\n"
               "Content-Type: text/plain\r\n\r\n",
               body_length);

      write(task.descriptor, response_header, strlen(response_header));

      write(task.descriptor, header.body, body_length);
    }
    else if (strcmp(header.method, "DELETE") == 0)
    {
      printf("Received DELETE request for: %s\n", header.path);

      char file_path[1024] = "public";
      strcat(file_path, header.path);

      if (access(file_path, F_OK) == 0)
      {
        if (remove(file_path) == 0)
        {
          const char *message =
              "HTTP/1.1 200 OK\r\n"
              "Content-Type: text/plain\r\n"
              "Content-Length: 23\r\n\r\n"
              "Del";

          write(task.descriptor, message, strlen(message));
          printf("Deleted resource: %s\n", file_path);
        }
        else
        {
          const char *error_response =
              "HTTP/1.1 500 Internal Server Error\r\n"
              "Content-Type: text/plain\r\n"
              "Content-Length: 20\r\n\r\n"
              "Failed to delete file";

          write(task.descriptor, error_response, strlen(error_response));
          perror("Error deleting file");
        }
      }
      else
      {
        const char *not_found_response =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 24\r\n\r\n"
            "Resource not found on server";

        write(task.descriptor, not_found_response, strlen(not_found_response));
        printf("Resource not found: %s\n", file_path);
      }
    }
    else if (strcmp(header.method, "PUT") == 0)
    {
      printf("Received PUT request for: %s\n", header.path);

      char file_path[1024] = "public";
      strcat(file_path, header.path);

      if (strlen(header.path) == 0 || header.path[0] != '/')
      {
        const char *bad_request_response =
            "HTTP/1.1 400 Bad Request\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 18\r\n\r\n"
            "Invalid file path";

        write(task.descriptor, bad_request_response, strlen(bad_request_response));
        printf("Invalid file path for PUT request\n");
        return NULL;
      }

      int file_descriptor = open(file_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (file_descriptor < 0)
      {
        const char *error_response =
            "HTTP/1.1 500 Internal Server Error\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 21\r\n\r\n"
            "Failed to create file";

        write(task.descriptor, error_response, strlen(error_response));
        perror("Error creating file");
        return NULL;
      }

      ssize_t bytes_written = write(file_descriptor, header.body, strlen(header.body));
      if (bytes_written < 0)
      {
        const char *error_response =
            "HTTP/1.1 500 Internal Server Error\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 23\r\n\r\n"
            "Failed to write to file";

        write(task.descriptor, error_response, strlen(error_response));
        perror("Error writing to file");
        close(file_descriptor);
        return NULL;
      }

      close(file_descriptor);

      const char *success_response =
          "HTTP/1.1 201 Created\r\n"
          "Content-Type: text/plain\r\n"
          "Content-Length: 16\r\n\r\n"
          "File created OK";

      write(task.descriptor, success_response, strlen(success_response));
      printf("File created: %s\n", file_path);
    }
    else if (strcmp(header.method, "HEAD") == 0)
    {
      printf("Received HEAD request\n");

      char file_path[1024] = "public";

      // Determinar el archivo solicitado
      if (strcmp(header.path, "/") == 0)
      {
        strcat(file_path, "/index.html");
      }
      else
      {
        strcat(file_path, header.path);
      }

      // Intentar abrir el archivo
      int file_descriptor = open(file_path, O_RDONLY);
      if (file_descriptor < 0)
      {
        // Si no se encuentra, enviar 404
        const char *not_found =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: 0\r\n\r\n"; // Sin cuerpo
        write(task.descriptor, not_found, strlen(not_found));
      }
      else
      {
        // Si se encuentra, generar los encabezados
        struct stat file_stat;
        fstat(file_descriptor, &file_stat);

        size_t file_size = file_stat.st_size;

        char response_header[256];
        snprintf(response_header, sizeof(response_header),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Length: %zu\r\n"
                 "Content-Type: %s\r\n\r\n", // Solo los encabezados
                 file_size, get_mime_type(file_path));

        // Enviar los encabezados
        write(task.descriptor, response_header, strlen(response_header));

        // Cerrar el archivo después de usarlo
        close(file_descriptor);
      }
    }
    else
    {
      const char *error_response =
          "HTTP/1.1 500 Internal Server Error\r\n"
          "Content-Type: text/plain\r\n"
          "Content-Length: 23\r\n\r\n"
          "Failed to write to file";

      write(task.descriptor, error_response, strlen(error_response));
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

    thread_task task = {.task_status = active, .descriptor = connection_descriptor};

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
