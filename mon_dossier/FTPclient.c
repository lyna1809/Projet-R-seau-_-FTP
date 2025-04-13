/*
 * echoclient.c - An echo client
 */
#define _POSIX_C_SOURCE 199309L 
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

struct addrinfo; 
#include "csapp.h"

#define TAILLE_BLOC 2000
#define COMMANDE_INEXISTANTE 3

typedef enum {GET, bye,GETR} typereq_t;


typedef struct { 
    typereq_t type;
    char* nom_fichier;
    int taille_nom_fichier ; //taille du nom du fichier
}request_t;


typedef struct {
    int status;            // Etat de la requette : 0 succes -1 sinon 
    int taille_fichier;    // taille totale du fichier (nombre d'octets)
} response_t;



//cette focntion est pour recuperer la ligne de commande (exemple GET toto.txt) 
// et trouver l'indice de separation entre le nom de la commande et le nom du fichier : 
void get_command_line(int *tmp,char buf[MAXLINE]){

    //lire le contenu de la ligne de commande ecrite par l'utilisateur
    printf("> FTP ");
    Fgets(buf, MAXLINE, stdin); 
    buf[strlen(buf)-1] = '\0' ; 

    //recuperer la commande (GET,...) et le nom du fichier : 
    for(int i=0 ; buf[i] != '\0'; i++){
        if(buf[i]== ' '){
            *tmp = i ;
            break; 
        }
    }
}


//recuperer le nom de la commande et le nom du fichier (s'il ne sagit pas de bye):
void extract_filename(char commande[20],char buf[MAXLINE],request_t *request,int tmp){
    //s'il sa'git d'une commande existante "GET"
    if (strcmp(commande, "GET") == 0) {
        // Calculer la longueur du nom de fichier (après l'espace)
        int len = strlen(buf) - tmp - 1;
        // Allouer de la mémoire pour le nom du fichier (+1 pour le '\0')
        request->nom_fichier = malloc(len + 1);
        //verifier l'allocation: 
        if (request->nom_fichier == NULL) {
            perror("Erreur d'allocation de mémoire pour nom_fichier");
            exit(1);
        }
        // Copier le nom du fichier (commence juste après l'espace)
        strncpy(request->nom_fichier, buf + tmp + 1, len);
        request->nom_fichier[len] = '\0';

        // Enregistrer la taille du nom du fichier
        request->taille_nom_fichier = len;
    } 

}

void get_type_command(char buf[MAXLINE], request_t *request){
    //cette focntion est pour recuperer la ligne de commande (exemple GET toto.txt) 
        // et trouver l'indice de separation entre le nom de la commande et le nom du fichier : 
        char commande [20];
        int tmp = -1 ; 
        get_command_line(&tmp,buf);
        //recuperer la commande : 
        if (tmp != -1) { 
            strncpy(commande, buf, tmp);
            commande[tmp] = '\0';
        } else {
            // Si aucun espace trouvé, on suppose que l'utilisateur a entré une commande seule
            strncpy(commande, buf, sizeof(commande) - 1);
            commande[sizeof(commande) - 1] = '\0'; // Sécurité
        }

        //si le nom du fichier existe :
        if(tmp != -1){
            //recuperer le nom du fichier (s'il ne s'aggit pas de commande bye) : 
            extract_filename(commande,buf,request,tmp);
        }
    
        //definir le type selon la commande: 
        if (strcmp(commande, "GET") == 0){
            request->type = 0 ;
            //si y a pas de fichier apres GET : 
            if (tmp == -1 || buf[tmp + 1] == '\0') {
                request->type = COMMANDE_INEXISTANTE +1 ; // Type par defaut pour forcer a continuer 
            }
        }else if(strcmp(commande, "bye")==0) {
            request->type = 1 ;  
        }
        else {
            // si la commande n'existe pas , on lui donne la valeure 2 par defaut , pour assurer que ca rentre dans le while
            // et donc elle affiche une erreur , et redonne la chance a l'utilisateur d'inserer une autre commande : 
            request->type = COMMANDE_INEXISTANTE ; 
        }
}

void lecture_ecriture_bloc(int nb_blocs ,int dernier_bloc, rio_t rio ,int fd , int clientfd , int *nmbr_octets_lus ,
                           int fd_cache ){
    //lire en blocs: 
    int lu ; 
    char buffer[TAILLE_BLOC];
    

    for(int i=0 ; i<nb_blocs ; i++){
        //verifier les erreurs lors de la lecture des blocs :
        if(i==3){
            
            sleep(1);
        }
       
        if((lu = rio_readnb(&rio, buffer , TAILLE_BLOC )) < 0  ){
            perror("Erreur lors de la lecture des blocs");
            close(fd);
            close(clientfd);
            exit(1);
        }
    
        //ecrire le contenu lu dans le repertoire du client :  
        write(fd, buffer, lu); 
        *nmbr_octets_lus += lu ; 

        //pour positionner de pointeur de lecteur fichier au dernier int ecrit (nombre d'octets lus)
       if (lseek(fd_cache, -((off_t)sizeof(int)), SEEK_END) == -1) {
           perror("Erreur lors du positionnement de lseek pour la mise à jour");
           exit(1);
       }
       
       // Mise a jour de la valeur dans le fichier cache.txt
       if (write(fd_cache, nmbr_octets_lus, sizeof(int)) != sizeof(int)) {
           perror("Erreur lors de la mise à jour du nombre d'octets lus");
           exit(1);
       }
       
    }
   

    //gerer le dernier bloc :
    if(dernier_bloc > 0){
        //si une erreur lors de la lecture des blocs :
        if((lu = rio_readnb(&rio, buffer , dernier_bloc )) < 0 ){
            perror("Erreur lors de la lecture des blocs");
            close(fd);
            close(clientfd);
            exit(1);
        }
       
        //ecrire le contenu lu dans le repertoire du client : 
        write(fd, buffer, lu);
        *nmbr_octets_lus += lu ; 

         //pour positionner de pointeur de lecteur fichier au dernier int ecrit (nombre d'octets lus)
       if (lseek(fd_cache, -((off_t)sizeof(int)), SEEK_END) == -1) {
           perror("Erreur lors du positionnement de lseek pour la mise à jour");
           exit(1);
       }
       
       // Mise a jour de la valeur dans le fichier cache.txt
       if (write(fd_cache, nmbr_octets_lus, sizeof(int)) != sizeof(int)) {
           perror("Erreur lors de la mise à jour du nombre d'octets lus");
           exit(1);
       }
    }
}
void tratiement_cache(int fd_cache ,int clientfd,rio_t rio){
     //informer le serveur que y a un fichier qui doit finir :   
     int taille_nom_f ;
     char * nom_fichier = NULL ;
     int taille_fichier ;
     int nmb_oct_lus ;
     request_t req ;
     response_t res ; 
     req.type = 2 ;
     

     printf("y a un fichier qu'il faudra finir de charger avant \n");
     //lecture depuis le fichier  
     
     int r = read(fd_cache ,&taille_nom_f,sizeof(int));
     if (r != sizeof(int)) {
         perror("Error reading file name length");
         exit(1);
     }
     
     nom_fichier = malloc(taille_nom_f + 1);
     if (nom_fichier == NULL) {
         perror("Memory allocation failed");
         exit(1);
     }

     r=read(fd_cache ,nom_fichier,taille_nom_f);
     if (r != taille_nom_f) {
         perror("Error reading filename");
         free(nom_fichier);
         exit(1);
     }
     nom_fichier[taille_nom_f] = '\0';

     
     r = read(fd_cache ,&taille_fichier,sizeof(int));
     if (r != sizeof(int)) {
         perror("Error reading file size");
         free(nom_fichier);
         exit(1);
     }

     r = read(fd_cache ,&nmb_oct_lus,sizeof(int));
     if (r != sizeof(int)) {
         perror("Error reading bytes read");
         free(nom_fichier);
         exit(1);
     }

     req.nom_fichier = nom_fichier;
     req.taille_nom_fichier = taille_nom_f;


     printf("Nom fichier : %s\n", nom_fichier);
     printf("Taille fichier : %d\n", taille_fichier);
     printf("Nombre d'octets déja lus : %d\n", nmb_oct_lus);

    
     Rio_writen(clientfd, &req.type, sizeof(req.type)) ;
     Rio_writen(clientfd, &nmb_oct_lus, sizeof(int)) ;
     Rio_writen(clientfd, &taille_nom_f, sizeof(int)) ;
     Rio_writen(clientfd, nom_fichier, taille_nom_f) ;
    
     Rio_readnb(&rio, &res.status, sizeof(int));
     Rio_readnb(&rio, &res.taille_fichier, sizeof(int));


     //on cree un fichier avec le meme nom dans le repertoire "fichier_client" pour ecrire 
     //le contenu recu par le serveur :
     char filepath2 [256]; 
     sprintf(filepath2, "fichier_client/%s", nom_fichier);
     int fd = open(filepath2, O_WRONLY | O_CREAT | O_APPEND, 0644);

     if (fd == -1) { //verifier l'ouverture
         perror("Erreur lors de la creation du fichier");
         Close(clientfd);
         exit(1);
     } 

     off_t offset = nmb_oct_lus;
    lseek(fd, offset, SEEK_SET);

     //Calcul du nombre de blocs et du dernier bloc de la nouvelle taille :  
     int nouvelle_taille = taille_fichier - nmb_oct_lus; 
     int nb_blocs = nouvelle_taille / TAILLE_BLOC;
     int dernier_bloc = nouvelle_taille % TAILLE_BLOC;
     
     //fonction qui fait les lecture du serveur et ecrit dans le fichier :
     lecture_ecriture_bloc(nb_blocs ,dernier_bloc,rio ,fd ,clientfd ,&nmb_oct_lus , fd_cache);
    
     //supprimer le fichier :
     close(fd_cache);
     remove("cache.txt");

     printf("On a fini de charger l'ancien fichier \n");
     free( req.nom_fichier );

}


int main(int argc, char **argv)
{ 
    int clientfd, port;
    char *host, buf[MAXLINE];
    rio_t rio;
   
    

    // Déclaration des variables pour mesurer le temps
    struct timespec start, end;
    double elapsed;
    

    if (argc != 2) {
        fprintf(stderr, "usage: %s <host> \n", argv[0]);
        exit(0);
    }
    host = argv[1];
    port = 2121 ; 


    /*
     * Note that the 'host' can be a name or an IP address.
     * If necessary, Open_clientfd will perform the name resolution
     * to obtain the IP address.
     */
    clientfd = Open_clientfd(host, port);
    
    /*
     * At this stage, the connection is established between the client
     * and the server OS ... but it is possible that the server application
     * has not yet called "Accept" for this connection
     */
    printf("client connected to server OS\n"); 
    
    Rio_readinitb(&rio, clientfd); 

    request_t request;
    response_t response;
    int fd;

    //tenter d'ouvrir 
    int fd_cache = open("cache.txt", O_RDWR, 0666);
    if (fd_cache > 0) { //le fichier existe 
       tratiement_cache(fd_cache ,clientfd,rio);
    } 
        
    get_type_command(buf, &request); // GET
   

    //tant que le client n'envoie pas de "bye" :
    while(request.type != 1){

        if (request.type == 0){  //0 pour GET 
            fd_cache = open("cache.txt", O_RDWR | O_CREAT, 0666);
            if (fd_cache < 0) {
                perror("Erreur lors la creation de cache.txt\n");
                exit(1);
            }
            
            //ecrire la structure dans la socket du client : 
            Rio_writen(clientfd, &request.type, sizeof(request.type)) ;
            Rio_writen(clientfd, &request.taille_nom_fichier, sizeof(int)) ;
            Rio_writen(clientfd, request.nom_fichier, request.taille_nom_fichier) ;

            //sauvgrade la taille du nom defichier en local : 
            // Écriture des détails dans le fichier
            write(fd_cache, &request.taille_nom_fichier, sizeof(int));  

            //sauvgrade du nom du fichier en local : 
            
            write(fd_cache, request.nom_fichier, request.taille_nom_fichier); 
            printf("taille : %s \n",request.nom_fichier);
        
            // Récupérer le temps de début:
            clock_gettime(CLOCK_MONOTONIC, &start);

            // lire la reponse envoye par le serveur:
            Rio_readnb(&rio, &response.status, sizeof(int));
            Rio_readnb(&rio, &response.taille_fichier, sizeof(int));

            //sauvgarde de la taille du fichier en local : 
            write(fd_cache, &response.taille_fichier, sizeof(int)); 
                
            //si le fichier existe : 
            if(response.status == 0){ 

                //on cree un fichier avec le meme nom dans le repertoire "fichier_client" pour ecrire 
                //le contenu recu par le serveur :
                char filepath[256]; 
                sprintf(filepath, "fichier_client/%s", request.nom_fichier);
                fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644);

                if (fd == -1) { //verifier l'ouverture
                    perror("Erreur lors de la creation du fichier");
                    Close(clientfd);
                    exit(1);
                }
                
                //Calcul du nombre de blocs et du dernier bloc: 
                int nb_blocs = response.taille_fichier / TAILLE_BLOC;
                int dernier_bloc = response.taille_fichier % TAILLE_BLOC;
                int nmbr_octets_lus= 0 ; 

               //sauvgarder une copie des octets lus : initiallement 0
               if (write(fd_cache, &nmbr_octets_lus, sizeof(int)) != sizeof(int)) {
                   perror("Erreur lors de l'écriture du nombre d'octets lus initial");
                   exit(1);
               }
               
                // nombre d'octets lu màj dans lecture bloc
                //fonction qui fait les lecture du serveur et ecrit dans le fichier :
                lecture_ecriture_bloc(nb_blocs ,dernier_bloc,rio ,fd ,clientfd ,&nmbr_octets_lus , fd_cache);
            
                //si on a tout lu et y a pas eu d'interruption au milieu : 
                if(nmbr_octets_lus == response.taille_fichier) {
                    close(fd_cache);
                    remove("cache.txt");
                }
                
                // Récupérer le temps de fin: 
                clock_gettime(CLOCK_MONOTONIC, &end);

                // Calculer la différence en secondes avec une précision en nanosecondes:
                elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
                printf("Transfer successfully complete.\n");
                printf("%d bytes received in %.2f seconds (%.2f Kbytes/s).\n",
                response.taille_fichier, elapsed, (response.taille_fichier / 1024.0) / elapsed);


            } else {
                printf("Erreur : Fichier non existant \n"); 
            }
            
        }

        else if (request.type == COMMANDE_INEXISTANTE + 1){ //par defaut : "un GET sans fichier "
           printf("Erreur : commande GET non suivi d'un fichier \n ");
       }
       else if(request.type == COMMANDE_INEXISTANTE){
           printf("commande introuvable \n");
       }

       get_type_command(buf,&request);

       free(request.nom_fichier);
       
   }

   //informer le serveur de la fin "bye" : 
   Rio_writen(clientfd, &request.type, sizeof(int)) ;

   printf("The end of the communication with this client .\n");
   close (fd); 
   Close(clientfd);
   exit(0);

}