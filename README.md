<p align="center">
  <strong>🇫🇷 Français</strong> &nbsp;|&nbsp; <a href="README.en.md">🇬🇧 English</a>
</p>

# iCompress

> Un archiveur et une suite de compression de données sans perte, ultra-léger et sans dépendance externe, écrit en C++17 moderne.

`iCompress` implémente les algorithmes canoniques de la théorie de l'information et de la compression de données à partir de zéro (*from scratch*). Il fournit un outil en ligne de commande (CLI) unifié, un moteur interactif d'évaluation de performances (*battle-test*), ainsi qu'une interface graphique desktop moderne.

---

## Fonctionnalités clés

- **Architecture 100% C++17 native** : Zéro dépendance externe. Flux binaires bit à bit (*bitstreams*), fenêtres glissantes et codeurs arithmétiques entièrement autonomes.
- **8 Codecs de référence intégrés** :
  - `Huffman` : Codage entropique à préfixe de longueur variable.
  - `RLE` (*Run-Length Encoding*) : Compactage des séquences d'octets identiques consécutifs.
  - `BWT` (*Burrows-Wheeler Transform*) : Permutation de blocs par tri lexicographique, index d'inversion et étage RLE.
  - `Arithmetic` : Codage arithmétique par subdivision d'intervalles sur entiers 32 bits.
  - `LZ77` : Dictionnaire à fenêtre glissante avec taille de recherche configurable.
  - `LZ78` : Dictionnaire dynamique basé sur un arbre de motifs (*trie*).
  - `LZW` (*Lempel-Ziv-Welch*) : Dictionnaire pré-rempli sur l'alphabet étendu 256 symboles.
  - `DEFLATE` : Combinaison hybride LZ77 + codage entropique de Huffman (mode `--auto` par défaut).
- **Garantie d'intégrité bit-exacte** : Chaque archive encapsule une somme de contrôle CRC32 (norme IEEE 802.3), automatiquement recalculée et vérifiée lors de l'extraction.
- **Validé sur 4 modalités de médias** : Évalué rigoureusement sur des flux Texte, Audio (formes d'ondes PCM), Image (rasters bruts RGB) et Vidéo (séquences temporelles YUV).
- **Double interface** : CLI POSIX réactive avec commandes et drapeaux courts, complétée par une interface graphique Studio minimaliste (`icompress ui`).

---

## Résultats des bancs d'essai (*Battle-Test*)

La suite a été éprouvée sur quatre modalités de données représentatives :

| Modalité | Fichier de référence | Codec optimal | Taux de compression | Réduction d'espace | Intégrité |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Texte** | `source.txt` (16,0 Ko) | **DEFLATE** | **26,5x** | **96,2%** | `PASS (CRC32)` |
| **Audio** | `tone.pcm` (15,6 Ko PCM 16-bit) | **DEFLATE** | **2,31x** | **56,8%** | `PASS (CRC32)` |
| **Image** | `raster.rgb` (14,1 Ko RGB brut) | **Arithmétique**| **1,11x** | **10,2%** | `PASS (CRC32)` |
| **Vidéo** | `video.yuv` (15,6 Ko frames YUV) | **BWT + RLE** | **14,7x** | **93,2%** | `PASS (CRC32)` |

---

## Spécification du conteneur (`ICMP`)

Toute archive `.ic` encapsule un en-tête binaire rigide de 20 octets suivi du flux de données compressées :

```
+---------------+---------------+---------------+-----------------------+-----------------------+
| Octets Magiques| Version (1B)  | Algorithme(1B)| Taille originale (8B) | CRC32 original (4B)   |
| "ICMP" (4B)   | 0x01          | 0x01 .. 0x08  | uint64_t (Little-End) | uint32_t (Little-End) |
+---------------+---------------+---------------+-----------------------+-----------------------+
|                               Flux de données compressées ...                                 |
+-----------------------------------------------------------------------------------------------+
```

---

## Compilation et génération (Build)

### Prérequis

- Un compilateur compatible **C++17** :
  - **Windows** : MinGW-w64 (`g++` >= 8) ou Microsoft Visual Studio (MSVC >= 2019).
  - **Linux** : `g++` (>= 8) ou `clang++` (>= 7).
  - **macOS** : Apple Clang ou `brew install gcc`.
- *(Optionnel)* CMake >= 3.16 ou GNU Make.

---

### Compilation sous Windows

#### Option 1 : Script automatisé en 1 clic (Recommandé)
Le dépôt inclut le script `build.bat` qui détecte automatiquement votre environnement MinGW-w64 (dans le `PATH` ou dans `C:\mingw64\bin`), compile les ressources d'icône Windows et génère tous les exécutables avec optimisations de niveau maximal (`-O3`) :

```cmd
.\build.bat
```

#### Option 2 : Avec CMake
```cmd
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

#### Option 3 : Avec GNU Make
```cmd
make
:: ou avec make.bat si make n'est pas dans le PATH
.\make.bat
```

---

### Compilation sous Linux et macOS

#### Option 1 : Avec CMake (Recommandé)
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

#### Option 2 : Avec GNU Make
```bash
make all
```

---

### Exécutables générés dans le dossier `bin/`

À l'issue de la compilation, les binaires suivants sont créés dans `bin/` :
- `bin/icompress` (ou `icompress.exe`) : L'outil principal en ligne de commande.
- `bin/icompress-gui` (ou `icompress-gui.exe`) : L'application graphique Studio pour Windows.
- `bin/test_codecs` (ou `test_codecs.exe`) : La suite de tests unitaires validant chaque algorithme.
- `bin/battle_test` (ou `battle_test.exe`) : La suite de bancs d'essai comparative multi-codecs.

---

## Installation

### Installation sous Windows

#### Méthode 1 : Ajout au PATH utilisateur (Recommandé)
Pour pouvoir exécuter `icompress` depuis n'importe quel terminal PowerShell ou Invite de commandes :

Ouvrez un terminal PowerShell dans le dossier du projet et exécutez :
```powershell
[Environment]::SetEnvironmentVariable("Path", $env:Path + ";$((Get-Item .).FullName)\bin", [EnvironmentVariableTarget]::User)
```
*(Redémarrez votre terminal pour que le nouveau PATH soit pris en compte).*

#### Méthode 2 : Installation via CMake
Vous pouvez installer les exécutables dans le répertoire de votre choix (par exemple `%LOCALAPPDATA%\Programs\iCompress` ou `C:\Program Files\iCompress`) :
```cmd
cmake --install build --prefix "%LOCALAPPDATA%\Programs\iCompress"
```
Ajoutez ensuite `%LOCALAPPDATA%\Programs\iCompress\bin` à votre variable d'environnement `PATH`.

#### Méthode 3 : Copie manuelle
Copiez simplement `bin\icompress.exe` et `bin\icompress-gui.exe` dans un dossier déjà présent dans votre `PATH` (ex: `C:\Windows\System32` avec droits administrateur, ou votre dossier personnel de scripts).

---

### Installation sous Linux / macOS

#### Méthode 1 : Installation système via CMake
```bash
sudo cmake --install build --prefix /usr/local
```

#### Méthode 2 : Installation manuelle directe
```bash
sudo cp bin/icompress /usr/local/bin/
sudo chmod +x /usr/local/bin/icompress
```

#### Vérifier l'installation
Une fois installé, vérifiez l'accès à la commande :
```bash
icompress --help
```

---

## Guide d'utilisation (CLI & GUI)

### 1. Compresser un fichier

Compresser un fichier avec l'algorithme recommandé par défaut (`deflate`) :
```bash
icompress compress document.pdf -o document.ic
```

Compresser en spécifiant un algorithme particulier :
```bash
# Codage par dictionnaire LZ77
icompress -c video.raw -m lz77 -o video.ic

# Transformation de Burrows-Wheeler + RLE
icompress -c texte.txt -m bwt -o texte.ic

# Codage Arithmétique
icompress -c audio.wav -m arithmetic -o audio.ic

# Codage de Huffman
icompress -c source.c -m huffman -o source.ic
```

Algorithmes disponibles pour l'option `-m` / `--method` :
- `deflate` (défaut / recommandé)
- `huffman`
- `rle`
- `bwt`
- `arithmetic`
- `lz77`
- `lz78`
- `lzw`

---

### 2. Décompresser et extraire une archive

Extraire vers un fichier de destination spécifique :
```bash
icompress extract archive.ic -o fichier_restaure.txt
```

Ou en syntaxe abrégée :
```bash
icompress -d archive.ic
```
*(L'intégrité CRC32 est vérifiée en temps réel avant d'écrire le fichier sur le disque).*

---

### 3. Tester l'intégrité d'une archive sans extraction

Permet de vérifier qu'une archive n'est pas corrompue sans toucher au disque :
```bash
icompress test archive.ic
# ou
icompress -t archive.ic
```

---

### 4. Inspecter les métadonnées d'une archive

Affiche le nom de l'algorithme utilisé, la taille originale et la somme CRC32 attendue :
```bash
icompress list archive.ic
# ou
icompress -l archive.ic
```

---

### 5. Lancer un comparatif de performance (*Benchmark*)

Exécute simultanément les 8 algorithmes sur un fichier donné, affiche les ratios de compression, les gains d'espace et les temps d'exécution, puis identifie le codec optimal :
```bash
icompress benchmark donnees.dat
```

---

### 6. Lancer l'interface graphique Studio

Sous Windows, vous pouvez démarrer l'interface graphique dédiée soit en double-cliquant sur `bin/icompress-gui.exe`, soit via la commande :
```bash
icompress ui
```

---

## Validation et tests unitaires

Le projet intègre une suite de tests automatisée pour garantir la fiabilité absolue et l'absence de régression :

```bash
# Vérification des 8 codecs et validation bit-exacte
./bin/test_codecs

# Exécution du banc d'essai complet multi-modalités
./bin/battle_test
```

Si vous utilisez CMake :
```bash
ctest --test-dir build --output-on-failure
```

---

## Arborescence du projet

```
iCompress/
├── CMakeLists.txt        # Configuration CMake multi-plateforme
├── Makefile              # Makefile pour GNU Make
├── build.bat             # Script de compilation Windows 1-clic
├── make.bat              # Script d'exécution make Windows
├── include/              # En-têtes C++ (.hpp)
│   ├── arithmetic.hpp
│   ├── benchmark.hpp
│   ├── bitstream.hpp
│   ├── bwt.hpp
│   ├── codec.hpp
│   ├── container.hpp
│   ├── crc32.hpp
│   ├── deflate.hpp
│   ├── huffman.hpp
│   ├── lz77.hpp
│   ├── lz78.hpp
│   └── lzw.hpp
├── src/                  # Implémentations C++ (.cpp) et ressources
│   ├── main.cpp          # Point d'entrée de l'outil CLI
│   ├── gui_main.cpp      # Application Studio (Desktop GUI)
│   ├── resources.rc      # Fichier de ressources Windows (icône)
│   └── ...
├── tests/                # Suites de tests et benchmarks
│   ├── test_codecs.cpp
│   └── battle_test.cpp
├── ui/                   # Éléments visuels, logos SVG et icônes
├── LICENSE               # Licence MIT
├── README.md             # Documentation en français
└── README.en.md          # Documentation en anglais
```

---

## Licence

Distribué sous licence **MIT**. Consultez le fichier [LICENSE](LICENSE) pour plus d'informations.
