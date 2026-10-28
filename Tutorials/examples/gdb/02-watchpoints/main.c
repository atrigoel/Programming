#include <stdio.h>

/*
 * "Who changed my variable?"  balance is modified in an obvious place (a debit)
 * AND through an alias pointer in apply_bonus() — the kind of change that's hard
 * to find by reading code. A watchpoint catches EVERY write, with old/new values
 * and the exact line responsible.
 */

typedef struct {
    int id;
    int balance;
} Account;

static void apply_bonus(int *p) {
    *p += 100;                 /* mysterious change: writes through an alias */
}

int main(void) {
    Account a = { .id = 1, .balance = 500 };
    int *alias = &a.balance;   /* alias pointing into the struct */

    a.balance -= 50;           /* legit debit */
    apply_bonus(alias);        /* balance changes here — but not obviously */
    a.balance -= 50;           /* another debit */

    printf("account %d final balance = %d\n", a.id, a.balance);
    return 0;
}
