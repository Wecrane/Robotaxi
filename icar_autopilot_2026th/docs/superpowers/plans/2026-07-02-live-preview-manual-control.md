# Live Preview and Manual Control Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a production-mode four-panel X11 preview and make Windows manual takeover low-latency and fail-safe.

**Architecture:** Reuse the existing `Show` compositor without enabling debug video playback, producing live Original/Binary/Track/Control panels inside the normal vehicle-control path. Replace event-only manual commands with periodic latest-state transmission and make the C++ server use complete writes, bounded image traffic, joined connection threads, and immediate control-state clearing on disconnect or timeout.

**Tech Stack:** C++17, OpenCV, POSIX TCP sockets, Python 3, unittest/pytest-compatible tests.

---

### Task 1: Testable Windows command scheduler

**Files:**
- Create: `tests/test_manual_client.py`
- Modify: `src/tool/manual_client.py`

- [ ] **Step 1: Write failing tests for key mapping, periodic resend, and STOP priority**

Use fake key-state and fake socket objects to assert `WA\n`, release-to-`STOP\n`, periodic resend within 50 ms, and a queued STOP not being overwritten.

- [ ] **Step 2: Run tests and confirm the current event-only sender fails periodic resend/priority cases**

Run: `python -m unittest tests.test_manual_client -v`

- [ ] **Step 3: Implement a condition-variable latest-state sender**

Keep a dedicated sender thread. Store the latest movement state, wake immediately on changes, resend it every 30 ms as a heartbeat, and keep STOP/RETURN as explicit priority events.

- [ ] **Step 4: Remove avoidable display-loop latency and make shutdown synchronous**

Poll keys at a bounded 5–10 ms interval, send STOP synchronously before socket shutdown, and join sender/receiver threads with bounded waits.

- [ ] **Step 5: Run client tests and syntax checks**

Run: `python -m unittest tests.test_manual_client -v` and `python -m py_compile src/tool/manual_client.py`.

### Task 2: Fail-safe C++ manual-control transport

**Files:**
- Modify: `include/fsm/manualControl.hpp`
- Modify: `src/fsm/manualControl.cpp`
- Create: `tests/manual_protocol_test.cpp`

- [ ] **Step 1: Add a failing protocol/state test harness**

Test complete command parsing for split/combined lines and assert STOP, disconnect, and timeout clear all direction flags and set emergency stop.

- [ ] **Step 2: Add `sendAll` and stop truncating JPEG buffers**

Loop until every byte is written or an error occurs. Encode at bounded quality/resolution; never resize an encoded byte vector.

- [ ] **Step 3: Replace detached per-connection threads with owned lifecycle**

Run command receive in an owned thread, stop it using socket shutdown, join it before accepting another client, and prevent an old connection from touching a new descriptor.

- [ ] **Step 4: Apply immediate fail-safe state transitions**

On receive failure or 300 ms command timeout, atomically clear W/S/A/D, set emergency stop, keep manual takeover active, and ensure `applyManualControl` produces zero speed and centered steering.

- [ ] **Step 5: Build and run protocol tests**

Run the available native test harness, then `cmake --build build --target icar -j 4` in the EdgeBoard-compatible build environment.

### Task 3: Production four-panel live preview

**Files:**
- Modify: `include/utils/show.hpp`
- Modify: `include/icar.hpp`

- [ ] **Step 1: Separate live compositor behavior from debug playback state**

Allow `Show(4)` to compose and refresh a live window without frame index, trackbar, mouse callbacks, or debug pause behavior.

- [ ] **Step 2: Feed four panels from the normal frame pipeline**

After correction, preserve `Original`; after thresholding set `Binary`; after track handling set `Track`; after FSM and control computation create `Control` with track edges, detection boxes, fitted path, speed, and steering overlays.

- [ ] **Step 3: Refresh once at the end of each processed frame**

Call `show()` only after all four panels have been updated. Keep `debug=false`, live camera input, MCU output, and automatic driving behavior unchanged.

- [ ] **Step 4: Verify configuration compatibility**

Confirm missing `showCamera` defaults false and `showCamera=true` creates `ICAR Live` without creating the old raw-only `Camera` window.

### Task 4: End-to-end verification

**Files:**
- Modify: `docs/icar_autopilot_2026th_完全参赛指南与技术手册_V5.md` only if operational instructions are stale.

- [ ] **Step 1: Run all local Python tests and syntax checks**

Run: `python -m unittest discover -s tests -v` and `python -m py_compile src/start.py src/tool/manual_client.py`.

- [ ] **Step 2: Review source diff for unrelated changes**

Run: `git diff --check` and inspect only the planned files.

- [ ] **Step 3: Build on the EdgeBoard**

Run: `cd ~/workspace/icar_autopilot_2026th/build && make -j4`.

- [ ] **Step 4: Perform wheels-up hardware checks**

Verify four live panels, W/S/A/D combinations, release-to-stop, emergency stop, cable/Wi-Fi disconnect stop, RETURN recovery, and camera rotation direction with the drive wheels lifted.

### Task 5: Correctly mounted camera and headless manual client

**Files:**
- Modify: `include/utils/show.hpp`
- Modify: `include/icar.hpp`
- Modify: `include/fsm/manualControl.hpp`
- Modify: `src/fsm/manualControl.cpp`
- Modify: `src/tool/manual_client.py`
- Modify: `tests/test_live_preview_source.py`
- Modify: `tests/test_manual_client.py`
- Modify: `tests/test_manual_server_source.py`

- [ ] **Step 1: Change regression expectations to require no orientation rotation**

Assert that `ROTATE_90_`, `cv2.rotate`, and the manual-client rotation documentation are absent while `predeal->correction(img)` remains present.

- [ ] **Step 2: Run focused tests and confirm they fail on the existing rotations**

Run: `python -m unittest tests.test_live_preview_source tests.test_manual_client -v`.

- [ ] **Step 3: Remove both display-only rotations and restore a landscape X11 window**

Display `imgShow` directly in `Show::show()` and pass decoded JPEG frames through unchanged in the Windows client. Do not modify camera calibration in `Predeal`.

- [ ] **Step 4: Require a headless manual client and no JPEG transport**

Assert the Windows client does not call `cv2.namedWindow`, `cv2.imshow`, or `cv2.waitKey`; assert `Icar::running()` no longer calls `sendImage`; and assert the C++ manual server contains no `IMAGE:`, `imencode`, or image buffer state.

- [ ] **Step 5: Run focused tests and confirm they fail on the existing video UI/transport**

Run: `python -m unittest tests.test_live_preview_source tests.test_manual_client tests.test_manual_server_source -v`.

- [ ] **Step 6: Replace the OpenCV display loop with a keyboard control loop**

Poll `GetAsyncKeyState` every 5 ms, send state changes through the existing heartbeat sender, and print only command changes to the terminal. Keep STOP, RETURN, QUIT, and synchronous shutdown behavior unchanged.

- [ ] **Step 7: Remove manual JPEG production and transport**

Delete `sendImage`, image buffers/locks, JPEG encoding, `IMAGE:` framing, and the client image parser. Retain lightweight `STATE:` parsing for speed and steering diagnostics.

- [ ] **Step 8: Verify locally and on EdgeBoard**

Run all Python tests and syntax checks, upload changed C++ files, perform a clean EdgeBoard build, and confirm the AArch64 `build/icar` executable exists.
