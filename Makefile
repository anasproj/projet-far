# ==============================================================================
# Makefile — Projet Chess Tournament (FAR)
# ==============================================================================

CC       = gcc
CFLAGS   = -Wall -Wextra -std=c11 -g -D_POSIX_C_SOURCE=200809L
LDFLAGS  = -lm -ldl -lpthread -lX11 -lXrandr -lXinerama -lXi -lXcursor -lGL

# Raylib
RAYLIB_CFLAGS  = $(shell pkg-config --cflags raylib 2>/dev/null)
RAYLIB_LDFLAGS = $(shell pkg-config --libs raylib 2>/dev/null)

# Répertoires
SRC_DIR     = src
ENGINE_DIR  = extern/chess-engine-c/src
BUILD_DIR   = build
ASSETS_DIR  = $(ENGINE_DIR)/assets

# Sources
SERVEUR_SRCS = src/serveur_main.c src/serveur_tournament.c src/salon_main.c src/salon_logic.c \
               $(ENGINE_DIR)/chess.c $(ENGINE_DIR)/ds.c

CLIENT_SRCS = src/client_main.c src/client_network.c src/client_ui.c src/client_input.c src/client_assets.c \
              $(ENGINE_DIR)/chess.c $(ENGINE_DIR)/ds.c

# Cibles
.PHONY: all clean assets help

all: assets build/serveur build/client

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# SERVEUR
build/serveur: $(BUILD_DIR) $(SERVEUR_SRCS)
	$(CC) $(CFLAGS) -I$(ENGINE_DIR) -I$(SRC_DIR) -o $@ $(SERVEUR_SRCS) $(LDFLAGS)

# CLIENT
build/client: $(BUILD_DIR) $(CLIENT_SRCS)
	$(CC) $(CFLAGS) $(RAYLIB_CFLAGS) -I$(ENGINE_DIR) -I$(SRC_DIR) -o $@ $(CLIENT_SRCS) $(RAYLIB_LDFLAGS) $(LDFLAGS)

# ASSETS
assets: $(BUILD_DIR)
	@if [ -d "$(ASSETS_DIR)" ]; then \
		cp -r $(ASSETS_DIR) $(BUILD_DIR)/assets; \
		echo "Assets copiés dans $(BUILD_DIR)/assets"; \
	fi

# NETTOYAGE
clean:
	rm -rf $(BUILD_DIR)

# AIDE
help:
	@echo "Utilisation :"
	@echo "  make all       — Compile le serveur et le client"
	@echo "  make clean     — Supprime les fichiers compilés"
	@echo ""
	@echo "Exécution :"
	@echo "  ./build/serveur              — Lance le serveur"
	@echo "  ./build/client [IP] [PSEUDO] — Lance un client"
