/**
 * @file hardware.c
 * @brief IoT tabanli Ongorucu Bakim (Predictive Maintenance) simulasyonu:
 *        para verme motorunun saglik durumunu (Hardware Health) yonetir.
 *
 * @details Gercek bir ATM'de bu deger sensor verisinden (motor akim
 *          tuketimi, titresim, sikisma sayaci vb.) hesaplanir. Bu simulasyon
 *          her basarili cekimde motoru rastgele bir oranda asindirarak
 *          gercekci bir "aginma" (wear) modeli sunar.
 */

#include <stdlib.h>
#include "../include/atm.h"

/**
 * @brief Para verme motorunu bir cekim sonrasi rastgele (%2 - %5) asindirir.
 *
 * @note rand() cagrisi icin tohum (seed) main() icinde bir kez srand() ile
 *       verilmelidir; bu fonksiyon kendi seed'ini ayarlamaz.
 *
 * @param ctx Saglik degerinin tutuldugu FSM context'i.
 */
void hardware_wear(AtmContext *ctx) {
    double range = HARDWARE_WEAR_MAX - HARDWARE_WEAR_MIN;
    double wear = HARDWARE_WEAR_MIN + ((double)rand() / (double)RAND_MAX) * range;

    ctx->hardware_health -= wear;
    if (ctx->hardware_health < 0.0) {
        ctx->hardware_health = 0.0;
    }
}

/**
 * @brief Motor sagliginin bakim esiginin (MAINTENANCE_THRESHOLD) altina
 *        dusup dusmedigini kontrol eder.
 *
 * @param ctx Kontrol edilecek FSM context'i.
 * @return Bakim gerekiyorsa 1, aksi halde 0.
 */
int hardware_needs_maintenance(const AtmContext *ctx) {
    return ctx->hardware_health < MAINTENANCE_THRESHOLD;
}

/**
 * @brief Motoru degistirme/bakim simulasyonu: sagligi tekrar %100'e
 *        sifirlar. Sadece admin paneli uzerinden cagrilmalidir.
 *
 * @param ctx Sifirlanacak FSM context'i.
 */
void hardware_reset(AtmContext *ctx) {
    ctx->hardware_health = HARDWARE_HEALTH_MAX;
}