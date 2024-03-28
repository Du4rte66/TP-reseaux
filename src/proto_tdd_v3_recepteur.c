/*************************************************************
* proto_tdd_v3 -  récepteur                                  *
* TRANSFERT DE DONNEES  v3                                   *
*                                                            *
* Protocole sans contrôle de flux, sans reprise sur erreurs  *
*                                                            *
* E. Lavinal - Univ. de Toulouse III - Paul Sabatier         *
**************************************************************/

#include <stdio.h>
#include "application.h"
#include "couche_transport.h"
#include "services_reseau.h"
#define CAPACITE 8

/* =============================== */
/* Programme principal - récepteur */
/* =============================== */
int main(int argc, char* argv[])
{
    unsigned char message[MAX_INFO]; /* message pour l'application */
    paquet_t paquet, ack; /* paquet utilisé par le protocole et ack */
    int fin = 0; /* condition d'arrêt */
    int paquet_attendu = 0;

    ack.type = ACK;
    ack.num_seq = CAPACITE - 1;
    ack.lg_info = 0;
    ack.somme_ctrl = generer_controle(ack);

    init_reseau(RECEPTION);

    printf("[TRP] Initialisation reseau : OK.\n");
    printf("[TRP] Debut execution protocole transport.\n");

    /* tant que le récepteur reçoit des données */
    while ( !fin ) {
        // attendre(); /* optionnel ici car de_reseau() fct bloquante */
        de_reseau(&paquet);
        if (verifier_controle(paquet)) {
            if (paquet.num_seq == paquet_attendu) {
                /* extraction des donnees du paquet recu */
                for (int i=0; i < paquet.lg_info; i++) {
                    message[i] = paquet.info[i];
                }
                ack.type = ACK;
                ack.num_seq = paquet.num_seq;
                ack.lg_info = 0;
                ack.somme_ctrl = generer_controle(ack);

                /* remise des données à la couche application */
                fin = vers_application(message, paquet.lg_info);
                inc(&paquet_attendu, CAPACITE);
            }
            vers_reseau(&ack);
        }
    }
    vers_reseau(&ack);

    printf("[TRP] Fin execution protocole transport.\n");
    return 0;
}
