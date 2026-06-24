# P1 Media Pipeline Contracts

These contracts are fake backend shells. They do not perform real network, demux, decode, or rendering work.

## Network Cache

`read_fake_cache_extent` proves that cache reads are allowed only when the request and extent are bound to the same `RemoteObjectIdentity`.

## Demux

`build_fake_stream_inventory` produces a versioned `StreamInventory` with fake video, audio, subtitle, chapter, attachment, HDR, and DV metadata entries.

## Decode

`build_fake_decode_path` produces a fake HEVC Main10 decode path with hardware preference and software fallback availability.

## Boundary

No FFmpeg, D3D, DXGI, WASAPI, or third-party SDK type crosses into these core contracts.

