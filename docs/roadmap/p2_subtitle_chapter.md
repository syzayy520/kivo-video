# P2 Subtitle/Chapter System

Updated: 2026-06-26

This document defines the subtitle and chapter system for P2, covering subtitle formats, chapter navigation, and related metadata.

## Purpose

The subtitle/chapter system provides comprehensive support for subtitle rendering, chapter navigation, and subtitle format handling across different media sources and containers.

## Subtitle System

### 1. Subtitle Types

#### Internal Subtitles
**Source**: Embedded in media container
**Formats**: SRT, ASS/SSA, WebVTT, PGS, VobSub
**Advantages**: Always available, no external files needed
**Disadvantages**: Limited to container support

#### External Subtitles
**Source**: Separate subtitle files
**Formats**: SRT, ASS/SSA, WebVTT
**Advantages**: More format options, easy to edit
**Disadvantages**: May not be available, file management needed

### 2. Subtitle Formats

#### SRT (SubRip Text)
**Extension**: .srt
**Features**: Simple text-based, timestamps, basic formatting
**Support**: Universal support
**Complexity**: Low

#### ASS/SSA (Advanced SubStation Alpha)
**Extension**: .ass, .ssa
**Features**: Rich formatting, positioning, styling, animations
**Support**: Good support in modern players
**Complexity**: High

#### WebVTT (Web Video Text Tracks)
**Extension**: .vtt
**Features**: HTML5 standard, metadata support, cue settings
**Support**: Web and modern players
**Complexity**: Medium

#### PGS (Presentation Graphic Stream)
**Source**: Blu-ray subtitles
**Features**: Bitmap-based, high quality
**Support**: Blu-ray players only
**Complexity**: High

#### VobSub (Video Object Subtitle)
**Source**: DVD subtitles
**Features**: Bitmap-based, indexed colors
**Support**: DVD players only
**Complexity**: Medium

### 3. Subtitle Metadata

#### Language
**Field**: `language` (ISO 639-2 code)
**Example**: `eng`, `chs`, `jpn`
**Usage**: Language selection, display

#### Default Flag
**Field**: `is_default` (boolean)
**Purpose**: Automatically select subtitle track
**Behavior**: Show by default if true

#### Forced Flag
**Field**: `is_forced` (boolean)
**Purpose**: Essential subtitles (signs, translations)
**Behavior**: Always show if true

#### Delay
**Field**: `delay_ms` (integer)
**Purpose**: Synchronization adjustment
**Range**: -10000 to +10000 milliseconds

### 4. MKV Font Attachments

#### Font Embedding
**Feature**: Fonts embedded in MKV container
**Purpose**: Ensure subtitle rendering consistency
**Formats**: TTF, OTF, WOFF

#### Font Selection
**Process**: Extract fonts from MKV, use for rendering
**Fallback**: System fonts if extraction fails
**Privacy**: No font file path leakage

## Chapter System

### 1. Chapter Types

#### Simple Chapters
**Format**: Timestamp + title
**Example**: `00:05:30 - Chapter 1`
**Support**: Universal

#### Ordered Chapters
**Format**: Timestamp + title + order
**Example**: `00:05:30 - Chapter 1 (order: 1)`
**Support**: MKV only

#### Chapter Images
**Format**: Timestamp + title + image
**Example**: `00:05:30 - Chapter 1 (image: chapter01.jpg)`
**Support**: Limited players

### 2. Chapter Metadata

#### Title
**Field**: `title` (string)
**Purpose**: Chapter name display
**Example**: "Opening Credits", "Act 1", "Credits"

#### Start Time
**Field**: `start_time_ms` (integer)
**Purpose**: Chapter start position
**Precision**: Milliseconds

#### End Time
**Field**: `end_time_ms` (integer)
**Purpose**: Chapter end position
**Calculation**: Next chapter start or media end

### 3. Chapter Navigation

#### Jump Policy
**Rule**: Jump to nearest chapter boundary
**Tolerance**: ±500 milliseconds
**Fallback**: Jump to start/end of media

#### Chapter Selection
**Algorithm**: Find chapter containing current position
**Edge Cases**: Before first chapter, after last chapter
**Behavior**: Select nearest chapter

## Subtitle Rendering

### 1. Rendering Pipeline
**Process**: Parse → Format → Render → Overlay
**Stages**:
1. Parse subtitle format (SRT, ASS, etc.)
2. Format text with styling
3. Render to bitmap/texture
4. Overlay on video frame

### 2. Rendering Quality
**Resolution**: Match video resolution
**Scaling**: Scale with video
**Anti-aliasing**: Enable for text subtitles
**Alpha blending**: Support for transparency

### 3. Rendering Performance
**Caching**: Cache rendered subtitles
**Pre-rendering**: Pre-render upcoming subtitles
**Threading**: Separate rendering thread
**Memory**: Limit subtitle cache size

## Privacy and Redaction

### 1. Subtitle File Paths
**Rule**: No full paths in subtitle references
**Method**: Use relative paths or redacted names
**Example**: `C:\subtitles\eng.srt` → `eng.srt`

### 2. Font File Paths
**Rule**: No full paths in font references
**Method**: Extract fonts, use temporary paths
**Example**: `C:\fonts\arial.ttf` → `temp://font_123.ttf`

### 3. Chapter Image Paths
**Rule**: No full paths in chapter image references
**Method**: Extract images, use temporary paths
**Example**: `C:\chapters\chapter01.jpg` → `temp://chapter_001.jpg`

## Gate Integration

### SUB-001 Gate Requirements
1. Subtitle types defined
2. Subtitle formats documented
3. Chapter system documented
4. Privacy/redaction rules enforced
5. Rendering pipeline defined

### PFFG-001 Gate Integration
- Foundation Final Gate Reader checks subtitle/chapter definitions
- All subtitle formats must have rendering support
- All chapter types must have navigation support

### PMX-001 Gate Integration
- Provider Runtime Matrix includes subtitle support
- Each provider must declare subtitle capabilities
- Subtitle support must match verification status

## Test Cases

### Positive Test Cases
1. Parse SRT subtitle with timestamps → Successful parsing
2. Parse ASS subtitle with styling → Successful parsing
3. Render subtitle with proper formatting → Successful rendering
4. Navigate chapters with jump policy → Successful navigation
5. Extract MKV fonts for rendering → Successful extraction
6. Redact subtitle file paths → Privacy compliant

### Negative Test Cases
1. Parse invalid subtitle format → Graceful failure
2. Render subtitle with missing fonts → Fallback to system fonts
3. Navigate to non-existent chapter → Fallback to nearest chapter
4. Extract corrupted MKV fonts → Graceful failure
5. Render subtitle with full path → Privacy violation
6. Navigate with invalid timestamps → Graceful handling

## Implementation Requirements

### Subtitle Parsing
1. Support multiple subtitle formats
2. Handle encoding issues (UTF-8, UTF-16, etc.)
3. Parse timestamps accurately
4. Handle malformed subtitle files

### Chapter Parsing
1. Support multiple chapter formats
2. Handle missing chapter metadata
3. Calculate chapter durations
4. Handle chapter ordering

### Rendering
1. Efficient subtitle rendering
2. Memory-efficient caching
3. Thread-safe rendering
4. Quality-preserving scaling