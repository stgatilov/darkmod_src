/*****************************************************************************
                    The Dark Mod GPL Source Code

 This file is part of the The Dark Mod Source Code, originally based
 on the Doom 3 GPL Source Code as published in 2011.

 The Dark Mod Source Code is free software: you can redistribute it
 and/or modify it under the terms of the GNU General Public License as
 published by the Free Software Foundation, either version 3 of the License,
 or (at your option) any later version. For details, see LICENSE.TXT.

 Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
#pragma hdrstop

#include "tr_local.h"
#include "FrameBuffer.h"

// Vista OpenGL wrapper check
#ifdef _WIN32
#include "../sys/win32/win_local.h"
#endif

// functions that are not called every frame

glconfig_t	glConfig;

idCVar r_glDriver( "r_glDriver", "", CVAR_RENDERER, "\"opengl32\", etc." );
idCVar r_useLightPortalFlow( "r_useLightPortalFlow", "1", CVAR_RENDERER | CVAR_BOOL, "use a more precise area reference determination" );
idCVar r_multiSamples( "r_multiSamples", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "number of antialiasing samples" );
idCVar r_mode( "r_mode", "5", CVAR_ARCHIVE | CVAR_RENDERER | CVAR_INTEGER, "video mode number" ); // grayman #4022
idCVar r_displayRefresh( "r_displayRefresh", "0", CVAR_RENDERER | CVAR_INTEGER | CVAR_NOCHEAT, "optional display refresh rate option for vid mode", 0.0f, 200.0f );
idCVar r_fullscreen( "r_fullscreen", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "0 = windowed, 1 = full screen" );
idCVar r_customWidth( "r_customWidth", "720", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "custom screen width. set r_mode to -1 to activate" );
idCVar r_customHeight( "r_customHeight", "486", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "custom screen height. set r_mode to -1 to activate" );
idCVar r_singleTriangle( "r_singleTriangle", "0", CVAR_RENDERER | CVAR_BOOL, "only draw a single triangle per primitive" );
idCVar r_checkBounds( "r_checkBounds", "0", CVAR_RENDERER | CVAR_BOOL, "compare all surface bounds with precalculated ones" );

idCVar r_useConstantMaterials( "r_useConstantMaterials", "1", CVAR_RENDERER | CVAR_BOOL, "use pre-calculated material registers if possible" );
idCVar r_useSilRemap( "r_useSilRemap", "1", CVAR_RENDERER | CVAR_BOOL, "consider verts with the same XYZ, but different ST the same for shadows" );
idCVar r_useNodeCommonChildren( "r_useNodeCommonChildren", "1", CVAR_RENDERER | CVAR_BOOL, "stop pushing reference bounds early when possible" );
idCVar r_useShadowProjectedCull( "r_useShadowProjectedCull", "1", CVAR_RENDERER | CVAR_BOOL, "discard triangles outside light volume before shadowing" );
idCVar r_useShadowSurfaceScissor( "r_useShadowSurfaceScissor", "1", CVAR_RENDERER | CVAR_BOOL, "scissor shadows by the scissor rect of the interaction surfaces" );
idCVar r_useTurboShadow( "r_useTurboShadow", "1", CVAR_RENDERER | CVAR_BOOL, "use the infinite projection with W technique for dynamic shadows" );
idCVar r_useTwoSidedStencil( "r_useTwoSidedStencil", "1", CVAR_RENDERER | CVAR_BOOL, "do stencil shadows in one pass with different ops on each side" );
idCVar r_useDeferredTangents( "r_useDeferredTangents", "1", CVAR_RENDERER | CVAR_BOOL, "defer tangents calculations after deform" );
idCVar r_useCachedDynamicModels( "r_useCachedDynamicModels", "1", CVAR_RENDERER | CVAR_BOOL, "cache snapshots of dynamic models" );

//duzenko & stgatilov:
idCVar r_softShadowsQuality( "r_softShadowsQuality", "0", CVAR_RENDERER | CVAR_INTEGER | CVAR_ARCHIVE, "Number of samples in soft shadows blur. 0 = hard shadows, 6 = low-quality, 24 = good, 96 = perfect" );
idCVar r_softShadowsRadius( "r_softShadowsRadius", "1.0", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE, "Radius of light source for soft shadows. Decreasing it makes soft shadows less blurry." );

/* ~ss
idCVar r_softShadows( "r_softShadows", "0", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE, "Soft shadows. 0 = hard shadows, >0 = light radius" );
idCVar r_softShadDebug( "ssdebug", "0", CVAR_RENDERER | CVAR_INTEGER, "Soft shadows debug. 1 = Show penumbra lines, 2 = show penumbra sampling regions, "
																	  "4 = show light attenuation. Can be used together, e.g. 5 = 4 + 1 = show lines and attenuation.");
idCVar r_softShadMaxSize( "ssmax", "20", CVAR_RENDERER | CVAR_FLOAT, "Soft shadows max penumbra size in pixels. FIXME: Probably wants changing to be a % of screen size.");
*/

idCVar r_useStateCaching( "r_useStateCaching", "1", CVAR_RENDERER | CVAR_BOOL, "avoid redundant state changes in GL_*() calls" );

idCVar r_znear( "r_znear", "3", CVAR_RENDERER | CVAR_FLOAT, "near Z clip plane distance", 0.001f, 200.0f );

idCVar r_ignoreGLErrors( "r_ignoreGLErrors", "1", CVAR_RENDERER | CVAR_BOOL, "ignore GL errors" );
idCVar r_finish( "r_finish", "0", CVAR_RENDERER | CVAR_BOOL, "force a call to glFinish() every frame" );
idCVar r_swapInterval( "r_swapInterval", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "changes wglSwapIntarval" );
idCVar r_swapIntervalTemp( "r_swapIntervalTemp", "1", CVAR_RENDERER | CVAR_INTEGER, "forces VSync in GUI" );

idCVar r_gamma( "r_gamma", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT, "changes gamma tables", 0.5f, 10.0f );
idCVar r_brightness( "r_brightness", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT, "changes gamma tables", 0.5f, 2.0f );

idCVar r_jitter( "r_jitter", "0", CVAR_RENDERER | CVAR_BOOL, "randomly subpixel jitter the projection matrix" );

idCVar r_skipSuppress( "r_skipSuppress", "0", CVAR_RENDERER | CVAR_BOOL, "ignore the per-view suppressions" );
idCVar r_skipPostProcess( "r_skipPostProcess", "0", CVAR_RENDERER | CVAR_BOOL, "skip all post-process renderings" );
idCVar r_skipInteractions( "r_skipInteractions", "0", CVAR_RENDERER | CVAR_BOOL, "skip all light/surface interaction drawing" );
idCVar r_skipDynamicTextures( "r_skipDynamicTextures", "0", CVAR_RENDERER | CVAR_BOOL, "don't dynamically create textures" );
idCVar r_skipCopyTexture( "r_skipCopyTexture", "0", CVAR_RENDERER | CVAR_BOOL, "do all rendering, but don't actually copyTexSubImage2D" );
idCVar r_skipBackEnd( "r_skipBackEnd", "0", CVAR_RENDERER | CVAR_BOOL, "don't draw anything" );
idCVar r_skipRender( "r_skipRender", "0", CVAR_RENDERER | CVAR_BOOL, "skip 3D rendering, but pass 2D" );
idCVar r_skipRenderContext( "r_skipRenderContext", "0", CVAR_RENDERER | CVAR_BOOL, "NULL the rendering context during backend 3D rendering" );
idCVar r_skipTranslucent( "r_skipTranslucent", "0", CVAR_RENDERER | CVAR_BOOL, "skip the translucent interaction rendering" );
idCVar r_skipAmbient( "r_skipAmbient", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = bypasses all non-interaction drawing, 2 = skips ambient light interactions" );
idCVar r_skipNewAmbient( "r_skipNewAmbient", "0", CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, "bypasses all vertex/fragment program ambient drawing" );
idCVar r_skipBlendLights( "r_skipBlendLights", "0", CVAR_RENDERER | CVAR_BOOL, "skip all blend lights" );
idCVar r_skipFogLights( "r_skipFogLights", "0", CVAR_RENDERER | CVAR_BOOL, "skip all fog lights" );
idCVar r_skipDeforms( "r_skipDeforms", "0", CVAR_RENDERER | CVAR_BOOL, "leave all deform materials in their original state" );
idCVar r_skipFrontEnd( "r_skipFrontEnd", "0", CVAR_RENDERER | CVAR_BOOL, "bypasses all front end work, but 2D gui rendering still draws" );
idCVar r_skipUpdates( "r_skipUpdates", "0", CVAR_RENDERER | CVAR_BOOL, "1 = don't accept any entity or light updates, making everything static" );
idCVar r_skipOverlays( "r_skipOverlays", "0", CVAR_RENDERER | CVAR_BOOL, "skip overlay surfaces" );
idCVar r_skipSpecular( "r_skipSpecular", "0", CVAR_RENDERER | CVAR_BOOL | CVAR_CHEAT | CVAR_ARCHIVE, "use black for specular1" );
idCVar r_skipBump( "r_skipBump", "0", CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, "uses a flat surface instead of the bump map" );
idCVar r_skipDiffuse( "r_skipDiffuse", "0", CVAR_RENDERER | CVAR_BOOL, "use black for diffuse" );
idCVar r_skipROQ( "r_skipROQ", "0", CVAR_RENDERER | CVAR_BOOL, "skip ROQ decoding" );
idCVar r_skipDepthCapture( "r_skipDepthCapture", "0", CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, "skip depth capture" ); // #3877 #4418
idCVar r_useSoftParticles( "r_useSoftParticles", "1", CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, "soften particle transitions when player walks through them or they cross solid geometry" ); // #3878 #4418

idCVar r_ignore( "r_ignore", "0", CVAR_RENDERER, "used for random debugging without defining new vars" );
idCVar r_ignore2( "r_ignore2", "0", CVAR_RENDERER, "used for random debugging without defining new vars" );
idCVar r_usePreciseTriangleInteractions( "r_usePreciseTriangleInteractions", "0", CVAR_RENDERER | CVAR_BOOL, "1 = do winding clipping to determine if each ambiguous tri should be lit" );
idCVar r_useCulling( "r_useCulling", "2", CVAR_RENDERER | CVAR_INTEGER, "0 = none, 1 = sphere, 2 = sphere + box", 0, 2, idCmdSystem::ArgCompletion_Integer<0, 2> );
idCVar r_useLightCulling( "r_useLightCulling", "3", CVAR_RENDERER | CVAR_INTEGER, "0 = none, 1 = box, 2 = exact clip of polyhedron faces, 3 = also areas", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3> );
idCVar r_useLightScissors( "r_useLightScissors", "1", CVAR_RENDERER | CVAR_BOOL, "1 = use custom scissor rectangle for each light" );
//anon begin
idCVar r_useLightPortalCulling( "r_useLightPortalCulling", "1", CVAR_RENDERER | CVAR_INTEGER, "0 = none, 1 = cull frustum corners to plane, 2 = exact clip the frustum faces", 0, 2, idCmdSystem::ArgCompletion_Integer<0, 2> );
idCVar r_useEntityPortalCulling( "r_useEntityPortalCulling", "1", CVAR_RENDERER | CVAR_INTEGER, "0 = none, 1 = cull frustum corners to plane, 2 = exact clip the frustum faces", 0, 2, idCmdSystem::ArgCompletion_Integer<0, 2> );
//anon end
idCVar r_useClippedLightScissors( "r_useClippedLightScissors", "1", CVAR_RENDERER | CVAR_INTEGER, "0 = full screen when near clipped, 1 = exact when near clipped, 2 = exact always", 0, 2, idCmdSystem::ArgCompletion_Integer<0, 2> );
idCVar r_useEntityCulling( "r_useEntityCulling", "1", CVAR_RENDERER | CVAR_BOOL, "0 = none, 1 = box" );
idCVar r_useEntityScissors( "r_useEntityScissors", "0", CVAR_RENDERER | CVAR_BOOL, "1 = use custom scissor rectangle for each entity" );
idCVar r_useInteractionCulling( "r_useInteractionCulling", "1", CVAR_RENDERER | CVAR_BOOL, "1 = cull interactions" );
idCVar r_useInteractionScissors( "r_useInteractionScissors", "2", CVAR_RENDERER | CVAR_INTEGER, "1 = use a custom scissor rectangle for each shadow interaction, 2 = also crop using portal scissors", -2, 2, idCmdSystem::ArgCompletion_Integer < -2, 2 > );
idCVar r_useShadowCulling( "r_useShadowCulling", "1", CVAR_RENDERER | CVAR_BOOL, "try to cull shadows from partially visible lights" );
idCVar r_useFrustumFarDistance( "r_useFrustumFarDistance", "0", CVAR_RENDERER | CVAR_FLOAT, "if != 0 force the view frustum far distance to this distance" );
idCVar r_logFile( "r_logFile", "0", CVAR_RENDERER | CVAR_INTEGER, "number of frames to emit GL logs" );
idCVar r_clear( "r_clear", "2", CVAR_RENDERER | CVAR_ARCHIVE, "force screen clear every frame, 1 = purple, 2 = black, 'r g b' = custom" );
idCVar r_offsetFactor( "r_offsetfactor", "-2", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE, "polygon offset parameter" ); // #4079
idCVar r_offsetUnits( "r_offsetunits", "-0.1", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE, "polygon offset parameter" ); // #4079
idCVar r_shadowPolygonOffset( "r_shadowPolygonOffset", "-1", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE, "bias value added to depth test for stencil shadow drawing" );
idCVar r_shadowPolygonFactor( "r_shadowPolygonFactor", "0", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE, "scale value for stencil shadow drawing" );
idCVar r_frontBuffer( "r_frontBuffer", "0", CVAR_RENDERER | CVAR_BOOL, "draw to front buffer for debugging" );
idCVar r_skipSubviews( "r_skipSubviews", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = don't render any gui elements on surfaces" );
idCVar r_skipGuiShaders( "r_skipGuiShaders", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = skip all gui elements on surfaces, 2 = skip drawing but still handle events, 3 = draw but skip events", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3> );
idCVar r_skipParticles( "r_skipParticles", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = skip all particle systems", 0, 1, idCmdSystem::ArgCompletion_Integer<0, 1> );
idCVar r_subviewOnly( "r_subviewOnly", "0", CVAR_RENDERER | CVAR_BOOL, "1 = don't render main view, allowing subviews to be debugged" );
idCVar r_shadows( "r_shadows", "1", CVAR_RENDERER | CVAR_INTEGER  | CVAR_ARCHIVE, "1 = stencil shadows, 2 = shadow maps (requires r_useGLSL 1)" );
idCVar r_testARBProgram( "r_testARBProgram", "1", CVAR_RENDERER | CVAR_ARCHIVE, "experiment with vertex/fragment programs" );
idCVar r_testGamma( "r_testGamma", "0", CVAR_RENDERER | CVAR_FLOAT, "if > 0 draw a grid pattern to test gamma levels", 0, 195 );
idCVar r_testGammaBias( "r_testGammaBias", "0", CVAR_RENDERER | CVAR_FLOAT, "if > 0 draw a grid pattern to test gamma levels" );
idCVar r_testStepGamma( "r_testStepGamma", "0", CVAR_RENDERER | CVAR_FLOAT, "if > 0 draw a grid pattern to test gamma levels" );
idCVar r_lightScale( "r_lightScale", "2", CVAR_RENDERER | CVAR_FLOAT, "all light intensities are multiplied by this" );
idCVar r_lightSourceRadius( "r_lightSourceRadius", "0", CVAR_RENDERER | CVAR_FLOAT, "for soft-shadow sampling" );
idCVar r_flareSize( "r_flareSize", "1", CVAR_RENDERER | CVAR_FLOAT, "scale the flare deforms from the material def" );

idCVar r_useExternalShadows( "r_useExternalShadows", "1", CVAR_RENDERER | CVAR_INTEGER, "1 = skip drawing caps when outside the light volume", 0, 1, idCmdSystem::ArgCompletion_Integer<0, 1> );
idCVar r_useOptimizedShadows( "r_useOptimizedShadows", "1", CVAR_RENDERER | CVAR_BOOL, "use the dmap generated static shadow volumes" );
idCVar r_useScissor( "r_useScissor", "1", CVAR_RENDERER | CVAR_BOOL, "scissor clip as portals and lights are processed" );
idCVar r_useDepthBoundsTest( "r_useDepthBoundsTest", "1", CVAR_RENDERER | CVAR_BOOL, "use depth bounds test to reduce shadow fill" );

idCVar r_screenFraction( "r_screenFraction", "100", CVAR_RENDERER | CVAR_INTEGER, "for testing fill rate, the resolution of the entire screen can be changed" );
idCVar r_demonstrateBug( "r_demonstrateBug", "0", CVAR_RENDERER | CVAR_BOOL, "used during development to show IHV's their problems" );
idCVar r_usePortals( "r_usePortals", "1", CVAR_RENDERER | CVAR_BOOL, " 1 = use portals to perform area culling, otherwise draw everything" );
idCVar r_singleLight( "r_singleLight", "-1", CVAR_RENDERER | CVAR_INTEGER, "suppress all but one light" );
idCVar r_singleEntity( "r_singleEntity", "-1", CVAR_RENDERER | CVAR_INTEGER, "suppress all but one entity" );
idCVar r_singleSurface( "r_singleSurface", "-1", CVAR_RENDERER | CVAR_INTEGER, "suppress all but one surface on each entity" );
idCVar r_singleArea( "r_singleArea", "0", CVAR_RENDERER | CVAR_BOOL, "only draw the portal area the view is actually in" );
idCVar r_forceLoadImages( "r_forceLoadImages", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "draw all images to screen after registration" );
idCVar r_orderIndexes( "r_orderIndexes", "1", CVAR_RENDERER | CVAR_BOOL, "perform index reorganization to optimize vertex use" );
idCVar r_lightAllBackFaces( "r_lightAllBackFaces", "0", CVAR_RENDERER | CVAR_BOOL, "light all the back faces, even when they would be shadowed" );
idCVar r_skipModels( "r_skipModels", "0", CVAR_RENDERER | CVAR_INTEGER, "0 - draw all, 1 - static only, 2 - dynamic only" );

// visual debugging info
idCVar r_showPortals( "r_showPortals", "0", CVAR_RENDERER | CVAR_BOOL, "draw portal outlines in color: green = player sees through portal; yellow = not seen through but visleaf is open through another portal; red = portal and visleaf the other side are closed." );
idCVar r_showUnsmoothedTangents( "r_showUnsmoothedTangents", "0", CVAR_RENDERER | CVAR_BOOL, "if 1, put all nvidia register combiner programming in display lists" );
idCVar r_showSilhouette( "r_showSilhouette", "0", CVAR_RENDERER | CVAR_BOOL, "highlight edges that are casting shadow planes" );
idCVar r_showVertexColor( "r_showVertexColor", "0", CVAR_RENDERER | CVAR_BOOL, "draws all triangles with the solid vertex color" );
idCVar r_showUpdates( "r_showUpdates", "0", CVAR_RENDERER | CVAR_BOOL, "report entity and light updates and ref counts" );
idCVar r_showDemo( "r_showDemo", "0", CVAR_RENDERER | CVAR_BOOL, "report reads and writes to the demo file" );
idCVar r_showDynamic( "r_showDynamic", "0", CVAR_RENDERER | CVAR_BOOL, "report stats on dynamic surface generation" );
idCVar r_showDefs( "r_showDefs", "0", CVAR_RENDERER | CVAR_BOOL, "report the number of modeDefs and lightDefs in view" );
idCVar r_showTrace( "r_showTrace", "0", CVAR_RENDERER | CVAR_INTEGER, "show the intersection of an eye trace with the world", idCmdSystem::ArgCompletion_Integer<0, 2> );
idCVar r_showIntensity( "r_showIntensity", "0", CVAR_RENDERER | CVAR_BOOL, "draw the screen colors based on intensity, red = 0, green = 128, blue = 255" );
idCVar r_showImages( "r_showImages", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = show all images instead of rendering, 2 = show in proportional size", 0, 2, idCmdSystem::ArgCompletion_Integer<0, 2> );
idCVar com_smp( "com_smp", "0", CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, "enable SMP" );
idCVar r_showSmp( "r_showSmp", "0", CVAR_RENDERER | CVAR_BOOL, "show which end (front or back) is blocking" );
idCVar r_logSmpTimings( "r_logSmpTimings", "0", CVAR_RENDERER | CVAR_BOOL, "log timings for frontend and backend rendering" );
idCVar r_showLights( "r_showLights", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = just print volumes numbers, highlighting ones covering the view, 2 = also draw planes of each volume, 3 = also draw edges of each volume", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3> );
idCVar r_showShadows( "r_showShadows", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = visualize the stencil shadow volumes, 2 = draw filled in, 3 = lines with depth test", -1, 3, idCmdSystem::ArgCompletion_Integer < -1, 3 > );
idCVar r_showShadowCount( "r_showShadowCount", "0", CVAR_RENDERER | CVAR_INTEGER, "colors screen based on shadow volume depth complexity, >= 2 = print overdraw count based on stencil index values, 3 = only show turboshadows, 4 = only show static shadows", 0, 4, idCmdSystem::ArgCompletion_Integer<0, 4> );
idCVar r_showLightScissors( "r_showLightScissors", "0", CVAR_RENDERER | CVAR_BOOL, "show light scissor rectangles" );
idCVar r_showEntityScissors( "r_showEntityScissors", "0", CVAR_RENDERER | CVAR_BOOL, "show entity scissor rectangles" );
idCVar r_showInteractionFrustums( "r_showInteractionFrustums", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = show a frustum for each interaction, 2 = also draw lines to light origin, 3 = also draw entity bbox", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3> );
idCVar r_showInteractionScissors( "r_showInteractionScissors", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = show screen rectangle which contains the interaction frustum, 2 = also draw construction lines", 0, 2, idCmdSystem::ArgCompletion_Integer<0, 2> );
idCVar r_showLightCount( "r_showLightCount", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = colors surfaces based on light count, 2 = also count everything through walls, 3 = also print overdraw", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3> );
idCVar r_showViewEntitys( "r_showViewEntitys", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = displays the bounding boxes of all view models, 2 = print index numbers, 3 = index number and render model name" );
idCVar r_showTris( "r_showTris", "0", CVAR_RENDERER | CVAR_INTEGER, "enables wireframe rendering of the world, 1 = only draw visible ones, 2 = draw all front facing, 3 = draw all", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3> );
idCVar r_showSurfaceInfo( "r_showSurfaceInfo", "0", CVAR_RENDERER | CVAR_BOOL, "show surface material name under crosshair" );
idCVar r_showNormals( "r_showNormals", "0", CVAR_RENDERER | CVAR_FLOAT, "draws wireframe normals" );
idCVar r_showMemory( "r_showMemory", "0", CVAR_RENDERER | CVAR_BOOL, "print frame memory utilization" );
idCVar r_showCull( "r_showCull", "0", CVAR_RENDERER | CVAR_BOOL, "report sphere and box culling stats" );
idCVar r_showInteractions( "r_showInteractions", "0", CVAR_RENDERER | CVAR_BOOL, "report interaction generation activity" );
idCVar r_showDepth( "r_showDepth", "0", CVAR_RENDERER | CVAR_BOOL, "display the contents of the depth buffer and the depth range" );
idCVar r_showSurfaces( "r_showSurfaces", "0", CVAR_RENDERER | CVAR_BOOL, "report surface/light/shadow counts" );
idCVar r_showPrimitives( "r_showPrimitives", "0", CVAR_RENDERER | CVAR_INTEGER, "report drawsurf/index/vertex counts" );
idCVar r_showEdges( "r_showEdges", "0", CVAR_RENDERER | CVAR_BOOL, "draw the sil edges" );
idCVar r_showTexturePolarity( "r_showTexturePolarity", "0", CVAR_RENDERER | CVAR_BOOL, "shade triangles by texture area polarity" );
idCVar r_showTangentSpace( "r_showTangentSpace", "0", CVAR_RENDERER | CVAR_INTEGER, "shade triangles by tangent space, 1 = use 1st tangent vector, 2 = use 2nd tangent vector, 3 = use normal vector", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3> );
idCVar r_showDominantTri( "r_showDominantTri", "0", CVAR_RENDERER | CVAR_BOOL, "draw lines from vertexes to center of dominant triangles" );
idCVar r_showAlloc( "r_showAlloc", "0", CVAR_RENDERER | CVAR_BOOL, "report alloc/free counts" );
idCVar r_showTextureVectors( "r_showTextureVectors", "0", CVAR_RENDERER | CVAR_FLOAT, " if > 0 draw each triangles texture (tangent) vectors" );
idCVar r_showOverDraw( "r_showOverDraw", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = geometry overdraw, 2 = light interaction overdraw, 3 = geometry and light interaction overdraw", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3> );

idCVar r_lockSurfaces( "r_lockSurfaces", "0", CVAR_RENDERER | CVAR_BOOL, "allow moving the view point without changing the composition of the scene, including culling" );
idCVar r_useEntityCallbacks( "r_useEntityCallbacks", "1", CVAR_RENDERER | CVAR_BOOL, "if 0, issue the callback immediately at update time, rather than defering" );

idCVar r_showSkel( "r_showSkel", "0", CVAR_RENDERER | CVAR_INTEGER, "draw the skeleton when model animates, 1 = draw model with skeleton, 2 = draw skeleton only", 0, 2, idCmdSystem::ArgCompletion_Integer<0, 2> );
idCVar r_jointNameScale( "r_jointNameScale", "0.02", CVAR_RENDERER | CVAR_FLOAT, "size of joint names when r_showskel is set to 1" );
idCVar r_jointNameOffset( "r_jointNameOffset", "0.5", CVAR_RENDERER | CVAR_FLOAT, "offset of joint names when r_showskel is set to 1" );

idCVar r_debugLineDepthTest( "r_debugLineDepthTest", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "perform depth test on debug lines" );
idCVar r_debugLineWidth( "r_debugLineWidth", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "width of debug lines" );
idCVar r_debugArrowStep( "r_debugArrowStep", "120", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "step size of arrow cone line rotation in degrees", 0, 120 );
idCVar r_debugPolygonFilled( "r_debugPolygonFilled", "1", CVAR_RENDERER | CVAR_BOOL, "draw a filled polygon" );

idCVar r_materialOverride( "r_materialOverride", "", CVAR_RENDERER, "overrides all materials", idCmdSystem::ArgCompletion_Decl<DECL_MATERIAL> );

idCVar r_debugRenderToTexture( "r_debugRenderToTexture", "0", CVAR_RENDERER | CVAR_INTEGER, "" );

// greebo: screenshot format CVAR, by default convert the generated TGA to JPG
idCVar r_screenshot_format(	"r_screenshot_format", "jpg",   CVAR_RENDERER | CVAR_ARCHIVE, "Image format used to store ingame screenshots: png/tga/jpg/bmp." );

// rebb: toggle for dedicated ambient light shader use, mainly for performance testing
idCVar r_dedicatedAmbient( "r_dedicatedAmbient", "1", CVAR_RENDERER | CVAR_BOOL, "enable dedicated ambientLight shader" );

// bloom related - J.C.Denton
idCVar r_postprocess_debugMode( "r_postprocess_debugMode", "0", CVAR_GAME | CVAR_INTEGER, " Shows all the textures generated for postprocessing effects. \n 1: Shows currentRender \n 2: Shows bloom Images \n 3: Shows Cooked Math Data." );
idCVar r_postprocess( "r_postprocess", "0", CVAR_GAME | CVAR_INTEGER | CVAR_ARCHIVE, " Activates bloom ( Requires DX9 compliant Hardware )" );
idCVar r_postprocess_brightPassThreshold( "r_postprocess_brightPassThreshold", "0.2", CVAR_GAME | CVAR_FLOAT, " Intensities of this value are subtracted from scene render to extract bloom image" );
idCVar r_postprocess_brightPassOffset( "r_postprocess_brightPassOffset", "2", CVAR_GAME | CVAR_FLOAT, " Bloom image receives smooth fade along a curve from bright to very bright areas based on this variable's value" );
idCVar r_postprocess_colorCurveBias( "r_postprocess_colorCurveBias", "0.8", CVAR_GAME | CVAR_FLOAT, " Applies Exponential Color Curve to final pass (range 0 to 1), 1 = color curve fully applied , 0= No color curve" );
idCVar r_postprocess_colorCorrection( "r_postprocess_colorCorrection", "5", CVAR_GAME | CVAR_FLOAT, " Applies an exponential color correction function to final scene " );
idCVar r_postprocess_colorCorrectBias( "r_postprocess_colorCorrectBias", "0.1", CVAR_GAME | CVAR_FLOAT, " Applies an exponential color correction function to final scene with this bias. \n E.g. value ranges between 0-1. A blend is performed between scene render and color corrected image based on this value " );
idCVar r_postprocess_desaturation( "r_postprocess_desaturation", "0.05", CVAR_GAME | CVAR_FLOAT, " Desaturates the scene " );
idCVar r_postprocess_sceneExposure( "r_postprocess_sceneExposure", "0.9", CVAR_GAME | CVAR_FLOAT, " Scene render is linearly scaled up. Try values lower or greater than 1.0" );
idCVar r_postprocess_sceneGamma( "r_postprocess_sceneGamma", "0.82", CVAR_GAME | CVAR_FLOAT, " Gamma Correction." );
idCVar r_postprocess_bloomIntensity( "r_postprocess_bloomIntensity", "0", CVAR_GAME | CVAR_FLOAT | CVAR_ARCHIVE, " Adjusts the Bloom intensity. 0.0 disables the bloom but other postprocessing effects remain unaffected." );
idCVar r_postprocess_bloomKernelSize( "r_postprocess_bloomKernelSize", "2", CVAR_GAME | CVAR_INTEGER | CVAR_ARCHIVE, " Sets Bloom's Kernel size. Smaller is faster, takes less memory. Also, smaller kernel means larger bloom spread. \n 1. Large (2x smaller than current resolution) \n 2. Small (4x smaller than current resolution) " );

// 2016-2018 additions by duzenko
idCVar r_useAnonreclaimer( "r_useBfgPortalCulling", "0", CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, "test anonreclaimer culling patch" );
idCVar r_ambient_testadd( "r_ambient_testadd", "0", CVAR_RENDERER | CVAR_FLOAT, "Added ambient brightness for testing purposes. ", 0, 1 );
idCVar r_useGLSL( "r_useGLSL", "1", CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, "Use GLSL shaders instead of ARB" );

// FBO
idCVar r_useFbo( "r_useFBO", "0", CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, "Use framebuffer objects" );
idCVar r_nVidiaOverride( "r_nVidiaOverride", "1", CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, "Force FBO if Soft Shadows are enabled with Nvidia hardware" );
idCVar r_fboDebug( "r_fboDebug", "0", CVAR_RENDERER | CVAR_INTEGER, "0-3 individual fbo attachments" );
idCVar r_fboColorBits( "r_fboColorBits", "32", CVAR_RENDERER | CVAR_INTEGER | CVAR_ARCHIVE, "15, 32" );
idCVar r_fboDepthBits( "r_fboDepthBits", "24", CVAR_RENDERER | CVAR_INTEGER | CVAR_ARCHIVE, "16, 24, 32" );
idCVar r_shadowMapSize( "r_shadowMapSize", "256", CVAR_RENDERER | CVAR_INTEGER | CVAR_ARCHIVE, "Shadow map texture resolution" );
idCVar r_fboResolution( "r_fboResolution", "1", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE, "internal rendering resolution factor" );
idCVar r_fboSeparateStencil( "r_fboSeparateStencil", "0", CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, "Use separate depth and stencil FBO attachments. Only supported on some Intel GPU's" );

// relocate stgatilov ROQ options
idCVar r_cinematic_legacyRoq( "r_cinematic_legacyRoq", "0", CVAR_RENDERER | CVAR_INTEGER | CVAR_ARCHIVE,
                              "Play cinematics with original Doom3 code or with FFmpeg libraries. "
                              "0 - always use FFmpeg libraries, 1 - use original Doom3 code for ROQ and FFmpeg for other videos, 2 - never use FFmpeg" );

void ( APIENTRY * qglActiveTexture )( GLenum texture );

// separate stencil
PFNGLSTENCILOPSEPARATEPROC				qglStencilOpSeparate;

// ARB_texture_compression
PFNGLCOMPRESSEDTEXIMAGE2DARBPROC		qglCompressedTexImage2DARB;
PFNGLGETCOMPRESSEDTEXIMAGEARBPROC		qglGetCompressedTexImageARB;

// ARB_vertex_buffer_object
PFNGLBINDBUFFERARBPROC					qglBindBufferARB;
PFNGLDELETEBUFFERSARBPROC				qglDeleteBuffersARB;
PFNGLGENBUFFERSARBPROC					qglGenBuffersARB;
PFNGLISBUFFERARBPROC					qglIsBufferARB;
PFNGLBUFFERDATAARBPROC					qglBufferDataARB;
PFNGLBUFFERSUBDATAARBPROC				qglBufferSubDataARB;
PFNGLGETBUFFERSUBDATAARBPROC			qglGetBufferSubDataARB;
PFNGLMAPBUFFERARBPROC					qglMapBufferARB;
PFNGLUNMAPBUFFERARBPROC					qglUnmapBufferARB;
PFNGLGETBUFFERPARAMETERIVARBPROC		qglGetBufferParameterivARB;
PFNGLGETBUFFERPOINTERVARBPROC			qglGetBufferPointervARB;
PFNGLMAPBUFFERRANGEPROC					qglMapBufferRange;
PFNGLUNMAPBUFFERPROC					qglUnmapBuffer;
PFNGLFLUSHMAPPEDBUFFERRANGEPROC			qglFlushMappedBufferRange;
PFNGLBUFFERSUBDATAPROC					qglBufferSubData;

// ARB_vertex_program / ARB_fragment_program
PFNGLVERTEXATTRIBPOINTERARBPROC			qglVertexAttribPointer;
PFNGLENABLEVERTEXATTRIBARRAYARBPROC		qglEnableVertexAttribArray;
PFNGLDISABLEVERTEXATTRIBARRAYARBPROC	qglDisableVertexAttribArray;
PFNGLPROGRAMSTRINGARBPROC				qglProgramStringARB;
PFNGLBINDPROGRAMARBPROC					qglBindProgramARB;
PFNGLGENPROGRAMSARBPROC					qglGenProgramsARB;
PFNGLPROGRAMENVPARAMETER4FVARBPROC		qglProgramEnvParameter4fvARB;
PFNGLPROGRAMLOCALPARAMETER4FVARBPROC	qglProgramLocalParameter4fvARB;

// GL_EXT_depth_bounds_test
PFNGLDEPTHBOUNDSEXTPROC                 qglDepthBoundsEXT;

// arb assembly info
PFNGLGETPROGRAMIVARBPROC				qglGetProgramivARB;

// Frame Buffer Objects
PFNGLISRENDERBUFFERPROC					qglIsRenderbuffer;
PFNGLBINDRENDERBUFFERPROC				qglBindRenderbuffer;
PFNGLDELETERENDERBUFFERSPROC			qglDeleteRenderbuffers;
PFNGLGENRENDERBUFFERSPROC				qglGenRenderbuffers;
PFNGLRENDERBUFFERSTORAGEPROC			qglRenderbufferStorage;
PFNGLGETRENDERBUFFERPARAMETERIVPROC		qglGetRenderbufferParameteriv;
PFNGLISFRAMEBUFFERPROC					qglIsFramebuffer;
PFNGLBINDFRAMEBUFFERPROC				qglBindFramebuffer;
PFNGLDELETEFRAMEBUFFERSPROC				qglDeleteFramebuffers;
PFNGLGENFRAMEBUFFERSPROC				qglGenFramebuffers;
PFNGLCHECKFRAMEBUFFERSTATUSPROC			qglCheckFramebufferStatus;
PFNGLFRAMEBUFFERTEXTURE1DPROC			qglFramebufferTexture1D;
PFNGLFRAMEBUFFERTEXTURE2DPROC			qglFramebufferTexture2D;
PFNGLFRAMEBUFFERTEXTUREPROC				qglFramebufferTexture;
PFNGLFRAMEBUFFERRENDERBUFFERPROC		qglFramebufferRenderbuffer;
PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC qglGetFramebufferAttachmentParameteriv;
PFNGLGENERATEMIPMAPPROC					qglGenerateMipmap;
PFNGLBLITFRAMEBUFFERPROC				qglBlitFramebuffer;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC qglRenderbufferStorageMultisample;
PFNGLFRAMEBUFFERTEXTURELAYERPROC		qglFramebufferTextureLayer;
PFNGLDRAWBUFFERSPROC					qglDrawBuffers;
PFNGLCOPYIMAGESUBDATANVPROC				qglCopyImageSubData;

// GLSL
PFNGLATTACHSHADERPROC						qglAttachShader;
PFNGLCOMPILESHADERPROC						qglCompileShader;
PFNGLCREATEPROGRAMPROC						qglCreateProgram;
PFNGLCREATESHADERPROC						qglCreateShader;
PFNGLLINKPROGRAMPROC						qglLinkProgram;
PFNGLSHADERSOURCEPROC						qglShaderSource;
PFNGLUSEPROGRAMPROC							qglUseProgram;
PFNGLUNIFORM1FPROC							qglUniform1f;
PFNGLUNIFORM2FPROC							qglUniform2f;
PFNGLUNIFORM3FPROC							qglUniform3f;
PFNGLUNIFORM4FPROC							qglUniform4f;
PFNGLUNIFORM1IPROC							qglUniform1i;
PFNGLUNIFORM2IPROC							qglUniform2i;
PFNGLUNIFORM3IPROC							qglUniform3i;
PFNGLUNIFORM4IPROC							qglUniform4i;
PFNGLUNIFORM1FVPROC							qglUniform1fv;
PFNGLUNIFORM2FVPROC							qglUniform2fv;
PFNGLUNIFORM3FVPROC							qglUniform3fv;
PFNGLUNIFORM4FVPROC							qglUniform4fv;
PFNGLUNIFORM1IVPROC							qglUniform1iv;
PFNGLUNIFORM2IVPROC							qglUniform2iv;
PFNGLUNIFORM3IVPROC							qglUniform3iv;
PFNGLUNIFORM4IVPROC							qglUniform4iv;
PFNGLUNIFORMMATRIX2FVPROC					qglUniformMatrix2fv;
PFNGLUNIFORMMATRIX3FVPROC					qglUniformMatrix3fv;
PFNGLUNIFORMMATRIX4FVPROC					qglUniformMatrix4fv;
PFNGLVALIDATEPROGRAMPROC					qglValidateProgram;
PFNGLGETSHADERIVPROC						qglGetShaderiv;
PFNGLGETATTRIBLOCATIONPROC					qglGetAttribLocation;
PFNGLGETUNIFORMLOCATIONPROC					qglGetUniformLocation;
PFNGLISPROGRAMPROC							qglIsProgram;
PFNGLISSHADERPROC							qglIsShader;
PFNGLGETSHADERINFOLOGPROC					qglGetShaderInfoLog;
PFNGLDELETEPROGRAMPROC						qglDeleteProgram;
PFNGLDELETESHADERPROC						qglDeleteShader;
PFNGLGETPROGRAMIVPROC						qglGetProgramiv;
PFNGLGETPROGRAMINFOLOGPROC					qglGetProgramInfoLog;
PFNGLBINDATTRIBLOCATIONPROC					qglBindAttribLocation;

// GL fence sync
PFNGLFENCESYNCPROC						qglFenceSync;
PFNGLCLIENTWAITSYNCPROC					qglClientWaitSync;
PFNGLDELETESYNCPROC						qglDeleteSync;

// profiling
PFNGLGENQUERIESPROC						qglGenQueries;
PFNGLDELETEQUERIESPROC					qglDeleteQueries;
PFNGLQUERYCOUNTERPROC					qglQueryCounter;
PFNGLGETQUERYOBJECTUI64VPROC			qglGetQueryObjectui64v;
PFNGLBEGINQUERYPROC						qglBeginQuery;
PFNGLENDQUERYPROC						qglEndQuery;
// debug groups
PFNGLPUSHDEBUGGROUPPROC					qglPushDebugGroup;
PFNGLPOPDEBUGGROUPPROC					qglPopDebugGroup;

/*
=================
R_CheckExtension
=================
*/
bool R_CheckExtension( const char *name ) {
	if ( !strstr( glConfig.extensions_string, name ) ) {
		common->Printf( "^1X^0 - %s not found\n", name );
		return false;
	}
	common->Printf( "^2v^0 - using %s\n", name );
	return true;
}

/*
==================
R_CheckPortableExtensions

==================
*/
static void R_CheckPortableExtensions( void ) {
	glConfig.glVersion = atof( glConfig.version_string );

	common->Printf( "Checking portable OpenGL extensions...\n" );

	// GL_ARB_multitexture
	if ( !R_CheckExtension( "GL_ARB_multitexture" ) ) {
		common->Error( "GL_ARB_multitexture not supported!\n" );
	}
	qglActiveTexture = ( void( APIENTRY * )( GLenum ) )GLimp_ExtensionPointer( "glActiveTexture" );
	qglGetIntegerv( GL_MAX_TEXTURE_COORDS_ARB, &glConfig.maxTextureCoords );
	common->Printf( "Max texture coords: %d\n", glConfig.maxTextureCoords );
	qglGetIntegerv( GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &glConfig.maxTextures );
	common->Printf( "Max active textures: %d\n", glConfig.maxTextures );

	if ( glConfig.maxTextures < MAX_MULTITEXTURE_UNITS ) {
		common->Error( "   Too few!\n" );
	}

	// GL_ARB_texture_cube_map
	if ( !R_CheckExtension( "GL_ARB_texture_cube_map" ) ) {
		common->Error( "GL_ARB_texture_cube_map not supported!\n" );
	}

	// GL_ARB_texture_non_power_of_two
	glConfig.textureNonPowerOfTwoAvailable = R_CheckExtension( "GL_ARB_texture_non_power_of_two" );

	// GL_ARB_texture_compression + GL_S3_s3tc
	// DRI drivers may have GL_ARB_texture_compression but no GL_EXT_texture_compression_s3tc
	if ( R_CheckExtension( "GL_ARB_texture_compression" ) && R_CheckExtension( "GL_EXT_texture_compression_s3tc" ) ) {
		glConfig.textureCompressionAvailable = true;
		qglCompressedTexImage2DARB = ( PFNGLCOMPRESSEDTEXIMAGE2DARBPROC )GLimp_ExtensionPointer( "glCompressedTexImage2DARB" );
		qglGetCompressedTexImageARB = ( PFNGLGETCOMPRESSEDTEXIMAGEARBPROC )GLimp_ExtensionPointer( "glGetCompressedTexImageARB" );
	} else {
		glConfig.textureCompressionAvailable = false;
	}
	glConfig.textureCompressionRgtcAvailable = R_CheckExtension( "GL_ARB_texture_compression_rgtc" );

	// GL_EXT_texture_filter_anisotropic
	glConfig.anisotropicAvailable = R_CheckExtension( "GL_EXT_texture_filter_anisotropic" );
	if ( glConfig.anisotropicAvailable ) {
		qglGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glConfig.maxTextureAnisotropy );
		common->Printf( "    maxTextureAnisotropy: %f\n", glConfig.maxTextureAnisotropy );
	} else {
		glConfig.maxTextureAnisotropy = 1;
	}

	// GL_EXT_texture_lod_bias
	if ( R_CheckExtension( "GL_EXT_texture_lod_bias" ) ) {
		glConfig.textureLODBiasAvailable = true;
	} else {
		glConfig.textureLODBiasAvailable = false;
	}

	// EXT_stencil_wrap
	// This isn't very important, but some pathological case might cause a clamp error and give a shadow bug.
	// Nvidia also believes that future hardware may be able to run faster with this enabled to avoid the
	// serialization of clamping.
	if ( R_CheckExtension( "GL_EXT_stencil_wrap" ) ) {
		tr.stencilIncr = GL_INCR_WRAP_EXT;
		tr.stencilDecr = GL_DECR_WRAP_EXT;
	} else {
		tr.stencilIncr = GL_INCR;
		tr.stencilDecr = GL_DECR;
	}

	// separate stencil (part of OpenGL 2.0 spec)
	if ( glConfig.glVersion >= 2.0 ) {
		qglStencilOpSeparate = ( PFNGLSTENCILOPSEPARATEPROC )GLimp_ExtensionPointer( "glStencilOpSeparate" );
		if ( qglStencilOpSeparate ) {
			common->Printf( "^2v^0 - using %s\n", "glStencilOpSeparate" );
			glConfig.twoSidedStencilAvailable = true;
		} else {
			common->Printf( "^1X^0 - %s not found\n", "glStencilOpSeparate" );
			glConfig.twoSidedStencilAvailable = false;
		}
	}

	// ARB_vertex_buffer_object
	//glConfig.vertexBufferObjectAvailable = R_CheckExtension( "GL_ARB_vertex_buffer_object" );
	if ( !R_CheckExtension( "GL_ARB_vertex_buffer_object" ) ) {
		common->Error( "VBO NOT SUPPORTED" );
	}
	qglBindBufferARB = ( PFNGLBINDBUFFERARBPROC )GLimp_ExtensionPointer( "glBindBufferARB" );
	qglDeleteBuffersARB = ( PFNGLDELETEBUFFERSARBPROC )GLimp_ExtensionPointer( "glDeleteBuffersARB" );
	qglGenBuffersARB = ( PFNGLGENBUFFERSARBPROC )GLimp_ExtensionPointer( "glGenBuffersARB" );
	qglIsBufferARB = ( PFNGLISBUFFERARBPROC )GLimp_ExtensionPointer( "glIsBufferARB" );
	qglBufferDataARB = ( PFNGLBUFFERDATAARBPROC )GLimp_ExtensionPointer( "glBufferDataARB" );
	qglBufferSubDataARB = ( PFNGLBUFFERSUBDATAARBPROC )GLimp_ExtensionPointer( "glBufferSubDataARB" );
	qglGetBufferSubDataARB = ( PFNGLGETBUFFERSUBDATAARBPROC )GLimp_ExtensionPointer( "glGetBufferSubDataARB" );
	qglMapBufferARB = ( PFNGLMAPBUFFERARBPROC )GLimp_ExtensionPointer( "glMapBufferARB" );
	qglUnmapBufferARB = ( PFNGLUNMAPBUFFERARBPROC )GLimp_ExtensionPointer( "glUnmapBufferARB" );
	qglGetBufferParameterivARB = ( PFNGLGETBUFFERPARAMETERIVARBPROC )GLimp_ExtensionPointer( "glGetBufferParameterivARB" );
	qglGetBufferPointervARB = ( PFNGLGETBUFFERPOINTERVARBPROC )GLimp_ExtensionPointer( "glGetBufferPointervARB" );
	qglUnmapBuffer = ( PFNGLUNMAPBUFFERPROC )GLimp_ExtensionPointer( "glUnmapBuffer" );
	qglBufferSubData = ( PFNGLBUFFERSUBDATAPROC )GLimp_ExtensionPointer( "glBufferSubData" );

	// ARB_map_buffer_range
	glConfig.mapBufferRangeAvailable = R_CheckExtension( "GL_ARB_map_buffer_range" );
	if ( glConfig.mapBufferRangeAvailable ) {
		qglMapBufferRange = ( PFNGLMAPBUFFERRANGEPROC )GLimp_ExtensionPointer( "glMapBufferRange" );
		qglFlushMappedBufferRange = ( PFNGLFLUSHMAPPEDBUFFERRANGEPROC )GLimp_ExtensionPointer( "glFlushMappedBufferRange" );
	}

	// ARB_vertex_program
	qglVertexAttribPointer = ( PFNGLVERTEXATTRIBPOINTERARBPROC )GLimp_ExtensionPointer( "glVertexAttribPointerARB" );
	qglEnableVertexAttribArray = ( PFNGLENABLEVERTEXATTRIBARRAYARBPROC )GLimp_ExtensionPointer( "glEnableVertexAttribArrayARB" );
	qglDisableVertexAttribArray = ( PFNGLDISABLEVERTEXATTRIBARRAYARBPROC )GLimp_ExtensionPointer( "glDisableVertexAttribArrayARB" );
	qglProgramStringARB = ( PFNGLPROGRAMSTRINGARBPROC )GLimp_ExtensionPointer( "glProgramStringARB" );
	qglBindProgramARB = ( PFNGLBINDPROGRAMARBPROC )GLimp_ExtensionPointer( "glBindProgramARB" );
	qglGenProgramsARB = ( PFNGLGENPROGRAMSARBPROC )GLimp_ExtensionPointer( "glGenProgramsARB" );
	qglProgramEnvParameter4fvARB = ( PFNGLPROGRAMENVPARAMETER4FVARBPROC )GLimp_ExtensionPointer( "glProgramEnvParameter4fvARB" );
	qglProgramLocalParameter4fvARB = ( PFNGLPROGRAMLOCALPARAMETER4FVARBPROC )GLimp_ExtensionPointer( "glProgramLocalParameter4fvARB" );

	// GL_EXT_depth_bounds_test
	glConfig.depthBoundsTestAvailable = R_CheckExtension( "GL_EXT_depth_bounds_test" );
	if ( glConfig.depthBoundsTestAvailable ) {
		qglDepthBoundsEXT = ( PFNGLDEPTHBOUNDSEXTPROC )GLimp_ExtensionPointer( "glDepthBoundsEXT" );
	}

	// GLSL (core since GL 2.0)
	qglAttachShader = ( PFNGLATTACHSHADERPROC )GLimp_ExtensionPointer( "glAttachShader" );
	qglCompileShader = ( PFNGLCOMPILESHADERPROC )GLimp_ExtensionPointer( "glCompileShader" );
	qglCreateProgram = ( PFNGLCREATEPROGRAMPROC )GLimp_ExtensionPointer( "glCreateProgram" );
	qglCreateShader = ( PFNGLCREATESHADERPROC )GLimp_ExtensionPointer( "glCreateShader" );
	qglLinkProgram = ( PFNGLLINKPROGRAMPROC )GLimp_ExtensionPointer( "glLinkProgram" );
	qglShaderSource = ( PFNGLSHADERSOURCEPROC )GLimp_ExtensionPointer( "glShaderSource" );
	qglUseProgram = ( PFNGLUSEPROGRAMPROC )GLimp_ExtensionPointer( "glUseProgram" );
	qglUniform1f = ( PFNGLUNIFORM1FPROC )GLimp_ExtensionPointer( "glUniform1f" );
	qglUniform2f = ( PFNGLUNIFORM2FPROC )GLimp_ExtensionPointer( "glUniform2f" );
	qglUniform3f = ( PFNGLUNIFORM3FPROC )GLimp_ExtensionPointer( "glUniform3f" );
	qglUniform4f = ( PFNGLUNIFORM4FPROC )GLimp_ExtensionPointer( "glUniform4f" );
	qglUniform1i = ( PFNGLUNIFORM1IPROC )GLimp_ExtensionPointer( "glUniform1i" );
	qglUniform2i = ( PFNGLUNIFORM2IPROC )GLimp_ExtensionPointer( "glUniform2i" );
	qglUniform3i = ( PFNGLUNIFORM3IPROC )GLimp_ExtensionPointer( "glUniform3i" );
	qglUniform4i = ( PFNGLUNIFORM4IPROC )GLimp_ExtensionPointer( "glUniform4i" );
	qglUniform1fv = ( PFNGLUNIFORM1FVPROC )GLimp_ExtensionPointer( "glUniform1fv" );
	qglUniform2fv = ( PFNGLUNIFORM2FVPROC )GLimp_ExtensionPointer( "glUniform2fv" );
	qglUniform3fv = ( PFNGLUNIFORM3FVPROC )GLimp_ExtensionPointer( "glUniform3fv" );
	qglUniform4fv = ( PFNGLUNIFORM4FVPROC )GLimp_ExtensionPointer( "glUniform4fv" );
	qglUniform1iv = ( PFNGLUNIFORM1IVPROC )GLimp_ExtensionPointer( "glUniform1iv" );
	qglUniform2iv = ( PFNGLUNIFORM2IVPROC )GLimp_ExtensionPointer( "glUniform2iv" );
	qglUniform3iv = ( PFNGLUNIFORM3IVPROC )GLimp_ExtensionPointer( "glUniform3iv" );
	qglUniform4iv = ( PFNGLUNIFORM4IVPROC )GLimp_ExtensionPointer( "glUniform4iv" );
	qglUniformMatrix2fv = ( PFNGLUNIFORMMATRIX2FVPROC )GLimp_ExtensionPointer( "glUniformMatrix2fv" );
	qglUniformMatrix3fv = ( PFNGLUNIFORMMATRIX3FVPROC )GLimp_ExtensionPointer( "glUniformMatrix3fv" );
	qglUniformMatrix4fv = ( PFNGLUNIFORMMATRIX4FVPROC )GLimp_ExtensionPointer( "glUniformMatrix4fv" );
	qglValidateProgram = ( PFNGLVALIDATEPROGRAMPROC )GLimp_ExtensionPointer( "glValidateProgram" );
	qglGetShaderiv = ( PFNGLGETSHADERIVPROC )GLimp_ExtensionPointer( "glGetShaderiv" );
	qglGetAttribLocation = ( PFNGLGETATTRIBLOCATIONPROC )GLimp_ExtensionPointer( "glGetAttribLocation" );
	qglGetUniformLocation = ( PFNGLGETUNIFORMLOCATIONPROC )GLimp_ExtensionPointer( "glGetUniformLocation" );
	qglIsProgram = ( PFNGLISPROGRAMPROC )GLimp_ExtensionPointer( "glIsProgram" );
	qglIsShader = ( PFNGLISSHADERPROC )GLimp_ExtensionPointer( "glIsShader" );
	qglGetShaderInfoLog = ( PFNGLGETSHADERINFOLOGPROC )GLimp_ExtensionPointer( "glGetShaderInfoLog" );
	qglDeleteProgram = ( PFNGLDELETEPROGRAMPROC )GLimp_ExtensionPointer( "glDeleteProgram" );
	qglDeleteShader = ( PFNGLDELETESHADERPROC )GLimp_ExtensionPointer( "glDeleteShader" );
	qglGetProgramiv = ( PFNGLGETPROGRAMIVPROC )GLimp_ExtensionPointer( "glGetProgramiv" );
	qglGetProgramInfoLog = ( PFNGLGETPROGRAMINFOLOGPROC )GLimp_ExtensionPointer( "glGetProgramInfoLog" );
	qglBindAttribLocation = ( PFNGLBINDATTRIBLOCATIONPROC )GLimp_ExtensionPointer( "glBindAttribLocation" );

	bool hasArbFramebuffer = R_CheckExtension( "GL_ARB_framebuffer_object" );
	if ( hasArbFramebuffer ) {
		glConfig.framebufferObjectAvailable = true;
		glConfig.framebufferBlitAvailable = true;
		glConfig.framebufferMultisampleAvailable = true;
		glConfig.framebufferPackedDepthStencilAvailable = true;
		// Frame Buffer Objects
		qglIsRenderbuffer = ( PFNGLISRENDERBUFFERPROC )GLimp_ExtensionPointer( "glIsRenderbuffer" );
		qglBindRenderbuffer = ( PFNGLBINDRENDERBUFFERPROC )GLimp_ExtensionPointer( "glBindRenderbuffer" );
		qglDeleteRenderbuffers = ( PFNGLDELETERENDERBUFFERSPROC )GLimp_ExtensionPointer( "glDeleteRenderbuffers" );
		qglGenRenderbuffers = ( PFNGLGENRENDERBUFFERSPROC )GLimp_ExtensionPointer( "glGenRenderbuffers" );
		qglRenderbufferStorage = ( PFNGLRENDERBUFFERSTORAGEPROC )GLimp_ExtensionPointer( "glRenderbufferStorage" );
		qglGetRenderbufferParameteriv = ( PFNGLGETRENDERBUFFERPARAMETERIVPROC )GLimp_ExtensionPointer( "glGetRenderbufferParameteriv" );
		qglIsFramebuffer = ( PFNGLISFRAMEBUFFERPROC )GLimp_ExtensionPointer( "glIsFramebuffer" );
		qglBindFramebuffer = ( PFNGLBINDFRAMEBUFFERPROC )GLimp_ExtensionPointer( "glBindFramebuffer" );
		qglDeleteFramebuffers = ( PFNGLDELETEFRAMEBUFFERSPROC )GLimp_ExtensionPointer( "glDeleteFramebuffers" );
		qglGenFramebuffers = ( PFNGLGENFRAMEBUFFERSPROC )GLimp_ExtensionPointer( "glGenFramebuffers" );
		qglCheckFramebufferStatus = ( PFNGLCHECKFRAMEBUFFERSTATUSPROC )GLimp_ExtensionPointer( "glCheckFramebufferStatus" );
		qglFramebufferTexture1D = ( PFNGLFRAMEBUFFERTEXTURE1DPROC )GLimp_ExtensionPointer( "glFramebufferTexture1D" );
		qglFramebufferTexture2D = ( PFNGLFRAMEBUFFERTEXTURE2DPROC )GLimp_ExtensionPointer( "glFramebufferTexture2D" );
		qglFramebufferTexture = ( PFNGLFRAMEBUFFERTEXTUREPROC )GLimp_ExtensionPointer( "glFramebufferTexture" );
		qglFramebufferRenderbuffer = ( PFNGLFRAMEBUFFERRENDERBUFFERPROC )GLimp_ExtensionPointer( "glFramebufferRenderbuffer" );
		qglGetFramebufferAttachmentParameteriv = ( PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC )GLimp_ExtensionPointer( "glGetFramebufferAttachmentParameteriv" );
		qglGenerateMipmap = ( PFNGLGENERATEMIPMAPPROC )GLimp_ExtensionPointer( "glGenerateMipmap" );
		qglBlitFramebuffer = ( PFNGLBLITFRAMEBUFFERPROC )GLimp_ExtensionPointer( "glBlitFramebuffer" );
		qglRenderbufferStorageMultisample = ( PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC )GLimp_ExtensionPointer( "glRenderbufferStorageMultisample" );
		qglFramebufferTextureLayer = ( PFNGLFRAMEBUFFERTEXTURELAYERPROC )GLimp_ExtensionPointer( "glFramebufferTextureLayer" );
		qglDrawBuffers = ( PFNGLDRAWBUFFERSPROC )GLimp_ExtensionPointer( "glDrawBuffers" );
		qglCopyImageSubData = ( PFNGLCOPYIMAGESUBDATANVPROC )GLimp_ExtensionPointer( "glCopyImageSubData" );
	} else {
		glConfig.framebufferObjectAvailable = R_CheckExtension( "GL_EXT_framebuffer_object" );
		glConfig.framebufferBlitAvailable = R_CheckExtension( "GL_EXT_framebuffer_blit" );
		glConfig.framebufferMultisampleAvailable = R_CheckExtension( "GL_EXT_framebuffer_multisample" );
		glConfig.framebufferPackedDepthStencilAvailable = R_CheckExtension( "GL_EXT_packed_depth_stencil" );
		if ( glConfig.framebufferObjectAvailable ) {
			qglGenFramebuffers = ( PFNGLGENFRAMEBUFFERSEXTPROC )GLimp_ExtensionPointer( "glGenFramebuffersEXT" );
			qglBindFramebuffer = ( PFNGLBINDFRAMEBUFFERPROC )GLimp_ExtensionPointer( "glBindFramebufferEXT" );
			qglDeleteFramebuffers = ( PFNGLDELETEFRAMEBUFFERSPROC )GLimp_ExtensionPointer( "glDeleteFramebuffersEXT" );
			qglFramebufferTexture2D = ( PFNGLFRAMEBUFFERTEXTURE2DEXTPROC )GLimp_ExtensionPointer( "glFramebufferTexture2DEXT" );
			qglCheckFramebufferStatus = ( PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC )GLimp_ExtensionPointer( "glCheckFramebufferStatusEXT" );
			qglGenRenderbuffers = ( PFNGLGENRENDERBUFFERSEXTPROC )GLimp_ExtensionPointer( "glGenRenderbuffersEXT" );
			qglBindRenderbuffer = ( PFNGLBINDRENDERBUFFEREXTPROC )GLimp_ExtensionPointer( "glBindRenderbufferEXT" );
			qglRenderbufferStorage = ( PFNGLRENDERBUFFERSTORAGEEXTPROC )GLimp_ExtensionPointer( "glRenderbufferStorageEXT" );
			qglFramebufferRenderbuffer = ( PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC )GLimp_ExtensionPointer( "glFramebufferRenderbufferEXT" );
			qglGenerateMipmap = ( PFNGLGENERATEMIPMAPPROC )GLimp_ExtensionPointer( "glGenerateMipmapEXT" ); // why here?
		}
		if ( glConfig.framebufferBlitAvailable ) {
			qglBlitFramebuffer = ( PFNGLBLITFRAMEBUFFEREXTPROC )GLimp_ExtensionPointer( "glBlitFramebufferEXT" );
		}
		if ( glConfig.framebufferMultisampleAvailable ) {
			qglRenderbufferStorageMultisample = ( PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC )GLimp_ExtensionPointer( "glRenderbufferStorageMultisampleEXT" );
		}
	}
	glConfig.pixelBufferAvailable = R_CheckExtension("GL_ARB_pixel_buffer_object");

	if( glConfig.glVersion > 3.2 || R_CheckExtension( "GL_ARB_timer_query" ) ) {
		glConfig.timerQueriesAvailable = true;
		qglGenQueries = ( PFNGLGENQUERIESPROC )GLimp_ExtensionPointer( "glGenQueries" );
		qglDeleteQueries = ( PFNGLDELETEQUERIESPROC )GLimp_ExtensionPointer( "glDeleteQueries" );
		qglQueryCounter = ( PFNGLQUERYCOUNTERPROC )GLimp_ExtensionPointer( "glQueryCounter" );
		qglGetQueryObjectui64v = ( PFNGLGETQUERYOBJECTUI64VPROC )GLimp_ExtensionPointer( "glGetQueryObjectui64v" );
		qglBeginQuery = ( PFNGLBEGINQUERYPROC )GLimp_ExtensionPointer( "glBeginQuery" );
		qglEndQuery = ( PFNGLENDQUERYPROC )GLimp_ExtensionPointer( "glEndQuery" );
	}

	if( glConfig.glVersion > 4.2 || R_CheckExtension( "GL_KHR_debug" ) ) {
		glConfig.debugGroupsAvailable = true;
		qglPushDebugGroup = ( PFNGLPUSHDEBUGGROUPPROC )GLimp_ExtensionPointer( "glPushDebugGroup" );
		qglPopDebugGroup = ( PFNGLPOPDEBUGGROUPPROC )GLimp_ExtensionPointer( "glPopDebugGroup" );
	}

	//	 -----====+++|   END TDM ~SS Extensions   |+++====-----   */
	glConfig.pixelBufferAvailable = R_CheckExtension("GL_ARB_pixel_buffer_object");

	glConfig.fenceSyncAvailable = R_CheckExtension( "GL_ARB_sync" );
	if ( glConfig.fenceSyncAvailable ) {
		qglFenceSync = ( PFNGLFENCESYNCPROC )GLimp_ExtensionPointer( "glFenceSync" );
		qglClientWaitSync = ( PFNGLCLIENTWAITSYNCPROC )GLimp_ExtensionPointer( "glClientWaitSync" );
		qglDeleteSync = ( PFNGLDELETESYNCPROC )GLimp_ExtensionPointer( "glDeleteSync" );
		common->Printf( "GL fence sync available\n" );
	}
	int n;
	qglGetIntegerv( GL_MAX_VERTEX_ATTRIBS, &n );
	common->Printf( "Max vertex attribs: %d\n", n );
	qglGetProgramivARB = ( PFNGLGETPROGRAMIVARBPROC )GLimp_ExtensionPointer( "glGetProgramivARB" );
	if ( qglGetProgramivARB ) {
		qglGetProgramivARB( GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_ENV_PARAMETERS_ARB, &n );
		common->Printf( "Max env parameters: %d\n", n );
	}
}

/*
====================
R_GetModeInfo

r_mode is normally a small non-negative integer that
looks resolutions up in a table, but if it is set to -1,
the values from r_customWidth, amd r_customHeight
will be used instead.
====================
*/
typedef struct vidmode_s {
	const char *description;
	int         width, height;
} vidmode_t;

vidmode_t r_vidModes[] = {
	{ "Mode  0: 320x240",		320,	240 },
	{ "Mode  1: 400x300",		400,	300 },
	{ "Mode  2: 512x384",		512,	384 },
	{ "Mode  3: 640x480",		640,	480 },
	{ "Mode  4: 800x600",		800,	600 },
	{ "Mode  5: 1024x768",		1024,	768 },
	{ "Mode  6: 1152x864",		1152,	864 },
	{ "Mode  7: 1280x1024",		1280,	1024 },
	{ "Mode  8: 1600x1200",		1600,	1200 },
};
static int	s_numVidModes = ( sizeof( r_vidModes ) / sizeof( r_vidModes[0] ) );

#if MACOS_X
bool R_GetModeInfo( int *width, int *height, int mode ) {
#else
static bool R_GetModeInfo( int *width, int *height, int mode ) {
#endif
	vidmode_t	*vm;

	if ( mode < -1 ) {
		return false;
	}

	if ( mode >= s_numVidModes ) {
		return false;
	}

	if ( mode == -1 ) {
		*width = r_customWidth.GetInteger();
		*height = r_customHeight.GetInteger();
		return true;
	}
	vm = &r_vidModes[mode];

	if ( width ) {
		*width  = vm->width;
	}

	if ( height ) {
		*height = vm->height;
	}
	return true;
}


/*
==================
R_InitOpenGL

This function is responsible for initializing a valid OpenGL subsystem
for rendering.  This is done by calling the system specific GLimp_Init,
which gives us a working OGL subsystem, then setting all necessary openGL
state, including images, vertex programs, and display lists.

Changes to the vertex cache size or smp state require a vid_restart.

If glConfig.isInitialized is false, no rendering can take place, but
all renderSystem functions will still operate properly, notably the material
and model information functions.
==================
*/
void R_InitOpenGL( void ) {
	GLint			temp;
	glimpParms_t	parms;
	int				i;

	common->Printf( "----- Initializing OpenGL -----\n" );

	if ( glConfig.isInitialized ) {
		common->FatalError( "R_InitOpenGL called while active" );
	}

	// in case we had an error while doing a tiled rendering
	tr.viewportOffset[0] = 0;
	tr.viewportOffset[1] = 0;

	//
	// initialize OS specific portions of the renderSystem
	//
	for ( i = 0 ; i < 2 ; i++ ) {
		// set the parameters we are trying
		R_GetModeInfo( &glConfig.vidWidth, &glConfig.vidHeight, r_mode.GetInteger() );

		parms.width = glConfig.vidWidth;
		parms.height = glConfig.vidHeight;
		parms.fullScreen = r_fullscreen.GetBool();
		parms.displayHz = r_displayRefresh.GetInteger();
		parms.stereo = false;

		if ( !r_useFbo.GetBool() ) {
			parms.multiSamples = r_multiSamples.GetInteger();
		} else {
			parms.multiSamples = 0;
		}

		if ( GLimp_Init( parms ) ) {
			// it worked
			break;
		}

		if ( i == 1 ) {
			common->FatalError( "Unable to initialize OpenGL" );
		}

		// if we failed, set everything back to "safe mode"
		// and try again
		r_mode.SetInteger( 3 );
		r_fullscreen.SetInteger( 1 );
		r_displayRefresh.SetInteger( 0 );
		r_multiSamples.SetInteger( 0 );
	}

	// input and sound systems need to be tied to the new window
	Sys_InitInput();
	soundSystem->InitHW();

	// get our config strings
	glConfig.vendor_string = ( const char * )qglGetString( GL_VENDOR );
	glConfig.renderer_string = ( const char * )qglGetString( GL_RENDERER );
	glConfig.version_string = ( const char * )qglGetString( GL_VERSION );
	glConfig.extensions_string = ( const char * )qglGetString( GL_EXTENSIONS );

	if ( strcmp( glConfig.vendor_string, "Intel" ) == 0 ) { 
		glConfig.vendor = glvIntel; 
	}

	if ( strcmp( glConfig.vendor_string, "ATI Technologies Inc." ) == 0 ) { 
		glConfig.vendor = glvAMD; 
	}

	if ( strncmp( glConfig.vendor_string, "NVIDIA", 6 ) == 0 || 
		 strncmp( glConfig.vendor_string, "Nvidia", 6 ) == 0 || 
		 strncmp( glConfig.vendor_string, "nvidia", 6 ) == 0 ) {
		 glConfig.vendor = glvNVIDIA;
	}

	// OpenGL driver constants
	qglGetIntegerv( GL_MAX_TEXTURE_SIZE, &temp );
	glConfig.maxTextureSize = temp;

	// stubbed or broken drivers may have reported 0...
	if ( glConfig.maxTextureSize <= 0 ) {
		glConfig.maxTextureSize = 256;
	}
	glConfig.isInitialized = true;

	common->Printf( "OpenGL vendor: %s\n", glConfig.vendor_string );
	common->Printf( "OpenGL renderer: %s\n", glConfig.renderer_string );
	common->Printf( "OpenGL version: %s\n", glConfig.version_string );

	// recheck all the extensions (FIXME: this might be dangerous)
	R_CheckPortableExtensions();

	cmdSystem->AddCommand( "reloadGLSLprograms", R_ReloadGLSLPrograms_f, CMD_FL_RENDERER, "reloads GLSL programs" );
	cmdSystem->AddCommand( "reloadARBprograms", R_ReloadARBPrograms_f, CMD_FL_RENDERER, "reloads ARB programs" );

	R_ReloadARBPrograms_f( idCmdArgs() );
	R_ReloadGLSLPrograms_f( idCmdArgs() );

	// allocate the vertex array range or vertex objects
	vertexCache.Init();

	// allocate the frame data, which may be more if smp is enabled
	R_InitFrameData();

	// Reset our gamma
	R_SetColorMappings();

	if ( ( glConfig.vendor == glvNVIDIA ) && r_softShadowsQuality.GetBool() && r_nVidiaOverride.GetBool() ) {
		common->Printf( "Nvidia Hardware Detected. Forcing FBO\n" );
		r_useFbo.SetBool( true );
	}

#ifdef _WIN32
	static bool glCheck = false;
	if ( !glCheck && win32.osversion.dwMajorVersion >= 6 ) {
		glCheck = true;
		if ( !idStr::Icmp( glConfig.vendor_string, "Microsoft" ) && idStr::FindText( glConfig.renderer_string, "OpenGL-D3D" ) != -1 ) {
			if ( cvarSystem->GetCVarBool( "r_fullscreen" ) ) {
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, "vid_restart partial windowed\n" );
				Sys_GrabMouseCursor( false );
			}
			int ret = MessageBox( NULL, "Please install OpenGL drivers from your graphics hardware vendor to run " GAME_NAME ".\nYour OpenGL functionality is limited.",
										"Insufficient OpenGL capabilities", MB_OKCANCEL | MB_ICONWARNING | MB_TASKMODAL );
			if ( ret == IDCANCEL ) {
				cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "quit\n" );
				cmdSystem->ExecuteCommandBuffer();
			}
			if ( cvarSystem->GetCVarBool( "r_fullscreen" ) ) {
				cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "vid_restart\n" );
			}
		}
	}
#endif
}

/*
==================
GL_CheckErrors
==================
*/
void GL_CheckErrors( void ) {
	if ( r_ignoreGLErrors.GetBool() ) {
		return;
	}
	int		err;
	char	s[64];
	int		i;
	// check for up to 10 errors pending
	for ( i = 0 ; i < 10 ; i++ ) {
		err = qglGetError();
		if ( err == GL_NO_ERROR ) { 
			return; 
		}
		switch ( err ) {
		case GL_INVALID_ENUM:
			strcpy( s, "GL_INVALID_ENUM" );
			break;
		case GL_INVALID_VALUE:
			strcpy( s, "GL_INVALID_VALUE" );
			break;
		case GL_INVALID_OPERATION:
			strcpy( s, "GL_INVALID_OPERATION" );
			break;
		case GL_STACK_OVERFLOW:
			strcpy( s, "GL_STACK_OVERFLOW" );
			break;
		case GL_STACK_UNDERFLOW:
			strcpy( s, "GL_STACK_UNDERFLOW" );
			break;
		case GL_OUT_OF_MEMORY:
			strcpy( s, "GL_OUT_OF_MEMORY" );
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			strcpy( s, "GL_INVALID_FRAMEBUFFER_OPERATION" );
			break;
		default:
			idStr::snPrintf( s, sizeof( s ), "%i", err );
			break;
		}
		common->Printf( "GL_CheckErrors: %s\n", s );
	}
}

/*
=====================
R_ReloadSurface_f

Reload the material displayed by r_showSurfaceInfo
=====================
*/
static void R_ReloadSurface_f( const idCmdArgs &args ) {
	// Skip if the current render is the lightgem render (default RENDERTOOLS_SKIP_ID)
	if ( tr.primaryView->IsLightGem() )	{
		return;
	}
	modelTrace_t mt;

	// start far enough away that we don't hit the player model
	const idVec3 start = tr.primaryView->renderView.vieworg + tr.primaryView->renderView.viewaxis[0] * 16;
	const idVec3 end = start + tr.primaryView->renderView.viewaxis[0] * 1000.0f;
	if ( !tr.primaryWorld->Trace( mt, start, end, 0.0f, false, true ) ) {
		return;
	}
	common->Printf( "Reloading %s\n", mt.material->GetName() );

	// reload the decl
	mt.material->base->Reload();

	// reload any images used by the decl
	mt.material->ReloadImages( false );
}

/*
==============
R_ListModes_f
==============
*/
static void R_ListModes_f( const idCmdArgs &args ) {
	int i;
	common->Printf( "\n" );
	for ( i = 0; i < s_numVidModes; i++ ) {
		common->Printf( "%s\n", r_vidModes[i].description );
	}
	common->Printf( "\n" );
}

/*
=============
R_TestImage_f

Display the given image centered on the screen.
testimage <number>
testimage <filename>
=============
*/
void R_TestImage_f( const idCmdArgs &args ) {
	int imageNum;

	if ( tr.testVideo ) {
		delete tr.testVideo;
		tr.testVideo = NULL;
	}
	tr.testImage = NULL;

	if ( args.Argc() != 2 ) {
		return;
	}

	if ( idStr::IsNumeric( args.Argv( 1 ) ) ) {
		imageNum = atoi( args.Argv( 1 ) );
		if ( imageNum >= 0 && imageNum < globalImages->images.Num() ) {
			tr.testImage = globalImages->images[imageNum];
		}
	} else {
		tr.testImage = globalImages->ImageFromFile( args.Argv( 1 ), TF_DEFAULT, false, TR_REPEAT, TD_DEFAULT );
	}
}

/*
=============
TestVideoClean
=============
*/
static void TestVideoClean() {
	if ( tr.testVideo ) {
		tr.testVideo->Close();
		tr.testVideo = NULL;
	}
	tr.testImage = NULL;
}

/*
=============
R_TestVideo_f

Plays the cinematic file in a testImage
=============
*/
void R_TestVideo_f( const idCmdArgs &args ) {
	TestVideoClean();
	if ( args.Argc() < 2 ) {
		return;
	}
	//stgatilov #4847: support testing FFmpeg videos with audio stream
	bool withAudio = args.Argc() >= 3 && strcmp( args.Argv( 2 ), "withAudio" ) == 0;

	tr.testImage = globalImages->ImageFromFile( "_scratch", TF_DEFAULT, false, TR_REPEAT, TD_DEFAULT );
	tr.testVideo = idCinematic::Alloc( args.Argv( 1 ) );
	tr.testVideo->InitFromFile( args.Argv( 1 ), false, withAudio );
	tr.testVideoStartTime = tr.primaryRenderView.time * 0.001;

	cinData_t cin;
	cin = tr.testVideo->ImageForTime( 0 );
	if ( !cin.image ) {
		common->Warning( "Failed to get first frame from video file" );
		return TestVideoClean();
	}
	common->Printf( "%i x %i images\n", cin.imageWidth, cin.imageHeight );
	int	len = tr.testVideo->AnimationLength();
	common->Printf( "%5.1f seconds of video\n", len * 0.001 );

	if ( withAudio ) {
		//stgatilov #4847: check that audio stream is peekable
		float buff[4096] = { 0 };
		int cnt = 1024;
		bool ok = tr.testVideo->SoundForTimeInterval( 0, &cnt, 44100, buff );
		if ( !ok ) {
			common->Warning( "Failed to get first few sound samples from video file" );
			return TestVideoClean();
		}
		common->Printf( "Sound stream opened\n" );

		//create implicit sound shader with special name
		char soundName[256];
		idStr::snPrintf( soundName, sizeof( soundName ), "__testvideo:%p__", tr.testVideo );
		session->sw->PlayShaderDirectly( soundName );
	} else {
		// try to play the matching wav file
		idStr	wavString = args.Argv( ( args.Argc() == 2 ) ? 1 : 2 );
		wavString.StripFileExtension();
		wavString = wavString + ".wav";
		session->sw->PlayShaderDirectly( wavString.c_str() );
	}
}

static int R_QsortSurfaceAreas( const void *a, const void *b ) {
	const idMaterial	*ea, *eb;
	int	ac, bc;

	ea = *( idMaterial ** )a;

	if ( !ea->EverReferenced() ) {
		ac = 0;
	} else {
		ac = ea->GetSurfaceArea();
	}
	eb = *( idMaterial ** )b;

	if ( !eb->EverReferenced() ) {
		bc = 0;
	} else {
		bc = eb->GetSurfaceArea();
	}

	if ( ac < bc ) {
		return -1;
	}

	if ( ac > bc ) {
		return 1;
	}
	return idStr::Icmp( ea->GetName(), eb->GetName() );
}


/*
===================
R_ReportSurfaceAreas_f

Prints a list of the materials sorted by surface area
===================
*/
void R_ReportSurfaceAreas_f( const idCmdArgs &args ) {
	int	i, count;
	idMaterial	**list;

	count = declManager->GetNumDecls( DECL_MATERIAL );
	list = ( idMaterial ** )_alloca( count * sizeof( *list ) );

	for ( i = 0 ; i < count ; i++ ) {
		list[i] = ( idMaterial * )declManager->DeclByIndex( DECL_MATERIAL, i, false );
	}
	qsort( list, count, sizeof( list[0] ), R_QsortSurfaceAreas );

	// skip over ones with 0 area
	for ( i = 0 ; i < count ; i++ ) {
		if ( list[i]->GetSurfaceArea() > 0 ) {
			break;
		}
	}

	for ( /**/; i < count ; i++ ) {
		// report size in "editor blocks"
		int	blocks = list[i]->GetSurfaceArea() / 4096.0;
		common->Printf( "%7i %s\n", blocks, list[i]->GetName() );
	}
}

/*
===================
R_ReportImageDuplication_f

Checks for images with the same hash value and does a better comparison
===================
*/
void R_ReportImageDuplication_f( const idCmdArgs &args ) {
	int	count = 0;

	common->Printf( "Images with duplicated contents:\n" );
	if ( !globalImages->image_blockChecksum.GetBool() ) // duzenko #4400
	{ common->Printf( "Warning: image_blockChecksum set to 0, results invalid.\n" ); }

	for ( int i = 0 ; i < globalImages->images.Num() ; i++ ) {
		idImage	*image1 = globalImages->images[i];

		if ( image1->generatorFunction  || // ignore procedural images
		     image1->type != TT_2D		|| // ignore cube maps
		     image1->imageHash == 0		|| // FIXME: This is a hack - Some images are not being hashed - Fonts/gui mainly
		     image1->imageHash == -1	|| // FIXME: This is a hack - Some images are not being hashed - Fonts/gui mainly
		     image1->defaulted ) {
			continue;
		}
		byte *data1;
		int	h1 = 0;
		int w1 = 0;

		R_LoadImageProgram( image1->imgName, &data1, &w1, &h1, NULL );

		if ( !data1 ) { // duzenko #4400
			continue;
		}

		for ( int j = 0 ; j < i ; j++ ) {
			idImage	*image2 = globalImages->images[j];
			if ( image2->generatorFunction  || // ignore procedural images
			     image2->type != TT_2D		|| // ignore cube maps
			     image2->imageHash == 0		|| // FIXME: This is a hack - Some images are not being hashed - Fonts/gui mainly
			     image2->imageHash == -1	|| // FIXME: This is a hack - Some images are not being hashed - Fonts/gui mainly
			     image2->defaulted ) {
				continue;
			} else if ( image1->imageHash    != image2->imageHash    ||
			            image1->uploadWidth  != image2->uploadWidth  ||
			            image1->uploadHeight != image2->uploadHeight ||
			            !idStr::Icmp( image1->imgName, image2->imgName ) ) { // ignore same image-with-different-parms
				continue;
			} else {
				byte *data2;
				int	h2 = 0;
				int w2 = 0;

				R_LoadImageProgram( image2->imgName, &data2, &w2, &h2, NULL );

				if ( !data2 || w1 != w2 || h1 != h2 || memcmp( data1, data2, w1 * h1 * 4 ) ) { // duzenko #4400
					R_StaticFree( data2 );
					continue;
				}
				R_StaticFree( data2 ); // duzenko #4400

				common->Printf( "%s == %s\n", image1->imgName.c_str(), image2->imgName.c_str() );
				session->UpdateScreen( true );
				count++;
				break;
			}
		}
		R_StaticFree( data1 );
	}
	const idStr repcol = ( count < 20 ) ? S_COLOR_GREEN : S_COLOR_RED;

	common->Printf( repcol + "Result : %i collisions out of %i images\n", count, globalImages->images.Num() );
}

/*
==============================================================================

						THROUGHPUT BENCHMARKING

==============================================================================
*/

/*
================
R_RenderingFPS
================
*/
static float R_RenderingFPS( const renderView_t &renderView ) {
	qglFinish();

	int		start = Sys_Milliseconds();
	static const int SAMPLE_MSEC = 1000;
	int		end;
	int		count = 0;

	while ( 1 ) {
		// render
		renderSystem->BeginFrame( glConfig.vidWidth, glConfig.vidHeight );
		tr.primaryWorld->RenderScene( renderView );
		renderSystem->EndFrame( NULL, NULL );
		qglFinish();
		count++;
		end = Sys_Milliseconds();
		if ( end - start > SAMPLE_MSEC ) {
			break;
		}
	}
	float fps = count * 1000.0 / ( end - start );

	return fps;
}

/*
================
R_Benchmark_f
================
*/
void R_Benchmark_f( const idCmdArgs &args ) {
	float	fps, msec;
	renderView_t	view;

	if ( !tr.primaryView ) {
		common->Printf( "No primaryView for benchmarking\n" );
		return;
	}
	view = tr.primaryRenderView;

	for ( int size = 100 ; size >= 10 ; size -= 10 ) {
		r_screenFraction.SetInteger( size );
		fps = R_RenderingFPS( view );
		int	kpix = glConfig.vidWidth * glConfig.vidHeight * ( size * 0.01 ) * ( size * 0.01 ) * 0.001;
		msec = 1000.0 / fps;
		common->Printf( "kpix: %4i  msec:%5.1f fps:%5.1f\n", kpix, msec, fps );
	}

	// enable r_singleTriangle 1 while r_screenFraction is still at 10
	r_singleTriangle.SetBool( 1 );
	fps = R_RenderingFPS( view );
	msec = 1000.0 / fps;
	common->Printf( "single tri  msec:%5.1f fps:%5.1f\n", msec, fps );
	r_singleTriangle.SetBool( 0 );
	r_screenFraction.SetInteger( 100 );

	// enable r_skipRenderContext 1
	r_skipRenderContext.SetBool( true );
	fps = R_RenderingFPS( view );
	msec = 1000.0 / fps;
	common->Printf( "no context  msec:%5.1f fps:%5.1f\n", msec, fps );
	r_skipRenderContext.SetBool( false );
}


/*
==============================================================================

						SCREEN SHOTS

==============================================================================
*/

/*
====================
R_ReadTiledPixels

Allows the rendering of an image larger than the actual window by
tiling it into window-sized chunks and rendering each chunk separately

If ref isn't specified, the full session UpdateScreen will be done.
====================
*/
void R_ReadTiledPixels( int width, int height, byte *buffer, renderView_t *ref = NULL ) {
	// include extra space for OpenGL padding to word boundaries
	byte *temp = ( byte * )R_StaticAlloc( ( glConfig.vidWidth + 3 ) * glConfig.vidHeight * 3 );

	const int oldWidth = glConfig.vidWidth;
	const int oldHeight = glConfig.vidHeight;

	tr.tiledViewport[0] = width;
	tr.tiledViewport[1] = height;

	// disable scissor, so we don't need to adjust all those rects
	r_useScissor.SetBool( false );

	for ( int xo = 0 ; xo < width ; xo += oldWidth ) {
		for ( int yo = 0 ; yo < height ; yo += oldHeight ) {
			tr.viewportOffset[0] = -xo;
			tr.viewportOffset[1] = -yo;
			int w = ( xo + oldWidth > width ) ? ( width - xo ) : oldWidth;
			int h = ( yo + oldHeight > height ) ? ( height - yo ) : oldHeight;

			if ( ref ) {
				tr.BeginFrame( oldWidth, oldHeight );
				tr.primaryWorld->RenderScene( *ref );
				copyRenderCommand_t &cmd = *( copyRenderCommand_t * )R_GetCommandBuffer( sizeof( cmd ) );
				cmd.commandId = RC_COPY_RENDER;
				cmd.buffer = temp;
				cmd.usePBO = false;
				cmd.image = NULL;
				cmd.x = 0;
				cmd.y = 0;
				cmd.imageWidth = oldWidth;
				cmd.imageHeight = oldHeight;
				tr.EndFrame( NULL, NULL );
				tr.BeginFrame( oldWidth, oldHeight );
				tr.EndFrame( NULL, NULL );
			} else {
				session->UpdateScreen( false );
				qglReadBuffer( GL_FRONT );
				qglReadPixels( 0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, temp );
			}
			int	row = ( oldWidth * 3 + 3 ) & ~3;		// OpenGL pads to dword boundaries

			for ( int y = 0 ; y < h ; y++ ) {
				memcpy( buffer + ( ( yo + y )* width + xo ) * 3, temp + y * row, w * 3 );
			}
		}
	}
	r_useScissor.SetBool( true );

	tr.viewportOffset[0] = 0;
	tr.viewportOffset[1] = 0;
	tr.tiledViewport[0] = 0;
	tr.tiledViewport[1] = 0;

	R_StaticFree( temp );

	glConfig.vidWidth = oldWidth;
	glConfig.vidHeight = oldHeight;
}

/*
====================
Screenshot_ChangeFilename
====================
*/
void Screenshot_ChangeFilename( idStr& filename, const char *extension ) {
	idStr mapname( cvarSystem->GetCVarString( "fs_currentfm" ) );
	char thetime[MAX_IMAGE_NAME / 2];

	if ( !mapname || mapname[0] == '\0' ) {
		mapname = "noFm";
	}

	// uhm any particular reason this was encapsulated in brackets ?
	time_t tt;
	time( &tt );
	struct tm * ltime = localtime( &tt );
	strftime( thetime, sizeof( thetime ), "_%Y-%m-%d_%H.%M.%S.", ltime );
	const idStr fileOnly = mapname + thetime + extension;

	filename = "screenshots/";
	filename.AppendPath( fileOnly );
}

/*
==================
TakeScreenshot

Move to tr_imagefiles.c...

Will automatically tile render large screen shots if necessary
Downsample is the number of steps to mipmap the image before saving it
If ref == NULL, session->updateScreen will be used
==================
*/
void idRenderSystemLocal::TakeScreenshot( int width, int height, const char *fileName, int blends, renderView_t *ref, bool envshot ) {
	byte *buffer;
	int	i, j, c, temp;

	takingScreenshot = true;

	int	pix = width * height;

	buffer = ( byte * )R_StaticAlloc( pix * 3 + 18 );
	memset( buffer, 0, 18 );

	if ( blends <= 1 ) {
		R_ReadTiledPixels( width, height, buffer + 18, ref );
	} else {
		unsigned short *shortBuffer = ( unsigned short * )R_StaticAlloc( pix * 2 * 3 );
		memset( shortBuffer, 0, pix * 2 * 3 );

		// enable anti-aliasing jitter
		r_jitter.SetBool( true );

		for ( i = 0 ; i < blends ; i++ ) {
			R_ReadTiledPixels( width, height, buffer + 18, ref );

			for ( j = 0 ; j < pix * 3 ; j++ ) {
				shortBuffer[j] += buffer[18 + j];
			}
		}

		// divide back to bytes
		for ( i = 0 ; i < pix * 3 ; i++ ) {
			buffer[18 + i] = shortBuffer[i] / blends;
		}
		R_StaticFree( shortBuffer );
		r_jitter.SetBool( false );
	}

	// fill in the header (this is vertically flipped, which qglReadPixels emits)
	buffer[2] = 2;		// uncompressed type
	buffer[12] = width & 255;
	buffer[13] = width >> 8;
	buffer[14] = height & 255;
	buffer[15] = height >> 8;
	buffer[16] = 24;	// pixel size

	// swap rgb to bgr
	c = 18 + width * height * 3;
	for ( i = 18 ; i < c ; i += 3 ) {
		temp = buffer[i];
		buffer[i] = buffer[i + 2];
		buffer[i + 2] = temp;
	}

	// greebo: Check if we should save a screen shot format other than TGA
	if ( !envshot && ( idStr::Icmp( r_screenshot_format.GetString(), "tga" ) != 0 ) ) {
		// load screenshot file buffer into image
		Image image;
		image.LoadImageFromMemory( ( const unsigned char * )buffer, ( unsigned int )c, "TDM_screenshot" );

		// find the preferred image format
		idStr extension = r_screenshot_format.GetString( );

		Image::Format format = Image::GetFormatFromString( extension.c_str() );

		if ( format == Image::AUTO_DETECT ) {
			common->Warning( "Unknown screenshot extension %s, falling back to default.", extension.c_str() );

			format = Image::TGA;
			extension = "tga";
		}

		// change extension and index of screenshot file
		idStr changedPath( fileName );
		Screenshot_ChangeFilename( changedPath, extension.c_str() );

		// try to save image in other format
		if ( !image.SaveImageToVfs( changedPath, format ) ) {
			common->Warning( "Could not save screenshot: %s", changedPath.c_str() );
		} else {
			common->Printf( "Wrote %s\n", changedPath.c_str() );
		}
	} else {
		// change extension and index of screenshot file
		idStr changedPath( fileName );

		// if envshot is being used, don't name the image using the map + date convention
		if ( !envshot ) {
			Screenshot_ChangeFilename( changedPath, "tga" );
		}

		// Format is TGA, just save the buffer
		fileSystem->WriteFile( changedPath.c_str(), buffer, c, "fs_savepath", "" );

		common->Printf( "Wrote %s\n", changedPath.c_str() );
	}
	R_StaticFree( buffer );

	takingScreenshot = false;
}

/*
==================
R_BlendedScreenShot

screenshot
screenshot [filename]
screenshot [width] [height]
screenshot [width] [height] [samples]
==================
*/
#define	MAX_BLENDS	256	// to keep the accumulation in shorts
void R_ScreenShot_f( const idCmdArgs &args ) {
	idStr checkname;
	qglFinish();

	static bool stopTimeT = false;
	int width = glConfig.vidWidth;
	int height = glConfig.vidHeight;
	int	blends = 0;

	if ( g_stopTime.GetBool() ) {
		stopTimeT = true;
	} else {
		g_stopTime.SetBool( true );
	}

	switch ( args.Argc() ) {
	case 1:
		width = glConfig.vidWidth;
		height = glConfig.vidHeight;
		blends = 1;
		break;
	case 2:
		width = glConfig.vidWidth;
		height = glConfig.vidHeight;
		blends = 1;
		checkname = args.Argv( 1 );
		break;
	case 3:
		width = atoi( args.Argv( 1 ) );
		height = atoi( args.Argv( 2 ) );
		blends = 1;
		break;
	case 4:
		width = atoi( args.Argv( 1 ) );
		height = atoi( args.Argv( 2 ) );
		blends = atoi( args.Argv( 3 ) );
		if ( blends < 1 ) {
			blends = 1;
		}
		if ( blends > MAX_BLENDS ) {
			blends = MAX_BLENDS;
		}
		break;
	default:
		common->Printf( "usage: screenshot\n       screenshot <filename>\n       screenshot <width> <height>\n       screenshot <width> <height> <blends>\n" );
		return;
	}

	// put the console away
	console->Close();

	tr.TakeScreenshot( width, height, checkname, blends, NULL );

	if ( !stopTimeT ) {
		g_stopTime.SetBool( false );
	}
	stopTimeT = false;

}

/*
===============
R_StencilShot
Save out a screenshot showing the stencil buffer expanded by 16x range
===============
*/
void R_StencilShot( void ) {
	byte		*buffer;

	const int	width = tr.GetScreenWidth();
	const int	height = tr.GetScreenHeight();
	const int	pix = width * height;
	const int	flen = pix * 3 + 18;

	buffer = ( byte * )Mem_Alloc( flen );
	memset( buffer, 0, 18 );

	byte *byteBuffer = ( byte * )Mem_Alloc( pix );

	qglReadPixels( 0, 0, width, height, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, byteBuffer );

	for ( int i = 0 ; i < pix ; i++ ) {
		buffer[18 + i * 3] =
		    buffer[18 + i * 3 + 1] =
		        buffer[18 + i * 3 + 2] = byteBuffer[i];
	}

	// fill in the header (this is vertically flipped, which qglReadPixels emits)
	buffer[ 2] = 2;		// uncompressed type
	buffer[12] = width & 255;
	buffer[13] = width >> 8;
	buffer[14] = height & 255;
	buffer[15] = height >> 8;
	buffer[16] = 24;	// pixel size

	fileSystem->WriteFile( "screenshots/stencilShot.tga", buffer, flen, "fs_savepath", "" );

	Mem_Free( buffer );
	Mem_Free( byteBuffer );
}

// nbohr1more #4041: add envshotGL for cubicLight
/*
==================
R_EnvShotGL_f

envshotGL <basename>

(OpenGL orientation) Saves out env/<basename>_ft.tga, etc
==================
*/
const static char *GLcubeExtensions[6] = { "_px.tga", "_nx.tga", "_py.tga", "_ny.tga", "_pz.tga", "_nz.tga" };

void R_EnvShotGL_f( const idCmdArgs &args ) {
	idStr		fullname;
	const char	*baseName;
	idMat3		axis[6];
	renderView_t	ref;
	viewDef_t	primary;
	int			blends, size;

	if ( !tr.primaryView ) {
		common->Printf( "No primary view.\n" );
		return;
	} else if ( args.Argc() != 2 && args.Argc() != 3 && args.Argc() != 4 ) {
		common->Printf( "USAGE: envshotGL <basename> [size] [blends]\n" );
		return;
	}
	primary = *tr.primaryView;
	baseName = args.Argv( 1 );
	blends = 1;

	if ( args.Argc() == 4 ) {
		size = atoi( args.Argv( 2 ) );
		blends = atoi( args.Argv( 3 ) );
	} else if ( args.Argc() == 3 ) {
		size = atoi( args.Argv( 2 ) );
		blends = 1;
	} else {
		size = 256;
		blends = 1;
	}
	memset( &axis, 0, sizeof( axis ) );
	axis[0][0][0] = 1;
	axis[0][1][2] = 1;
	axis[0][2][1] = 1;

	axis[1][0][0] = -1;
	axis[1][1][2] = -1;
	axis[1][2][1] = 1;

	axis[2][0][1] = 1;
	axis[2][1][0] = -1;
	axis[2][2][2] = -1;

	axis[3][0][1] = -1;
	axis[3][1][0] = -1;
	axis[3][2][2] = 1;

	axis[4][0][2] = 1;
	axis[4][1][0] = -1;
	axis[4][2][1] = 1;

	axis[5][0][2] = -1;
	axis[5][1][0] = 1;
	axis[5][2][1] = 1;

	for ( int i = 0 ; i < 6 ; i++ ) {
		ref = primary.renderView;
		ref.x = ref.y = 0;
		ref.fov_x = ref.fov_y = 90;
		ref.width = SCREEN_WIDTH;// glConfig.vidWidth;
		ref.height = SCREEN_HEIGHT; //glConfig.vidHeight;
		ref.viewaxis = axis[i];
		sprintf( fullname, "env/%s%s", baseName, GLcubeExtensions[i] );
		tr.TakeScreenshot( size, size, fullname, blends, &ref, true );
	}
	common->Printf( "Wrote %s, etc\n", fullname.c_str() );
}

//============================================================================

/*
==================
R_EnvShot_f

envshot <basename>

Saves out env/<basename>_ft.tga, etc
==================
*/
const static char *cubeExtensions[6] = { "_forward.tga", "_left.tga", "_right.tga", "_back.tga", "_down.tga", "_up.tga" }; // names changed for TDM in #4041

void R_EnvShot_f( const idCmdArgs &args ) {
	idStr fullname;
	const char *baseName;
	idMat3 axis[6];
	renderView_t ref;
	viewDef_t primary;
	int	blends, size;

	if ( !tr.primaryView ) {
		common->Printf( "No primary view.\n" );
		return;
	} else if ( args.Argc() != 2 && args.Argc() != 3 && args.Argc() != 4 ) {
		common->Printf( "USAGE: envshot <basename> [size] [blends]\n" );
		return;
	}
	primary = *tr.primaryView;
	baseName = args.Argv( 1 );

	blends = 1;
	if ( args.Argc() == 4 ) {
		size = atoi( args.Argv( 2 ) );
		blends = atoi( args.Argv( 3 ) );
	} else if ( args.Argc() == 3 ) {
		size = atoi( args.Argv( 2 ) );
		blends = 1;
	} else {
		size = 256;
		blends = 1;
	}
	memset( &axis, 0, sizeof( axis ) );

	// SteveL #4041: these axes were wrong, causing some of the images to be flipped and rotated.
	// forward = east (positive x-axis in DR)
	axis[0][0][0] = 1;
	axis[0][1][1] = 1;
	axis[0][2][2] = 1;
	// left = north
	axis[1][0][1] = 1;
	axis[1][1][0] = -1;
	axis[1][2][2] = 1;
	// right = south
	axis[2][0][1] = -1;
	axis[2][1][0] = 1;
	axis[2][2][2] = 1;
	// back = west
	axis[3][0][0] = -1;
	axis[3][1][1] = -1;
	axis[3][2][2] = 1;
	// down, while facing forward
	axis[4][0][2] = -1;
	axis[4][1][1] = 1;
	axis[4][2][0] = 1;
	// up, while facing forward
	axis[5][0][2] = 1;
	axis[5][1][1] = 1;
	axis[5][2][0] = -1;

	for ( int i = 0 ; i < 6 ; i++ ) {
		ref = primary.renderView;
		ref.x = ref.y = 0;
		ref.fov_x = ref.fov_y = 90;
		ref.width = SCREEN_WIDTH;// glConfig.vidWidth;
		ref.height = SCREEN_HEIGHT;// glConfig.vidHeight;
		ref.viewaxis = axis[i];
		sprintf( fullname, "env/%s%s", baseName, cubeExtensions[i] );
		tr.TakeScreenshot( size, size, fullname, blends, &ref, true );
	}
	common->Printf( "Wrote %s, etc\n", fullname.c_str() );
}

//============================================================================


/*
==================
R_MakeAmbientMap_f

R_MakeAmbientMap_f <basename> [size]

Saves out env/<basename>_amb_ft.tga, etc
==================
*/
void R_MakeAmbientMap_f( const idCmdArgs &args ) {
	MakeAmbientMapParam param;
	idStr		fullname;
	const char	*baseName;

	if ( args.Argc() < 2 || args.Argc() > 6 ) {
		common->Printf( "USAGE: MakeAmbientMap <basename> [size [sample_count [crutch_up [specular?]]]]\n" );
		return;
	}
	baseName = args.Argv( 1 );

	byte* facepalm[6];
	param.buffers = facepalm;

	param.outSize = 32;
	if ( args.Argc() > 2 ) {
		param.outSize = atoi( args.Argv( 2 ) );
	}
	param.samples = 1000;
	if ( args.Argc() > 3 ) {
		param.samples = atoi( args.Argv( 3 ) );
	}
	param.crutchUp = 1;
	if ( args.Argc() > 4 ) {
		param.crutchUp = atoi( args.Argv( 4 ) );
	}
	int	specular = 1;
	if ( args.Argc() > 5 ) {
		specular = atoi( args.Argv( 5 ) );
	}

	// read all of the images
	for ( int i = 0 ; i < 6 ; i++ ) {
		sprintf( fullname, "env/%s%s", baseName, cubeExtensions[i] );
		common->Printf( "loading %s\n", fullname.c_str() );
		session->UpdateScreen();
		R_LoadImage( fullname, &param.buffers[i], &param.size, &param.size, NULL, true );
		if ( !param.buffers[i] ) {
			common->Printf( "failed.\n" );
			for ( i-- ; i >= 0 ; i-- ) {
				Mem_Free( param.buffers[i] );
			}
			return;
		}
	}
	param.outBuffer = ( byte* )R_StaticAlloc( 4 * param.outSize * param.outSize );

	for ( param.specular = false;; ) {
		common->Printf( !param.specular ? "Ambient (1/2)\n" : "Specular (2/2)\n" );
		session->UpdateScreen();

		for ( param.side = 0; param.side < 6; param.side++ ) {
			sprintf( fullname, param.specular ? "env/%s_spec%s" : "env/%s_amb%s", baseName, cubeExtensions[param.side] );
			common->Printf( "%d/6: %s\n", param.side + 1, fullname.c_str() );
			session->UpdateScreen();

			// resample with hemispherical blending
			R_MakeAmbientMap( param );

			common->Printf( "Writing out...\n" );
			session->UpdateScreen();
			R_WriteTGA( fullname, param.outBuffer, param.outSize, param.outSize );
		}

		if ( !param.specular && specular ) { // TODO move to the loop operator above
			param.specular = true;
		} else {
			break;
		}
	}
	R_StaticFree( param.outBuffer );

	for ( int f = 0 ; f < 6 ; f++ ) {
		if ( param.buffers[f] ) {
			Mem_Free( param.buffers[f] );
		}
	}
	session->UpdateScreen();
}

//============================================================================

/*
===============
R_SetColorMappings
===============
*/
void R_SetColorMappings( void ) {
	int		j;
	float	g, b;
	int		inf;

	b = r_brightness.GetFloat();
	g = 1;// r_gamma.GetFloat();

	for ( int i = 0; i < 256; i++ ) {
		j = i * b;

		if ( j > 255 ) {
			j = 255;
		}

		if ( g == 1 ) {
			inf = ( j << 8 ) | j;
		} else {
			inf = 0xffff * pow( j / 255.0f, 1.0f / g ) + 0.5f;
		}

		if ( inf < 0 ) {
			inf = 0;
		}

		if ( inf > 0xffff ) {
			inf = 0xffff;
		}
		tr.gammaTable[i] = inf;
	}
	GLimp_SetGamma( tr.gammaTable, tr.gammaTable, tr.gammaTable );
}


/*
================
GfxInfo_f
================
*/
static void GfxInfo_f( const idCmdArgs &args ) {
	const char *fsstrings[] = {
		"windowed",
		"fullscreen"
	};

	common->Printf( "\nGL_VENDOR: %s\n", glConfig.vendor_string );
	common->Printf( "GL_RENDERER: %s\n", glConfig.renderer_string );
	common->Printf( "GL_VERSION: %s\n", glConfig.version_string );
	common->Printf( "GL_EXTENSIONS: %s\n", glConfig.extensions_string );
	if ( glConfig.wgl_extensions_string ) {
		common->Printf( "WGL_EXTENSIONS: %s\n", glConfig.wgl_extensions_string );
	}
	common->Printf( "GL_MAX_TEXTURE_SIZE: %d\n", glConfig.maxTextureSize );
	common->Printf( "GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS: %d\n", glConfig.maxTextures );
	common->Printf( "GL_MAX_TEXTURE_COORDS_ARB: %d\n", glConfig.maxTextureCoords );
	common->Printf( "\nPIXELFORMAT: color(%d-bits) Z(%d-bit) stencil(%d-bits)\n", glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits );
	common->Printf( "MODE: %d, %d x %d %s hz:", r_mode.GetInteger(), glConfig.vidWidth, glConfig.vidHeight, fsstrings[r_fullscreen.GetBool()] );

	if ( glConfig.displayFrequency ) {
		common->Printf( "%d\n", glConfig.displayFrequency );
	} else {
		common->Printf( "N/A\n" );
	}
	common->Printf( "CPU: %s\n", Sys_GetProcessorString() );

	common->Printf( "ARB2 path ENABLED\n" );

	//=============================

	if ( r_finish.GetBool() ) {
		common->Printf( "Forcing glFinish\n" );
	} else {
		common->Printf( "glFinish not forced\n" );
	}

#ifdef _WIN32
	// WGL_EXT_swap_interval
	typedef BOOL ( WINAPI * PFNWGLSWAPINTERVALEXTPROC )( int interval );
	extern	PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;

	if ( r_swapInterval.GetInteger() && wglSwapIntervalEXT ) {
		common->Printf( "swapInterval forced (%i)\n", r_swapInterval.GetInteger() );
	} else {
		common->Printf( "swapInterval not forced\n" );
	}
#endif

	if ( !r_useTwoSidedStencil.GetBool() && glConfig.twoSidedStencilAvailable ) {
		common->Printf( "Two sided stencil available but disabled\n" );
	} else if ( glConfig.twoSidedStencilAvailable ) {
		common->Printf( "Two sided stencil available and enabled\n" );
	} else if ( !glConfig.twoSidedStencilAvailable ) {
		common->Printf( "Two sided stencil not available\n" );
	}
}

/*
=================
R_VidRestart_f
=================
*/
void R_VidRestart_f( const idCmdArgs &args ) {

	// if OpenGL isn't started, do nothing
	if ( !glConfig.isInitialized ) {
		return;
	}
	bool full = true;
	bool forceWindow = false;

	for ( int i = 1 ; i < args.Argc() ; i++ ) {
		if ( idStr::Icmp( args.Argv( i ), "partial" ) == 0 ) {
			full = false;
			continue;
		}
		if ( idStr::Icmp( args.Argv( i ), "windowed" ) == 0 ) {
			forceWindow = true;
			continue;
		}
	}

	// this could take a while, so give them the cursor back ASAP
	Sys_GrabMouseCursor( false );

	// dump ambient caches
	renderModelManager->FreeModelVertexCaches();

	// free any current world interaction surfaces and vertex caches
	R_FreeDerivedData();

	// make sure the defered frees are actually freed
	R_ToggleSmpFrame();
	R_ToggleSmpFrame();

	// free the vertex caches so they will be regenerated again
	vertexCache.PurgeAll();

	// sound and input are tied to the window we are about to destroy
	if ( full ) {
		// free all of our texture numbers
		soundSystem->ShutdownHW();
		Sys_ShutdownInput();
		globalImages->PurgeAllImages();
		// free the context and close the window
		session->TerminateFrontendThread();
		vertexCache.Shutdown();
		GLimp_Shutdown();
		glConfig.isInitialized = false;

		// create the new context and vertex cache
		bool latch = cvarSystem->GetCVarBool( "r_fullscreen" );
		if ( forceWindow ) {
			cvarSystem->SetCVarBool( "r_fullscreen", false );
		}
		R_InitOpenGL();
		cvarSystem->SetCVarBool( "r_fullscreen", latch );

		// regenerate all images
		globalImages->ReloadAllImages();
		session->StartFrontendThread();
	} else {
		glimpParms_t	parms;
		R_GetModeInfo( &glConfig.vidWidth, &glConfig.vidHeight, r_mode.GetInteger() );
		parms.width = glConfig.vidWidth;
		parms.height = glConfig.vidHeight;
		parms.fullScreen = ( forceWindow ) ? false : r_fullscreen.GetBool();
		parms.displayHz = r_displayRefresh.GetInteger();
		if ( !r_useFbo.GetBool() ) {
			parms.multiSamples = r_multiSamples.GetInteger();
		} else {
			parms.multiSamples = 0;
		}
		parms.stereo = false;
		GLimp_SetScreenParms( parms );
	}

	// make sure the regeneration doesn't use anything no longer valid
	tr.viewCount++;
	tr.viewDef = NULL;

	// regenerate all necessary interactions
	R_RegenerateWorld_f( idCmdArgs() );

	// check for problems
	// use the builtin function instead revelator.
	GL_CheckErrors();

	// start sound playing again
	soundSystem->SetMute( false );

	if ( game != NULL ) {
		game->OnVidRestart();
	}
}


/*
=================
R_InitMaterials
=================
*/
void R_InitMaterials( void ) {
	tr.defaultMaterial = declManager->FindMaterial( "_default", false );
	if ( !tr.defaultMaterial ) {
		common->FatalError( "_default material not found" );
	}
	declManager->FindMaterial( "_default", false );

	tr.defaultShaderPoint = declManager->FindMaterial( "lights/defaultPointLight" );
	tr.defaultShaderProj  = declManager->FindMaterial( "lights/defaultProjectedLight" );
}


/*
=================
R_SizeUp_f

Keybinding command
=================
*/
static void R_SizeUp_f( const idCmdArgs &args ) {
	if ( r_screenFraction.GetInteger() + 10 > 100 ) {
		r_screenFraction.SetInteger( 100 );
	} else {
		r_screenFraction.SetInteger( r_screenFraction.GetInteger() + 10 );
	}
}


/*
=================
R_SizeDown_f

Keybinding command
=================
*/
static void R_SizeDown_f( const idCmdArgs &args ) {
	if ( r_screenFraction.GetInteger() - 10 < 10 ) {
		r_screenFraction.SetInteger( 10 );
	} else {
		r_screenFraction.SetInteger( r_screenFraction.GetInteger() - 10 );
	}
}

/*
===============
TouchGui_f

  this is called from the main thread
===============
*/
void R_TouchGui_f( const idCmdArgs &args ) {
	const char	*gui = args.Argv( 1 );

	if ( !gui[0] ) {
		common->Printf( "USAGE: touchGui <guiName>\n" );
		return;
	}
	common->Printf( "touchGui %s\n", gui );
	session->UpdateScreen();
	uiManager->Touch( gui );
}

/*
=================
R_InitCvars
=================
*/
void R_InitCvars( void ) {
	// update latched cvars here

}

/*
=================
R_InitCommands
=================
*/
void R_InitCommands( void ) {
	cmdSystem->AddCommand( "MakeMegaTexture", idMegaTexture::MakeMegaTexture_f, CMD_FL_RENDERER | CMD_FL_CHEAT, "processes giant images" );
	cmdSystem->AddCommand( "sizeUp", R_SizeUp_f, CMD_FL_RENDERER, "makes the rendered view larger" );
	cmdSystem->AddCommand( "sizeDown", R_SizeDown_f, CMD_FL_RENDERER, "makes the rendered view smaller" );
	cmdSystem->AddCommand( "reloadGuis", R_ReloadGuis_f, CMD_FL_RENDERER, "reloads guis" );
	cmdSystem->AddCommand( "listGuis", R_ListGuis_f, CMD_FL_RENDERER, "lists guis" );
	cmdSystem->AddCommand( "touchGui", R_TouchGui_f, CMD_FL_RENDERER, "touches a gui" );
	cmdSystem->AddCommand( "screenshot", R_ScreenShot_f, CMD_FL_RENDERER, "takes a screenshot" );
	cmdSystem->AddCommand( "envshot", R_EnvShot_f, CMD_FL_RENDERER, "takes an environment shot" );
	cmdSystem->AddCommand( "envshotGL", R_EnvShotGL_f, CMD_FL_RENDERER, "takes an environment shot in opengl orientation" ); // nbohr1more #4041: add envshotGL for cubicLight
	cmdSystem->AddCommand( "makeAmbientMap", R_MakeAmbientMap_f, CMD_FL_RENDERER | CMD_FL_CHEAT, "makes an ambient map" );
	cmdSystem->AddCommand( "benchmark", R_Benchmark_f, CMD_FL_RENDERER, "benchmark" );
	cmdSystem->AddCommand( "gfxInfo", GfxInfo_f, CMD_FL_RENDERER, "show graphics info" );
	cmdSystem->AddCommand( "modulateLights", R_ModulateLights_f, CMD_FL_RENDERER | CMD_FL_CHEAT, "modifies shader parms on all lights" );
	cmdSystem->AddCommand( "testImage", R_TestImage_f, CMD_FL_RENDERER | CMD_FL_CHEAT, "displays the given image centered on screen", idCmdSystem::ArgCompletion_ImageName );
	cmdSystem->AddCommand( "testVideo", R_TestVideo_f, CMD_FL_RENDERER | CMD_FL_CHEAT, "displays the given cinematic", idCmdSystem::ArgCompletion_VideoName );
	cmdSystem->AddCommand( "reportSurfaceAreas", R_ReportSurfaceAreas_f, CMD_FL_RENDERER, "lists all used materials sorted by surface area" );
	cmdSystem->AddCommand( "reportImageDuplication", R_ReportImageDuplication_f, CMD_FL_RENDERER, "checks all referenced images for duplications" );
	cmdSystem->AddCommand( "regenerateWorld", R_RegenerateWorld_f, CMD_FL_RENDERER, "regenerates all interactions" );
	cmdSystem->AddCommand( "showInteractionMemory", R_ShowInteractionMemory_f, CMD_FL_RENDERER, "shows memory used by interactions" );
	cmdSystem->AddCommand( "showTriSurfMemory", R_ShowTriSurfMemory_f, CMD_FL_RENDERER, "shows memory used by triangle surfaces" );
	cmdSystem->AddCommand( "vid_restart", R_VidRestart_f, CMD_FL_RENDERER, "restarts renderSystem" );
	cmdSystem->AddCommand( "listRenderEntityDefs", R_ListRenderEntityDefs_f, CMD_FL_RENDERER, "lists the entity defs" );
	cmdSystem->AddCommand( "listRenderLightDefs", R_ListRenderLightDefs_f, CMD_FL_RENDERER, "lists the light defs" );
	cmdSystem->AddCommand( "listModes", R_ListModes_f, CMD_FL_RENDERER, "lists all video modes" );
	cmdSystem->AddCommand( "reloadSurface", R_ReloadSurface_f, CMD_FL_RENDERER, "reloads the decl and images for selected surface" );
}

/*
===============
idRenderSystemLocal::Clear
===============
*/
void idRenderSystemLocal::Clear( void ) {
	registered = false;
	frameCount = 0;
	viewCount = 0;
	staticAllocCount = 0;
	frameShaderTime = 0.0f;
	viewportOffset[0] = 0;
	viewportOffset[1] = 0;
	tiledViewport[0] = 0;
	tiledViewport[1] = 0;
	ambientLightVector.Zero();
	sortOffset = 0;
	worlds.Clear();
	primaryWorld = NULL;
	memset( &primaryRenderView, 0, sizeof( primaryRenderView ) );
	primaryView = NULL;
	defaultMaterial = NULL;
	testImage = NULL;
	ambientCubeImage = NULL;
	viewDef = NULL;
	memset( &pc, 0, sizeof( pc ) );
	memset( &lockSurfacesCmd, 0, sizeof( lockSurfacesCmd ) );
	memset( &identitySpace, 0, sizeof( identitySpace ) );
	logFile = NULL;
	stencilIncr = 0;
	stencilDecr = 0;
	memset( renderCrops, 0, sizeof( renderCrops ) );
	currentRenderCrop = 0;
	guiRecursionLevel = 0;
	guiModel = NULL;
	demoGuiModel = NULL;
	memset( gammaTable, 0, sizeof( gammaTable ) );
	takingScreenshot = false;
	// duzenko #4425 reset fbo
	FB_Clear();
}

/*
===============
idRenderSystemLocal::Init
===============
*/
void idRenderSystemLocal::Init( void ) {
	r_swapIntervalTemp.SetModified();

	// clear all our internal state
	viewCount = 1;		// so cleared structures never match viewCount
						// we used to memset tr, but now that it is a class, we can't, so
						// there may be other state we need to reset

	ambientLightVector[0] = 0.5f;
	ambientLightVector[1] = 0.5f - 0.385f;
	ambientLightVector[2] = 0.8925f;
	ambientLightVector[3] = 1.0f;

	memset( &backEnd, 0, sizeof( backEnd ) );

	R_InitCvars();

	R_InitCommands();

	guiModel = new idGuiModel;
	guiModel->Clear();

	demoGuiModel = new idGuiModel;
	demoGuiModel->Clear();

	R_InitTriSurfData();

	globalImages->Init();

	idCinematic::InitCinematic( );

	// build brightness translation tables
	R_SetColorMappings();

	R_InitMaterials();

	renderModelManager->Init();

	// set the identity space
	identitySpace.modelMatrix[0 * 4 + 0] = 1.0f;
	identitySpace.modelMatrix[1 * 4 + 1] = 1.0f;
	identitySpace.modelMatrix[2 * 4 + 2] = 1.0f;
}

/*
===============
idRenderSystemLocal::Shutdown
===============
*/
void idRenderSystemLocal::Shutdown( void ) {
	common->Printf( "idRenderSystem::Shutdown()\n" );
	R_DoneFreeType( );

	if ( glConfig.isInitialized ) {
		globalImages->PurgeAllImages();
	}
	renderModelManager->Shutdown();

	idCinematic::ShutdownCinematic( );

	globalImages->Shutdown();

	// close the r_logFile
	if ( logFile ) {
		fprintf( logFile, "*** CLOSING LOG ***\n" );
		fclose( logFile );
		logFile = 0;
	}

	// free frame memory
	R_ShutdownFrameData();

	// free the vertex cache, which should have nothing allocated now
	vertexCache.Shutdown();

	R_ShutdownTriSurfData();

	RB_ShutdownDebugTools();

	delete guiModel;
	delete demoGuiModel;

	Clear();

	ShutdownOpenGL();
}

/*
========================
idRenderSystemLocal::BeginLevelLoad
========================
*/
void idRenderSystemLocal::BeginLevelLoad( void ) {
	renderModelManager->BeginLevelLoad();
	globalImages->BeginLevelLoad();
}

/*
========================
idRenderSystemLocal::EndLevelLoad
========================
*/
void idRenderSystemLocal::EndLevelLoad( void ) {
	renderModelManager->EndLevelLoad();
	globalImages->EndLevelLoad();
	if ( r_forceLoadImages.GetBool() ) {
		RB_ShowImages();
	}
	common->Printf( "----------------------------------------\n" );
}

/*
========================
idRenderSystemLocal::InitOpenGL
========================
*/
void idRenderSystemLocal::InitOpenGL( void ) {
	// if OpenGL isn't started, start it now
	if ( !glConfig.isInitialized ) {
		R_InitOpenGL();
		globalImages->ReloadAllImages();
		GL_CheckErrors(); // use the existing internal function instead: revelator.
	}
}

/*
========================
idRenderSystemLocal::ShutdownOpenGL
========================
*/
void idRenderSystemLocal::ShutdownOpenGL( void ) {
	// free the context and close the window
	R_ShutdownFrameData();
	GLimp_Shutdown();
	glConfig.isInitialized = false;
}

/*
========================
idRenderSystemLocal::IsOpenGLRunning
========================
*/
bool idRenderSystemLocal::IsOpenGLRunning( void ) const {
	if ( !glConfig.isInitialized ) {
		return false;
	}
	return true;
}

/*
========================
idRenderSystemLocal::IsFullScreen
========================
*/
bool idRenderSystemLocal::IsFullScreen( void ) const {
	return glConfig.isFullscreen;
}

/*
========================
idRenderSystemLocal::GetScreenWidth
========================
*/
int idRenderSystemLocal::GetScreenWidth( void ) const {
	return glConfig.vidWidth;
}

/*
========================
idRenderSystemLocal::GetScreenHeight
========================
*/
int idRenderSystemLocal::GetScreenHeight( void ) const {
	return glConfig.vidHeight;
}
