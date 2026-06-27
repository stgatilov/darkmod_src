# load GDB pretty-printers for TDM custom types
# note: this file must be in CWD of TDM executable at launch time (CLion)
source ../darkmod_src/sys/linux/gdbpp/main.py
# downcast to most derived runtime type, even the classes which are not covered by pretty printer
set print object on
