import gdb
from tdmuc_base import *


class idStrPrinter:
    wildcard = 'idStr'
    allow_derived = True    # idPoolStr

    def __init__(self, value):
        self.value = value

    # note: display_hint = string is unwanted, it adds doublequotes implicitly
    def to_string(self):
        return '"' + self.value['data'].string(length = int(self.value['len'])) + '"'
    
    def children(self):
        yield raw_child_expandable(self.value)
        n = int(self.value['len'])
        pChars = self.value['data']
        for i in range(n):
            yield (str(i), pChars[i])


class idKeyValuePrinter:
    wildcard = 'idKeyValue'

    def __init__(self, value):
        self.value = value

    def to_string(self):
        keyStr = display_string(self.value['key'].dereference())
        valStr = display_string(self.value['value'].dereference())
        return '%s: %s' % (keyStr, valStr)

    def children(self):
        return raw_children_inline(self.value)


class idDictPrinter:
    wildcard = 'idDict'

    def __init__(self, value):
        self.value = value
        self.list = value['args']

    def children(self):
        yield raw_child_expandable(self.value)
        yield from children_of(self.list)

    def to_string(self):
        res = display_string(self.list)
        return res.replace('List', 'Dict')

    #def get(self, key):    TODO: do we need this?
    

class idListPrinter:
    wildcard = 'idList<*>'

    def __init__(self, value):
        self.value = value

    def display_hint(self):
        return 'array'

    def children(self):
        yield raw_child_expandable(self.value)
        n = int(self.value['num'])
        pArr = self.value['list']
        for i in range(n):
            yield (str(i), pArr[i])

    def to_string(self):
        n = int(self.value['num'])
        return 'List[%d]' % n
    
    # used in other pretty-printers to get element by index
    def get(self, index):
        pArr = self.value['list']
        return pArr[index]
    

class idLinkListPrinter:
    wildcard = 'idLinkList<*>'

    def __init__(self, value):
        self.value = value

    class EnumSynthetic:
        def __init__(self, value):
            self.value = value

        def children(self):
            pnode = self.value['next']
            if pnode == self.value.address:
                return []
            k = 1
            while int(pnode) != 0:
                vnode = pnode.dereference()
                yield ('[%d]' % k, vnode['owner'])
                if vnode['next'] == vnode['head']:
                    break
                pnode = vnode['next']
                k += 1

    def children(self):
        yield from raw_children_inline(self.value)

        headPtr = self.value['head']
        if headPtr == self.value.address:
            yield ('[Enum All]', embed_printer_for_value(self.value, idLinkListPrinter.EnumSynthetic))

    def to_string(self):
        return 'LinkList'
        

def build_pretty_printer():
    return TdmPrettyPrinterCollection.create_with_printers('idlib', [idListPrinter, idStrPrinter, idKeyValuePrinter, idDictPrinter, idLinkListPrinter])
