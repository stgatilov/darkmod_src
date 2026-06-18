import gdb
from tdmuc_base import *
from tdmuc_idlib import *

def getRenderWorld():
    renderWorld = gdb.lookup_global_symbol('gameRenderWorld', gdb.SYMBOL_VAR_DOMAIN).value()
    localType = gdb.lookup_type('idRenderWorldLocal*')
    return renderWorld.cast(localType).dereference()

def getGameLocal():
    return gdb.lookup_global_symbol('gameLocal', gdb.SYMBOL_VAR_DOMAIN).value()


class RenderEntityPrinter:
    wildcard = 'renderEntity_s'

    def __init__(self, value):
        self.value = value
        self.pEntity = None

        enum = int(self.value['entityNum'])
        if enum != 0:
            self.pEntity = idListPrinter(getGameLocal()['entities']).get(enum)
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


class RenderLightPrinter:
    wildcard = 'renderLight_s'

    def __init__(self, value):
        self.value = value
        self.pEntity = None

        enum = int(self.value['entityNum'])
        if enum != 0:
            self.pEntity = idListPrinter(getGameLocal()['entities']).get(enum)

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


engine_pplist = [
    make_simple_printer('idScreenRect', '[{$x1}..{$x2}] x [{$y1}..{$y2}] x [{$zmin}..{$zmax}]'),
    make_simple_printer('idCVar',
        'CVar {@name_} is: {@value_}  (default:{$resetString})',
        class_attribs = {'allow_derived': True}, structure = lambda v: {
            '^': raw_children_inline(v),
            '@name_': v['name'].string(),
            '@value_': v['value'].string(),
        }
    ),
    make_simple_printer('idImage',
        'image {$imgName} gl:{$texnum} {$uploadWidth}x{$uploadHeight} {$depth} {$type} {$filter}',
        class_attribs = {'allow_derived': True}
    ),
    make_simple_printer('idRenderModelStatic', 'RModel: {$name}', class_attribs = {'allow_derived': True}),
]

engine_pplist += [
    make_simple_printer('idRenderEntityLocal', '{$parms}', lambda v: {
        '^': raw_children_inline(v),
        '[World Area]': idListPrinter(v['world'].dereference()['portalAreas']).get(int(v['index'])),
        '[All Interactions]': make_synthetic(lambda: linked_list_children_list(
            v['firstInteraction'],
            lambda n: n.dereference()['entityNext']
        )),
        '[All refAreas]': make_synthetic(lambda: linked_list_children_list(
            v['entityRefs'],
            lambda n: n.dereference()['next'],
            lambda n: idListPrinter(v['world'].dereference()['portalAreas']).get(int(n.dereference()['areaIdx']))
        )),
    }),
    make_simple_printer('idRenderLightLocal', '{$parms}', lambda v: {
        '^': raw_children_inline(v),
        '[All Interactions]': make_synthetic(lambda: linked_list_children_list(
            v['firstInteraction'],
            lambda n: n.dereference()['lightNext']
        )),
        '[All refAreas]': make_synthetic(lambda: linked_list_children_list(
            v['references'],
            lambda n: n.dereference()['next'],
            lambda n: idListPrinter(v['world'].dereference()['portalAreas']).get(int(n.dereference()['areaIdx']))
        )),
    }),
]

engine_pplist += [
    make_simple_printer('idDeclLocal', 'Decl {$name} from {@filename}', lambda v: {
        '^': raw_children_inline(v),
        '@filename': v['sourceFile'].dereference()['fileName'],
    }),
    make_simple_printer('idDeclFile', 'DeclFile {$fileName}  ({$numLines} lines)', lambda v: {
        '^': raw_children_inline(v),
        '[All Decls]': make_synthetic(lambda: linked_list_children_list(
            v['decls'],
            lambda n: n['nextInFile']
        )),
    }),

    make_simple_printer('idDecl', '{@base_}', class_attribs = {'allow_derived': True}, structure = lambda v: {
        '^': raw_children_inline(v),
        '@base_': v['base'].dereference(),
    }),
    make_simple_printer('idMaterial', '{@base_}', structure = lambda v: {
        '^': raw_children_inline(v),
        '@base_': v['base'].dereference(),
        '[stages]': array_children_list(v['stages'], v['numStages']),
        '[interactionGroups]': array_children_list(v['interactionGroupStarts'], int(v['numInteractionGroups']) + 1),
    }),
]

def engine_baseCommand_children(v):
    cid = int(v['commandId'])
    if cid == int(gdb.lookup_global_symbol('RC_DRAW_VIEW').value()):
        type = gdb.lookup_type('drawSurfsCommand_t*')
    elif cid == int(gdb.lookup_global_symbol('RC_COPY_RENDER').value()):
        type = gdb.lookup_type('copyRenderCommand_t*')
    else:
        return raw_children_inline(v)
    return raw_children_inline(v.address.cast(type).dereference())

engine_pplist += [
    make_simple_printer('emptyCommand_t', '[Expand command list]', class_attribs = {'allow_derived': False},
        structure = lambda v: linked_list_children_list(
            v.address,
            lambda n: n['next'].cast(gdb.lookup_type('emptyCommand_t*')),
            lambda n: n.cast(gdb.lookup_type('baseCommand_t*'))
        )
    ),
    make_simple_printer('baseCommand_t', '{$commandId}', class_attribs = {'allow_derived': False},
        structure = lambda v: engine_baseCommand_children(v)
    ),
    make_simple_printer('frameData_t', 'frameData[{$frameMemoryUsed} / {$frameMemoryAllocated}]', lambda v: {
        '^':  raw_children_inline(v),
        '[DeferredFreeSurfs]': make_synthetic(lambda: linked_list_children_list(
            v['firstDeferredFreeTriSurf'],
            lambda n: n.dereference()['nextDeferredFree']
        ))
    }),
]

engine_pplist += [
    RenderEntityPrinter, RenderLightPrinter,
]

def build_pretty_printer():
    return TdmPrettyPrinterCollection.create_with_printers('engine', engine_pplist)
