# Tanzanite — mod menu Geometry Dash

Mod [Geode](https://geode-sdk.org) en C++ (Windows, GD 2.2081, Geode 5.11.2).

**Touche `²` (au-dessus de Tab) pour ouvrir ou fermer le menu**, partout dans le jeu. `Échap` le ferme aussi.

## Catégories
| Catégorie | Options |
|---|---|
| Joueur | Noclip, Speedhack (vitesse 0.05 à 10, boutons 0.5x / 1x / 1.5x / 2x) |
| Niveau | Safe mode, Safe mode auto (activé par défaut), Pourcentage précis |
| Labels | Cheat/Legit, FPS, CPS, Précision noclip, Morts noclip, Essais, Sauts, Temps du niveau, Horloge, Message perso — coin, taille et opacité réglables |
| Infos | Raccourcis, version, lien du dépôt |

Le safe mode fait compter le niveau comme un test : rien n'est enregistré.
Le safe mode auto l'active tout seul dès que le noclip ou le speedhack sont allumés.

## Compiler
Chaque push sur GitHub lance le workflow **Build Tanzanite** (onglet Actions) qui produit `saltoback.tanzanite.geode`.
Mets ce fichier dans `Geometry Dash/geode/mods/` (il faut aussi le mod **Node IDs**, que Geode propose d'installer).

En local : Visual Studio 2022 (C++), CMake, Geode CLI + SDK (`geode sdk install`), puis `geode build`.

Remarque : le speedhack ne change pas la vitesse de la musique.

## Licence
Open source sous licence MIT (voir `LICENSE`).
