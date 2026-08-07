/**
 * @file atm.h
 * @brief ATM Simulasyonu icin tum yapi (struct) tanimlari, sabitler ve
 *        fonksiyon prototiplerini iceren merkezi header dosyasi.
 */

#ifndef ATM_H
#define ATM_H

#include <stdio.h>

/* ============================== SABITLER ============================== */

#define MAX_ACCOUNTS            1000
#define MAX_NAME_LEN            50
#define PIN_INPUT_LEN           16
#define PIN_HASH_LEN            21
#define MAX_HISTORY             10
#define TRANSACTION_TYPE_LEN    24
#define DATE_LEN                20
#define SHORT_DATE_LEN          11

#define MAX_FAILED_ATTEMPTS     3
#define DAILY_WITHDRAWAL_LIMIT  5000.0
#define MONTHLY_INTEREST_RATE   0.015

#define ADMIN_PASSWORD          "admin123"

#define HARDWARE_HEALTH_MAX     100.0
#define HARDWARE_WEAR_MIN       2.0
#define HARDWARE_WEAR_MAX       5.0
#define MAINTENANCE_THRESHOLD   20.0

#define DATA_DIR                "data"
#define ACCOUNTS_FILE           "data/accounts.dat"
#define ADMIN_LOG_FILE          "data/admin_actions.log"

/* ============================ VERI YAPILARI ============================ */

typedef struct {
    char   type[TRANSACTION_TYPE_LEN];
    double amount;
    char   timestamp[DATE_LEN];
} Transaction;

typedef struct {
    int    account_number;
    char   name[MAX_NAME_LEN];
    char   pin_hash[PIN_HASH_LEN];
    double balance;
    int    failed_attempts;
    int    is_locked;
    int    is_active;
    double daily_withdrawn;
    char   last_withdrawal_date[SHORT_DATE_LEN];
    Transaction history[MAX_HISTORY];
    int    history_count;
} Account;

typedef struct {
    Account accounts[MAX_ACCOUNTS];
    int     count;
} Bank;

/* ============================ FSM TANIMLARI ============================ */

typedef enum {
    STATE_MAIN_MENU = 0,
    STATE_LOGIN,
    STATE_CREATE_ACCOUNT,
    STATE_USER_MENU,
    STATE_DEPOSIT,
    STATE_WITHDRAW,
    STATE_TRANSFER,
    STATE_BALANCE,
    STATE_CHANGE_PIN,
    STATE_HISTORY,
    STATE_INTEREST,
    STATE_QR_WITHDRAW,
    STATE_LOGOUT,
    STATE_ADMIN_LOGIN,
    STATE_ADMIN_MENU,
    STATE_ADMIN_LIST,
    STATE_ADMIN_DELETE,
    STATE_ADMIN_UNLOCK,
    STATE_ADMIN_RESET_HW,
    STATE_BAKIM_GEREKLI,
    STATE_EXIT,
    STATE_COUNT
} AtmState;

typedef enum {
    EVT_NONE = 0,
    EVT_SELECT_LOGIN,
    EVT_SELECT_CREATE_ACCOUNT,
    EVT_SELECT_ADMIN,
    EVT_SELECT_EXIT,
    EVT_SELECT_DEPOSIT,
    EVT_SELECT_WITHDRAW,
    EVT_SELECT_TRANSFER,
    EVT_SELECT_BALANCE,
    EVT_SELECT_CHANGE_PIN,
    EVT_SELECT_HISTORY,
    EVT_SELECT_INTEREST,
    EVT_SELECT_QR,
    EVT_SELECT_LOGOUT,
    EVT_SELECT_ADMIN_LIST,
    EVT_SELECT_ADMIN_DELETE,
    EVT_SELECT_ADMIN_UNLOCK,
    EVT_SELECT_ADMIN_RESET_HW,
    EVT_SELECT_ADMIN_BACK,
    EVT_OPERATION_DONE,
    EVT_AUTH_OK,
    EVT_AUTH_FAIL
} AtmEvent;

typedef struct {
    Bank     bank;
    Account *active_account;
    double   hardware_health;
} AtmContext;

/* ============================ file_utils.c ============================ */
void ensure_data_directory(void);
int load_accounts(Bank *bank);
int save_accounts(const Bank *bank);
void log_admin_action(const char *action);

/* =============================== utils.c ================================ */
void clear_input_buffer(void);
int read_int(const char *prompt);
double read_positive_double(const char *prompt);
void read_string(const char *prompt, char *buffer, size_t size);
void read_hidden_input(const char *prompt, char *buffer, size_t size);
void hash_pin(const char *pin, char *out_hash);
void get_current_timestamp(char *buffer, size_t size);
void get_current_date(char *buffer, size_t size);
void press_enter_to_continue(void);

/* =============================== auth.c ================================= */
void create_account(Bank *bank);
Account *login(Bank *bank);
void change_pin(Bank *bank, Account *acc);

/* ============================= account.c ================================ */
void deposit(Bank *bank, Account *acc);
int withdraw(Bank *bank, Account *acc);
void transfer(Bank *bank, Account *acc);
void show_balance(const Account *acc);
void calculate_interest(const Account *acc);
int qr_withdrawal_simulation(Bank *bank, Account *acc);
Account *find_account_by_number(Bank *bank, int account_number);

/* ============================ hardware.c ================================ */
void hardware_wear(AtmContext *ctx);
int hardware_needs_maintenance(const AtmContext *ctx);
void hardware_reset(AtmContext *ctx);

/* ============================ transaction.c ============================= */
void add_transaction(Account *acc, const char *type, double amount);
void print_transaction_history(const Account *acc);

/* =============================== admin.c ================================ */
void admin_panel(Bank *bank);
void admin_list_accounts(const Bank *bank);
void admin_delete_account(Bank *bank);
void admin_unlock_account(Bank *bank);

#endif /* ATM_H */
