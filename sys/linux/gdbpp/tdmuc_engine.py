import gdb
from tdmuc_base import *
from tdmuc_idlib import *


class idRenderModelStaticPrinter:
    wildcard = 'idRenderModelStatic'
    allow_derived = True    # e.g. idRenderModelMD5 (and others)
    def __init__(self, value):
        self.value = value
    def to_string(self):
        return 'RModel: ' + display_string(self.value['name'])
    def children(self):
        return raw_children_inline(self.value)


class RenderEntityPrinter:
    wildcard = 'renderEntity_s'

    def __init__(self, value):
        self.value = value
        self.pEntity = None

        enum = int(self.value['entityNum'])
        if enum != 0:
            gameLocal = gdb.lookup_global_symbol('gameLocal', gdb.SYMBOL_VAR_DOMAIN).value()
            self.pEntity = idListPrinter(gameLocal['entities']).get(enum)
        else:
            self.pModel = self.value['hModel']

    def to_string(self):
        if self.pEntity:
            entity_str = display_string(self.pEntity.dereference())
            return entity_str.replace('Entity', 'REntity')
        elif int(self.pModel) != 0:
            return display_string(self.pModel.dereference())
        else:
            return 'REntParms: ?'
    
    def children(self):
        res = []
        if self.pEntity:
            res.append(('[entity]', self.pEntity.dereference()))
        res += raw_children_inline(self.value)
        return res


class idRenderEntityLocalPrinter:
    wildcard = 'idRenderEntityLocal'
    def __init__(self, value):
        self.value = value
    def to_string(self):
        return display_string(self.value['parms'])
    def children(self):
        return raw_children_inline(self.value)


class RenderLightPrinter:
    wildcard = 'renderLight_s'

    def __init__(self, value):
        self.value = value
        self.pEntity = None

        enum = int(self.value['entityNum'])
        if enum != 0:
            gameLocal = gdb.lookup_global_symbol('gameLocal', gdb.SYMBOL_VAR_DOMAIN).value()
            self.pEntity = idListPrinter(gameLocal['entities']).get(enum)

    def to_string(self):
        if self.pEntity:
            entity_str = display_string(self.pEntity.dereference())
            return entity_str.replace('Entity', 'REntity')
        else:
            return 'RLgtParms: ?'
    
    def children(self):
        res = []
        if self.pEntity:
            res.append(('[entity]', self.pEntity.dereference()))
        res += raw_children_inline(self.value)
        return res


class idRenderLightLocalPrinter:
    wildcard = 'idRenderLightLocal'
    def __init__(self, value):
        self.value = value
    def to_string(self):
        return display_string(self.value['parms'])
    def children(self):
        return raw_children_inline(self.value)


def build_pretty_printer():
    return TdmPrettyPrinterCollection.create_with_printers('engine', [
        idRenderModelStaticPrinter,
        RenderEntityPrinter, idRenderEntityLocalPrinter,
        RenderLightPrinter, idRenderLightLocalPrinter,
    ])
