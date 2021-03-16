#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>

int Client_Function(){
	//vom folosi structura struct addrinfo care va fi umpluta automat de functia getaddrinfo()
	struct addrinfo ServerAddress;
	struct addrinfo *servinfo;
	int status;	
	//ne asiguram ca structura e goala
	memset(&ServerAddress, 0, sizeof ServerAddress);
	ServerAddress.ai_family = AF_INET6;
	ServerAddress.ai_socktype = SOCK_STREAM;
	// getaddrinfo() returneaza un pointer la o lista ordonata de structuri addrinfo
	status = getaddrinfo("www.ipv6.com", "80", &ServerAddress, &servinfo);
	
	if(status !=0){
		perror("Adresa IP necunoscuta:");
		return 0;
	}
	//creare socket cu ajutorul pointerului servinfo
	int conn_socket = socket(servinfo->ai_family, servinfo->ai_socktype,0);
	if(conn_socket == -1){
		perror("Eroare la crearea socketului!");
		return 0;
	}
	printf("Socket creat cu succes!!!\n");
	// realizare conexiune la adresa specificata 
	int connect_val = connect(conn_socket, servinfo->ai_addr, servinfo->ai_addrlen);
	if(connect_val == -1){
		perror("Conexiunea cu serverul nu s-a putut realiza!");
		return 0;
	}
	printf("Conexiunea cu serverul s-a realizat cu succes!\n");
	// golim lista inlantuita
	freeaddrinfo(servinfo);

	FILE *fptr;
	//deschid in modul w pentru scrierea octetilor primiti
	fptr = fopen("index.html","w");
	if(fptr == NULL){
		perror("Eroare deschidere fisier pentru scriere!");
		return 0;
	}
	//pointeaza la datele pe care le trimit
	char *msg = "GET / HTTP/1.0\r\n\r\n";
	int len = strlen(msg);
	int send_val = send(conn_socket, msg, len,0);
   
	// send() returneaza numarul de octeti trimisi
	if(send_val == -1){
		perror("Eroare la trimiterea cererii:");
		return 0;
	}
	printf("Cererea a fost trimisa catre serverul web!\n");
	//vom folosi while pentru a primi bucati din pagina (octeti)
	//declaram buffer-ul
	//Daca pachetul este mai mic decat 1KO, se poate trimite intreg mesajul fara sa se piarda continut => in buffer vom avea maxim  1024 octeti
	char buffer[1024];
	// daca returneaza 0, serverul a inchis conexiunea
	int recv_val = 1;
	bool eroare = false;
	while(recv_val > 0){
		//in recv_val avem nr de  octeti primiti de la server
		//in buffer se pun informatiile citite
		recv_val=recv(conn_socket,buffer,sizeof buffer,0);
		if(recv_val == -1) {
			perror("Eroare la primirea octetilor de la server:");
			eroare = true;
			break;
		}else if(recv_val==0){
			break;
		}else{
			//printf("%d\n",recv_val);
		        //printf("%.*s\n", recv_val, buffer);
			//am scris in fisier continutul buffer-ului
			
			int err_write = fwrite(buffer,recv_val,1,fptr);
			if(err_write !=1){
				perror("Eroare fwrite");
			}
		}		
	}  
	//inchidem fisierul
	fclose(fptr);
	//inchidem socket-ul
	int close_socket = close(conn_socket);
	if(close_socket != 0){
		perror("Eroare close");
	}
        printf("Serverul Ipv6 a inchis conexiune\n");
	if(eroare == true){
		printf("Eroare!\n");
		return 0;
	}else{
		return 1;
	}		
}
