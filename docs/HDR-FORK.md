# Hyprland 0.57.0 HDR backup

This branch preserves the HDR build installed on 3 November 2026:
`hyprland 0.57.0.hdrfix2-1`.

## Source and changes

Base: upstream v0.57.0, commit
`0aa6745f705f2c4b0a5004eb019ef76e5bbdb426`.
HDR fixes: ekremx25/HyprlandX main at
`212f778d28e8e7c41dba957f22793e112c7edf9b`.

- Advertise configured SDR reference white to PQ/HDR clients, including Edge.
- Preserve PQ reference white when converting desktop content to SDR captures.
- Sample external YUV textures through the color-management shader.
- Preserve the existing PQ content-peak tone mapping and optional window saturation.
- Keep upstream proportional blur at bit 14; saturation uses bit 15 and external
  textures bit 16, with 32-bit shader feature storage.
- Retain upstream automatic HDR descriptions and extended YUV format recognition.

The client-facing white adjustment is `sdr_max_luminance * sdrbrightness`, clamped
to the HDR peak. It does not rewrite scanout or work-buffer descriptions.
The capture fix addresses SDR content carried by PQ surfaces; genuine HDR
highlight compression and native HDR PipeWire metadata are not implemented here.

## Library compatibility

The release package is built against Aquamarine 0.16.0 (`libaquamarine.so.15`)
and Hyprgraphics 0.6.0 (`libhyprgraphics.so.5`). Install it together with compatible
system libraries, using a full Arch/CachyOS upgrade transaction when needed.
Do not remove SONAME dependency checks or install this package with older libraries.
The package has no build-directory RPATH or RUNPATH.

## Build

With the runtime and build dependencies listed in `packaging/arch/PKGBUILD` installed:

```sh
git clone --recursive --branch hdr-0.57.0 https://github.com/ekremx25/HyprlandX.git
cd HyprlandX/packaging/arch
makepkg
```

The recipe fetches the immutable `v0.57.0-hdrfix2-1` tag. It is an Arch/CachyOS
package recipe; library SONAME constraints intentionally target this rebuild.
The original binary was built with the new dependency packages staged separately,
without replacing live system libraries during compilation. Rebuilding against
later system dependencies is not guaranteed to produce the identical binary.

## Validation and installation

- 857 of 857 tests passed, including GPU capture and saturation tests.
- Pacman's full-upgrade dependency resolution succeeded with the patched package.
- Installation of the package and new libraries completed successfully.
- After restart, the running compositor reported Aquamarine 0.16.0 and
  Hyprgraphics 0.6.0. Visual HDR confirmation for this rebuild remains user testing.

Standalone `--verify-config` encountered an existing Lua timer-cleanup crash with
the user's current config, also reproduced with the previously installed binary.
An unrelated local minimize plugin failed its ABI check after the upgrade and
was removed at the user's request; it is not part of this backup.

The original archive build reports upstream commit metadata. Use `pacman -Q
hyprland` to distinguish `0.57.0.hdrfix2-1` from an unpatched 0.57.0 package.

Release assets include the installed package, a source patch, and SHA256 checksums.
