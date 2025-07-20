#######################################################################
# UTG (Unicode Trie Generator) utility was written by Sergey Epishkin,
# and is placed in the public domain.
# The author hereby disclaims copyright to this source code.
#######################################################################

SEARCH = [['L'], ['WS', 'S', 'B']]
TABLENAME = 'alpha'
FILE = 'alpha.c'

memory = []
MAIN = 0
LEAF = 256
DATA = 256 + 256*256
for i in range(256):
    memory.append(LEAF + i*256)
for i in range(256*256):
    memory.append(DATA + i*17)
for i in range(256*256*17):
    memory.append(0)

def code_to_b(b):
    return b&0xFF, (b>>8)&0xFF, (b>>16)&0xFF, (b>>24)&0xFF

print(len(memory))

started_at = None
with open('UnicodeData.txt') as fi:
    for line in fi:
        code, name, __, ___, cat = line.split(';')[:5]
        
        start = None
        if name[0] == '<' and name != '<control>':
            if started_at is None:
                started_at = code
                continue
            else:
                start = int(started_at, 16)
                started_at = None
        else:
            start = int(code, 16)
        end = int(code, 16)
        
        i = 0
        was = False
        for j, srch in enumerate(SEARCH):
            if cat in srch:
                was = True
                i = j
                break
        if not was:
            continue
        
        for cd in range(start, end+1):
            b = code_to_b(cd)
            
            leaf_pos = memory[           b[0]]
            data_pos = memory[leaf_pos + b[1]]
            
            memory[data_pos + b[2]] = 1 << i

input('next')

fails_count = 0
with open('UnicodeData.txt') as fi:
    for line in fi:
        code, name, __, ___, cat = line.split(';')[:5]
        
        start = None
        if name[0] == '<' and name != '<control>':
            if started_at is None:
                started_at = code
                continue
            else:
                start = int(started_at, 16)
                started_at = None
        else:
            start = int(code, 16)
        end = int(code, 16)
        
        i = 0
        was = False
        for j, srch in enumerate(SEARCH):
            if cat in srch:
                was = True
                i = j
                break

        for cd in range(start, end+1):
            b = code_to_b(cd)
            
            leaf_pos = memory[           b[0]]
            data_pos = memory[leaf_pos + b[1]]
            
            if was:
                if (memory[data_pos + b[2]] & (1 << i)) == 0:
                    print(f'F fail {cat}')
                    fails_count += 1
                else:
                    print(f'G good {cat}')
            else:
                if memory[data_pos + b[2]] != 0:
                    print(f'F fail {cat}')
                    fails_count += 1
                else:
                    print(f'G good {cat}')
            
if fails_count == 0:
    print('PERFECT')
else:
    print(f'FAILS COUNT {fails_count}')

with open('test.txt', 'w') as f:
    print(memory, file=f)
input('next')

def add_table(memory, new_memory, pos, start, tab_size):
    a = memory[start+pos:pos+tab_size]
    for i in a:
        new_memory.append(i)
    
def eq_table(memory, new_memory, tab, at, tab_size):
    for i in range(tab_size):
        if len(new_memory) <= at+i:
            return True, i
        if new_memory[at+i] != memory[tab+i]:
            return False, 0
    return True, 0

new_memory  = []
main_memory = []
leaf_memory = []
leaf_tables = []
data_memory = []

def push(memory, new_memory, pos, tables, tab_size):
    global hashing
    hash = tuple(memory[pos:pos+tab_size])
    if hash in hashing:
        tables.append(hashing[hash])
        return
    
    nmend   = len(new_memory)
    nmstart = max(nmend - tab_size-1, 0)
    for k in range(pos+tab_size, len(memory), tab_size):
        biggest     = None
        biggest_val = 257
        for l in range(nmstart, nmend):
            eq, start = eq_table(memory, new_memory, pos, l, tab_size)
            if biggest_val < start:
                biggest_val = start
                biggest     = ((memory, new_memory, pos, start, tab_size), l)
                break
        
        if biggest_val != 257:
            args, l = biggest
            add_table(*args)
            tables.append(l)
            hashing[hash] = l
            nmend += args[3]
            for i in range(nmstart, nmend):
                hashing[tuple(memory[i:i+tab_size])] = i
            return
    
    i = len(new_memory)
    tables.append(i)
    add_table(memory, new_memory, pos, 0, tab_size)
    hashing[hash] = i
    nmend += tab_size
    for i in range(nmstart, nmend):
        hashing[tuple(memory[i:i+tab_size])] = i

hashing = {}
print('DATA COMPRESSION')
for i in range(256):
    leaf_pos = memory[i]
    for j in range(256):
        data_pos = memory[leaf_pos + j]
        push(memory, data_memory, data_pos, leaf_tables, 17)
    print(f'current data size: {len(data_memory)}')
print(f'data size: {len(data_memory)}')

hashing = {}
print('LEAF COMPRESSION')
for i in range(256):
    push(leaf_tables, leaf_memory, i*256, main_memory, 256)

print(f'leaf size: {len(leaf_memory)}')

j = len(main_memory)
print('MAIN READDRESSATION')
main_addresses = []
for i in range(len(main_memory)):
    id = main_memory[i] + j
    if id in main_addresses:
        main_memory[i] = main_addresses.index(id)
        continue
    val = id
    main_memory[i] = len(main_addresses)
    main_addresses.append(val)
    
print(f'main addresses count: {len(main_addresses)}')

j += len(leaf_memory)
print('LEAF READDRESSATION')
leaf_addresses = []
for i in range(len(leaf_memory)):
    id = leaf_memory[i] + j
    if id in leaf_addresses:
        leaf_memory[i] = leaf_addresses.index(id)
        continue
    val = id
    leaf_memory[i] = len(leaf_addresses)
    leaf_addresses.append(val)

print(f'leaf addresses count: {len(leaf_addresses)}')

print('MAIN WRITING')
for e in main_memory:
    new_memory.append(e)

print('LEAF WRITING')
for e in leaf_memory:
    new_memory.append(e)

print('DATA WRITING')
for e in data_memory:
    new_memory.append(e)

input('next')

print(f'memory size before: {len(memory)}')
print(f'memory size after:  {len(new_memory)}')
print(f'memory packed: {(len(memory)/len(new_memory))} times')

def unform(arr, space, bits):
    i = 0
    f = True
    s = ''
    for e in arr:
        if f:
            f = False
        else:
            s += ','
        if i == space:
            s += '\n    '
            i = 0
        else:
            i += 1
        s += '0x'
        for j in reversed(range(0, bits, 4)):
            c = ((e>>j) & 0xF)
            if c <= 9: s += chr(ord('0') + c)
            else:      s += chr(ord('A') + c - 10)
    return s

with open('newtest.txt', 'w') as fi:
    print(f'const u32 symbols_flags_trie_leaf_addresses[{len(leaf_addresses)}] = {{\n    ' + unform(leaf_addresses, 8, 32) + "};", file=fi)
    print(f'const u32 symbols_flags_trie_main_addresses[{len(main_addresses)}] = {{\n    ' + unform(main_addresses, 16, 32) + "};", file=fi)
    print(f'const u8  symbols_flags_trie[{len(new_memory)}] = {{\n    ' + unform(new_memory, 128, 8) + "};", file=fi)
    for i, srch in enumerate(SEARCH):
        print(f'#define is{srch[0]}(c) (c <= 0x10FFFF && (symbols_flags_trie[symbols_flags_trie_leaf_addresses[symbols_flags_trie[symbols_flags_trie_main_addresses[symbols_flags_trie[(rune)c&0xFF]] + (((rune)c>>8)&0xFF)]] + (((rune)c>>16)&0xFF)]&{1<<i})!=0)\n', file=fi)
input('next')
