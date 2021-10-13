#!/bin/env python3

import re
import sys

class Parser:

    class BitFields:
        def __init__(self, name, body):
            self.name = name
            self.byte_fields = body
        def to_c(self):
            str = "// -------- bitfields {} --------\n".format(self.name)
            for l in self.byte_fields:
                str += l.to_c(self.name)
            str += "// -------- struct definitions of {} --------\n".format(self.name)
            entries = []
            for bf in self.byte_fields:
                name = bf.name or ("word%d" % (bf.byte_off))
                entries.append((bf.byte_off, bf.nr_bytes, name))
            entries.sort()
            body_str = ""
            cursor = 0
            for e in entries:
                assert(cursor == e[0])
                if e[1] in [1, 2, 4, 8]:
                    body_str += "\tu%d %s;\n" % (e[1] * 8, e[2])
                else:
                    body_str += "\tu8 %s[%d];\n" % (e[2], e[1])
                cursor += e[1]
            str += '''\
struct %s {
%s
};''' % (self.name, body_str)
            return str

    class ByteField:
        def __init__(self, name, byte_off, nr_bytes, body, comment):
            self.name = name
            self.byte_off = byte_off
            self.nr_bytes = nr_bytes
            self.bit_fields = body
            self.comment = comment
        def to_c(self, bitfields):
            var_name = "%s_%s" % (bitfields,
                    self.name if self.name else ("word%s" % (self.byte_off)))
            fmt = '''{desc}
#define getp_{var_name}({bitfields}) \
  ((volatile {type} *)((char *)({bitfields}) + {byte_off}))
#define read_{var_name}({bitfields}) \
  (*getp_{var_name}({bitfields}))
#define write_{var_name}({bitfields}, v) do {{ \\
  typeof(getp_{var_name}({bitfields})) p = getp_{var_name}({bitfields}); \\
  (*p = (v)); \\
  /* __asm__ __volatile__("dc cvau, %0\\n\\t" : : "r" (p) :"memory"); */ \\
  }} while (0)
'''
            str = fmt.format(bitfields = bitfields,
                    var_name = var_name,
                    byte_off = self.byte_off,
                    nr_bytes = self.nr_bytes,
                    type = "u%d" % (self.nr_bytes * 8),
                    desc = "// {}\n".format(self.comment) if self.comment else "")
            for l in self.bit_fields:
                str += l.to_c(bitfields, var_name, self.name)
            return str

    class BitField:
        def __init__(self, name, perm, bit_off, nr_bits, comment):
            self.name = name
            self.perm = perm
            self.bit_off = bit_off
            self.nr_bits = nr_bits
            self.comment = comment
        def to_c(self, bitfields, byte_var_name, byte_name):
            if byte_name:
                bit_var_name = byte_var_name + "_" + self.name
            else:
                bit_var_name = "%s_%s" % (bitfields, self.name)
            fmt = '''{desc}\
#define read_{bit_var_name}({bitfields}) \\
  (((read_{byte_var_name}({bitfields})) >> {bit_off}) & ((1ULL << {nr_bits}) - 1))

#define write_{bit_var_name}({bitfields}, v) do {{ \\
  typeof(getp_{byte_var_name}({bitfields})) p = getp_{byte_var_name}({bitfields}); \\
  (*p) = ((*p) & ~(((1ULL << {nr_bits}) - 1) << {bit_off})) | (((v) & ((1ULL << {nr_bits}) - 1)) << {bit_off}); \\
  /* __asm__ __volatile__(\"dc cvau, %0\\n\\t\" : : \"r\" (p) :\"memory\"); */ \\
  }} while (0)
'''
            return fmt.format(bitfields = bitfields,
                    byte_var_name = byte_var_name,
                    bit_var_name = bit_var_name,
                    nr_bits = self.nr_bits,
                    bit_off = self.bit_off,
                    desc = "// {}\n".format(self.comment) if self.comment else "")


    def __init__(self, fn):
        with open(fn) as f:
            self.lines = [l.strip() for l in f.readlines()
                # empty line and comment line
                    if len(l.strip()) > 0 and not l.strip().startswith("//")]
            self.defines = [l for l in self.lines if l.startswith("#define ")]
            self.lines = [l for l in self.lines if not l.startswith("#define ")]
        self.source = fn
        self.i = 0
        self.n = len(self.lines)
        self.bitfields = []
        while self.i < self.n:
            l = self.lines[self.i]
            if l.startswith("bitfields"):
                bf = self.parse_bitfields()
                if bf: self.bitfields.append(bf)
                continue
            print(l)
            assert(0)

    def parse_bitfields(self):
        """
        Extract a BitFields from [self.i:].
        self.i should point to the line after BitFields before return.
        """
        tuples = self.lines[self.i].strip().split()
        assert(len(tuples) == 3)
        assert(tuples[0] == "bitfields")
        assert(tuples[2] == "{")
        name = tuples[1]
        body = []
        self.i += 1
        while self.i < self.n:
            l = self.lines[self.i]
            if "=>" in l:
                bf = self.parse_bytefield()
                if bf: body.append(bf)
                continue
            if l == "}":
                self.i += 1
                break
            print(l)
            assert(0)
        return self.BitFields(name, body)

    def parse_bytefield(self):
        """
        Extract a ByteField from [self.i:].
        self.i should point to the next line after ByteField before return.
        """
        tuples = self.lines[self.i].split()
        assert(len(tuples) == 5 or len(tuples) == 4)
        assert("=>" in tuples[3] or "=>" in tuples[2])
        assert(tuples[-1] == "{")
        assert(tuples[0][0] == "[")
        assert(len(tuples[1]) > 0 and tuples[1][-1] == "]")

        assert(tuples[0][-1] == "h")
        byte_off = int(tuples[0][1:-1], 16)
        assert(tuples[1][-2] == "h")
        nr_bytes = int(tuples[1][:-2], 16) - byte_off + 1

        name = tuples[2] if len(tuples) == 5 else None

        body = []
        self.i += 1
        while self.i < self.n:
            l = self.lines[self.i]
            if l.startswith("}"):
                if "//" in l:
                    comment = l[l.index("//") + 2:]
                else:
                    comment = None
                self.i += 1
                break
            bf = self.parse_bitfield()
            if bf: body.append(bf)
        return self.ByteField(name, byte_off, nr_bytes, body, comment)

    def parse_bitfield(self):
        """
        Parse a BitField.
        It only occupies a single line and the self.i will proceed by 1 before return.
        """
        l = self.lines[self.i]
        if "//" in l:
            comment = l[l.index("//") + 2:]
            l = l[:l.index("//")]
        else:
            comment = None
        tuples = l.split()
        assert(len(tuples) == 3)
        if ":" in tuples[0]:
            __tuples = tuples[0].split(":")
            assert(len(__tuples) == 2)
            bit_off = int(__tuples[1])
            nr_bits = int(__tuples[0]) - bit_off + 1
        else:
            bit_off = int(tuples[0])
            nr_bits = 1
        perm = tuples[1]
        name = tuples[2]
        self.i += 1
        if name == "_" or name == "Reserved":
            return None
        return self.BitField(name, perm, bit_off, nr_bits, comment)

    def output(self, fn=None):
        if fn:
            f = open(fn, "w")
        else:
            f = sys.stdout

        header_info = """\
/*-
 * DO NOT MODIFY!
 * The file is generated from {} by the {} script.
 * Please update the source file and run the script again.
 */
""".format(self.source, __file__)

        print(header_info, file=f)
        for l in self.defines:
            print(l, file=f)
        print(file=f)
        for l in self.bitfields:
            print(l.to_c(), file=f)

        if fn:
            f.close()


if __name__ == "__main__":
    Parser("ufs.bitfields").output("ufs-bitfields.gen.h")
