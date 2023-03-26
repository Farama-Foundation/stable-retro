def test_core_import():
    try:
        import mgba.core  # noqa: F401
    except Exception:
        raise AssertionError
