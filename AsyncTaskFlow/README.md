![C++](https://img.shields.io/badge/C++-20-blue.svg)
![License](https://img.shields.io/badge/License-MIT-green.svg)
![CMake](https://img.shields.io/badge/CMake-3.12+-brightgreen.svg)

# AsyncTaskFlow

A lightweight concurrent task scheduler with dependency resolution using modern C++20 features.

## Features

- Thread pool with efficient task scheduling
- Directed Acyclic Graph (DAG) for task dependencies
- Modern C++20 features (concepts, templates, lambdas)
- Exception-safe and RAII compliant

## Building

```bash
mkdir build
cd build
cmake ..
make