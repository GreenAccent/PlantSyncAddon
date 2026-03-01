# ClassSync - User Manual

## Overview

ClassSync is an ArchiCAD 28 add-on that synchronizes plant classification systems between ArchiCAD projects and a shared master XML file. It allows a team to maintain a consistent classification database (e.g., "Green Accent PLANTS" with ~489 plant species) across multiple projects.

## Getting Started

1. **Open the palette**: Menu bar > ClassSync > Sync
2. **Select the master XML file**: Click **Browse...** and choose your shared classification XML file
3. The palette loads automatically and shows three columns comparing Project vs Server data

The XML file path is saved in ArchiCAD preferences and remembered across sessions.

## Interface

The palette has three main columns:

| Column | Description |
|--------|-------------|
| **Project** (left) | Classification items currently in the open ArchiCAD project |
| **Differences** (center) | Items that differ between Project and Server, grouped by type |
| **Server (XML)** (right) | Classification items from the shared XML file on disk |

### Color coding

- **Green** - Item exists only on this side (new/unique)
- **Blue** - Item is missing from this side (exists on the other side)
- **Brick red** - Conflict: same ID but different name on each side

### Differences panel sections

- **Conflicts (N)** - Items with the same ID but different names. Shows both names: `P:"project name"  S:"server name"`
- **Only in Project (N)** - Items that exist in the project but not in the XML
- **Only on Server (N)** - Items that exist in the XML but not in the project

Clicking an item in the Differences panel automatically selects and scrolls to the corresponding item in the Project and Server trees.

## Write Mode (Database Locking)

Before making changes to the XML file, you must enter write mode:

1. Click **Open for write** - this creates a `.lock` file next to the XML
2. The button changes to **Close write** and the Export/Use Project buttons become available
3. When done, click **Close write** to release the lock

### Lock behavior

- **Only one session at a time** can hold the write lock (each ArchiCAD instance counts as a separate session)
- If another user or another ArchiCAD instance has the lock, clicking "Open for write" shows who holds it and since when
- Click **Refresh** to check if the lock has been released
- The lock is **automatically released** when:
  - You click "Close write"
  - You close the ClassSync palette
  - ArchiCAD exits
  - You switch to a different XML file via Browse

### Stale locks

If ArchiCAD crashes while a lock is held, the `.lock` file may remain. In this case, the lock file can be manually deleted from the file system. It is located next to the XML file (e.g., `Green Accent PLANTS.xml.lock`).

## Actions

| Button | Available when | What it does |
|--------|---------------|--------------|
| **<- Import** | "Only on Server" item selected | Imports all missing items from the XML into the ArchiCAD project |
| **Export ->** | "Only in Project" item selected + write mode | Adds the selected item to the XML file (sorted alphabetically) |
| **Use Project** | "Conflict" item selected + write mode | Updates the XML item's name to match the project version |
| **Use Server** | "Conflict" item selected | Updates the project item's name to match the XML version |
| **Refresh** | Always | Reloads project data, re-reads XML, and recalculates differences |

Note: **Import** and **Use Server** do not require write mode because they modify the ArchiCAD project, not the XML file.

## Changelog

Every sync action is logged to a human-readable changelog file:

- **Location**: `changelog/` subdirectory next to the XML file
- **File naming**: `YYYY-MM-DD.txt` (one file per day)
- **Format**: Each entry shows timestamp, user, action type, and affected items

Example:
```
[12:34:56] Jan (DESKTOP-ABC)
  Export: DRZ.L.01.03 "Acer palmatum"
  Parent: DRZ.L.01

[12:35:12] Jan (DESKTOP-ABC)
  Use Project (resolve conflict): DRZ.L.02
  Server name: "Betula pendula"
  Project name: "Betula pendula 'Youngii'"
```

## Build Scripts

```bash
./configure.sh    # Re-run CMake (needed after adding new source files)
./build.sh        # Incremental build
./rebuild.sh      # Clean + full rebuild
./deploy.sh       # Copy .apx to ArchiCAD (requires admin, ArchiCAD must be closed)
```

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Palette doesn't appear | Menu > ClassSync > Sync. Check ArchiCAD Report window for errors |
| "Database is locked" | Another user is editing. Click Refresh to check if they're done |
| Stale lock after crash | Manually delete the `.lock` file next to the XML |
| XML path not remembered | Check ArchiCAD preferences (File > Preferences) |
| Export inserts in wrong place | Items are sorted alphabetically by ID within their parent |
| Trees don't update | Click Refresh to reload all data |
