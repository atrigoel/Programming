# Demo: record execution, then step BACKWARDS to find the last writer of `state`.
set pagination off

break main
run

echo \n== start recording from here ==\n
record

echo \n== run forward to the end of main ==\n
break step_c
continue
finish                 # let step_c run; state is now final (7)
printf "state is now %d\n", state

echo \n== now rewind: reverse-continue stops at the previous write ==\n
watch state
reverse-continue       # back to step_c's write (10 -> 7)
printf "before step_c: state was %d\n", state
reverse-continue       # back to step_b's write (5 -> 10)
printf "before step_b: state was %d\n", state
reverse-continue       # back to step_a's write (0 -> 5)
printf "before step_a: state was %d\n", state
