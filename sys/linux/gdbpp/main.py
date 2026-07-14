import traceback, sys, os
import gdb

# GDB executes this file from god-knows-where
# I have no idea how to do a relative import in this case
basedir = os.path.dirname(__file__)
sys.path.append(basedir)

# force reloading all our modules marked as "uncached"
# this allows us to refresh our pretty printers without restarting TDM
uncached_modules_names = []
for modname in sys.modules:
    if modname.startswith('tdmuc_'):
        uncached_modules_names.append(modname)
for modname in uncached_modules_names:
    del sys.modules[modname]

# enable verbose tracing of Python execution into a log file
# this is very useful for debugging, since exceptions are often silently suppressed,
# GDB often hangs trying to evaluate all the stuff, and we don't see well what IDE is doing
#import tracing

try:
    pp_list = []
    import tdmuc_idlib
    import tdmuc_engine
    import tdmuc_game
    pp_list += tdmuc_idlib.get_pretty_printers()
    pp_list += tdmuc_engine.get_pretty_printers()
    pp_list += tdmuc_game.get_pretty_printers()

    import tdmuc_base
    collection = tdmuc_base.TdmPrettyPrinterCollection.create_with_printers('all', pp_list)
    gdb.printing.register_pretty_printer(gdb.current_objfile(), collection, replace = True)

    print("TheDarkMod GDB pretty printers: registration finished")

except Exception:
    print("TheDarkMod GDB pretty printers: failed with exception")
    print(traceback.format_exc())
