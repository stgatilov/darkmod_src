import re
codeprintl = print

class apiset:
	def __init__(self, version_string, extensions, exclude_funcs = []):
		self.version_string = version_string
		self.extensions = extensions
		self.exclude_funcs = exclude_funcs
		tokens = version_string.split('_')
		self.version = tokens[2] + '.' + tokens[3]

def fptype(f):
	return 'PFN' + f.upper() + 'PROC'

def read_gl_header(filename, extract_regex, req):
	functions_list = []
	current_feature = req.version
	with open(filename, "r") as f:
		for line in f:
			if current_feature and re.search(r'#endif.*' + current_feature, line):
				current_feature = None
			m = re.search(r'#define (\w+) 1', line)
			if m and m.group(1) in req.extensions:
				current_feature = m.group(1)
			if current_feature:
				m = re.search(extract_regex, line)
				if m:
					func = m.group(1)
					if any([re.search(pat, func) for pat in req.exclude_funcs]):
						print("Excluded %s" % func)
					else:
						functions_list.append(func)
	return functions_list

requirements = {
	"gl": apiset(
		"GL_VERSION_3_0",
		[
			"GL_EXT_texture_compression_s3tc",
			"GL_ARB_vertex_program",
			"GL_ARB_fragment_program",
			"GL_ARB_draw_instanced",      # core in GL 3.1
		],
	),
	"wgl": apiset(
		"WGL_VERSION_1_0",
		[
			"WGL_ARB_extensions_string",
			"WGL_ARB_create_context",
			"WGL_ARB_create_context_profile",
			"WGL_ARB_pixel_format",
		],
		[        
			r"UseFont",     # do not load functions with "UseFont" in name
		],
	),
	"glx": apiset(
		"GLX_VERSION_1_4",
		[
		],
	),
}


def doall():
	# parse GL headers and generate list of functions
	gl_funcs = read_gl_header('GL/glall.h', r'^GLAPI.*APIENTRY (\w+)', requirements['gl'])
	wgl_funcs = read_gl_header('GL/wgl.h', r'WINAPI (wgl\w+)', requirements['wgl'])
	glx_funcs = read_gl_header('GL/glx.h', r'\W(glX\w+)\W', requirements['glx'])

	# generate macros for required GL version
	codeprintl('#define QGL_REQUIRED_VERSION_MAJOR %s' % requirements['gl'].version.split('.')[0])
	codeprintl('#define QGL_REQUIRED_VERSION_MINOR %s' % requirements['gl'].version.split('.')[1])

	codeprintl()

	# generate function declarations
	for f in gl_funcs:
		codeprintl('QGLFUNC %s q%s;' % (fptype(f), f))
	codeprintl()
	codeprintl('#ifdef _WIN32')
	for f in wgl_funcs:
		codeprintl('QGLFUNC %s q%s;' % (fptype(f), f))
	codeprintl('#endif //_WIN32')
	codeprintl()
	codeprintl('#ifdef __linux__')
	for f in glx_funcs:
		codeprintl('QGLFUNC %s q%s;' % (fptype(f), f))
	codeprintl('#endif //__linux__')

	codeprintl()
	codeprintl()

	# generate initialization preamble
	codeprintl('#ifdef QGL_DEFINITIONS')
	codeprintl('void GLimp_LoadBaseFunctions() {')
	codeprintl('\tstatic const char* RequiredExtensions[] = {%sNULL};' % ''.join(['"'+req+'", ' for req in requirements['gl'].extensions]))
	codeprintl('\tGLimp_LoadPreamble(%s, RequiredExtensions);' % requirements['gl'].version)
	codeprintl()
	# generate functions loading code
	for f in gl_funcs:
		codeprintl('\tGLimp_LoadFunctionPointer(&q%s, "%s");' % (f, f))
	codeprintl()
	codeprintl('#ifdef _WIN32')
	for f in wgl_funcs:
		codeprintl('\tGLimp_LoadFunctionPointer(&q%s, "%s");' % (f, f))
	codeprintl('#endif //_WIN32')
	codeprintl()
	codeprintl('#ifdef __linux__')
	for f in glx_funcs:
		codeprintl('\tGLimp_LoadFunctionPointer(&q%s, "%s");' % (f, f))
	codeprintl('#endif //__linux__')
	codeprintl('}')
	codeprintl('#endif //QGL_DEFINITIONS')


if __name__ == "__main__":
	doall()
