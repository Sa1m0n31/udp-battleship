#include<sys/types.h>
#include<sys/socket.h>
#include<stdio.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<signal.h>
#include<string.h>

#include<sys/ipc.h>
#include<sys/shm.h>

/* Kod na podstawie przykladu ,,A. Mroz - zad. na SK, do modyfikacji'' */
/* bez pelnej obslugi bledow! */

int shmid;
key_t key;

int sockfd;
struct sockaddr_in server_addr;
ssize_t bytes;
struct gracz *ja;
struct gracz *przeciwnik;
int my_port = 6767;
char tmp[10];

struct strzal s, sPrzeciwnik;
char decision;
int pid, pid2;

struct my_msg{
    char name[16];
    char text[255];
};

struct gracz {
	char nick[16];
	char jm1[4];
	char jm2[4];
	char dm1[4];
	char dm2[4];
	char komunikat[32];
	short kolejka;
	char strzal[10];
};

struct propozycja {
	char nick[16];
	char adres[32];
};

struct strzal {
	char strzal[10];
};

void endClient() {
	if(shmctl(shmid, IPC_RMID, 0) != 0) {
		printf("Problem z usunieciem pamieci dzielonej\n");
		exit(1);
	}

	exit(0);

}

void endServer() {
	if(shmctl(shmid, IPC_RMID, 0) != 0) {
		printf("Problem z usunieciem pamieci dzielonej\n");
		exit(1);
	}

	exit(0);

}

void start(struct gracz *ja, char *host) {
	/* przygotowanie adresu serwera */
	server_addr.sin_family = AF_INET; /* IPv4 */
	inet_aton(host,&server_addr.sin_addr); /* 1. argument = adres IP serwera */
	server_addr.sin_port = htons(my_port); /* port 6767 */
	
	/* towrze gniazdo - na razie tylko czesc "protokol" */
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	
	/* Ustalenie polozenia okretow */
	while(8) {
		if(strcmp(ja->komunikat, "Nowa") == 0) {
			printf("Czesc %s, podaj polozenie swoich okretow:\n1. Jednomasztowiec:", ja->nick);
			fgets(ja->jm1, 4, stdin);
			ja->jm1[strlen(ja->jm1)-1] = '\0';
			printf("2. Jednomasztowiec: ");
			fgets(ja->jm2, 4, stdin);
			ja->jm2[strlen(ja->jm2)-1] = '\0';
			printf("3. Dwumasztowiec: ");
			fgets(tmp, 8, stdin);
			strncpy(ja->dm1, &tmp[strlen(tmp)-3], 2);
			strncpy(ja->dm2, tmp, 2);
	
			printf("Polozenie Twoich okretow:\nJednomasztowiecA: %s\nJednomasztowiecB: %s\nDwumasztowiec: %s %s\n", ja->jm1, ja->jm2, ja->dm1, ja->dm2);		
			break;
		}
	}
	
	/* Wysylanie strzalow, pierwszy strzal - zaproszenie */
	if(ja->kolejka != 1) {
		ja->kolejka = 1;
	}
	else {
		ja->kolejka = 2;
	}
	
	while(8) {
		bytes = sendto(sockfd, &ja, sizeof(ja), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
		
		/* Czekanie na start gry */
		while(7) {
			if(strcmp(ja->komunikat, "Tak") == 0) {
				strncpy(ja->komunikat, "Nie", 64);
				break;
			}
		}
		
		printf("Propozycja przyjeta. Prosze podac pole do strzalu: ");
		fgets(ja->strzal, 10, stdin);
		ja->strzal[strlen(ja->strzal)-1] = '\0';
		
		if(strcmp(ja->strzal, "<koniec>") == 0) {
			printf("Zakonczyles gre\n");
			strncpy(ja->strzal, "KK", 3);
			bytes = sendto(sockfd, &ja, sizeof(ja), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
			shmdt(ja);
			kill(getppid(), SIGINT);
			kill(getpid(), SIGINT);
			break;
		}
	}

	start(ja, host);
}

void startServer(struct gracz *ja) {
	/* Nasluchujemy na strzaly przeciwnika */
	while(8) {
		recvfrom(sockfd, &ja, sizeof(ja), 0, NULL, NULL);
		printf("Przeciwnik zweryfikowany\n");

		if((ja->kolejka == 1)||(ja->kolejka == 2)) {
			printf("Kolejka: %d\n", ja->kolejka);
			if(ja->kolejka == 1) {
				strncpy(ja->komunikat, "Tak", 64);
			}
			ja->kolejka = 0;
		}
		else if(strcmp(ja->strzal, "NN") == 0) {
			printf("NN\n");
		}
		else if(strcmp(ja->strzal, "KK") == 0) {
			strncpy(ja->komunikat, "Nie", 64);
			decision = 'z';
			printf("Przeciwnik zakonczyl gre. Czy chcesz ustawic nowa plansze? (t/n)\n");
			scanf("%c", &decision);
			if(decision == 'n') {
				/* Koniec gry */
				kill(pid, SIGINT); /* Zabijanie dziecka */
				exit(0);
			}
			else if(decision == 't') {
				strncpy(ja->komunikat, "Nowa",  64);
				break; /* Idziemy na koniec funkcji */
			}
		}
		else {
			printf("Przeciwnik strzelil: %s\n", ja->strzal);
			printf("Weryfikacja...\n");
			
			strncpy(ja->komunikat, "Tak", 64);
		}
	}

	/* Skoro tu doszlismy to jeszcze jedna gra */
	startServer(ja);
}

int main(int argc, char *argv[]) {

	ja = (struct gracz*)malloc(sizeof(struct gracz));
	przeciwnik = (struct gracz*)malloc(sizeof(struct gracz));

	/* Forkowanie */
	if((pid = fork()) < 0) {
		printf("Blad funkcji fork\n");
		exit(1);
	}
	else if(pid == 0) {
		/* PROCES POTOMNY - KLIENT */

		/* Obsluga przerwania */
		signal(SIGINT, endClient);
		
		/* Przygotowanie kolejki komunikatow */
		if((key = ftok("plik", 'R')) == -1) {
			printf("Blad funkcji ftok\n");
			exit(1);
		}

		if((shmid = shmget(key, 64, 0644 | IPC_CREAT)) == -1) {
			printf("Blad funkcji shmget\n");
			exit(1);
		}

		ja = shmat(shmid, (void*)0, 0);
		if(ja == (struct gracz*)(-1)) {
			printf("Blad funkcji shmat\n");
			exit(1);
		}

		/* Obsluga bledow */
		if((argc != 2)&&(argc != 3)) {
			printf("Podaj jeden lub dwa argumenty programu!\n");
			exit(1);
		}
		else if(argc == 2) {
			/* Nie podano nicku - ustawienie domyslnego nicku */
			strcpy(ja->nick, "NN");
		}
		else {
			/* Podano nick - ustawienie nicku podanego jako drugi argument */
			strcpy(ja->nick, argv[2]);
		}
		
		/* GLOWNA FUNKCJA KLIENTA - WYSYLANIE */
		start(ja, argv[1]);

		shmdt(ja);
	}
	else {
		/* PROCES MACIERZYSTY - SERWER */

		/* Obsluga przerwania */
		signal(SIGINT, endServer);

		/* Przygotowanie kolejki komunikatow */
		if((key = ftok("plik", 'R')) == -1) {
			printf("Blad funkcji ftok\n");
			exit(1);
		}

		if((shmid = shmget(key, 64, 0644 | IPC_CREAT)) == -1) {
			printf("Blad funkcji shmget\n");
			exit(1);
		}

		ja = shmat(shmid, (void*)0, 0);
		if(ja == (struct gracz*)(-1)) {
			printf("Blad funkcji shmat\n");
			exit(1);
		}
		
		/* tworze gniazdo - na razie tylko czesc "protokol" */
		sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	
		/* podpinam gniazdo pod  konkretny "adres-lokalny" 
		   i "proces-lokalny" (= port) */
		server_addr.sin_family	    = AF_INET;           /* IPv4 */
		server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		server_addr.sin_port        = htons(my_port);    /* moj port */
		
		bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
		
		strncpy(ja->komunikat, "Nowa", 64);

		/* GLOWNA FUNKCJE SERWERA - ODBIERANIE */
		startServer(ja);
		
		shmdt(ja);
	}

	free(ja);
	close(sockfd);
	return 0;
}
