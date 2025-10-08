<!---
SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
SPDX-License-Identifier: FSFAP
-->

# Playback Cache Architecture

## Overview

This document describes the architecture for caching track events for real-time playback. The system provides thread-safe caching of MIDI, audio, and automation events to prevent threading issues between UI operations and real-time audio processing.

## Problem Statement

The architecture solves the problem described in [GitLab issue #5140](https://gitlab.zrythm.org/zrythm/zrythm/-/work_items/5140) where reading live data from the UI while audio threads are processing causes threading conflicts. Previously, the engine had to be stopped on each edit, causing poor user experience.

## Architectural Goals

- **Thread Safety**: Allow UI modifications while audio processing continues uninterrupted
- **Performance**: Minimize overhead for real-time audio processing
- **Flexibility**: Support multiple event types (MIDI, audio, automation)
- **Efficiency**: Only update changed portions of the timeline
- **Debouncing**: Avoid excessive cache regeneration during rapid UI changes

## Key Components

### 1. PlaybackCacheScheduler

Located in [`src/utils/playback_cache_scheduler.h`](src/utils/playback_cache_scheduler.h)

**Purpose**: Collects and debounces cache requests, then triggers cache generation.

**Key Features:**
- Debounces multiple rapid requests into single operations
- Expands affected ranges to ensure complete coverage
- Configurable delay for optimal performance

```mermaid
classDiagram
    class PlaybackCacheScheduler {
        +queueCacheRequestForRange(start, end)
        +queueFullCacheRequest()
        +queueCacheRequest(range)
        +setDelay(ms)
        #cacheRequested(range) signal
        -QTimer timer
        -std::chrono::milliseconds delay
        -bool pending_call
        -std::optional~ExpandableTickRange~ affected_range
    }
```

### 2. ExpandableTickRange

Located in [`src/utils/expandable_tick_range.h`](src/utils/expandable_tick_range.h)

**Purpose**: Represents a range of ticks that can be expanded to include additional ranges.

**Key Features:**
- Can represent full content or specific ranges
- Supports range expansion operations
- Handles conversion between ticks and samples

### 3. TimelineDataCache

Located in [`src/dsp/timeline_data_cache.h`](src/dsp/timeline_data_cache.h)

**Purpose**: Unified cache holder that manages both MIDI and audio event sequences for playback.

**Key Features:**
- Stores MIDI sequences and audio regions by interval for efficient updates
- Validates event boundaries and ensures note completeness
- Merges sequences for final playback
- Creates independent copies of audio data for thread safety
- Removes both MIDI and audio data that overlaps with specified intervals
- Handles both MIDI note events and audio sample data with equal priority

```mermaid
classDiagram
    class TimelineDataCache {
        +clear()
        +remove_sequences_matching_interval(interval)
        +add_midi_sequence(interval, sequence)
        +add_audio_region(interval, audio_buffer)
        +finalize_changes()
        +get_midi_events() const
        +get_audio_regions() const
        -std::map~IntervalType, juce::MidiMessageSequence~ midi_sequences_
        -std::vector~AudioRegionEntry~ audio_regions_
        -juce::MidiMessageSequence merged_midi_events_
    }
```

### 4. TimelineDataProvider

Located in [`src/structure/arrangement/timeline_data_provider.h`](src/structure/arrangement/timeline_data_provider.h)

**Purpose**: Unified bridge between the playback cache system and track processors, providing both MIDI and audio events for real-time playback.

**Key Features:**
- Generates MIDI event sequences from timeline regions
- Generates audio region data from timeline regions
- Manages thread-safe access to cached events using `farbot::RealtimeObject`
- Processes events for specific time ranges during audio processing
- Handles range-based cache updates for efficiency
- Provides consistent interface for both MIDI and audio event processing

```mermaid
classDiagram
    class TimelineDataProvider {
        +generate_midi_events(tempo_map, midi_regions, affected_range)
        +generate_audio_events(tempo_map, audio_regions, affected_range)
        +process_midi_events(time_info, output_buffer)
        +process_audio_events(time_info, output_left, output_right)
        -set_midi_events(events)
        -set_audio_regions(regions)
        -farbot::RealtimeObject active_midi_playback_sequence_
        -farbot::RealtimeObject active_audio_regions_
        -TimelineDataCache unified_playback_cache_
    }
```

### 5. Track and TrackProcessor Integration

**Track** ([`src/structure/tracks/track.h`](src/structure/tracks/track.h)):
- Owns the `TimelineDataProvider` instance
- Contains `PlaybackCacheScheduler` for managing requests
- Connects to arrangement object changes

**TrackProcessor** ([`src/structure/tracks/track_processor.h`](src/structure/tracks/track_processor.h)):
- Uses `TimelineDataProvider` for MIDI and audio event access
- Implements the playback interface using cached events
- Integrates with the track's event provider system
- Provides unified processing for both MIDI and audio events

## Signal Flow

The caching process follows this sequence:

```mermaid
sequenceDiagram
    participant UI as UI/Arranger Object
    participant AOLM as ArrangerObjectListModel
    participant Scheduler as PlaybackCacheScheduler
    participant Track as Track
    participant Provider as TimelineDataProvider
    participant Cache as TimelineDataCache
    participant Processor as TrackProcessor

    UI->>AOLM: Object changed (position, content, etc.)
    AOLM->>Scheduler: contentChanged(affectedRange)
    Scheduler->>Scheduler: Debounce and expand range
    Scheduler->>Track: cacheRequested(finalRange)
    Track->>Provider: generate_midi_events(tempo_map, midi_regions, range)
    Track->>Provider: generate_audio_events(tempo_map, audio_regions, range)
    Provider->>Cache: remove_sequences_matching_interval(range)
    Provider->>Provider: Process regions in range
    Provider->>Cache: add_midi_sequence(interval, sequence)
    Provider->>Cache: add_audio_region(interval, audio_buffer)
    Provider->>Cache: finalize_changes()
    Provider->>Provider: set_midi_events(cached_events())
    Provider->>Provider: set_audio_regions(cached_regions())
    Provider->>Provider: Swap realtime cache (thread-safe)
    Processor->>Provider: process_midi_events(time_info, output_buffer)
    Processor->>Provider: process_audio_events(time_info, output_left, output_right)
    Provider->>Processor: Return events for playback
```

## Real-time Safety

The architecture ensures real-time safety through:

1. **RealtimeObject**: Uses `farbot::RealtimeObject` for atomic cache swapping in `TimelineDataProvider`
2. **Debouncing**: Prevents cache thrashing during rapid UI changes
3. **Range-based Updates**: Only processes affected portions of the timeline
4. **Separation of Concerns**: UI thread manages cache generation, audio thread uses cached data
5. **Event Provider Pattern**: `TimelineDataProvider` abstracts the cache access pattern
6. **Independent Copies**: Audio data is copied into the cache to prevent threading issues
7. **Unified Processing**: Both MIDI and audio use the same threading and caching patterns

## Event Types Support

### Currently Implemented
- **MIDI Events**: Fully implemented with region-based caching via `TimelineDataProvider`
- **Audio Events**: Fully implemented with region-based caching via `TimelineDataProvider`
- **Automation Events**: Will be handled in AutomationTracks

### Future Extensibility

The architecture is designed to support additional event types:
- Unified cache system for different event types
- Configurable caching strategies per event type
- Event provider pattern for different event types
- Consistent threading patterns across all event types

## Performance Considerations

- **Debounce Timing**: 100ms default delay balances responsiveness and performance
- **Range Optimization**: Only processes changed regions, not entire timeline
- **Memory Efficiency**: Caches are stored in optimal formats for playback
- **CPU Usage**: Cache generation happens off the real-time thread
- **Event Provider Efficiency**: `TimelineDataProvider` optimizes event access patterns
- **Audio Buffer Management**: Efficient copying and management of audio sample data
- **Unified Processing**: Both MIDI and audio benefit from the same performance optimizations

## Error Handling

- **Validation**: Events are validated to ensure they fit within specified intervals
- **Note Completeness**: Ensures all note-on events have corresponding note-offs
- **Tempo Awareness**: Properly handles tempo changes through tempo map integration
- **Provider Safety**: `TimelineDataProvider` handles edge cases in event processing
- **Audio Buffer Independence**: Ensures cached audio buffers are independent copies
- **Cross-Type Validation**: Consistent error handling across MIDI and audio event types

## Usage Examples

### MIDI Cache Generation
```cpp
// In Track::regeneratePlaybackCaches
timeline_data_provider_->generate_midi_events(
    base_dependencies_.tempo_map_,
    lanes_view(),
    affectedRange
);
```

### Audio Cache Generation
```cpp
// In Track::regeneratePlaybackCaches
timeline_data_provider_->generate_audio_events(
    base_dependencies_.tempo_map_,
    audio_regions_view(),
    affectedRange
);
```

### Real-time MIDI Cache Access
```cpp
// In TrackProcessor::fill_midi_events
timeline_data_provider_->process_midi_events(
    time_info,
    output_buffer
);
```

### Real-time Audio Cache Access
```cpp
// In TrackProcessor::fill_audio_events
timeline_data_provider_->process_audio_events(
    time_info,
    output_left,
    output_right
);
```

## Related Components

- **ArrangerObjectListModel**: Propagates change notifications
- **TrackLaneList**: Contains MIDI regions and triggers cache updates
- **MidiRegionSerializer**: Converts MIDI regions to message sequences
- **AudioRegionSerializer**: Converts audio regions to sample buffers
- **TempoMap**: Provides timing conversion between ticks and samples
- **TimelineDataProvider**: Bridge between cache system and track processors
- **ClipLauncherEventProvider**: Handles clip-based MIDI and audio event processing
- **ClipPlaybackDataProvider**: Provides unified MIDI and audio processing for clip launcher

## Conclusion

This caching architecture provides a robust solution for thread-safe playback. The addition of `TimelineDataProvider` with unified `TimelineDataCache` creates a clean separation between cache management and real-time event processing, improving maintainability and extensibility. The system balances performance, flexibility, and real-time safety while providing comprehensive support for MIDI and audio event caching, with a foundation for future expansion to support automation event caching.

The unified approach to handling both MIDI and audio events ensures consistent behavior, performance characteristics, and threading safety across all event types. This architecture provides a solid foundation for professional-grade audio production with real-time clip launching and timeline playback capabilities.
