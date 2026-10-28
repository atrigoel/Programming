#include <stdio.h>

/*
 * Walks a linked list but forgets to stop at the end: the loop dereferences a
 * NULL `next` and crashes with SIGSEGV. We debug it AFTER the fact from a core
 * dump — the way you handle a crash that already happened (in CI, in the field).
 */

typedef struct Node {
    int v;
    struct Node *next;
} Node;

static int sum_list(Node *head) {
    int s = 0;
    for (Node *p = head; ; p = p->next) {   /* BUG: no p != NULL test */
        s += p->v;                          /* crashes here when p == NULL */
    }
    return s;
}

int main(void) {
    Node b = { 2, NULL };
    Node a = { 1, &b };                      /* a -> b -> NULL */

    printf("sum = %d\n", sum_list(&a));
    return 0;
}
