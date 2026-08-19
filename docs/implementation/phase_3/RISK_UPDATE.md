# Phase 3 risk update

| Risk | State | Phase 3 evidence |
|---|---|---|
| R-001 real landmark source | RESOLVED (Phase 2) | backend/model not changed |
| R-002 MediaPipe backend | RESOLVED (Phase 2) | backend/bridge not changed |
| R-006 model compatibility | RESOLVED (Phase 2) | model/pin not changed |
| R-008 shared OneEuro state | **RESOLVED** | independent scalar arrays for every axis, landmark, coordinate space, and named hand; isolation/order-swap tests pass |
| R-009 derivative cutoff unused | **RESOLVED** | canonical derivative low-pass calls `alpha(derivativeCutoff, dt)`; behavioral dCutoff test passes |
| R-010 gesture orientation | OPEN | gesture semantics intentionally unchanged |
| R-011 pinch normalization | OPEN | gesture thresholds intentionally unchanged |
| R-012 gesture FSM | OPEN | not started |
| R-013 native OS input | OPEN | not started |
| R-017 tracking contract | RESOLVED (Phase 2) | raw contract preserved and filtered copy added |
| R-025 blocking camera/VIDEO | OPEN | filter does not alter capture/inference scheduling |

Remaining tuning risk: `minCutoff=1.0`, `beta=0.05`, derivative cutoff `1.0`, timeout 400 ms, and teleport threshold 0.35 are tested initial defaults but have not been visually tuned on live hardware.
