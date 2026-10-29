#!/bin/sh
# Static analysis only. The target binaries are never executed and ldd is not used.
rm -rf analysis
mkdir -p analysis

for b in data_sync data_sync_v2; do
    file "$b"          > "analysis/${b}_file.txt"
    readelf -W -h "$b" > "analysis/${b}_readelf_h.txt"
    readelf -W -S "$b" > "analysis/${b}_readelf_S.txt"
    readelf -W -l "$b" > "analysis/${b}_readelf_l.txt"
    readelf -W -s "$b" > "analysis/${b}_readelf_s.txt"
    readelf -W -d "$b" > "analysis/${b}_readelf_d.txt"
    nm -n "$b"         > "analysis/${b}_nm_n.txt"
    nm -D "$b"         > "analysis/${b}_nm_D.txt"
    objdump -h "$b"    > "analysis/${b}_objdump_h.txt"
    objdump -t "$b"    > "analysis/${b}_objdump_t.txt"
    objdump -T "$b"    > "analysis/${b}_objdump_T.txt"
    objdump -d "$b"    > "analysis/${b}_objdump_d.txt"
    objdump -p "$b"    > "analysis/${b}_objdump_p.txt"
done
echo "done: $(ls analysis | wc -l) files in analysis/"