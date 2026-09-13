# Carnet

A desktop budget app for Windows, written in C++23 and Qt 6.

Most banks let you download your account history as a CSV file, but the file on its own doesn't say
much. Carnet imports those files, sorts each transaction into a category using rules you write, and
shows what came in, what went out and how each category did against its monthly budget. Everything
stays in a single SQLite file on the computer.

![The monthly overview](docs/screenshots/overview.png)

## What it does

Importing works with any bank as long as it exports CSV. You tell Carnet once which column holds the
date, the amount and the description, and it keeps that as a profile for next time. For French,
Dutch or English column names it guesses the mapping for you, and it can skip rows with a status
such as "Refused". Importing a file twice doesn't create duplicates, but two identical coffees on the
same day still count as two.

Categories come from rules like *contains "delhaize" -> Groceries*, checked in the order you set and
ignoring case and accents. You can always override a single transaction by hand. Money moved between
your own accounts can go in a category that is left out of the totals, so it doesn't look like
income or spending.

The overview shows income, spending and what's left for the month, one bar per category (red once
it passes its budget) and the last six months side by side. A month can be exported as CSV, either
the summary or every transaction, in a format Excel opens directly.

| Transactions | Import | Rules |
| --- | --- | --- |
| ![Transactions](docs/screenshots/transactions.png) | ![Import](docs/screenshots/import.png) | ![Rules](docs/screenshots/rules.png) |

## Trying it

There's a made-up export in [`docs/demo/demo-bank.csv`](docs/demo/demo-bank.csv), laid out the way
many European banks export. Start Carnet, click Import CSV and open it: the columns and the refused payment are
filled in, so you only press Import and give the profile a name. The screenshots above were made
from that file.

Ctrl+PgUp and Ctrl+PgDown move between months, Ctrl+Home jumps back to the current one.

## Building

On Windows you need Visual Studio 2026 with the C++ workload, CMake 3.25+, Qt 6 (the MSVC 64-bit kit
from the Qt installer) and [vcpkg](https://github.com/microsoft/vcpkg) for SQLite and GoogleTest.

The presets expect `VCPKG_ROOT` to be set, which a Developer PowerShell for VS 2026 does. To use your
own vcpkg or tell CMake where Qt is, add a `CMakeUserPresets.json` (it's gitignored):

```json
{
  "version": 6,
  "configurePresets": [
    {
      "name": "local",
      "inherits": "windows-msvc",
      "environment": { "VCPKG_ROOT": "C:/path/to/vcpkg" },
      "cacheVariables": { "CMAKE_PREFIX_PATH": "C:/Qt/6.12.0/msvc2022_64" }
    }
  ],
  "buildPresets": [{ "name": "local", "configurePreset": "local" }],
  "testPresets": [{ "name": "local", "configurePreset": "local", "output": { "outputOnFailure": true } }]
}
```

```
cmake --preset local
cmake --build --preset local
ctest --preset local
```

The app ends up in `build/local/app/carnet.exe` and runs from there. For a copy you can zip and move
to another PC, use the `windows-msvc-release` preset (or a `local-release` one inheriting it) and then
`cmake --install build/local-release`, which collects the exe and the Qt and SQLite DLLs in
`build/install`. The other PC needs the Visual C++ Redistributable, which it most likely has already.

To regenerate the screenshots after changing the UI, configure with `-DCARNET_BUILD_SCREENSHOTS=ON`
and run the `carnet_screenshots` target. It draws the windows from the demo file without putting
anything on screen.

## Notes on the code

The project is split in two. `core/` is plain C++ with no Qt: parsing amounts, dates and CSV,
import profiles, rules, the database and the monthly totals. `app/` is the Qt Widgets interface
built on top, using Qt's model/view classes.

Amounts are kept as whole cents in a 64-bit integer from the moment they're read, never as floating
point. Parsing a bank file returns `std::expected`, so a broken row comes back as an error with its
line number rather than an exception; only real database failures throw, and the interface turns
those into a message box.

The schema is versioned with `PRAGMA user_version` and upgraded by numbered migrations, each in its
own transaction. The monthly totals are SQL queries, and their test checks them against a month worked
out by hand. The charts are small `QPainter` widgets rather than Qt Charts.

The core has GoogleTest tests and the interface has Qt Test tests (models, widgets, dialogs, the
main window's settings and export). The Qt tests run offscreen and use their own settings folder.

Qt is linked dynamically under the LGPL v3. SQLite is public domain.
