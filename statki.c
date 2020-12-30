#include<stdio.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>

struct zawodnik {
	char nick[16];
	char jednomasztowiec1[4];
	char jednomasztowiec2[4];
	char dwumasztowiec[8];
	char adres[20];
};

struct propozycja {
	char nick[16];
	char adres[32];
};

int main(int argc, char **argv) {
	int portNumber = 6767;
	struct sockaddr_in addr;
	struct zawodnik gracz;
	struct propozycja prop;
	int sockfd;
	ssize_t bytes;

	/* Obsluga bledow - podano wiecej lub mniej niz jeden argument */
	if((argc != 3)&&(argc != 2)) {
		printf("Blad - niepoprawna liczba argumentow programu\n");
		exit(1);
	}
	else if(argc == 2) {
		strcpy(gracz.nick, "NN");
	}
	else {
		strcpy(gracz.nick, argv[2]);
	}

	/* Tworzenie gniazda */
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd == -1) {
		printf("Blad w funkcji socket\n");
		exit(1);
	}

	/* Przygotowanie struktury sockaddr_in */
	addr.sin_family = AF_INET; /* IP v4 */
	addr.sin_addr.s_addr = htonl(INADDR_ANY); /* Adres */
	addr.sin_port = htons(portNumber); /* Port */

	bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));

	/* Komunikacja */
	printf("Rozpoczynam czat z %s\nNapisz <koniec> aby zakonczyc gre\nUstal polozenie swoich okretow:\n", argv[1]);
	
	/* Ustawienie polozenia okretow */
	printf("1. Jednomasztowiec:");
	fgets(gracz.jednomasztowiec1, 4, stdin);
	gracz.jednomasztowiec1[strlen(gracz.jednomasztowiec1)-1] = '\0';
	printf("2. Jednomasztowiec:");
	fgets(gracz.jednomasztowiec2, 4, stdin);
	gracz.jednomasztowiec2[strlen(gracz.jednomasztowiec2)-1] = '\0';
	printf("3. Dwumasztowiec:");
	fgets(gracz.dwumasztowiec, 8, stdin);
	gracz.dwumasztowiec[strlen(gracz.dwumasztowiec)-1] = '\0';

	/* Wyswietlenie polozenia okretow */
	printf("Jednomasztowiec1: %s\nJednomasztowiec2: %s\nDwumasztowiec: %s\n", gracz.jednomasztowiec1, gracz.jednomasztowiec2, gracz.dwumasztowiec); 

	/* Proces glowny zajmuje sie nasluchiwaniem informacji, proces macierzysty
	 * - wysylaniem */
	while(8) {
		recvfrom(sockfd, &prop, sizeof(prop), 0, NULL, NULL);
		printf("%s z maszyny o adresie %s dolaczyl do gry", prop.nick, prop.adres);
	}

	/* Wysylanie */
	/*bytes = sendto(sockfd,
			&gracz, sizeof(gracz), 0,
			(struct sockaddr*)&addr, sizeof(addr)			
			);

	if(bytes > 0) {
		printf("Pakiet wyslano (%d bajtow)\n", bytes);
	}
	else {
		printf("Cos poszlo nie tak\n");
	} */

	/* Zamkniecie gniazda */
	close(sockfd);

	return 0;
}
