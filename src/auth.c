/**
 * @file auth.c
 * @brief Hesap olusturma, giris (login) ve PIN degistirme islemlerinin
 *        implementasyonu. Kart bloke (3 yanlis PIN) mantigi burada yonetilir.
 */

#include <stdio.h>
#include <string.h>
#include "../include/atm.h"

/**
 * @brief Bank yapisi icinde bir sonraki kullanilabilir hesap numarasini uretir.
 *
 * Basit bir artan sayac mantigidir: 1000'den baslar, mevcut en yuksek
 * hesap numarasinin bir fazlasini doner.
 *
 * @param bank Mevcut hesaplarin tutuldugu Bank yapisi.
 * @return Kullanima hazir yeni hesap numarasi.
 */
static int generate_account_number(const Bank *bank) {
    int max_no = 1000;
    for (int i = 0; i < bank->count; i++) {
        if (bank->accounts[i].account_number >= max_no) {
            max_no = bank->accounts[i].account_number + 1;
        }
    }
    return max_no;
}

/**
 * @brief Yeni bir musteri hesabi olusturur.
 *
 * Kullanicidan ad-soyad ve 4 haneli PIN alir, hesabi Bank dizisine ekler
 * ve degisikligi diske kaydeder.
 *
 * @param bank Hesabin ekelenecegi Bank yapisina pointer.
 */
void create_account(Bank *bank) {
    if (bank->count >= MAX_ACCOUNTS) {
        printf("\nUzgunuz, sistem maksimum hesap kapasitesine ulasti.\n");
        return;
    }

    printf("\n===== YENI HESAP OLUSTURMA =====\n");

    Account new_acc;
    memset(&new_acc, 0, sizeof(Account));

    read_string("Adiniz Soyadiniz: ", new_acc.name, sizeof(new_acc.name));
    if (strlen(new_acc.name) == 0) {
        printf("Isim bos birakilamaz! Islem iptal edildi.\n");
        return;
    }

    char pin[PIN_INPUT_LEN];
    char pin_confirm[PIN_INPUT_LEN];

    read_hidden_input("4 haneli bir PIN belirleyin: ", pin, sizeof(pin));
    if (strlen(pin) < 4) {
        printf("PIN en az 4 haneli olmalidir! Islem iptal edildi.\n");
        return;
    }

    read_hidden_input("PIN'i tekrar girin: ", pin_confirm, sizeof(pin_confirm));
    if (strcmp(pin, pin_confirm) != 0) {
        printf("PIN'ler eslesmiyor! Islem iptal edildi.\n");
        return;
    }

    new_acc.account_number = generate_account_number(bank);
    hash_pin(pin, new_acc.pin_hash);
    new_acc.balance = 0.0;
    new_acc.failed_attempts = 0;
    new_acc.is_locked = 0;
    new_acc.is_active = 1;
    new_acc.daily_withdrawn = 0.0;
    strcpy(new_acc.last_withdrawal_date, "");
    new_acc.history_count = 0;

    bank->accounts[bank->count] = new_acc;
    bank->count++;

    if (save_accounts(bank)) {
        printf("\nHesabiniz basariyla olusturuldu!\n");
        printf("Hesap Numaraniz: %d  (Lutfen not edin, giris icin gereklidir)\n", new_acc.account_number);
    } else {
        printf("\nHesap olusturuldu ancak diske kaydedilirken bir sorun olustu!\n");
    }
}

/**
 * @brief Kullanici giris akisini yonetir.
 *
 * Hesap numarasi ve PIN dogrulanir. Hesap kilitliyse (3 yanlis deneme
 * sonrasi) giris reddedilir. Yanlis PIN girildikce sayac artar; dogru
 * girise sayac sifirlanir.
 *
 * @param bank Hesaplarin tutuldugu Bank yapisi.
 * @return Basarili girise ait Account pointer'i, aksi halde NULL.
 */
Account *login(Bank *bank) {
    printf("\n===== GIRIS YAP =====\n");

    int acc_no = read_int("Hesap Numarasi: ");
    Account *acc = find_account_by_number(bank, acc_no);

    if (!acc) {
        printf("Bu hesap numarasina ait bir hesap bulunamadi!\n");
        return NULL;
    }

    if (acc->is_locked) {
        printf("Hesabiniz bloke edilmis! Ust uste %d yanlis PIN girisi tespit edildi.\n", MAX_FAILED_ATTEMPTS);
        printf("Lutfen hesabinizin kilidini actirmak icin ATM yoneticisiyle iletisime gecin.\n");
        return NULL;
    }

    char pin[PIN_INPUT_LEN];
    char hash[PIN_HASH_LEN];

    read_hidden_input("PIN: ", pin, sizeof(pin));
    hash_pin(pin, hash);

    if (strcmp(hash, acc->pin_hash) == 0) {
        acc->failed_attempts = 0;
        save_accounts(bank);
        printf("\nHos geldiniz, %s!\n", acc->name);
        return acc;
    }

    acc->failed_attempts++;
    printf("Hatali PIN! (%d/%d deneme)\n", acc->failed_attempts, MAX_FAILED_ATTEMPTS);

    if (acc->failed_attempts >= MAX_FAILED_ATTEMPTS) {
        acc->is_locked = 1;
        printf("Hesabiniz %d kez hatali PIN girisi nedeniyle BLOKE EDILDI!\n", MAX_FAILED_ATTEMPTS);
        printf("Kilidi actirmak icin lutfen bankaniza basvurun.\n");
    }

    save_accounts(bank);
    return NULL;
}

/**
 * @brief Oturum acmis bir kullanicinin PIN'ini degistirir.
 *
 * Guvenlik amaciyla once mevcut PIN dogrulanir, ardindan yeni PIN iki kez
 * alinarak (yazim hatasina karsi) teyit edilir.
 *
 * @param bank Diske kaydetmek icin Bank yapisi.
 * @param acc  PIN'i degistirilecek oturum sahibi hesap.
 */
void change_pin(Bank *bank, Account *acc) {
    printf("\n===== SIFRE (PIN) DEGISTIRME =====\n");

    char current_pin[PIN_INPUT_LEN];
    char current_hash[PIN_HASH_LEN];

    read_hidden_input("Mevcut PIN'iniz: ", current_pin, sizeof(current_pin));
    hash_pin(current_pin, current_hash);

    if (strcmp(current_hash, acc->pin_hash) != 0) {
        printf("Mevcut PIN hatali! Islem iptal edildi.\n");
        return;
    }

    char new_pin[PIN_INPUT_LEN];
    char confirm_pin[PIN_INPUT_LEN];

    read_hidden_input("Yeni PIN (en az 4 hane): ", new_pin, sizeof(new_pin));
    if (strlen(new_pin) < 4) {
        printf("Yeni PIN en az 4 haneli olmalidir! Islem iptal edildi.\n");
        return;
    }

    read_hidden_input("Yeni PIN (tekrar): ", confirm_pin, sizeof(confirm_pin));
    if (strcmp(new_pin, confirm_pin) != 0) {
        printf("Girilen PIN'ler eslesmiyor! Islem iptal edildi.\n");
        return;
    }

    hash_pin(new_pin, acc->pin_hash);

    if (save_accounts(bank)) {
        printf("PIN basariyla guncellendi!\n");
    } else {
        printf("PIN degisti ancak diske kaydedilirken bir sorun olustu!\n");
    }
}
