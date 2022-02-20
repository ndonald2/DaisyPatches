# DaisyPatches

My patches for Daisy platform

This is a "soft fork" of [DaisyExamples](https://github.com/electro-smith/DaisyExamples) with
simplifications and modifications to suit my own workflow needs. As such it carries the original MIT
license from DaisyExamples. See that repo's README for additional information.

## Getting Started

1. [Setup your environment](https://github.com/electro-smith/DaisyWiki/wiki/1.-Setting-Up-Your-Development-Environment) for daisy
2. Clone the repo recursively

## Patches

All patches are located in individual directories in `patches/`. To edit or build a patch, open the
patch's folder in VSCode and run the `build all` task at least once to build `libDaisy` and
`daisySP` dependencies. From then on you can just run the `build` task to compile the code or
`build_and_upload_dfu` to program the board.

**Unless otherwise indicated, all patches are for Daisy Patch Submodule (implied to be attached to 
Patch.init() module)**
