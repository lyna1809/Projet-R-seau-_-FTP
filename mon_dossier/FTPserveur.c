/*
 * echoserveri.c - An iterative echo server
 */

 #include "csapp.h"

 #define MAX_NAME_LEN 256       
 #define NB_PROC 10            // nombre des fils serveurs 
 #define TAILLE_BLOC 2000      // taille de bloc de chargement du fichier en mémoire

 
 pid_t pids[NB_PROC];          // tabeaux des pids des fils de serveur   
 
//GETR : get_reprendre (cas de panne, envoyer automatiquement par le client lors de connexion)
typedef enum {GET, bye , GETR} typereq_t;    // enumerations des requettes possibles

 typedef struct{ 
     typereq_t type;             // type de requette 
     int taille_nom_fichier;    // taille de nom de fichier 
     char* nom_fichier;         // le nom de fichier 
 }request_t;


 typedef struct {
     int status;            // Etat de la requette : 0 succes -1 sinon 
     int taille_fichier;    // taille totale du fichier (nombre d'octets)
 } response_t;


 void init_requette(request_t* req){
    req->type= bye;
    req->taille_nom_fichier= 0;
    req->nom_fichier=NULL;
 }

 void init_reponse(response_t* reponse){
    reponse->status=0;
    reponse->taille_fichier=0;
 }
 

 void sigint_handler(int sig){

    for(int i=0;i<NB_PROC;i++){          // envoi un signal au tous les fils 
         kill(pids[i],sig);
    }

    while(waitpid(-1,NULL,WNOHANG)>0);  // attendre la mort de tous les fils en premier
    exit(0);                            // tuer le pere
 }



 void traitement_requette(rio_t* rio , int connfd ){

    //objet requette et reponse
    int nb_octets_lus; 
    request_t req; 
    response_t reponse;
    
    init_requette(&req);
    init_reponse(&reponse);

    // lire le type de la 1ère requette 
    if((nb_octets_lus = Rio_readnb( rio ,&req.type , sizeof(int)))==0){ 
        fprintf(stderr, "Client déconnecté \n");
        return;
    }
    else if(nb_octets_lus <0 || nb_octets_lus <sizeof(int)){
        fprintf(stderr, "Erreur lors la lecture de type de requette\n");
        return;
    } 
    
    //tanque y'a encore des requettes (cas de GET ou GETR)
    while(req.type != bye){   

        //lecture de nombre des octets deja envoyé (cas GETR : nb_octets != 0 donc reprendre le travaille)
        int nb_octets_envoyer=0;  
        if(req.type == GETR){ 
            if((nb_octets_lus = Rio_readnb( rio ,&nb_octets_envoyer , sizeof(int)))==0){
                fprintf(stderr, "Client déconnecté \n");
                return;
            }
            else if(nb_octets_lus <0 || nb_octets_lus <sizeof(int)){
                fprintf(stderr, "Erreur lors la lecture de nombre des octets déja lus (GETR)\n");
                return;
            } 
        }

        printf("NOmbre d'octets deja lus est : %d\n",nb_octets_envoyer );
       
        // lire la taille de nom de fichier
        if((nb_octets_lus = Rio_readnb( rio ,&req.taille_nom_fichier , sizeof(int)))==0){ 
            fprintf(stderr, "Client déconnecté \n\n");
            return;
        } else if(nb_octets_lus <0 || nb_octets_lus <sizeof(int)){
            fprintf(stderr, "Erreur lors la lecture de taille de nom de fichier\n");
            return;
        }   

        req.nom_fichier = malloc(req.taille_nom_fichier + 1);       //allocation de mèmoire pour le nom

        if (req.nom_fichier == NULL) {
            perror("Erreur lors l'allocation de memoire\n");
            reponse.status = -1;   
            reponse.taille_fichier = 0; 

            Rio_writen ( connfd , &reponse.status, sizeof(int)); 
            Rio_writen ( connfd , &reponse.taille_fichier, sizeof(int)); 
            return ; 
        }

        // lire le nom de fichier
        if ((nb_octets_lus= Rio_readnb( rio , req.nom_fichier , req.taille_nom_fichier ))==0){
            fprintf(stderr, "Client déconnecté \n");
            free(req.nom_fichier);
            return;
        }
        else if(nb_octets_lus <0 || nb_octets_lus <sizeof(int)){
            fprintf(stderr, "Erreur lors la lecture de nom de fichier\n");
            free(req.nom_fichier);
            return;
        }  
        
        req.nom_fichier[req.taille_nom_fichier] = '\0';
        printf("Nom ficier: %s  \n",  req.nom_fichier );
            
        // ouverture fichier   
        char filepath[256];  
        sprintf(filepath, "fichier_serveur/%s", req.nom_fichier); //concatenation nom fichier et repertoire

        int fd_fichier = open(filepath, O_RDONLY);

        if (fd_fichier < 0) {      

            reponse.status = -1;      // Fichier introuvable ou erreur d'ouverture
            reponse.taille_fichier = 0; 

            printf("Erreur lors l'overteur de fichier!\n");
            Rio_writen ( connfd , &reponse.status, sizeof(int)); 
            Rio_writen ( connfd , &reponse.taille_fichier, sizeof(int)); 

        }
        else{
                  
            reponse.status = 0;  // fichier existe 
        
            // tructure pour obtenir les informations relatives au fichier
            struct stat st; 
            if (fstat(fd_fichier, &st) < 0) {   // obtenir la taille de fichier 
                perror("Erreur fstat\n");
                reponse.taille_fichier = 0;
            } else {
                reponse.taille_fichier = st.st_size;

                // Positionnement à l’offset (0 si GET, sinon offset si GETR)
                // off_t :  conçu pour tailles de fichiers 
                off_t offset = nb_octets_envoyer;
                lseek(fd_fichier, offset, SEEK_SET);

                // cas GET : nb_octets_envoyer = 0
                // cas GETR : nb_octets_envoyer != 0
                int nb_octets_reste= ( reponse.taille_fichier - nb_octets_envoyer );
                int nombre_des_blocs = nb_octets_reste / TAILLE_BLOC; 
                int taille_dernier_bloc = nb_octets_reste % TAILLE_BLOC; 

                Rio_writen ( connfd , &reponse.status, sizeof(int)); 
                Rio_writen ( connfd , &reponse.taille_fichier, sizeof(int)); 

                char buffer [TAILLE_BLOC];
                int nb_lus;

                printf("Taille de fichier: %d \n", reponse.taille_fichier);

                for (int i = 0; i < nombre_des_blocs; i++) {
                    nb_lus = read(fd_fichier, buffer, TAILLE_BLOC);
                    if (nb_lus < 0) {
                        perror("Erreur de lecture\n");
                        break;
                    }
                    Rio_writen(connfd, buffer, TAILLE_BLOC);
                }
            
                if (taille_dernier_bloc > 0) {  // Lire et envoyer le dernier bloc 
                    nb_lus = read(fd_fichier, buffer, taille_dernier_bloc);
                    if (nb_lus < 0) {
                        perror("Erreur de lecture du dernier bloc\n");
                    } else {
                        Rio_writen(connfd, buffer, taille_dernier_bloc);
                    }
                } 
                Close(fd_fichier); 

            }   
           
        }
        
        printf("Fin de traitement! \n\n");

        free(req.nom_fichier);
        init_requette(&req);
        init_reponse(&reponse);
    
        // lire le type d'une autre requette 
        if ((nb_octets_lus= Rio_readnb( rio ,&req.type , sizeof(int)))==0){
            fprintf(stderr, "Client déconnecté \n");
            return;
        }
        else if(nb_octets_lus <0 || nb_octets_lus <sizeof(int)){
            fprintf(stderr, "Erreur lors la lecture de type de requette\n");
            return;
        }  
        
    } 
}

 


 /* 
  * Note that this code only works with IPv4 addresses
  * (IPv6 is not supported)
  */

 int main(int argc, char **argv)
 {


    Signal(SIGINT,sigint_handler);  //re_definition de handler (pour le père)
    Signal(SIGPIPE, SIG_IGN);  // pour ne pas crasher sur write)


    int listenfd, connfd, port;
    socklen_t clientlen;
    struct sockaddr_in clientaddr;
    char client_ip_string[INET_ADDRSTRLEN];
    char client_hostname[MAX_NAME_LEN];
    
    if (argc != 1) {
        fprintf(stderr, "usage: %s \n", argv[0]);
        exit(0);
    }
    
    //numero de port par defaut 
    port = 2121;
 
    clientlen = (socklen_t)sizeof(clientaddr);
    listenfd = Open_listenfd(port);
    
    pid_t pid; 

    //ceartion des fils
    for(int i=0 ; i< NB_PROC; i++){
 
        if((pid = fork()) > 0){
            pids[i]=pid;    // sauvgarder les pids des fils crée                             
        } else if (pid <0) {
            unix_error("Erreur : lors la creation de fils\n");
        }
        else {
            break;   // si on est dans le fils, arrete la creation des fils                            
        }
    }
      
    
    while (1) {

        if(pid != 0){ // cas de pere 
            pause();  // pour ne pas boucler pour rien
        }
        else{ // cas des fils , tous les fils sort de boucle avec break donc la valeur de pid = 0 toujours 

            Signal(SIGINT, SIG_DFL);  //re_mettre de handler par default pour les fils

            while((connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen)) == -1){
                //attente active de connexion avec un client 
            }

            // Initialiser rio avec connfd (socket de communication)
            rio_t rio;
            Rio_readinitb(&rio, connfd);

            /* determine the name of the client */
            Getnameinfo((SA *) &clientaddr, clientlen,
                         client_hostname, MAX_NAME_LEN, 0, 0, 0);
             
            /* determine the textual representation of the client's IP address */
            Inet_ntop(AF_INET, &clientaddr.sin_addr, client_ip_string,
                     INET_ADDRSTRLEN);
             
            printf("Serveur connecté avec : %s (%s)\n\n", client_hostname,
                 client_ip_string);
            
            traitement_requette(&rio, connfd );
            
            printf("Fin de communication avec %s (%s)\n\n", client_hostname,
                client_ip_string);
            Close(connfd); // bye recu ou client déconnecté 
            
         }    
     }

     exit(0);
 }