# SweetShot Third-Party Dependencies

All third-party dependency wrappers live under `3dparty`.

Each vendored dependency should provide:

- `METADATA` with the upstream version, source, license name, and retrieved files.
- `LICENSE` with the upstream license text or license selection note.
- `sweetshot_3p.cmake` defining stable `SweetShot3p::*` targets.

Source dependencies should keep upstream files under `include` and `src`.

FetchContent dependencies should live in their own directory and expose all setup
through `sweetshot_3p.cmake`. Project CMake code should include
`cmake/SweetShotThirdParty.cmake` instead of calling `FetchContent` directly.

Current dependencies:

- `catch2` provides `SweetShot3p::Catch2`.
- `lodepng` fetches LodePNG and provides `SweetShot3p::lodepng`.
- `lunasvg` fetches LunaSVG and provides `SweetShot3p::lunasvg`.
- `resvg` fetches resvg and provides `SweetShot3p::resvg`.
- `sweetline` fetches SweetLine and provides `SweetLine::sweetline_static`.

The SweetLine wrapper also sets `SWEETSHOT_3P_SWEETLINE_SOURCE_DIR` for paths
that need the fetched SweetLine source tree, such as the bundled syntax files.

Project code should depend on CMake targets instead of referencing `3dparty`
paths directly.
