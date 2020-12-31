#include<sys/types.h>
#include<sys/socket.h>
#include<stdio.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<string.h>

#include<sys/ipc.h>
#include<sys/shm.h>

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
	char dm1[4];
	char dm2[4];
	char komunikat[32];
	short skip;
};

struct plansza {
	
};

struct propozycja {
	char nick[16];
	char adres[32];
};

struct strzal {
	char strzal[4];
};

struct myTurn {
	int myTurn;
};

short sprawdzTrafienie(char *traf, struct gracz* ja) {
	short trafiony = 1;

	if(strcmp(traf, ja->jm1) == 0) {
		trafiony = 1;
		printf("Trafiony jednomasztowiec1\n");
	}	
	else if(strcmp(traf, ja->jm2) == 0) {

		printf("Trafiony jednomasztowiec2\n");
	}
	else if((strcmp(traf, ja->dm1) == 0)||(strcmp(traf, ja->dm2) == 0)) {
		printf("Trafiony dwumasztowiec\n");
	}
	else {
		if(strcmp(traf, "NN") != 0) {
			printf("Pudlo\n");
		}
		else {
			printf("TEST: Trzecia faza - strzal pomijany\n");
		}
		trafiony = 0;
	}

	return trafiony;
}

int main(int argc, char *argv[]) {
	int sockfd;
	struct sockaddr_in server_addr;
	ssize_t bytes;
	int pid;
	int my_port = 6767;
	char tmp[10];

	struct propozycja propMy;
	struct strzal s, sPrzeciwnik;
	struct gracz *ja, przeciwnik;

	key_t key;
	int shmid;

	/* Obsluga bledow */
	if((argc != 2)&&(argc != 3)) {
		printf("Podaj jeden lub dwa argumenty programu!\n");
		exit(1);
	}
	else if(argc == 2) {
		/* Nie podano nicku - ustawienie domyslnego nicku */
		strcpy(ja->nick, "NN");
		strcpy(propMy.nick, "NN");
	}
	else {
		/* Podano nick - ustawienie nicku podanego jako drugi argument */
		strcpy(ja->nick, argv[2]);
		strcpy(propMy.nick, argv[2]);
	}

	/* Forkowanie */
	if((pid = fork()) < 0) {
		printf("Blad funkcji fork\n");
		exit(1);
	}
	else if(pid == 0) {
		/* PROCES POTOMNY - KLIENT */
		
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
		
		/* przygotowanie adresu serwera */
		server_addr.sin_family = AF_INET; /* IPv4 */
		inet_aton(argv[1],&server_addr.sin_addr); /* 1. argument = adres IP serwera */
		server_addr.sin_port = htons(my_port); /* port 6767 */
	
		/* towrze gniazdo - na razie tylko czesc "protokol" */
		sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	
		/* Komunikacja */
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
		
		strncpy(ja->komunikat, "Ustawione", 64);

		/* Wyslanie propozycji gry */
		bytes = sendto(sockfd, &ja, sizeof(ja), 0, 
					(struct sockaddr *)&server_addr, sizeof(server_addr));

		/* Wysylanie informacji o mojej kolejce */
		/*if(strcmp(data, "Start") == 0) {
			strncpy(data, "turn1", 64);
			printf("Zmieniam wartosc na turn1\n");
			bytes = sendto(sockfd, &turn, sizeof(turn), 0, (struct sockaddr*)&server_addr, sizeof(server_addr)); 
		} */
		
		if(bytes > 0) {
			printf("[Wyslano propozycje gry do %s]\n", argv[1]);
		}
		else {
			printf("Nie udalo sie wyslac propozycji gry do %s\n", argv[1]);
			exit(1);
		}

		/* Wlasciwa gra */
		while(8) {
			if((strcmp("Tak", ja->komunikat) == 0)||(strcmp("Skip", ja->komunikat) == 0)) {
				if(strcmp("Skip", ja->komunikat) == 0) {
					printf("TEST: faza druga ok\n");
					strcpy(s.strzal, "NN");
					s.strzal[strlen(s.strzal)-1] = '\0';
					bytes = sendto(sockfd, &s, sizeof(s), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
					strncpy(ja->komunikat, "Nie", 64);
				}
				else {
					printf("Wybierz pole do strzalu: ");
					fgets(s.strzal, 4, stdin);
					bytes = sendto(sockfd, &s, sizeof(s), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
					strncpy(ja->komunikat, "Nie", 64);
				}
			}
			else {
				while((strcmp("Tak", ja->komunikat) != 0)&&(strcmp("Skip", ja->komunikat) != 0)) {
					/* Busy waiting - zapobiega problemom przy niegrzecznym zachowaniu */
				}
			}
		}

		shmdt(ja);
	}
	else {
		/* PROCES MACIERZYSTY - SERWER */

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
		server_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* dowolny (moj) interfejs */
		/*inet_addr("158.75.112.121"); //juliusz na sztywno */
		server_addr.sin_port        = htons(my_port);    /* moj port */
		
		bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)); 
	
		/*strncpy(data, "Start", 64); */

		/* Odbieranie informacji o kolejce przeciwnika */
		/*while(1) {
			/* turn.myTurn == 1 -> moja kolej, nie czekamy na pakiet od przeciwnika */
			/*if(strcmp(data, "Start")  == 0) {
				continue;
			}
			if(strcmp(data, "turn1") == 0) {
				break;
				printf("Moja kolej\n");
			}
			/* W przeciwnym wypadku - czekamy na pakiet od przeciwnika i ustamy nie nasza kolej */
			/*recvfrom(sockfd, &turn, sizeof(turn), 0, NULL, NULL);
			strncpy(data, "turn0", 64);
			printf("Kolej przeciwnika\n");
			break;
		}*/

		/* Czekanie na przyjecie propozycji */
		while(1) {
			recvfrom(sockfd, &przeciwnik, sizeof(przeciwnik), 0, NULL, NULL);
			while(2) {
				if(strcmp(ja->komunikat, "Ustawione") == 0) {
					printf("Propozycja gry przyjeta\n");
					break;
				}
			}
			
			strncpy(ja->komunikat, "Tak", 64);

			break;
		}

		/* Wlasciwa gra */
		while(8) {
			recvfrom(sockfd, &sPrzeciwnik, sizeof(sPrzeciwnik), 0, NULL, NULL);
			printf("Przeciwnik strzelil: %s\n", sPrzeciwnik.strzal);
			
			sPrzeciwnik.strzal[strlen(sPrzeciwnik.strzal)-1] = '\0';
			if(sprawdzTrafienie(sPrzeciwnik.strzal, ja) == 1) {
				printf("TEST: komunikat = Nastepny\n");
				strncpy(ja->komunikat, "Skip", 64);
			}
			else {
				strncpy(ja->komunikat, "Tak", 64);
			}
		}
		printf("Rozpoczynamy gre elo!\n");
		
		shmdt(ja);
	}

	close(sockfd);
	return 0;
}
