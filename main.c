#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define PORT "1112"
#define MAXCLIENTS 4
#define SZO 0
#define JATEK 1
#define MAX_BUFF_LEN 256
#define MAXWORDLEN 30

struct client {
    int fd;
    int mod;
    int point;
    char ip[INET6_ADDRSTRLEN];
    char port[6];
    struct client *next;
};

struct word {
    char w[MAXWORDLEN];
    struct word *next;
};

struct tipp {
    char message[MAX_BUFF_LEN];
    struct tipp *next;
};

int main(void)
{
    srand(time(NULL));

    struct addrinfo hints, *res;
    struct sockaddr claddr;

    struct client *clients=NULL;
    struct word *dictionary=NULL;
    struct tipp *history=NULL;

    int listener, fdmax, new_fd, yes=1, clientnumber=0, i, j, k, gameisstarted=0, wordnumber=0, sorsoltlen, akt_jatekos_fd;

    char sorsolt[MAXWORDLEN];

    FILE * input;
    input = fopen("dictionary.txt", "r");
    if(!input){
        printf("nem sikerült megnyitni\n");
    } else {
        printf("file megnyitva\n");
        while(!feof(input)){
            struct word *proba=(struct word *) malloc(sizeof(struct word));
            if(1==fscanf(input, "%s", proba->w)){
                proba->next=dictionary;
                dictionary=proba;
                wordnumber++;
            }
        }
    }
    fclose(input);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    getaddrinfo(NULL, PORT, &hints, &res);


    listener = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    bind(listener, res->ai_addr, res->ai_addrlen);
    listen(listener, SOMAXCONN);

    fd_set master, readfds;
    FD_ZERO(&master);
    FD_ZERO(&readfds);
    FD_SET(listener, &master);
    fdmax=listener;

    printf("A szerver elindult!\n");

    while(1){
        readfds = master;
        select(fdmax +1, &readfds, NULL, NULL, NULL);

        for(i=0; i<=fdmax; i++){
            if(FD_ISSET(i, &readfds)){
                if(i==listener){
                    new_fd = accept(i, 0, 0);

                    printf("%d jatekos csatlakozott!\n", i);

                    if(MAXCLIENTS!= clientnumber){
                        if(fdmax < new_fd){
                            fdmax=new_fd;
                        }
                        FD_SET(new_fd, &master);
                        clientnumber++;

                        struct client *newclient = (struct client *) malloc(sizeof(struct client));
                        newclient->fd=new_fd;
                        newclient->mod=SZO;
                        newclient->point=0;

                        if(claddr.sa_family==AF_INET){
                            inet_ntop(AF_INET, &(((struct sockaddr_in *)&claddr)->sin_addr), newclient->ip, INET_ADDRSTRLEN);
                            int port =ntohs(((struct sockaddr_in *)&claddr)->sin_port);
                            sprintf(newclient->port, "%d", port);
                        } else {
                            inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)&claddr)->sin6_addr), newclient->ip, INET6_ADDRSTRLEN);
                            int port =ntohs(((struct sockaddr_in6 *)&claddr)->sin6_port);
                            sprintf(newclient->port, "%d", port);
                        }

                        newclient->next=clients;
                        clients=newclient;

                        FILE * in;
                        in = fopen("rules.txt", "r");
                        if(!in){
                            printf("nem sikerült megnyitni\n");
                        } else {
                            printf("file megnyitva\n");
                            while(!feof(in)){
                                char row[1000];
                                if(fgets(row, 1000 , in)!=NULL){
                                       printf("%s", row);
                                       send(new_fd, row, strlen(row), 0);
                                }
                            }
                        }
                        fclose(in);

                    } else {
                        if(!fork()){
                            FD_CLR(listener, &master);
                            close(listener);
                        } else {
                            FD_ZERO(&master);
                            FD_SET(listener, &master);
                            FD_SET(new_fd, &master);
                            fdmax=(listener<new_fd)?new_fd:listener;
                            clientnumber=1;

                            struct client *cltmp=clients;
                            for(;clients!=NULL;){
                                cltmp=clients;
                                clients=clients->next;
                                free(cltmp);
                            }

                            struct client *newclient = (struct client *) malloc(sizeof(struct client));
                            newclient->fd=new_fd;
                            newclient->mod= SZO;
                            newclient->point=0;

                            if(claddr.sa_family==AF_INET){
                                inet_ntop(AF_INET, &(((struct sockaddr_in *)&claddr)->sin_addr), newclient->ip, INET_ADDRSTRLEN);
                                int port =ntohs(((struct sockaddr_in *)&claddr)->sin_port);
                                sprintf(newclient->port, "%d", port);
                            } else {
                                inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)&claddr)->sin6_addr), newclient->ip, INET6_ADDRSTRLEN);
                                int port =ntohs(((struct sockaddr_in6 *)&claddr)->sin6_port);
                                sprintf(newclient->port, "%d", port);
                            }

                            newclient->next=clients;
                            clients=newclient;

                            FILE * in;
                            in = fopen("rules.txt", "r");
                            if(!in){
                                printf("nem sikerült megnyitni\n");
                            } else {
                                printf("file megnyitva\n");
                                while(!feof(in)){
                                    char row[1000];
                                    if(fgets(row, 1000 , in)!=NULL){
                                           printf("%s", row);
                                           send(new_fd, row, strlen(row), 0);
                                    }
                                }
                            }
                            fclose(in);

                            gameisstarted=0;
                        }
                    }
                } else {
                    char buff[MAX_BUFF_LEN];
                    int recbyts = recv(i, buff, sizeof(buff), 0);

                    if(recbyts == 0){
                        FD_CLR(i, &master);
                        clientnumber--;
                        close(i);

                        if(i == clients->fd){
                            struct client *cltmp = clients;
                            clients=clients->next;
                            free(cltmp);
                        } else {
                            struct client *akt = clients->next;
                            struct client *pre = clients;
                            for(; akt->fd!=i; pre=pre->next, akt=akt->next);
                            pre->next=akt->next;
                            free(akt);
                        }
                    } else {
                        buff[recbyts - 2]='\0';

                        if(strncmp(buff, "INDUL", 5) == 0){
                            struct client *cltmp = clients;
                            for(; cltmp->fd != i; cltmp = cltmp->next);
                            cltmp->mod = JATEK;

                            printf("%d jatekos belépett a jatekba!\n", i);
                            send(i, "BELEPTEL A JETEKBA\n", strlen("BELEPTEL A JETEKBA\n"), 0);

                            for (j = 0; j <= fdmax; j++) {
                                if ((FD_ISSET(j, &master)) && (j != listener)&& (j != i)) {
                                    char uzenet[500] = "";
                                    sprintf(uzenet, "%d BELEPTETT A JETEKBA\n", i);
                                    send(j, uzenet, strlen(uzenet), 0);
                                }
                            }
                            if(!gameisstarted){
                                //TODO elindítani a játékot
                                j  = rand() % (wordnumber) + 0;
                                printf("%d\n", j);
                                struct word *tmpword = dictionary;
                                for(k=0; k<j; k++, tmpword=tmpword->next);
                                strcpy(sorsolt, tmpword->w);
                                sorsoltlen=strlen(sorsolt);
                                printf("%s, a szó, hossza: %d karakter\n", sorsolt, sorsoltlen);

                                gameisstarted=1;

                                akt_jatekos_fd = i;

                                for (j = 0; j <= fdmax; j++) {
                                    if ((FD_ISSET(j, &master)) && (j != listener)) {
                                        struct client *clienttmp = clients;
                                        for(; clienttmp->fd != j; clienttmp = clienttmp->next);
                                        if(clienttmp->mod==JATEK){
                                            char uzenet[500] = "";
                                            sprintf(uzenet, "%d ELINDITOTTA A JETEKOT\nA szo hossza: %d\n", i, sorsoltlen);
                                            send(j, uzenet, strlen(uzenet), 0);
                                        }
                                    }
                                }
                            }

                        } else if(strncmp(buff, "KILEP", 5) == 0){
                            struct client *cltmp = clients;
                            for(; cltmp->fd != i; cltmp = cltmp->next);
                            cltmp->mod = SZO;

                            printf("%d jatekos kilépett jatekból!\n", i);
                            send(i, "KILEPTEL A JETEKBOL\n", strlen("KILEPTEL A JETEKOL\n"), 0);

                            for (j = 0; j <= fdmax; j++) {
                                if ((FD_ISSET(j, &master)) && (j != listener) && (j != i)) {
                                    char uzenet[500] = "";
                                    sprintf(uzenet, "%d KILEPETT A JATEKBOL\n", i);
                                    send(j, uzenet, strlen(uzenet), 0);
                                }
                            }
                        } else if(strncmp(buff, "SZOTAR", 6) == 0){
                            //TODO üzenet többi része bemegy a szótárba
                            struct client *cltmp = clients;
                            for(; cltmp->fd != i; cltmp = cltmp->next);
                            if(cltmp->mod == JATEK){
                                struct word *newword = (struct word *) malloc(sizeof(struct word));
                                strtok(buff, " ");
                                char *sec_part = strtok(NULL, " ");
                                printf("AZ UJ SZÓ %s", sec_part);
                                strcpy(newword->w,sec_part);
                                newword->next=dictionary;
                                dictionary=newword;
                                wordnumber++;

                                struct word *tmpword = dictionary;
                                for(;tmpword!=NULL;tmpword=tmpword->next){
                                    printf("a listában %d sz van: %s\n", wordnumber, tmpword->w);
                                }
                            }

                        } else if(strncmp(buff, "?", 1) == 0){
                            //TODO visszaadni a historyt
                            char uzenet[MAX_BUFF_LEN]="";
                            sprintf(uzenet, "S: %d\n", sorsoltlen);
                            send(i, uzenet, strlen(uzenet), 0);
                            struct tipp *htmp = history;
                            for(; htmp!=NULL; htmp=htmp->next){
                                printf("%s", htmp->message);
                                send(i, htmp->message, strlen(htmp->message), 0);
                            }
                        } else {
                            printf("szó elküldve\n");
                            struct client *cltmp = clients;
                            for(; cltmp->fd != i; cltmp = cltmp->next);
                            if(cltmp->mod == SZO){
                                //TODO a buff bemegy a szótárba
                                struct word *newword = (struct word *) malloc(sizeof(struct word));
                                strcpy(newword->w,buff);
                                newword->next=dictionary;
                                dictionary=newword;
                                wordnumber++;

                                struct word *tmpword = dictionary;
                                for(;tmpword!=NULL;tmpword=tmpword->next){
                                    printf("a listában %d sz van: %s\n", wordnumber, tmpword->w);

                                }

                            } else {
                                //TODO vissazadjuk a tipp értékelését
                                if(i==akt_jatekos_fd){
                                    if(strlen(buff)==sorsoltlen){
                                        printf("jó jatakos elég hosszú szót küldött\n");
                                        k=0;
                                        for(j=0; j<sorsoltlen; j++){
                                            if(buff[j]==sorsolt[j]){
                                                k++;
                                            }
                                        }
                                        for (j = 0; j <= fdmax; j++) {
                                            if ((FD_ISSET(j, &master)) && (j != listener)) {
                                                struct client *clienttmp = clients;
                                                for(; clienttmp->fd != j; clienttmp = clienttmp->next);
                                                if(clienttmp->mod==JATEK){
                                                    char uzenet[MAX_BUFF_LEN] = "";
                                                    sprintf(uzenet, "C%d: %s\nS: %d\n", i, buff, k);
                                                    send(j, uzenet, strlen(uzenet), 0);                                                    
                                                }
                                            }
                                        }
                                        if(k==sorsoltlen){  //vége a játéknak
                                            cltmp->point++;
                                            char uzenet[MAX_BUFF_LEN] = "";
                                            sprintf(uzenet, "S: C%d megnyerte a játékot\n\nA játék állása:\n", i);
                                            struct client *clienttmp = clients;
                                            for(; clienttmp!= NULL; clienttmp = clienttmp->next){
                                                char row[MAXWORDLEN];
                                                sprintf(row,  "C%d: %d\n", clienttmp->fd, clienttmp->point);
                                                strcat(uzenet, row);
                                            }
                                            send(j, uzenet, strlen(uzenet), 0);
                                            for (j = 0; j <= fdmax; j++) {
                                                if ((FD_ISSET(j, &master)) && (j != listener)) {
                                                    struct client *clienttmp = clients;
                                                    for(; clienttmp->fd != j; clienttmp = clienttmp->next);
                                                    if(clienttmp->mod==JATEK){                                                        
                                                        send(j, uzenet, strlen(uzenet), 0);
                                                    }
                                                }
                                            }
                                            //TODO historyt üríteni
                                            struct tipp *htmp=history;
                                            for(;history!=NULL;){
                                                htmp=history;
                                                history=history->next;
                                                free(htmp);
                                            }
                                            //TODO új jatekot indítani
                                            j  = rand() % (wordnumber) + 0;
                                            printf("%d\n", j);
                                            struct word *tmpword = dictionary;
                                            for(k=0; k<j; k++, tmpword=tmpword->next);
                                            strcpy(sorsolt, tmpword->w);
                                            sorsoltlen=strlen(sorsolt);
                                            printf("%s, a szó, hossza: %d karakter\n", sorsolt, sorsoltlen);

                                            for (j = 0; j <= fdmax; j++) {
                                                if ((FD_ISSET(j, &master)) && (j != listener)) {
                                                    struct client *clienttmp = clients;
                                                    for(; clienttmp->fd != j; clienttmp = clienttmp->next);
                                                    if(clienttmp->mod==JATEK){
                                                        char uzenet[500] = "";
                                                        sprintf(uzenet, "\nÚJ JATEK INDULT\nS: %d\n", sorsoltlen);
                                                        send(j, uzenet, strlen(uzenet), 0);
                                                    }
                                                }
                                            }
                                        } else {
                                            //TODO megkeresni a következő játékost
                                            struct client *clienttmp = cltmp->next;
                                            for(; clienttmp!=NULL; clienttmp=clienttmp->next){
                                                if(clienttmp->mod==JATEK && i==akt_jatekos_fd){
                                                    akt_jatekos_fd=clienttmp->fd;
                                                }
                                            }
                                            if(akt_jatekos_fd==i){
                                                clienttmp = clients;
                                                for(; clienttmp!=cltmp; clienttmp=clienttmp->next){
                                                    if(clienttmp->mod==JATEK && i==akt_jatekos_fd){
                                                        akt_jatekos_fd=clienttmp->fd;
                                                    }
                                                }
                                            }
                                            printf("a következő játékos: %d\n", akt_jatekos_fd);

                                            struct tipp *htmp = (struct tipp *) malloc(sizeof(struct tipp));
                                            char uzenet[MAX_BUFF_LEN] = "";
                                            sprintf(uzenet, "C%d: %s\nS: %d\n", i, buff, k);
                                            strcpy(htmp->message, uzenet);
                                            htmp->next=NULL;
                                            if(history==NULL){
                                                history=htmp;
                                                printf("a history->message: %s \n", history->message);
                                            } else {
                                                printf("beléptem a nem első tipp mentésébe\n");
                                                struct tipp *htmp2 = history;
                                                if(htmp2!=NULL){
                                                    printf("létrejött htmp2\n");
                                                }
                                                for(;htmp2->next!=NULL; htmp2=htmp2->next);
                                                printf("ráálltam a %s message-ú adattgra\n", htmp2->message);
                                                htmp2->next=htmp;
                                            }
                                        }
                                    } else {
                                        char uzenet[500] = "";
                                        sprintf(uzenet, "TÉVES SZÓHOSSZ, %d KARAKTERT VÁROK\n", sorsoltlen);
                                        send(i, uzenet, strlen(uzenet), 0);
                                    }
                                } else {
                                    printf("nem jó jatakos küldött szót\n");
                                    send(i, "NEM TE KÖVETKEZEL\n", strlen("NEM TE KÖVETKEZEL\n"), 0);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

