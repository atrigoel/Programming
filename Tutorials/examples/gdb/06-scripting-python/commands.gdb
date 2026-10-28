# Demo: user-defined commands, a GDB while-loop, and a Python pretty-printer.
set pagination off

# A reusable command that walks any Node* list with a GDB while-loop.
define printlist
  set $n = $arg0
  set $i = 0
  while $n != 0
    printf "  [%d] value = %d\n", $i, $n->value
    set $n = $n->next
    set $i = $i + 1
  end
end
document printlist
Walk a Node* linked list and print each value.  Usage: printlist <Node*>
end

break report
run

echo \n== 1) raw GDB is clumsy: chasing ->next by hand ==\n
print list->value
print list->next->value

echo \n== 2) our user-defined command walks the whole list ==\n
printlist list

echo \n== 3) load a Python pretty-printer, then printing is automatic ==\n
source prettyprint.py
print *list

continue
