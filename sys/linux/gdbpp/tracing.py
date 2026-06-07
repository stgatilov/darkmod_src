import gdb
import sys, inspect

COMMON_PATH = 'sys/linux/gdbpp'
tracefile = open('trace.txt', 'w')

tablevel = 0


def traceprint(message):
    print(message, file = tracefile)


def formatvalue(value):
    if isinstance(value, str) or isinstance(value, int) or isinstance(value, float) or isinstance(value, bool) or value is None:
        return str(value)
    if isinstance(value, gdb.Value):
        vtype = value.type
        typename = vtype.name
        if isinstance(typename, str):
            addr = value.address
            addr = 'none' if addr is None else '%x' % int(addr)
            return 'VT:' + typename + "|" + addr
        return 'VT:' + str(type)
    if hasattr(value, 'name'):
        name = value.name
        if isinstance(name, str):
            return 'N:' + name
    if isinstance(value, list):
        return 'list[%d]' % len(value)
    if isinstance(value, dict):
        return 'dict[%d]' % len(value)
    if inspect.isclass(value):
        return 'C:' + value.__name__
    return 'O:' + type(value).__name__


def printpoint(event, frame, args = []):
    filename = '[nowhere]'
    funcname = '[somefunc]'
    lineno = '000'

    if frame:
        if frame.f_code:
            filename = frame.f_code.co_filename
            funcname = frame.f_code.co_qualname
        lineno = frame.f_lineno

    common_pos = filename.find(COMMON_PATH)
    if common_pos >= 0:
        common_pos += len(COMMON_PATH)
        filename = filename[common_pos:]

    args_end = ''
    if args:
        arg_strs = [formatvalue(v) for v in args]
        args_end = ' ( ' + ', '.join(arg_strs) + ' )'

    traceprint('%s%s %s:%s %s%s' % ('  ' * tablevel, event, filename, lineno, funcname, args_end))


def trace(frame, event, arg):
    if COMMON_PATH not in frame.f_code.co_filename:
        return trace
    
    global tablevel

    if event == 'call':
        printpoint('callfrom', frame.f_back)
        tablevel += 1

        nargs = frame.f_code.co_argcount
        args = [frame.f_locals[frame.f_code.co_varnames[i]] for i in range(nargs)]
        printpoint('call', frame, args)

    if event in ['exception']:
        printpoint(event, frame)

    if event == 'return':
        printpoint('return', frame, [arg])

        tablevel -= 1
        printpoint('returnto', frame.f_back)
        
    return trace


sys.settrace(trace)
