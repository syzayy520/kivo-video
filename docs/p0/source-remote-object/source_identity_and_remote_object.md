# Source Identity And Remote Object

Remote media identity is not a URL. A URL is an access path.

## SourcePlayableObject

Supported P1 fake source kinds:

- LocalFilePlayable
- SMBPlayable
- WebDavPlayable
- AListDirectPlayable
- EmbyDirectPlayable
- EmbyProxyPlayable
- CloudDrivePlayable

## RemoteObjectIdentity

RemoteObjectIdentity records provider/server identity, canonical path, content length, etag, last modified, hash if available, range behavior, direct link lifetime, refresh strategy, cache identity key, and trace id.

## Fail-Closed Rule

Cache extents must bind to RemoteObjectIdentity. RemoteObjectIdentity conflict fails closed or enters user-confirm repair.

