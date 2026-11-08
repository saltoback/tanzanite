# Tanzanite — mod menu Geometry Dash

Mod [Geode](https://geode-sdk.org) en C++ (Windows, GD 2.2081, Geode 5.11.2).

## Fonctions
| Option | Effet |
|---|---|
| Noclip | Tu traverses les obstacles |
| Speedhack | Vitesse du jeu (0.05 à 10, champ à droite) |
| Safe mode | Le niveau compte comme un test : rien n'est enregistré |
| Safe mode auto | Active le safe mode tout seul dès qu'un cheat est allumé (activé par défaut) |
| Pourcentage précis | Affiche le % avec 2 décimales |

Pour ouvrir le menu : bouton violet **Tanzanite** en bas du menu principal, ou à droite dans le menu pause.

## Compiler le mod (sans rien installer)
1. Crée un dépôt GitHub et envoie-y ce dossier.
2. Va dans l'onglet **Actions** : le workflow « Build Tanzanite » se lance tout seul.
3. Quand il a fini, télécharge l'artifact **Tanzanite** : il contient `saltoback.tanzanite.geode`.
4. Mets ce fichier dans `Geometry Dash/geode/mods/` et relance le jeu.

## Compiler en local
Installe Visual Studio 2022 (C++), CMake, puis le Geode CLI et le SDK (`geode sdk install`), et lance :
```
geode build
```

## Si ça ne compile pas
Si ta version de GD ou de Geode est plus récente, mets à jour `"gd"` et `"geode"` dans `mod.json`.
Les noms des fonctions du jeu changent parfois d'une version à l'autre.

Remarque : le speedhack ne change pas la vitesse de la musique.

## Licence
Open source sous licence MIT (voir `LICENSE`).
