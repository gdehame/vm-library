# ArSys-Proj

- Répartition des fichiers:
	+ `vm.h` et `vm.c` implémentent les fonctions de la vm
	+ `rr.h` et `rr.c` implémentent la fonction de scheduling pour Round Robin
	+ `hm_main.h` et `hm_main.c` implémentent les fonctions d'interfaces du heap memory manager
	+  `hmc.h` et `hmc.c` implémentent les fonctions du heap memory manager utilisant les canarys
	+ `hmgp.h` et `hmgp.c` implémentent les fonctions du heap memory manager utilisant les pages de gardes
	
- Fonctions de la vm:
	+ `yield(n)` qui met en pause l'exécution d'un uthread jusqu'à la fin du quantum courant et empêche son scheduling durant les `n` quantums suivants
	+ `create_vcpu(n)` qui créé `n` vCPUs pour le prochain lancement de VM. A noter qui si l'on exécute `create_vcpu(n)` puis `create_vcpu(m)` avant `run()`, alors la VM aura m vCPUs
	+ `config_scheduler(q, s)` qui configure le scheduler avec un quantum de `q` secondes et un mode de scheduling attribué en fonction de `s` (Round Robin pour `s=1` et CFS pour `s=0`)
	+ `create_uthread(f)` créé un uthread pour la prochaine VM qui exécutera la fonction `f`
	+ `configure_hmm(n)` qui configure le heap memory manager, et permet de choisir entre la sécurisation par canarys ou par pages de gardes (canarys si `n=0` et pages de gardes si `n=1`)
	+ `run()` qui lance la VM (bloquant)

- Utilisation:
	Pour que la VM fonctionne, il faut tout d'abord configurer le heap memory manager, et il faut configurer le scheduler avant de créer des uthread et de lancer la VM, des messages d'erreurs seront envoyés dans la sortie standard si ce n'est pas la fonction appelée ne s'exécutera pas.
  Un exemple d'utilisation de la librarie est fourni dans `exemple.c` 

- Fonctionnalités manquantes:
	+ Fréquences et classes pour les heap memory manager
