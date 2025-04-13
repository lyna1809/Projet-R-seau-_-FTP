# Projet FTP avec serveur concurrent

Ce projet est une implémentation d’un **serveur FTP simplifié** en C, avec un **pool de processus**, capable de gérer plusieurs clients simultanément via des **sockets TCP**.

---

##  Objectif

Créer un **protocole personnalisé FTP** (`MYFTP`) entre un client et un serveur en deux étapes :

1.  Version de base du serveur FTP : GET d’un seul fichier par connexion
2. Version avancée : gestion par blocs, persistance, multi-commandes, reprise de transfert, architecture maître-esclaves

---

##  Fonctionnalités implémentées

###  Étape I : Serveur FTP de base

- [x] Définition du type énuméré `typereq_t` : `GET`, etc.
- [x] Structure `request_t` : type + nom de fichier
- [x] Structure `response_t` : code retour (succès/échec)
- [x] Client/Serveur sur le port `2121`
- [x] Serveur concurrent via un **pool de processus** (`NB_PROC`)
- [x] Dossiers `server_files/` et `client_files/` pour les transferts
- [x] Requête GET : chargement du fichier en mémoire, transfert en une fois
- [x] Gestion propre du signal `SIGINT` pour arrêter le serveur
- [x] Client CLI : envoie la requête, reçoit le fichier, affiche les statistiques

###  Étape II : Fonctionnalités avancées

- [x] Découpage en **blocs** pour transferts volumineux
- [x] Requêtes multiples dans une même connexion (`get`, `get`, ..., `bye`)
- [x] **Reprise de transfert** si crash du client (sauvegarde locale du point d’arrêt)
- [x] Architecture maître/esclaves : **répartition des connexions** (round-robin)
