<!---
SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
SPDX-License-Identifier: FSFAP
-->

# Plugin Group Architecture

## Overview

Plugin Group provides a unified container system for implementing racks, layers, and plugin containers. It supports recursive nesting, serial/parallel processing, and type-safe signal routing.

## Core Concepts

### Universal Container
Plugin Group serves as a universal container that can represent:
- Simple plugin wrapper with fader
- Serial processing chain
- Parallel processing layers
- Nested rack structures

### Processing Types
- **Serial**: Process elements sequentially, connecting output to next input
- **Parallel**: Process elements separately and sum outputs

### Signal Types
- **Audio**: Audio effects and processing devices
- **MIDI**: MIDI effects and processing devices
- **Instrument**: Instrument generators and related effects

*Note: While we use "plugin" terminology, these can also be referred to as "devices" in UI/documentation.*

## Architecture Overview

```mermaid
graph TB
    subgraph "Channel Architecture"
        Channel[MIDI FX Channel] --> PG1[Plugin Group: MIDI FX]
        Channel --> PG2[Plugin Group: Instrument]
        Channel --> PG3[Plugin Group: Audio FX]

        PG1 --> Serial1[Serial Processing]
        PG2 --> Parallel1[Parallel Layers]
        PG3 --> Serial2[Serial Processing]

        Serial1 --> Plugin1[MIDI Plugin]
        Serial1 --> Plugin2[MIDI Plugin]

        Parallel1 --> Layer1[Layer 1: Instrument]
        Parallel1 --> Layer2[Layer 2: Instrument]

        Layer1 --> AudioChain1[Audio FX Chain]
        Layer2 --> AudioChain2[Audio FX Chain]

        AudioChain1 --> Nested1[Nested Plugin Group]
        AudioChain2 --> Plugin3[Audio Plugin]
    end
```

## Component Structure

```mermaid
classDiagram
    class PluginGroup {
        +ProcessingType processing_type
        +PluginGroupType type
        +Fader fader
        +insert_plugin(plugin, index)
        +remove_plugin(plugin_id)
        +element_at_idx(index)
        -vector~PluginOrGroup~ plugins_
    }

    class PluginOrGroup {
        <<variant>>
        PluginGroup
        PluginUuidReference
    }

    class ProcessingType {
        <<enumeration>>
        Serial
        Parallel
    }

    class PluginGroupType {
        <<enumeration>>
        Audio
        MIDI
        Instrument
    }

    PluginGroup --> PluginOrGroup : contains
    PluginOrGroup --> PluginGroup : recursive
    PluginGroup --> ProcessingType : uses
    PluginGroup --> PluginGroupType : uses
```

## Signal Flow Patterns

### Serial Processing
```mermaid
sequenceDiagram
    participant Input as Audio/MIDI Input
    participant PG as Plugin Group
    participant Plugin1 as Plugin 1
    participant Plugin2 as Plugin 2
    participant Fader as Group Fader
    participant Output as Final Output

    Input->>PG: Signal In
    PG->>Plugin1: Process
    Plugin1->>Plugin2: Output -> Input
    Plugin2->>Fader: Final Signal
    Fader->>Output: Group Output
```

### Parallel Processing
```mermaid
sequenceDiagram
    participant Input as Audio/MIDI Input
    participant PG as Plugin Group
    participant Layer1 as Layer 1
    participant Layer2 as Layer 2
    participant Fader as Group Fader
    participant Output as Final Output

    Input->>PG: Signal In
    par Parallel Processing
        PG->>Layer1: Process Copy
        PG->>Layer2: Process Copy
    end
    Layer1->>Fader: Layer Output
    Layer2->>Fader: Layer Output
    Fader->>Output: Summed Output
```

## Channel Integration

Each channel contains three main Plugin Group containers:

```mermaid
graph LR
    subgraph "Channel Signal Flow"
        Input[Input Signal] --> MidiFX[MIDI FX Plugin Group]
        MidiFX --> Instrument[Instrument Plugin Group]
        Instrument --> AudioFX[Audio FX Plugin Group]
        AudioFX --> Prefader[Prefader]
        Prefader --> Fader[Channel Fader]
        Fader --> Postfader[Postfader]
        Postfader --> Output[Channel Output]
    end
```

## Real-time Processing

### DSP Graph Integration
```mermaid
graph TB
    subgraph "DSP Graph Processing"
        Input[Audio/MIDI Input] --> PGProcessor[Plugin Group Processor]

        PGProcessor --> Processors[Child Processors]
        Processors --> SerialProcessor[Serial Processor]
        Processors --> ParallelProcessor[Parallel Processor]

        SerialProcessor --> Child1[Child Plugin 1]
        SerialProcessor --> Child2[Child Plugin 2]

        ParallelProcessor --> Parallel1[Parallel Chain 1]
        ParallelProcessor --> Parallel2[Parallel Chain 2]

        Parallel1 --> Chain1[Chain Processor 1]
        Parallel2 --> Chain2[Chain Processor 2]

        Chain1 --> Output1[Chain Output 1]
        Chain2 --> Output2[Chain Output 2]

        Output1 --> Mixer[Signal Mixer]
        Output2 --> Mixer
        Mixer --> Fader[Group Fader]
        Fader --> FinalOutput[Final Output]
    end
```

## QML Integration

```mermaid
graph TB
    subgraph "QML Architecture"
        PluginGroupModel[PluginGroup QML Model] --> ListView[Plugin List View]
        PluginGroupModel --> FaderControl[Fader Control]
        PluginGroupModel --> TypeSelector[Type Selector]
        PluginGroupModel --> ProcessingSelector[Processing Mode]

        ListView --> PluginItem[Plugin Item Delegate]
        PluginItem --> PluginView[Plugin View]
        PluginItem --> NestedGroupView[Nested Group View]

        FaderControl --> FaderSlider[Fader Slider]
        FaderControl --> MuteButton[Mute Button]
        FaderControl --> SoloButton[Solo Button]
    end
```

## Common Usage Patterns

### Simple Plugin with Fader
```mermaid
graph LR
    Plugin[Plugin] --> Wrapper[Plugin Group: Serial]
    Wrapper --> Fader[Fader]
    Wrapper --> Output[Output]
```

### Effect Chain
```mermaid
graph LR
    Input[Input] --> Chain[Plugin Group: Serial]
    Chain --> FX1[Effect 1]
    Chain --> FX2[Effect 2]
    Chain --> Fader[Chain Fader]
    Fader --> Output[Output]
```

### Layered Instruments
```mermaid
graph TB
    Input[MIDI Input] --> Rack[Plugin Group: Parallel]
    Rack --> Layer1[Layer 1: Synth]
    Rack --> Layer2[Layer 2: Sampler]

    Layer1 --> Fader1[Layer Fader]
    Layer2 --> Fader2[Layer Fader]

    Fader1 --> Mixer[Signal Mixer]
    Fader2 --> Mixer
    Mixer --> Output[Output]
```

### Complex Nested Rack
```mermaid
graph TB
    Input[Input] --> MainRack[Main Rack: Parallel]

    MainRack --> Chain1[Chain 1: Serial]
    MainRack --> Chain2[Chain 2: Nested Rack]

    Chain1 --> FX1[FX Plugin]
    Chain1 --> FX2[FX Plugin]

    Chain2 --> SubRack[Sub Rack: Parallel]
    SubRack --> SubChain1[Sub Chain 1]
    SubRack --> SubChain2[Sub Chain 2]

    MainRack --> MainFader[Main Fader]
    Chain1 --> MainFader
    Chain2 --> MainFader

    MainFader --> Output[Output]
```

## Key Benefits

1. **Unified Abstraction**: Single concept covers all container use cases
2. **Recursive Nesting**: Arbitrary depth of nested groups
3. **Processing Flexibility**: Mixed serial/parallel modes
4. **Type Safety**: Clear separation of signal types
5. **Real-time Safety**: Thread-safe processing architecture
6. **UI Integration**: Seamless QML model/view separation

## Migration Strategy

### Phase 1: Basic Integration
- Replace existing plugin lists with Plugin Groups
- Maintain current functionality with single-plugin groups

### Phase 2: Advanced Features
- Implement nested groups
- Add UI for complex routing

### Phase 3: Optimization
- Performance tuning for complex nests
- Advanced UI features
