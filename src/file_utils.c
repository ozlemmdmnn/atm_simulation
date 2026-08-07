/**
 * @file file_utils.c
 * @brief Hesap verilerinin binary dosyada guvenli sekilde kalicilastirilmasi
 *        (persistence) ile ilgili fonksiyonlarin implementasyonu.
 *
 * @details Account struct'i icinde pointer/dinamik bellek bulunmadigi icin
 *          tum Bank yapisi tek bir fwrite/fread cagrisiyla dogrudan
 *          diskten okunup diske yazilabilir. Bu yaklasim hem basit hem de
 *          bellek sizintisi riskini ortadan kaldiran bir tasarimdir.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include "../include/atm.h"

/**
 * @brief data/ klasorunun mevcut oldugundan emin olur; yoksa olusturur.
 */
void ensure_data_directory(void) {
#if defined(_WIN32)
    mkdir(DATA_DIR);
#else
    if (mkdir(DATA_DIR, 0755) != 0 && errno != EEXIST) {
        fprintf(stderr, "UYARI: '%s' klasoru olusturulamadi (errno=%d).\n", DATA_DIR, errno);
    }
#endif
}

/**
 * @brief Hesaplari ACCOUNTS_FILE dosyasindan Bank yapisina yukler.
 *
 * Dosya yoksa (ilk calistirma) hata vermez, bos bir banka ile devam eder.
 *
 * @param bank Doldurulacak Bank yapisina pointer.
 * @return Basarili ise 1, okuma hatasi olustuysa 0.
 */
int load_accounts(Bank *bank) {
    bank->count = 0;

    FILE *fp = fopen(ACCOUNTS_FILE, "rb");
    if (!fp) {
        /* Ilk calistirma senaryosu: dosya henuz yok, hata sayilmaz. */
        return 1;
    }

    Account temp;
    int n = 0;
    while (n < MAX_ACCOUNTS && fread(&temp, sizeof(Account), 1, fp) == 1) {
        bank->accounts[n] = temp;
        n++;
    }

    int had_error = ferror(fp) != 0;
    fclose(fp);

    bank->count = n;
    return had_error ? 0 : 1;
}

/**
 * @brief Bank yapisindaki tum hesaplari ACCOUNTS_FILE dosyasina yazar.
 *
 * Once gecici bir dosyaya yazip basariliysa asil dosyanin yerine
 * tasima (atomic-ish) yaklasimi yerine, portfolyo/egitim projesi
 * kapsaminda basitlik icin dogrudan "wb" modunda yazilir.
 *
 * @param bank Kaydedilecek Bank yapisina const pointer.
 * @return Basarili ise 1, yazma hatasi olustuysa 0.
 */
int save_accounts(const Bank *bank) {
    ensure_data_directory();

    FILE *fp = fopen(ACCOUNTS_FILE, "wb");
    if (!fp) {
        fprintf(stderr, "HATA: '%s' dosyasi yazma icin acilamadi!\n", ACCOUNTS_FILE);
        return 0;
    }

    size_t written = fwrite(bank->accounts, sizeof(Account), (size_t)bank->count, fp);
    int ok = (written == (size_t)bank->count);

    if (fclose(fp) != 0) {
        ok = 0;
    }

    if (!ok) {
        fprintf(stderr, "HATA: Hesap verileri diske tam olarak yazilamadi!\n");
    }

    return ok;
}

/**
 * @brief Admin panelinde gerceklestirilen kritik islemleri denetim (audit)
 *        amaciyla bir log dosyasina ekler.
 *
 * @param action Loglanacak islemi aciklayan metin.
 */
void log_admin_action(const char *action) {
    ensure_data_directory();

    FILE *fp = fopen(ADMIN_LOG_FILE, "a");
    if (!fp) {
        return; /* Loglama basarisiz olsa da uygulama akisini bozmamali */
    }

    char timestamp[DATE_LEN];
    get_current_timestamp(timestamp, sizeof(timestamp));

    fprintf(fp, "[%s] %s\n", timestamp, action);
    fclose(fp);
}