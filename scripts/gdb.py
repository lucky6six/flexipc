import gdb
import io
import re
import subprocess

# Load symbols in elf file to gdb by add-symbol-file
# @filepath: the path to elf file
# @base_addr: address where elf is loaded
def add_symbol_file(filepath, base_addr):
        if type(base_addr) == type(1):
            base_addr = hex(base_addr)

        # Get .text section offset
        proc = subprocess.Popen(['readelf', '-WS', filepath],
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        readelf_result, err = proc.communicate()
        readelf_result = readelf_result.decode("utf-8").split('\n')
        readelf_result = filter(lambda x: x.find(".text") >= 0, readelf_result)
        readelf_result = list(readelf_result)[0]
        readelf_result = readelf_result.replace('[', '').replace(']', '')

        text_offset = filter(lambda x: len(x) > 0, readelf_result.split(' '))
        # Format of `readelf -WS binary`:
        #   [Nr] Name Type Address
        text_offset = '0x' + list(text_offset)[3]
        # print("load lib:", filepath, "  to:", base_addr, "  off:", text_offset)

        r = gdb.execute("add-symbol-file {} {}"
                        .format(filepath, int(base_addr, 16) + int(text_offset, 16)))
        print(r)

class AddSymbolFileOff(gdb.Command):
    """
    Usage: add-symbol-file-off FILEPATH [OFFSET]
    Add a symbol file with given loading offset.
    If OFFSET is not set, use 0 as the default value.
    """

    def __init__(self):
        super(AddSymbolFileOff, self).__init__('add-symbol-file-off', gdb.COMMAND_USER)

    def invoke(self, args, from_tty):
        args = args.split(' ')
        if len(args) == 1:
            filepath = args[0]
            off = 0
        elif len(args) == 2:
            filepath = args[0]
            off = args[1]
        else:
            print("Wrong arguments")
            return
        add_symbol_file(filepath, off)


class DynamicElfLoader(gdb.Command):
    """
    Usage: prepare-load-lib
    Load dynamically loaded elf files to gdb.
    Firstly, invoke `prepare-load-lib` before running an application.
    Secondly, run application in chcore's shell. GDB will break after all libraries are loaded.
    """

    def __init__(self):
        super(DynamicElfLoader, self).__init__('prepare-load-lib', gdb.COMMAND_USER)

    def invoke(self, args, from_tty):
        f = open("./build/exec_log", "r")
        f.seek(0, io.SEEK_END)

        # Set breakpoint to when all libraris are loaded.
        # This helper funtion locates in musl-1.1.24/ldso/dynlink.c.
        r = gdb.execute('hbreak gdb_loadefl_break', to_string=True)
        print(r)

        # Get the breakpoint id and disable it later
        bp_number = re.search("(\d+)", r).group(1)

        gdb.execute('continue')
        gdb.execute('delete ' + bp_number)

        while True:
            line = f.readline()
            # End of loading binaries
            if len(line) == 0:
                return

            # Get library name
            try:
                lib_name = re.search("load library name:(.*)", line).group(1)
            except AttributeError:
                continue
            if lib_name.find('/') >= 0:
                lib_name = lib_name[lib_name.find('/') + 1: len(lib_name)]
            lib_name = lib_name.strip(' ')

            # Get library load address
            while True:
                line = f.readline()
                if len(line) != 0:
                    try:
                        lib_base = re.search("map library base:(.*)", line).group(1)
                        break
                    except AttributeError:
                        continue
            lib_base = lib_base.strip(' ')

            # Find the library
            proc = subprocess.Popen(['find', './user/build/ramdisk', '-name', lib_name],
                                    stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            find_result, err = proc.communicate()
            # Library not found
            if len(find_result) == 0:
                print("lib:", lib_name, "not found")
                continue
            lib_filepath = find_result.decode("utf-8").split('\n')[0]

            add_symbol_file(lib_filepath, lib_base)

        f.close()

# Init gdb connection
gdb_port_file = open("build/gdb-port", "r")
gdb_port = gdb_port_file.readline()
gdb_port_file.close()
r = gdb.execute('target remote localhost:{}'.format(gdb_port), to_string=True)
print(r)

# Add custom commands
DynamicElfLoader()
AddSymbolFileOff()
