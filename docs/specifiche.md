
La macchina gestisce 16 canali (i.e. uscite digitali) autonomi. 
Di questi, due possono avere una funzione speciale (il numero 8 e il numero 12). 
Uno e' l'uscita DAC di pressione (0/10V) che puo' assumere 3 valori oltre a "spento"; I valori sono codificati a colore (verde, giallo e rosso) e il valore effettivo fa parte dei parametri del sistema.
L'altro e' un sensore digitale (potenziometro) che cerca la posizione della pressa. Anche questo puo' avere 3 valori che corrispondonoa 3 posizioni.
In pratica il canale 12 puo' bloccare la pressa nella posizione rilevata dal potenziometro; oltre a un parametro macchina il margine dopo il quale viene fermato il cilindro e' sottoposto ad altri due parametri (utente) che ne indicano il margine in una direzione e nell'altra.
Il canale 8 invece mettendo la pressione muove la pressa (a meno che non sia bloccata).
La pressa puo' entrare in posizione perche' viene esercitata pressione oppure perche' manca e sta "cadendo" di nuovo alla posizione 0.
I programmi possono essere fino a 20. Ognuno ha una base dei tempi che corrisponde alla dimensione dell'unita' di tempo base (da .5 a 2 secondi). 
Per ogni canale se e' abilitato con un valore in un determinato istante l'uscita corrispondente (o la pressione, o il sensore) e' attiva.
Si puo' assegnare un nome ad ogni canale e ad ogni programma.
Devo poter importare ed esportare i programmi via USB e copiare un programma in un'altra posizione.

## Parametri macchina

 - tipo macchina: 16 = macchina singola, 16+16 macchina doppia (con due schede di potenza). Nel caso di 2 schede si usa un solo sensore potenziometro ma fino a due canali di pressione.
 - Gap costruttore: margine in entrambe le direzioni del sensore sul cilindro
 - Gap utente: margine in basso e in alto del sensore sul cilindro
 - Tipo potenziometro (festo 100, 200, 80)
 - Modo macchina: normale/carosello: il carosello e' una macchina rotativa con 2 piani inferiori e 1 superiore. Funziona a 16+16. In questo caso ci possono essere due sensori di posizione (macchina carosello 120 gradi) dove i due programmi (16+16) si alternano
 - Calibrazione offset pressostato

Alla macchina sono collegati anche dei pedali (fino a 3 che controllano l'aspirazione e il passaggio fra i programmi).
Nella macchina carosello i pedali agiscono sulle due forme inferiori, con un pedale di rotazione che scambia le forme. Ci sono due ingressi (sensori di prossimita) che indicano quale forma e' attiva.

## Note

 - La lettura analogica del potenziometro deve essere molto veloce perche' i movimenti possono essere brevi.
 - Deve andare in WiFi, senza obiettivi prominenti per ora.

## Domande

 - Quanto puo' essere lungo un programma?
 - I nomi dei canali sono diversi per ogni programma o gli stessi per tutta la macchina?
