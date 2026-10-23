# Android bring-up - 2026-10-15

Target: original PC speed.exe SHA-256 80774c2e5d619b4f120b48d4462896fd504c263399d203a238769cffde1d253c.

## Runtime and input

D3DX qsort comparators use cdecl. The host removes both callback arguments after each comparison; startup checks sorting, duplicates, preserved registers and stack balance. Android keyboard/mouse Acquire uses the default nonexclusive background cooperative level, as verified against real Windows DirectInput.

Most Wanted polls raw keyboard snapshots. Its GameFlow state 6 selects driving controls. Gas is Up, brake/reverse Down, handbrake Space, nitrous Left Alt, shifts Left Shift/Control, reset R, camera C. The menu X sends **Esc (DIK 0x01)**. The user confirmed gas and X work after installing the updated APK. Menu touches include a visible pointer.

All three PC ports distinguish physical gamepad sources and joystick devices with both X/Y axes from fingerprint sensors advertising only a Z axis. Connecting a controller hides touch controls; touching the screen reveals them. Repeated device enumeration no longer hides them again after that touch. Bluetooth hardware testing remains pending because no controller was available.

## Proportional rendering

The earlier camera/FOV and motion-blur experiments were removed after a white-screen/fisheye regression. The current adaptation changes engine dimensions and HUD coordinates while retaining the original camera and post-processing instructions. The user confirmed the corrected image on the S25.

Resolution returns use the launcher-selected render dimensions. The 720p preset preserves the display aspect ratio (1560x720 on a 2340x1080 display). HUD horizontal scale/center, frontend origins and mouse coordinates use the same aspect ratio, based on [ThirteenAG's WidescreenFix](https://github.com/ThirteenAG/WidescreenFixesPack/tree/master/source/NFSMostWanted.WidescreenFix). See THIRD_PARTY_NOTICES.md. The x86 ASI itself is not loaded on ARM64. The in-game selector still returns launcher-selected dimensions; separate selectable in-game modes are not implemented.

The independent depth-reference correction remains: MW copies its initial depth surface from 0x982A2C to descriptor 0x93DAC4 without AddRef and later releases both. The bridge retains the descriptor reference.

## Race crashes

A computed tail jump inside CRT memmove (0x7C4840, missed interior target 0x7C4AFC) is replaced with native memmove using the original cdecl ABI. tests/memmove_override.c checks 4,825 overlap/alignment/length and zero-length cases, guards, EAX and ESP.

The later null callback at 0x826749 traced to sound gain routine 0x81CDC6. At 0x81CE0E it compares a channel index with -1, then executes FUCOMPP before the integer JNE at 0x81CE1C. The shared lifter incorrectly treated FUCOMPP as an integer flag setter. An all-channel update then became a single-channel write at voice + 0x13C - 0x24 = voice + 0x118, corrupting the effects descriptor. The logged 0x432F4047 was a floating-point gain, not a callback address.

scripts/mw_lifter.py preserves integer flags for legacy x87 compares and FTST. FNSTSW still exposes the x87 result, and TEST/SAHF still explicitly transfer it to integer flags. FCOMI/FUCOMI variants keep their existing separate behavior. This correction is local to this project and does not edit the sibling pcrecomp checkout.

Both full generation and incremental relifting use the corrected lifter. To repair an existing local translation:

```powershell
python scripts/relift_hooks.py --x87-flags
```

Deleting hooks also removes their previously generated sites. Generated C remains local and ignored.

In a VS developer prompt, with capstone, pefile and Unicorn installed:

```powershell
python tests/sound_channel_flags.py
# Optional: reproduce the old lifter's known corruption too.
python tests/sound_channel_flags.py --prove-old-bug
```

The test compares the actual locally supplied sound routine with x86 execution over 16 channel/gain combinations, including -1, checking all general registers, stack cleanup, the complete voice object and surrounding guards. The old lift reproduces the overwritten descriptor; the corrected lift matches all cases.

## Online / LAN

The Android Winsock adapter now implements event creation/reset/set/close/waits and WSARecv/WSARecvFrom/WSAGetOverlappedResult, including pending scatter receives, event signaling, completion bytes/flags and cancellation on socket close. Pending operations are pumped nonblockingly during guest waits and scheduler yields.

A startup loopback regression exercises the actual HLE calls and their guest stack cleanup: timeout, event signal/reset, pending UDP scatter receive, incomplete/completed results and socket-close cancellation. This passed on the S25. Opening network menus reached a game lobby without the previous missing-WSAWaitForMultipleEvents crash.

The backend defaults to eahub.eu with EA-host DNS redirection. DNS resolution, backend TCP connection and a visible lobby do **not** prove successful community-server login or an Internet race. A real remote session remains unverified. Receive completion routines/APCs and overlapped ICMP receives return unsupported errors; the tested game path uses events.

## Final validation

Android builds for all three ports and the MW Windows host build pass. The
updated APKs install over the existing S25 apps without clearing their data.
The MW startup Winsock regression passes, including two independent events.
The updated game reaches a quick race, accelerates and progresses along the
track; nitrous sends DIK 0x38 and pause/back sends DIK 0x01. A brief driving
test did not reproduce the null sound callback. Sustained play and a complete
race remain to be tested. The game was left paused after the test.

No game files, generated translations or logs are included in source control.
Release APKs are published separately as GitHub release assets.


## Redmi Note 8 follow-up (2026-10-15)

Leaving Online reached speed.exe 0x7EC268, which calls the previously missing
kernel32!TerminateThread. The Android HLE now requests cooperative thread
termination, unwinds at scheduler/network-wait checkpoints, preserves the
requested exit code and signals thread completion through existing waits.
It does not forcibly cancel the host pthread. The device startup regression
passed blocked, running, suspended, self, already-completed, invalid-handle,
exit-code and guest ABI checks. The user confirmed that exiting Online works.
This confirmation does not establish that an Internet race is functional.

The launcher adds aspect-preserving 480p and 360p presets. On the Redmi's
2340x1080 display these render at 1040x480 and 780x360. The previous default
was 1756x810 (75%), independently of the game's low detail settings. Captured
3D menu samples at that default were about 13-16 FPS, with Present taking
34-53 ms. At 780x360, menu samples reached about 40-46 FPS; gameplay samples
varied around 17-39 FPS depending on the scene, with Present near 1 ms.
These are observed presentation rates across different scenes, not a controlled
race benchmark or a guarantee of sustained performance. CPU-side guest work
and render submission remain limiting factors after reducing GPU pixel load.

See [Audio and frame pacing follow-up](AUDIO_FRAME_PACING.md) for the subsequent
shared audio changes and outstanding device testing. The last audio revision
was compiled for review; it has not yet been validated on tester hardware.


The subsequent audio/frame-pacing release is documented in [AUDIO_FRAME_PACING.md](AUDIO_FRAME_PACING.md): ARM64 gameplay was exercised on the Redmi, both ABI APKs compile, and the final ARM64 releases install on the S25. Physical ARMv7 testing remains pending because only the ARM64-only S25 was connected at release time.


## Lobby disconnect event race (2026-10-15, v0.1.3)

An S25 session reproduced `Guest integration boundary: map::at: key not found`
after WSACloseEvent during lobby teardown. The user reported the same failure
on a Redmi Note 8, including the other phone when a player leaves before a
race starts. Its preserved log independently showed the same boundary at
16:46:28 and 17:36:55. Android recorded no native process crash: the runtime caught the
exception and stopped guest execution.

WaitForMultipleObjects originally validated handles only before entering its
wait loop. A different guest thread could close an event while the waiter
released the machine. The next iteration treated the missing event as a
thread and called threadHandles.at(handle), which threw std::out_of_range.

The wait now checks every object's state on every iteration, before acquiring
any ready object. A handle closed during a wait returns WAIT_FAILED with
ERROR_INVALID_HANDLE (6), rather than stopping guest execution. A handle
already invalid on entry still fails immediately. Windows documents closing
handles during a pending wait as undefined; this is a controlled failure for
that race, not a claim of defined Windows close-during-wait semantics.

The startup Winsock regression now closes an event through WSACloseEvent on
another native thread while WSAWaitForMultipleEvents is blocked. All four
combinations of wait-any/wait-all and first/last closed event return failure,
preserve the surviving auto-reset signal, and maintain the five-argument
32-bit guest stack ABI. The updated ARM64 build passed these checks on S25 and Redmi Note 8.
Full remote-race completion and physical ARMv7 testing remain separate checks.

API reference: [Microsoft WSAWaitForMultipleEvents](https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-wsawaitformultipleevents).


### Follow-up: peer connection remains under investigation

The user confirmed that the two phones no longer show the previous error,
but both wait for players to connect. This is not proof of a working online
race, and v0.1.3 release notes explicitly preserve that limitation.

A separate defect was found: NetBIOS NCBASTAT synthesized its MAC by hashing
hostGameRoot. Both phones use /storage/emulated/0/nfsmw, so both received the
same machine identity. Game code at 0x0083CBF0 invokes NetBIOS to read the
adapter status during network initialization; the device logs confirm those
calls. This collision is real, but it has not been established as the only
cause of the stalled peer connection.

The Android activity now supplies a random installation ID from private
SharedPreferences before NativeActivity starts its native thread. It survives
updates and switching APK ABI; app backup is disabled. The runtime exposes
its lower 46 random bits as a locally administered unicast MAC. It does not
read the hardware MAC or a platform device identifier. UDP overlapped receives
now contribute to the existing per-peer diagnostics, including the first
received packet and periodic byte/packet counts; payloads are not logged.

The actual NetBIOS HLE regression passed on Redmi Note 8: repeated adapter
status reads, the local/unicast bits, 60-byte status length, zero-filled tail,
surrounding buffer guards and guest ABI. The persisted installation identity
was checked to differ from the old shared path identity. ARM64 builds and the
new event-close regression also passed. This candidate was installed on the
Redmi; the S25 was disconnected by then. Repeat peer connection remains
pending. These follow-up changes are not part of the v0.1.3 release tag.

The first instrumented Redmi session received UDP from the S25 at the LAN
address on ports 3658 (8-byte datagrams) and 9999 (384-byte discovery packets).
At 17:48:05 a wait observed a closed event, returned ERROR_INVALID_HANDLE,
and subsequent frames continued without the previous integration boundary.
This demonstrates receive delivery and controlled teardown, not a completed
peer handshake or race. The ARMv7 candidate also compiled successfully.


### LAN server integration and two-phone validation

The previous APK had no generated server.dll translation and no server.dll
entry in guestModuleSpecs. LoadLibrary returned zero, so creating a LAN
server silently failed. Underground 1 and 2 both include their own lifted
server.dll at 0x00A00000; Most Wanted requires a different image and base.
Its DLL is now translated at 0x00E00000 (preferred base 0x10000000, image size
0x6F000), after speed.exe and d3dx9_26.dll and below both ABI heap arenas.

scripts/prepare_server.py reproduces the relocation, seed collection,
catalog and prefixed lift from a locally owned DLL. The catalog/lift has
1770 functions and zero lift failures; ten unsupported instructions remain
in discovered paths, so this is not a claim of complete instruction coverage.
CMake now fails if the server translation is absent instead of producing an
APK that cannot host LAN. The module uses the existing validated DLL loader,
relocations, import binding, DllMain and export calls.

The first S25 host test executed the real DLL and exposed Winsock ordinal
151, __WSAFDIsSet. This helper is now implemented for the Windows fd_set
layout and opaque guest SOCKET handles, and tested through the actual HLE
ABI for first/middle/last hits, misses, empty sets and input preservation.
All 35 Winsock imports used by server.dll have mappings. The Redmi also
lacked server.dll/server.cfg; these were copied from the user's local game
without changing the original files or saved profiles.

With the corrected ARM64 build on both phones, the LAN server loaded, read
server.cfg, listened at 192.168.88.164:9900 and accepted a client. Both phones
reached the same grid with two players and a countdown; the user confirmed
LAN worked. Captured gameplay traffic on UDP 3658 flowed in both directions
at roughly 65-75 packets per five seconds per phone. This validates actual
LAN gameplay, not just a visible lobby. A completed race was not recorded.

Online remains separate: both devices advertise the same public IPv4/3658.
One phone's peer channel timed out while the other kept receiving sequence/
acknowledgment packets even after that channel had closed. This suggests a
same-public-endpoint/NAT loop rather than proof of a completed peer link.
Testing with distinct Internet connections is pending user authorization;
mobile data is not enabled or switched automatically.

The user subsequently reported a VPN on the S25. A fresh connectivity
snapshot confirms Proton VPN, tun0, with the game UID inside its UID range
and the local Wi-Fi subnet excluded from tunnel routes. Its connection
timestamp is 18:08:58 local, after the captured peer attempt started at
18:06. That attempt therefore cannot establish whether a newly created
session works through the VPN. Keep the VPN enabled and restart both game
processes before comparing new peer endpoints; do not attribute the
remaining Online failure to the router or VPN without that test.

Both v0.1.4 ABI builds including the server module compile successfully.
The ARM64 candidate was installed on both phones for the fresh VPN test;
the ARMv7 candidate has not yet been physically validated.

### Online race reached with the Redmi hosting

After restarting both games with the S25 VPN retained, the user created
the Online session on the Redmi Note 8. Both phones reached the same race
with two Golf cars and the two Online account handles on the scoreboard.
Screenshots captured the race start and lap 1/3, with complementary first/
second positions and the other player's car visible. This establishes
actual Online gameplay for this configuration, beyond lobby discovery.

At 18:13:49-56 local, the S25 sent 72-73 and received 82-87 peer packets per
five-second reporting interval, while the Redmi sent 82-89 and received
72-73. Peer traffic advanced from the eight-byte handshake into larger
gameplay packets. No integration boundary error appeared in this attempt.
The existing endpoint logs still showed the same public IPv4/3658 on both
phones, so that tuple alone is insufficient evidence of a self-loop; the
earlier NAT-loop interpretation remains an unproven hypothesis, not a
confirmed cause. The test also changed host and restarted both processes,
so it does not isolate the VPN as the cause of success. A completed race,
S25-hosted Online session and chat delivery remain to be validated.

The user subsequently confirmed Online was working and requested source
publication on main plus both APK architectures as a GitHub release.

## Camera FOV on wide phones (#127, #167)

The projection at 0x6CF400 derives the vertical angle from the horizontal one times the viewport height/width (Vert-), so on 20:9 phones the car filled the screen and the hood camera showed no hood. Four hooks there (config/hooks.txt, `fix_fov_view` in src/runtime/widescreen.c) apply ThirteenAG's FixFOV with Scaling = 1: Hor+ for the player views 1 and 4, 0.4 for the rear-view mirror (view 3), reflections unchanged. Unlike the removed experiment, the x87 stack is only rescaled at the mod's three constants, not rewritten.

## Audio on MediaTek, Mali and Unisoc phones

The AAudio buffer was two bursts (4-10 ms). Those drivers report no xruns, so it never grew, and any render or guest-lock hiccup broke the sound. It now starts at 40 ms, grows to 100 ms on reported underruns, and falls back to a non-low-latency stream when the low-latency one cannot hold 40 ms. Streaming loops (music, engine mix) the guest stops refilling are muted after one full ring instead of repeating it.
