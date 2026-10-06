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
