#include<sys/types.h>
#include<sys/socket.h>
#include<stdio.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<string.h>

/* Kod na podstawie przykladu ,,A. Mroz - zad. na SK, do modyfikacji'' */
/* bez pelnej obslugi bledow! */

struct my_msg{
    char name[16];
    char text[255];
};

struct gracz {
	char nick[16];
	char jm1[4];
	char jm2[4];
	char dm[8];
};

struct propozycja {
	char nick[16];
	char adres[32];
};

int main(int argc, char *argv[]) {
	int sockfd;
	struct sockaddr_in server_addr;
	ssize_t bytes;
	int pid;
	int my_port = 6767;

	struct gracz ja, przeciwnik;
	struct propozycja propMy;

	/* Obsluga bledow */
	if((argc != 2)&&(argc != 3)) {
		printf("Podaj jeden lub dwa argumenty programu!\n");
		exit(1);
	}
	else if(argc == 2) {
		/* Nie podano nicku - ustawienie domyslnego nicku */
		strcpy(ja.nick, "NN");
		strcpy(propMy.nick, "NN");
	}
	else {
		/* Podano nick - ustawienie nicku podanego jako drugi argument */
		strcpy(ja.nick, argv[2]);
		strcpy(propMy.nick, argv[2]);
	}

	/* Forkowanie */
	if((pid = fork()) < 0) {
		printf("Blad funkcji fork\n");
		exit(1);
	}
	else if(pid == 0) {
		/* PROCES POTOMNY - KLIENT */
		
		/* przygotowanie adresu serwera */
		server_addr.sin_family = AF_INET; /* IPv4 */
		inet_aton(argv[1],&server_addr.sin_addr ); /* 1. argument = adres IP serwera */
		server_addr.sin_port = htons(my_port); /* port 6767 */
	
		/* towrze gniazdo - na razie tylko czesc "protokol" */
		sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	
		/* Komunikacja */
		printf("Czesc %s, podaj polozenie swoich okretow:\n1. Jednomasztowiec:", ja.nick);
		fgets(ja.jm1, 4, stdin);
		ja.jm1[strlen(ja.jm1)-1] = '\0';
		printf("2. Jednomasztowiec: ");
		fgets(ja.jm2, 4, stdin);
		ja.jm2[strlen(ja.jm2)-1] = '\0';
		printf("3. Dwumasztowiec: ");
		fgets(ja.dm, 8, stdin);
		ja.dm[strlen(ja.dm)-1] = '\0';
		
		printf("Polozenie Twoich okretow:\nJednomasztowiecA: %s\nJednomasztowiecB: %s\nDwumasztowiec: %s\n", ja.jm1, ja.jm2, ja.dm);
	
		bytes = sendto(sockfd, &ja, sizeof(ja), 0, 
					(struct sockaddr *)&server_addr, sizeof(server_addr));
		
		printf(" (%s: %zd bajtow)\n", (bytes > 0)?"OK":"cos nie tak!", bytes);
	}
	else {
		/* PROCES MACIERZYSTY - SERWER */
	
		/* tworze gniazdo - na razie tylko czesc "protokol" */
		sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	
		/* podpinam gniazdo pod  konkretny "adres-lokalny" 
		   i "proces-lokalny" (= port) */
		server_addr.sin_family	    = AF_INET;           /* IPv4 */
		server_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* dowolny (moj) interfejs */
		/*inet_addr("158.75.112.121"); //juliusz na sztywno */
		server_addr.sin_port        = htons(my_port);    /* moj port */
		
		bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
	
		printf("[Serwer]: Utworzone gniazdo, slucham na porcie: %d\n", my_port); 
	
		while(1) {
			recvfrom(sockfd, &przeciwnik, sizeof(przeciwnik), 0, NULL, NULL);
			printf("Wiadomosc od %s:\nJednomasztowiecA: %s\nJednomasztowiecB: %s\nDwumasztowiec: %s\n", przeciwnik.nick, przeciwnik.jm1, przeciwnik.jm2, przeciwnik.dm);
		}
	}

	close(sockfd);
	return 0;
}
