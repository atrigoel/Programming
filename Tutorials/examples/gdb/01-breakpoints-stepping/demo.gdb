# Demo: breakpoints, the call stack, and the stepping family.
set pagination off

echo \n== break on a line inside the loop ==\n
break main.c:12
run

echo \n== where are we? walk the stack ==\n
backtrace
info args
info locals

echo \n== step INTO square(), then finish back out ==\n
step
backtrace
finish

echo \n== one loop iteration with next (steps OVER square) ==\n
next
print total
print i

echo \n== run to the end ==\n
continue
