/**
 * @file atm.h
 * @brief ATM Simulasyonu icin tum yapi (struct) tanimlari, sabitler ve
 *        fonksiyon prototiplerini iceren merkezi header dosyasi.
 *
 * @author  Senior C Developer (Portfolio Project)
 * @details Bu proje egitim/portfolyo amacli gelistirilmis bir ATM
 *          simulasyonudur. PIN'ler basit bir hash fonksiyonu ile
 *          saklanir; gercek bir bankacilik sisteminde kriptografik
 *          olarak guvenli algoritmalar (bcrypt, Argon2 vb.) kullanilmalidir.
 */

#ifndef ATM_H
#define ATM_H

#include <stdio.h>

/* ============================== SABITLER ============================== */

#define MAX_ACCOUNTS            1000    /**< Sistemde tutulabilecek maksimum hesap sayisi */
#define MAX_NAME_LEN            50      /**< Kullanici isim/soyisim maksimum uzunlugu */
#define PIN_INPUT_LEN           16      /**< Kullanicidan alinan ham PIN girisi icin tampon boyutu */
#define PIN_HASH_LEN            21      /**< Hashlenmis PIN'in hex string olarak tutulacagi boyut */
#define MAX_HISTORY             10      /**< Her hesap icin saklanan son islem sayisi */
#define TRANSACTION_TYPE_LEN    24      /**< Islem turu string uzunlugu (orn: "Para Yatirma") */
#define DATE_LEN                20      /**< Tarih/saat damgasi string uzunlugu */
#define SHORT_DATE_LEN          11      /**< "YYYY-MM-DD" formatinda tarih uzunlugu */

#define MAX_FAILED_ATTEMPTS     3       /**< Hesabin kilitlenmesi icin izin verilen yanlis PIN sayisi */
#define DAILY_WITHDRAWAL_LIMIT  5000.0  /**< Gunluk maksimum para cekme limiti (TL) */
#define MONTHLY_INTEREST_RATE   0.015   /**< Vadeli hesap simulasyonu icin aylik faiz orani (%1.5) */

#define ADMIN_PASSWORD          "admin123" /**< Admin paneli sifresi (simulasyon amaclidir) */

#define DATA_DIR                "data"
#define ACCOUNTS_FILE           "data/accounts.dat"
#define ADMIN_LOG_FILE          "data/admin_actions.log"

/* ============================ VERI YAPILARI ============================ */

/**
 * @struct Transaction
 * @brief Tek bir islemi (yatirma, cekme, havale vb.) temsil eder.
 */
typedef struct {
    char   type[TRANSACTION_TYPE_LEN]; /**< Islem turu (orn: "Para Yatirma") */
    double amount;                     /**< Islem tutari */
    char   timestamp[DATE_LEN];        /**< Islemin gerceklestigi tarih/saat */
} Transaction;

/**
 * @struct Account
 * @brief Bir banka hesabini ve tum ilgili verilerini temsil eder.
 *
 * @note Struct icinde dinamik bellek (pointer/malloc) kullanilmaz; bu sayede
 *       struct dogrudan binary dosyaya yazilip okunabilir (fwrite/fread) ve
 *       bellek sizintisi riski ortadan kalkar.
 */
typedef struct {
    int    account_number;                  /**< Benzersiz hesap numarasi */
    char   name[MAX_NAME_LEN];              /**< Hesap sahibinin adi soyadi */
    char   pin_hash[PIN_HASH_LEN];          /**< PIN'in hashlenmis hali (hex string) */
    double balance;                         /**< Guncel bakiye */
    int    failed_attempts;                 /**< Ust uste yanlis PIN giris sayisi */
    int    is_locked;                       /**< 1 ise hesap kilitli (kart bloke) */
    int    is_active;                       /**< 0 ise hesap silinmis/pasif kabul edilir */
    double daily_withdrawn;                 /**< Bugun cekilen toplam tutar */
    char   last_withdrawal_date[SHORT_DATE_LEN]; /**< Son para cekme tarihi (YYYY-MM-DD) */
    Transaction history[MAX_HISTORY];       /**< Dairesel (circular) islem gecmisi tamponu */
    int    history_count;                   /**< history dizisine simdiye kadar yazilan toplam islem sayisi */
} Account;

/**
 * @struct Bank
 * @brief Tum hesaplari ve toplam hesap sayisini tutan ust seviye yapi.
 */
typedef struct {
    Account accounts[MAX_ACCOUNTS];
    int     count;
} Bank;

/* ============================ file_utils.c ============================ */

/** @brief data/ klasorunun var oldugundan emin olur, yoksa olusturur. */
void ensure_data_directory(void);

/** @brief Hesaplari ACCOUNTS_FILE dosyasindan Bank yapisina yukler. */
int load_accounts(Bank *bank);

/** @brief Bank yapisindaki tum hesaplari ACCOUNTS_FILE dosyasina kaydeder. */
int save_accounts(const Bank *bank);

/** @brief Verilen turdeki islemi admin log dosyasina ekler (audit trail). */
void log_admin_action(const char *action);

/* =============================== utils.c ================================ */

/** @brief stdin tamponunda kalan karakterleri (ozellikle '\n') temizler. */
void clear_input_buffer(void);

/** @brief Kullanicidan guvenli sekilde tam sayi okur; hatali girdide tekrar sorar. */
int read_int(const char *prompt);

/** @brief Kullanicidan guvenli sekilde pozitif double (tutar) okur; hatali girdide tekrar sorar. */
double read_positive_double(const char *prompt);

/** @brief Kullanicidan sabit uzunlukta guvenli string okur (bufferi asmaz, newline temizler). */
void read_string(const char *prompt, char *buffer, size_t size);

/** @brief Terminalde girilen PIN'i ekrana yansitmadan ('*' ile) okur (POSIX termios). */
void read_hidden_input(const char *prompt, char *buffer, size_t size);

/** @brief Basit ama tutarli bir hash (djb2) uretir; PIN'i duz metin olarak saklamamak icindir. */
void hash_pin(const char *pin, char *out_hash /* size >= PIN_HASH_LEN */);

/** @brief Su anki tarih/saati "YYYY-MM-DD HH:MM" formatinda buffera yazar. */
void get_current_timestamp(char *buffer, size_t size);

/** @brief Su anki tarihi "YYYY-MM-DD" formatinda buffera yazar. */
void get_current_date(char *buffer, size_t size);

/** @brief Ekranin okunabilirligi icin devam etmeden once kullanicidan Enter bekler. */
void press_enter_to_continue(void);

/* =============================== auth.c ================================= */

/** @brief Yeni bir hesap olusturur; kullanicidan isim ve PIN alir, banka yapisina ekler. */
void create_account(Bank *bank);

/**
 * @brief Kullanici girisini yonetir (hesap no + PIN dogrulama, bloke kontrolu).
 * @return Basarili girise ait Account'a pointer, basarisizlikta NULL.
 */
Account *login(Bank *bank);

/** @brief Oturum acmis kullanicinin PIN'ini degistirir (eski PIN dogrulanir). */
void change_pin(Bank *bank, Account *acc);

/* ============================= account.c ================================ */

/** @brief Hesaba para yatirma islemini gerceklestirir. */
void deposit(Bank *bank, Account *acc);

/** @brief Hesaptan para cekme islemini (gunluk limit kontrolu dahil) gerceklestirir. */
void withdraw(Bank *bank, Account *acc);

/** @brief Iki hesap arasinda havale/EFT islemini gerceklestirir. */
void transfer(Bank *bank, Account *acc);

/** @brief Guncel bakiyeyi ve hesap bilgilerini ekrana yazdirir. */
void show_balance(const Account *acc);

/** @brief Vadeli hesap simulasyonu: girilen ay sayisina gore projekte edilen bakiyeyi gosterir. */
void calculate_interest(const Account *acc);

/** @brief ASCII sanati ile sahte bir QR kod cizer ve dogrulama kodlu para cekme akisini simule eder. */
void qr_withdrawal_simulation(Bank *bank, Account *acc);

/** @brief Belirtilen hesap numarasina sahip aktif hesabi bulup pointer doner (yoksa NULL). */
Account *find_account_by_number(Bank *bank, int account_number);

/* ============================ transaction.c ============================= */

/** @brief Hesabin islem gecmisine (dairesel tampon mantigiyla) yeni bir kayit ekler. */
void add_transaction(Account *acc, const char *type, double amount);

/** @brief Hesabin son islemlerini (en yeniden en eskiye) ekrana listeler. */
void print_transaction_history(const Account *acc);

/* =============================== admin.c ================================ */

/** @brief Admin sifresini dogrular ve basariliysa admin panelini acar. */
void admin_panel(Bank *bank);

/** @brief Sistemdeki tum hesaplari (bakiye, durum, kilit bilgisi ile) listeler. */
void admin_list_accounts(const Bank *bank);

/** @brief Verilen hesap numarasina sahip hesabi sistemden (soft-delete ile) siler. */
void admin_delete_account(Bank *bank);

/** @brief Kilitli (bloke) bir hesabin kilidini acar ve yanlis deneme sayacini sifirlar. */
void admin_unlock_account(Bank *bank);

#endif /* ATM_H */
