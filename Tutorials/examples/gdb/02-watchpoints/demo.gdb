# Demo: a watchpoint reports every change to a variable, including the aliased write.
set pagination off

# Get into main so `a` is in scope, then watch the field.
break main
run
watch a.balance

echo \n== change #1: the legit debit (a.balance -= 50) ==\n
continue
backtrace

echo \n== change #2: the mysterious write via alias, inside apply_bonus() ==\n
continue
backtrace

echo \n== change #3: the second debit ==\n
continue

echo \n== let it finish ==\n
continue
