#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <protocol.h>
#include <pthread.h>

#define MAX_MESSAGE_LENGTH 300
#define MAX_MESSAGES 100
#define MULTICAST_GROUP "239.0.0.1"
#define MULTICAST_PORT 12345
#define PORT 8080
#define SERVER_IP "127.0.0.1"

typedef struct
{
    char message[MAX_MESSAGE_LENGTH];
    char username[100];
    int is_private;
    char username_sento[100];
    int is_file;
} Message;

typedef struct
{
    Message messages[MAX_MESSAGES];
    char activeUsers[1024];
    int message_count;
    pthread_mutex_t *mutex;
} SharedData;

typedef struct
{
    int sockFileDescriptor;
    int chatFileDescriptor;
    int filesFileDescriptor;
    SharedData *shared_data;
    int isFileSending;
} ThreadData;

WINDOW *msg_win;
WINDOW *input_win;
WINDOW *active_users_win;
struct sockaddr_in multicastAddress, serverAddress, multicastFileAddress;
struct sockaddr_in server_chat_address;

char *username;

pthread_mutex_t message_mutex = PTHREAD_MUTEX_INITIALIZER;

void draw_active_users_window(WINDOW *active_users_win, const char *users)
{
    werase(active_users_win);

    box(active_users_win, 0, 0);
    
    mvwprintw(active_users_win, 1, 1, "Active Users");

    mvwprintw(active_users_win, 2, 1, "%s", users);

    wrefresh(active_users_win);
}

void draw_input_window(WINDOW *input_win, const char *prompt)
{
    werase(input_win);

    box(input_win, 0, 0);
    
    mvwprintw(input_win, 1, 1, "%s", prompt);
    
    wrefresh(input_win);
}

void draw_message_window(WINDOW *msg_win, Message messages[MAX_MESSAGES], int message_count)
{
    int height, width;
    getmaxyx(msg_win, height, width);

    wclear(msg_win);
    box(msg_win, 0, 0);
    mvwprintw(msg_win, 1, 1, "Chat Messages");

    int visible_messages = height - 3;

    int start = message_count > visible_messages ? message_count - visible_messages : 0;

    for (int i = start; i < message_count; i++)
    {
        if (messages[i].is_private)
        {
            mvwprintw(msg_win, 2 + i - start, 1, "[Private] %s: %s", messages[i].username, messages[i].message);
        }
        else
        {
            mvwprintw(msg_win, 2 + i - start, 1, "%s: %s", messages[i].username, messages[i].message);
        }
    }

    wrefresh(msg_win);
}

void *receive_messages(void *arg)
{
    ThreadData *data = (ThreadData *)arg;

    int sockFileDescriptor = data->sockFileDescriptor;

    SharedData *shared_data = data->shared_data;

    Message received_message;

    socklen_t addressLength = sizeof(serverAddress);

    while (1)
    {
        int bytes = recvfrom(sockFileDescriptor, &received_message, sizeof(received_message), 0,
                             (struct sockaddr *)&serverAddress, &addressLength);

        if (bytes < 0)
        {
            perror("RECEIVING FAILED");
            continue;
        } 

        if(received_message.is_file) {
            continue;
        }

        if (!received_message.is_private || (received_message.is_private && strcmp(received_message.username_sento, username)) == 0)
        {
            if (shared_data->message_count < MAX_MESSAGES)
            {
                shared_data->messages[shared_data->message_count] = received_message;

                shared_data->message_count++;
            }
            else
            {
                for (int i = 1; i < MAX_MESSAGES; i++)
                {
                    shared_data->messages[i - 1] = shared_data->messages[i];
                }

                shared_data->messages[MAX_MESSAGES - 1] = received_message;
            }
        }
    }

    return NULL;
}

void *receive_active_users(void *arg)
{
    ThreadData *data = (ThreadData *)arg;

    SharedData *shared_data = data->shared_data;

    int sockFileDescriptor;

    if ((sockFileDescriptor = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    socklen_t addressLength = sizeof(server_chat_address);

    Packet packet = {
        .type = 3,
    };

    while (1)
    {   

        sleep(1);

        sendPacket(sockFileDescriptor, &server_chat_address, &packet);

        Packet userList;

        int bytes = recvfrom(sockFileDescriptor, &userList, sizeof(Packet), 0,
                             (struct sockaddr *)&server_chat_address, &addressLength);
        
        if (bytes < 0)
        {
            perror("RECEIVING FAILED");
            continue;
        }

        strncpy(shared_data->activeUsers, userList.data, sizeof(shared_data->activeUsers));
        
    }

    return NULL;
}

void *repaint_messages_screen(void *arg)
{
    SharedData *shared_data = (SharedData *)arg;

    while (1)
    {
        sleep(1);

        draw_message_window(msg_win, shared_data->messages, shared_data->message_count);
    }

    return NULL;
}

void *repaint_userlist_screen(void *arg)
{
    SharedData *shared_data = (SharedData *)arg;

    while (1)
    {
        sleep(2);

        draw_active_users_window(active_users_win, shared_data->activeUsers);
    }

    return NULL;
}

void build_user_interface()
{
    initscr();            // Iniciar el modo ncurses
    raw();                // Desactivar el buffering de línea
    keypad(stdscr, TRUE); // Habilitar teclas especiales
    noecho();             // Desactivar el eco automático de la entrada del usuario

    int row, col;
    getmaxyx(stdscr, row, col); // Obtener el tamaño de la terminal

    int msg_win_width = col * 0.7;                // 70% width for the messages window
    int active_users_width = col - msg_win_width; // Remaining 30% width for active users

    // Crear ventanas para las secciones
    msg_win = newwin(row - 3, msg_win_width, 0, 0);                           // Ventana principal para los mensajes de chat
    active_users_win = newwin(row - 3, active_users_width, 0, msg_win_width); // Ventana para los usuarios activos
    input_win = newwin(3, col, row - 3, 0);                                   // Ventana de entrada en la parte inferior
}

int start_multicast_chat()
{
    int sockFileDescriptor;
    struct ip_mreq multicastRequest;

    // Create socket
    sockFileDescriptor = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockFileDescriptor < 0)
    {
        perror("SOCKET CREATE FAILED");
        exit(EXIT_FAILURE);
    }

    int reuse = 1;
    if (setsockopt(sockFileDescriptor, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
        perror("SET OPTIONS FAILED");
        close(sockFileDescriptor);
        exit(EXIT_FAILURE);
    }

    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(MULTICAST_PORT);

    if (bind(sockFileDescriptor, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        perror("BIND FAILED");
        close(sockFileDescriptor);
        exit(EXIT_FAILURE);
    }

    multicastRequest.imr_multiaddr.s_addr = inet_addr(MULTICAST_GROUP);
    multicastRequest.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(sockFileDescriptor, IPPROTO_IP, IP_ADD_MEMBERSHIP, &multicastRequest, sizeof(multicastRequest)) < 0)
    {
        perror("JOIN MULTICAST FAILED");
        close(sockFileDescriptor);
        exit(EXIT_FAILURE);
    }

    memset(&multicastAddress, 0, sizeof(multicastAddress));
    multicastAddress.sin_family = AF_INET;
    multicastAddress.sin_addr.s_addr = inet_addr(MULTICAST_GROUP);
    multicastAddress.sin_port = htons(MULTICAST_PORT);

    return sockFileDescriptor;
}

int start_chat_server()
{
    int sockfd;
    socklen_t addressLength = sizeof(server_chat_address);

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    // Clear and initialize the server address structure
    memset(&server_chat_address, 0, sizeof(server_chat_address));
    server_chat_address.sin_family = AF_INET;
    server_chat_address.sin_port = htons(PORT);

    // Convert the IP address from string format to binary format
    if (inet_pton(AF_INET, SERVER_IP, &server_chat_address.sin_addr) <= 0)
    {
        perror("Invalid address/ Address not supported");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <username>\n", argv[0]);
        return EXIT_FAILURE;
    }

    username = argv[1];

    int serverFileDescriptor = start_chat_server();

    int sockFileDescriptor = start_multicast_chat();

    Packet packet = {
        .type = 1,
    };

    strncpy(packet.data, username, strlen(username));

    sendPacket(serverFileDescriptor, &server_chat_address, &packet);

    Packet userList;

    socklen_t addressLength = sizeof(server_chat_address);

    int bytes = recvfrom(serverFileDescriptor, &userList, sizeof(Packet), 0,
                         (struct sockaddr *)&server_chat_address, &addressLength);

    if (bytes <= 0)
    {
        exit(EXIT_FAILURE);
    }

    build_user_interface();

    SharedData shared_data;

    memset(&shared_data, 0, sizeof(shared_data));

    shared_data.mutex = &message_mutex;

    ThreadData thread_data = {
        .sockFileDescriptor = sockFileDescriptor,
        .shared_data = &shared_data,
        .chatFileDescriptor = serverFileDescriptor,
    };

    pthread_t receiver_thread;

    if (pthread_create(&receiver_thread, NULL, receive_messages, &thread_data) != 0)
    {
        perror("Failed to create thread");
        exit(EXIT_FAILURE);
    }

    pthread_t user_interface_thread;

    if (pthread_create(&user_interface_thread, NULL, repaint_messages_screen, &shared_data) != 0)
    {
        perror("Failed to create the thread \n");
        exit(EXIT_FAILURE);
    }

    pthread_t userlist_thread;

    if (pthread_create(&userlist_thread, NULL, receive_active_users, &thread_data) != 0)
    {
         perror("Failed to create the thread \n");
         exit(EXIT_FAILURE);
    }

    pthread_t userlist_interface_thread;

    if (pthread_create(&userlist_interface_thread, NULL, repaint_userlist_screen, &shared_data) != 0)
    {
        perror("Failed to create the thread \n");
        exit(EXIT_FAILURE);
    }

    draw_message_window(msg_win, shared_data.messages, shared_data.message_count);

    draw_input_window(input_win, "> ");

    draw_active_users_window(active_users_win, userList.data);

    while (1)
    {
        char input[MAX_MESSAGE_LENGTH] = "";
        echo();

        mvwgetnstr(input_win, 1, 3, input, MAX_MESSAGE_LENGTH - 1);

        noecho();

        if (strcmp(input, "exit") == 0)
        {
            Packet exitPacket;

            exitPacket.type = 5;

            strncpy(exitPacket.data, username, sizeof(exitPacket.data));

            sendPacket(serverFileDescriptor, &server_chat_address, &exitPacket);

            break;
        }

        Message message = { .is_private = 0 };

        strncpy(message.username, username, sizeof(message.username) - 1);

        if (input[0] == '@')
        {
            char *space_position = strchr(input, ' ');

            if (space_position != NULL)
            {
                size_t username_text = space_position - (input + 1);

                strncpy(message.username_sento, input + 1, username_text);

                message.username_sento[username_text] = '\0';

                strncpy(message.message, space_position + 1, sizeof(message.message) - 1);

                message.is_private = 1;

                message.is_file = 0;
            }
            else
            {
                continue;
            }
        }
        else if(input[0] == '#') {
            char *space_position = strchr(input, ' ');

            message.is_private = 0;

            strncpy(message.message, "Sending file", sizeof(message.message) - 1);

            Packet sendFilePacket;

            sendFilePacket.type = 6;

            sendPacket(serverFileDescriptor, &server_chat_address, &sendFilePacket);

            sendFile(serverFileDescriptor, &server_chat_address, "test.txt");
        }
        else
        {
            strncpy(message.message, input, sizeof(message.message) - 1);

            message.is_private = 0;

            message.is_file = 0;
        }

        sendto(sockFileDescriptor, &message, sizeof(message), 0, (struct sockaddr *)&multicastAddress, sizeof(multicastAddress));

        draw_input_window(input_win, "> ");
    }

    pthread_cancel(receiver_thread);

    pthread_cancel(user_interface_thread);

    pthread_cancel(userlist_thread);
    
    pthread_cancel(userlist_interface_thread);

    close(sockFileDescriptor);

    endwin();

    return 0;
}
