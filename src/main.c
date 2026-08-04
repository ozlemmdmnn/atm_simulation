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
#include <string.h>
#include <time.h>
#include "../include/atm.h"

/* Her state fonksiyonu: mevcut context ile çalışır, sonraki AtmState'i döner. */
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

int main(void) {
    srand((unsigned int)time(NULL));

    static AtmContext ctx;   /* Bank buyuk oldugu icin static omurle ayrilir */
    memset(&ctx, 0, sizeof(ctx));

    ensure_data_directory();
    if (!load_accounts(&ctx.bank)) {
        fprintf(stderr, "UYARI: Hesap verileri okunurken sorun olustu.\n");
    }
    printf("Toplam %d hesap yuklendi.\n", ctx.bank.count);

    AtmState state = STATE_MAIN_MENU;

    /* ---- FSM DISPATCHER DONGUSU: tek kontrol noktasi ---- */
    while (state != STATE_EXIT) {
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
            case STATE_EXIT:
            case STATE_COUNT:
            default:
                /* Gecersiz duruma dusmek FSM'de bir hata sinyalidir;
                   fail-safe olarak programi guvenle sonlandiriyoruz. */
                fprintf(stderr, "HATA: Gecersiz FSM durumu (%d)!\n", (int)state);
                state = STATE_EXIT;
        }
    }

    printf("\nATM Simulasyonundan cikiliyor. Iyi gunler!\n");
    return 0;
}
/* İki aşamalı geçiş: 1) girdi -> event, 2) event -> sonraki state.
   Bu ayrım FSM'i test edilebilir kılar (event üretimini state
   geçişinden bağımsız test edebilirsin). */
static AtmState state_main_menu(AtmContext *ctx) {
    (void)ctx;
    printf("\n1. Giris Yap\n2. Yeni Hesap Olustur\n3. Admin Paneli\n0. Cik\n");
    int choice = read_int("Seciminiz: ");

    AtmEvent evt;
    switch (choice) {
        case 1: evt = EVT_SELECT_LOGIN;          break;
        case 2: evt = EVT_SELECT_CREATE_ACCOUNT;  break;
        case 3: evt = EVT_SELECT_ADMIN;           break;
        case 0: evt = EVT_SELECT_EXIT;            break;
        default: printf("Gecersiz secim!\n"); evt = EVT_NONE;
    }

    switch (evt) {
        case EVT_SELECT_LOGIN:          return STATE_LOGIN;
        case EVT_SELECT_CREATE_ACCOUNT: return STATE_CREATE_ACCOUNT;
        case EVT_SELECT_ADMIN:          return STATE_ADMIN_LOGIN;
        case EVT_SELECT_EXIT:           return STATE_EXIT;
        default:                        return STATE_MAIN_MENU;
    }
}

/* auth.c::login() DEGISMEDEN cagriliyor, sonucuna gore state degisir */
static AtmState state_login(AtmContext *ctx) {
    ctx->active_account = login(&ctx->bank);
    return (ctx->active_account != NULL) ? STATE_USER_MENU : STATE_MAIN_MENU;
}

/* account.c::deposit() DEGISMEDEN cagriliyor */
static AtmState state_deposit(AtmContext *ctx) {
    deposit(&ctx->bank, ctx->active_account);
    press_enter_to_continue();
    return STATE_USER_MENU;   /* EVT_OPERATION_DONE ile esdeger */
}

/* transaction.c::print_transaction_history() DEGISMEDEN cagriliyor */
static AtmState state_history(AtmContext *ctx) {
    print_transaction_history(ctx->active_account);
    press_enter_to_continue();
    return STATE_USER_MENU;
}

/* admin.c'deki alt fonksiyonlar (admin_panel() DEGIL, onun ic parcalari) */
static AtmState state_admin_list(AtmContext *ctx) {
    admin_list_accounts(&ctx->bank);
    press_enter_to_continue();
    return STATE_ADMIN_MENU;
}