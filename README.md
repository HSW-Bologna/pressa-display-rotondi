## Setup

git submodule update --init

sudo apt-get install scons
sudo apt-get install libsdl2-dev

## TODO

 - try hardware rotation (https://components.espressif.com/components/espressif/esp_lvgl_port/versions/2.6.0)

 - ~L'impostazione della pressione deve essere in bar~
 - L'impostazione del livello e' in mm (+- 1 mm)
 - ~12 + 8 discesa, solo 12 salita~
 - il 4-20 copre una certa distanza in mm, configurabile
 - ~Serve una calibrazione del punto 0 (tra 4 e 20 ma)~
 - ~ingresso 1 = start; ingresso 2 = emergenza~
 - ~Gli ingressi configurabili muovono le uscite corrispondenti, condivise anche con i programmi~
 - ingresso 3:
    - macchina tradizionale: on/off flip flop ma si spegne quando parte il programma (uscita 3)
    - doppio davanti: on/off flip flop
 - ingresso 4:
    - macchina tradizionale: Niente
    - doppio davanti: on/off flip flop uscita 9, si spegne quando parte il programma
 - ingresso 12:
    - uscita 3 e 9 impulsive
 
 - I canali normali sono 15, non 14

 10V -> 6bar
 5V -> 3.12bar
 2.5V -> 1.57bar


## TODO
 - I parametri relativi all’offset , al valore di lunghezza del sensore , il punto zero e la scelta del tipo di macchina devono essere in un menu a parte di configurazione macchine sotto password 1970 perché vale per tutti i programmi 
    Sposta i parametri sotto "S"

 - Procedura per il punto zero non è molto chiara bisogna ridefinirla, ti dico come la facciamo ora, facciamo scendere il piano superiore su quello inferiore, e per questo ci deve essere un tasto apposta che comanda l’uscita numero 8, in più devo aver la possibilità di agire sulla valvola proporzionale (uscita analogica) per poter schiacciare ulteriormente quindi un altro tasto che memorizza la quota e la usa come zero, 
    Fondamentalmente aggiungi il tasto di calibrazione nel test
  
 - Quando programmi i canali più in basso devi scorrere lo schermo ma così facendo non hai il riferimento dei canali superiori
 - Dopo l’ultimo step il programma deve finire e non continuare a scorrere anche gli spazi vuoti perché così si perde tempo
 - Non fa la sfumatura ne in salita, ne in discesa
