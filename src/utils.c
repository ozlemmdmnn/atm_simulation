/**
 * @file utils.c
 * @brief Guvenli kullanici girdisi okuma, hashleme ve tarih/saat yardimci
 *        fonksiyonlarinin implementasyonu.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include "../include/atm.h"

#if defined(__unix__) || defined(__APPLE__)
    #include <termios.h>
    #include <unistd.h>
    #define HAS_TERMIOS 1
#else
    #define HAS_TERMIOS 0
#endif

/**
 * @brief stdin tamponunda kalan karakterleri ('\n' dahil) temizler.
 *
 * scanf/fgets ile karisik kullanimda tamponda kalan artik karakterlerin
 * bir sonraki okuma islemini bozmasini engeller.
 */
void clear_input_buffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {
        /* bosalt */
    }
}

/**
 * @brief Kullanicidan guvenli bicimde bir tam sayi okur.
 *
 * fgets + strtol kombinasyonu kullanilir; scanf("%d") gibi fonksiyonlarin
 * harf girildiginde sonsuz donguye girme riskini ortadan kaldirir.
 *
 * @param prompt Kullaniciya gosterilecek mesaj.
 * @return Gecerli sekilde girilen tam sayi.
 */
int read_int(const char *prompt) {
    char line[128];
    long value;
    char *endptr;

    while (1) {
        printf("%s", prompt);
        if (!fgets(line, sizeof(line), stdin)) {
            clearerr(stdin);
            continue;
        }

        /* Sadece newline girildiyse tekrar sor */
        if (line[0] == '\n') {
            printf("Gecersiz giris! Lutfen bir sayi girin.\n");
            continue;
        }

        value = strtol(line, &endptr, 10);

        /* endptr, sayidan sonra sadece bosluk/newline iceriyorsa gecerlidir */
        while (*endptr == ' ' || *endptr == '\t') endptr++;
        if (endptr == line || (*endptr != '\n' && *endptr != '\0')) {
            printf("Gecersiz giris! Lutfen sadece sayi girin.\n");
            continue;
        }

        return (int)value;
    }
}

/**
 * @brief Kullanicidan pozitif (0'dan buyuk) bir ondalikli sayi (tutar) okur.
 * @param prompt Kullaniciya gosterilecek mesaj.
 * @return Gecerli ve pozitif double deger.
 */
double read_positive_double(const char *prompt) {
    char line[128];
    double value;
    char *endptr;

    while (1) {
        printf("%s", prompt);
        if (!fgets(line, sizeof(line), stdin)) {
            clearerr(stdin);
            continue;
        }

        if (line[0] == '\n') {
            printf("Gecersiz giris! Lutfen bir tutar girin.\n");
            continue;
        }

        value = strtod(line, &endptr);
        while (*endptr == ' ' || *endptr == '\t') endptr++;

        if (endptr == line || (*endptr != '\n' && *endptr != '\0')) {
            printf("Gecersiz giris! Lutfen sadece sayi girin (orn: 150.50).\n");
            continue;
        }

        if (value <= 0.0) {
            printf("Tutar 0'dan buyuk olmalidir!\n");
            continue;
        }

        return value;
    }
}

/**
 * @brief Kullanicidan sabit boyutlu bir string okur; bufferi asmaz.
 * @param prompt Kullaniciya gosterilecek mesaj.
 * @param buffer Verinin yazilacagi tampon.
 * @param size   Tamponun toplam boyutu (null terminator dahil).
 */
void read_string(const char *prompt, char *buffer, size_t size) {
    printf("%s", prompt);

    if (!fgets(buffer, (int)size, stdin)) {
        buffer[0] = '\0';
        return;
    }

    /* fgets '\n' karakterini de aliyorsa temizle */
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    } else {
        /* Satir buffera sigmadiysa kalan kismi tampondan temizle */
        clear_input_buffer();
    }
}

/**
 * @brief PIN gibi hassas verileri ekrana '*' basarak (POSIX sistemlerde)
 *        gizli bicimde okur. Termios desteklenmeyen ortamlarda normal
 *        (gorunur) girise geri duser.
 */
void read_hidden_input(const char *prompt, char *buffer, size_t size) {
    printf("%s", prompt);
    fflush(stdout);

#if HAS_TERMIOS
    struct termios old_term, new_term;
    tcgetattr(STDIN_FILENO, &old_term);
    new_term = old_term;
    new_term.c_lflag &= (tcflag_t)~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_term);

    size_t idx = 0;
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF && idx < size - 1) {
        if (ch == 127 || ch == 8) { /* backspace */
            if (idx > 0) {
                idx--;
                printf("\b \b");
                fflush(stdout);
            }
            continue;
        }
        buffer[idx++] = (char)ch;
        printf("*");
        fflush(stdout);
    }
    buffer[idx] = '\0';
    printf("\n");

    tcsetattr(STDIN_FILENO, TCSANOW, &old_term);
#else
    read_string("", buffer, size);
#endif
}

/**
 * @brief djb2 algoritmasina dayali basit bir hash fonksiyonu.
 *
 * @note Bu fonksiyon kriptografik olarak guvenli DEGILDIR; sadece PIN'in
 *       duz metin (plaintext) olarak dosyada tutulmamasini saglayan bir
 *       simulasyon amaclidir. Gercek sistemlerde bcrypt/Argon2 gibi
 *       tuzlanmis (salted) ve yavas hash algoritmalari kullanilmalidir.
 *
 * @param pin       Hashlenecek ham PIN.
 * @param out_hash  Sonucun yazilacagi tampon (en az PIN_HASH_LEN boyutunda).
 */
void hash_pin(const char *pin, char *out_hash) {
    unsigned long hash = 5381;
    int c;

    while ((c = *pin++)) {
        hash = ((hash << 5) + hash) + (unsigned long)c; /* hash * 33 + c */
    }

    snprintf(out_hash, PIN_HASH_LEN, "%016lx", hash);
}

/**
 * @brief Su anki tarih/saati "YYYY-MM-DD HH:MM" formatinda yazar.
 */
void get_current_timestamp(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M", t);
}

/**
 * @brief Su anki tarihi "YYYY-MM-DD" formatinda yazar.
 */
void get_current_date(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d", t);
}

/**
 * @brief Kullaniciya "devam etmek icin Enter'a basin" bekletmesi yapar.
 */
void press_enter_to_continue(void) {
    printf("\nDevam etmek icin Enter'a basin...");
    fflush(stdout);
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }
}
