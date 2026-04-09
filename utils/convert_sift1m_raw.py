#!/usr/bin/env python3
import argparse
import os
import struct
from pathlib import Path


def read_fvecs(path: Path):
    with path.open("rb") as f:
        first = f.read(4)
        if len(first) != 4:
            raise ValueError(f"empty or invalid fvecs file: {path}")
        dim = struct.unpack("<i", first)[0]
        if dim <= 0:
            raise ValueError(f"invalid dimension {dim} in {path}")
        payload = f.read()

    record_size = 4 + dim * 4
    total_size = 4 + len(payload)
    if total_size % record_size != 0:
        raise ValueError(f"file size is not aligned to fvecs record size: {path}")

    nvecs = total_size // record_size
    vectors = []
    offset = 0
    raw = first + payload
    for _ in range(nvecs):
        cur_dim = struct.unpack_from("<i", raw, offset)[0]
        if cur_dim != dim:
            raise ValueError(f"inconsistent dimension {cur_dim} in {path}")
        offset += 4
        vec = struct.unpack_from(f"<{dim}f", raw, offset)
        offset += dim * 4
        vectors.append(vec)

    return nvecs, dim, vectors


def write_u8bin(path: Path, nvecs: int, dim: int, vectors):
    with path.open("wb") as f:
        f.write(struct.pack("<ii", nvecs, dim))
        for vec in vectors:
            for x in vec:
                # SIFT1M values are effectively byte-valued; clamp defensively.
                y = int(round(x))
                if y < 0:
                    y = 0
                elif y > 255:
                    y = 255
                f.write(struct.pack("<B", y))


def read_ivecs(path: Path):
    with path.open("rb") as f:
        first = f.read(4)
        if len(first) != 4:
            raise ValueError(f"empty or invalid ivecs file: {path}")
        k = struct.unpack("<i", first)[0]
        if k <= 0:
            raise ValueError(f"invalid width {k} in {path}")
        payload = f.read()

    record_size = 4 + k * 4
    total_size = 4 + len(payload)
    if total_size % record_size != 0:
        raise ValueError(f"file size is not aligned to ivecs record size: {path}")

    nvecs = total_size // record_size
    rows = []
    offset = 0
    raw = first + payload
    for _ in range(nvecs):
        cur_k = struct.unpack_from("<i", raw, offset)[0]
        if cur_k != k:
            raise ValueError(f"inconsistent width {cur_k} in {path}")
        offset += 4
        row = struct.unpack_from(f"<{k}i", raw, offset)
        offset += k * 4
        rows.append(row)

    return nvecs, k, rows


def write_gt_bin(path: Path, nvecs: int, k: int, rows):
    with path.open("wb") as f:
        f.write(struct.pack("<ii", nvecs, k))
        for row in rows:
            f.write(struct.pack(f"<{k}i", *row))


def main():
    parser = argparse.ArgumentParser(description="Convert raw SIFT1M fvecs/ivecs to PIM-ANNS formats.")
    parser.add_argument("--query", required=True, help="Input sift_query.fvecs")
    parser.add_argument("--learn", required=True, help="Input sift_learn.fvecs")
    parser.add_argument("--groundtruth", required=True, help="Input sift_groundtruth.ivecs")
    parser.add_argument("--out-dir", required=True, help="Output directory")
    args = parser.parse_args()

    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    query_n, query_dim, query_vecs = read_fvecs(Path(args.query))
    learn_n, learn_dim, learn_vecs = read_fvecs(Path(args.learn))
    gt_n, gt_k, gt_rows = read_ivecs(Path(args.groundtruth))

    if query_dim != 128 or learn_dim != 128:
        raise ValueError(f"SIFT1M should be 128D, got query={query_dim}, learn={learn_dim}")
    if gt_n < query_n:
        raise ValueError(f"groundtruth rows {gt_n} < query rows {query_n}")

    query_out = out_dir / "query.public.10K.u8bin"
    learn_out = out_dir / "learn.public.100K.u8bin"
    gt_out = out_dir / "sift1M_gt_k100.bin"

    write_u8bin(query_out, query_n, query_dim, query_vecs)
    write_u8bin(learn_out, learn_n, learn_dim, learn_vecs)
    write_gt_bin(gt_out, gt_n, gt_k, gt_rows)

    print(f"wrote {query_out}")
    print(f"wrote {learn_out}")
    print(f"wrote {gt_out}")
    print(f"query: n={query_n}, dim={query_dim}")
    print(f"learn: n={learn_n}, dim={learn_dim}")
    print(f"groundtruth: n={gt_n}, k={gt_k}")


if __name__ == "__main__":
    main()
