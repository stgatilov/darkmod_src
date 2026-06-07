import gdb
from tdmuc_base import *
from tdmuc_idlib import *


class idEntityPrinter:
    wildcard = 'idEntity'
    allow_derived = True        # there are many derived classes

    class TeamSynthetic:
        def __init__(self, value):
            self.value = value
        def to_string(self):
            return ''
        def children(self):
            curr = self.value['teamMaster']
            k = 0
            res = []
            while int(curr) != 0:
                key = '[%d]' % k
                if curr == self.value.address:
                    key = '=>' + key
                res.append((key, curr))
                k += 1
                curr = curr.dereference()['teamChain']
            return res


    def __init__(self, value):
        self.value = value

    def to_string(self):
        return 'Entity(%d): %s ' % (
            int(self.value['entityNumber']),
            display_string(self.value['name'])
        )
    
    def children(self):
        res = raw_children_inline(self.value)
        child = ('[Team]', embed_printer_for_value(self.value, idEntityPrinter.TeamSynthetic))
        res.append(child)
        return res


class idEntityPtrPrinter:
    wildcard = 'idEntityPtr<*>'

    def __init__(self, value):
        self.value = value
        self.entid = int(value['entityId'])
        self.spid = int(value['spawnId'])
        self.ptrValue = gdb.Value(0).cast(gdb.lookup_type('idEntity').pointer())
        self.invalid = False

        if self.spid != 0:
            gameLocal = gdb.lookup_global_symbol('gameLocal', gdb.SYMBOL_VAR_DOMAIN).value()
            self.ptrValue = idListPrinter(gameLocal['entities']).get(self.entid)
            currSpid = idListPrinter(gameLocal['spawnIds']).get(self.entid)
            if self.spid != currSpid:
                self.invalid = True

    def to_string(self):
        text = display_string(self.ptrValue)
        if self.invalid:
            text = "[WRONG] " + text
        return text
    
    def children(self):
        key = '[WRONG]' if self.invalid else '[entity]'
        res = [
            (key, self.ptrValue),
            raw_child_expandable(self.value),
        ]
        return res


def build_pretty_printer():
    return TdmPrettyPrinterCollection.create_with_printers('game', [
        idEntityPrinter,
        idEntityPtrPrinter,
    ])
