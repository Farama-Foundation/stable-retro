#!/usr/bin/env python
import subprocess
import sys

import pytest

import retro.testing as testdata

if len(sys.argv) == 2:
    branches = [sys.argv[1]]
elif len(sys.argv) == 3:
    branches = [sys.argv[1], sys.argv[2]]
else:
    branches = ["master"]

check = testdata.branch_new(*branches)
if check:
    args = ["-q", "--tb=no", "--disable-warnings", "-k", " or ".join(check)]
    pytest.main(args)
    for context, error in testdata.errors:
        print(f"\33[31mE: {context}: {error}\33[0m")
    for context, warning in testdata.warnings:
        print(f"\33[33mW: {context}: {warning}\33[0m")
    if testdata.errors:
        sys.exit(1)
