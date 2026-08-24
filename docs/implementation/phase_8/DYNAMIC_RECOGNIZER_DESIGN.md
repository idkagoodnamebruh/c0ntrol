# Dynamic recognizer design

## Data flow

For each unambiguous nominal LEFT or RIGHT hand, `GesturePipeline` extracts
`HandFeatures` once. The static `GestureEngine` and that hand's
`DynamicGestureRecognizer` consume the same extracted geometry. Dynamic samples
contain stabilized `palmCenter`, `handScale` and `timestampUs`; the recognizer
adds the event's `frameId` and nominal handedness at emission.

Each recognizer owns a timestamp-pruned deque. Displacement is computed from the
oldest and newest samples in the configured duration window and divided by the
median hand scale in that window. Velocity is the dominant normalized distance
divided by elapsed seconds. Horizontal and vertical candidates must also exceed
the configured dominance ratio; ambiguous diagonals remain `NONE`.

## Initial defaults

These are **INITIAL DEFAULTS / HARDWARE TUNING PENDING**:

- minimum distance: 1.25 hand scales;
- minimum velocity: 4.0 hand scales/second;
- direction dominance: 1.5;
- maximum swipe duration: 500,000 us;
- maximum inter-sample gap: 150,000 us;
- cooldown: 400,000 us;
- minimum samples: 3.

They are validated by deterministic geometry tests, not presented as tuned with
a physical camera.

## Safety and lifecycle

Every accepted sample must be finite, have positive finite `handScale`, match
the recognizer's nominal handedness, carry a nonnegative strictly increasing
timestamp and have static pose `OPEN_HAND`. `POINTING`, `PINCH`, invalid geometry
or lost tracking resets history. A gap above `maxSampleGapUs` also resets before
seeding a new sequence. Repeated or regressive timestamps reset safely without
division.

LEFT and RIGHT use separate instances. UNKNOWN hands and duplicate nominal
handedness are treated as missing and cannot transfer history. After one event,
the recognizer discards its window and suppresses further samples until the
absolute cooldown deadline; the first post-cooldown sample starts a new window.
`GesturePipeline::reset()` resets both static FSMs and both dynamic recognizers.

`GesturePipelineResult` reserves 12 event slots. The maximum expected static
plus dynamic output for two hands fits that capacity, and buffer overflow is
observable through `droppedCount` instead of being silent.
