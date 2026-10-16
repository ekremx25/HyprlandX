# HDR changes in this fork

This fork carries external YUV texture sampling through the color-management
shader, PQ content-peak tone mapping, an opt-in per-window `saturation` rule,
and SDR reference-white feedback for HDR clients such as Chromium and Edge.

## SDR reference white

A color-managed browser can submit ordinary SDR pages in a BT.2020/PQ surface.
That surface does not go through the compositor's SDR-to-HDR brightness adjustment.
Previously, the Wayland color-management protocol reported a fixed 203 nit
reference white even when the configured SDR white was different.

For PQ outputs with a linear FP16 working buffer, the output description and
preferred surface description now report:

```
SDR reference white = sdr_max_luminance * sdrbrightness
```

For example, 251 nit with a brightness multiplier of 1.2 is advertised as 301 nit
(the protocol uses whole nits). The result is bounded by the target peak. Changes
to these monitor settings notify connected clients.

The adjustment only changes client-facing reference-white metadata. The scanout
and working-buffer descriptions, HDR transfer function, mastering primaries and
peak luminance remain unchanged. SDR/ICC outputs, HLG, and the legacy non-linear
working-buffer path retain their existing behavior. No new option is required.
SDR saturation is a separate appearance adjustment and is not represented by
reference-white metadata.

## SDR screenshots and PipeWire capture

PQ surfaces now use their own reference white when producing the SDR mirror
copy. Previously this path divided by a fixed 80 nit white, washing out PQ
browser content at 203 or 301 nits. The mirror is generated before display tone
mapping; scanout retains its existing tone mapping and HDR luminance.

This corrects SDR desktop content carried inside PQ surfaces. It does not add
highlight compression for genuine HDR content above reference white, or change
the capture protocol to deliver HDR metadata. Selecting Rec.2100 PQ in OBS does
not by itself make its PipeWire source a native HDR source.

GPU regression tests exercise the production shader with 80, 203, and 301 nit
reference whites, colored pixels, premultiplied alpha, unchanged SDR capture,
and unchanged HDR display output. All 676 tests passed. A real Edge PQ surface
captured with grim reproduced the issue before the fix: #808080 became
#e9e9e9. With the fix it captured as #808080; primary color samples were within
one 8-bit code value of the original page.

## Validation and versions

The fix was first validated on a patched v0.56.2 desktop: the user confirmed that
Edge's washed-out appearance was resolved after restarting the compositor.
A separate virtual HDR output verified reference-white updates, and a real Edge
client continued to submit PQ surfaces using the updated reference white.

This branch ports that fix onto this fork's newer `main` renderer. It also
includes unit tests for metadata preservation, reference changes, target-peak
clamping, invalid inputs, and unchanged SDR/HLG/ICC descriptions.

The main-based port passed all 673 unit tests. A separate running compositor
verified 301 -> 251 -> 301 nit protocol updates while keeping the HDR peak at
486 nit. An actual Edge client received 301 nit white and continued submitting
PQ surfaces with matching reference-white and content-peak metadata.

Build with the dependencies documented by Hyprland:

```sh
git submodule update --init --recursive
bash scripts/generateShaderIncludes.sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DWITH_TESTS=ON
cmake --build build --target Hyprland hyprland_gtests --parallel
./build/hyprland_gtests
```

The running v0.56.2 package and this main-based source tree are different builds.
Before installing a main-based build, validate your configuration with
`./build/Hyprland --verify-config --config /path/to/hyprland.lua`. In particular,
this newer base limits `decoration.shadow.render_power` to 4 and
`decoration.blur.vibrancy` to 1.0. Configuration reload does not replace the running
compositor binary; installing a new build requires a new session to activate it.
