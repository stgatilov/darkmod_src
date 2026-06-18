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


class idHashMapPrinter:
    wildcard = 'idHashMap<*>'

    def __init__(self, value):
        self.value = value

    def children(self):
        res = [raw_child_expandable(self.value)]
        res += array_children_list(self.value['table'], self.value['size'])
        # TODO: I'd like to filter away elements with key == empty.
        # But I can't find a way to do that... and I think calling C++ functions often crashes.
        # See also: https://stackoverflow.com/questions/22774067/gdb-python-api-is-it-possible-to-make-a-call-to-a-class-struct-method
        return res

    def to_string(self):
        return 'HashMap[%d / %d]' % (self.value['count'], self.value['size'])


class idListPrinter:
    regex = r'^id(List|StaticList|FlexList)<.*>$'

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
    def get(self, index, oobValue = 0):
        pArr = self.value['list']
        if index >= 0 and index < int(self.value['num']):
            return pArr[index]
        else:
            return oobValue
    

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


idlib_pplist = [
    make_simple_printer('idVec2', '({$x}, {$y})'),
    make_simple_printer('idVec3', '({$x}, {$y}, {$z})'),
    make_simple_printer('idVec4', '({$x}, {$y}, {$z}, {$w})'),
    make_simple_printer('idVec5', '({$x}, {$y}, {$z}, {$s}, {$t})'),
    make_simple_printer('idVec6', '({@x}, {@y}, {@z}, {@u}, {@v}, {@w})', lambda v: {
        'x': v['p'][0], 'y': v['p'][1], 'z': v['p'][2],
        'u': v['p'][3], 'v': v['p'][4], 'w': v['p'][5],
    }),
    make_simple_printer('idPlane', '(a={$a}, b={$b}, c={$c}, d={$d})'),
    make_simple_printer('idAngles', '(p={$pitch}, y={$yaw}, r={$roll})'),
    make_simple_printer('idBounds', '({@min} ... {@max})', lambda v: {'min': v['b'][0], 'max': v['b'][1]}),
    make_simple_printer('idMat2', '2 x 2 matrix', lambda v: [('[%d]' % i, v['mat'][i]) for i in range(2)]),
    make_simple_printer('idMat3', '3 x 3 matrix', lambda v: [('[%d]' % i, v['mat'][i]) for i in range(3)]),
    make_simple_printer('idMat4', '4 x 4 matrix', lambda v: [('[%d]' % i, v['mat'][i]) for i in range(4)]),
    make_simple_printer('idRenderMatrix', 'render matrix', lambda v: [
        ('row%d' % i, v['m'][4 * i].address.cast(gdb.lookup_type('idVec4*')).dereference()) for i in range(4)
    ]),
    make_simple_printer('idWinding', 'winding[{$numPoints}]', lambda v: [('numPoints', This)] + array_children_list(v['p'], v['numPoints'])),
    make_simple_printer('idKeyValue', '{@key}: {@value}', lambda v: {'key': v['key'].dereference(), 'value': v['value'].dereference()}),
    make_simple_printer('idKeyVal<*>', '{$key}: {$value}'),
]

idlib_pplist += [
    idListPrinter,
    idLinkListPrinter,
    idStrPrinter,
    idDictPrinter,
    idHashMapPrinter,
]

def build_pretty_printer():
    return TdmPrettyPrinterCollection.create_with_printers('idlib', idlib_pplist)
