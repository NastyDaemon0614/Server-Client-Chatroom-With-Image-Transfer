#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <ctype.h>
#include<iostream>
#include<fstream>

#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"
#define RESET "\033[0m"

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
get_username (char *username)
{
    while (true)
      {
          printf ("Enter a username: ");
          fflush (stdout);
          memset (username, 0, 1000);
          fgets (username, 22, stdin);
          trim_newline (username);

          if (strlen (username) > 20)
            {

                puts ("Username must be 20 characters or less.");

            }
          else
            {
                break;
            }
      }
}

void
set_username (connection_info * connection)
{
    message msg;
    msg.type = SET_USERNAME;
    strncpy (msg.username, connection->username, 20);

    if (send (connection->socket, (void *) &msg, sizeof (msg), 0) < 0)
      {
          perror ("Send failed");
          exit (1);
      }
}

void
stop_client (connection_info * connection)
{
    close (connection->socket);
    exit (0);
}

void
connect_to_server (connection_info * connection, char *address, char *port)
{

    while (true)
      {
          get_username (connection->username);

          if ((connection->socket = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
            {
                perror ("Could not create socket");
            }

          connection->address.sin_addr.s_addr = inet_addr (address);
          connection->address.sin_family = AF_INET;
          connection->address.sin_port = htons (atoi (port));

          if (connect (connection->socket, (struct sockaddr *) &connection->address, sizeof (connection->address)) < 0)
            {
                perror ("Connect failed.");
                exit (1);
            }

          set_username (connection);

          message msg;
          ssize_t recv_val = recv (connection->socket, &msg, sizeof (message), 0);
          if (recv_val < 0)
            {
                perror ("recv failed");
                exit (1);

            }
          else if (recv_val == 0)
            {
                close (connection->socket);
                printf ("The username \"%s\" is taken, please try another name.\n", connection->username);
                continue;
            }
          break;
      }

    puts ("Connected to server.");
    puts ("Type /help for usage.");
}

void encrypt(char* msg)
{
  int len = strlen(msg);
  srand(len);
  for(int i=0;i<len;i++)
    {
      msg[i] = (msg[i]+(rand()%50)) % 256;
      while(msg[i]==0)
        msg[i] = (msg[i]+(rand()%50)) % 256;
    }
}

void decrypt(char* msg)
{
  int len = strlen(msg);
  srand(len);
  for(int i=0;i<len;i++)
    {
      msg[i] = (msg[i]-(rand()%50)+256) % 256;
      while(msg[i]==0)
        msg[i] = (msg[i]-(rand()%50)+256) % 256;
    }
}


void
handle_user_input (connection_info * connection)
{
    char input[255];
    fgets (input, 255, stdin);
    trim_newline (input);

    if (strcmp (input, "/q") == 0 || strcmp (input, "/quit") == 0)
      {
          stop_client (connection);
      }
    else if (strcmp (input, "/l") == 0 || strcmp (input, "/list") == 0)
      {
          message msg;
          msg.type = GET_USERS;

          if (send (connection->socket, &msg, sizeof (message), 0) < 0)
            {
                perror ("Send failed");
                exit (1);
            }
      }
    else if (strcmp (input, "/h") == 0 || strcmp (input, "/help") == 0)
      {
          puts ("/quit or /q: Exit the program.");
          puts ("/help or /h: Displays help information.");
          puts ("/list or /l: Displays list of users in chatroom.");
          puts ("/file or/f: Send a file in the chat room");
          puts ("@<username> <message> Send a private message to given username.");

      }
    else if((strcmp(input,"/f")==0)||(strcmp(input, "/file") == 0))
      {
          message msg;

          msg.type = FILE_SHARE;
          strncpy (msg.username, connection->username, 20);

          puts("Enter file name");
          fgets (input, 255, stdin);
          trim_newline(input);
          puts(input);
          FILE *f;
          f = fopen(input,"r");
          char ch;
          while(1)
          {
              char buf[1];
              ch=fgetc(f);
              buf[0]=ch;
              strncpy (msg.data, buf, 255);

              if (send (connection->socket, &msg, sizeof (message), 0) < 0)
                {
                    perror ("Send failed");
                    exit (1);
                }
              if(ch==EOF)
                {
                    break;
                }
          }
          puts("The File Was Sent Successfully");
      }
    else if((strcmp(input,"/i")==0)||(strcmp(input, "/image") == 0))
      {
          message msg;
          msg.type= IMG_SHARE;
          strncpy (msg.username, connection->username, 20);

          puts("Enter image name");
          fgets(input,255, stdin);

          strncpy(msg.data, input, 255);
          send(connection->socket, &msg, sizeof( message),0);

          puts(input);

          FILE *picture;
          int size, read_size, stat, packet_index;
          char send_buffer[10240], read_buffer[256];
          packet_index = 1;

          picture = fopen("capture.jpeg", "r");
          printf("Getting Picture Size\n");   

          printf("hii");

          if(picture == NULL) {
                printf("Error Opening Image File"); } 

          fseek(picture, 0, SEEK_END);
          size = ftell(picture);
          fseek(picture, 0, SEEK_SET);
          printf("Total Picture size: %i\n",size);

          //Send Picture Size
          printf("Sending Picture Size\n");
          write(connection->socket, (void *)&size, sizeof(int));

          //Send Picture as Byte Array
          printf("Sending Picture as Byte Array\n");

          do { //Read while we get errors that are due to signals.
              stat=read(connection->socket, &read_buffer , 255);
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
                stat = write(connection->socket, send_buffer, read_size);  
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
    else if (strncmp (input, "@", 1) == 0)
      {
          message msg;
          msg.type = PRIVATE_MESSAGE;

          char *toUsername, *chatMsg;

          toUsername = strtok (input + 1, " ");

          if (toUsername == NULL)
            {
                puts (KRED "The format for private messages is: @<username> <message>" RESET);
                return;
            }

          if (strlen (toUsername) == 0)
            {
                puts (KRED "You must enter a username for a private message." RESET);
                return;
            }

          if (strlen (toUsername) > 20)
            {
                puts (KRED "The username must be between 1 and 20 characters." RESET);
                return;
            }

          chatMsg = strtok (NULL, "");
          

          if (chatMsg == NULL)
            {
                puts (KRED "You must enter a message to send to the specified user." RESET);
                return;
            }

          strncpy (msg.username, toUsername, 20);
          encrypt(chatMsg);
          strncpy (msg.data, chatMsg, 255);

          if (send (connection->socket, &msg, sizeof (message), 0) < 0)
            {
                perror ("Send failed");
                exit (1);
            }

      }
    else
      {
          message msg;
          msg.type = PUBLIC_MESSAGE;
          strncpy (msg.username, connection->username, 20);

          if (strlen (input) == 0)
            {
                return;
            }
          encrypt(input);
          strncpy (msg.data, input, 255);

          if (send (connection->socket, &msg, sizeof (message), 0) < 0)
            {
                perror ("Send failed");
                exit (1);
            }
      }

}

int recv_image(int socket)
{
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
            return -1;
            }

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
    return 0;   
}

void
handle_server_message (connection_info * connection)
{
    message msg;

    ssize_t recv_val = recv (connection->socket, &msg, sizeof (message), 0);
    if (recv_val < 0)
      {
          perror ("recv failed");
          exit (1);

      }
    else if (recv_val == 0)
      {
          close (connection->socket);
          puts ("Server disconnected.");
          exit (0);
      }

      char curr_sender[20];
      char prev_sender[20];

    switch (msg.type)
      {

      case CONNECT:
          printf (KYEL "%s has connected." RESET "\n", msg.username);
          break;

      case DISCONNECT:
          printf (KYEL "%s has disconnected." RESET "\n", msg.username);
          break;

      case GET_USERS:
          printf (KMAG "%s" RESET "\n", msg.data);
          break;

      case PUBLIC_MESSAGE:
          decrypt(msg.data);
          printf (KGRN "%s" RESET ": %s\n", msg.username, msg.data);
          break;

      case PRIVATE_MESSAGE:
          decrypt(msg.data);
          printf (KWHT "From %s:" KCYN " %s\n" RESET, msg.username, msg.data);
          break;

      case TOO_FULL:
          fprintf (stderr, KRED "Server chatroom is too full to accept new clients." RESET "\n");
          exit (0);
          break;

      case FILE_SHARE:
          {

              FILE *fp;
              char ch;
              char filename[40];
              strncpy(filename,msg.username,40);
              int l=strlen(filename);
              char suf[14]="_received.txt";
              for(int m=0;m<13;m++)
                filename[l+m]=suf[m];

              fp=fopen(filename,"a");

              while(1)
              {

                strncpy(curr_sender,msg.username,20);
                if(strncmp(curr_sender,prev_sender,20)!=0)
                printf (KMAG "From %s:" KCYN "Receiving File" RESET ,msg.username);
                strncpy(prev_sender,curr_sender,20);


              char buf[1];
              strncpy(buf ,msg.data, 1);
            //  printf("%c",buf[0]);
              ch=buf[0];
              if(buf[0]==EOF)
              {
                printf(KYEL "\nFile received" RESET "\n");
                fclose(fp);

                break;
              }
              else
              {
                fprintf(fp,"%c",ch);
              }

              ssize_t recv_val = recv (connection->socket, &msg, sizeof (message), 0);
              if (recv_val < 0)
                {
                    perror ("recv failed");
                    exit (1);

                }
              else if (recv_val == 0)
                {
                    close (connection->socket);
                    puts ("Server disconnected.");
                    exit (0);
                }

              }
            }
          break;
      case IMG_SHARE:
        {
            recv_image(connection->socket);
            break;
        }

      default:
          fprintf (stderr, KRED "Unknown message type received." RESET "\n");
          break;
      }
}

int
main (int argc, char *argv[])
{
    connection_info connection;
    fd_set file_descriptors;

    if (argc != 3)
      {
          fprintf (stderr, "Usage: %s <IP> <port>\n", argv[0]);
          exit (1);
      }

    connect_to_server (&connection, argv[1], argv[2]);

    while (true)
      {
          FD_ZERO (&file_descriptors);
          FD_SET (STDIN_FILENO, &file_descriptors);
          FD_SET (connection.socket, &file_descriptors);
          fflush (stdin);

          if (select (connection.socket + 1, &file_descriptors, NULL, NULL, NULL) < 0)
            {
                perror ("Select failed.");
                exit (1);
            }

          if (FD_ISSET (STDIN_FILENO, &file_descriptors))
            {
                handle_user_input (&connection);
            }

          if (FD_ISSET (connection.socket, &file_descriptors))
            {
                handle_server_message (&connection);
            }
      }

    close (connection.socket);
    return 0;
}
