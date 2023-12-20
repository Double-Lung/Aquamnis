#!/bin/bash

pushd ..
premake5 --cc=clang --file=Build.lua gmake2
popd