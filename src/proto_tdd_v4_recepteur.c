/*************************************************************
* proto_tdd_v4 -  récepteur                                  *
* TRANSFERT DE DONNEES  v4                                   *
*                                                            *
* Protocole sans contrôle de flux, sans reprise sur erreurs  *
*                                                            *
* E. Lavinal - Univ. de Toulouse III - Paul Sabatier         *
**************************************************************/

#include <stdio.h>
#include <stdlib.h>
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
    int fin, paquet_attendu, connect, service; /* condition d'arrêt */
    fin = 0;
    paquet_attendu = 0;
    connect = 0;
    service = 0;

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
        switch(paquet.type) {
            case DATA:
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
                        fin = vers_application_mode_c(T_DATA, message, paquet.lg_info);
                        inc(&paquet_attendu, CAPACITE);
                    }
                    vers_reseau(&ack);
                }
                break;
            case CON_REQ:
                if (connect || (vers_application_mode_c(T_CONNECT, message, paquet.lg_info) == T_CONNECT_ACCEPT)) {
                    ack.type = CON_ACCEPT;
                    vers_reseau(&ack);
                    connect = 1;
                }
                else {
                    ack.type = CON_REFUSE;
                    vers_reseau(&ack);
                    fprintf(stderr, "[ERREUR] demande de connexion refusée\n");
                    exit(1);
                }
                break;
            case CON_CLOSE:
                service = T_DISCONNECT;
                vers_application_mode_c(service, message, paquet.lg_info);
                ack.type = CON_CLOSE_ACK;
                vers_reseau(&ack);
                fin = 1;
                break;
            default:
                fprintf(stderr, "[ERREUR] type de paquet ni DATA, ni REQ, ni CLOSE");
                exit(2);
        }

    }
    vers_reseau(&ack);

    printf("[TRP] Fin execution protocole transport.\n");
    return 0;
}
