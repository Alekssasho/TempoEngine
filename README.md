# TempoEngine
Runtime should be minimal and should only use memcpy-able assets aka no random asset reading just binary files ready for direct consumption(possible compressed).

Tooling is exclusivly written in Rust and should handle cooking of assets (and probably some editor stuff) and generating ready to go binary data for direct consumption by runtime.
