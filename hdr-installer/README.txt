Hyprland HDR fix 4 installer

Share install.sh: it contains the PKGBUILD and all three patches.
The separate PKGBUILD and patches directory are included for inspection.

Supported: Arch Linux / CachyOS, x86_64, installed hyprland 0.57.1.
Run as a normal user:
  bash install.sh --check
  bash install.sh
Optional build without installation:
  bash install.sh --build-only
Control parallel compilation (default 2 to limit memory use):
  HDR_JOBS=4 bash install.sh

Prerequisites: base-devel and git; internet access; enough space and RAM
for a full Hyprland build. makepkg asks to install missing build/runtime
dependencies with pacman and asks before installing the completed package.
No backup is created. Temporary build files are removed on exit unless
--build-only succeeds, in which case the printed output directory is kept.

The source is pinned to upstream commit
acc1aa0bfd4bd575dcb0f76c35ccf12b660a1ab9 (v0.57.1).
Submodule udis86 is fetched at that source tree's pinned revision.
Patches are checked before application. A failed build cannot proceed to
package installation. Existing hdrfix4 installations are left alone.
This replaces the system hyprland package; it does not restart the session.
Log out/in to activate the new compositor.

Includes the existing HDR video/reference-white fixes, BT.1886 gamma 2.4
CPU/GPU conversion, SDR brightness mapping on HDR outputs, and capture fixes.
Browser profiles, Firefox/Edge settings and monitor configuration are not edited.
HDR must already be enabled in the user's monitor configuration.

Future package-manager updates may replace this custom build. Updates are
not blocked. A different Hyprland version requires a separately validated port.
This is not a universal installer for Fedora, other releases or architectures.
No uninstall/rollback script is provided because no backup is retained.
To return to your distribution package, use its package manager.

Validation: all three patches apply to the official v0.57.1 source archive;
the five modified color-rendering implementation files match the locally
working hdrfix4 source. That source previously passed 875 unit tests and
6 GPU tests. Installer checks are tested without reinstalling the compositor.
