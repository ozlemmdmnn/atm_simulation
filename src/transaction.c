/**
 * @file transaction.c
 * @brief Hesap bazli islem gecmisi (son N islem) yonetimi.
 *
 * @details Her hesap kendi icinde sabit boyutlu (MAX_HISTORY) dairesel
 *          bir tampon tutar. Bu sayede dinamik bellek kullanmadan, sabit
 *          ve ongorulebilir bellek tuketimi ile son islemler saklanabilir.
 */

#include <stdio.h>
#include <string.h>
#include "../include/atm.h"

/**
 * @brief Hesabin islem gecmisine yeni bir kayit ekler.
 *
 * history_count, MAX_HISTORY sinirina ulastiginda dairesel tampon mantigi
 * ile en eski kaydin uzerine yazilir (modulo aritmetigi kullanilir).
 *
 * @param acc    Kaydin ekelenecegi hesap.
 * @param type   Islem turu (orn. "Para Yatirma", "Havale Gonderme").
 * @param amount Islem tutari.
 */
void add_transaction(Account *acc, const char *type, double amount) {
    int index = acc->history_count % MAX_HISTORY;

    strncpy(acc->history[index].type, type, TRANSACTION_TYPE_LEN - 1);
    acc->history[index].type[TRANSACTION_TYPE_LEN - 1] = '\0';

    acc->history[index].amount = amount;
    get_current_timestamp(acc->history[index].timestamp, DATE_LEN);

    acc->history_count++;
}

/**
 * @brief Hesabin son islemlerini en yeniden en eskiye dogru listeler.
 * @param acc Gecmisi goruntulenecek hesap.
 */
void print_transaction_history(const Account *acc) {
    printf("\n===== ISLEM GECMISI =====\n");

    if (acc->history_count == 0) {
        printf("Henuz hicbir islem yapilmamis.\n");
        return;
    }

    int total_stored = (acc->history_count < MAX_HISTORY) ? acc->history_count : MAX_HISTORY;

    printf("%-20s %-22s %12s\n", "Tarih", "Islem Turu", "Tutar (TL)");
    printf("--------------------------------------------------------\n");

    /* En son eklenen islemden baslayarak geriye dogru yazdir */
    for (int i = 0; i < total_stored; i++) {
        int seq = acc->history_count - 1 - i;      /* en yeniden eskiye siralama */
        int index = seq % MAX_HISTORY;
        printf("%-20s %-22s %12.2f\n",
               acc->history[index].timestamp,
               acc->history[index].type,
               acc->history[index].amount);
    }
    printf("--------------------------------------------------------\n");

    if (acc->history_count > MAX_HISTORY) {
        printf("(Sadece son %d islem gosterilmektedir.)\n", MAX_HISTORY);
    }
}