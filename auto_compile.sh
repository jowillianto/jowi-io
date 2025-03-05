#!/bin/bash
fswatch -0 -o ./src ./tests ./bindings ./pdb.cc CMakeLists.txt | ./compile.sh