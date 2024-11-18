#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>  // For threading and mutex

#define MAX_MESSAGE_LENGTH 300
#define MAX_MESSAGES 100  // Maximum capacity of the message history
#define MULTICAST_GROUP "239.0.0.1"
#define MULTICAST_PORT 12345

typedef struct {
    char message[MAX_MESSAGE_LENGTH];
    char username[100];
    int is_private;
    char username_sento[100];
} Message;

typedef struct {
    Message messages[MAX_MESSAGES];
    int message_count;
    pthread_mutex_t *mutex;
} SharedData;

typedef struct {
    int sockFileDescriptor;
    SharedData *shared_data;
} ThreadData;

// GLobal stuff -_-
WINDOW *msg_win;
WINDOW *input_win;
WINDOW *active_users_win;
struct sockaddr_in multicastAddress, serverAddress;
char *username;

pthread_mutex_t message_mutex = PTHREAD_MUTEX_INITIALIZER;  // Mutex to protect shared resources

void draw_input_window(WINDOW *input_win, const char *prompt) {
    werase(input_win); // Clear the input window
    box(input_win, 0, 0);
    mvwprintw(input_win, 1, 1, "%s", prompt);
    wrefresh(input_win);
}

void draw_message_window(WINDOW *msg_win, Message messages[MAX_MESSAGES], int message_count) {
    int height, width;
    getmaxyx(msg_win, height, width);  // Get the size of the message window

    wclear(msg_win);          // Clear the message window
    box(msg_win, 0, 0);       // Draw border around the message window
    mvwprintw(msg_win, 1, 1, "Chat Messages");

    int visible_messages = height - 3; // Calculate visible messages (height minus header and border)

    // Determine where to start displaying messages if there are more messages than can fit
    int start = message_count > visible_messages ? message_count - visible_messages : 0;

    // Display the visible messages
    for (int i = start; i < message_count; i++) {
        if(messages[i].is_private) {
             mvwprintw(msg_win, 2 + i - start, 1, "[Private] %s: %s", messages[i].username, messages[i].message);
        } else {
            mvwprintw(msg_win, 2 + i - start, 1, "%s: %s", messages[i].username, messages[i].message);
        }
    }

    wrefresh(msg_win);
}

void *receive_messages(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    
    int sockFileDescriptor = data->sockFileDescriptor;

    SharedData *shared_data = data->shared_data;

    struct sockaddr_in serverAddress;
    
    Message received_message;  // To hold the received message struct

    socklen_t addressLength = sizeof(serverAddress);

    while (1) {
        // Receive the full Message struct
        int bytes = recvfrom(sockFileDescriptor, &received_message, sizeof(received_message), 0, 
                             (struct sockaddr *)&serverAddress, &addressLength);

        if (bytes < 0) {
            perror("RECEIVING FAILED");
            continue;
        }

        // Add the new message to the message history if its global or private to you

        if(!received_message.is_private || (received_message.is_private && strcmp(received_message.username_sento, username)) == 0 ) {
            if (shared_data->message_count < MAX_MESSAGES) {
                // Copy the received message struct into the shared message history
                shared_data->messages[shared_data->message_count] = received_message;
                shared_data->message_count++;
            } else {
                // Shift the messages to make space for the new one
                for (int i = 1; i < MAX_MESSAGES; i++) {
                    shared_data->messages[i - 1] = shared_data->messages[i];
                }
                // Add the new message at the end
                shared_data->messages[MAX_MESSAGES - 1] = received_message;
            }
        }
    }

    return NULL;
}

void *repaint_messages_screen(void *arg) {
    SharedData *shared_data = (SharedData *)arg;

    while(1) {
        sleep(1);

        draw_message_window(msg_win, shared_data->messages, shared_data->message_count);
    }

    return NULL;
}

void draw_active_users_window(WINDOW *active_users_win) {
    werase(active_users_win); // Clear the active users window
    box(active_users_win, 0, 0);
    mvwprintw(active_users_win, 1, 1, "Active Users"); // Header for the active users window
    wrefresh(active_users_win);
}

void build_user_interface() {
    initscr();            // Iniciar el modo ncurses
    raw();                // Desactivar el buffering de línea
    keypad(stdscr, TRUE); // Habilitar teclas especiales
    noecho();             // Desactivar el eco automático de la entrada del usuario

    int row, col;
    getmaxyx(stdscr, row, col); // Obtener el tamaño de la terminal

    int msg_win_width = col * 0.7; // 70% width for the messages window
    int active_users_width = col - msg_win_width; // Remaining 30% width for active users

    // Crear ventanas para las secciones
    msg_win = newwin(row - 3, msg_win_width, 0, 0);               // Ventana principal para los mensajes de chat
    active_users_win = newwin(row - 3, active_users_width, 0, msg_win_width); // Ventana para los usuarios activos
    input_win = newwin(3, col, row - 3, 0);                       // Ventana de entrada en la parte inferior
}

int start_chat_server() {
    int sockFileDescriptor;
    struct ip_mreq multicastRequest;

    // Create socket
    sockFileDescriptor = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockFileDescriptor < 0) {
        perror("SOCKET CREATE FAILED");
        exit(EXIT_FAILURE);
    }

    int reuse = 1;
    if (setsockopt(sockFileDescriptor, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("SET OPTIONS FAILED");
        close(sockFileDescriptor);
        exit(EXIT_FAILURE);
    }

    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(MULTICAST_PORT);

    if (bind(sockFileDescriptor, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("BIND FAILED");
        close(sockFileDescriptor);
        exit(EXIT_FAILURE);
    }

    multicastRequest.imr_multiaddr.s_addr = inet_addr(MULTICAST_GROUP);
    multicastRequest.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(sockFileDescriptor, IPPROTO_IP, IP_ADD_MEMBERSHIP, &multicastRequest, sizeof(multicastRequest)) < 0) {
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

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <username>\n", argv[0]);
        return EXIT_FAILURE;
    }

    username = argv[1];
    
    int sockFileDescriptor = start_chat_server();

    build_user_interface();

    // Initialize shared data
    SharedData shared_data;
    memset(&shared_data, 0, sizeof(shared_data)); // Initialize shared data structure
    shared_data.mutex = &message_mutex;  // Use the global mutex

    // Create thread data structure to pass both sockFileDescriptor and shared_data
    ThreadData thread_data = { sockFileDescriptor, &shared_data };

    // Create the thread to receive messages
    pthread_t receiver_thread;

    if (pthread_create(&receiver_thread, NULL, receive_messages, &thread_data) != 0) {
        perror("Failed to create thread");
        exit(EXIT_FAILURE);
    }
    
    pthread_t user_interface_thread;

    if(pthread_create(&user_interface_thread, NULL, repaint_messages_screen, &shared_data) != 0) {
        perror("Failed to create the thread \n");
        exit(EXIT_FAILURE);
    }

    // Draw initial windows
    draw_message_window(msg_win, shared_data.messages, shared_data.message_count);
    draw_input_window(input_win, "> ");
    draw_active_users_window(active_users_win);

    // Main chat interaction loop
    while (1) {
        char input[MAX_MESSAGE_LENGTH] = "";  // Clear input buffer
        echo(); // Allow input to be displayed on screen

        // Move the cursor just after the prompt
        mvwgetnstr(input_win, 1, 3, input, MAX_MESSAGE_LENGTH - 1); // Get user input
        noecho(); // Disable input echoing after user types

        // If user types "exit", break the loop
        if (strcmp(input, "exit") == 0) {
            break;
        }

        Message message = { .is_private = 0 };

        strncpy(message.username, username, sizeof(message.username) - 1);

        if(input[0] == '@') {
            char * space_position = strchr(input, ' ');

            if(space_position != NULL) {
                size_t username_text = space_position - (input + 1);

                strncpy(message.username_sento, input + 1, username_text);

                message.username_sento[username_text] = '\0';

                strncpy(message.message, space_position + 1, sizeof(message.message) - 1);

                message.is_private = 1;
            } else {
                continue;
            }
        } else {
            strncpy(message.message, input, sizeof(message.message) - 1);
            message.is_private = 0;
        }
    

        // Send the message to the multicast address
        sendto(sockFileDescriptor, &message, sizeof(message), 0, (struct sockaddr *)&multicastAddress, sizeof(multicastAddress));

        draw_input_window(input_win, "> ");
    }

    // Clean up and close
    pthread_cancel(receiver_thread);  // Cancel the receiver thread

    close(sockFileDescriptor);        // Close the socket
    
    endwin();

    return 0;
}


// TODO: Send private messages
// TODO: Send files to the chat
