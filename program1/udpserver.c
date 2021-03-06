/*
	Brendan Sailer
	Netid: bsailer1
	Program 1 - Computer Networks

	Run: ./updserver <Port Number>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pg1lib.h>

void server(char * argv[]);

int main(int argc, char * argv[]) {
	if (argc == 2) {
		server(argv);
	} else {
		fprintf(stderr, "udpserver <Port Number> \n");
		exit(1);
	}
}

void server(char * argv[]){
	struct sockaddr_in sin, client_addr;
	char received_check_sum[BUFSIZ];
	char current_check_sum[BUFSIZ];
	char buf[BUFSIZ];
	char client_key[BUFSIZ];
	int s;

	int port = atoi(argv[1]); 

	//* build address data structure */
	bzero((char *)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY; //Use the default IP address of server
	sin.sin_port = htons(port);

	/* Generate public key */
	char *pubKey = getPubKey();

	/* Setup passive open */
	if((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		fprintf(stderr, "udpserver: failed to open socket\n");
		exit(1);
	}
	
	if((bind(s, (struct sockaddr *)&sin, sizeof(sin))) < 0) {
		fprintf(stderr, "udpserver: failed to bind\n");
		exit(1);
	}
	
	int addr_len = sizeof(client_addr);

	while (1){
		printf("Waiting ...\n");

		/* Get public key from client */
		if(recvfrom(s, buf, sizeof(buf), 0,  (struct sockaddr *)&client_addr, &addr_len)==-1){
			fprintf(stderr, "udpserver: failed to receive public key\n");
			exit(1);
		}
		memcpy(client_key, buf, sizeof(buf));
		bzero((char*)&buf, sizeof(buf));

		/* Encrypt public key and send it back*/
		char * encryptedKey = encrypt(pubKey, client_key);
		int len = strlen(encryptedKey);
		if(sendto(s, encryptedKey, len, 0, (struct sockaddr *)&client_addr, sizeof(struct sockaddr))==-1){
			fprintf(stderr, "udpserver: failed to send encrypted key\n");
			exit(1);
		}
		
		/* Receive the checksum */	
		if(recvfrom(s, buf, sizeof(buf), 0,  (struct sockaddr *)&client_addr, &addr_len)==-1){
			fprintf(stderr, "udpserver: failed to receive checksum\n");
			exit(1);
		}
		memcpy(received_check_sum, buf, sizeof(buf));
		bzero((char*)&buf, sizeof(buf));
	
		/* Receive the message */	
		if(recvfrom(s, buf, sizeof(buf), 0,  (struct sockaddr *)&client_addr, &addr_len)==-1){
			fprintf(stderr, "udpserver: failed to receive message\n");
			exit(1);
		}

		/* Get the current time */
		struct timeval start_time;
		gettimeofday(&start_time, NULL);
		time_t t = start_time.tv_sec;
		struct tm *date = localtime(&t);
		
		/* Decrypt the message */
		char * decrypted_msg = decrypt(buf);

		/* Get the checksum and load it into a buffer to compare later */
		unsigned long check_sum = checksum(decrypted_msg);
		if(snprintf(current_check_sum, sizeof(current_check_sum), "%lu", check_sum) < 0){
			fprintf(stderr, "udpserver: failed to snprintf checksum\n");
			exit(1);
		}

		/* Print the relevant information to stdout */
		printf("*** New Message ***\n");
		printf("Received Time: %s", asctime(date));
		printf("Received Message:\n%s\n", decrypted_msg);
		printf("Received Client Checksum: %s\n", received_check_sum);
		printf("Calculated Checksum: %lu\n", check_sum);
		printf("\n");
		bzero((char*)&buf, sizeof(buf));
		
		/* Compare the current checksum to the received checksum and respond appropriately */
		if(strcmp(current_check_sum, received_check_sum)==0){ // Normal response - no errors 
			sprintf(buf, "%s", asctime(date));
			len = sizeof(buf);
			
			/* Respond to the client with the result*/
			if(sendto(s, buf, len, 0, (struct sockaddr *)&client_addr, sizeof(struct sockaddr))==-1){
				fprintf(stderr, "udpserver: failed to send timestamp response\n");
				exit(1);
			}
			bzero((char*)&buf, sizeof(buf));
		} else { // There is an error, so respond with the value 0
			fprintf(stderr, "udpserver: recevied and calculated checksum do NOT match!\n");
			char * answer = "0";
			len = strlen(answer);
			
			/* Respond to the client with the result*/
			if(sendto(s, answer, len, 0, (struct sockaddr *)&client_addr, sizeof(struct sockaddr))==-1){
				fprintf(stderr, "udpserver: failed to send error response\n");
				exit(1);
			}
		}
	}
	close(s);
}
