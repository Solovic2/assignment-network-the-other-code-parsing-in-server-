#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <regex.h>
// Semaphore variables
sem_t x, y; // one for read request , post request
pthread_t tid;
pthread_t writerthreads[100];
pthread_t readerthreads[100];
int countReaders = 0;
int count_file = 0;
struct files  
{  
	int id;
    char filename[500];  
    char body[1024];  
}; 
// struct data to save the acceptSocket number and the request message from client socket
typedef struct data {
    int acceptSocket;
    char client_request [4086];
	struct files file;
	
} requestData;

struct files server_files[10];

void* readRequestWithFileFromServer(void* request)
{
    // Lock the semaphore
    sem_wait(&x);
    countReaders++;
 
	// if there is one reading file , wait on  the second semaphore to not upload any thing
    if (countReaders == 1)
        sem_wait(&y);
 
    // Unlock the semaphore 
    sem_post(&x);

	// Read The Existens File
	struct data *requestData = (struct data*) request; // Get The Request Message Struct
 	char response[4096];
    bzero(response,4096);							// erase the data from response
	strcpy(response, "HTTP/1.1 200 OK\r\n\n");		// copy to response
	strcat(response,requestData->client_request);	// concatenate response message with the request from client
	printf("GET : %s \n",response);

	// send the response to client socket
	send(requestData->acceptSocket,response,sizeof(response),0);

 	printf("\n%d reader is inside",countReaders);
    sleep(5);
 
    // Lock the semaphore
    sem_wait(&x);
    countReaders--;
 
    if (countReaders == 0) {
        sem_post(&y);
    }
	
    // Lock the semaphore
    sem_post(&x);
 
    printf("\n%d Reader is leaving",
           countReaders + 1);
    pthread_exit(NULL);
}
// Function to send response to client socket.

void* uploadFileToServer(void* request)
{

	// Lock the semaphore
	sem_wait(&y);
	printf("WE R HERE \n");
	struct data *requestData = (struct data*) request; // Get The Request Message Struct

	send(requestData->acceptSocket,requestData->file.filename,sizeof(requestData->file.filename),0);
	printf(" \nAFTER");
	printf("\nindex %d , filename %s\n",requestData->file.id,requestData->file.filename);
	printf("server file name %s",server_files[requestData->file.id].filename);

	bzero(server_files[requestData->file.id].body,1024);
	recv(requestData->acceptSocket,server_files[requestData->file.id].body ,sizeof(server_files[requestData->file.id].body),0);
	
	printf("recieving %s\n",server_files[requestData->file.id].body);

	printf("\nwrite here");
	
	printf("\n Finished!");

	// Unlock the semaphore
	sem_post(&y);

	printf("\n Writer Leaveing ...");
	pthread_exit(NULL);
}

int checkIfExistFile(struct files myfiles[10],char *filename){
	if(strcmp(filename,"") == 0) return -1;
	for(int i=0; i<10 ;i++){
		if(strcmp(myfiles[i].filename,filename) == 0){
			return i;
		}
	}
	return -1;
}
int main(int arg,char* argv[])
{
	if(arg > 2 || arg < 2){
		printf(" You Should Add exact Two Arguments only\n");
		return 0;
	}
	int port  = strtol(argv[1],NULL,10);
	sem_init(&x, 0, 1);
	sem_init(&y, 0, 1);
	// initialize the socket
	int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in serverAddr;
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);


	
	// Bind socket to address and port number.
	if (bind(serverSocket,(struct sockaddr*)&serverAddr,sizeof(serverAddr)) != 0) {
        printf("socket bind failed...\n");
    }
	// Listen on the socket
	if (listen(serverSocket, 40) == 0) printf("Listening now ....\n");
	else printf("Error In Listening\n");

	// initialize pthread and semaphores
	pthread_t tid[60];
	int i = 0;

	// accept variable and serverStorage 
	int acceptSocket;
	// To store the address 
	struct sockaddr_storage storage;
	socklen_t addr_size;
	
	char recieved_request[4096];
	while(1){

		addr_size = sizeof(storage);
		// Accept connection to get the first connection in the queue 
		acceptSocket = accept(serverSocket, (struct sockaddr*) &storage, &addr_size);
		if (acceptSocket < 0) {
			printf("server accept failed...\n");
    	}
		// Recieve Message From Client  
		printf("Server Recived : \n");
		bzero(recieved_request,4096);	// erase the data 
		read(acceptSocket,&recieved_request, sizeof(recieved_request)); // recieve the request client message
		printf("here %s",recieved_request);

		/**************************  check for request get or post or elses **************************/
		
		regex_t regex;	// regex
		regmatch_t pmatch[4]; // to match
		size_t nmatch = 4; 

		// struct data *requestData = (struct data*) request; // Get The Request Message Struct
		char response[4096],regex_func[4096];
		int pattern;
		bzero(regex_func,4096);				// erase previous regex
		// GET | POST REGEX
		strcpy(regex_func, "^(GET|POST)\\s\\/\\s(\\S+)\\.(jpg|JPG|png|PNG|gif|GIF|jpeg|JPEG|html|HTML|txt|TXT)\\s\\/\\s(HTTP\\/1\\.1)");
		pattern = regcomp(&regex, regex_func, REG_EXTENDED);
		pattern = regexec(&regex, recieved_request,nmatch, pmatch, 0);
		// if matches
		if(pattern == 0){
			printf("%s","SSs");
			char type[5];			 // to get type of request GET or POST
			char filename[100];	 // to store filename of request
			char file_extention[5]; //	to store extention of file 
			if(pmatch[1].rm_so != -1 && pmatch[2].rm_so != -1 && pmatch[3].rm_so != -1) {
				// Set Variables type, filename,file_extention
				bzero(type,5);bzero(filename,100);bzero(file_extention,5);
				strncpy(type, &recieved_request[pmatch[1].rm_so], pmatch[1].rm_eo-pmatch[1].rm_so);
				strncpy(filename, &recieved_request[pmatch[2].rm_so], pmatch[2].rm_eo-pmatch[2].rm_so);
				strncpy(file_extention, &recieved_request[pmatch[3].rm_so], pmatch[3].rm_eo-pmatch[3].rm_so);
				// filename name.ext
				strcat(filename, ".");
				strcat(filename, file_extention);
			}
		

			// GET REQUEST (G) or POST REQUEST (P)
			if(strcmp(type,"GET") == 0){
				// Check if file exists or not
		
  				int index = checkIfExistFile(server_files, filename);
		
				printf(" Get index  is %d",index);

				//Read File if post or get and exists
				if(index != -1){				// if file is exist
					// Set Response message
					// Add Body of file
					strcpy(recieved_request,server_files[index].body);
					requestData data;
					data.acceptSocket = acceptSocket;
					strcpy(data.client_request , recieved_request);
					if (pthread_create(&readerthreads[i++], NULL,readRequestWithFileFromServer, &data)!= 0) printf("Error to create Read File thread\n");
					
				}else{		// if file not existent in get => return not found
				printf("hey");
					// Set Response message not found 
					bzero(response,4096);									// erase the data from response
					strcpy(response, "HTTP/1.1 404 Not Found\r\n");		// copy to response
					// send the response to client socket
					send(acceptSocket,response,sizeof(response),0);	// send the response to client socket
				} 
			}
			else{
				// Check if file exists or not
				int index = checkIfExistFile(server_files, filename);
				printf(" Post index  is %d",index);
				if(index != -1){
					// Open file from client and put it into server
					// send to client to get the file 
					server_files[index].id = index;
					strcpy(server_files[index].filename,filename);
					// محتاج اعمل ستركت جوا استركت هنا :(
					requestData data;
					data.acceptSocket = acceptSocket;
					strcpy(data.client_request , recieved_request);
					data.file = server_files[index];
					if (pthread_create(&writerthreads[i++], NULL,uploadFileToServer, &data)!= 0) printf("Error to create write thread\n");
				}else{
					if(count_file > 10){
						// Set Response message not found 
						bzero(response,4096);									// erase the data from response
						strcpy(response, "HTTP/1.1 404 Not Found\r\n");		// copy to response
						// send the response to client socket
						send(acceptSocket,response,sizeof(response),0);	// send the response to client socket
					}else{
						// Open file from client and put it into server
						// send to client to get the file 
						printf(" sss %d",count_file);
						server_files[count_file].id = count_file;
						strcpy(server_files[count_file].filename,filename);
						// محتاج اعمل ستركت جوا استركت هنا :(
						requestData data;
						data.acceptSocket = acceptSocket;
						strcpy(data.client_request , recieved_request);
						data.file = server_files[count_file];
						count_file++;
						if (pthread_create(&writerthreads[i++], NULL,uploadFileToServer, &data)!= 0) printf("Error to create write thread\n");
						printf("\nFile Uploaded Successfully!\n");
					}
				}
			
			}
	
		}else{
			// Set Response message
			bzero(response,4096);									// erase the data from response
			strcpy(response, "HTTP/1.1 404 Not Found\r\n");		// copy to response			
			// send the response to client socket
			send(acceptSocket,response,sizeof(response),0);	// send the response to client socket
		}

		if (i >= 40) {
			// reset i to zero
			i = 0;
			while (i < 40) {
				// join threads
				pthread_join(writerthreads[i++],NULL);
				pthread_join(readerthreads[i++],NULL);
				
			}
			i = 0;
		}
	}

	return 0;
}


// regex_t regex;	// regex
// 	regmatch_t pmatch[4]; // to match
// 	size_t nmatch = 4; 
// 	struct data *requestData = (struct data*) request; // Get The Request Message Struct
// 	struct files server_files[10];
// 	int fileNumber = 0;
// 	/**************************  check for request get or post or elses **************************/
// 	char response[4096],regex_func[4096];
// 	int pattern;
// 	bzero(regex_func,4096);				// erase previous regex
	
// 	// GET | POST REGEX
// 	strcpy(regex_func, "^(GET|POST)\\s\\/\\s(\\S+)\\.(jpg|JPG|png|PNG|gif|GIF|jpeg|JPEG|html|HTML|txt|TXT)\\s\\/\\s(HTTP\\/1\\.1)");
// 	pattern = regcomp(&regex, regex_func, REG_EXTENDED);
// 	pattern = regexec(&regex, requestData->client_request,nmatch, pmatch, 0);
// 	if(pattern == 0){
// 		char type[5];			 // to get type of request GET or POST
// 		char filename[100];	 // to store filename of request
// 		char file_extention[5]; //	to store extention of file 
//         if(pmatch[1].rm_so != -1 && pmatch[2].rm_so != -1 && pmatch[3].rm_so != -1) {
// 			// Set Variables type, filename,file_extention
// 			strncpy(type, &requestData->client_request[pmatch[1].rm_so], pmatch[1].rm_eo-pmatch[1].rm_so);
// 			strncpy(filename, &requestData->client_request[pmatch[2].rm_so], pmatch[2].rm_eo-pmatch[2].rm_so);
// 			strncpy(file_extention, &requestData->client_request[pmatch[3].rm_so], pmatch[3].rm_eo-pmatch[3].rm_so);

//         }
// 		// GET REQUEST (G) or POST REQUEST (P)
// 		if(strcmp(type,"GET") == 0){ 			// GET REQUEST
// 			// Set Response message
// 			bzero(response,4096);							// erase the data from response
// 			strcpy(response, "HTTP/1.1 200 OK\r\n\n");		// copy to response
// 			strcat(response,requestData->client_request);	// concatenate response message with the request from client
// 			printf("GET : %s \n",response);

// 			// send the response to client socket
// 			send(requestData->acceptSocket,response,sizeof(response),0);

// 		}else{

// 		// POST REQUEST

// 		}
// 	}else{
// 		// Set Response message
// 		bzero(response,4096);									// erase the data from response
// 		strcpy(response, "HTTP/1.1 404 NOTFOUND\r\n\n");		// copy to response
		
// 		// send the response to client socket
// 		send(requestData->acceptSocket,response,sizeof(response),0);	// send the response to client socket
// 	}
