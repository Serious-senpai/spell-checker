# spell-checker
[![Compile and run](https://github.com/Serious-senpai/spell-checker/actions/workflows/run.yml/badge.svg)](https://github.com/Serious-senpai/spell-checker/actions/workflows/run.yml)
[![Lint](https://github.com/Serious-senpai/spell-checker/actions/workflows/lint.yml/badge.svg)](https://github.com/Serious-senpai/spell-checker/actions/workflows/lint.yml)

Vietnamese spell checker

## Setup and running
Clone the repository and submodules recursively:
```bash
$ git clone --depth 1 --recursive https://github.com/Serious-senpai/spell-checker
```

Create a Python virtual environment:
```bash
$ python -m venv .venv
```

Run the [build script](/scripts/build.sh):
```bash
$ scripts/build.sh
```

Start the spell checker server:
```bash
$ python main.py
```
