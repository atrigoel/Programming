#include <stdio.h>
#include <stdlib.h>

/*
 * A singly linked list — the classic data structure that's annoying to inspect in
 * raw GDB (you'd chase `->next` by hand). This example adds:
 *   - a user-defined GDB command (printlist) that walks it with a while-loop
 *   - a Python pretty-printer so `print *list` shows the whole chain automatically
 */

typedef struct Node {
    int value;
    struct Node *next;
} Node;

static Node *push(Node *head, int v) {
    Node *n = malloc(sizeof *n);
    n->value = v;
    n->next = head;
    return n;
}

static void report(Node *list) {
    int sum = 0;
    for (Node *p = list; p; p = p->next)
        sum += p->value;
    printf("sum = %d\n", sum);     /* break here: `list` is fully built */
}

int main(void) {
    Node *list = NULL;
    for (int i = 1; i <= 5; i++)
        list = push(list, i * 10);  /* 50 -> 40 -> 30 -> 20 -> 10 */
    report(list);
    return 0;
}
