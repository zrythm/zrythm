<!---
SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
SPDX-License-Identifier: CC-BY-SA-4.0
-->

# Scenes System Architecture

## Overview

This document outlines the architecture for Zrythm's scenes system, which provides clip launching functionality. The system integrates seamlessly with Zrythm's existing architecture while providing professional-grade clip launching capabilities.

## Architecture Overview

The scenes system is organized into several key components that work together to provide clip launching functionality:

```mermaid
graph TB
    Project --> ClipLauncher
    Project --> ClipPlaybackService
    ClipLauncher --> Scene
    Scene --> ClipSlot
    ClipPlaybackService --> TrackProcessor
    TrackProcessor --> ClipLauncherEventProvider
    TrackProcessor --> TimelineEventProvider
    Track --> TrackProcessor
```

### Key Components

1. **Project**: Owns the global ClipLauncher and ClipPlaybackService instances
2. **ClipLauncher**: Manages scenes and clip slots (UI/data layer)
3. **ClipPlaybackService**: Handles all clip playback operations (service layer)
4. **Scene**: Represents a vertical column of clips
5. **ClipSlot**: Contains a region and manages clip state
6. **TrackProcessor**: Manages playback modes and event providers
7. **ClipLauncherEventProvider**: Handles clip-specific MIDI and audio event processing
8. **TimelineEventProvider**: Handles timeline-specific MIDI and audio event processing

## Service-Based Architecture

The system uses a service-based approach where the ClipPlaybackService handles all playback operations, providing a clean separation between the UI layer and the audio engine.

### Service Layer Benefits

- **Realtime Safety**: The service manages state transitions without blocking the audio thread
- **Centralized Logic**: All clip playback logic is centralized in one place
- **Testability**: The service can be easily mocked for testing
- **Flexibility**: Multiple UI components can use the same service

## Playback Mode Management

Tracks can operate in two distinct playback modes:

- **Timeline Mode**: Uses regions from the timeline arrangement
- **Clip Launcher Mode**: Uses regions from clip launcher scenes

The TrackProcessor manages switching between these modes by activating/deactivating the appropriate event providers.

```mermaid
stateDiagram-v2
    [*] --> TimelineMode
    TimelineMode --> ClipLauncherMode: User launches clip
    ClipLauncherMode --> TimelineMode: User switches mode
    ClipLauncherMode --> ClipLauncherMode: User launches different clip
```

## User Interface Architecture

### Clip Launcher View Layout

The clip launcher is displayed as a grid with tracks as rows and scenes as columns:

```mermaid
graph TB
    subgraph "Clip Launcher Grid"
        Header[Track Headers] --> Scene1[Scene 1]
        Header --> Scene2[Scene 2]
        Header --> Scene3[Scene 3]

        Track1[Track 1] --> Slot1_1[Clip Slot]
        Track1 --> Slot1_2[Clip Slot]
        Track1 --> Slot1_3[Clip Slot]

        Track2[Track 2] --> Slot2_1[Clip Slot]
        Track2 --> Slot2_2[Clip Slot]
        Track2 --> Slot2_3[Clip Slot]
    end
```

### View Toggle System

The interface supports toggling between Timeline and Clip Launcher views:

```mermaid
stateDiagram-v2
    [*] --> TimelineView
    TimelineView --> BothViews: User enables Clip Launcher
    BothViews --> ClipLauncherView: User hides Timeline
    ClipLauncherView --> BothViews: User enables Timeline
    BothViews --> TimelineView: User hides Clip Launcher
```

## Clip Launching Flow

### State Management

The system uses a timer-based approach to monitor playback state changes, ensuring realtime safety:

```mermaid
stateDiagram-v2
    [*] --> Stopped
    Stopped --> PlayQueued: User launches clip
    PlayQueued --> Playing: Audio engine confirms
    Playing --> StopQueued: User stops clip
    StopQueued --> Stopped: Audio engine confirms
    PlayQueued --> Stopped: Track mode switched
```

### Quantization System

Clips can be launched with various quantization options to ensure musical timing:

```mermaid
graph LR
    Current[Current Position] --> NextBar[Next Bar]
    Current --> NextBeat[Next Beat]
    Current --> NextQuarter[Next Quarter]
    Current --> Immediate[Immediate]

    NextBar --> Launch[Launch Clip]
    NextBeat --> Launch
    NextQuarter --> Launch
    Immediate --> Launch
```

## Event Processing Architecture

### Provider System

The TrackProcessor uses a flexible provider system to handle different playback modes:

```mermaid
graph TB
    subgraph "TrackProcessor"
        TP[Track Processor] --> TLEP[Timeline Event Provider]
        TP --> CLEP[Clip Launcher Event Provider]

        TLEP --> TimelineCache[Timeline Cache]
        CLEP --> ClipCache[Clip Cache]

        TimelineCache --> MidiOutput[MIDI Output]
        TimelineCache --> AudioOutput[Audio Output]
        ClipCache --> MidiOutput
        ClipCache --> AudioOutput
    end
```

### Thread-Safe State Management

The system uses lock-free structures to ensure thread safety between the UI and audio threads:

```mermaid
sequenceDiagram
    participant UI as UI Thread
    participant Audio as Audio Thread
    participant State as Thread-Safe State

    UI->>State: Schedule clip launch
    UI->>State: Set state to PlayQueued

    Note over Audio: Audio processing loop
    Audio->>State: Check playback state
    State->>Audio: Return current state

    alt State changed to Playing
        Audio->>State: Update state to Playing
        Audio->>UI: Notify state change
    end
```

## Clip Playback Service

### Service Responsibilities

The ClipPlaybackService handles all clip playback operations, providing a clean interface between the UI and the audio engine:

```mermaid
graph TB
    subgraph "ClipPlaybackService"
        CPS[Clip Playback Service] --> Launch[Launch Clip]
        CPS --> Stop[Stop Clip]
        CPS --> Monitor[State Monitor]
        CPS --> Cancel[Cancel Operations]
        CPS --> Position[Get Playback Position]

        Launch --> Timer[Timer Setup]
        Stop --> Timer
        Monitor --> Timer
        Cancel --> Timer
        Position --> UI[UI Update]
    end
```

### State Monitoring

The service uses timers to monitor playback state changes without blocking the audio thread:

```mermaid
sequenceDiagram
    participant Service as ClipPlaybackService
    participant Timer as State Monitor Timer
    participant Processor as TrackProcessor
    participant UI as UI Update

    Service->>Timer: Start timer (60fps)
    loop Every frame
        Timer->>Processor: Check playing state
        Processor->>Timer: Return current state

        alt State changed
            Timer->>Service: Update clip state
            Service->>Timer: Stop timer
            Service->>UI: Notify state change
        end
    end
```

### Mode Switching

When a track is switched to timeline mode, the service cancels any pending clip operations:

```mermaid
stateDiagram-v2
    [*] --> ClipLauncherMode
    ClipLauncherMode --> Pending: Clip launch scheduled
    Pending --> Playing: Audio engine confirms

    Pending --> Cancelled: User switches mode
    Cancelled --> Stopped: Service cancels timer

    Playing --> Stopped: User stops clip
    Stopped --> TimelineMode: User switches mode
```

## Clip Caching Strategy

### Position-Independent Caching with Transport Awareness

The system uses position-independent caching to allow clips to start at any position and loop continuously, with special handling for transport position changes:

```mermaid
graph TB
    subgraph "Clip Caching Process"
        MidiRegion[MIDI Region] --> MidiCache[MIDI Cache]
        AudioRegion[Audio Region] --> AudioCache[Audio Cache]
        MidiCache --> Launch[Launch at Any Position]
        AudioCache --> Launch
        Launch --> Loop[Continuous Looping]
        Transport[Transport Change] --> Recalc[Recalculate Position]
        Recalc --> Loop
    end
```

### Cache Generation Flow

```mermaid
sequenceDiagram
    participant Service as ClipPlaybackService
    participant Provider as ClipLauncherEventProvider
    participant Cache as PlaybackCacheBuilder
    participant Audio as Audio Thread

    Service->>Provider: Launch clip
    Provider->>Cache: Generate position-independent cache
    Cache->>Provider: Return cached events
    Provider->>Provider: Store cache for real-time use

    Note over Audio: Audio processing loop
    Audio->>Provider: Process events from cache
    Provider->>Audio: Output MIDI events
    Provider->>Audio: Output audio samples

    Note over Audio: Transport position change
    Audio->>Provider: Detect position change
    Provider->>Provider: Recalculate internal position
    Provider->>Audio: Continue with new position
```

### Transport Position Handling

The ClipLauncherEventProvider tracks timeline position changes and recalculates the internal buffer position accordingly for both MIDI and audio. It also handles transport state transitions to prevent hanging notes:

```mermaid
stateDiagram-v2
    [*] --> Playing
    Playing --> Playing: Normal playback
    Playing --> Recalculating: Transport position change
    Recalculating --> Playing: Position updated
    Playing --> Stopped: Transport stops
    Stopped --> Playing: Transport resumes

    note right of Stopped
        Send all-notes-off on all 16 MIDI channels
        Reset internal playback state
    end note

    note right of Recalculating
        Calculate time since launch
        Apply modulo for looping
        Update internal buffer position
        Handle both MIDI and audio events
    end note
```

## Integration with Audio Engine

### DSP Graph Integration

The TrackProcessor is already part of the DSP graph, so no new nodes are needed. Each TrackProcessor delegates to active event providers during audio processing:

```mermaid
graph TB
    subgraph "DSP Graph"
        Input[Audio Input] --> TP1[Track Processor 1]
        Input --> TP2[Track Processor 2]
        Input --> TP3[Track Processor 3]

        TP1 --> Output[Audio Output]
        TP2 --> Output
        TP3 --> Output

        TP1 --> TLEP1[Timeline Provider]
        TP1 --> CLEP1[Clip Provider]

        TP2 --> TLEP2[Timeline Provider]
        TP2 --> CLEP2[Clip Provider]

        TP3 --> TLEP3[Timeline Provider]
        TP3 --> CLEP3[Clip Provider]
    end
```

### Realtime-Safe Operations

The system ensures realtime safety by:

1. **Lock-free State Updates**: Using atomic operations for state changes
2. **Timer-based Monitoring**: Avoiding blocking calls in the audio thread
3. **Cache Swapping**: Using thread-safe cache swapping mechanisms
4. **Non-blocking Operations**: All operations that might block are performed outside the audio thread
5. **Transport Position Tracking**: Efficient tracking of timeline position changes without blocking
6. **Unified Event Processing**: Both MIDI and audio events use the same thread-safe caching mechanisms
7. **Transport State Transitions**: Detect when transport stops/starts and send appropriate MIDI messages (all-notes-off)
8. **Buffer Management**: Providers add to buffers while TrackProcessor handles clearing at cycle boundaries

## Data Serialization

### Project Format

The clip launcher data is serialized as part of the project file:

```mermaid
graph TB
    subgraph "Project Structure"
        Project[Project] --> ClipLauncher[ClipLauncher]
        Project --> TrackCollection[Track Collection]

        ClipLauncher --> Scenes[Scene Array]

        Scenes --> Scene1[Scene 1]
        Scenes --> Scene2[Scene 2]

        Scene1 --> ClipSlots1[Clip Slots Map]
        Scene2 --> ClipSlots2[Clip Slots Map]

        ClipSlots1 --> Slot1_1[Track UUID -> Clip Slot]
        ClipSlots1 --> Slot1_2[Track UUID -> Clip Slot]
    end
```

## Future Enhancements

### Planned Features

1. **Follow Actions**: Configure what happens after a clip finishes
2. **Scene Launch Quantization**: Different quantization options for scene launches
3. **Clip Envelopes**: Volume and effect envelopes for clips
4. **MIDI Mapping**: MIDI controls for clip launching
5. **Clip Recording**: Record clips directly into the clip launcher
6. **Playback Position Visualization**: Enhanced visual feedback for clip playback
7. **Audio Clip Warping**: Time-stretching and pitch-shifting for audio clips

### Architecture Extensibility

The service-based architecture makes it easy to add new features:

```mermaid
graph TB
    subgraph "Extension Points"
        Service[ClipPlaybackService] --> Future1[Follow Actions Service]
        Service --> Future2[Recording Service]
        Service --> Future3[Envelope Service]

        UI[Clip Launcher UI] --> FutureUI1[Follow Actions UI]
        UI --> FutureUI2[Recording UI]
        UI --> FutureUI3[Envelope UI]
    end
```

## Benefits of This Architecture

1. **Realtime Safety**: Timer-based monitoring prevents audio thread blocking
2. **Clean Separation**: Service layer separates UI from audio engine
3. **Testability**: Services can be easily mocked for testing
4. **Flexibility**: Multiple providers can be active simultaneously
5. **Maintainability**: Clear boundaries between components
6. **Extensibility**: Easy to add new features without affecting existing code
7. **Performance**: Efficient caching and event processing for both MIDI and audio
8. **User Experience**: Professional-grade clip launching with quantization
9. **Unified Processing**: Consistent handling of MIDI and audio events across the system
10. **Audio Quality**: High-fidelity audio playback with proper caching and real-time processing

This architecture provides a solid foundation for a professional-grade scenes system that integrates seamlessly with Zrythm's existing architecture while providing the flexibility and power users expect from modern DAWs. The unified approach to MIDI and audio event processing ensures consistent behavior and high performance across all clip types.
