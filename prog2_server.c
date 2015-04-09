#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#define QLEN 50 /* size of request queue */

/*------------------------------------------------------------------------
* Program: server
*
* Purpose: Chat room with participants and observers
*
* Syntax: server [ port ] [ port ]
*
* port - protocol port number to use
*
*
*------------------------------------------------------------------------
*/

static int MAX_PARTICIPANTS = 64, MAX_OBSERVERS = 64, MAX_MSG = 1024;

int main(int argc, char **argv) {
	struct protoent *ptrp; /* pointer to a protocol table entry */
	struct sockaddr_in psad, osad; /* structure to hold server's address */
	struct sockaddr_in cad1,cad2; /* structure to hold client's address */
	int psd, osd; /* socket descriptors */
	int pport,oport; /* participant port and observer port numbers */
	int alen; /* length of address */
	if( argc != 3 ) {
		fprintf(stderr,"Error: Wrong number of arguments\n");
		fprintf(stderr,"usage:\n");
		fprintf(stderr,"./server participant_port observer_port\n");
		exit(EXIT_FAILURE);
	}

	memset((char *)&psad,0,sizeof(psad)); /* clear sockaddr structure */
	psad.sin_family = AF_INET; /* set family to Internet */
	psad.sin_addr.s_addr = INADDR_ANY; /* set the local IP address */

	memset((char *)&osad,0,sizeof(osad)); /* clear sockaddr structure */
	osad.sin_family = AF_INET; /* set family to Internet */
	osad.sin_addr.s_addr = INADDR_ANY; /* set the local IP address */

	pport = atoi(argv[1]); /* convert argument to binary */
	if (pport > 0) { /* test for illegal value */
		psad.sin_port = htons((u_short)pport);
	} else { /* print error message and exit */
		fprintf(stderr,"Error: Bad participant port number %s\n",argv[1]);
		exit(EXIT_FAILURE);
	}
	
	oport = atoi(argv[2]); /* convert argument to binary */
	if (oport > 0) { /* test for illegal value */
		osad.sin_port = htons((u_short)oport);
	} else { /* print error message and exit */
		fprintf(stderr,"Error: Bad observer port number %s\n",argv[1]);
		exit(EXIT_FAILURE);
	}

	/* Map TCP transport protocol name to protocol number */
	if ( ((long int)(ptrp = getprotobyname("tcp"))) == 0) {
		fprintf(stderr, "Error: Cannot map \"tcp\" to protocol number");
		exit(EXIT_FAILURE);
	}

	/* Create a socket */
	psd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if (psd < 0) {
		fprintf(stderr, "Error: Socket creation failed\n");
		exit(EXIT_FAILURE);
	}

	osd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if (osd < 0) {
		fprintf(stderr, "Error: Socket creation failed\n");
		exit(EXIT_FAILURE);
	}

	/* Bind a local address to the socket */
	if (bind(psd, (struct sockaddr *)&psad, sizeof(psad)) < 0) {
		fprintf(stderr,"Error: Bind failed\n");
		exit(EXIT_FAILURE);
	}

	if (bind(osd, (struct sockaddr *)&osad, sizeof(osad)) < 0) {
		fprintf(stderr,"Error: Bind failed\n");
		exit(EXIT_FAILURE);
	}

	/* Specify size of request queue */
	if (listen(psd, QLEN) < 0) {
		fprintf(stderr,"Error: Listen failed\n");
		exit(EXIT_FAILURE);
	}
	
	if (listen(osd, QLEN) < 0) {
		fprintf(stderr,"Error: Listen failed\n");
		exit(EXIT_FAILURE);
	}
	
	
	/* Main server loop - accept and handle requests */

	int part_sock[MAX_PARTICIPANTS], obsv_sock[MAX_OBSERVERS];
	char in_msg_buf[MAX_MSG],out_msg_buf[MAX_MSG+5];
	int cur_sd = 0, new_sd = 0, max_sd = 0;
	struct sockaddr_in cad;
	int rdy,n=0,msg_len = 0;
	fd_set rd_sd;
	char p_id[3];
	
	if (osd>psd){
		max_sd = osd; //initialize the max_sd
	} else {			//needed for select
		max_sd = psd;
	}
	
	int i,j;
	for (i = 0; i < MAX_PARTICIPANTS; i++){//Initialize list of participant sockets
		part_sock[i] = 0;
	}
	for (i = 0; i < MAX_OBSERVERS; i++){//Initialize list of observer sockets
		obsv_sock[i] = 0;
	}
	
	while(1){
		FD_ZERO(&rd_sd); //Clear the whole fd_set
		FD_SET(psd,&rd_sd);//Add the paricipant entry socket to fd_set
		FD_SET(osd,&rd_sd);//Add the observer entry socket to fd_set

		for (i=0; i<MAX_PARTICIPANTS; i++){//Add all current participants to fd_set
			cur_sd = part_sock[i];
			if (cur_sd>0){
				FD_SET(cur_sd, &rd_sd);
				if (cur_sd>max_sd){
					max_sd = cur_sd;
				}
			}
		}
		for (i=0; i<MAX_OBSERVERS; i++){//Add all current observers to fd_set
			cur_sd = obsv_sock[i];
			if (cur_sd>0){
				FD_SET(cur_sd, &rd_sd);
				if (cur_sd>max_sd){
					max_sd = cur_sd;
				}
			}
		}
		rdy = select(max_sd+1, &rd_sd, NULL, NULL, NULL);
		if (rdy < 0){
			fprintf(stderr,"Error: Select failed\n");
			exit(-1);
		}
		/*Listen on entry sockets in fd_set*/
		if (FD_ISSET(psd,&rd_sd)){//If select heard the participant entry socket..
			fprintf(stdout,"New participant joining..\n");
			alen = sizeof(cad);
			new_sd = accept(psd, (struct sockaddr *)&cad, &alen);
			if (new_sd<0){
				fprintf(stderr,"Error: Accept failed\n");
			}
			
			for (i=0;i<MAX_PARTICIPANTS;i++){//Store the socket descriptor (and give it an id)
				if (part_sock[i] == 0){
					part_sock[i] = new_sd;
					if (new_sd > max_sd){
						max_sd = new_sd;
					}
					break;
				}
			}
			
			if (i == MAX_PARTICIPANTS){
				fprintf(stderr,"Error: Too many participants, closing its socket..\n");
				close(new_sd);
			} else {
				char new_part[20] = "User ## has joined\n";//Notify observers
				sprintf(p_id,"%d",i);
				if (i<10){
					new_part[5] = '0';
					new_part[6] = p_id[0];
				} else {
					new_part[5] = p_id[0];
					new_part[6] = p_id[1];
				}
				send_all_obsv(obsv_sock, 20, new_part);
			}
		} else if (FD_ISSET(osd,&rd_sd)){//If select heard the observer entry socket..
			//check if too many observers
			fprintf(stderr,"New observer joining..\n");
			alen = sizeof(cad);
			new_sd = accept(osd, (struct sockaddr *)&cad, &alen);
			if (new_sd<0){
				fprintf(stderr,"Error: Accept failed\n");
			}
			
			for (i=0;i<MAX_OBSERVERS;i++){//Store the socket descriptor (and give it an id)
				if (obsv_sock[i] == 0){
					obsv_sock[i] = new_sd;
					if (new_sd > max_sd){
						max_sd = new_sd;
					}
					break;
				}
				if (i == MAX_OBSERVERS){
					fprintf(stderr,"Error: Too many observers, closing its socket..\n");
					close(new_sd);
				}
			}
			
			char new_obsv[26] = "A new observer has joined\n";//Notify observers
			send_all_obsv(obsv_sock,26,new_obsv);
		} else {
			/*Listen on all participant sockets in fd_set*/
			for (i=0;i<MAX_PARTICIPANTS;i++){
				cur_sd = part_sock[i];
				
				if (FD_ISSET(cur_sd, &rd_sd)){//A participant has sent something
					n = recv(cur_sd, in_msg_buf, MAX_MSG, 0);//recieve the message
					if (n <= 0){
						fprintf(stderr,"Participant %d has left, closing its socket..\n", i);
						close(cur_sd);
						part_sock[i] = 0;
						char left_part[17] = "User ## has left\n";
						sprintf(p_id,"%d",i);
						if (i<10){
							left_part[5] = '0';
							left_part[6] = p_id[0];
						} else {
							left_part[5] = p_id[0];
							left_part[6] = p_id[1];
						}
						send_all_obsv(obsv_sock,17,left_part);
					} else {
						sprintf(p_id,"%d",i);//get their id #
						/*Create message to send to observers*/
						if (i<10){
							out_msg_buf[0] = '0';
							out_msg_buf[1] = p_id[0];
						} else {
							out_msg_buf[0] = p_id[0];
							out_msg_buf[1] = p_id[1];
						}
						out_msg_buf[2] = ':';
						out_msg_buf[3] = ' ';
				//		if (n > MAX_MSG){
				//			fprintf(stderr,"Participant %d sent a message larger than MAX_MSG\n", i);
				//		} else {
							for (j=0;j<=n;j++){
								out_msg_buf[j+4] = in_msg_buf[j];
							}
							if (out_msg_buf[j+2] != '\n'){
								out_msg_buf[j+3] = '\n';
								fprintf(stderr,"Participant %d sent a message larger than MAX_MSG\n", i);
								j = j+4; //now j is the length of out_msg_buf
							} else {
								j = j+3; //now j is the length of out_msg_buf
							}
						/*DEBUG*/
						//for (z=0;z<j;z++){
						//	fprintf(stderr,"%c",out_msg_buf[z]);
						//}
						/*DEBUG*/
							send_all_obsv(obsv_sock,j,out_msg_buf);//Notify observers
				//		}
					}
				}
			}
			/*Listen on all observer sockets in fd_set*/
			for (i=0;i<MAX_OBSERVERS;i++){
				cur_sd = obsv_sock[i];
				if (FD_ISSET(cur_sd, &rd_sd)){//Observer sent something, most likely left
					n = recv(cur_sd, in_msg_buf, MAX_MSG, 0);//recieve the message
					
					if (n <= 0){
						fprintf(stderr,"Observer has left, closing its socket..\n");
						close(cur_sd);
						obsv_sock[i] = 0;
					}
				}
			}
		}
	}
}

int send_all_obsv(int obsv_sock[MAX_OBSERVERS], int msg_len, char msg[MAX_MSG]){
	int i;
	//int j; //debug
	for(i=0;i<MAX_OBSERVERS;i++){
		if(obsv_sock[i] != 0){
			//fprintf(stderr,"Found valid observer at obsv_sock[%d]\nSending %d bytes: \n",i,msg_len);
			//for(j=0;j<msg_len;j++){
			//	fprintf(stderr,"%c",msg[j]);
			//}
			send(obsv_sock[i],msg,msg_len,0);
		}	
	}
	return 0;
}
