"""A GDB pretty-printer for `Node` linked lists.

Once sourced (`source prettyprint.py`), `print *list` shows the entire chain
(`Node list: 50 -> 40 -> 30 -> 20 -> 10`) instead of a single node. This is the
same mechanism libstdc++ uses to make `std::vector`/`std::map` print nicely.
"""
import gdb


class NodePrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        values = []
        node = self.val
        # Follow `next` until NULL; guard against a runaway/cyclic list.
        for _ in range(1000):
            values.append(str(int(node['value'])))
            nxt = node['next']
            if int(nxt) == 0:
                break
            node = nxt.dereference()
        return "Node list: " + " -> ".join(values)


def lookup(val):
    # Match a `Node` value (what you get from `print *list`).
    if val.type.strip_typedefs().code == gdb.TYPE_CODE_STRUCT and val.type.tag == "Node":
        return NodePrinter(val)
    return None


# Register globally so it applies to every `Node` from now on.
gdb.pretty_printers.append(lookup)
print("[prettyprint.py] Node pretty-printer registered")
