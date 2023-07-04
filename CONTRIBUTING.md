# Stable Retro Contribution Guidelines

At this time we are currently accepting the current forms of contributions:

- Bug reports in either the core functionality or game integrations
- Pull requests for core functionality bug fixes

Notably, we are not accepting these forms of contributions:

- New game integrations
- New features

This may change in the future.
In the meantime if you wish to integrate new games you are more than welcome to maintain unofficial repositories of additional games.

## Issue reports

Please include the following information in your issue reports:

- Operating system
- Python version
- Stable Retro version or git commit
- A detailed description of the issue

## Code contributions

Please try to adhere to the existing code style. There is a linter script included at `scripts/lint.sh`.
Before creating a pull request, make sure that your new code does not cause any tests to fail. To run the tests, see the instructions below.

#### Testing on Linux
```bash
sudo apt-get install -y python3-opengl
python3 -m pip install pytest
pytest
```

### Python

Stable Retro is written in a [PEP 8-compliant code style](https://www.python.org/dev/peps/pep-0008/) (minus the line length restriction). Please make sure to maintain this style in any contributions.

### C++

There is a `.clang-format` file that documents as best as possible the code style for Stable Retro. Please make sure to follow it.
