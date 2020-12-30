#include<stdio.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<unistd.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<string.h>

#include<netdb.h>

struct propozycja {
	char nick[16];
	char adres[32];
};

int main(int argc, char **argv) {
	int sockfd;
	struct sockaddr_in addr;
	int portNumber;
	struct propozycja prop;
	ssize_t bytes;

	/* Obsluga bledow */
	if(argc != 2) {
		printf("Niepoprawna liczba argumentow programu\n");
		exit(1);
	}

	/* Alokacja pamieci dla struktury */
	/*prop = malloc(sizeof(struct propozycja)); */
	/*prop->nick = malloc(16 * sizeof(char));
	prop->adres = malloc(32 * sizeof(char));
*/
	/* Unikalny port dla serwera */
	portNumber = 6767;

	/* Przygotowanie struktury sockaddr_in */
	addr.sin_family = AF_INET; /* IP v4 */
	inet_aton(argv[1], &addr.sin_addr);
	/*addr.sin_addr.s_addr = inet_addr(argv[1]); */ /* Adres */
	addr.sin_port = htons(portNumber); /* Number portu */
	
	/* Otwieranie gniazda */
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd == -1) {
		printf("Blad w funkcji socket\n");
		exit(1);
	}
	
	/* Powiazanie gniazda z adresem protokolowym */
	/*bind(sockfd, (struct sockaddr*) &addr, sizeof(addr));

	printf("Utworzone gniazdo, slucham na porcie %d\n", portNumber);
	*/

	strcpy(prop.nick, "Szymon");
	strcpy(prop.adres, "158.75.112.121");
	 
	bytes = sendto(sockfd, &prop, sizeof(prop), 0, 
			(struct sockaddr*)&addr, sizeof(addr)
			);

	if(bytes == 0) {
		printf("Cos poszlo nie tak...\n");
	}
	else {
		printf("Wyslano %d bajtow\n", bytes);
	}
	

	/* Busy waiting */
	/*while(8) {
		if((recvfrom(sockfd, &wiadomosc, sizeof(wiadomosc), 0, NULL, NULL)) == -1) {
			printf("Blad resvfrom\n");
			exit(1);
		}
		printf("Wiadomosc od %s: %s\n", wiadomosc.name, wiadomosc.msg);
	}*/

	close(sockfd);

	return 0;
}
