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
        res = [raw_child_expandable(self.value)]
        res += array_children_list(self.value['data'], self.value['len'])
        return res


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
        res = [raw_child_expandable(self.value)]
        res += children_of(self.list)
        return res

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
        res = [raw_child_expandable(self.value)]
        res += array_children_list(self.value['list'], self.value['num'])
        return res

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

    def children(self):
        res = raw_children_inline(self.value)
        headPtr = self.value['head']
        if headPtr == self.value.address:
            def synthetic():
                return linked_list_children_list(
                    self.value['next'],
                    lambda p: p.dereference()['next'],
                    lambda p: p.dereference()['owner'],
                    terminate_if = lambda p: int(p) == self.value.address,
                )
            child = ('[Enum All]', make_synthetic(synthetic))
            res.append(child)
        return res

    def to_string(self):
        return 'LinkList'
        

def build_pretty_printer():
    return TdmPrettyPrinterCollection.create_with_printers('idlib', [
        idListPrinter,
        idLinkListPrinter,
        idStrPrinter,
        idKeyValuePrinter, idDictPrinter,
    ])
