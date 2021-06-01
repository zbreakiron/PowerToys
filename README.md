# Microsoft PowerToys

<img src="./doc/images/overview/PT%20hero%20image.png"/>

[Downloads & Release notes][github-release-link] | [Contributing to PowerToys](#contributing) | [What's Happening](#whats-happening) | [Roadmap](#powertoys-roadmap)

## Build status

| Architecture | Master | Stable | Installer |
|--------------|--------|--------|-----------|
| x64 | [![Build Status for Master](https://dev.azure.com/ms/PowerToys/_apis/build/status/microsoft.PowerToys?branchName=master)](https://dev.azure.com/ms/PowerToys/_build/latest?definitionId=219&branchName=master) | [![Build Status for Stable](https://dev.azure.com/ms/PowerToys/_apis/build/status/microsoft.PowerToys?branchName=stable)](https://dev.azure.com/ms/PowerToys/_build/latest?definitionId=219&branchName=stable) | [![Build Status for Installer](https://github-private.visualstudio.com/microsoft/_apis/build/status/CDPX/powertoys/powertoys-Windows-Official-master-Test?branchName=master)](https://github-private.visualstudio.com/microsoft/_build/latest?definitionId=61&branchName=master) |

## About

Microsoft PowerToys is a set of utilities for power users to tune and streamline their Windows 10 experience for greater productivity. For more info on [PowerToys overviews and guides][usingPowerToys-docs-link], or any other tools and resources for [Windows development environments](https://docs.microsoft.com/windows/dev-environment/overview), head over to [docs.microsoft.com][usingPowerToys-docs-link]! 

|              | Current utilities: |              |
|--------------|--------------------|--------------|
| [Color Picker](https://aka.ms/PowerToysOverview_ColorPicker) |  [FancyZones](https://aka.ms/PowerToysOverview_FancyZones) | [File Explorer Add-ons](https://aka.ms/PowerToysOverview_FileExplorerAddOns) |
| [Image Resizer](https://aka.ms/PowerToysOverview_ImageResizer) | [Keyboard Manager](https://aka.ms/PowerToysOverview_KeyboardManager) | [PowerRename](https://aka.ms/PowerToysOverview_PowerRename) |
| [PowerToys Run](https://aka.ms/PowerToysOverview_PowerToysRun) | [Shortcut Guide](https://aka.ms/PowerToysOverview_ShortcutGuide) | [Video Conference Mute (Experimental)](https://aka.ms/PowerToysOverview_VideoConference) |
| Espresso (Coming soon in 0.39) |  |  |
## Installing and running Microsoft PowerToys

### Requirements

- Windows 10 v1903 (build 18362) or newer.
   - ⚠️ PowerToys minimum version of Windows 10 is v1903 starting with the 0.37 release
- Have [.NET Core 3.1.14 Desktop Runtime](https://dotnet.microsoft.com/download/dotnet/thank-you/runtime-desktop-3.1.14-windows-x64-installer). The installer should handle this but we want to directly make people aware.

### Via GitHub with EXE [Recommended]

#### Stable version

Install from the [Microsoft PowerToys GitHub releases page][github-release-link]. Click on `Assets` to show the files available in the release and then click on `PowerToysSetup-0.37.2-x64.exe` to download the PowerToys installer.

This is our preferred method.

#### Experimental version
To install the Video Conference mute, please use the [v0.36 experimental version of PowerToys][github-prerelease-link] to try out this version. It includes all improvements from v0.35 in addition to the Video conference utility. Click on `Assets` to show the files available in the release and then download the .exe installer.

### Via WinGet (Preview)
Download PowerToys from [WinGet](https://github.com/microsoft/winget-cli#installing-the-client). To install PowerToys, run the following command from the command line / PowerShell:

```powershell
WinGet install powertoys
```

### Other install methods

There are [community driven install methods](./doc/unofficialInstallMethods.md) such as Chocolatey and Scoop.  If these are your preferred install solutions, this will have the install instructions.

### Processor support

We currently support the matrix below.

| x64 | x86 | ARM64 |
|:---:|:---:|:---:|
| [Supported][github-release-link] | [Issue #602](https://github.com/microsoft/PowerToys/issues/602) | [Issue #490](https://github.com/microsoft/PowerToys/issues/490) |

## Contributing

This project welcomes contributions of all types. Help spec'ing, design, documentation, finding bugs are ways everyone can help on top of coding features / bug fixes. We are excited to work with the power user community to build a set of tools for helping you get the most out of Windows.

We ask that **before you start work on a feature that you would like to contribute**, please read our [Contributor's Guide](CONTRIBUTING.md). We will be happy to work with you to figure out the best approach, provide guidance and mentorship throughout feature development, and help avoid any wasted or duplicate effort.

Most contributions require you to agree to a [Contributor License Agreement (CLA)][oss-CLA] declaring that you have the right to, and actually do, grant us the rights to use your contribution.

For guidance on developing for PowerToys, please read the [developer docs](/doc/devdocs) for a detailed breakdown. This includes how to setup your computer to compile.

## What's Happening

### PowerToys Roadmap

Our [prioritized roadmap][roadmap] of features and utilities that the core team is focusing on.

### 0.37 - April 2021 Update

Our goals for [v0.37 release cycle](https://github.com/microsoft/PowerToys/issues?q=is%3Aopen+is%3Aissue+project%3Amicrosoft%2FPowerToys%2F19) Video Conference Mute work so we can bring it into the stable branch, general bug fixes, moving Keyboard manager out, and removing the legacy settings app.

The 0.36 experimental release was released this month as well which includes Video Conference Mute which is based off the 0.35 code base.

Our [prioritized roadmap][roadmap] of features and utilities will dictate what the core team is focusing on for the near future. 

#### Highlights from v0.37

**General**

- PowerToys now requires Windows 10, version 1903 or higher
- FancyZones editor default launching key is <kbd>Win</kbd>+<kbd>Shift</kbd>+<kbd>`</kbd>
   - Windows Terminal's new Quake mode will use <kbd>Win</kbd>+<kbd>`</kbd>.  We feel this is a far better use of the keystroke. 
   - Current PowerToys users can update this in our settings in the FancyZone section.
- Removed our v1 HTML based settings system

## New Spec - Feedback please!

- What is new in PowerToys (SCOOBE) - [Pull Request](https://github.com/microsoft/PowerToys/pull/10978)

### FancyZones
- Editor UX bug fixes. Thanks [@niels9001](https://github.com/niels9001)
- Monitor resolution is added to the top to directly infer the boxes on top are your monitors
- Fix for editor crash when editing a custom layout

### PowerRename
- Option added for capitalization.  
- Improved loading responsiveness with large sums of files.

### PowerToys Run
- Changed XAML to improve rendering. Thanks [@niels9001](https://github.com/niels9001)
- Disabled plugins are no longer loaded
- VS Code plugin workspaces showing up now.  Thanks [@ricardosantos9521](https://github.com/ricardosantos9521)

### Keyboard manager 
- Now an independent exe.  This now runs high priority in its own process.  When your CPU is under load, this should allow the process to continue to be prioritized

### Color Picker 
- uses a centralized keyhook.  This should improve activation
- Esc for closing will no longer bubble through.  Thanks [@DoctorNefario](https://github.com/DoctorNefario)

### Settings / Welcome to PowerToys
- Shortcuts will stand out more
- Few accessability bugs fixed. Thanks [@niels9001](https://github.com/niels9001)

### Shortcut Guide
- Excluded apps for Shortcut Guide.  Thanks [@davidegiacometti ](https://github.com/davidegiacometti)

### Installer
- new arg for starting PT after silent install

### Developer quality of life
- Ability to directly debug against Settings

## Community contributions

We'd like to directly mention (in alphabetical order) for their continued community support this month and helping directly make PowerToys a better piece of software.  

[@Aaron-Junker](https://github.com/Aaron-Junker), [@addrum](https://github.com/addrum), [@davidegiacometti ](https://github.com/davidegiacometti), [@DoctorNefario](https://github.com/DoctorNefario), [@htcfreek](https://github.com/htcfreek), [@Jay-o-Way](https://github.com/Jay-o-Way), [@niels9001](https://github.com/niels9001), and [@ricardosantos9521](https://github.com/ricardosantos9521)

#### What is being planned for v0.39 - May 2021

For [v0.39][github-next-release-work], we are planning to work on:

- Stability and bug fixes
- Improving VCM
- Moving Shortcutguide out of the main exe

## PowerToys Community

The PowerToys team is extremely grateful to have the [support of an amazing active community][community-link]. The work you do is incredibly important. PowerToys wouldn’t be nearly what it is today without your help filing bugs, updating documentation, guiding the design, or writing features. We want to say thank you and take time to recognize your work.

## Code of Conduct

This project has adopted the [Microsoft Open Source Code of Conduct][oss-conduct-code].

## Privacy Statement

The application logs basic telemetry. Our Telemetry Data page (Coming Soon) has the trends from the telemetry. Please read the [Microsoft privacy statement][privacy-link] for more information.

[oss-CLA]: https://cla.opensource.microsoft.com
[oss-conduct-code]: CODE_OF_CONDUCT.md
[community-link]: COMMUNITY.md
[github-release-link]: https://github.com/microsoft/PowerToys/releases/
[roadmap]: https://github.com/microsoft/PowerToys/wiki/Roadmap
[privacy-link]: http://go.microsoft.com/fwlink/?LinkId=521839
[vidConfOverview]: https://aka.ms/PowerToysOverview_VideoConference
[loc-bug]: https://github.com/microsoft/PowerToys/issues/new?assignees=&labels=&template=translation_issue.md&title=
[usingPowerToys-docs-link]: https://docs.microsoft.com/windows/powertoys/

<!-- items that need to be updated release to release -->
[github-next-release-work]: https://github.com/microsoft/PowerToys/issues?q=is%3Aopen+is%3Aissue+project%3Amicrosoft%2FPowerToys%2F20
[github-prerelease-link]: https://github.com/microsoft/PowerToys/releases/tag/v0.36.0
