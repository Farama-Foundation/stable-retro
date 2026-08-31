#!/usr/bin/env python3

import argparse
import json
from pathlib import Path


class ManifestError(ValueError):
    pass


def require(condition, message):
    if not condition:
        raise ManifestError(message)


def load_manifest(path):
    try:
        with path.open(encoding="utf-8") as manifest_file:
            document = json.load(manifest_file)
    except (OSError, json.JSONDecodeError) as error:
        raise ManifestError(f"cannot read {path}: {error}") from error

    require(
        isinstance(document, dict) and document,
        "manifest must be a non-empty object",
    )
    return document


def validate_string_list(value, location, allow_null=False):
    require(isinstance(value, list), f"{location} must be an array")
    for index, item in enumerate(value):
        valid = isinstance(item, str) and bool(item)
        require(
            valid or (allow_null and item is None),
            f"{location}[{index}] must be a non-empty string"
            + (" or null" if allow_null else ""),
        )


def validate_platform(platform, config):
    location = f"platform {platform!r}"
    require(
        isinstance(platform, str) and platform,
        "platform names must be non-empty strings",
    )
    require(isinstance(config, dict), f"{location} must contain an object")

    library = config.get("lib")
    require(
        isinstance(library, str) and library,
        f"{location}.lib must be a non-empty string",
    )

    extensions = config.get("ext")
    validate_string_list(extensions, f"{location}.ext")
    require(
        len(extensions) == len(set(extensions)),
        f"{location}.ext contains duplicates",
    )
    for extension in extensions:
        require(
            extension == extension.lower(),
            f"extension {extension!r} must be lowercase",
        )
        require(
            not extension.startswith("."),
            f"extension {extension!r} must not start with a dot",
        )
        require(
            not any(character.isspace() for character in extension),
            f"extension {extension!r} must not contain whitespace",
        )

    buttons = config.get("buttons")
    keybinds = config.get("keybinds")
    validate_string_list(buttons, f"{location}.buttons", allow_null=True)
    validate_string_list(keybinds, f"{location}.keybinds", allow_null=True)
    require(
        len(buttons) == len(keybinds),
        f"{location}.buttons and keybinds must have equal lengths",
    )

    button_names = {button for button in buttons if button is not None}
    require(
        len(button_names) == sum(button is not None for button in buttons),
        f"{location}.buttons contains duplicate names",
    )

    actions = config.get("actions", [])
    require(isinstance(actions, list), f"{location}.actions must be an array")
    for group_index, group in enumerate(actions):
        require(
            isinstance(group, list),
            f"{location}.actions[{group_index}] must be an array",
        )
        for action_index, action in enumerate(group):
            require(
                isinstance(action, list),
                f"{location}.actions[{group_index}][{action_index}] must be an array",
            )
            for button in action:
                require(
                    button in button_names,
                    f"{location}.actions references unknown button {button!r}",
                )

    if "rambase" in config:
        require(
            isinstance(config["rambase"], int) and config["rambase"] >= 0,
            f"{location}.rambase must be a non-negative integer",
        )
    if "types" in config:
        validate_string_list(config["types"], f"{location}.types")
    if "overlay" in config:
        overlay = config["overlay"]
        require(
            isinstance(overlay, list) and len(overlay) == 3,
            f"{location}.overlay must have three entries",
        )
        require(
            all(isinstance(item, str) and len(item) == 1 for item in overlay[:2]),
            f"{location}.overlay byte-order entries must be single characters",
        )
        require(
            isinstance(overlay[2], int) and overlay[2] >= 0,
            f"{location}.overlay offset must be a non-negative integer",
        )


def find_extension_owners(manifest_path):
    owners = {}
    for sibling in sorted(manifest_path.parent.glob("*.json")):
        if sibling.resolve() == manifest_path.resolve():
            continue
        document = load_manifest(sibling)
        for platform, config in document.items():
            for extension in config.get("ext", []):
                owners.setdefault(extension, []).append(f"{platform} ({sibling.name})")
    return owners


def validate_manifest(manifest_path):
    document = load_manifest(manifest_path)
    extension_owners = find_extension_owners(manifest_path)
    extension_count = 0

    for platform, config in document.items():
        validate_platform(platform, config)
        for extension in config["ext"]:
            owners = extension_owners.get(extension, [])
            require(
                not owners,
                f"extension {extension!r} is already mapped by {', '.join(owners)}",
            )
            extension_count += 1

    print(
        f"Validated {manifest_path}: {len(document)} platform entries, {extension_count} ROM extensions",
    )


def main():
    parser = argparse.ArgumentParser(
        description="Validate a Stable Retro core manifest",
    )
    parser.add_argument("manifest", type=Path, help="path to cores/<platform>.json")
    args = parser.parse_args()

    try:
        validate_manifest(args.manifest)
    except ManifestError as error:
        parser.error(str(error))


if __name__ == "__main__":
    main()
