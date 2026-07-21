import gdb
from tdmuc_base import *
from tdmuc_idlib import *
from tdmuc_engine import *


class idEntityPrinter:
    wildcard = 'idEntity'
    allow_derived = True        # there are many derived classes

    def __init__(self, value):
        self.value = value

    def to_string(self):
        return 'Entity(%d): %s ' % (
            int(self.value['entityNumber']),
            display_string(self.value['name'])
        )
    
    def children(self):
        res = raw_children_inline(self.value)
        def synthetic():
            return linked_list_children_list(
                self.value['teamMaster'],
                lambda p: p.dereference()['teamChain'],
                marked_if = lambda p: int(p) == self.value.address,
            )
        child = ('[Team]', make_synthetic(synthetic))
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
            text = '[WRONG] ' + text
        return text
    
    def children(self):
        key = '[WRONG]' if self.invalid else '[entity]'
        res = [
            (key, self.ptrValue),
            raw_child_expandable(self.value),
        ]
        return res


class idEventPrinter:
    wildcard = 'idEvent'

    def __init__(self, value):
        self.value = value
        threadexec = gdb.lookup_global_symbol('EV_Thread_Execute', gdb.SYMBOL_VAR_DOMAIN).value().address
        self.is_thread_exec = int(self.value['eventdef']) == int(threadexec)

    def to_string(self):
        time = int(self.value['time'])
        if self.is_thread_exec:
            threadname = self.value['object'].cast(gdb.lookup_type('idThread*')).dereference()['threadName']
            threadname = idStrPrinter(threadname).to_string()
            return 'idThread::Execute (%d): %s' % (time, threadname)
        classname = print_simple_value(self.value['typeinfo'].dereference()['classname'])
        eventname = print_simple_value(self.value['eventdef'].dereference()['name'])
        objdata = display_string(self.value['object'].dereference())
        return '%s::%s (%d): %s' % (classname, eventname, time, objdata)

    def children(self):
        return raw_children_inline(self.value)


game_pplist = [
    idEntityPrinter,
    idEntityPtrPrinter,
    idEventPrinter,
]

game_pplist += [
    make_simple_printer('activeSmokeStage_t', 'activeSmoke', lambda v: {
        '^':  raw_children_inline(v),
        '[All Particles]': make_synthetic(lambda: linked_list_children_list(
            v['smokes'],
            lambda n: n.dereference()['next']
        ))
    }, class_attribs = {'auto_summary': True}),
    make_simple_printer('singleSmoke_s', 'smoke[{$index}] T{$privateStartTime} at {$origin}'),
]

game_pplist += [
    make_simple_printer('contactInfo_t', 'contact at {$point}', lambda v: {
        '^':  raw_children_inline(v),
        '[Entity]': idListPrinter(getGameLocal()['entities']).get(int(v['entityNum'])),
    }, class_attribs = {'auto_summary': True}),
]

def get_pretty_printers():
    return game_pplist
