# Moteur d'Échecs (Chess Engine)

Un moteur d'échecs et une application de jeu en ligne pour jouer aux échecs contre des IA ou d'autres joueurs en réseau.

## 📋 Table des matières

- [Installation](#installation)
- [Lancer le jeu](#lancer-le-jeu)
- [Architecture](#architecture)
- [Module Tournoi](#module-tournoi)
- [Crédits](#crédits)

## 🚀 Installation

### Préalables

- `clang` (compilateur C)
- `raylib` (bibliothèque graphique)
- `pthread` (threads POSIX)
- `make`

### Compilation

Pour compiler le jeu principal et les modules IA :

```bash
make
```

Pour compiler le serveur avec le support du tournoi :

```bash
make server
```

## 🎮 Lancer le jeu

### Jeu local

```bash
./dist/main
```

Lance l'application graphique pour jouer contre une IA locale.

### Serveur de tournoi

```bash
./dist/server
```

Lance le serveur UDP/TCP pour gérer les inscriptions aux tournois et les matchs en réseau.

Le serveur écoute sur le port **8080** et crée des salons de jeu en TCP dynamiquement.

## 🏗 Architecture

Le projet est organisé en plusieurs modules :

### Moteur de jeu (`src/`)

- **`chess.c/h`** — Logique du jeu d'échecs (règles, validation des coups)
- **`game.c/h`** — Gestion de la partie (état, historique)
- **`util.c/h`** — Utilitaires généraux
- **`ds.c/h`** — Structures de données

### Intelligence Artificielle

- **`minmax.c`** — Algorithme minimax pour l'évaluation des coups
- **`random.c`** — IA aléatoire
- **`human.c`** — Interface pour le joueur humain

Les IA sont compilées en tant que bibliothèques partagées (`.so`) pour une intégration flexible.

### Réseau et Serveur

- **`reseau.h`** — Définition du protocole de communication
- **`serveur.c`** — Serveur global UDP pour le lobby
- **`tournoi.c/h`** — Gestion des tournois à élimination directe

### Interface graphique

- **`main.c`** — Point d'entrée du jeu
- **`index.ts`** — (optionnel) Interface TypeScript/Web
- **`raylib.ts`** — Intégration Raylib

## 🏆 Module Tournoi

Le module tournoi implémente un système d'inscriptions et de matchmaking pour des tournois en ligne.

### Caractéristiques

- Tournois à élimination directe pour **2, 4 ou 8 joueurs**
- Inscription en ligne des participants
- Lancement automatique quand tous les joueurs sont inscrits
- Enregistrement des résultats de match

### API

```c
void init_tournoi(void);                              // Initialise le module
int create_tournoi(int max_players);                 // Crée un tournoi
int join_tournoi(int sock, const char *pseudo);      // Inscrit un joueur
void tournoi_match_result(const char *winner_pseudo); // Enregistre un résultat
const char *tournoi_last_error(void);                // Récupère le dernier erreur
```

### Workflow

1. **Initialisation** : `init_tournoi()`
2. **Création** : Appel du créateur : `create_tournoi(4)` pour 4 joueurs
3. **Inscription** : Les joueurs se connectent : `join_tournoi(sock, "Alice")`
4. **Lancement** : Quand tous les joueurs sont inscrits, le tournoi démarre automatiquement
5. **Matchmaking** : Le module dédié `salon` coordonne les paires
6. **Résultats** : `tournoi_match_result("Alice")` enregistre les gagnants

## 📦 Dépendances de compilation

| Cible | Dépendances |
|-------|------------|
| `make` | `raylib`, compilateur C standard |
| `make server` | `pthread` |

## 🔨 Commandes Makefile

```bash
make              # Compile le jeu et les IA
make server       # Compile le serveur de tournoi
make clean        # Supprime les fichiers compilés
```

## 🎯 Crédits

- **Icônes de pièces d'échecs** : <https://commons.wikimedia.org/wiki/Category:PNG_chess_pieces/Standard_transparent>
