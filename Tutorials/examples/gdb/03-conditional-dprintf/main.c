#include <stdio.h>

/*
 * A loop over items where exactly one item is bad (negative value). Stopping on
 * every iteration is useless; a CONDITIONAL breakpoint stops only on the bad one.
 * dprintf then traces every iteration without stopping — and without editing code.
 */

typedef struct {
    int id;
    int value;
} Item;

static int process(Item *it) {
    return it->value * 2;
}

int main(void) {
    Item items[] = {
        {1, 10}, {2, 20}, {3, -1}, {4, 40}, {5, 50}   /* id 3 is the bad one */
    };
    int n = (int)(sizeof(items) / sizeof(items[0]));
    long acc = 0;

    for (int i = 0; i < n; i++) {
        acc += process(&items[i]);
    }

    printf("acc = %ld\n", acc);
    return 0;
}
