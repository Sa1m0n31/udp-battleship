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
	char jm1[4];
	char jm2[4];
	char dm1[4];
	char dm2[4];
};

struct Strzal {
	char strzal[16];
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
char nickPrzeciwnika[16];

struct Strzal strzal, mojStrzal;
char decision;
int pid;
short first = 1, trafienie, zatopioneStatki = 0;
struct Info *info;
short jm1Zatopiony = 0, jm2Zatopiony = 0, dm1Zatopiony = 0, dm2Zatopiony = 0;

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

/* Sprawdzanie trafienia */
short sprawdzTrafienie(char strzal[]) {
	if(strcmp(strzal, info->jm1) == 0) {
		if(jm1Zatopiony == 0) zatopioneStatki++;
		jm1Zatopiony = 1;
		return 1;
	}
	else if(strcmp(strzal, info->jm2) == 0) {
		if(jm2Zatopiony == 0) zatopioneStatki++;
		jm2Zatopiony = 1;
		return 1;
	}
	else if(strcmp(strzal, info->dm1) == 0) {
		if(dm1Zatopiony == 0) zatopioneStatki++;
		dm1Zatopiony = 1;
		return 2;
	}
	else if(strcmp(strzal, info->dm2) == 0) {
		if(dm2Zatopiony == 0) zatopioneStatki++;
		dm2Zatopiony = 1;
		return 2;
	}
	else {
		return 0;
	}
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
		
		/* Kopiowanie danych do struktury info */
		strncpy(info->jm1, ja->jm1, 4);
		strncpy(info->jm2, ja->jm2, 4);
		strncpy(info->dm1, ja->dm1, 4);
		strncpy(info->dm2, ja->dm2, 4);

		/* Wyslanie pierwszego strzalu - zaproszenia, w pierwszym strzale wysylamy swoj nick */
		strncpy(mojStrzal.strzal, ja->nick, 16);
		bytes = sendto(sockfd, &mojStrzal, sizeof(mojStrzal), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
		
		/* Nasluchiwanie na wpisanie strzalu */
		while(8) {
			if(info->twojaKolejka == 0) continue;
			if(info->twojaKolejka == -1) {
				/* Trafiony dwumasztowiec - pusty strzal */
				strncpy(mojStrzal.strzal, "Z1", 4);
				bytes = sendto(sockfd, &mojStrzal, sizeof(mojStrzal), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
				info->twojaKolejka = 0;
				continue;
			}
			else if(info->twojaKolejka == -2) {
				/* Trafiony dwumasztowiec - pusty strzal */
				strncpy(mojStrzal.strzal, "Z2", 4);
				bytes = sendto(sockfd, &mojStrzal, sizeof(mojStrzal), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
				info->twojaKolejka = 0;
				continue;
			}
			else if(info->twojaKolejka == 3) {
				break;
			}
			fgets(mojStrzal.strzal, 16, stdin);
			mojStrzal.strzal[strlen(mojStrzal.strzal)-1] = '\0';
			info->twojaKolejka = 0;
			if(strcmp(mojStrzal.strzal, "<koniec>") == 0) {
				printf("Zakonczyles gre\n");
				strncpy(mojStrzal.strzal, "KK", 4);
				bytes = sendto(sockfd, &mojStrzal, sizeof(mojStrzal), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
				info->twojaKolejka = 3;
				break;
			}
			else {
				bytes = sendto(sockfd, &mojStrzal, sizeof(mojStrzal), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
			}
		}

		endClient();
		
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
				printf("[%s [%s] dolaczyl do gry", strzal.strzal, argv[1]);
				strncpy(nickPrzeciwnika, strzal.strzal, 16);
				first = 0;
				if(info->okretyUstawione != 0) {
					printf(", podaj pole do strzalu]\n");
					info->twojaKolejka = 1;
				}
			}
			else {
				if(strcmp(strzal.strzal, "Z1") == 0) {
					printf("[%s (%s): trafiles jednomasztowiec! Podaj kolejne pole do strzalu]", nickPrzeciwnika, argv[1]);
					info->twojaKolejka = 1;
				}
				else if(strcmp(strzal.strzal, "Z2") == 0) {
					printf("[%s (%s): trafiles dwumasztowiec! Podaj kolejne pole do strzalu]", nickPrzeciwnika, argv[1]);
					info->twojaKolejka = 1;
				}
				else if(strcmp(strzal.strzal, "KK") == 0) {
					printf("[%s (%s) zakonczyl gre\n", nickPrzeciwnika, argv[1]);
					strncpy(mojStrzal.strzal, "MK", 4);
					bytes = sendto(sockfd, &mojStrzal, sizeof(mojStrzal), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
					info->twojaKolejka = 3;
					break;
				}
				else if(strcmp(strzal.strzal, "MK") == 0) {
					info->twojaKolejka = 3;
					break;
				}
				else {
					printf("Trafienie: %s\n", strzal.strzal);
					trafienie = sprawdzTrafienie(strzal.strzal);
					if(trafienie == 0) {
						printf("[%s [%s] strzelil: %s - pudlo. Podaj pole do strzalu]", nickPrzeciwnika, argv[1], strzal.strzal);
						info->twojaKolejka = 1;
					}
					else if(trafienie == 1) {
						printf("[%s [%s] strzelil %s - trafiony jednomasztowiec]", nickPrzeciwnika, argv[1], strzal.strzal);
						info->twojaKolejka = -1;
					}
					else if(trafienie == 2) {
						printf("[%s [%s] strzelil %s - trafiony dwumasztowiec]", nickPrzeciwnika, argv[1], strzal.strzal);
						info->twojaKolejka = -2;
					}
				}
			}
			printf("\n");
		}

		endServer();
	}
	
	return 0;
}
