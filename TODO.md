# TODO

- [ ] Passer en mode filaire avec le MCP2515
- [x] Schéma de câblage en HTML (`docs/wiring.html`) : une version modules en couleur, une version blueprint, deux liens pour passer de l'une à l'autre
- [x] Captures du schéma (couleur et blueprint) dans `docs/resources/`, générées par `mise run wiring`, affichées dans `docs/wiring.md`
- [x] Qualité de code : hooks prek (clang-format pour le C, Biome pour le TS/CSS/JSON, tsc, tests hôte)
- [ ] Activer les hooks : `prek install` (une fois, dans ce clone)
- [ ] Éthanol sans câblage : lancer « Scan ECU RAM » moteur tournant en notant l'E% d'OFT, puis lire les candidats dans les logs
- [ ] Mode sport : lancer « Listen CAN » (45 s sans rien toucher, puis appuis répétés sur le bouton), puis lire les candidats dans les logs
- [ ] Vérifier dans la caisse : polling multi-PID (log `polling every PID at once`), jauges plus réactives
- [ ] Vérifier dans la caisse : Send log vers le téléphone, accusé de réception et retour aux jauges
- [ ] Vérifier dans la caisse : nom du dongle lu et affiché sur READY et OFFLINE, reconnexion depuis OFFLINE sans reboot
- [ ] Vérifier sur la carte : encodeur demi-pas (un cran = une ligne), loader fluide
- [ ] Héberger la page web sur gt.ombrage.space (`Dockerfile` à la racine, derrière le proxy https)
- [x] Doc câblage, section MCP2515 : plus de références au code Go, `DATA_SOURCE` et les registres CNF à la place
