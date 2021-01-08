#define _XOPEN_SOURCE 600

#include<sys/shm.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<stdio.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<signal.h>
#include<string.h>
#include<netdb.h>
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
	char decision;
	char plansza[16];
};

struct Strzal {
	char strzal[16];
};

/* Zmienne */
int shmid;
key_t key;

int sockfd;
/*struct sockaddr_in server_addr; */
ssize_t bytes;
struct Gracz *ja;
int my_port = 6767;
char tmp[10];
char nickPrzeciwnika[16];

struct Strzal strzal, mojStrzal;
char decision;
int pid;
short first = 1, trafienie, zatopioneStatki = 0, trafioneStatki = 0;
struct Info *info;
short jm1Zatopiony = 0, jm2Zatopiony = 0, dm1Zatopiony = 0, dm2Zatopiony = 0;

/* GETADDRINFO */
struct addrinfo hints;
struct addrinfo *result, *rp;
int s, end;

char decision;

/* End */
void endClient() {
	close(sockfd);
	freeaddrinfo(result);
	
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
	/* Sprawdz czy celny */
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
	return 0;
}

/* Zapisanie mojego strzalu */
void odnotujStrzal(char *strzal, short trafiony) {
	int n;
	
	/* Zarejestruj strzal na planszy */
	if(strcmp(strzal, "a1") == 0) n = 0;
	else if(strcmp(strzal, "a2") == 0) n =1;
	else if(strcmp(strzal, "a3") == 0) n =2;
	else if(strcmp(strzal, "a4") == 0) n =3;
	else if(strcmp(strzal, "b1") == 0) n =4;
	else if(strcmp(strzal, "b2") == 0) n =5;
	else if(strcmp(strzal, "b3") == 0) n =6;
	else if(strcmp(strzal, "b4") == 0) n =7;
	else if(strcmp(strzal, "c1") == 0) n =8;
	else if(strcmp(strzal, "c2") == 0) n =9;
	else if(strcmp(strzal, "c3") == 0) n =10;
	else if(strcmp(strzal, "c4") == 0) n =11;
	else if(strcmp(strzal, "d1") == 0) n =12;
	else if(strcmp(strzal, "d2") == 0) n =13;
	else if(strcmp(strzal, "d3") == 0) n =14;
	else if(strcmp(strzal, "d4") == 0) n =15;

	if(trafiony == 0) info->plansza[n] = 'X';
	else {
		info->plansza[n] = 'Z';
		trafioneStatki++;
	}
}

void startClient(char *host) {
	/* Zerowanie planszy */
	int i;
	for(i=0; i<16; i++) {
		info->plansza[i] = ' ';
	}

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
	bytes = sendto(sockfd, &mojStrzal, sizeof(mojStrzal), 0, NULL, 0);
	printf("[Wyslano propozycje gry do %s]\n", host);
}

void wypiszPlansze() {
	printf(".\t1\t2\t3\t4\n");
	printf("A\t%c\t%c\t%c\t%c\n", info->plansza[0], info->plansza[1], info->plansza[2], info->plansza[3]);
	printf("B\t%c\t%c\t%c\t%c\n", info->plansza[4], info->plansza[5], info->plansza[6], info->plansza[7]);
	printf("C\t%c\t%c\t%c\t%c\n", info->plansza[8], info->plansza[9], info->plansza[10], info->plansza[11]);
	printf("D\t%c\t%c\t%c\t%c\n", info->plansza[12], info->plansza[13], info->plansza[14], info->plansza[15]);
}

/* Program */
int main(int argc, char **argv) {

	ja = (struct Gracz*)malloc(sizeof(struct Gracz));
	info = (struct Info*)malloc(sizeof(struct Info));
	
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
	info->decision = 'a';
	
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
		
		/* GETADDRINFO */
		memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_INET;    
        hints.ai_socktype = SOCK_DGRAM; 
        hints.ai_flags = 0;
        hints.ai_protocol = 0;          

        s = getaddrinfo(argv[1], "6767", &hints, &result);
        if (s != 0) {
           fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
           exit(EXIT_FAILURE);
        }

		for (rp = result; rp != NULL; rp = rp->ai_next) {
           	sockfd = socket(rp->ai_family, rp->ai_socktype,
                        rp->ai_protocol);
           	if (sockfd == -1) {
           		continue;
			}

           	if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1) {
           		break;
			}             

           	close(sockfd);
         }

         if (rp == NULL) {              
           fprintf(stderr, "Could not connect\n");
           exit(EXIT_FAILURE);
        }
		
		/* Ustawienie okretow i wyslanie zaproszenia do przeciwnika */
		startClient(argv[1]);
		
		/* Nasluchiwanie na wpisanie strzalu */
		while(8) {
			if(info->twojaKolejka == 0) continue;
			if(info->twojaKolejka == -1) {
				/* Trafiony dwumasztowiec - pusty strzal */
				strncpy(mojStrzal.strzal, "Z1", 4);
				bytes = sendto(sockfd, &mojStrzal, sizeof(mojStrzal), 0, NULL, 0);
				info->twojaKolejka = 0;
				continue;
			}
			else if(info->twojaKolejka == -2) {
				/* Trafiony dwumasztowiec - pusty strzal */
				strncpy(mojStrzal.strzal, "Z2", 4);
				bytes = sendto(sockfd, &mojStrzal, sizeof(mojStrzal), 0, NULL, 0);
				info->twojaKolejka = 0;
				continue;
			}
			else if((info->twojaKolejka == 3)||(info->twojaKolejka == 4)) {
				/* 3 - woj przeciwnik zakonczyl gre */
				if(info->twojaKolejka == 3) printf("[Twoj przeciwnik zakonczyl gre. Ustawic nowa plansze? (t/n)]\n");
				/* 4 - przegrales */
				if(info->twojaKolejka == 4) printf("[Przegrales. Ustawic nowa plansze? (t/n)]\n");

				scanf("%c", &decision);
				info->decision = decision;
				if(decision == 't') {
					info->twojaKolejka = 0;
					startClient(argv[1]);
					continue;
				}
				else {
					kill(getppid(), SIGINT); /* Wysylanie sygnalu do rodzica */
					break;
				}
			}
			else if(info->twojaKolejka == 2) {
				/* Trafiles przeciwnika - masz kolejny strzal */
				odnotujStrzal(mojStrzal.strzal, 1); /* Zapisz poprzedni strzal jako trafiony */
				printf("Trafione strzaly: %d\n", trafioneStatki);
				if(trafioneStatki == 4) {
					printf("Wygrales!\n");
					break;
				}
			}
			fgets(mojStrzal.strzal, 16, stdin);
			mojStrzal.strzal[strlen(mojStrzal.strzal)-1] = '\0';
			info->twojaKolejka = 0;
			if(strcmp(mojStrzal.strzal, "<koniec>") == 0) {
				/* Ty zakonczyles gre */
				printf("Zakonczyles gre\n");
				strncpy(mojStrzal.strzal, "KK", 4);
				bytes = sendto(sockfd, &mojStrzal, sizeof(mojStrzal), 0, NULL, 0);
				kill(getppid(), SIGINT); /* Wysylanie sygnalu do rodzica */
				break;
			}
			else if(strcmp(mojStrzal.strzal, "wypisz") == 0) {
				/* Wypisywanie planszy */
				wypiszPlansze();
				info->twojaKolejka = 1;
				continue;
			}
			else {
				/* Wyslij strzal */
				odnotujStrzal(mojStrzal.strzal, 0);
				bytes = sendto(sockfd, &mojStrzal, sizeof(mojStrzal), 0, NULL, 0);
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
		
		memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_INET;    
        hints.ai_socktype = SOCK_DGRAM; 
        hints.ai_flags = AI_PASSIVE;   
        hints.ai_protocol = 0;        
        hints.ai_canonname = NULL;
        hints.ai_addr = NULL;
        hints.ai_next = NULL;

        s = getaddrinfo((const char*)NULL, "6767", &hints, &result);
        if (s != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
            exit(EXIT_FAILURE);
           }


        for (rp = result; rp != NULL; rp = rp->ai_next) {
               sockfd = socket(rp->ai_family, rp->ai_socktype,
                       rp->ai_protocol);
               if (sockfd == -1) {
               	continue;
			   }

               if (bind(sockfd, rp->ai_addr, rp->ai_addrlen) == 0) {
               	break;
			   }

               close(sockfd);
           }

		freeaddrinfo(result);
		
		/* Nasluchiwanie na strzaly */
		while(8) {
			if(first == 1) printf("Rozpoczynam nasluchiwanie na zaproszenie przeciwnika\n");
			printf("---Czekam na strzal---\n");
			recvfrom(sockfd, &strzal, sizeof(strzal), 0, NULL, NULL);
			if(first == 1) {
				/* Pierwszy strzal - zaproszenie do gry */
				strncpy(nickPrzeciwnika, strzal.strzal, 16);
				first = 0;
				if(info->okretyUstawione != 0) {
					printf("[%s [%s] dolaczyl do gry, podaj pole do strzalu]", strzal.strzal, argv[1]);
					info->twojaKolejka = 1;
				}
			}
			else {
				/* Normalna gra */
				if(strcmp(strzal.strzal, "Z1") == 0) {
					printf("[%s (%s): trafiles jednomasztowiec! Podaj kolejne pole do strzalu]", nickPrzeciwnika, argv[1]);
					info->twojaKolejka = 2;
					if(trafioneStatki == 4) {
						printf("Wygrales!\n");
						info->twojaKolejka = 3;
						break;
					}
				}
				else if(strcmp(strzal.strzal, "Z2") == 0) {
					printf("[%s (%s): trafiles dwumasztowiec! Podaj kolejne pole do strzalu]", nickPrzeciwnika, argv[1]);
					info->twojaKolejka = 2;
					if(trafioneStatki == 4) {
						printf("Wygranes!\n");
						info->twojaKolejka = 3;
						break;
					}
				}
				else if(strcmp(strzal.strzal, "KK") == 0) {
					printf("[%s (%s) zakonczyl gre\n", nickPrzeciwnika, argv[1]);
					strncpy(mojStrzal.strzal, "MK", 4);
					bytes = sendto(sockfd, &mojStrzal, sizeof(mojStrzal), 0, NULL, 0);
					info->twojaKolejka = 3;
					
					/* Czekamy na decyzje - konczymy czy gramy dalej */
					while(7) {
						if(info->decision == 't') {
							first = 1;
							end = 0;
							break;
						}
						else if(info->decision == 'n') {
							end = 1;
							break;
						}
					}
					if(end == 0) continue;
					else break;

				}
				else if(strcmp(strzal.strzal, "WW") == 0) {
					printf("WYGRALES!\n");
					info->twojaKolejka = 3;
					break;
				}
				else {
					/* Normalny strzal przeciwnika */
					trafienie = sprawdzTrafienie(strzal.strzal);

					if(zatopioneStatki != 4) {
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
					else {
						/* zatopioneStatki = 4 ---> przegrales */
						strncpy(strzal.strzal, "WW", 4);
						bytes = sendto(sockfd, &strzal, sizeof(strzal), 0, NULL, 0);
						info->twojaKolejka = 4;
						
						/* Czekamy na decyzje - konczymy czy gramy dalej */
						while(7) {
							if(info->decision == 't') {
								first = 1;
								end = 0;
								break;
							}
							else if(info->decision == 'n') {
								end = 1;
								break;
							}
						}
						if(end == 0) continue;
						else break;

					}

				}
			}
			printf("\n");
		}

		endServer();
	}
	
	return 0;
}
