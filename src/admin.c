/**
 * @file admin.c
 * @brief Yonetici (admin) paneli implementasyonu: tum hesaplari listeleme,
 *        hesap silme ve bloke hesaplarin kilidini acma islemleri.
 */

#include <stdio.h>
#include <string.h>
#include "../include/atm.h"

/**
 * @brief Admin sifresini dogrular; basariliysa admin menusunu acar.
 * @param bank Uzerinde islem yapilacak Bank yapisi.
 */
void admin_panel(Bank *bank) {
    char password[PIN_INPUT_LEN];
    read_hidden_input("\nAdmin Sifresi: ", password, sizeof(password));

    if (strcmp(password, ADMIN_PASSWORD) != 0) {
        printf("Hatali admin sifresi! Erisim reddedildi.\n");
        return;
    }

    log_admin_action("Admin girisi yapildi.");

    int choice;
    do {
        printf("\n===== ADMIN PANELI =====\n");
        printf("1. Tum Hesaplari Listele\n");
        printf("2. Hesap Sil\n");
        printf("3. Bloke Hesabin Kilidini Ac\n");
        printf("0. Ana Menuye Don\n");

        choice = read_int("Seciminiz: ");

        switch (choice) {
            case 1:
                admin_list_accounts(bank);
                break;
            case 2:
                admin_delete_account(bank);
                break;
            case 3:
                admin_unlock_account(bank);
                break;
            case 0:
                printf("Admin panelinden cikiliyor...\n");
                break;
            default:
                printf("Gecersiz secim! Lutfen tekrar deneyin.\n");
        }
    } while (choice != 0);
}

/**
 * @brief Sistemdeki tum aktif hesaplari tablo halinde listeler.
 * @param bank Listelenecek Bank yapisi.
 */
void admin_list_accounts(const Bank *bank) {
    printf("\n===== TUM HESAPLAR =====\n");

    int active_count = 0;
    printf("%-10s %-20s %-12s %-10s %-8s\n", "Hesap No", "Ad Soyad", "Bakiye", "Durum", "Kilit");
    printf("-------------------------------------------------------------------\n");

    for (int i = 0; i < bank->count; i++) {
        const Account *a = &bank->accounts[i];
        if (!a->is_active) continue;

        active_count++;
        printf("%-10d %-20s %-12.2f %-10s %-8s\n",
               a->account_number,
               a->name,
               a->balance,
               "Aktif",
               a->is_locked ? "EVET" : "Hayir");
    }

    printf("-------------------------------------------------------------------\n");
    printf("Toplam Aktif Hesap Sayisi: %d\n", active_count);
}

/**
 * @brief Belirtilen hesap numarasina sahip hesabi sistemden siler.
 *
 * Bellekteki diziden fiziksel olarak cikarma yerine "soft delete"
 * (is_active = 0) yaklasimi kullanilir; bu, hesap numarasi cakismalarini
 * ve islem gecmisi tutarliligini korumaya yardimci olur.
 *
 * @param bank Uzerinde islem yapilacak Bank yapisi.
 */
void admin_delete_account(Bank *bank) {
    printf("\n===== HESAP SILME =====\n");
    int acc_no = read_int("Silinecek Hesap Numarasi: ");

    Account *acc = find_account_by_number(bank, acc_no);
    if (!acc) {
        printf("Bu numarada aktif bir hesap bulunamadi!\n");
        return;
    }

    printf("Hesap Sahibi: %s | Bakiye: %.2f TL\n", acc->name, acc->balance);
    char confirm[8];
    read_string("Bu hesabi silmek istediginize emin misiniz? (E/H): ", confirm, sizeof(confirm));

    if (confirm[0] != 'E' && confirm[0] != 'e') {
        printf("Islem iptal edildi.\n");
        return;
    }

    acc->is_active = 0;

    if (save_accounts(bank)) {
        char log_msg[100];
        snprintf(log_msg, sizeof(log_msg), "Hesap silindi: #%d (%s)", acc->account_number, acc->name);
        log_admin_action(log_msg);
        printf("Hesap basariyla silindi.\n");
    } else {
        printf("Hesap silindi ancak kayit sirasinda bir sorun olustu!\n");
    }
}

/**
 * @brief 3 hatali PIN girisi nedeniyle bloke edilmis bir hesabin kilidini
 *        acar ve yanlis deneme sayacini sifirlar.
 * @param bank Uzerinde islem yapilacak Bank yapisi.
 */
void admin_unlock_account(Bank *bank) {
    printf("\n===== HESAP KILIDI ACMA =====\n");
    int acc_no = read_int("Kilidi Acilacak Hesap Numarasi: ");

    Account *acc = find_account_by_number(bank, acc_no);
    if (!acc) {
        printf("Bu numarada aktif bir hesap bulunamadi!\n");
        return;
    }

    if (!acc->is_locked) {
        printf("Bu hesap zaten kilitli degil.\n");
        return;
    }

    acc->is_locked = 0;
    acc->failed_attempts = 0;

    if (save_accounts(bank)) {
        char log_msg[100];
        snprintf(log_msg, sizeof(log_msg), "Hesap kilidi acildi: #%d (%s)", acc->account_number, acc->name);
        log_admin_action(log_msg);
        printf("Hesabin kilidi basariyla acildi.\n");
    } else {
        printf("Kilit acildi ancak kayit sirasinda bir sorun olustu!\n");
    }
}
