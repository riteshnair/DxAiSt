# Third-party notices

## Microsoft DirectX-Headers

- Source: https://github.com/microsoft/DirectX-Headers
- Pinned source commit: `ee479f0bd5f7b884f202bcf0c3f076cc050dd256`
- License: MIT, as provided in `vendor/DirectX-Headers/LICENSE`
- Included material: official DirectX header source and helper headers only.
- Redistribution requirement: retain the copyright and MIT license notice.

## Windows SDK

Traditional Windows SDK headers and libraries are not copied into this repository. The Windows SDK remains a target-machine development prerequisite installed through Microsoft-supported Visual Studio/Windows SDK mechanisms.

## D3D12 operating-system runtime

The system D3D12 runtime is expected to come from Windows. The project does not copy OS-provided D3D12 runtime files into the source package.

## Agility SDK, DXC, DirectStorage, and HIP SDK

These dependencies are consumed through configurable SDK roots and/or their official redistributable packages. No binary is included unless it is obtained from an approved distribution and its license/notice is added here with an exact version and checksum.
