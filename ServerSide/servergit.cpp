#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <iostream>
#include <fstream>

#define MAX_CLIENTS 10

typedef enum
{
    CONNECT,
    DISCONNECT,
    GET_USERS,
    SET_USERNAME,
    PUBLIC_MESSAGE,
    PRIVATE_MESSAGE,
    TOO_FULL,
    USERNAME_ERROR,
    SUCCESS,
    ERROR,
    FILE_SHARE,
    IMG_SHARE
} message_type;

typedef struct
{
    message_type type;
    char username[21];
    char data[256];

} message;

typedef struct connection_info
{
    int socket;
    struct sockaddr_in address;
    char username[20];
} connection_info;

void
trim_newline (char *text)
{
    int len = strlen (text) - 1;
    if (text[len] == '\n')
      {
          text[len] = '\0';
      }
}

void
initialize_server (connection_info * server_info, int port)
{
    if ((server_info->socket = socket (AF_INET, SOCK_STREAM, 0)) < 0)
      {
          perror ("Failed to create socket");
          exit (1);
      }

    server_info->address.sin_family = AF_INET;
    server_info->address.sin_addr.s_addr = INADDR_ANY;
    server_info->address.sin_port = htons (port);

    if (bind (server_info->socket, (struct sockaddr *) &server_info->address, sizeof (server_info->address)) < 0)
      {
          perror ("Binding failed");
          exit (1);
      }

    const int optVal = 1;
    const socklen_t optLen = sizeof (optVal);
    if (setsockopt (server_info->socket, SOL_SOCKET, SO_REUSEADDR, (void *) &optVal, optLen) < 0)
      {
          perror ("Set socket option failed");
          exit (1);
      }

    if (listen (server_info->socket, 3) < 0)
      {
          perror ("Listen failed");
          exit (1);
      }

    printf ("Waiting for incoming connections...\n");
}

void
send_public_message (connection_info clients[], int sender, char *message_text)
{
    message msg;
    msg.type = PUBLIC_MESSAGE;
    strncpy (msg.username, clients[sender].username, 20);
    strncpy (msg.data, message_text, 256);
    int i = 0;
    for (i = 0; i < MAX_CLIENTS; i++)
      {
          if (i != sender && clients[i].socket != 0)
            {
                if (send (clients[i].socket, &msg, sizeof (msg), 0) < 0)
                  {
                      perror ("Send failed");
                      exit (1);
                  }
            }
      }
}

void send_file(connection_info clients[], int sender, char* message_text)
{
  message msg;
  msg.type=FILE_SHARE;
  strncpy (msg.username, clients[sender].username, 20);
  strncpy (msg.data, message_text, 256);
  int i=0;
  for (i = 0; i < MAX_CLIENTS; i++)
    {
        if (i != sender && clients[i].socket != 0)
          {
              if (send (clients[i].socket, &msg, sizeof (msg), 0) < 0)
                {
                    perror ("Send failed");
                    exit (1);
                }
          }
    }
}

int send_image(connection_info clients[], int sender)
{
    message msg;
          msg.type= IMG_SHARE;
          strncpy (msg.username, clients[sender].username, 20);
          strncpy (msg.data, "data", 256);
          int i=0;
          for(i=0; i<MAX_CLIENTS;i++)
            {
                if(i!=sender && clients[i].socket != 0)
                {
                    send(clients[i].socket, &msg, sizeof( message),0);

                    FILE *picture;
                    int size, read_size, stat, packet_index;
                    char send_buffer[10240], read_buffer[256];
                    packet_index = 1;

                    picture = fopen("recv_image.jpeg", "r");
                    printf("Getting Picture Size\n");   

                    if(picture == NULL) {
                            printf("Error Opening Image File"); } 

                    fseek(picture, 0, SEEK_END);
                    size = ftell(picture);
                    fseek(picture, 0, SEEK_SET);
                    printf("Total Picture size: %i\n",size);

                    //Send Picture Size
                    printf("Sending Picture Size\n");
                    write(clients[i].socket, (void *)&size, sizeof(int));

                    //Send Picture as Byte Array
                    printf("Sending Picture as Byte Array\n");

                    do { //Read while we get errors that are due to signals.
                        stat=read(clients[i].socket, &read_buffer , 255);
                        printf("Bytes read: %i\n",stat);
                    } while (stat < 0);

                    printf("Received data in socket\n");
                    printf("Socket data: %s\n", read_buffer);

                    while(!feof(picture)) {
                    //while(packet_index = 1){
                        //Read from the file into our send buffer
                        read_size = fread(send_buffer, 1, sizeof(send_buffer)-1, picture);

                        //Send data through our socket 
                        do{
                            stat = write(clients[i].socket, send_buffer, read_size);  
                        }while (stat < 0);

                        printf("Packet Number: %i\n",packet_index);
                        printf("Packet Size Sent: %i\n",read_size);     
                        printf(" \n");
                        printf(" \n");

                        packet_index++;  

                        //Zero out our send buffer
                        bzero(send_buffer, sizeof(send_buffer));
                    }
                }
            }
        return 0;
}

int recv_image(connection_info clients[], int sender, char* img_name)
{
    int socket=clients[sender].socket;

    int buffersize = 0, recv_size = 0,size = 0, read_size, write_size, packet_index =1,stat;

    char imagearray[10241],verify = '1';
    FILE *image = fopen("recv_image.jpeg", "w");;

    //Find the size of the image
    do{
    stat = read(socket, &size, sizeof(int));
    }while(stat<0);

    printf("Packet received.\n");
    printf("Packet size: %i\n",stat);
    printf("Image size: %i\n",size);
    printf(" \n");

    char buffer[] = "Got it";

    //Send our verification signal
    do{
    stat = write(socket, &buffer, sizeof(int));
    }while(stat<0);

    printf("Reply sent\n");
    printf(" \n");

    if( image == NULL) {
    printf("Error has occurred. Image file could not be opened\n");
    return -1; }

    //Loop while we have not received the entire file yet


    int need_exit = 0;
    struct timeval timeout = {10,0};

    fd_set fds;
    int buffer_fd, buffer_out;

    while(recv_size < size) {
    //while(packet_index < 2){

        FD_ZERO(&fds);
        FD_SET(socket,&fds);

        buffer_fd = select(FD_SETSIZE,&fds,NULL,NULL,&timeout);

        if (buffer_fd < 0)
          printf("error: bad file descriptor set.\n");

        if (buffer_fd == 0)
          printf("error: buffer read timeout expired.\n");

        if (buffer_fd > 0)
        {
            do{
                  read_size = read(socket,imagearray, 10241);
                }while(read_size <0);

                printf("Packet number received: %i\n",packet_index);
            printf("Packet size: %i\n",read_size);


            //Write the currently read data into our image file
            write_size = fwrite(imagearray,1,read_size, image);
            printf("Written image size: %i\n",write_size); 

                if(read_size !=write_size) {
                    printf("error in read write\n");    }


                //Increment the total number of bytes read
                recv_size += read_size;
                packet_index++;
                printf("Total received image size: %i\n",recv_size);
                printf(" \n");
                printf(" \n");
        }

    }
    fclose(image);
    printf("Image successfully Received!\n");

    send_image(clients,sender);
    return 1;
}

void
send_private_message (connection_info clients[], int sender, char *username, char *message_text)
{
    message msg;
    msg.type = PRIVATE_MESSAGE;
    strncpy (msg.username, clients[sender].username, 20);
    strncpy (msg.data, message_text, 256);

    int i;
    for (i = 0; i < MAX_CLIENTS; i++)
      {
          if (i != sender && clients[i].socket != 0 && strcmp (clients[i].username, username) == 0)
            {
                if (send (clients[i].socket, &msg, sizeof (msg), 0) < 0)
                  {
                      perror ("Send failed");
                      exit (1);
                  }
                return;
            }
      }

    msg.type = USERNAME_ERROR;
    sprintf (msg.data, "Username \"%s\" does not exist or is not logged in.", username);

    if (send (clients[sender].socket, &msg, sizeof (msg), 0) < 0)
      {
          perror ("Send failed");
          exit (1);
      }

}

void
send_connect_message (connection_info * clients, int sender)
{
    message msg;
    msg.type = CONNECT;
    strncpy (msg.username, clients[sender].username, 21);
    int i = 0;
    for (i = 0; i < MAX_CLIENTS; i++)
      {
          if (clients[i].socket != 0)
            {
                if (i == sender)
                  {
                      msg.type = SUCCESS;
                      if (send (clients[i].socket, &msg, sizeof (msg), 0) < 0)
                        {
                            perror ("Send failed");
                            exit (1);
                        }
                  }
                else
                  {
                      if (send (clients[i].socket, &msg, sizeof (msg), 0) < 0)
                        {
                            perror ("Send failed");
                            exit (1);
                        }
                  }
            }
      }
}

void
send_disconnect_message (connection_info * clients, char *username)
{
    message msg;
    msg.type = DISCONNECT;
    strncpy (msg.username, username, 21);
    int i = 0;
    for (i = 0; i < MAX_CLIENTS; i++)
      {
          if (clients[i].socket != 0)
            {
                if (send (clients[i].socket, &msg, sizeof (msg), 0) < 0)
                  {
                      perror ("Send failed");
                      exit (1);
                  }
            }
      }
}

void
send_user_list (connection_info * clients, int receiver)
{
    message msg;
    msg.type = GET_USERS;
    char *list = msg.data;

    int i;
    for (i = 0; i < MAX_CLIENTS; i++)
      {
          if (clients[i].socket != 0)
            {
                list = stpcpy (list, clients[i].username);
                list = stpcpy (list, "\n");
            }
      }

    if (send (clients[receiver].socket, &msg, sizeof (msg), 0) < 0)
      {
          perror ("Send failed");
          exit (1);
      }

}

void
send_too_full_message (int socket)
{
    message too_full_message;
    too_full_message.type = TOO_FULL;

    if (send (socket, &too_full_message, sizeof (too_full_message), 0) < 0)
      {
          perror ("Send failed");
          exit (1);
      }

    close (socket);
}

void
stop_server (connection_info connection[])
{
    int i;
    for (i = 0; i < MAX_CLIENTS; i++)
      {
          close (connection[i].socket);
      }
    exit (0);
}

void
handle_client_message (connection_info clients[], int sender)
{
    int read_size;
    message msg;

    if ((read_size = recv (clients[sender].socket, &msg, sizeof (message), 0)) == 0)
      {
          printf ("User disconnected: %s.\n", clients[sender].username);
          close (clients[sender].socket);
          clients[sender].socket = 0;
          send_disconnect_message (clients, clients[sender].username);

      }
    else
      {

          switch (msg.type)
            {
            case GET_USERS:
                send_user_list (clients, sender);
                break;

            case SET_USERNAME:;
                int i;
                for (i = 0; i < MAX_CLIENTS; i++)
                  {
                      if (clients[i].socket != 0 && strcmp (clients[i].username, msg.username) == 0)
                        {
                            close (clients[sender].socket);
                            clients[sender].socket = 0;
                            return;
                        }
                  }

                strcpy (clients[sender].username, msg.username);
                printf ("User connected: %s\n", clients[sender].username);
                send_connect_message (clients, sender);
                break;

            case PUBLIC_MESSAGE:
                send_public_message (clients, sender, msg.data);
                break;

            case PRIVATE_MESSAGE:
                send_private_message (clients, sender, msg.username, msg.data);
                break;

            case FILE_SHARE:
                send_file (clients, sender,msg.data);
                break;

            case IMG_SHARE:
                recv_image(clients,sender, msg.data);
                break;

            default:
                fprintf (stderr, "Unknown message type received.\n");
                break;
            }
      }
}

int
construct_fd_set (fd_set * set, connection_info * server_info, connection_info clients[])
{
    FD_ZERO (set);
    FD_SET (STDIN_FILENO, set);
    FD_SET (server_info->socket, set);

    int max_fd = server_info->socket;
    int i;
    for (i = 0; i < MAX_CLIENTS; i++)
      {
          if (clients[i].socket > 0)
            {
                FD_SET (clients[i].socket, set);
                if (clients[i].socket > max_fd)
                  {
                      max_fd = clients[i].socket;
                  }
            }
      }
    return max_fd;
}

void
handle_new_connection (connection_info * server_info, connection_info clients[])
{
    int new_socket;
    int address_len;
    new_socket = accept (server_info->socket, (struct sockaddr *) &server_info->address, (socklen_t *) & address_len);

    if (new_socket < 0)
      {
          perror ("Accept Failed");
          exit (1);
      }

    int i;
    for (i = 0; i < MAX_CLIENTS; i++)
      {
          if (clients[i].socket == 0)
            {
                clients[i].socket = new_socket;
                break;

            }
          else if (i == MAX_CLIENTS - 1)
            {
                send_too_full_message (new_socket);
            }
      }
}

void
handle_user_input (connection_info clients[])
{
    char input[255];
    fgets (input, sizeof (input), stdin);
    trim_newline (input);

    if (input[0] == 'q')
      {
          stop_server (clients);
      }
}

int
main (int argc, char *argv[])
{
    puts ("Starting server.");

    fd_set file_descriptors;

    connection_info server_info;
    connection_info clients[MAX_CLIENTS];

    int i;
    for (i = 0; i < MAX_CLIENTS; i++)
      {
          clients[i].socket = 0;
      }

    if (argc != 2)
      {
          fprintf (stderr, "Usage: %s <port>\n", argv[0]);
          exit (1);
      }

    initialize_server (&server_info, atoi (argv[1]));

    while (true)
      {
          int max_fd = construct_fd_set (&file_descriptors, &server_info, clients);

          if (select (max_fd + 1, &file_descriptors, NULL, NULL, NULL) < 0)
            {
                perror ("Select Failed");
                stop_server (clients);
            }

          if (FD_ISSET (STDIN_FILENO, &file_descriptors))
            {
                handle_user_input (clients);
            }

          if (FD_ISSET (server_info.socket, &file_descriptors))
            {
                handle_new_connection (&server_info, clients);
            }

          for (i = 0; i < MAX_CLIENTS; i++)
            {
                if (clients[i].socket > 0 && FD_ISSET (clients[i].socket, &file_descriptors))
                  {
                      handle_client_message (clients, i);
                  }
            }
      }

    return 0;
}
