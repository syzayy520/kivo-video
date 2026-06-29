# P7 Generated Command Conflict Matrix

| Command | State | Decision |
|---|---|---|
| Open | NotCreated | Accept |
| Open | BuildFailed | Accept |
| Start | Ready | Accept |
| Pause | Starting | PendingTransportIntent |
| Pause | Playing | Accept |
| Resume | Paused | Accept |
| Seek | Ready | Accept |
| Seek | Playing | Accept |
| Seek | Paused | Accept |
| SwitchAudioTrack | Playing | Accept |
| SwitchVideoTrack | Playing | Accept |

Generated from backend/tools/video/playback_graph/matrix_codegen/p7_matrix_codegen.py.