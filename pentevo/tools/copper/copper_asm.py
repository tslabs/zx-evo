import sys
import re
from ast import parse, BinOp, Name, Constant, Add, Sub, Mult, Div, Mod, Pow, BitOr, BitAnd, BitXor, LShift, RShift

def parse_number(s):
    if s.startswith('0x'):
        return int(s, 16)
    elif s.startswith('0b'):
        return int(s, 2)
    else:
        return int(s)

def evaluate_operand(op, labels, predefined, pc):
    op = op.strip()
    try:
        return parse_number(op)
    except ValueError:
        if op in labels:
            return labels[op]
        if op in predefined:
            return predefined[op]
        try:
            # Replace |, &, ^ with Python-compatible operators for parsing
            op = re.sub(r'\|', '|', op)
            op = re.sub(r'&', '&', op)
            op = re.sub(r'\^', '^', op)
            tree = parse(op, mode='eval')
            return evaluate_expression(tree.body, labels, predefined)
        except:
            raise ValueError(f"Invalid operand: {op}")

def evaluate_expression(node, labels, predefined):
    if isinstance(node, Constant):
        return node.value
    elif isinstance(node, Name):
        if node.id in labels:
            return labels[node.id]
        if node.id in predefined:
            return predefined[node.id]
        raise ValueError(f"Unknown identifier: {node.id}")
    elif isinstance(node, BinOp):
        left = evaluate_expression(node.left, labels, predefined)
        right = evaluate_expression(node.right, labels, predefined)
        if isinstance(node.op, Add):
            return left + right
        elif isinstance(node.op, Sub):
            return left - right
        elif isinstance(node.op, Mult):
            return left * right
        elif isinstance(node.op, Div):
            return left // right  # Integer division
        elif isinstance(node.op, Mod):
            return left % right
        elif isinstance(node.op, Pow):
            return left ** right
        elif isinstance(node.op, BitOr):
            return left | right
        elif isinstance(node.op, BitAnd):
            return left & right
        elif isinstance(node.op, BitXor):
            return left ^ right
        elif isinstance(node.op, LShift):
            return left << right
        elif isinstance(node.op, RShift):
            return left >> right
    raise ValueError("Unsupported expression")

def main():
    if len(sys.argv) != 3:
        print("Usage: assembler.py <input> <output>")
        sys.exit(1)
    
    input_file, output_file = sys.argv[1], sys.argv[2]
    
    instructions = {
        'WREG': (0x00, 2, 8, 8),
        'WAIT': (0xFBF, 1, 4),  
        'SIGNAL': (0xF5, 1, 4),
        'SETA': (0xF6, 1, 8),
        'WAITX': (0xF7, 1, 8),
        'SETB': (0xF8, 1, 9),
        'WAITY': (0xFA, 1, 9),
        'DJNZA': (0xFC, 1, 8),
        'DJNZB': (0xFD, 1, 8),
        'CALL': (0xFE, 1, 8),
        'JUMP': (0xFF, 1, 8),
        'RET': (0xFF, 0, 8),  # Pseudo-instruction, alias for JUMP 0xFF
        'NOP': (0xFBF, 0, 4), # Pseudo-instruction, alias for WAIT EVT_NONE
        'HALT': (0xFF, 0, 8)  # Pseudo-instruction, alias for JUMP to current PC
    }
    
    # Predefined constants
    predefined = {
        # Events
        'EVT_NONE': 0,
        'EVT_DMA': 1,
        'EVT_XY0': 2,
        'EVT_X0': 3,
        
        # Signals
        'SIG_INT': 0b0001,
        'SIG_RDY': 0b0010,

        # TS-conf regs
        'VCONFIG': 0x00,
        'STATUS': 0x00,
        'VPAGE': 0x01,
        'GXOFFSL': 0x02,
        'GXOFFSH': 0x03,
        'GYOFFSL': 0x04,
        'GYOFFSH': 0x05,
        'TSCONFIG': 0x06,
        'PALSEL': 0x07,
        'BORDER': 0x0F,
        'PAGE0': 0x10,
        'PAGE1': 0x11,
        'PAGE2': 0x12,
        'PAGE3': 0x13,
        'FMADDR': 0x15,
        'TMPAGE': 0x16,
        'T0GPAGE': 0x17,
        'T1GPAGE': 0x18,
        'SGPAGE': 0x19,
        'DMASADDRL': 0x1A,
        'DMASADDRH': 0x1B,
        'DMASADDRX': 0x1C,
        'DMADADDRL': 0x1D,
        'DMADADDRH': 0x1E,
        'DMADADDRX': 0x1F,
        'SYSCONFIG': 0x20,
        'MEMCONFIG': 0x21,
        'HSINT': 0x22,
        'VSINTL': 0x23,
        'VSINTH': 0x24,
        'DMAWPD': 0x25,
        'DMALEN': 0x26,
        'DMACTR': 0x27,
        'DMASTATUS': 0x27,
        'DMANUM': 0x28,
        'FDDVIRT': 0x29,
        'INTMASK': 0x2A,
        'CACHECONF': 0x2B,
        'DMANUMH': 0x2C,
        'DMAWPA': 0x2D,
        'T0XOFFSL': 0x40,
        'T0XOFFSH': 0x41,
        'T0YOFFSL': 0x42,
        'T0YOFFSH': 0x43,
        'T1XOFFSL': 0x44,
        'T1XOFFSH': 0x45,
        'T1YOFFSL': 0x46,
        'T1YOFFSH': 0x47,
        
        # TS-Conf parameters
        
        # FPGA arrays
        'FM_EN': 0x10,
        'FM_CRAM': 0x0000,
        'FM_SFILE': 0x0200,
        'FM_CLIST': 0x0600,
        
        # VIDEO
        'VID_256X192': 0x00,
        'VID_320X200': 0x40,
        'VID_320X240': 0x80,
        'VID_360X288': 0xC0,
        'VID_RASTER_BS': 6,
        'VID_ZX': 0x00,
        'VID_16C': 0x01,
        'VID_256C': 0x02,
        'VID_TEXT': 0x03,
        'VID_FT812': 0x04,
        'VID_NOGFX': 0x20,
        'VID_MODE_BS': 0,
        
        # PALSEL
        'PAL_GPAL_MASK': 0x0F,
        'PAL_GPAL_BS': 0,
        'PAL_T0PAL_MASK': 0x30,
        'PAL_T0PAL_BS': 4,
        'PAL_T1PAL_MASK': 0xC0,
        'PAL_T1PAL_BS': 6,
        
        #TSU
        'TSU_T0ZEN': 0x04,
        'TSU_T1ZEN': 0x08,
        'TSU_T0EN': 0x20,
        'TSU_T1EN': 0x40,
        'TSU_SEN': 0x80,
        
        # SYSTEM
        'SYS_ZCLK3_5': 0x00,
        'SYS_ZCLK7': 0x01,
        'SYS_ZCLK14': 0x02,
        'SYS_ZCLK_BS': 0,
        'SYS_CACHEEN': 0x04,
        
        # MEMORY
        'MEM_ROM128': 0x01,
        'MEM_W0WE': 0x02,
        'MEM_W0MAP_N': 0x04,
        'MEM_W0RAM': 0x08,
        'MEM_LCK512': 0x00,
        'MEM_LCK128': 0x40,
        'MEM_LCKAUTO': 0x80,
        'MEM_LCK1024': 0xC0,
        'MEM_LCK_BS': 6,
        
        # INT
        'INT_VEC_FRAME': 0xFF,
        'INT_VEC_LINE': 0xFD,
        'INT_VEC_DMA': 0xFB,
        'INT_VEC_WTP': 0xF9,
        'INT_MSK_FRAME': 0x01,
        'INT_MSK_LINE': 0x02,
        'INT_MSK_DMA': 0x04,
        'INT_MSK_WTP': 0x08,
        
        # DMA
        'DMA_WNR': 0x80,
        'DMA_SALGN': 0x20,
        'DMA_DALGN': 0x10,
        'DMA_ASZ': 0x08,
        'DMA_RAM': 0x01,
        'DMA_BLT': 0x81,
        'DMA_FILL': 0x04,
        'DMA_SPI_RAM': 0x02,
        'DMA_WTP_RAM': 0x07,
        'DMA_RAM_SPI': 0x82,
        'DMA_IDE_RAM': 0x03,
        'DMA_RAM_IDE': 0x83,
        'DMA_RAM_CRAM': 0x84,
        'DMA_RAM_SFILE': 0x85,
        'WPD_GLU': 0,
        'WPD_COM': 1
    }
    
    labels = {}
    program = []
    pc = 0
    source_lines = {}
    
    with open(input_file, 'r') as f:
        lines = f.readlines()
    
    # First pass: collect labels and definitions
    for line in lines:
        line = line.strip()
        if not line or line.startswith('#'):
            continue
        if line.endswith(':'):
            labels[line[:-1]] = pc
            continue
        if ':' in line and not line.startswith('org'):
            label, value = [x.strip() for x in line.split(':')]
            labels[label] = parse_number(value)
            continue
        if line.startswith('org'):
            pc = parse_number(line.split()[1])
            continue
        source_lines[pc] = line
        pc += 1
    
    # Second pass: assemble
    pc = 0
    for line in lines:
        line = line.strip()
        if not line or line.startswith('#') or line.endswith(':') or ':' in line:
            continue
        if line.startswith('org'):
            pc = parse_number(line.split()[1])
            continue
            
        parts = line.split(maxsplit=1)
        if not parts:
            continue
            
        inst = parts[0].upper()
        if inst not in instructions:
            print(f"Unknown instruction: {inst}")
            sys.exit(1)
            
        cmd_code, num_ops, *op_bits = instructions[inst]
        
        ops = parts[1].split(',') if len(parts) > 1 else []
        ops = [op.strip() for op in ops]
        
        if inst == 'RET':
            if ops:
                print("RET takes no operands")
                sys.exit(1)
            ops = ['0xFF']  # Treat RET as JUMP 0xFF
            num_ops = 1
        elif inst == 'NOP':
            if ops:
                print("NOP takes no operands")
                sys.exit(1)
            ops = ['0']  # Treat NOP as WAIT EVT_NONE
            num_ops = 1
        elif inst == 'HALT':
            if ops:
                print("HALT takes no operands")
                sys.exit(1)
            ops = [str(pc)]  # Treat HALT as JUMP to current PC
            num_ops = 1
        
        if len(ops) != num_ops:
            print(f"Invalid number of operands for {inst}")
            sys.exit(1)
            
        word = cmd_code << 4 if inst == 'WAIT' or inst == 'NOP' else cmd_code << 8
        
        for i, op in enumerate(ops):
            val = evaluate_operand(op, labels, predefined, pc)
            if val is None:
                print(f"Invalid operand: {op}")
                sys.exit(1)
                
            if val.bit_length() > op_bits[i]:
                print(f"Operand {op} too large for {inst}")
                sys.exit(1)
                
            if i == 0:
                if inst == 'WREG':
                    word |= (val & ((1 << op_bits[i]) - 1)) << 8
                else:
                    word |= val & ((1 << op_bits[i]) - 1)
            else:
                word |= val & ((1 << op_bits[i]) - 1)
        
        source_line = 'RET' if inst == 'RET' else 'NOP' if inst == 'NOP' else 'HALT' if inst == 'HALT' else source_lines.get(pc, "")
        program.append((pc, word, source_line))
        pc += 1
    
    # Write output
    with open(output_file, 'w') as f:
        if output_file.endswith('.hex'):
            # Hex text file output
            hex_values = []
            for _, word, _ in program:
                high_byte = (word >> 8) & 0xFF
                low_byte = word & 0xFF
                hex_values.append(f'{low_byte:02X} {high_byte:02X}')
            f.write(' '.join(hex_values) + '\n')
        elif output_file.endswith('.asm'):
            # ASM output (dw 0xXXXX per word with comments, little-endian)
            for addr, word, src in program:
                f.write(f'    dw 0x{word:04X} ; 0x{addr:04X}: {src}\n')
        else:
            # C array output
            f.write('#include <stdint.h>\n\n')
            f.write('const uint16_t program[] = {\n')
            for addr, word, src in program:
                f.write(f'    0x{word:04X}, // 0x{addr:04X}: {src}\n')
            f.write('};\n')
            f.write(f'const uint32_t program_size = {len(program)};\n')

if __name__ == '__main__':
    main()