# Demo: conditional breakpoints, breakpoint command lists, and dprintf.
set pagination off

echo \n== 1) conditional breakpoint: stop ONLY on the bad item ==\n
break process if it->value < 0
run
print *it
backtrace

echo \n== 2) a command list: auto-print each call, then keep going ==\n
delete
break process
commands
  silent
  printf "  process: id=%d value=%d\n", it->id, it->value
  continue
end
# restart under the new breakpoint
run

echo \n== 3) dprintf: same trace in ONE line, no code edit, no recompile ==\n
delete
dprintf process, "  dprintf: id=%d value=%d\n", it->id, it->value
run
