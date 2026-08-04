/**
 * @file main.c
 * @brief ATM Simulasyonu - Program giris noktasi ve ana menu dongusu.
 *
 * @details Bu dosya sadece akisi yonetir; is mantigi (business logic)
 *          auth.c, account.c, transaction.c ve admin.c dosyalarina
 *          dagitilmistir (modularite / single-responsibility prensibi).
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../include/atm.h"

/** @brief Oturum acmis kullanici icin islem menusunu gosterir ve yonetir. */
static void user_menu(Bank *bank, Account *acc) {
    int choice;
    do {
        printf("\n========== KULLANICI MENUSU (%s) ==========\n", acc->name);
        printf("1. Para Yatir\n");
        printf("2. Para Cek\n");
        printf("3. Havale / EFT Gonder\n");
        printf("4. Bakiye Goruntule\n");
        printf("5. Sifre (PIN) Degistir\n");
        printf("6. Islem Gecmisini Goruntule\n");
        printf("7. Vadeli Hesap / Faiz Simulasyonu\n");
        printf("8. QR ile Para Cek\n");
        printf("0. Cikis Yap\n");

        choice = read_int("Seciminiz: ");

        switch (choice) {
            case 1: deposit(bank, acc); break;
            case 2: withdraw(bank, acc); break;
            case 3: transfer(bank, acc); break;
            case 4: show_balance(acc); break;
            case 5: change_pin(bank, acc); break;
            case 6: print_transaction_history(acc); break;
            case 7: calculate_interest(acc); break;
            case 8: qr_withdrawal_simulation(bank, acc); break;
            case 0: printf("Oturum kapatiliyor, iyi gunler %s!\n", acc->name); break;
            default: printf("Gecersiz secim! Lutfen tekrar deneyin.\n");
        }

        if (choice != 0) {
            press_enter_to_continue();
        }
    } while (choice != 0);
}

/** @brief Uygulamanin ana (giris seviyesi) menusunu gosterir ve yonetir. */
static void main_menu(Bank *bank) {
    int choice;
    do {
        printf("\n================================================\n");
        printf("            *** ATM SIMULASYONU ***\n");
        printf("================================================\n");
        printf("1. Giris Yap\n");
        printf("2. Yeni Hesap Olustur\n");
        printf("3. Admin Paneli\n");
        printf("0. Programdan Cik\n");

        choice = read_int("Seciminiz: ");

        switch (choice) {
            case 1: {
                Account *acc = login(bank);
                if (acc) {
                    user_menu(bank, acc);
                }
                break;
            }
            case 2:
                create_account(bank);
                press_enter_to_continue();
                break;
            case 3:
                admin_panel(bank);
                break;
            case 0:
                printf("\nATM Simulasyonundan cikiliyor. Iyi gunler!\n");
                break;
            default:
                printf("Gecersiz secim! Lutfen tekrar deneyin.\n");
        }
    } while (choice != 0);
}

/**
 * @brief Programin giris noktasi.
 *
 * Baslangicta hesap verilerini diskten yukler, ana menuyu calistirir.
 * Bank yapisi yigin (stack) uzerinde tek bir statik/otomatik degisken
 * olarak tutulur; hicbir noktada malloc/free kullanilmadigindan bellek
 * sizintisi riski bulunmaz.
 *
 * @return Basarili cikiste 0.
 */
int main(void) {
    srand((unsigned int)time(NULL));

    static Bank bank; /* MAX_ACCOUNTS * sizeof(Account) buyuk olabileceginden
                          stack yerine static/global omurle ayrilir. */

    ensure_data_directory();

    if (!load_accounts(&bank)) {
        fprintf(stderr, "UYARI: Hesap verileri okunurken bir sorun olustu; bos veriyle devam ediliyor.\n");
    }

    printf("Toplam %d hesap yuklendi.\n", bank.count);

    main_menu(&bank);

    return 0;
}
