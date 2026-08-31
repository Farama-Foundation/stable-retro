import json
import subprocess
import sys
from pathlib import Path

REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
VALIDATOR = (
    REPOSITORY_ROOT
    / ".github"
    / "skills"
    / "add-emulator-platform"
    / "scripts"
    / "validate_manifest.py"
)


def platform(extension, library="core"):
    return {
        "lib": library,
        "ext": [extension],
        "keybinds": ["Z"],
        "buttons": ["A"],
        "actions": [[[], ["A"]]],
    }


def write_manifest(path, document):
    path.write_text(json.dumps(document), encoding="utf-8")


def run_validator(manifest):
    return subprocess.run(
        [sys.executable, str(VALIDATOR), str(manifest)],
        capture_output=True,
        check=False,
        text=True,
    )


def test_accepts_unique_repository_mappings(tmp_path):
    write_manifest(tmp_path / "existing.json", {"Existing": platform("old")})
    target = tmp_path / "target.json"
    write_manifest(target, {"Target": platform("new")})

    result = run_validator(target)

    assert result.returncode == 0, result.stderr


def test_rejects_extension_collision_within_target(tmp_path):
    target = tmp_path / "target.json"
    write_manifest(
        target,
        {
            "First": platform("rom", "first"),
            "Second": platform("rom", "second"),
        },
    )

    result = run_validator(target)

    assert result.returncode != 0
    assert "extension 'rom' is already mapped" in result.stderr


def test_rejects_platform_collision_with_sibling(tmp_path):
    write_manifest(tmp_path / "existing.json", {"Duplicate": platform("old")})
    target = tmp_path / "target.json"
    write_manifest(target, {"Duplicate": platform("new")})

    result = run_validator(target)

    assert result.returncode != 0
    assert "platform 'Duplicate' is already defined" in result.stderr


def test_rejects_duplicate_json_keys(tmp_path):
    target = tmp_path / "target.json"
    target.write_text(
        '{"Duplicate":{"lib":"first"},"Duplicate":{"lib":"second"}}',
        encoding="utf-8",
    )

    result = run_validator(target)

    assert result.returncode != 0
    assert "duplicate object key 'Duplicate'" in result.stderr
