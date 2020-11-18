// Aléatoire équilibré ?
// Auteurs : Allan Mercou, Léon Souffes

// gcc main.c -lm

#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>

#include <sys/mman.h>

#define TEST_RAND_MAX 500000
#define INTERVAL_ACTUALISATION_TABLEAU 10000
#define NOMBRE_PASSAGE 10000
#define NB_FORK 5

int main() {
    srand(time(NULL));
    pthread_mutex_t lock;

    // On crée un tableau d'entiers ayant pour capacité TEST_RAND_MAX cases.
    int *tableau = mmap(NULL, sizeof(int) * TEST_RAND_MAX,
                        PROT_READ | PROT_WRITE,
                        MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    // On crée NB_FORK forks pour effectuer nos calculs.
    for (int i = 0; i < NB_FORK; ++i) {
        pid_t pid = fork();
        if (pid == -1) {
            // Erreur.
            exit(1);
        } else if (pid == 0) {
            for (int j = 0; j < NOMBRE_PASSAGE; ++j) {
                int tableauTemporaire[TEST_RAND_MAX];
                // On initialise chaque case du tableau à 0.
                for (int k = 0; k < TEST_RAND_MAX; ++k) {
                    tableauTemporaire[k] = 0;
                }

                /* A chaque valeur aléatoire trouvée n, on incrémente la
                 * n-ième valeur du tableau temporaire de 1.
                 */
                for (int k = 0; k < INTERVAL_ACTUALISATION_TABLEAU; ++k) {
                    int nbRandom = rand() % TEST_RAND_MAX;
                    ++tableauTemporaire[nbRandom];
                }

                /* On verrouille avec un mutex la modification du tableau
                 * dans la mémoire partagée afin d'éviter des problèmes de
                 * concourance dans les modifications du tableau.
                 *
                 * (accès par plusieurs fork à la même case du tableau
                 *  simultanément)
                 */
                pthread_mutex_lock(&lock);
                for (int k = 0; k < TEST_RAND_MAX; ++k) {
                    tableau[k] += tableauTemporaire[k];
                }
                pthread_mutex_unlock(&lock);
            }

            exit(0);
        }
    }

    // On attend que chacun des processus fils soient terminés.
    for (int i = 0; i < NB_FORK; ++i) {
        wait(NULL);
    }

    int somme = 0;
    int plusBasse = TEST_RAND_MAX + 1;
    int plusHaute = 0;
    for (int i = 0; i < TEST_RAND_MAX; ++i) {
        somme += tableau[i];

        if (tableau[i] > plusHaute) {
            plusHaute = tableau[i];
        }
        if (tableau[i] < plusBasse) {
            plusBasse = tableau[i];
        }
    }

    int moyenneParCase = somme / TEST_RAND_MAX;

    double pre_ecart_type = 0.0;
    for (int i = 0; i < TEST_RAND_MAX; ++i) {
        pre_ecart_type +=
                (tableau[i] - moyenneParCase) * (tableau[i] - moyenneParCase);
    }

    double ecart_type = sqrt(pre_ecart_type / TEST_RAND_MAX);
    double coef_variation = ecart_type / moyenneParCase * 100;

    printf("Moyenne théorique par case : %d\n", moyenneParCase);
    printf("Ecart type : %.2f\n", ecart_type);
    printf("Coefficient de variation total : %.2f%c\n", coef_variation, '%');
    printf("Valeur la plus basse : %d\n", plusBasse);
    printf("Valeur la plus haute : %d\n", plusHaute);

    return 0;
}