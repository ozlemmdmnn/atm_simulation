/**
 * @file account.c
 * @brief Temel bankacilik islemlerinin (yatirma, cekme, havale, bakiye,
 *        faiz simulasyonu, QR ile cekim) implementasyonu.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/atm.h"

/**
 * @brief Verilen hesap numarasina sahip AKTIF hesabi bulur.
 *
 * @param bank           Aranacak Bank yapisi.
 * @param account_number Aranan hesap numarasi.
 * @return Bulunursa Account pointer'i, aksi halde NULL.
 */
Account *find_account_by_number(Bank *bank, int account_number) {
    for (int i = 0; i < bank->count; i++) {
        if (bank->accounts[i].account_number == account_number && bank->accounts[i].is_active) {
            return &bank->accounts[i];
        }
    }
    return NULL;
}

/**
 * @brief Kullaniciya para yatirma islemini gerceklestirir.
 * @param bank Diske kaydetmek icin Bank yapisi.
 * @param acc  Islem yapilacak hesap.
 */
void deposit(Bank *bank, Account *acc) {
    printf("\n===== PARA YATIRMA =====\n");
    printf("Guncel Bakiye: %.2f TL\n", acc->balance);

    double amount = read_positive_double("Yatirilacak Tutar: ");

    acc->balance += amount;
    add_transaction(acc, "Para Yatirma", amount);

    if (save_accounts(bank)) {
        printf("Islem basarili! Yeni Bakiye: %.2f TL\n", acc->balance);
    } else {
        printf("Islem yapildi ancak kayit sirasinda bir sorun olustu!\n");
    }
}

/**
 * @brief Bugunku gunluk cekim sayacini gerekirse (tarih degistiyse) sifirlar.
 * @param acc Kontrol edilecek hesap.
 */
static void reset_daily_limit_if_new_day(Account *acc) {
    char today[SHORT_DATE_LEN];
    get_current_date(today, sizeof(today));

    if (strcmp(acc->last_withdrawal_date, today) != 0) {
        acc->daily_withdrawn = 0.0;
        strncpy(acc->last_withdrawal_date, today, SHORT_DATE_LEN - 1);
        acc->last_withdrawal_date[SHORT_DATE_LEN - 1] = '\0';
    }
}

/**
 * @brief Hesaptan para cekme islemini gunluk limit ve bakiye kontrolu ile
 *        gerceklestirir.
 * @param bank Diske kaydetmek icin Bank yapisi.
 * @param acc  Islem yapilacak hesap.
 */
void withdraw(Bank *bank, Account *acc) {
    printf("\n===== PARA CEKME =====\n");
    printf("Guncel Bakiye: %.2f TL\n", acc->balance);

    reset_daily_limit_if_new_day(acc);
    double remaining_limit = DAILY_WITHDRAWAL_LIMIT - acc->daily_withdrawn;
    printf("Bugun kalan gunluk cekim limitiniz: %.2f TL\n", remaining_limit);

    double amount = read_positive_double("Cekilecek Tutar: ");

    if (amount > acc->balance) {
        printf("Yetersiz bakiye! Mevcut bakiyeniz: %.2f TL\n", acc->balance);
        return;
    }

    if (amount > remaining_limit) {
        printf("Gunluk cekim limitini asiyorsunuz! Bugun en fazla %.2f TL daha cekebilirsiniz.\n", remaining_limit);
        return;
    }

    acc->balance -= amount;
    acc->daily_withdrawn += amount;
    add_transaction(acc, "Para Cekme", amount);

    if (save_accounts(bank)) {
        printf("Islem basarili! Lutfen paranizi aliniz. Yeni Bakiye: %.2f TL\n", acc->balance);
    } else {
        printf("Islem yapildi ancak kayit sirasinda bir sorun olustu!\n");
    }
}

/**
 * @brief Iki hesap arasinda para transferi (havale/EFT) gerceklestirir.
 *
 * Alici hesap dogrudan Bank dizisi uzerinden (pointer ile) guncellenir ve
 * her iki hesabin islem gecmisine kayit dusulur.
 *
 * @param bank Alici hesabin da aranacagi ve diske kaydedilecegi Bank yapisi.
 * @param acc  Gonderen (oturum sahibi) hesap.
 */
void transfer(Bank *bank, Account *acc) {
    printf("\n===== HAVALE / EFT =====\n");
    printf("Guncel Bakiye: %.2f TL\n", acc->balance);

    int target_no = read_int("Alici Hesap Numarasi: ");

    if (target_no == acc->account_number) {
        printf("Kendi hesabiniza havale yapamazsiniz!\n");
        return;
    }

    Account *target = find_account_by_number(bank, target_no);
    if (!target) {
        printf("Belirtilen hesap numarasina ait bir hesap bulunamadi!\n");
        return;
    }

    double amount = read_positive_double("Gonderilecek Tutar: ");

    if (amount > acc->balance) {
        printf("Yetersiz bakiye! Mevcut bakiyeniz: %.2f TL\n", acc->balance);
        return;
    }

    /* Once bellekte her iki hesabi da guncelle, sonra tek seferde diske yaz.
       Boylece yazma sirasinda bir hata olursa iki hesap da tutarli kalir. */
    acc->balance -= amount;
    target->balance += amount;

    char desc[TRANSACTION_TYPE_LEN];
    snprintf(desc, sizeof(desc), "Havale Gonderme");
    add_transaction(acc, desc, amount);

    snprintf(desc, sizeof(desc), "Havale Alma");
    add_transaction(target, desc, amount);

    if (save_accounts(bank)) {
        printf("Havale basarili! %s adli kisiye %.2f TL gonderildi.\n", target->name, amount);
        printf("Yeni Bakiyeniz: %.2f TL\n", acc->balance);
    } else {
        printf("Havale yapildi ancak kayit sirasinda bir sorun olustu!\n");
    }
}

/**
 * @brief Oturum sahibi hesabin guncel bakiyesini ve temel bilgilerini gosterir.
 * @param acc Bilgileri gosterilecek hesap.
 */
void show_balance(const Account *acc) {
    printf("\n===== BAKIYE GORUNTULEME =====\n");
    printf("Hesap Sahibi   : %s\n", acc->name);
    printf("Hesap Numarasi : %d\n", acc->account_number);
    printf("Guncel Bakiye  : %.2f TL\n", acc->balance);
}

/**
 * @brief Vadeli hesap simulasyonu: kullanicinin girdigi ay sayisina gore
 *        bilesik faiz ile projekte edilen gelecekteki bakiyeyi hesaplar
 *        ve aylik dokum halinde gosterir. Gercek bakiyeyi DEGISTIRMEZ.
 * @param acc Faiz hesaplamasi yapilacak hesap.
 */
void calculate_interest(const Account *acc) {
    printf("\n===== VADELI HESAP / FAIZ SIMULASYONU =====\n");
    printf("Mevcut Bakiye: %.2f TL\n", acc->balance);
    printf("Aylik faiz orani: %%%.2f (sadece simulasyondur, gercek bakiyenizi etkilemez)\n",
           MONTHLY_INTEREST_RATE * 100);

    int months = read_int("Kac ay vadeye gore projeksiyon gormek istersiniz? ");
    if (months <= 0 || months > 120) {
        printf("Lutfen 1 ile 120 arasinda bir deger girin!\n");
        return;
    }

    double projected = acc->balance;
    printf("\n%-6s %-15s\n", "Ay", "Projekte Bakiye");
    printf("--------------------------\n");
    for (int i = 1; i <= months; i++) {
        projected *= (1.0 + MONTHLY_INTEREST_RATE);
        printf("%-6d %-15.2f\n", i, projected);
    }
    printf("--------------------------\n");
    printf("Not: Bu bir simulasyondur; gosterilen tutar gercek bakiyenize yansitilmaz.\n");
}

/**
 * @brief Basit bir ASCII "QR kod" deseni cizer.
 *
 * Gercek bir QR kodu kodlamasi degildir; sadece terminalde gorsel bir
 * simulasyon sunar.
 */
static void draw_ascii_qr(void) {
    const char *pattern[] = {
        "#########  ##  #########",
        "#       #  ##  #       #",
        "# ##### #  ##  # ##### #",
        "# ##### #  ##  # ##### #",
        "# ##### #  ##  # ##### #",
        "#       #  ##  #       #",
        "#########  ##  #########",
        "           ##           ",
        "## ## ##### ## ### ## ##",
        "#########  ##  #########",
        "#       #  ##  ## # ####",
        "# ##### #  ##  #  ## ###",
        "# ##### #  ##  ## ## ###",
        "# ##### #  ##  #  #  ###",
        "#       #  ##  ## ## ###",
        "#########  ##  #########",
    };
    int n = (int)(sizeof(pattern) / sizeof(pattern[0]));
    for (int i = 0; i < n; i++) {
        printf("  %s\n", pattern[i]);
    }
}

/**
 * @brief Telefon uzerinden onaylanan bir kod ile "temassiz" para cekme
 *        akisini simule eder: ASCII QR gosterilir, rastgele bir dogrulama
 *        kodu uretilir ve kullanicidan bu kodu girmesi istenir.
 *
 * @param bank Diske kaydetmek icin Bank yapisi.
 * @param acc  Islem yapilacak hesap.
 */
void qr_withdrawal_simulation(Bank *bank, Account *acc) {
    printf("\n===== QR ILE PARA CEKME (SIMULASYON) =====\n");
    printf("Guncel Bakiye: %.2f TL\n", acc->balance);

    double amount = read_positive_double("Cekilecek Tutar: ");

    if (amount > acc->balance) {
        printf("Yetersiz bakiye! Mevcut bakiyeniz: %.2f TL\n", acc->balance);
        return;
    }

    reset_daily_limit_if_new_day(acc);
    double remaining_limit = DAILY_WITHDRAWAL_LIMIT - acc->daily_withdrawn;
    if (amount > remaining_limit) {
        printf("Gunluk cekim limitini asiyorsunuz! Bugun en fazla %.2f TL daha cekebilirsiniz.\n", remaining_limit);
        return;
    }

    printf("\nLutfen telefonunuzdaki ATM uygulamasi ile asagidaki QR kodu okutun:\n\n");
    draw_ascii_qr();

    /* 6 haneli rastgele dogrulama kodu uret (100000 - 999999) */
    int verification_code = 100000 + (rand() % 900000);
    printf("\n(Simulasyon amacli: telefonunuza gelen kod = %d)\n", verification_code);

    int user_input = read_int("Telefonunuza gelen dogrulama kodunu giriniz: ");

    if (user_input != verification_code) {
        printf("Dogrulama kodu hatali! Islem iptal edildi.\n");
        return;
    }

    acc->balance -= amount;
    acc->daily_withdrawn += amount;
    add_transaction(acc, "QR ile Para Cekme", amount);

    if (save_accounts(bank)) {
        printf("\nDogrulama basarili! Lutfen paranizi alin. Yeni Bakiye: %.2f TL\n", acc->balance);
    } else {
        printf("Islem yapildi ancak kayit sirasinda bir sorun olustu!\n");
    }
}
