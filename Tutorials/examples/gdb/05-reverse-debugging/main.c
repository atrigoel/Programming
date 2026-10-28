#include <stdio.h>

/*
 * Three stages mutate a shared value. Suppose the final result is "wrong" and you
 * want to see which stage last touched it — WITHOUT rerunning from scratch.
 * GDB's `record` lets you run forward, then step BACKWARDS through history.
 */

static int state = 0;

static void step_a(void) { state += 5; }   /* 0  -> 5  */
static void step_b(void) { state *= 2; }   /* 5  -> 10 */
static void step_c(void) { state -= 3; }   /* 10 -> 7  */

int main(void) {
    step_a();
    step_b();
    step_c();
    printf("final state = %d\n", state);
    return 0;
}
