<div align = center>

<img src="https://raw.githubusercontent.com/hyprwm/Hyprland/main/assets/header.svg" width="750" height="300" alt="banner">

<br>

[![Badge Workflow]][Workflow]
[![Badge License]][License] 
![Badge Language] 
[![Badge Pull Requests]][Pull Requests] 
[![Badge Issues]][Issues] 
![Badge Hi Mom]<br>

<br>

Hyprland is a 100% independent, dynamic tiling Wayland compositor that doesn't sacrifice on its looks.

It provides the latest Wayland features, is highly customizable, has all the eyecandy, the most powerful plugins,
easy IPC, much more QoL stuff than other compositors and more...
<br>
<br>

---

**[<kbd> <br> Install <br> </kbd>][Install]** 
**[<kbd> <br> Quick Start <br> </kbd>][Quick Start]** 
**[<kbd> <br> Configure <br> </kbd>][Configure]** 
**[<kbd> <br> Contribute <br> </kbd>][Contribute]**

---

<br>

</div>

# HyprlandX HDR fix 4

Fixes washed-out browser video on HDR displays through Hyprland's color pipeline,
including BT.1886 gamma, SDR brightness mapping, reference-white feedback and capture.
Firefox and Edge profiles are not modified.

## Download

- [Standalone install.sh](https://github.com/ekremx25/HyprlandX/releases/download/v0.57.1-hdrfix4-2/install.sh)
- [Installer bundle with patches and build recipe](https://github.com/ekremx25/HyprlandX/releases/download/v0.57.1-hdrfix4-2/Hyprland-HDR-Installer.tar.gz)

The release source archives contain upstream Hyprland 0.57.1 with all three HDR
patches applied, plus the standalone installer and its readable build recipe.
The installer uses pinned upstream source and applies the bundled patches itself.

## Requirements

Arch Linux or CachyOS, x86_64, and an installed `hyprland` 0.57.1 package.
Internet access and sufficient disk space/RAM for compiling Hyprland are required.
Install `base-devel` and `git` first if missing; makepkg prompts for other dependencies.
HDR must already be enabled in your monitor configuration.

## Install

Run as your normal user, without prefixing the script with sudo:

```sh
curl -fL https://github.com/ekremx25/HyprlandX/releases/download/v0.57.1-hdrfix4-2/install.sh -o install.sh
bash install.sh --check
bash install.sh
```

The script verifies the installed version, applies the bundled patches to pinned
upstream 0.57.1 source, builds a package and installs it through pacman.
**No backups are created.** Unsupported versions are rejected; existing HDR fix 4
installations are left unchanged. Log out and back in after installation.

Optional: `bash install.sh --build-only` builds without installing.
`HDR_JOBS=4 bash install.sh` changes the compilation job count (default: 2).

Monitor settings, browser profiles, logins and history are not changed.
Future Hyprland updates may replace the fix; updates are not blocked.
Other Hyprland versions need a separately validated port.

## Source and validation

The [readable build recipe and patches](hdr-installer/) are included alongside the
self-contained script. See [requirements and behavior](hdr-installer/README.txt),
[installer validation](hdr-installer/VALIDATION.txt), and
[HDR implementation details](docs/HDR-FORK.md).

All three patches apply to upstream 0.57.1 and the resulting color-rendering files
match the locally working HDR fix 4 source. That source passed 875 unit tests and
6 GPU tests. The installer was checked without a new full compilation or reinstall.

# Features

- All of the eyecandy: gradient borders, many types of blur, animations, glow, shadows and much more
- A lot of customization
- 100% independent, no wlroots, no libweston, no kwin, no mutter.
- Custom bezier and spring curves for the best animations
- Powerful plugin support
- Built-in plugin manager
- Tearing support for better gaming performance
- Easily expandable and readable codebase
- Fast and active development
- Not afraid to provide bleeding-edge features
- Config reloaded instantly upon saving
- Global keybinds passed to your apps of choice
- Tiling/pseudotiling/floating/fullscreen windows
- Special workspaces (scratchpads)
- Window groups (tabbed mode)
- Powerful window/monitor/layer rules
- Socket-based IPC
- Native IME and Input Panels Support
- Fully dynamic workspaces
- Extensive layout support
    - Dwindle
    - Scrolling
    - Master
    - Monocle
    - Custom layout with Lua
    - Custom layouts with plugins
- Per Workspace Layouts
- and much more...

<br>
<br>

<div align = center>

# Gallery

<br>

![Preview A]

<br>

![Preview B]

<br>

![Preview C]

<br>
<br>

</div>

# Special Thanks

<br>

**[wlroots]** - *For powering Hyprland in the past*

**[tinywl]** - *For showing how 2 do stuff*

**[Sway]** - *For showing how 2 do stuff the overkill way*

**[Vivarium]** - *For showing how 2 do stuff the simple way*

**[dwl]** - *For showing how 2 do stuff the hacky way*

**[Wayfire]** - *For showing how 2 do some graphics stuff*


<!----------------------------------------------------------------------------->

[Configure]: https://wiki.hypr.land/configuring/
[Stars]: https://starchart.cc/hyprwm/Hyprland
[Hypr]: https://github.com/hyprwm/Hypr

[Pull Requests]: https://github.com/hyprwm/Hyprland/pulls
[Issues]: https://github.com/hyprwm/Hyprland/issues
[Todo]: https://github.com/hyprwm/Hyprland/projects?type=beta

[Contribute]: https://wiki.hypr.land/contributing-and-debugging/
[Install]: https://wiki.hypr.land/getting-started/installation/
[Quick Start]: https://wiki.hypr.land/getting-started/master-tutorial/
[Workflow]: https://github.com/hyprwm/Hyprland/actions/workflows/ci.yaml
[License]: LICENSE


<!----------------------------------{ Thanks }--------------------------------->

[Vivarium]: https://github.com/inclement/vivarium
[WlRoots]: https://gitlab.freedesktop.org/wlroots/wlroots
[Wayfire]: https://github.com/WayfireWM/wayfire
[TinyWl]: https://gitlab.freedesktop.org/wlroots/wlroots/-/blob/master/tinywl/tinywl.c
[Sway]: https://github.com/swaywm/sway
[DWL]: https://codeberg.org/dwl/dwl

<!----------------------------------{ Images }--------------------------------->

[Preview A]: ./assets/prev1.png
[Preview B]: ./assets/prev2.png
[Preview C]: ./assets/prev3.png


<!----------------------------------{ Badges }--------------------------------->

[Badge Workflow]: https://github.com/hyprwm/Hyprland/actions/workflows/ci.yaml/badge.svg

[Badge Issues]: https://img.shields.io/github/issues/hyprwm/Hyprland
[Badge Pull Requests]: https://img.shields.io/github/issues-pr/hyprwm/Hyprland
[Badge Language]: https://img.shields.io/github/languages/top/hyprwm/Hyprland
[Badge License]: https://img.shields.io/github/license/hyprwm/Hyprland
[Badge Lines]: https://img.shields.io/tokei/lines/github/hyprwm/Hyprland
[Badge Hi Mom]: https://img.shields.io/badge/Hi-mom!-ff69b4
