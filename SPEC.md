\# PROJECT SPECIFICATION: VALORANT BIOMECHANICAL SENSITIVITY FINDER (C++ / WINDOWS)



\## 1. TECH STACK \& ARCHITECTURE

\- \*\*Language:\*\* C++20, built with CMake on Windows (MSVC/MinGW).

\- \*\*Graphics \& 3D Engine:\*\* Raylib (for 3D camera, primitive rendering, ray-sphere/ray-capsule collision) + Dear ImGui (for HUD, settings, and analytics charts).

\- \*\*Input System:\*\* Native Win32 Raw Input API (`WM\_INPUT`, `RIDEV\_INPUTSINK`) to read unaccelerated `lLastX` and `lLastY` directly from hardware.



\## 2. VALORANT 1:1 PHYSICS \& CAMERA CONSTANTS

\- \*\*Scale:\*\* 1.0 world unit = 1.0 cm.

\- \*\*Camera:\*\* Horizontal FOV = 103.0 degrees (16:9). Camera eye-height = 160.0 cm.

\- \*\*Rotation Formula:\*\*

&#x20; - `delta\_yaw = -raw\_dx \* sens \* 0.07f` (degrees)

&#x20; - `delta\_pitch = -raw\_dy \* sens \* 0.07f` (clamped between -89.0f and 89.0f)

&#x20; - `cm\_per\_360 = (360.0f \* 2.54f) / (dpi \* sens \* 0.07f)`

\- \*\*Agent Dimensions \& Physics:\*\*

&#x20; - Total height: 180 cm. Head hitbox: Sphere (center Y = 165.0 cm, radius = 11.0 cm for Micro-Adjustment and Target Switching, 24.0 cm for Medium Flicking, and 15.0 cm for Tracking). Body hitbox: Capsule (radius = 22.0 cm, height = 140.0 cm).

&#x20; - Strafing speed (Rifle): 540.0 cm/s. Acceleration/Deceleration (Counter-strafe): 3000.0 cm/s² (stops in 180 ms from 540.0 cm/s at 3000.0 cm/s²).

&#x20; - Environment: Standard range room with a horizontal reference line at Y = 165.0 cm (head-level). Targets spawn at 1500 cm to 2500 cm distance.



\## 3. THE 4 CORE AIM BENCHMARKS

All angular metrics are converted to 3D world positions at distance `D` (default 2000 cm).



1\. \*\*Micro-Adjustment (Weight: 0.40):\*\*

&#x20;  - 25 targets. Target radius = 11 cm (\~0.63° at 20m).

&#x20;  - Spawns sequentially within 1.5° to 4.0° from the previous click position (70% biased along horizontal yaw axis).

&#x20;  - Measures: Finger stability, jitter, Effective Width ($W\_e$).

2\. \*\*Medium Flicking (Weight: 0.30):\*\*

&#x20;  - 25 trials. Requires shooting a Center Anchor (0°, 0°) before each target spawns at 12° to 25° away. The anchor and the main target both use a 24.0 cm sphere radius.

&#x20;  - Measures: Ballistic overshoot/undershoot (`max\_proj` along movement vector) and Fitts's Throughput.

3\. \*\*Target Switching (Weight: 0.20):\*\*

&#x20;  - 10 clusters of 3 targets (30 targets total, 1-tap kill).

&#x20;  - Cluster spawns 15°–25° from crosshair; targets within cluster are 4°–8° apart.

&#x20;  - Measures: Inter-kill delay ($\\Delta t$) and Path Efficiency ($D\_{ideal} / D\_{actual}$).

4\. \*\*Smooth \& Reactive Tracking (Weight: 0.10):\*\*

&#x20;  - 20 seconds continuous tracking on a moving agent capsule (head + body). The Tracking head is a sphere with radius 15.0 cm (30.0 cm diameter), centered at Y = 165.0 cm before any jump.

&#x20;  - On-target color feedback: While the crosshair ray intersects either the head sphere or the body capsule, render the full agent in lime green (RGB 50, 220, 80). Restore the default warm color as soon as the ray leaves both hitboxes.

&#x20;  - First 10s (Smooth): Constant strafe at 540 cm/s within ±15° bounds.

&#x20;  - Last 10s (Reactive): Random direction reversals every 0.8s–1.6s with instant counter-strafe deceleration (3000 cm/s²) and occasional jumps.

&#x20;  - Measures: Time-on-Target (%), reaction delay on direction change, and hand tremor jitter.



## 4. ADAPTIVE OPTIMIZATION ALGORITHM (CLOSED-LOOP)

Each Auto-Calibration session runs five or six rounds. A round runs Condensed Micro-Adjustment (10 targets), Medium Flicking (10 targets and their center anchors), Target Switching (4 clusters of 3 targets) and Smooth & Reactive Tracking (5 seconds per phase), in that order. There is a three-second countdown before each round and a two-second break between tests. Each test reuses its random seed within a session to reduce variation in scenario difficulty. Target positions that depend on the player's aim can still vary.

### Per-target measurements

- A target contributes one acquisition time, from activation to the final hit. Throw out acquisitions whose MT differs from the test median by more than `max(2.5 × MAD, 0.05 × median(MT))`. Accuracy is the first-shot hit rate among remaining targets. Center anchors prepare Medium Flicking trials and do not contribute to scoring.
- Given ideal angular displacement `a`, use `D = ||a||` and `u = a / D`. The projected angular displacement is `d_proj = (click_angle − start_angle) · u`. Endpoint error is `d_proj − D`. Track the maximum projection at Raw Input cadence.
- Compute `De = mean(d_proj)` using successful acquisitions and `We = max(4.133 × sample_SD(endpoint_error), 0.25 × median(target_width))`. The target-width floor stabilizes very small samples. Compute `TP = log2(1 + De/We) / mean(MT)` in bits per second. At least two valid acquisitions are required for TP.
- Each retained target contributes one signed bias signal. At its first shot, set `e = (d_proj − D)/W` and `p = (max_proj − D)/W`, with `W` the angular diameter. Start with `sign(e) × clamp((abs(e) − 0.10)/0.90, 0, 1)`. If `abs(e) <= 0.5`, add `0.25 × clamp(p − 0.5, 0, 1)` for a corrected peak. Clamp the resulting target signal to [−1, 1] and compute `Bias = tanh(1.5 × mean(target_signals))` for each test. The round bias uses the normalized Micro, Flick and Switch weights for tests with retained data.

### Scoring and candidate search

- For the three click tests, `Score = 100 × first_shot_accuracy × TP/(TP+5)`. Tracking uses `100 × time_on_target_fraction`, with jitter penalty `0.10 / J` when there is at least 0.75 seconds of valid data and perpendicular jitter `J` exceeds 0.10° RMS. Excess jitter at or above 350 eDPI locks further increases.
- Composite Score = `0.40 × Micro + 0.30 × Flick + 0.20 × Switch + 0.10 × Tracking`. A sensitivity's standing uses its mean Composite Score over repeated rounds, with a two-point selection penalty while it has only one measurement.
- Round 1 tests the starting sensitivity. Rounds 2 and 3 probe `S0 × exp(±0.06)` in log space. Bias above 0.08 puts the lower probe first. eDPI stays within [150, 600] and the jitter cap is respected.
- Round 4 retests the leading candidate. Round 5 verifies the best remaining distinct contender when available. If a further round is needed, round 6 retests the current leader. If round 5 throughput falls more than 10% below the selected candidate's reference round and round 6 returns to that candidate, the dashboard marks backtracking.
- Stop after round 5 when the leader has at least two measurements spanning no more than three score points, its mean absolute directional bias (absolute value of mean bias) is below 0.08, and it leads the next candidate by at least two points with both candidates verified. With only one eligible candidate, there is no runner-up requirement.
- The final recommendation prioritizes verified, eligible sensitivities and ranks them by mean Composite Score. A higher sensitivity can win when its measured results are stronger.

