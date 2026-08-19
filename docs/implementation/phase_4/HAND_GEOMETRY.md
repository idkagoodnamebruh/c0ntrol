# Hand geometry

## Landmark contract

The extractor consumes the 21 normalized MediaPipe landmarks from `TrackedHand`:

- 0 wrist;
- 1-4 thumb CMC/MCP/IP/TIP;
- 5-8 index MCP/PIP/DIP/TIP;
- 9-12 middle MCP/PIP/DIP/TIP;
- 13-16 ring MCP/PIP/DIP/TIP;
- 17-20 pinky MCP/PIP/DIP/TIP.

World landmarks are optional and are not required by Phase 4.

## Scale and palm reference

The palm center is the mean of landmarks 0, 5, 9 and 17. The normalized hand
scale is:

`handScale = (distance(0, 9) + distance(5, 17)) / 2`.

This combines palm length and width. A non-finite scale or a scale at/below
`1e-6` invalidates the features. The pinch feature is:

`pinchRatio = distance(thumb tip 4, index tip 8) / handScale`.

It is therefore unchanged by uniform scaling. The pointer point is filtered
landmark 8 and remains in normalized camera coordinates.

## Curl metric

For each four-point finger chain, the extractor computes the internal angle at
PIP and DIP with clamped dot products:

`angle(a,b,c) = acos(clamp(dot(normalize(a-b), normalize(c-b)), -1, 1))`.

The continuous metric is:

`curl = clamp(1 - (pipAngle + dipAngle) / (2*pi), 0, 1)`.

A straight chain approaches zero; a folded chain increases toward one. Long
fingers are extended at curl `<= 0.22` and are sufficiently curled for
POINTING at curl `>= 0.38`. Because the metric uses relative vectors, it has no
global X/Y orientation assumption.

## Thumb-specific feature

The thumb is not classified only by the long-finger rule. Its articular curl
uses landmarks 1-4 and is combined with palm-relative spread:

`thumbSpreadRatio = distance(thumb tip 4, palmCenter) / handScale`.

`thumbCurl = clamp(0.75 * articularCurl + 0.25 * foldedIntoPalm, 0, 1)`.

The thumb is extended only when articular curl is at most `0.30` and spread is
at least `0.50`. Distances and angles are mirror invariant, so LEFT and RIGHT
use the same anatomical criteria without a sign inversion.

## Robustness policy

All 21 points must be finite. NaN, Inf, zero scale or a degenerate joint
segment returns `HandFeatures.valid = false`; no gesture is produced. Tests
cover scales 0.5/1/2, rotations 0/45/90 degrees around palm center, mirrored
LEFT/RIGHT thumbs and degenerate inputs.
