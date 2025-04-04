#pragma once

#include "utils/traits.h"

class MidiLane;
class AudioLane;
using TrackLaneVariant = std::variant<MidiLane, AudioLane>;
using TrackLanePtrVariant = to_pointer_variant<TrackLaneVariant>;
