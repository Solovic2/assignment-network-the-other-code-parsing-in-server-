// C program for the Client Side
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <regex.h>

struct client_data{
	char server_ip[16];
	int port;
	char client_request[4096];
};
// Function to send data to server socket and recieve respose
void* threadClient(void* args)
{
	// Get The Request Message
	struct client_data *client = (struct client_data*) args; // Get The Request Message Struct
	
	// Create socket and initialize it
	int network_socket = socket(AF_INET,SOCK_STREAM, 0);
	struct sockaddr_in server_address;
	server_address.sin_addr.s_addr = inet_addr(client->server_ip);
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(client->port);

	// Connect to socket
	int connection_status = connect(network_socket,(struct sockaddr*)&server_address,sizeof(server_address));

	// Check if no connection between two sockets
	if(connection_status < 0){
		printf("Error\n");
		return 0;
	}

	printf("Connected Successfully ! .. \n");
	
	// Send data to the socket
	send(network_socket, client->client_request,strlen(client->client_request), 0);

	// recieve if there server want client to send the file or not

	char respose[4096];

	// Recieve From Socket GET / POST REQUEST
	recv(network_socket, &respose,sizeof(respose), 0);

	if(strncmp(respose,"HTTP/1.1",8) != 0){
		// get file and upload it 
		printf("%s",respose);
		char fileData[1024];
		FILE *file = fopen(respose,"r");
		fgets(fileData,1024,file);
		send(network_socket, fileData,sizeof(fileData), 0);
	}
	
	printf("You Recived %s\n",respose);

	close(network_socket);
	pthread_exit(NULL);

	return 0;
}

// Driver Code
int main(int arg , char* argv[]){	

	if(arg > 3 || arg < 3){
		printf(" You Should Add exact Three Arguments only (./client)(server_ip)(port_number) \n");
		return 0;
	}
	struct client_data client;
	pthread_t tid;
	pthread_t readFile[100];
	char client_request[4096];
	int n = 0;
	int port  = strtol(argv[2],NULL,10); 	// port
	if(strcmp(argv[1],"localhost")==0 ||strcmp(argv[1],"127.0.0.1")==0 )
 		strcpy(client.server_ip,"127.0.0.1");	// if it localhost convert to 127.0.0.1
	else {
		printf("No server");
		return 0;
	}				
	client.port = port;		// Port
	/******			 Take Input From Client    ****/
	int count=0;
	int i = 0;
	FILE *file = fopen("input.txt","r");
	while(fgets(client_request,4086,file)) {
			count++;
			printf("From Client %s\n",client_request);
			strcpy(client.client_request,client_request);		// Message
			bzero(client_request,4086);
			if(pthread_create(&readFile[i++], NULL,threadClient,&client)!=0) printf("Error in thread");
			sleep(10);
			if (i >= 40) {
					// reset i to zero
				i = 0;
				while (i < 40) {
					// join threads
					pthread_join(readFile[i++],NULL);
				}
				i = 0;
			}
	
	}
	printf("All Request %d \n",count);
		
	return 0;
		
	
}
