# QtJsonViewer offline installer (Qt Installer Framework)

```
qt_installers/
├── build_installer.sh                  # builds the app + creates the installer
├── config/
│   ├── config.xml                      # installer settings (title, target dir, icons…)
│   ├── QtJsonViewer.png                # installer window icon
│   └── logo.png                        # wizard logo
├── packages/com.rbt.qtjsonviewer/meta/
│   ├── package.xml                     # component description
│   ├── installscript.qs                # desktop entry (Linux) / Start Menu shortcuts (Windows)
│   └── LICENSE.txt                     # MIT license shown in the wizard
└── output/                             # generated installers (git-ignored)
```

## Build

```bash
cd qt_installers
./build_installer.sh                    # version 1.0.0
./build_installer.sh --version 1.2.0    # custom version
./build_installer.sh --clean            # wipe _work/ (full rebuild)
```

Result: `output/QtJsonViewer-<version>-linux-x64-installer.run`

Overridable environment variables:

| Variable      | Default                                                   |
|---------------|-----------------------------------------------------------|
| `QT_DIR`      | `/home/programs/DEV/QtSDK/6.12.1/gcc_64`                  |
| `IFW_DIR`     | `/home/programs/DEV/QtSDK/Tools/QtInstallerFramework/4.11` |
| `APP_VERSION` | `1.0.0`                                                   |

The script builds `src/` in Release, runs `cmake --install` (which also deploys the Qt
libraries/plugins through `qt_generate_deploy_app_script`), bundles `libtbb.so.12`,
fills in the version placeholders in a staging copy under `_work/stage/`, and runs
`binarycreator --offline-only`. The files under `config/` and `packages/` are templates
and are never modified.

## Installing

- GUI: `./QtJsonViewer-1.0.0-linux-x64-installer.run`
- Unattended:
  ```bash
  ./QtJsonViewer-1.0.0-linux-x64-installer.run --root ~/QtJsonViewer \
      --accept-licenses --default-answer --confirm-command install
  ```

Default install location is `~/QtJsonViewer` (or `/opt/QtJsonViewer` when run as root).
A `QtJsonViewer.desktop` launcher, registered for `application/json`, is added to
`~/.local/share/applications`.

Uninstall with `QtJsonViewerMaintenanceTool` in the install directory
(`--confirm-command purge` for unattended removal).
