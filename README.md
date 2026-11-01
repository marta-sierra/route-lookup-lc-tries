# route-lookup-lc-tries

A C implementation of IPv4 longest-prefix-match route lookup using **Level-Compressed tries (LC-tries)**, following the paper:

> S. Nilsson and G. Karlsson, **"IP-address lookup using LC-tries"**, *IEEE Journal on Selected Areas in Communications*, vol. 17, no. 6, pp. 1083–1092, June 1999. [doi:10.1109/49.772439](https://ieeexplore.ieee.org/document/772439)

LC-tries (path-compressed *and* level-compressed binary tries) are the idea behind the Linux kernel's IPv4 Forwarding Information Base (`net/ipv4/fib_trie.c`). This repository rebuilds the structure from scratch to study how it works: how a plain binary trie becomes a compact array-based structure that resolves most lookups in a handful of memory accesses.

---

## How it works

Building the structure is a pipeline of transformations over the routing table (FIB):

| Phase | Function(s) | What it does |
|---|---|---|
| 1. Binary trie | `insertPrefixSimpleTrie` | Inserts every `prefix/len → interface` into a 1-bit-per-level binary trie. |
| 2. Path compression | `patriciaCompress` | Removes internal nodes that have one child and no route (Patricia / path compression), so chains of single-child nodes become skips. |
| 3. Level compression | `levelCompress` | Replaces the top *k* complete levels of a subtree with one node that has 2^k children. The node then branches on *k* bits at once. |
| 4. Table build | `buildprefixTable`, `deleteSingleChildren`, `buildMainBaseTables` | Flattens the trie into the paper's three arrays (below), placing the children of each node contiguously in breadth-first order. |
| 5. Lookup | `lookUp` | Walks the arrays for each destination address and counts memory accesses. |

### Paper data layout

As in Section III of the paper, the trie is stored in three arrays instead of pointer-linked nodes:

- **Main (trie) vector**: one entry per node: `branch` (number of bits to inspect, 0 for a leaf), `skip` (bits skipped by path compression), and `pointer` (index of the first child, or into the base vector for a leaf).
- **Base vector**: one entry per leaf: the full prefix string, its next hop, and a pointer into the prefix vector.
- **Prefix vector**: the shorter prefixes that are proper prefixes of a leaf, linked from longest to shortest. Lookups fall back to them when the leaf's own prefix does not match.

### Lookup

```
node = main[0];  pos = 0
while node.branch != 0:
    pos  += node.skip
    node  = main[node.pointer + bits(ip, pos, node.branch)]
    pos  += node.branch
leaf = base[node.pointer]
if ip matches leaf.prefix:            return leaf.next_hop
for p in prefix chain of leaf:        if ip matches p: return p.next_hop
return 0   # no route (MISS)
```

---

## Repository layout

```
.
├── src/
│   ├── my_route_lookup.c   # main: reads FIB, runs build phases, performs lookups
│   ├── trie.c / trie.h     # LC-trie: construction, compression, tables, lookup
│   ├── io.c / io.h         # FIB / packet-file parsing and output (course-provided)
│   └── utils.c / utils.h   # small helpers (course-provided)
├── data/
│   ├── routing_table.txt         # real-world FIB, ~24k prefixes
│   ├── routing_table_mid.txt     # 13 hand-written prefixes, good for debugging
│   └── routing_table_simple.txt  # 2 prefixes
├── tests/
│   └── prueba{0..3}.txt          # destination IPs to look up
└── Makefile
```

## Build and run

Requirements: a C compiler (gcc or clang) and `make`. Tested on Linux and macOS.

```sh
make                                   # builds ./my_route_lookup
./my_route_lookup <routing_table> <packets_file>

# examples
./my_route_lookup data/routing_table_mid.txt tests/prueba0.txt
make run TABLE=data/routing_table.txt PACKETS=tests/prueba1.txt
make clean
```

For debugging, the program prints the trie after path compression and after level compression.

### Input formats

**Routing table**: one route per line, `prefix/length` and output interface separated by whitespace:

```
0.0.0.0/0        1
10.0.0.0/8       2
192.168.1.0/26   9
```

**Packets file**: one IPv4 address per line.

### Output

The results are printed to stdout and also written to `<packets_file>.out`. Each line has the format

```
<ip>;<interface or MISS>;<memory accesses>;<lookup time in ns>
```

The output ends with a summary: number of nodes in the trie, packets processed, average memory accesses per lookup, average lookup time, peak memory and CPU time.

## Credits

- The algorithm and data layout come from Nilsson & Karlsson (1999), cited above.
- Author: Marta Sierra Obea.
