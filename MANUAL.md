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

| Color | Meaning |
|-------|---------|
| **Green** | Item exists only on this side (new/unique) |
| **Blue** | Item is missing from this side (exists on the other side) |
| **Brick red** | ID Collision — same ID but different plant on each side |
| **Amber** | ID Mismatch — same plant but different ID on each side |

### Differences panel sections

The center panel groups differences into up to 6 sections:

| Section | Description | Example |
|---------|-------------|---------|
| **ID Collisions (N)** | Same ID, different Name. Two different plants got the same ID on different machines. | `DRZ.L.01.05  P:"Klon pospolity"  S:"Klon jawor"` |
| **ID Mismatches (N)** | Same Name, different ID. The same plant was added with different IDs. Shows cascade child count if applicable. | `"Brzoza pożyteczna"  P:DR.L.01.01  S:DRZ.L.01.01  (+47)` |
| **ID Cascades (N)** | Descendants of an ID Mismatch parent. A wrong prefix at the category level propagates to all plants underneath. | `DR.L.01.02 -> DRZ.L.01.02  "Dąb szypułkowy"` |
| **Double Conflicts (N)** | Rare: ID matches one server plant, Name matches a different server plant. | `DRZ.L.01.05  P:"Klon"  S(id):"Jawor"  S(name):.08` |
| **Only in Project (N)** | Item exists in the project but not in the XML. | `DRZ.L.01.03  -  Acer palmatum` |
| **Only on Server (N)** | Item exists in the XML but not in the project. | `DRZ.L.01.04  -  Betula utilis` |

Clicking an item in the Differences panel automatically selects and scrolls to the corresponding item in the Project and Server trees. For ID Mismatches and Cascades, the Project and Server trees highlight items with different IDs.

## Write Mode (Database Locking)

Before making changes to the XML file, you must enter write mode:

1. Click **Open for write** — this creates a `.lock` file next to the XML
2. The button changes to **Close write** and the Export button becomes available
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

### Row 1 — Standard actions

| Button | Available when | What it does |
|--------|---------------|--------------|
| **<- Import** | "Only on Server" item selected | Imports all missing items from the XML into the ArchiCAD project |
| **Export ->** | "Only in Project" item selected + write mode | Adds the selected item to the XML file |
| **Use Server ID** | "ID Mismatch" or "Double Conflict" selected | Changes the project item's ID to match the server version. The item itself (GUID) stays linked to all elements in the project — only the ID string changes. |
| **Use Server** | "ID Collision" item selected | Changes the project item's name to match the server version |

### Row 2 — ID repair actions

| Button | Available when | What it does |
|--------|---------------|--------------|
| **Reassign ID** | "ID Collision" item selected | Assigns a new, unused ID to the project item. After reassigning, export the item (now "Only in Project") and import the server item (now "Only on Server"). |
| **Fix Cascade** | "ID Cascade" item selected | Batch-renames all project items with the wrong prefix to the correct one from the server. E.g., changes all `DR.*` items to `DRZ.*` in one operation. Shows a confirmation dialog with the count of affected items. |

### Notes

- **Import**, **Use Server ID**, **Use Server**, **Reassign ID**, and **Fix Cascade** modify only the ArchiCAD project — they do NOT require write mode.
- **Export** modifies the XML file — it requires write mode (lock).
- All actions support **Undo** (Edit > Undo in ArchiCAD).
- **Reassign ID** calculates the next free ID by checking both Project and Server sides to avoid collisions.

## Typical Workflows

### Workflow 1: New plant added in one project

1. Open the project that has the new plant
2. Open ClassSync, click Refresh
3. The new item appears in "Only in Project"
4. Click **Open for write**, select the item, click **Export**
5. Click **Close write**
6. In other projects: open ClassSync, click **Import**

### Workflow 2: Same plant, different ID (e.g., DR vs DRZ)

1. Open ClassSync — the item appears in "ID Mismatches"
2. Select the item, click **Use Server ID**
3. The project item's ID changes to match the server
4. After Refresh, the item should show as matched

### Workflow 3: Wrong category prefix (cascade)

1. Open ClassSync — a category appears in "ID Mismatches" with `(+N)` showing N cascade children
2. The children appear in "ID Cascades"
3. Select any cascade item (or the parent), click **Fix Cascade**
4. Confirm the batch rename in the dialog
5. All items under the wrong prefix are renamed at once

### Workflow 4: ID collision (two different plants, same ID)

1. Open ClassSync — the collision appears in "ID Collisions"
2. Select the item, click **Reassign ID**
3. Confirm the new ID in the dialog
4. The project item moves to "Only in Project" — click **Export**
5. The server item appears in "Only on Server" — click **Import**
6. Both plants now have unique IDs

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
  Use Server ID (fix ID mismatch): "Brzoza pożyteczna"
  Old ID: DR.L.01.01
  New ID: DRZ.L.01.01

[12:36:00] Jan (DESKTOP-ABC)
  Fix Cascade: DR -> DRZ
  Items fixed: 47
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
| Use Server ID does nothing | Verify that `ACAPI_Classification_ChangeClassificationItem` supports changing the `id` field (API limitation risk) |
| Fix Cascade blocked | A collision would occur — resolve individual ID collisions first |
