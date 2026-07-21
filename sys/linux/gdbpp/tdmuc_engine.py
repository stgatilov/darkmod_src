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
            return entity_str.replace('Entity', 'RLight')
        else:
            return 'RLgtParms: ?'
    
    def children(self):
        res = []
        if self.pEntity:
            res.append(('[entity]', self.pEntity.dereference()))
        res += raw_children_inline(self.value)
        return res

engine_pplist = [
    RenderEntityPrinter, RenderLightPrinter,
]

def cut_rentity_prefix(text):
    if not (text.startswith('REntity') or text.startswith('RLight')):
        return text
    pos = text.find('):') + 3
    return text[pos : ]


engine_pplist += [
    make_simple_printer('idScreenRect', '[{$x1}..{$x2}] x [{$y1}..{$y2}] x [{$zmin}..{$zmax}]'),
    make_simple_printer('idCVar',
        'CVar {@name_} is: {@value_}  (default: {$resetString})',
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
    make_simple_printer('srfTriangles_s', 'srfTriangles V{$numVerts} I{$numIndexes}', lambda v: {
        '^': raw_children_inline(v),
        '[All Verts]': make_synthetic(lambda: array_children_list(v['verts'], v['numVerts'])),
        '[All Indexes]': make_synthetic(lambda: array_children_list(v['indexes'], v['numIndexes'])),
        '[All SilEdges]': make_synthetic(lambda: array_children_list(v['silEdges'], v['numSilEdges'])),
        '[All Planes]': make_synthetic(lambda: array_children_list(v['facePlanes'], int(v['numIndexes']) / 3)) if int(v['facePlanesCalculated']) != 0 else 'absent',
    }),
]

engine_pplist += [
    make_simple_printer('idRenderEntityLocal', '{$parms}', lambda v: {
        '^': raw_children_inline(v),
        '[World Area]': idListPrinter(v['world'].dereference()['portalAreas']).get(int(v['index'])),
        '[All Interactions]': make_synthetic(lambda: linked_list_children_list(
            v['firstInteraction'],
            lambda n: n.dereference()['entityNext']
        )),
        '[All RefAreas]': make_synthetic(lambda: linked_list_children_list(
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
        '[All RefAreas]': make_synthetic(lambda: linked_list_children_list(
            v['references'],
            lambda n: n.dereference()['next'],
            lambda n: idListPrinter(v['world'].dereference()['portalAreas']).get(int(n.dereference()['areaIdx']))
        )),
    }),
    make_simple_printer('drawSurf_s', 'drSurf: {@renderEntity}', lambda v: {
        '^': raw_children_inline(v),
        '@renderEntity': v['space'].dereference()['entityDef'].dereference(),
    }),
    make_simple_printer('viewEntity_s', 'VEntity: {@name_}', lambda v: {
        '^': raw_children_inline(v),
        '@name_': cut_rentity_prefix(display_string(v['entityDef'].dereference())),
    }),
    make_simple_printer('viewLight_s', 'VLight: {@name_}', lambda v: {
        '^': raw_children_inline(v),
        '@name_': cut_rentity_prefix(display_string(v['lightDef'].dereference())),
        '[All Global Interactions]': make_synthetic(lambda: linked_list_children_list(
            v['globalInteractions'],
            lambda n: n.dereference()['nextOnLight']
        )),
        '[All Local Interactions]': make_synthetic(lambda: linked_list_children_list(
            v['localInteractions'],
            lambda n: n.dereference()['nextOnLight']
        )),
        '[All Translucent Interactions]': make_synthetic(lambda: linked_list_children_list(
            v['translucentInteractions'],
            lambda n: n.dereference()['nextOnLight']
        )),
        '[All Global Shadows]': make_synthetic(lambda: linked_list_children_list(
            v['globalShadows'],
            lambda n: n.dereference()['nextOnLight']
        )),
        '[All Local Shadows]': make_synthetic(lambda: linked_list_children_list(
            v['localShadows'],
            lambda n: n.dereference()['nextOnLight']
        )),
    }),
    make_simple_printer('viewDef_s', '{@viewType_}', lambda v: {
        '^': raw_children_inline(v),
        '@viewType_': v['renderView']['viewID'],
        '[All ViewLights]': make_synthetic(lambda: linked_list_children_list(
            v['viewLights'],
            lambda n: n.dereference()['next']
        )),
        '[All ViewEntities]': make_synthetic(lambda: linked_list_children_list(
            v['viewEntitys'],
            lambda n: n.dereference()['next']
        )),
        '[All DrawSurfs]': make_synthetic(lambda: array_children_list(v['drawSurfs'], v['numDrawSurfs'])),
        '[All Parent Views]': make_synthetic(lambda: linked_list_children_list(
            v['superView'],
            lambda n: n.dereference()['superView']
        )),
    }),
    make_simple_printer('idInteraction', 'interaction({$numSurfaces}): {@lightname} & {@entityname}', lambda v: {
        '^': raw_children_inline(v),
        '@lightname': cut_rentity_prefix(display_string(v['lightDef'].dereference())),
        '@entityname': cut_rentity_prefix(display_string(v['entityDef'].dereference())),
        '[All Surfaces]': make_synthetic(lambda: array_children_list(v['surfaces'], v['numSurfaces'])),
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
            lambda n: n['next'].cast(gdb.lookup_type('emptyCommand_t').pointer()),
            lambda n: n.cast(gdb.lookup_type('baseCommand_t').pointer())
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
    make_simple_printer('portalArea_s', 'Area{$areaNum}', lambda v: {
        '^':  raw_children_inline(v),
        '[All EntityRefs]': make_synthetic(lambda: [
            ('[%d]' % i, idListPrinter(getRenderWorld()['entityDefs']).get(idListPrinter(v['entityRefs']).get(i)))
            for i in range(int(v['entityRefs']['num']))
        ]),
        '[All LightRefs]': make_synthetic(lambda: [
            ('[%d]' % i, idListPrinter(getRenderWorld()['lightDefs']).get(idListPrinter(v['lightRefs']).get(i)))
            for i in range(int(v['lightRefs']['num']))
        ]),
    }),
    make_simple_printer('doublePortal_s', 'dblPortal: {@area0_} | {@area1_}  block={$blockingBits}', lambda v: {
        '^':  raw_children_inline(v),
        '@area0_': v['portals'][0]['intoArea'],
        '@area1_': v['portals'][1]['intoArea'],
    }),
]

engine_pplist += [
    make_simple_printer('idWindow', 'guiwin: {$name}'),
]

engine_pplist += [
    make_simple_printer('idSoundChannel', 'sndChan: {$soundShader}'),
    make_simple_printer('idSoundEmitterLocal', 'sndEmit: {@summary_}', lambda v: {
        '^':  raw_children_inline(v),
        '@summary_': '[finished]' if str(v['removeStatus']) == 'REMOVE_STATUS_SAMPLEFINISHED' else v['channels'][0]['soundShader'].dereference(),
    }),
]

def get_pretty_printers():
    return engine_pplist
