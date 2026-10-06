# tickstore — concentric rings

A columnar store for market-data ticks. Append ticks, keep them small on disk,
read a time range back fast.

**The rule that makes it concentric: ring N ships before ring N+1 starts.** Tagged,
README updated, benchmark number recorded. A ring that is "nearly done" when the next
one begins is how this becomes another scaffold — and the record says that is the
failure mode to design against, not the one to hope away.

Every ring below is a complete, defensible project on its own. Stopping after ring 0
leaves something finished. Stopping after a scaffold leaves nothing.

---

## The seams — decide these on day one or the rings become rewrites

These four choices are the whole difference between widening and rebuilding. They cost
almost nothing in ring 0 and they are expensive to retrofit.

1. **Column-oriented from the first commit.** If ring 0 writes rows, ring 1 is a
   rewrite rather than an addition. Write each field to its own run of bytes even when
   there is no compression yet and it looks like pointless indirection.

2. **Block-structured from the first commit.** Fixed-count blocks (say 64k ticks), each
   with a small header. Ring 2's index then just records block boundaries that already
   exist. Without blocks, adding an index means changing the file format.

3. **A codec id in every block header**, set to `0 = raw` in ring 0. Ring 1 adds codecs
   by writing a different id. Readers switch on it. No format break, and old files stay
   readable — which is also how you measure ring 1's gain honestly, against real ring 0
   files.

4. **A version byte in the file header.** Cheap insurance. It is the thing you cannot
   add later, because by then there are files in the wild without it.

---

## Ring 0 — it works  (~8–10 commits)

One symbol, one file, fixed schema: `(timestamp int64, price int64, volume uint32)`.
Writer appends, reader scans, round-trip verified. No compression, no index.

- [ ] File + block headers, with version byte and codec id
- [ ] Writer: append, flush block at 64k ticks
- [ ] Reader: sequential scan, full file
- [ ] Round-trip test, 10M ticks, property-based if convenient
- [ ] Benchmark vs reading the same data from CSV

**Ships as:** "A columnar tick store. 10M ticks, round-trip verified, Nx faster to read
than CSV." That is a finished project.

## Ring 1 — it is small  (~6–8 commits)

Per-column encoding, which is the point of being columnar at all.

- [ ] Delta-of-delta for timestamps (near-monotonic, compresses hard)
- [ ] Delta + zigzag varint for prices
- [ ] Varint for volumes
- [ ] Codec chosen per block, recorded in the header
- [ ] Benchmark: compression ratio and read throughput vs ring 0

**Ships as:** the same claim plus a real number — "3x smaller, reads at N ticks/sec."

## Ring 2 — it is fast to query  (~5–7 commits)

- [ ] Block-level min/max timestamp footer
- [ ] `read(t0, t1)` skipping blocks outside the range
- [ ] Benchmark: range query over the full file vs full scan

**Ships as:** "Range queries over a billion ticks without scanning them."

## Ring 3 — it is general  (~6 commits)

- [ ] Multiple symbols
- [ ] Declared schemas rather than the fixed triple
- [ ] Per-column codec selection driven by declared type

## Ring 4 — it is zero-copy

- [ ] mmap reads, no deserialisation on the scan path
- [ ] Benchmark the difference, and be honest if there isn't one

## Ring 5 — pick one, only if rings 0–4 shipped

Concurrent writers · Arrow interop · a tiny query language · an aggregation layer.

---

## Why this project

Decided 2026-10-05, over an order book, because an order book repeats the day job.
Storage is the adjacent thing he does *not* already own: byte layout, compression,
mmap, indexing. Still market-data infrastructure, so it reads as on-domain to a Swiss
fintech interviewer.

And it is the same problem class as **CERN's RNTuple** — ROOT's columnar format for
petabyte-scale physics data. Same questions, different field. That gives real standing
to engage with `root-project/rntuple-validation` later, instead of arriving as a
web-app contributor. No other candidate provided that bridge.

**Not in quant-stack.** That repo is sixteen empty module directories and one commit;
adding a real module to it still reads as a scaffold. This is its own repo, public only
once ring 0 ships.

**Language:** C++ if it should double as interview preparation, which is the argument
for it — public C++ is currently 23KB. Go if finishing matters more than that.
