# Decisions

Short records of choices that are not obvious from the code, so they are not
re-litigated later. Newest last.

## 2026-10-06 — serialize into a caller-owned buffer, don't return bytes

`serialize_block(..., uint8_t* out, size_t out_size)` rather than returning a
`std::vector<uint8_t>`.

**Why.** A block is 64k ticks — roughly 1.5 MB. Returning it by value means an
allocation on every call, and a write path does thousands of blocks. With an out
parameter the caller allocates one buffer and reuses it for the whole run: zero
allocations in steady state.

It also keeps the destination open. The caller can serialize straight into an
mmap'd file region or a shared-memory ring; a returned vector forces a copy.

The general principle, and it is why `read()`, `memcpy` and `snprintf` all work this
way: **the caller owns the memory.**

**Cost.** Every function needs a size parameter and has to refuse to overflow. That
check is not optional — it is the only thing standing between a wrong length and a
buffer overrun.

## 2026-10-06 — `uint8_t*` for byte buffers, never a wider pointer type

Pointer arithmetic scales by the pointee size, so `out + 6` on a `uint32_t*` moves 24
bytes, not 6. Header fields sit at offsets 0, 4 and 6 — offset 6 is not reachable at
all with 4-byte strides, and dereferencing a misaligned `uint32_t*` is undefined
behaviour. Bytes are the only unit that can address an arbitrary offset.

## 2026-10-06 — scaled integers for price and volume, never float

`int64_t` with a precision exponent held once in the file header.

**Why.** Binary floating point cannot represent most decimal fractions, so prices
cannot be compared with `==` — equality becomes an epsilon choice, which in anything
order-matching-shaped is a bug factory. Errors also accumulate across sums. Integers
additionally delta-encode well, which ring 1 depends on; floats do not.

## 2026-10-06 — padding is made explicit, not left to the compiler

Every hole is a named field and every struct carries a `static_assert` on its size.
A file format defined by "whatever the compiler decided today" is not a format.

Caught three layout bugs on the first morning, which is the argument for it.

**Still native-endian and unpacked.** That is a deliberate choice, not an oversight —
revisit if the files ever have to move between architectures.

## 2026-10-06 — the block checksum covers the body only

Not the header. A checksum cannot cover the field it lives in, and zeroing-then-
computing is fiddle for a problem ring 0 does not have.

**Consequence, accepted knowingly:** a flipped bit in `ts_min` is undetectable. Ring 2
builds its index from that field, so if header integrity starts to matter, that is a
`version` bump to v2 — which is what the version byte exists for.

## 2026-10-06 — a sequence number per block

`uint32_t`, incrementing from 0.

**Why.** Files are sequential, so a block that is serialized but never written leaves
**no gap**. The next block lands flush against the previous one and the file is
perfectly well-formed: every header parses, every checksum validates, every length is
right. 64,000 ticks are gone and nothing says so.

That is strictly worse than truncation, which the length check already catches. A
sequence number is the only thing that makes a missing middle visible — a reader seeing
0, 1, 3 knows what happened.

Timestamp continuity does **not** work as a substitute: legitimate gaps exist whenever
the market is closed, so a jump in time proves nothing.

**Cost.** `block_header` goes 32 → 40. That is deliberate, not drift. Do not "fix" it
back.

## 2026-10-06 — file-level integrity deferred to a sealed-file footer

Not in ring 0, and not in the file header.

**Why not the file header.** It sits at the front, and the value cannot be computed
until the file is complete — so for a file that appends all day it would be absent or
wrong for its entire useful life.

**Why a footer, later.** A footer is only valid once the file is *sealed*, and a
crashed writer never writes one. That is fine for an end-of-day seal and useless during
the session.

The split is the same one the industry already makes: **Parquet** has a footer with
checksums and an index because its files are written once and sealed; **write-ahead
logs** use per-record CRCs and no file checksum because they append continuously. This
format is the second kind during the trading day and arguably the first kind after it
closes.

So: per-block checksums and sequence numbers while open; a footer carrying a whole-file
checksum **and ring 2's block index** when sealed. One future feature delivers both.

## 2026-10-06 — CRC-32C for the block checksum, hand-written

Castagnoli polynomial, table-driven, implemented in this repo rather than pulled in.

**Why CRC over a modern hash.** The threat model is disk corruption — flipped bits and
burst errors — and CRC-32 *proves* it detects every single-bit error, every double-bit
error, every odd number of bit errors, and every burst up to 32 bits. Those are
theorems. xxHash is faster and statistically excellent but offers no such guarantee; it
was designed for hash-table distribution, not for catching bad sectors.

The second argument is the field width. `checksum` is already a `uint32_t`. xxHash's
advantage is its 64-bit output; truncating it to 32 bits discards the benefit while
keeping the absence of guarantees.

**Why Castagnoli specifically.** CRC-32C has a dedicated instruction on both x86 and
ARM, so it runs at roughly a byte per cycle on this machine.

Not considered seriously: SHA-256 and friends. Slow, 32 bytes, and they defend against
a forger. Nobody is forging tick files.

**Why hand-written.** The standard library has no CRC, and the alternative is taking a
zlib dependency for thirty lines. It is also the only part of this format with real
mathematical structure, which makes it the right thirty lines to write by hand given
the project exists for practice.
