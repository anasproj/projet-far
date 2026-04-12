# Polytech Chess Tournament - World Polytech Edition

Une application de tournoi d'échecs en ligne complète, basée sur une architecture client/serveur robuste utilisant les principes fondamentaux des applications réparties.

## 🚀 Architecture Technique

Le projet repose sur une séparation claire des responsabilités et l'utilisation de protocoles adaptés :

### Serveur (Multi-processus)
- **Lobby UDP (Port 8080)** : Gestion du matchmaking simplifié, de l'inscription aux tournois et de la liste des salons.
- **Salons TCP (Ports dynamiques)** : Pour chaque match commencé, le serveur `fork()` un nouveau processus "Salon". Ce processus gère la partie d'échecs via TCP pour garantir l'ordre et la fiabilité des coups.
- **Multiplexage (`select`)** : Chaque salon utilise `select()` pour gérer simultanément deux joueurs et plusieurs spectateurs.
- **Persistance** : Sauvegarde automatique de l'historique des parties et des statistiques dans `data/history.txt`.

### Client (Graphique & Réseaux)
- **Interface Graphique** : Développée avec la bibliothèque **Raylib**.
- **Moteur d'Échecs** : Intégration d'un moteur d'échecs en C pour la validation des coups et la gestion du plateau.
- **Navigation** : Système de menus complet avec historique, lobby et observation en direct.
- **Chat** : Chat intégré en temps réel dans chaque salon de jeu.

## ✨ Fonctionnalités

- [x] **Matchmaking** : Mise en relation automatique de deux joueurs en attente.
- [x] **Spectateur** : Possibilité d'observer n'importe quel match en cours via la liste des salons.
- [x] **Chat** : Discussion en temps réel pendant les matchs.
- [x] **Tournoi** : Système d'inscription et de gestion automatique de tournoi à 4 joueurs.
- [x] **Historique** : Consultation des résultats des parties précédentes.
- [x] **Navigation Fluide** : Utilisation de la touche Echap et de boutons Retour.

## 🛠 Compilation et Lancement

### Prérequis
- GCC
- Raylib (installée sur le système)

### Compilation
Utilisez le Makefile fourni à la racine du projet :
```bash
make all
```

### Lancement du Serveur
```bash
./build/serveur
```

### Lancement du Client
```bash
./build/client [IP_SERVEUR] [PSEUDO]
```
Par défaut, l'IP est `127.0.0.1`.

## 📂 Structure du Projet

- `src/` : Code source C (modulaire).
  - `serveur_main.c` / `serveur_tournament.c` : Cœur du serveur et tournois.
  - `salon_main.c` / `salon_logic.c` : Gestion des processus de jeu.
  - `client_main.c` / `client_ui.c` / `client_network.c` / ... : Module client.
- `extern/` : Bibliothèques externes (Moteur d'échecs).
- `data/` : Fichiers de persistance (historique, stats).
- `build/` : Binaires et assets générés.

---
*Projet réalisé dans le cadre du module Fondamentaux des Applications Réparties - Polytech.*
