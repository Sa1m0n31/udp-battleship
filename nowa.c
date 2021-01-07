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

/* Struktury */
struct Gracz {
	char nick[16];
	char jm1[4];
	char jm2[4];
	char dm1[4];
	char dm2[4];
};

struct Info {
	short twojaKolejka;
	short okretyUstawione;
};

struct Strzal {
	char strzal[4];
};

/* Zmienne */
int shmid;
key_t key;

int sockfd;
struct sockaddr_in server_addr;
ssize_t bytes;
struct Gracz *ja;
int my_port = 6767;
char tmp[10];

struct Strzal strzal, mojStrzal;
char decision;
int pid;
short first = 1;
struct Info *info;

/* End */
void endClient() {
	if(shmctl(shmid, IPC_RMID, 0) != 0) {
		printf("Problem z usunieciem pamieci dzielonej\n");
		exit(1);
	}
	printf("Koncze\n");
	exit(0);
}

void endServer() {
	if(shmctl(shmid, IPC_RMID, 0) != 0) {
		printf("Problem z usunieciem pamieci dzielonej\n");
		exit(1);
	}
	printf("Koncze\n");
	exit(0);
}

/* Program */
int main(int argc, char **argv) {

	ja = (struct Gracz*)malloc(sizeof(struct Gracz));
	info = (struct Info*)malloc(sizeof(struct Info));
	/*strzal = (struct Strzal)malloc(sizeof(struct Strzal));
	mojStrzal = (struct Strzal)malloc(sizeof(struct Strzal)); */
	
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

	info->okretyUstawione = 0;
	info->twojaKolejka = 0;
	
	/* Forkowanie */
	if((pid = fork()) < 0) {
		printf("Blad funkcji fork\n");
		exit(1);
	}
	else if(pid == 0) {
		/* PROCES POTOMNY - wpisywanie strzalow */
		
		/* Obsluga sygnalu przerwania */
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

		info = shmat(shmid, (void*)0, 0);
		if(info == (struct Info*)(-1)) {
			printf("Blad funkcji shmat\n");
			exit(1);
		}

		/* Przygotowanie do polaczenia */
		server_addr.sin_family = AF_INET;
		inet_aton(argv[1], &server_addr.sin_addr);
		server_addr.sin_port = htons(my_port);

		sockfd = socket(AF_INET, SOCK_DGRAM, 0);
		
		/* Ustalenie polozenia okretow */
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
		info->okretyUstawione = 1;
		
		/* Wyslanie pierwszego strzalu - zaproszenia */
		strncpy(mojStrzal.strzal, "SS", 4);
		bytes = sendto(sockfd, &mojStrzal, sizeof(mojStrzal), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
		
		/* Nasluchiwanie na wpisanie strzalu */
		while(8) {
			if(info->twojaKolejka == 0) continue; 
			fgets(mojStrzal.strzal, 4, stdin);
			mojStrzal.strzal[strlen(mojStrzal.strzal)-1] = '\0';
			info->twojaKolejka = 0;
			bytes = sendto(sockfd, &mojStrzal, sizeof(mojStrzal), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
		}
		
	}
	else {
		/* PROCES MACIERZYSTY - nasluchiwanie na strzaly */
		
		/* Obsluga sygnalu przerwania */
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

		info = shmat(shmid, (void*)0, 0);
		if(info == (struct Info*)(-1)) {
			printf("Blad funkcji shmat\n");
			exit(1);
		}
		
		/* tworze gniazdo - na razie tylko czesc "protokol" */
		sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	
		/* podpinam gniazdo pod  konkretny "adres-lokalny" */
		server_addr.sin_family = AF_INET;         
		server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		server_addr.sin_port = htons(my_port);
		
		bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
		
		/* Nasluchiwanie na strzaly */
		while(8) {
			if(first == 1) printf("Rozpoczynam nasluchiwanie na zaproszenie przeciwnika\n");
			recvfrom(sockfd, &strzal, sizeof(strzal), 0, NULL, NULL);
			if(first == 1) {
				/* Pierwszy strzal - zaproszenie do gry */
				printf("Przeciwnik dolaczyl do gry\n");
				first = 0;
				if(info->okretyUstawione == 0) {
					printf("Jestes drugi w kolejce\n");
				}
				else {
					printf("Jestes pierwszy w kolejce\n");
					info->twojaKolejka = 1;
				}
			}
			else {
				printf("Przeciwnik strzelil: %s, Oddaj swoj strzal:", strzal.strzal);
				info->twojaKolejka = 1;
			}
			printf("---\n");
		}
	}
	
	return 0;
}
