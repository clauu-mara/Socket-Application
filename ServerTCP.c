#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include "ClientTCP.h"

#define PORT "22111" 

int main(void)
{	
	struct addrinfo ServerAddress;
	struct addrinfo *servinfo;
	int status;
	//ne asiguram ca structura e goala
	memset(&ServerAddress, 0, sizeof ServerAddress);
	ServerAddress.ai_family = AF_INET;
	ServerAddress.ai_socktype = SOCK_STREAM;
	// daca AI_PASSIVE este setat si primul parametru al addrinfo() este NULL, atunci socketul returnat va contine
	// adresa wildcard: INADDR_ANY pt Ipv4
	ServerAddress.ai_flags = AI_PASSIVE;
	//ServerAddress pointeaza la o structura addrinfo umpluta deja cu informatii
	//servinfo va pointa la o lista ordonata de structuri addrinfo, fiecare continand la randul ei cate o structura sockaddr
	status = getaddrinfo(NULL, PORT, &ServerAddress, &servinfo);
	//getaddeinfo() returneaza valoare diferita de 0 in caz de eroare, pe care o putem afisa folosind gai_strerror()
	if(status !=0){
		perror("Eroare getaddrinfo:");
		return 1;
	}
	//creare punct de comunicare - socket
	int conn_socket = socket(servinfo->ai_family, servinfo->ai_socktype, 0);
	if(conn_socket == -1){
		perror("Eroare la crearea socketului:");
		return 1;
	}
	printf("Socket creat cu succes!\n");
	// dupa crearea socketului, este necesara conectarea acestuia cu un port pe masina; pe acel port vom asculta ulterior cu listen()
	// pentru a putea reutiliza portul in cazul in care un socket ramane agatat
	int yes = 1;
	if(setsockopt(conn_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1)
	{
		perror("setsockopt");
		exit(1);
	}
	int bind_test = bind(conn_socket, servinfo->ai_addr, servinfo->ai_addrlen);
	//bind() returneaza -1 in caz de eroare
	if(bind_test == -1){
		perror("Bind esuat:");
		return 1;
	}
	printf("Bind realizat cu succes!\n");
	//curatam lista ordonata 
	freeaddrinfo(servinfo);
	//punem serverul intr-o stare pasiva astfel incat sa astepte cereri
	//specificam nr. de conexiuni care pot intra in coada noastra de asteptare pana cand le acceptam
	int listen_test = listen(conn_socket,5);
	if(listen_test == -1){
		perror("Listen esuat");
		return 1;
	}
	printf("Asteptam conexiuni...\n");
	
	struct sockaddr_storage IncommingClient;
	socklen_t address_size;
	
	//bufferul in care vom pune datele primite de la client
	char buffer[1024];
	int new_socket;
	int recv_serv;
	char *mesaj="";
	
	while(1){		
		// serverul accepta stabilirea unei conexiuni cu un client
		// accept() va returna un nou socket care va fi folosit doar pentru acea conexiune acceptata
		// al doilea parametru din accept() va fi un pointer la o structura locala sockaddr_storage in care se vor salva informatii despre clientul conectat	
		address_size = sizeof IncommingClient;
		new_socket = accept(conn_socket, (struct sockaddr *)&IncommingClient,&address_size);
	
		if(new_socket == -1){
			perror("Conexiune esuata:");
			break;
		}
		//pentru a vedea cine s-a conectat la server
		char address[NI_MAXHOST];
		char port[NI_MAXSERV];

		int client_conectat = getnameinfo((struct sockaddr *)&IncommingClient, address_size, address, sizeof(address), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV);
		if(client_conectat  == 0){
			printf("%d", sizeof(address));
			printf("Conexiune noua de la %s:%s\n", address, port);
		}
		
		if(fork()==0){ //se va crea procesul copil care va rula in acelasi timp
		    //in recv() vom avea nr de octeti primiti de la client
		    //in buffer se pun informatiile citite
		    int i;
		    int recv_serv = 1;
		    int send_val;
	
		    // punem recv_val in while pentru ca un client poate trimite mai multe cereri
		   
                    int close_parinte =	close(conn_socket); //copilul nu utilizeaza scoket-ul parinte
		    if(close_parinte != 0){
		    	perror("Eroare close");
		    }
		    
		    while(recv_serv > 0){			
				recv_serv = recv(new_socket, buffer, sizeof buffer, 0);
				if(recv_serv == -1){
					perror("Eroare recv():");
					break;
				}
				if(recv_serv == 0){
					break;
				}			
				char comanda_curenta[4];
				for(int i=0; i< recv_serv; i++){
					if(buffer[i] == '#' && isdigit(buffer[i-1]) && isdigit(buffer[i-2]) && i>=2){
						strncpy(comanda_curenta,buffer+i-2, 3);
						printf("Recv:S-a primit comanda - %.*s\n",recv_serv, comanda_curenta);			
						break;
					}
				}
				
				if(strncmp("20#", comanda_curenta,3)==0){
					i = Client_Function();
					//daca clientul Ipv6 a primit pagina de la serverul IPv6
					if(i==1){					
						char date[1024];			
						int recv_page;
						//preluam din fisier cate 1024 octeti
					        FILE *fptr;
						//deschidem fisierul creat in urma cererii clientului Ipv6 la serverul Ipv6
						fptr = fopen("index.html", "r");
						if(fptr == NULL){
							perror("Eroare deschidere fisier pentru citire!");
							break;
						}

						while(!feof(fptr)){
							recv_page = fread(date, 1, sizeof date, fptr);
						       	send_val = send(new_socket, date, recv_page, 0);
							if(send_val == -1){
								perror("Eroare send()");
								break;
							}				
						}						
						fclose(fptr);
						
					}else{
						mesaj = "Pagina nu a fost preluata!\n";
						send_val = send(new_socket, mesaj, strlen(mesaj),0);
						if(send_val == -1){
							perror("Eroare la trimiterea raspunsului catre client:");
							break;
						}
						break;
					}
				
				}else{
					mesaj = "Recv:Comanda neimplementata!\n";
					send_val = send(new_socket, mesaj, strlen(mesaj), 0);
					
					if(send_val == -1){
						perror("Eroare la trimiterea raspunsului catre client:");
					        break;
					}
				}		 			
				
		    }
			
		    int close_copil = close(new_socket);
		    if(close_copil != 0){
	            	perror("Eroare close");
			break;
		    }
		    printf("Serverul a inchis conexiunea cu clientul: %s:%s\n", address, port);
		    exit(1);
		}else if(fork()==-1){
			close(new_socket);
			continue;
		}
		int close_c = close(new_socket);
		if(close_c != 0){
			perror("Eroare close");
			break;
		}
	
	}	
	int close_p = close(conn_socket);
	if(close_p !=0 ){
		perror("Eroare close");
	}
	
	return 0;
}
