# IMAC_ISLAND_VIEWER_Romane-Chloe

# Rapport de projet

**Projet réalisé par :** Romane Marteau et Chloé Chabaud

---

## Question 1 — Bruit fractal (Octave Noise) (Chloé)

J’ai utilisé le premier document fourni dans le sujet :  
https://thebookofshaders.com/13/?lan=fr

L’objectif de cette fonction est de générer un relief naturel (type terrain ou île) en combinant plusieurs couches de bruit.

Les octaves sont une version de bruit à différentes échelles :  
• les premières octaves donnent les grosses formes du terrain (montagnes, continents)  
• les octaves suivantes ajoutent des détails de plus en plus fins

Au final, on superpose tout ça pour obtenir un terrain plus naturel.

On définit :

**Amplitude**  
L’amplitude sert à dire “combien cette octave influence le résultat”. À chaque octave, on réduit l’amplitude avec le gain (souvent < 1), sinon les hautes fréquences prennent trop de place.

**Fréquence**  
La fréquence sert à définir la taille des détails : à chaque octave, on augmente la fréquence.

On construit le résultat avec le bruit à une certaine fréquence, pondéré par son amplitude. Chaque octave ajoute une contribution différente au résultat.

À la fin, on normalise le résultat car les valeurs peuvent dépasser un certain intervalle. On normalise par rapport aux amplitudes, car chaque itération n’a pas le même poids.

---

### Problème rencontré

Au début, j’avais mis une amplitude < 1. Le bruit “fonctionnait”, jusqu’à ce que j’ajoute les couleurs, où j’ai eu un gros problème que j’ai eu du mal à retrouver.

En fait, en mettant une amplitude < 1, je multipliais par ce facteur à chaque octave, donc je réduisais fortement le signal, ce qui donnait un effet inattendu en fonction de la hauteur (zones trop sombres et/ou mal réparties).

---

## Question 2 — Génération de heightmap et couleurs (Chloé)

But : générer une couleur en fonction de la hauteur du point et de sa distance au centre.

J’ai choisi d’utiliser un masque avec une fonction linéaire (possible avec une fonction de la bibliothèque, mais j’ai préféré, pour ma propre compréhension, l’écrire moi-même).

En fonction de la distance, le masque a plus ou moins d’effet. Au-delà d’un certain rayon, c’est de l’eau.

On calcule la distance entre le point et le centre de la carte :  
• plus on est proche du centre → terrain élevé  
• plus on s’éloigne → terrain plus bas

Le fait de mettre le masque au carré permet de renforcer l’effet, sinon la transition était trop faible visuellement.

Le terrain final est obtenu en multipliant :

**height = noise × masque**

• le bruit donne la forme  
• le masque donne la structure d’île

---

### Difficultés rencontrées

J’ai eu beaucoup de mal à commencer, mais j’ai reçu l’aide d’Agathe, qui m’a expliqué le concept et j’ai pu commencer. J’ai galéré à trouver par quoi il fallait multiplier au début.

Concernant les couleurs, on voulait un dégradé propre donc on a réutilisé la methode de dégradés dans l'espace de couleur Lab ( utilisé dans le workshop de Jules). Il fallait donc adapter cette méthode.

J’ai créé des plages de couleurs, et j’en ai ajouté ensuite avec différentes teintes pour avoir un meilleur rendu.

---

### Problèmes rencontrés

J’ai eu longtemps des problèmes avec les intervalles des vecteurs de couleurs qui devaient passer dans les fonctions, je m’emmêlais. Puis, pour le pourcentage appliqué, il fallait mapper en fonction de l’intervalle où on se trouvait (solution trouvée grâce à Jules). Je suis passée par énormément de couleurs non voulues avant d’avoir un résultat satisfaisant.

---

## Question 3 — Poisson Disk (Romane)

J’ai eu plusieurs problèmes dans la compréhension de l’algorithme.

Une première vidéo (https://www.youtube.com/watch?v=flQgnCUxHlw) m’a aidée à comprendre pas à pas le papier de recherche. Le code était tout de même truffé de fautes et écrit dans un langage Processing que je ne maîtrisais pas trop, avec l'aide de Benoit j'ai pu cependant corriger les erreurs.

J’ai donc choisi de suivre une deuxième vidéo (https://www.youtube.com/watch?v=7WcmyxyFO7o) pour avoir les bonnes méthodes et les bonnes pratiques.

Cette méthode permet de générer des points répartis de manière uniforme, avec la possibilité de réduire ou augmenter le nombre de points, ainsi que de modifier le rayon.

---

## Question 4 — Placement des objets (Romane)

Les deux questions sur les objets ont été réalisées sans la map de Chloé, donc avec une île assez moche (très plate). D’où les sliders minimum_z qui commencent dans le négatif pour permettre de placer des arbres jusqu’à la base de la map.

On peut choisir les valeurs de z (min ou max), ce qui permet de contrôler où les objets apparaissent.

---

### Améliorations (Romane)

J’ai importé deux meshes 3D (deux types d’arbres) avec l’aide de Kellian, séparés en fonction des valeurs de z. Les meshes ont été trouvés sur free3D.fr. Ce sont des fichiers .obj triangulés avec des fichiers .mtl pour les textures et couleurs.

La difficulté venait du fait que les meshes de base n’étaient pas tous en .obj et étaient très lourds, ce qui compliquait leur intégration.

Les sapins sont placés en altitude et les arbres normaux en bas. Cette séparation peut être modifiée en fonction de la map : si les montagnes sont basses, on peut baisser l’altitude des sapins ou même les faire disparaître avec un bouton.

Il est aussi possible de changer la taille des arbres.

Le but était d’obtenir une map la plus harmonieuse possible : si la carte est très dense, on peut réduire les arbres, si elle est trop basse, on peut ajuster les sapins, afin de permettre à l’utilisateur de vraiment façonner son île.

---

## Organisation du travail

Nous sommes parties sur deux questions chacune, avec des points de rendez-vous réguliers pour montrer l’avancement du projet, demander des avis et poser des questions.

---

## Gestion du Git

Nous avons travaillé sur deux branches différentes.

Chloé a commencé sur une branche Noise ( une branche par exercice était initialement prévue, mais finalement, pour éviter les erreurs, tout a été regroupé sur une seule branche). Commits faits à chaque fois qu’une question était terminée ou qu’une amélioration/correction était complète.

Romane a effectué une branche par question ( 3 en tout avec amélioration) : 1 sur le poisson (placement en fonction de x et y), 1 pour le placement des arbres en fonction de z, 1 pour l'amélioration (les meshs 3d et leurs propriétés)

Nous faisions des commits réguliers sur la branche principale (main), puis nous avons décidé de merger uniquement à la fin du projet et de gérer les conflits ensemble.

Enfin le marge final a été fait ensemble sur le pc de Romane pour régler ensemble les conflicts
