import sys, os, string

def BuildList( s_prefix, s_string ):
	s_list = string.split( s_string )
	for i in range( len( s_list ) ):
		s_list[i] = os.path.join(s_prefix, s_list[i])
	return s_list

def ExecutableName(base_name, os, arch):
	if os == 'Linux':
		if arch == 'x86':
			return base_name + '.linux'
		elif arch == 'x64':
			return base_name + '.linux64'
	elif os == 'FreeBSD':
		return base_name + '.fbsd'
	elif os == 'MacOSX':
		return base_name + '.macosx'
	return base_name
