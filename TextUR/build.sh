#!/bin/bash

os_name=$(uname)

if [[ $os_name == *"Darwin"* ]]; then
  additional_flags="-I../../lib"
else
  additional_flags="-I../../lib"
  # export BUILD_PKG_CONFIG_MODULES='openal'
fi

../../lib/Core/build.sh textur "$1" "${additional_flags[@]}"

### Post-Build Actions ###

mkdir -p bin/fonts/
cp ../../lib/Termin8or/fonts/* bin/fonts/

