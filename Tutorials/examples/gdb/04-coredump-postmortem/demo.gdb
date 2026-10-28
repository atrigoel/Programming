# Demo: post-mortem inspection of a core dump.
set pagination off

echo \n== the stack at the moment of death ==\n
backtrace full

echo \n== inspect the crashing frame ==\n
frame 0
info locals
info args

echo \n== the offending pointer: p was NULL when we dereferenced p->v ==\n
print p
print head
