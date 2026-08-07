/**
 * @file main.c
 * @brief ATM Simulasyonu - Sonlu Durum Makinesi (Finite State Machine)
 *        mimarisiyle yazilmis program giris noktasi ve durum dongusu.
 *
 * @details Klasik if/else + while tabanli menu akisi yerine, her ekran/
 *          islem bir "state" (AtmState) olarak modellenmistir. Her state
 *          fonksiyonu kullanicidan girdi alir, bunu bir "event" (AtmEvent)
 *          olarak yorumlar ve bir sonraki state'i doner. Ana dongu (main)
 *          sadece bir switch-case dispatcher'dir; is mantigi (business
 *          logic) auth.c / account.c / transaction.c / admin.c icindeki
 *          MEVCUT fonksiyonlarda degismeden kalir - FSM sadece akisi
 *          (control flow) yeniden duzenler.
 *
 *          Durum gecis semasi (ozet):
 *
 *          MAIN_MENU --(login)--> LOGIN --(basarili)--> USER_MENU
 *          MAIN_MENU --(hesap ac)--> CREATE_ACCOUNT --> MAIN_MENU
 *          MAIN_MENU --(admin)--> ADMIN_LOGIN --(basarili)--> ADMIN_MENU
 *          USER_MENU --(1..8)--> DEPOSIT/WITHDRAW/.../QR_WITHDRAW --> USER_MENU
 *          USER_MENU --(cikis)--> LOGOUT --> MAIN_MENU
 *          ADMIN_MENU --(1..3)--> ADMIN_LIST/DELETE/UNLOCK --> ADMIN_MENU
 *          ADMIN_MENU --(geri)--> MAIN_MENU
 *          MAIN_MENU --(cikis)--> EXIT (donguyu sonlandirir)
 *
 *          IoT / Predictive-Maintenance eklentisi:
 *          Herhangi bir MUSTERI state'ine girilmeden once motor sagligi
 *          kontrol edilir; esigin altindaysa STATE_BAKIM_GEREKLI'ye
 *          yonlendirilir (bkz. dispatcher dongusu icindeki guard).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/atm.h"

/* ===================== STATE FONKSIYON PROTOTIPLERI ===================== */
/* Her fonksiyon: mevcut context ile calisir, bir sonraki AtmState'i doner. */

static AtmState state_main_menu(AtmContext *ctx);
static AtmState state_login(AtmContext *ctx);
static AtmState state_create_account(AtmContext *ctx);
static AtmState state_user_menu(AtmContext *ctx);
static AtmState state_deposit(AtmContext *ctx);
static AtmState state_withdraw(AtmContext *ctx);
static AtmState state_transfer(AtmContext *ctx);
static AtmState state_balance(AtmContext *ctx);
static AtmState state_change_pin(AtmContext *ctx);
static AtmState state_history(AtmContext *ctx);
static AtmState state_interest(AtmContext *ctx);
static AtmState state_qr_withdraw(AtmContext *ctx);
static AtmState state_logout(AtmContext *ctx);
static AtmState state_admin_login(AtmContext *ctx);
static AtmState state_admin_menu(AtmContext *ctx);
static AtmState state_admin_list(AtmContext *ctx);
static AtmState state_admin_delete(AtmContext *ctx);
static AtmState state_admin_unlock(AtmContext *ctx);
static AtmState state_admin_reset_hw(AtmContext *ctx);
static AtmState state_bakim_gerekli(AtmContext *ctx);
static int      is_customer_facing_state(AtmState state);

/* ============================== ANA DONGU ================================ */

/**
 * @brief Programin giris noktasi. FSM'i baslatir ve STATE_EXIT'e kadar
 *        durum dispatcher dongusunu calistirir.
 *
 * @return Basarili cikiste 0.
 */
int main(void) {
    /* Degisken tanimlamalari C standartlari geregi blok basina alindi */
    static AtmContext ctx;             /* Bank buyuk oldugu icin static omurle ayrilir */
    AtmState state = STATE_MAIN_MENU;

    srand((unsigned int)time(NULL));

    memset(&ctx, 0, sizeof(ctx));
    ctx.active_account = NULL;
    ctx.hardware_health = HARDWARE_HEALTH_MAX; /* ATM "fabrika cikisli" saglikli baslar */

    ensure_data_directory();
    if (!load_accounts(&ctx.bank)) {
        fprintf(stderr, "UYARI: Hesap verileri okunurken sorun olustu; bos veriyle devam ediliyor.\n");
    }
    printf("Toplam %d hesap yuklendi.\n", ctx.bank.count);

    /* ---- FSM DISPATCHER DONGUSU ----
       Spagetti if/else + ic ice while yerine TEK bir kontrol noktasi. */
    while (state != STATE_EXIT) {

        /* ---- DONANIM GUVENLIK KILIDI (predictive-maintenance interlock) ----
           Musteriye acik HERHANGI bir state'e girilmeden once motor sagligi
           kontrol edilir. Esigin altindaysa, istenen state ne olursa olsun
           STATE_BAKIM_GEREKLI'ye yonlendirilir. Admin/bakim state'leri bu
           kontrolden MUAF tutulur, aksi halde arizali sistem asla
           onarilamaz (kilitlenme/deadlock olurdu). */
        if (hardware_needs_maintenance(&ctx) && is_customer_facing_state(state)) {
            state = STATE_BAKIM_GEREKLI;
        }

        switch (state) {
            case STATE_MAIN_MENU:      state = state_main_menu(&ctx);      break;
            case STATE_LOGIN:          state = state_login(&ctx);          break;
            case STATE_CREATE_ACCOUNT: state = state_create_account(&ctx); break;
            case STATE_USER_MENU:      state = state_user_menu(&ctx);      break;
            case STATE_DEPOSIT:        state = state_deposit(&ctx);        break;
            case STATE_WITHDRAW:       state = state_withdraw(&ctx);       break;
            case STATE_TRANSFER:       state = state_transfer(&ctx);       break;
            case STATE_BALANCE:        state = state_balance(&ctx);        break;
            case STATE_CHANGE_PIN:     state = state_change_pin(&ctx);     break;
            case STATE_HISTORY:        state = state_history(&ctx);        break;
            case STATE_INTEREST:       state = state_interest(&ctx);       break;
            case STATE_QR_WITHDRAW:    state = state_qr_withdraw(&ctx);    break;
            case STATE_LOGOUT:         state = state_logout(&ctx);         break;
            case STATE_ADMIN_LOGIN:    state = state_admin_login(&ctx);    break;
            case STATE_ADMIN_MENU:     state = state_admin_menu(&ctx);     break;
            case STATE_ADMIN_LIST:     state = state_admin_list(&ctx);     break;
            case STATE_ADMIN_DELETE:   state = state_admin_delete(&ctx);   break;
            case STATE_ADMIN_UNLOCK:   state = state_admin_unlock(&ctx);   break;
            case STATE_ADMIN_RESET_HW: state = state_admin_reset_hw(&ctx); break;
            case STATE_BAKIM_GEREKLI:  state = state_bakim_gerekli(&ctx);  break;

            case STATE_EXIT:
            case STATE_COUNT:
            default:
                /* Bilinmeyen/gecersiz bir duruma dusulmesi FSM tasariminda
                   bir hata sinyalidir; guvenli tarafta kalip programi
                   duzgunce sonlandiriyoruz (embedded "fail-safe" pratigi). */
                fprintf(stderr, "HATA: Gecersiz FSM durumu (%d)! Program sonlandiriliyor.\n", (int)state);
                state = STATE_EXIT;
                break;
        }
    }

    printf("\nATM Simulasyonundan cikiliyor. Iyi gunler!\n");
    return 0;
}

/* ============================ STATE FONKSIYONLARI ========================= */

/**
 * @brief STATE_MAIN_MENU: Ana menuyu gosterir, kullanici secimini bir
 *        AtmEvent'e cevirir ve olaya gore sonraki state'i belirler.
 */
static AtmState state_main_menu(AtmContext *ctx) {
    int choice;
    AtmEvent evt;
    (void)ctx; /* Bu ekranda context'e ihtiyac yok, imza tutarliligi icin var */

    printf("\n================================================\n");
    printf("            *** ATM SIMULASYONU ***\n");
    printf("================================================\n");
    printf("1. Giris Yap\n");
    printf("2. Yeni Hesap Olustur\n");
    printf("3. Admin Paneli\n");
    printf("0. Programdan Cik\n");

    choice = read_int("Seciminiz: ");

    /* 1. asama: ham girdi -> anlamli olay (event) */
    switch (choice) {
        case 1: evt = EVT_SELECT_LOGIN;          break;
        case 2: evt = EVT_SELECT_CREATE_ACCOUNT;  break;
        case 3: evt = EVT_SELECT_ADMIN;           break;
        case 0: evt = EVT_SELECT_EXIT;            break;
        default:
            printf("Gecersiz secim! Lutfen tekrar deneyin.\n");
            evt = EVT_NONE;
    }

    /* 2. asama: olay -> durum gecisi */
    switch (evt) {
        case EVT_SELECT_LOGIN:          return STATE_LOGIN;
        case EVT_SELECT_CREATE_ACCOUNT: return STATE_CREATE_ACCOUNT;
        case EVT_SELECT_ADMIN:          return STATE_ADMIN_LOGIN;
        case EVT_SELECT_EXIT:           return STATE_EXIT;
        default:                        return STATE_MAIN_MENU; /* ayni durumda kal */
    }
}

/**
 * @brief STATE_LOGIN: Mevcut auth.c::login() fonksiyonunu DEGISTIRMEDEN
 *        cagirir; sonucuna gore USER_MENU veya MAIN_MENU'ye gecer.
 */
static AtmState state_login(AtmContext *ctx) {
    ctx->active_account = login(&ctx->bank);   /* <-- auth.c'deki mevcut fonksiyon */
    return (ctx->active_account != NULL) ? STATE_USER_MENU : STATE_MAIN_MENU;
}

/**
 * @brief STATE_CREATE_ACCOUNT: Mevcut auth.c::create_account() cagrilir.
 */
static AtmState state_create_account(AtmContext *ctx) {
    create_account(&ctx->bank);                /* <-- auth.c'deki mevcut fonksiyon */
    press_enter_to_continue();
    return STATE_MAIN_MENU;
}

/**
 * @brief STATE_USER_MENU: Oturum acmis kullanicinin islem menusu.
 */
static AtmState state_user_menu(AtmContext *ctx) {
    int choice;
    AtmEvent evt;

    printf("\n========== KULLANICI MENUSU (%s) ==========\n", ctx->active_account->name);
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
        case 1: evt = EVT_SELECT_DEPOSIT;     break;
        case 2: evt = EVT_SELECT_WITHDRAW;    break;
        case 3: evt = EVT_SELECT_TRANSFER;    break;
        case 4: evt = EVT_SELECT_BALANCE;     break;
        case 5: evt = EVT_SELECT_CHANGE_PIN;  break;
        case 6: evt = EVT_SELECT_HISTORY;     break;
        case 7: evt = EVT_SELECT_INTEREST;    break;
        case 8: evt = EVT_SELECT_QR;          break;
        case 0: evt = EVT_SELECT_LOGOUT;      break;
        default:
            printf("Gecersiz secim! Lutfen tekrar deneyin.\n");
            evt = EVT_NONE;
    }

    switch (evt) {
        case EVT_SELECT_DEPOSIT:    return STATE_DEPOSIT;
        case EVT_SELECT_WITHDRAW:   return STATE_WITHDRAW;
        case EVT_SELECT_TRANSFER:   return STATE_TRANSFER;
        case EVT_SELECT_BALANCE:    return STATE_BALANCE;
        case EVT_SELECT_CHANGE_PIN: return STATE_CHANGE_PIN;
        case EVT_SELECT_HISTORY:    return STATE_HISTORY;
        case EVT_SELECT_INTEREST:   return STATE_INTEREST;
        case EVT_SELECT_QR:         return STATE_QR_WITHDRAW;
        case EVT_SELECT_LOGOUT:     return STATE_LOGOUT;
        default:                    return STATE_USER_MENU;
    }
}

/* ---- Islem durumlari: her biri account.c/auth.c/transaction.c'deki
        MEVCUT fonksiyonu cagirip EVT_OPERATION_DONE ile USER_MENU'ye doner. ---- */

static AtmState state_deposit(AtmContext *ctx) {
    deposit(&ctx->bank, ctx->active_account);         /* <-- account.c */
    press_enter_to_continue();
    return STATE_USER_MENU; /* EVT_OPERATION_DONE ile esdeger gecis */
}

/**
 * @brief STATE_WITHDRAW: account.c::withdraw() cagrilir; para fiilen
 *        verildiyse (donus degeri 1) para verme motoru asindirilir
 *        (IoT/predictive-maintenance simulasyonu).
 */
static AtmState state_withdraw(AtmContext *ctx) {
    if (withdraw(&ctx->bank, ctx->active_account)) {   /* <-- account.c */
        hardware_wear(ctx);                             /* <-- hardware.c */
        printf("[SISTEM] Motor sagligi: %%%.1f\n", ctx->hardware_health);
    }
    press_enter_to_continue();
    return STATE_USER_MENU; /* Motor kritige dustuyse dispatcher guard'i
                                bir sonraki dongude otomatik olarak
                                STATE_BAKIM_GEREKLI'ye yonlendirecek. */
}

static AtmState state_transfer(AtmContext *ctx) {
    transfer(&ctx->bank, ctx->active_account);         /* <-- account.c */
    press_enter_to_continue();
    return STATE_USER_MENU;
}

static AtmState state_balance(AtmContext *ctx) {
    show_balance(ctx->active_account);                 /* <-- account.c */
    press_enter_to_continue();
    return STATE_USER_MENU;
}

static AtmState state_change_pin(AtmContext *ctx) {
    change_pin(&ctx->bank, ctx->active_account);        /* <-- auth.c */
    press_enter_to_continue();
    return STATE_USER_MENU;
}

static AtmState state_history(AtmContext *ctx) {
    print_transaction_history(ctx->active_account);     /* <-- transaction.c */
    press_enter_to_continue();
    return STATE_USER_MENU;
}

static AtmState state_interest(AtmContext *ctx) {
    calculate_interest(ctx->active_account);             /* <-- account.c */
    press_enter_to_continue();
    return STATE_USER_MENU;
}

/**
 * @brief STATE_QR_WITHDRAW: QR ile cekim de fiziksel para verdigi icin
 *        ayni sekilde motoru asindirir.
 */
static AtmState state_qr_withdraw(AtmContext *ctx) {
    if (qr_withdrawal_simulation(&ctx->bank, ctx->active_account)) { /* <-- account.c */
        hardware_wear(ctx);                                          /* <-- hardware.c */
        printf("[SISTEM] Motor sagligi: %%%.1f\n", ctx->hardware_health);
    }
    press_enter_to_continue();
    return STATE_USER_MENU;
}

/**
 * @brief STATE_LOGOUT: Oturumu kapatir (active_account = NULL) ve ana
 *        menuye doner. Guvenlik acisindan onemli: bir sonraki kullanicinin
 *        onceki oturumun hesabina erisememesini garanti eder.
 */
static AtmState state_logout(AtmContext *ctx) {
    printf("Oturum kapatiliyor, iyi gunler %s!\n", ctx->active_account->name);
    ctx->active_account = NULL;
    return STATE_MAIN_MENU;
}

/* ---- Admin durumlari: admin.c'deki MEVCUT alt fonksiyonlar dogrudan
        kullanilir; sadece eski admin_panel() icindeki while dongusu
        FSM state'lerine bolunmustur. ---- */

static AtmState state_admin_login(AtmContext *ctx) {
    char password[PIN_INPUT_LEN];
    (void)ctx;
    
    read_hidden_input("\nAdmin Sifresi: ", password, sizeof(password));

    if (strcmp(password, ADMIN_PASSWORD) != 0) {
        printf("Hatali admin sifresi! Erisim reddedildi.\n");
        return STATE_MAIN_MENU;   /* EVT_AUTH_FAIL ile esdeger gecis */
    }

    log_admin_action("Admin girisi yapildi.");   /* <-- file_utils.c */
    return STATE_ADMIN_MENU;                      /* EVT_AUTH_OK ile esdeger gecis */
}

static AtmState state_admin_menu(AtmContext *ctx) {
    int choice;
    AtmEvent evt;

    printf("\n===== ADMIN PANELI =====\n");
    printf("Motor Sagligi: %%%.1f%s\n", ctx->hardware_health,
           hardware_needs_maintenance(ctx) ? "  [!] BAKIM GEREKLI" : "");
    printf("1. Tum Hesaplari Listele\n");
    printf("2. Hesap Sil\n");
    printf("3. Bloke Hesabin Kilidini Ac\n");
    printf("4. Motoru Degistir / Saglik Sifirla\n");
    printf("0. Ana Menuye Don\n");

    choice = read_int("Seciminiz: ");

    switch (choice) {
        case 1: evt = EVT_SELECT_ADMIN_LIST;     break;
        case 2: evt = EVT_SELECT_ADMIN_DELETE;   break;
        case 3: evt = EVT_SELECT_ADMIN_UNLOCK;   break;
        case 4: evt = EVT_SELECT_ADMIN_RESET_HW; break;
        case 0: evt = EVT_SELECT_ADMIN_BACK;     break;
        default:
            printf("Gecersiz secim! Lutfen tekrar deneyin.\n");
            evt = EVT_NONE;
    }

    switch (evt) {
        case EVT_SELECT_ADMIN_LIST:     return STATE_ADMIN_LIST;
        case EVT_SELECT_ADMIN_DELETE:   return STATE_ADMIN_DELETE;
        case EVT_SELECT_ADMIN_UNLOCK:   return STATE_ADMIN_UNLOCK;
        case EVT_SELECT_ADMIN_RESET_HW: return STATE_ADMIN_RESET_HW;
        case EVT_SELECT_ADMIN_BACK:     return STATE_MAIN_MENU;
        default:                        return STATE_ADMIN_MENU;
    }
}

static AtmState state_admin_list(AtmContext *ctx) {
    admin_list_accounts(&ctx->bank);      /* <-- admin.c */
    press_enter_to_continue();
    return STATE_ADMIN_MENU;
}

static AtmState state_admin_delete(AtmContext *ctx) {
    admin_delete_account(&ctx->bank);     /* <-- admin.c */
    press_enter_to_continue();
    return STATE_ADMIN_MENU;
}

static AtmState state_admin_unlock(AtmContext *ctx) {
    admin_unlock_account(&ctx->bank);     /* <-- admin.c */
    press_enter_to_continue();
    return STATE_ADMIN_MENU;
}

/**
 * @brief STATE_ADMIN_RESET_HW: "Motoru Degistir/Sifirla" islemi.
 *        hardware.c::hardware_reset() cagrilir, saglik %100'e doner ve
 *        islem denetim (audit) logunda kayit altina alinir.
 */
static AtmState state_admin_reset_hw(AtmContext *ctx) {
    hardware_reset(ctx);   /* <-- hardware.c */
    log_admin_action("Motor degistirildi/bakim yapildi - saglik %100'e sifirlandi."); /* <-- file_utils.c */

    printf("\nMotor basariyla degistirildi. Yeni saglik: %%%.1f\n", ctx->hardware_health);
    printf("ATM tekrar musteri hizmetine acildi.\n");

    press_enter_to_continue();
    return STATE_ADMIN_MENU;
}

/**
 * @brief STATE_BAKIM_GEREKLI: Motor sagligi kritik esigin altina dustugunde
 *        dispatcher guard'i tarafindan zorla girilen durum. Musteri
 *        islemlerine tamamen kapalidir; tek cikis yolu admin girisi
 *        (bakim personeli) veya programdan cikistir.
 */
static AtmState state_bakim_gerekli(AtmContext *ctx) {
    int choice;

    printf("\n================================================\n");
    printf("   !!! DONANIM ARIZASI ONGORULDU !!!\n");
    printf("   Sistem Bakima Alindi - Musteri Islemlerine KAPALI\n");
    printf("   Motor Sagligi: %%%.1f  (Bakim Esigi: %%%.1f)\n",
           ctx->hardware_health, MAINTENANCE_THRESHOLD);
    printf("================================================\n");
    printf("1. Admin / Bakim Personeli Girisi\n");
    printf("0. Programdan Cik\n");

    choice = read_int("Seciminiz: ");

    switch (choice) {
        case 1: return STATE_ADMIN_LOGIN;
        case 0: return STATE_EXIT;
        default:
            printf("Bu ATM su anda sadece bakim personeli tarafindan kullanilabilir.\n");
            return STATE_BAKIM_GEREKLI;
    }
}

/**
 * @brief Verilen state'in "musteriye acik" (dolayisiyla donanim guard
 *        kontrolune tabi) bir state olup olmadigini belirler. Admin ve
 *        bakim state'leri kasitli olarak bu listenin DISINDA tutulur.
 *
 * @param state Kontrol edilecek durum.
 * @return Musteriye acik bir state ise 1, aksi halde (admin/bakim/exit) 0.
 */
static int is_customer_facing_state(AtmState state) {
    switch (state) {
        case STATE_MAIN_MENU:
        case STATE_LOGIN:
        case STATE_CREATE_ACCOUNT:
        case STATE_USER_MENU:
        case STATE_DEPOSIT:
        case STATE_WITHDRAW:
        case STATE_TRANSFER:
        case STATE_BALANCE:
        case STATE_CHANGE_PIN:
        case STATE_HISTORY:
        case STATE_INTEREST:
        case STATE_QR_WITHDRAW:
            return 1;
        default:
            return 0; /* ADMIN_*, BAKIM_GEREKLI, LOGOUT, EXIT vb. muaf */
    }
}