import gdb
import re, fnmatch, traceback, string
from collections import namedtuple

# this is the riskiest pretty-printer!
# it applies to ALL structs/pointers/arrays without custom pretty printer
# it has the largest chance to cause debug issues...
ENABLE_AUTO_SUMMARY = True

# return children list as generator in some builtin methods, like e.g. arrays or raw members
# this makes CLion much faster if large arrays are present
# but makes it harder to debug and turns some children lists into LazyArray, which has less methods than proper list
USE_GENERATORS_FOR_CHILDREN = True


# wraps an iterable into a lazy container
# which can be iterated through as many times as wanted
# and which evaluates elements on-demand and caches them internally
class CachedIterable:
    def __init__(self, iterable):
        self.prefix = []
        self.iterator = iter(iterable)

    def __iter__(self):
        return self.Iterator(self)
    
    class Iterator:
        def __init__(self, owner):
            self.owner = owner
            self.index = 0

        def __iter__(self):
            return self
        
        def __next__(self):
            if self.index < len(self.owner.prefix):
                ret = self.owner.prefix[self.index]
            else:
                assert self.index == len(self.owner.prefix)
                ret = next(self.owner.iterator)
                self.owner.prefix.append(ret)
            self.index += 1
            return ret

# list-like container of children
# it can be constructed from list and from generator,
# and it can be composited using append and '+' like with lists
class LazyArray:
    def __init__(self, arr = None):
        self.chunks = []
        if arr is not None:
            self.chunks.append(self._wrap_iterator(arr))

    def _wrap_iterator(self, arr):
        assert not isinstance(arr, LazyArray)
        if isinstance(arr, list):
            return arr
        assert hasattr(arr, '__iter__')
        return CachedIterable(arr)

    def clear(self):
        self.chunks.clear()

    def append(self, elem):
        self.chunks.append([elem])

    def __add__(self, other):
        if not isinstance(other, LazyArray):
            other = LazyArray(other)
        res = LazyArray()
        res.chunks = self.chunks + other.chunks
        return res

    def __radd__(self, other):
        if not isinstance(other, LazyArray):
            other = LazyArray(other)
        res = LazyArray()
        res.chunks = other.chunks + self.chunks
        return res

    def __iter__(self):
        for arr in self.chunks:
            yield from arr

    def __bool__(self):
        for i in self:
            return True
        return False

# this is a decorator applied to a few methods which return children as generator/iterator for performance reasons
def wrap_children_generator(generator):
    def wrapper(*args, **kwargs):
        iter = generator(*args, **kwargs)
        if USE_GENERATORS_FOR_CHILDREN:
            return LazyArray(iter)
        else:
            return list(iter)   # unwrap into explicit list immediately
    return wrapper

# ===================================================================================

# given gdb.Value and its gdb.Field, returns gdb.Value corresponding to the field
# note: unlike simple value[field.name], this function supports unnamed structs and unions
def get_field(value, field):
    # https://stackoverflow.com/questions/16951821/gdb-7-2-python-how-to-get-members-of-anonymous-structure
    if field.name:
        return value[field.name]
    if field.type is not None and field.bitpos is not None:
        assert field.bitpos % 8 == 0
        offset = field.bitpos // 8
        size = field.type.sizeof
        return gdb.Value(value.bytes[offset : offset + size], field.type)
    assert False, "Failed to get field of value"


# returns (N, elemType) for C++ array types
def get_array_length_and_element_type(vtype):
    assert vtype.code != gdb.TYPE_CODE_TYPEDEF
    elemtype = vtype.target()
    n = vtype.sizeof // elemtype.sizeof
    return (n, elemtype)

# ===================================================================================

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

TrieNode = namedtuple('TrieNode', ['next', 'term'])
# matches type name against many wildcards quickly
class MultiWildcardMatcher:
    def __init__(self):
        self.root = TrieNode({}, [])

    def add(self, wildcard, value = None):
        node = self.root
        for ch in wildcard:
            if ch in ['*', '?', '[']:
                break
            if ch not in node.next:
                node.next[ch] = TrieNode({}, [])
            node = node.next[ch]
        compiled_re = re.compile(fnmatch.translate(wildcard))
        node.term.append((compiled_re, value))

    def match(self, text):
        node = self.root
        n = len(text)
        for i in range(n + 1):
            for regex, value in node.term:
                if regex.match(text):
                    yield value
            if i == n:
                break
            node = node.next.get(text[i])
            if node is None:
                break


# custom collection for our pretty printers with more powers:
#  * cast value to its max-derived type via RTTI
#  * allow_pointer: resolve pointer/reference once
#  * allow_derived: apply pretty printer of base type to derived type
#  * simpler registration of types: wildcards, static class properties, auto printer name
class TdmPrettyPrinterCollection(gdb.printing.PrettyPrinter):
    
    class TdmSubprinter(gdb.printing.SubPrettyPrinter):
        def __init__(self, gen_printer, matcher, *,
                     wildcard = None,
                     name = None,
                     allow_pointer = True, allow_derived = False):
            assert wildcard, 'Matching criterion not set'
            if not isinstance(wildcard, list):
                wildcard = [wildcard]
            for w in wildcard:
                matcher.add(w, self)

            if name is None:
                name = gen_printer.__name__
            super(TdmPrettyPrinterCollection.TdmSubprinter, self).__init__(name)
            self.gen_printer = gen_printer
            self.allow_pointer = allow_pointer
            self.allow_derived = allow_derived

    def __init__(self, name):
        super(TdmPrettyPrinterCollection, self).__init__('tdm_' + name, [])
        self.matcher = MultiWildcardMatcher()

    def add_printer(self, gen_printer, **kwargs):
        self.subprinters.append(self.TdmSubprinter(gen_printer, self.matcher, **kwargs))

    # --------- registration: take properties from class static member

    def add_printer_implicit(self, gen_printer):
        kwargs = {}
        for pn in ['wildcard', 'name', 'allow_pointer', 'allow_derived']:
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
    @staticmethod
    def _get_base_types(type):
        assert type.code != gdb.TYPE_CODE_TYPEDEF
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
        if not typename:
            return None
        for printer in self.matcher.match(typename):
            if not printer.enabled:
                continue
            if is_deref and not printer.allow_pointer:
                continue
            if is_base and not printer.allow_derived:
                continue
            return printer
        return None

    # find printer for given type; include its base types in the search
    def _find_printer_maybe_derived(self, type, is_deref):
        type = gdb.types.get_basic_type(type)   # avoid TYPE_CODE_TYPEDEF
        if type.code != gdb.TYPE_CODE_STRUCT and type.code != gdb.TYPE_CODE_UNION:
            return None

        base_type_sequence = self._get_base_types(type)

        for i, base_type in enumerate(base_type_sequence):
            typename = self._get_normal_type_name(base_type)
            if not typename:
                continue

            printer = self._find_printer_exact(typename, is_deref, i > 0)
            if printer:
                return printer

    HANDLED_TYPE_CODES = {
        gdb.TYPE_CODE_STRUCT, gdb.TYPE_CODE_UNION,
        gdb.TYPE_CODE_PTR, gdb.TYPE_CODE_REF, gdb.TYPE_CODE_RVALUE_REF,
        gdb.TYPE_CODE_TYPEDEF,
    }

    # called by GDB to find pretty-printer
    def __call__(self, value):
        if value.type.code not in self.HANDLED_TYPE_CODES:
            return None

        # cast to most derived type based on RTTI
        # natvis does this automatically as well
        dynamic_type = value.dynamic_type
        if dynamic_type != value.type:
            value = value.cast(dynamic_type)

        # resolve 'as is': works for value types
        printer = self._find_printer_maybe_derived(value.type, False)
        if printer:
            return printer.gen_printer(value)

        # avoid TYPE_CODE_TYPEDEF below
        value = value.cast(gdb.types.get_basic_type(value.type))

        # resolve reference type
        if value.type.code == gdb.TYPE_CODE_REF or value.type.code == gdb.TYPE_CODE_RVALUE_REF:
            target_value = value.referenced_value()
            printer = self._find_printer_maybe_derived(target_value.type, False)
            if printer:
                return printer.gen_printer(target_value)

        # resolve pointer type (address is prepended)
        if value.type.code == gdb.TYPE_CODE_PTR:
            if int(value) == 0:
                return LiteralStringPrinter('null')
            prefix = '0x{:x} '.format(int(value))
            try:
                target_value = value.dereference()
            except:
                return LiteralStringPrinter('bad')  # includes pointer to void
            printer = self._find_printer_maybe_derived(target_value.type, True)
            if printer:
                return PrefixPrinter(prefix, printer.gen_printer(target_value))

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
GdbHelperEntry = namedtuple('GdbHelperEntry', ['value', 'gen_printer', 'size'])

# returns artificial gdb.Value which is later resolved to specified (gdb.Value + pretty-printer class) combination
# information is stored in the global table, and entry ID is wrapped into GdbHelper struct
def embed_printer_for_value(value, gen_printer, *, size = None):
    if size is None:
        size = len(value.bytes)
    id = g_gdb_helper_table.add_limited(GdbHelperEntry(value, gen_printer, size))
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
#       res += raw_children_inline(value)
#  2) all a synthetic child with raw members, natvis does this automatically:
#       res.append(raw_child_expandable(value))

class RawSubclassPrinter:
    def __init__(self, value):
        # avoid TYPE_CODE_TYPEDEF
        value = value.cast(gdb.types.get_basic_type(value.type))
        self.value = value

    def to_string(self):
        return ''

    @wrap_children_generator
    def children(self):
        vtype = self.value.type
        assert vtype.code != gdb.TYPE_CODE_TYPEDEF

        if vtype.code in [gdb.TYPE_CODE_STRUCT, gdb.TYPE_CODE_UNION]:
            for f in vtype.fields():
                if f.artificial:
                    continue
                if f.is_base_class:
                    base_type = gdb.lookup_type(f.name)
                    base_value = self.value.cast(base_type)
                    child = ('%s {base}' % f.name, embed_printer_for_value(base_value, RawSubclassPrinter))
                    yield child
                    continue
                child = (str(f.name), get_field(self.value, f))
                yield child

        if vtype.code == gdb.TYPE_CODE_ARRAY:
            (n, elemtype) = get_array_length_and_element_type(vtype)
            for i in range(n):
                yield ('[%d]' % i, self.value[i])

class RawPrinter(RawSubclassPrinter):
    def __init__(self, value):
        super().__init__(value)

    def to_string(self):
        return '[expand to see raw children]'

# add this child to the returned list in SomePrinter.children to add expandable [raw] child like in natvis
def raw_child_expandable(value):
    return ('[raw]', embed_printer_for_value(value, RawPrinter))

# append this array to the returned list in SomePrinter.children to add raw members inline
def raw_children_inline(value):
    return RawPrinter(value).children()

# ===================================================================================

# which values are allowed to be printed using builtin GDB algorithm
# we must be very careful to avoid the infinite deep-printing issue of GDB here!
def print_simple_value(value):
    vtype = gdb.types.get_basic_type(value.type)
    if vtype.code in [gdb.TYPE_CODE_ENUM, gdb.TYPE_CODE_INT, gdb.TYPE_CODE_FLT, gdb.TYPE_CODE_CHAR, gdb.TYPE_CODE_BOOL]:
        return str(value)
    if vtype.code in [gdb.TYPE_CODE_PTR, gdb.TYPE_CODE_ARRAY] and vtype.target().name == 'char':
        if vtype.code == gdb.TYPE_CODE_PTR and int(value) == 0:
            return 'null'
        try:
            return '"' + str(value.string()) + '"'
        except:
            return 'bad'
    return None

# getting custom pretty printer for a given value
# note: AutoDisplayStringPrinter is considered "default", not "custom"
def default_visualizer(value):
    pp = gdb.default_visualizer(value)
    if isinstance(pp, AutoDisplayStringPrinter):
        return None
    return pp

# returns "display string" for a value (in natvis terms)
# this is what MatchedPrinter.to_string returns, without looking at children
# works fine for types without pretty-printer, e.g. primitive type
def display_string(value):
    if not isinstance(value, gdb.Value):
        return str(value)
    pp = default_visualizer(value)
    if pp:
        return pp.to_string()
    simple = print_simple_value(value)
    if simple is not None:
        return simple
    return '???'


# returns children list for a value
# this is what MatchedPrinter.children returns
# returns raw members for a type without pretty-printer (empty for primitive types)
def children_of(value, skip_raw_child = True):
    pp = default_visualizer(value)
    if not pp:
        return raw_children_inline(value)
    res = []
    for x in pp.children():
        if skip_raw_child and x[0] == '[raw]':
            continue
        res.append(x)
    return res

# ===================================================================================

# returns all elements of the given array as a list of children
@wrap_children_generator
def array_children_list(ptr_value, count_value):
    n = int(count_value)
    for i in range(n):
        yield (str(i), ptr_value[i])

# returns all elements of the given linked list as a list of children
# terminates on None, null pointer, optional lambda, or revisiting the same node
@wrap_children_generator
def linked_list_children_list(first_node, func_next_node, func_item_of_node = None, *, terminate_if = None, marked_if = None):
    if not terminate_if:
        terminate_if = lambda p: False
    if not func_item_of_node:
        func_item_of_node = lambda p: p
    if not marked_if:
        marked_if = lambda p: False
    pnode = first_node
    visited_addresses = set()
    k = 0
    while pnode and int(pnode) != 0 and not terminate_if(pnode):
        name = '[%d]' % k
        if marked_if(pnode):
            name = '=>' + name
        cycled = int(pnode) in visited_addresses
        if cycled:
            name = '[cycle]'
        yield (name, func_item_of_node(pnode))
        k += 1
        visited_addresses.add(int(pnode))
        if cycled:
            break
        pnode = func_next_node(pnode)

# returns a synthetic gdb.Value that can be expanded to display the given list of children
# children_lambda should be a lambda wrapping a list of children for lazy evaluation
def make_synthetic(children_lambda, display = ''):
    class SyntheticPrinter:
        def __init__(self, value):
            pass
        def to_string(self):
            return display
        def children(self):
            if callable(children_lambda):
                return children_lambda()
            else:
                return children_lambda
    return embed_printer_for_value(gdb.Value(0), SyntheticPrinter)

# ===================================================================================

This = '$%#this#%$'

# Children tree is a json-like structure like this:
# {
#     'size': 15,                   # member 'size' = 15
#     'parent': This,               # member 'parent' = self.value['parent'] (taken from 'this' object)
#     'duration': gdb.Value(...),   # member with specified gdb.Value
#     '@hidden': gdb.Value(...)     # hidden member: not displayed, but can be referenced in display string
#     'extra': {                    # synthetic member: can be expanded to see its contents
#         'angle': 179.0
#         'analysed': gdb.Value(...),
#     },
#     '^dsfsd': raw_child_expandable(value),    # add expandable synthetic with all raw members (key name ignored)
#     '^oiwer': raw_children_inline(value),     # list all raw members here (key name ignored)
#     '^ivnfd': [('a': 7), ('b'): 15]           # in general case, '^???' means "insert this child/children list here"
# }
#
# Formatted display string looks similar to Python format string, for example:
#   'From {@parent} under angle {@extra.angle}: {@hidden}'
# The placeholders are surrounded with {}, and can be of two types:
#   {@path} --- use value from children tree under specified path
#   {$name} --- use self.value[name], i.e. take from 'this' object
# You can also add format specification after colon:
#   {$ptrToObject:*} --- take ptrToObject member and dereference it before display
#   {$mystruct:a} --- generate auto-summary for mystruct member (not recommended)


# given children tree as described above, or a lambda that returns it,
# returns same tree with all the values resolved as gdb.Value, 'this' no longer needed
def preprocess_children_tree(tree, thisValue):
    if tree is None:
        return raw_children_inline(thisValue)   # default if tree not specified
    if callable(tree):
        structure = tree(thisValue)             # typically the tree is behind lambda

    def preprocess_structure_recursive(tree):
        if isinstance(tree, dict):
            tree = list(tree.items())           # dict literal is alternative to list of tuples
        result = []
        for key, val in tree:
            if key.startswith('^'):             # insert value = list of key/value tuples (or one)
                if isinstance(val, (list, LazyArray)):
                    result += val
                else:
                    result.append(val)
                continue
            elif isinstance(val, str) and val == This:  # use member of 'this' by name
                val = thisValue[key]
            elif isinstance(val, (dict, list, LazyArray)):      # synthetic subobject
                val = preprocess_structure_recursive(val)
            elif isinstance(val, gdb.Value):            # normal value
                pass
            else:                                       # primitives like int, float, string
                val = gdb.Value(val)
            result.append((key, val))
        return result

    return preprocess_structure_recursive(structure)

# tracked against total memory limit of g_gdb_helper_table table
def estimate_size_of_preprocessed_tree(tree):
    res = 0
    for key, val in tree:
        if isinstance(val, (list, LazyArray)):
            res += estimate_size_of_preprocessed_tree(val)
        res += len(val.bytes)
    return res

# pretty-printer-like class used for synthetic objects
# note: unlike true PP, it accepts its contents instead of gdb.Value
# so it can only be used from inside convert_preprocessed_tree_into_children_list
class ContainerPrinter:
    def __init__(self, tree):
        self.tree = tree
    def children(self):
        return convert_preprocessed_tree_into_children_list(self.tree)

# converts the result of preprocess_children_tree to be returned from 'children' method of pretty printer
def convert_preprocessed_tree_into_children_list(tree):
    result = []
    for key, val in tree:
        if key.startswith('@'):     # hidden
            continue
        if isinstance(val, (list, LazyArray)):
            val = embed_printer_for_value(val, ContainerPrinter, size = estimate_size_of_preprocessed_tree(val))
        result.append((key, val))
    return result

# find member by name (or even path) inside the preprocessed children tree
# this is used by the feature: {@mystuff.data.first} in display string
def fetch_from_preprocessed_tree(tree, path):
    assert tree is not None
    parts = path.split('.')
    node = tree
    for p in parts:
        nnode = None
        for k, v in node:
            if k == p or k == '@' + p:
                assert nnode is None
                nnode = v
        node = nnode
    return node

# creates string from special 'format string', gdb value of 'this' and optionally preprocessed children tree
def compose_formatted_display_string(format, value, tree):
    pieces = string.Formatter().parse(format)
    res = ''
    for text, placeholder, specs, _ in pieces:
        res += text
        if placeholder:
            if placeholder.startswith('@'):
                member = fetch_from_preprocessed_tree(tree, placeholder[1:])
            elif placeholder.startswith('$'):
                member = value[placeholder[1:]]
            else:
                assert False
            try:
                if '*' in specs:
                    member = member.dereference()
            except:
                member = 'err'
            if ENABLE_AUTO_SUMMARY and specs.endswith('a'):
                s = AutoDisplayStringPrinter(member).to_string()
            else:
                s = display_string(member)
            res += str(s)
    return res

# Makes pretty-printer using simply and mostly declarative format.
# The return value can be registered directly in TdmPrettyPrinterCollection.
#   typename: C++ class name (can be wildcard)
#   format: will be shown as 'display string', placeholders like {...} are replaced
#   structure: json-like structure that describes 'children', possibly with synthetic subchildren
# See large comment above for syntax and possibilities of display string format and json-like structure.
# Note: if structure is None, then you can't use {@smth} placeholders in display string.
def make_simple_printer(typename, format, structure = None, *, class_attribs = {}):
    class SimplePrinter:
        def __init__(self, value):
            self.value = value
            self.structure = preprocess_children_tree(structure, value)
        def to_string(self):
            if ENABLE_AUTO_SUMMARY and getattr(SimplePrinter, 'auto_summary', False):
                return get_auto_summary_as_string(self.value)
            return compose_formatted_display_string(format, self.value, self.structure)
        def children(self):
            return convert_preprocessed_tree_into_children_list(self.structure)
    if typename is not None:
        setattr(SimplePrinter, 'wildcard', typename)
        SimplePrinter.__name__ += '(%s)' % typename
    for k, v in class_attribs.items():
        setattr(SimplePrinter, k, v)
    return SimplePrinter

# ===================================================================================

# auto-summary perform length-limited deep-printing
# this is how many characters it produces at maximum
AutoSummaryLengthLimitDefault = 100

# helper for simple concatenation of strings into a text of limited length
class StringBuilder:
    def __init__(self, limit = AutoSummaryLengthLimitDefault):
        self.limit = limit
        self.args = []
        self.size = 0

    def overflow(self):
        return self.size > self.limit

    def append(self, s):
        self.args.append(s)
        self.size += len(s)
        return self.overflow()

    def finalize(self):
        text = ''.join(self.args)
        if self.overflow():
            text = text[ : self.limit] + '...'
        return text

# appends deep-printed text for the given value
# return True iff build is overflown after the call
def append_deep_summary(builder, value):
    simple = print_simple_value(value)
    if simple is not None:
        return builder.append(simple)

    # avoid TYPE_CODE_TYPEDEF
    value = value.cast(gdb.types.get_basic_type(value.type))

    if value.type.code == gdb.TYPE_CODE_ARRAY:
        (n, elemtype) = get_array_length_and_element_type(value.type)
        if builder.append('{ '):
            return True
        for i in range(n):
            if i > 0 and builder.append(', '):
                return True
            if append_deep_summary(builder, value[i]):
                return True
        if builder.append(' }'):
            return True
        return False

    pp = default_visualizer(value)
    if pp and not getattr(type(pp), 'auto_summary', False):
        # never deep-print custom types with pretty-printer
        return builder.append(pp.to_string())

    if value.type.code == gdb.TYPE_CODE_PTR:
        if int(value) == 0:
            return builder.append('null')
        if builder.append('0x{:x} '.format(int(value))):
            return True
        try:
            target_value = value.dereference()
        except:
            return builder.append('bad') # includes void
        return append_deep_summary(builder, target_value)

    if value.type.code not in [gdb.TYPE_CODE_STRUCT, gdb.TYPE_CODE_UNION]:
        return builder.append('???')

    count = 0
    def process_base(tvalue):
        for f in tvalue.type.fields():
            if f.artificial:
                continue
            if f.is_base_class:
                base_type = gdb.lookup_type(f.name)
                base_value = tvalue.cast(base_type)
                if process_base(base_value):
                    return True
                continue
            nonlocal count
            if builder.append('%s%s: ' % (', ' if count > 0 else '', f.name)):
                return True
            count += 1
            if append_deep_summary(builder, get_field(tvalue, f)):
                return True
        return False

    if builder.append('{ '):
        return True
    if process_base(value):
        return True
    if builder.append(' }'):
        return True
    return False

# returns auto summary as a final string with limit applied
# for performance reasons, it should not be concatenated with anything else...
def get_auto_summary_as_string(value, prefix = ''):
    builder = StringBuilder()
    builder.append(prefix)
    if value is not None:
        append_deep_summary(builder, value)
    return builder.finalize()


# pretty-printer to produce auto-summary for non-customized types
class AutoDisplayStringPrinter:
    def __init__(self, value, prefix = ''):
        self.value = value
        self.prefix = prefix
    def to_string(self):
        return get_auto_summary_as_string(self.value, self.prefix)
    def children(self):
        if self.value is None:
            return []
        return raw_children_inline(self.value)

# auto-summary pretty printer must be separate
# it has to have the least priority, and it has the greatest risk of breaking everything =)
class AutoDisplayStringDefaultPrinterCollection(gdb.printing.PrettyPrinter):
    def __init__(self):
        super(AutoDisplayStringDefaultPrinterCollection, self).__init__('auto_display_string_printer', [])

    HANDLED_TYPE_CODES = {
        gdb.TYPE_CODE_STRUCT, gdb.TYPE_CODE_UNION, gdb.TYPE_CODE_ARRAY,
        gdb.TYPE_CODE_PTR, gdb.TYPE_CODE_REF, gdb.TYPE_CODE_RVALUE_REF,
        gdb.TYPE_CODE_TYPEDEF,
    }

    def __call__(self, value):
        if value.type.code not in self.HANDLED_TYPE_CODES:
            return None

        # cast to most derived type based on RTTI
        # natvis does this automatically as well
        dynamic_type = value.dynamic_type
        if dynamic_type != value.type:
            value = value.cast(dynamic_type)

        # avoid TYPE_CODE_TYPEDEF
        value = value.cast(gdb.types.get_basic_type(value.type))

        # resolve reference type
        if value.type.code == gdb.TYPE_CODE_REF or value.type.code == gdb.TYPE_CODE_RVALUE_REF:
            value = value.referenced_value()
            value = value.cast(gdb.types.get_basic_type(value.type))

        prefix = ""
        for d in range(3):
            if value.type.code in [gdb.TYPE_CODE_STRUCT, gdb.TYPE_CODE_UNION, gdb.TYPE_CODE_ARRAY]:
                return AutoDisplayStringPrinter(value, prefix)
            if value.type.code != gdb.TYPE_CODE_PTR:
                break
            # resolve pointer type (address is prepended)
            if int(value) == 0:
                return AutoDisplayStringPrinter(None, prefix + 'null')
            prefix += '0x{:x} '.format(int(value))
            try:
                value = value.dereference()
                value = value.cast(gdb.types.get_basic_type(value.type))
            except:
                return AutoDisplayStringPrinter(None, prefix + 'bad')

        return None

def register_auto_display_string(event = None):
    # according to docs, gdb.pretty_printers has the least priority of 3 groups
    for i, x in enumerate(gdb.pretty_printers):
        if isinstance(x, gdb.printing.PrettyPrinter) and x.name == 'auto_display_string_printer':
            del gdb.pretty_printers[i]
            break
    gdb.pretty_printers.append(AutoDisplayStringDefaultPrinterCollection())
    # CLion has builtin printer 'lookup' which covers all arrays in a way we don't like
    # in order to see array contents in display string, we have to override it
    gdb.execute('disable pretty-printer global lookup')

if ENABLE_AUTO_SUMMARY:
    # delay registration until executable is loaded into GDB
    # this allows us to set this pretty printer last
    # also it is necessary because 'lookup' is loaded late
    gdb.events.executable_changed.connect(register_auto_display_string)
    # but also run it now: this helps for manual refresh of pretty printers without restarting debugging
    register_auto_display_string()
