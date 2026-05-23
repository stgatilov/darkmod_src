import gdb
import re, fnmatch, traceback, sys
from collections import namedtuple


# displays given string, no children
class LiteralStringPrinter:
    def __init__(self, literal):
        assert isinstance(literal, str)
        self.literal = literal

    def to_string(self):
        return self.literal


# wraps the given pretty printer: prepends given prefix to its display string
class PrefixPrinter:
    def __init__(self, prefix, pointee_printer):
        assert isinstance(prefix, str)
        self.pointee_printer = pointee_printer
        self.prefix = prefix

    def to_string(self):
        try:    # printer can have no 'to_string' method
            res = str(self.pointee_printer.to_string())
        except:
            res = '???'
        return self.prefix + res

    def children(self):
        try:    # printer can have no 'children' method
            return self.pointee_printer.children()
        except:
            return []

# ===================================================================================

# custom collection for our pretty printers with more powers:
#  * cast value to its max-derived type via RTTI
#  * allow_pointer: resolve pointer/reference once
#  * allow_derived: apply pretty printer of base type to derived type
#  * simpler registration of types: wildcards, static class properties, auto printer name
class TdmPrettyPrinterCollection(gdb.printing.PrettyPrinter):
    
    class TdmSubprinter(gdb.printing.SubPrettyPrinter):
        def __init__(self, gen_printer, *,
                     regex = None, wildcard = None,
                     name = None,
                     allow_pointer = True, allow_derived = False):
            if regex is not None:
                compiled_re = re.compile(regex)
                self.match_type = lambda tn: compiled_re.search(tn)
            elif wildcard is not None:
                compiled_re = re.compile(fnmatch.translate(wildcard))
                self.match_type = lambda tn: compiled_re.match(tn)
            else:
                assert False, 'Matching criterion not set'

            if name is None:
                name = gen_printer.__name__

            super(TdmPrettyPrinterCollection.TdmSubprinter, self).__init__(name)
            self.gen_printer = gen_printer
            self.allow_pointer = allow_pointer
            self.allow_derived = allow_derived

    def __init__(self, name):
        super(TdmPrettyPrinterCollection, self).__init__('tdm_' + name, [])

    def add_printer(self, gen_printer, **kwargs):
        self.subprinters.append(self.TdmSubprinter(gen_printer, **kwargs))

    # --------- registration: take properties from class static member

    def add_printer_implicit(self, gen_printer):
        kwargs = {}
        for pn in ['regex', 'wildcard', 'name', 'allow_pointer', 'allow_derived']:
            if hasattr(gen_printer, pn):
                kwargs[pn] = getattr(gen_printer, pn)
        self.add_printer(gen_printer, **kwargs)

    @staticmethod
    def create_with_printers(name, gen_printer_list):
        ppcoll = TdmPrettyPrinterCollection(name)
        for gp in gen_printer_list:
            ppcoll.add_printer_implicit(gp)
        return ppcoll
    
    # --------- advanced search for matching pretty-printer

    # same as in the implementation of RegexpCollectionPrettyPrinter
    # most importantly, it follows typedefs and drops const/volatile
    @staticmethod
    def _get_normal_type_name(type):
        typename = gdb.types.get_basic_type(type).tag
        if typename:
            return typename
        return type.name
        
    # returns sequence of base class types
    # starting from specified type and going into bases
    # note: stops on multiple inheritance!
    # in case of pointer/reference, all bases also have pointer/reference
    @staticmethod
    def _get_base_types(type):
        if type.code == gdb.TYPE_CODE_PTR:
            value_bases = TdmPrettyPrinterCollection._get_base_types(type.target())
            return [bt.pointer() for bt in value_bases]
        
        if type.code == gdb.TYPE_CODE_REF or type.code == gdb.TYPE_CODE_RVALUE_REF:
            value_bases = TdmPrettyPrinterCollection._get_base_types(type.target())
            return [bt.reference() for bt in value_bases]
        
        if type.code != gdb.TYPE_CODE_STRUCT:
            return [type]

        res = []
        while True:
            res.append(type)
            base_names = [f.name for f in type.fields() if f.is_base_class]
            if len(base_names) != 1:
                break   # no base or multiple inheritance

            base_type = gdb.lookup_type(base_names[0])
            type = base_type

        return res

    # find printer with specified type name exactly
    def _find_printer_exact(self, typename, is_deref, is_base):
        for printer in self.subprinters:
            if not printer.enabled:
                continue
            if is_deref and not printer.allow_pointer:
                continue
            if is_base and not printer.allow_derived:
                continue
            if not printer.match_type(typename):
                continue
            return printer
        return None
    
    # find printer for given type; include its base types in the search
    def _find_printer_maybe_derived(self, type, is_deref):
            base_type_sequence = self._get_base_types(type)

            for i, base_type in enumerate(base_type_sequence):
                typename = self._get_normal_type_name(base_type)
                if not typename:
                    continue

                printer = self._find_printer_exact(typename, is_deref, i > 0)
                if printer:
                    return printer

    # called by GDB to find pretty-printer
    def __call__(self, value):
        # cast to most derived type based on RTTI
        # natvis does this automatically as well
        dynamic_type = value.dynamic_type
        if dynamic_type != value.type:
            value = value.cast(dynamic_type)

        # resolve 'as is': normally works for value types
        # can work for pointer only if printer has pointer in its wildcard
        type = value.type
        printer = self._find_printer_maybe_derived(type, False)
        if printer:
            return printer.gen_printer(value)

        # resolve reference type
        if type.code == gdb.TYPE_CODE_REF or type.code == gdb.TYPE_CODE_RVALUE_REF:
            value = value.referenced_value()
            type = value.type
            printer = self._find_printer_maybe_derived(type, False)
            if printer:
                return printer.gen_printer(value)
            
        # resolve pointer type (address is prepended)
        if type.code == gdb.TYPE_CODE_PTR:
            if int(value) == 0:
                return LiteralStringPrinter('null')
            prefix = '0x{:x} '.format(int(value))
            value = value.dereference()
            type = value.type
            printer = self._find_printer_maybe_derived(type, True)
            if printer:
                return PrefixPrinter(prefix, printer.gen_printer(value))            

        return None

# ===================================================================================
# this hack provides feature similar to the ",view(*)" and "Synthetic" of natvis
# it allows to pin specific pretty-printer class to a gdb.Value

# fixed-size circular buffer
# every added element has ID: you can find it by ID and check if it was removed
# special addition method drops oldest values to keep total size under budget
class GdbHelperCircularBuffer:
    def __init__(self, max_count, max_size):
        self.max_count = max_count
        self.max_size = max_size
        self.stat_count = 0     # number of values
        self.stat_size = 0      # sum of payload bytes inside values
        self.array = [None] * self.max_count
        self.beg = 0
        self.end = 0

    def remove(self):
        assert self.end - self.beg > 0
        elem = self.array[self.beg % len(self.array)]
        self.beg += 1
        self.stat_count -= 1
        self.stat_size -= elem.size
        return elem

    def add(self, elem):
        assert self.end - self.beg < self.max_count
        id = self.end
        self.array[id % len(self.array)] = elem
        self.end += 1
        self.stat_count += 1
        self.stat_size += elem.size
        return id
    
    # if budget is exceeded, then removes oldest values automatically
    # budget is: don't overwrite itself & sum of "e.size" is limited
    def add_limited(self, elem):
        assert elem.size <= self.max_size
        while self.stat_count + 1 >= self.max_count or self.stat_size + elem.size >= self.max_size:
            self.remove()
        
        id = self.add(elem)
        return id

    def get(self, id):
        if not (id >= self.beg and id < self.end):
            return None     # already removed
        index = id % len(self.array)
        value = self.array[index]
        return value

# every gdb.Value with pinned printer is stored in this huge global buffer
# we can't know when they are no longer necessary, so we only remove them after long time when budget is exceeded
g_gdb_helper_table = GdbHelperCircularBuffer(10 ** 6, 10 ** 9)
GdbHelperEntry = namedtuple('GdbHelperEntry', ['value', 'gen_printer', 'size', 'traceback'])

# returns artificial gdb.Value which is later resolved to specified (gdb.Value + pretty-printer class) combination
# information is stored in the global table, and entry ID is wrapped into GdbHelper struct
def embed_printer_for_value(value, gen_printer):
    tb = traceback.extract_stack()
    id = g_gdb_helper_table.add_limited(GdbHelperEntry(value, gen_printer, len(value.bytes), tb))
    helper_type = gdb.lookup_type('GdbHelper')
    assert helper_type and helper_type.sizeof == 8, "Make sure GdbHelper struct exists and is not stripped out."
    helper_value = gdb.Value(id.to_bytes(8, byteorder = 'little'), helper_type)
    return helper_value

# pretty-printer for GdbHelper
# extracts entry ID and looks into the global table
def GdbHelperPrinter(value):
    id = int.from_bytes(value.bytes, byteorder = 'little')
    elem = g_gdb_helper_table.get(id)
    if elem is None:
        return LiteralStringPrinter('{{value is obsolete}}')
    return elem.gen_printer(elem.value)

# register the pretty-printer for GdbHelper
def register_gdb_helper():
    coll = gdb.printing.RegexpCollectionPrettyPrinter('GdbHelper')
    coll.add_printer('GdbHelper', '^GdbHelper$', GdbHelperPrinter)
    gdb.printing.register_pretty_printer(gdb.current_objfile(), coll, replace = True)
register_gdb_helper()

# ===================================================================================
# enumerates all raw members of a class, including base classes
# there are two ways of using it:
#  1) provide members inline, like <ExpandedItem>this,!</ExpandedItem> from natvis:
#       yield from raw_children_inline(value)
#  2) all a synthetic child with raw members, natvis does this automatically:
#       yield raw_child_expandable(value)

class RawSubclassPrinter:
    def __init__(self, value):
        self.value = value

    def to_string(self):
        return ''

    def children(self):
        for f in self.value.type.fields():
            if f.artificial:
                continue
            if f.is_base_class:
                base_type = gdb.lookup_type(f.name)
                base_value = self.value.cast(base_type)
                yield ('%s {base}' % f.name, embed_printer_for_value(base_value, RawSubclassPrinter))
                continue
            yield (f.name, self.value[f.name])

class RawPrinter(RawSubclassPrinter):
    def __init__(self, value):
        super().__init__(value)

    def to_string(self):
        return '[expand to see raw children]'

# yield this from SomePrinter.children to add expandable [raw] child like in natvis
def raw_child_expandable(value):
    return ('[raw]', embed_printer_for_value(value, RawPrinter))

# yield this from SomePrinter.children to add raw members inline
def raw_children_inline(value):
    return RawPrinter(value).children()

# ===================================================================================

# returns "display string" for a value (in natvis terms)
# this is what MatchedPrinter.to_string returns, without looking at children
# works fine for types without pretty-printer, e.g. primitive type
def display_string(value):
    pp = gdb.default_visualizer(value)
    if not pp:
        return str(value)
    return pp.to_string()

# returns children list for a value
# this is what MatchedPrinter.children returns
# returns empty list for a type without pretty-printer, e.g. for primitive type
def children_of(value, skip_raw = True):
    pp = gdb.default_visualizer(value)
    if not pp:
        return []
    for x in pp.children():
        if skip_raw and x[0] == '[raw]':
            continue
        yield x
