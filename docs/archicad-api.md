# ArchiCAD C++ API Notes (DevKit 28.4001)

## DG::SingleSelTreeView (inherits DG::TreeView)

Constructor: `SingleSelTreeView(const Panel& panel, short item)`

### Key Methods
```cpp
Int32 AppendItem(Int32 parentItem);           // Add child at end, returns new item ID
Int32 InsertItem(Int32 parentItem, Int32 tvItem); // Insert after tvItem
void  DeleteItem(Int32 tvItem);
Int32 GetItemCount() const;

void          SetItemText(Int32 tvItem, const GS::UniString& text);
GS::UniString GetItemText(Int32 tvItem) const;

void SetItemTextColor(Int32 tvItem, const Gfx::Color& color);
void ResetItemTextColor(Int32 tvItem);
void SetItemBackgroundColor(Int32 tvItem, const Gfx::Color& color);
void ResetItemBackgroundColor(Int32 tvItem);

void ExpandItem(Int32 tvItem);
void CollapseItem(Int32 tvItem);

void EnableDraw();      // Re-enable drawing after batch updates
void DisableDraw();     // Disable drawing during batch updates
void RedrawItem(Int32 tvItem);  // Force redraw specific item

void SelectItem(Int32 tvItem);
Int32 GetSelectedItem(Int32 tvItem = RootItem) const;
```

### DG::Item base methods (inherited by all controls)
```cpp
void Redraw() const;      // Force immediate visual update
void Invalidate() const;  // Mark item for refresh
void Enable();
void Disable();
```

### Tree Constants
- `DG_TVI_ROOT` = root level parent
- `DG_TVI_BOTTOM` = insert at bottom (used with C-style, C++ uses AppendItem)

## Gfx::Color (in GSRoot/Color.hpp)

```cpp
constexpr Color();
constexpr Color(UChar r, UChar g, UChar b, UChar a = Opaque);  // 0-255
Color(UShortSelector, UShort r, UShort g, UShort b, UShort a); // 0-65535
Color(FloatSelector, float r, float g, float b, float a);       // 0.0-1.0

// Static constants: Gfx::Color::Black, Red, Green, Blue, Yellow, etc.
```

## DG::Palette

Constructor: `Palette(GSResModule resModule, short resId, GSResModule dialIconResModule, const GS::Guid& guid)`

Lifecycle:
```cpp
// Constructor:
Attach(*this);
buttonRefresh.Attach(*this);  // individual buttons (NOT AttachToAllItems)
BeginEventProcessing();

// Destructor:
EndEventProcessing();
buttonRefresh.Detach(*this);
Detach(*this);
```

**NOTE**: `AttachToAllItems(*this)` requires `DG::CompoundItemObserver` inheritance. Simpler to attach individual buttons.

Register with ArchiCAD:
```cpp
ACAPI_RegisterModelessWindow(refId, callbackProc, flags, guid);
// flags: API_PalEnabled_FloorPlan | API_PalEnabled_3D | etc.
```

Observer overrides:
- `PanelResized(const DG::PanelResizeEvent& ev)` - from DG::PanelObserver
- `PanelCloseRequested(const DG::PanelCloseRequestEvent& ev, bool* accepted)` - close button
- `ButtonClicked(const DG::ButtonClickEvent& ev)` - from DG::ButtonItemObserver
- `TreeViewSelectionChanged(const DG::TreeViewSelectionEvent& ev)` - from DG::TreeViewObserver

`DG::Panel::RedrawItems()` - redraws all items in panel.

## DG File Dialogs (DGFileDlg.hpp)

```cpp
bool DGGetOpenFile(IO::Location* retLoc, Int32 popupCount, DGTypePopupItem* popup,
                   const IO::Location* defLoc, const GS::UniString& title, Int32 flags = 0);

struct DGTypePopupItem {
    GS::UniString text;        // "XML Files (*.xml)"
    GS::UniString extensions;  // "xml"
    Int32 macType;             // 0 for Windows
};
```

## Preferences API

```cpp
ACAPI_SetPreferences(Int32 version, GSSize nByte, const void* data);
ACAPI_GetPreferences(Int32* version, GSSize* nByte, void* data);
// data can be nullptr on first call to get version and size
```

Pattern:
```cpp
struct MyPrefs { unsigned short platform; char path[1024]; };

// Load:
Int32 v; GSSize n;
ACAPI_GetPreferences(&v, &n, nullptr);
if (v == 1 && n == sizeof(MyPrefs)) {
    MyPrefs p; ACAPI_GetPreferences(&v, &n, &p);
}

// Save (in FreeData):
ACAPI_SetPreferences(1, sizeof(prefs), &prefs);
```

## Classification API

### Read
```cpp
ACAPI_Classification_GetClassificationSystems(GS::Array<API_ClassificationSystem>& systems)
ACAPI_Classification_GetClassificationSystemRootItems(const API_Guid& systemGuid, GS::Array<API_ClassificationItem>& items)
ACAPI_Classification_GetClassificationItemChildren(const API_Guid& itemGuid, GS::Array<API_ClassificationItem>& children)
ACAPI_Classification_GetClassificationItem(API_ClassificationItem& item)  // fill by guid
```

### Write (require ACAPI_CallUndoableCommand wrapper)
```cpp
ACAPI_Classification_CreateClassificationSystem(API_ClassificationSystem& system)
ACAPI_Classification_CreateClassificationItem(API_ClassificationItem& item, const API_Guid& systemGuid, const API_Guid& parentGuid, const API_Guid& nextGuid)
ACAPI_Classification_ChangeClassificationItem(const API_ClassificationItem& item)  // change name/id/desc
ACAPI_Classification_DeleteClassificationItem(const API_Guid& itemGuid)
```

### Import (bulk)
```cpp
ACAPI_Classification_Import(const GS::UniString& xmlString,
    API_ClassificationSystemNameConflictResolutionPolicy systemPolicy,
    API_ClassificationItemNameConflictResolutionPolicy itemPolicy)
```

Policies:
- `API_MergeConflictingSystems` / `API_ReplaceConflictingSystems` / `API_SkipConflictingSystems`
- `API_ReplaceConflictingItems` / `API_SkipConflicitingItems` (note: typo in API - "Confliciting")

### Structures
```cpp
struct API_ClassificationSystem {
    API_Guid guid;
    GS::UniString name, description, source, editionVersion;
    GSDateRecord editionDate;
};

struct API_ClassificationItem {
    API_Guid guid;
    GS::UniString id, name, description;
};
```

## GRC Palette Syntax
```
'GDLG' resId Palette | grow | close  x y w h "Title" {
/* [n] */ ControlType  x y w h FontSize "text"
}
```

## Classification XML Format
```xml
<BuildingInformation>
  <Classification>
    <System>
      <Name>Green Accent PLANTS</Name>
      <EditionVersion>03</EditionVersion>
      <Items>
        <Item>
          <ID>DRZ</ID>
          <Name>DRZEWA</Name>
          <Description/>
          <Children>
            <Item>...</Item>
          </Children>
        </Item>
      </Items>
    </System>
  </Classification>
  <PropertyDefinitionGroups>...</PropertyDefinitionGroups>
</BuildingInformation>
```

Self-closing `<Children/>` = leaf item (no children).

## MDID
- Developer ID: 860318800
- Authorization Key: VCMXXGWYMZV3XXQK
- ClassSync Local ID: 1954174874
- Portal: https://archicadapi.graphisoft.com/
