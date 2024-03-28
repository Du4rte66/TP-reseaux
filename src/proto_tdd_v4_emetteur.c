/*************************************************************
* proto_tdd_v4 -  émetteur                                   *
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
#define FENETRE 4

/* =============================== */
/* Programme principal - émetteur  */
/* =============================== */
void envoi(paquet_t *paquet) {
    do {
        vers_reseau(paquet);
        depart_temporisateur(100);
    } while (attendre() != -1);
    arret_temporisateur();
    attendre();
    de_reseau(paquet);
}

int main(int argc, char* argv[])
{
    unsigned char message[MAX_INFO]; /* message de l'application */
    int taille_msg, evt, borne_inf, curseur, taille_fenetre, i, service;
    paquet_t tab_p[CAPACITE], pack, connect; /* paquet utilisé par le protocole et ack */
    taille_fenetre = FENETRE;
    borne_inf = 0;
    curseur = 0;
    service = 0;

    init_reseau(EMISSION);

    printf("[TRP] Initialisation reseau : OK.\n");
    printf("[TRP] Debut execution protocole transport.\n");

    /* lecture de donnees provenant de la couche application */
    de_application_mode_c(&service, message, &taille_msg);
    if (service == T_CONNECT) { /* connexion */
        connect.type = CON_REQ;
        envoi(&connect);

        switch (connect.type) {
            case CON_ACCEPT:
                vers_application_mode_c(T_CONNECT_ACCEPT, message, taille_msg);
                break;
            case CON_REFUSE:
                vers_application_mode_c(T_CONNECT_REFUSE, message, taille_msg);
                break;
            default:
                fprintf(stderr, "[ERREUR], reponse ni acceptée ni refusée");
                exit(1);
        }
    } else {
        fprintf(stderr, "[ERREUR] pas de demande de connexion\n");
        exit(2);
    }

    de_application_mode_c(&service, message, &taille_msg);
    if (service == T_DATA) { /* envoi de données */
        /* tant que l'émetteur a des données à envoyer */
        while ( taille_msg > 0 || borne_inf != curseur) {
            if (dans_fenetre(borne_inf, curseur, taille_fenetre) && taille_msg > 0){ /* crédit restant */
                /* construction prochain paquet puis emission */

                /* construction paquet */
                for (int i=0; i < taille_msg; i++) {
                    tab_p[curseur].info[i] = message[i];
                }
                tab_p[curseur].lg_info = taille_msg;
                tab_p[curseur].type = DATA;
                tab_p[curseur].num_seq = curseur;
                tab_p[curseur].somme_ctrl = generer_controle(tab_p[curseur]);

                /* remise a la couche reseau */
                vers_reseau(&tab_p[curseur]);

                if (curseur == borne_inf) {
                    depart_temporisateur(100);
                }
                inc(&curseur, CAPACITE);

                /* lecture de donnees provenant de la couche application */
                de_application_mode_c(&service, message, &taille_msg);

            }
            else { /* plus de credit */
                evt = attendre();
                if (evt == -1) {
                    de_reseau(&pack);
                    if (dans_fenetre(borne_inf, pack.num_seq, taille_fenetre) &&
                    verifier_controle(pack) && pack.type == ACK) { /* on decale la fenetre */
                        borne_inf = (pack.num_seq + 1) % CAPACITE;


                        if (curseur == borne_inf) {
                            arret_temporisateur();
                        }
                    }
                    /* sinon on ignore l'ack */
                }
                else { /* timer expired */
                    /* procedure GBN */
                    /* reemission des paquets */
                    i = borne_inf;
                    depart_temporisateur(100);
                    while (i != curseur) {
                        vers_reseau(&tab_p[i]);
                        inc(&i, CAPACITE);
                    }
                }
            }
        }
    } else {
        fprintf(stderr, "[ERREUR] pas de DATA\n");
        exit(3);
    }

    if (service == T_DISCONNECT) { /*déconnexion*/
        connect.type = CON_CLOSE;
        envoi(&connect);
    } else {
        fprintf(stderr, "[ERREUR] pas de demande de déconnexion\n");
        exit(4);
    }

    return 0;
}
