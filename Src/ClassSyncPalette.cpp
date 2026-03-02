#include "ClassSyncPalette.hpp"
#include "XmlReader.hpp"
#include "XmlWriter.hpp"
#include "FileLock.hpp"
#include "ChangeLog.hpp"
#include "DGFileDlg.hpp"

#include <fstream>
#include <sstream>


// ---------------------------------------------------------------------------
// Palette GUID (unique, for Work Environment persistence)
// ---------------------------------------------------------------------------

static const GS::Guid kPaletteGuid ("C1A55710-DA7A-4B2E-9F3C-8E2D1A6B5C4E");


// ---------------------------------------------------------------------------
// Static members
// ---------------------------------------------------------------------------

ClassSyncPalette*  ClassSyncPalette::instance          = nullptr;
bool               ClassSyncPalette::wasOpenBeforeHide = false;
GS::UniString      ClassSyncPalette::xmlFilePath;


// ---------------------------------------------------------------------------
// Color constants (muted, 3-color scheme)
// ---------------------------------------------------------------------------

static const Gfx::Color kColorNew        (  0, 130,  60);   // dark green  - unique to this side
static const Gfx::Color kColorMissing   (  0,  80, 170);   // dark blue   - missing (exists on other side)
static const Gfx::Color kColorConflict  (180,  50,   0);   // brick red   - ID collision (same ID, different name)
static const Gfx::Color kColorIdMismatch(200, 120,   0);   // amber       - ID mismatch (same name, different ID)


// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

ClassSyncPalette::ClassSyncPalette () :
	DG::Palette        (ACAPI_GetOwnResModule (), ClassSyncPaletteResId,
						ACAPI_GetOwnResModule (), kPaletteGuid),
	labelProject       (GetReference (), ItemLabelProject),
	labelConflicts     (GetReference (), ItemLabelConflicts),
	labelServer        (GetReference (), ItemLabelServer),
	treeProject        (GetReference (), ItemTreeProject),
	treeConflicts      (GetReference (), ItemTreeConflicts),
	treeServer         (GetReference (), ItemTreeServer),
	countProject       (GetReference (), ItemCountProject),
	countConflicts     (GetReference (), ItemCountConflicts),
	countServer        (GetReference (), ItemCountServer),
	buttonRefresh      (GetReference (), ItemButtonRefresh),
	buttonClose        (GetReference (), ItemButtonClose),
	labelXmlPath       (GetReference (), ItemLabelXmlPath),
	buttonBrowse       (GetReference (), ItemButtonBrowse),
	buttonImport       (GetReference (), ItemButtonImport),
	buttonExport       (GetReference (), ItemButtonExport),
	buttonUseServerId   (GetReference (), ItemButtonUseServerId),
	buttonUseServer    (GetReference (), ItemButtonUseServer),
	labelVersion       (GetReference (), ItemLabelVersion),
	buttonLock         (GetReference (), ItemButtonLock),
	labelWriteMode     (GetReference (), ItemLabelWriteMode),
	buttonReassignId   (GetReference (), ItemButtonReassignId),
	buttonFixCascade   (GetReference (), ItemButtonFixCascade)
{
	writeMode = false;

	Attach (*this);
	buttonRefresh.Attach (*this);
	buttonClose.Attach (*this);
	buttonBrowse.Attach (*this);
	buttonImport.Attach (*this);
	buttonExport.Attach (*this);
	buttonUseServerId.Attach (*this);
	buttonUseServer.Attach (*this);
	buttonLock.Attach (*this);
	buttonReassignId.Attach (*this);
	buttonFixCascade.Attach (*this);
	treeConflicts.Attach (static_cast<DG::TreeViewObserver&> (*this));
	BeginEventProcessing ();

	// Version label
	labelVersion.SetText (GS::UniString ("v") + kClassSyncVersion);

	// XML path label
	if (!xmlFilePath.IsEmpty ())
		labelXmlPath.SetText (xmlFilePath);
	else
		labelXmlPath.SetText ("No XML file selected - click Browse...");

	// Disable action buttons initially
	buttonImport.Disable ();
	buttonExport.Disable ();
	buttonUseServerId.Disable ();
	buttonUseServer.Disable ();
	buttonReassignId.Disable ();
	buttonFixCascade.Disable ();

	// Lock button: enable only if XML path is set
	labelWriteMode.SetText ("WRITE MODE");
	labelWriteMode.SetTextColor (kColorConflict);
	labelWriteMode.Hide ();

	if (xmlFilePath.IsEmpty ())
		buttonLock.Disable ();
	else
		CheckLockStatus ();

	ACAPI_WriteReport ("ClassSync v%s started", false, kClassSyncVersion);

	if (!xmlFilePath.IsEmpty ())
		RefreshData ();
}


// ---------------------------------------------------------------------------
// Destructor
// ---------------------------------------------------------------------------

ClassSyncPalette::~ClassSyncPalette ()
{
	ReleaseLockIfHeld ();
	EndEventProcessing ();
	treeConflicts.Detach (static_cast<DG::TreeViewObserver&> (*this));
	buttonFixCascade.Detach (*this);
	buttonReassignId.Detach (*this);
	buttonLock.Detach (*this);
	buttonUseServer.Detach (*this);
	buttonUseServerId.Detach (*this);
	buttonExport.Detach (*this);
	buttonImport.Detach (*this);
	buttonBrowse.Detach (*this);
	buttonClose.Detach (*this);
	buttonRefresh.Detach (*this);
	Detach (*this);
}


// ---------------------------------------------------------------------------
// Singleton management
// ---------------------------------------------------------------------------

bool ClassSyncPalette::HasInstance ()
{
	return instance != nullptr;
}

ClassSyncPalette& ClassSyncPalette::GetInstance ()
{
	return *instance;
}

void ClassSyncPalette::CreateInstance ()
{
	if (instance == nullptr)
		instance = new ClassSyncPalette ();
}

void ClassSyncPalette::DestroyInstance ()
{
	delete instance;
	instance = nullptr;
}


GS::Guid ClassSyncPalette::GetPaletteGuid ()
{
	return kPaletteGuid;
}


// ---------------------------------------------------------------------------
// ArchiCAD palette control callback
// ---------------------------------------------------------------------------

GSErrCode ClassSyncPalette::PaletteControlCallback (Int32 referenceID,
													 API_PaletteMessageID messageID,
													 GS::IntPtr param)
{
	if (referenceID != GetRefId ())
		return NoError;

	switch (messageID) {
		case APIPalMsg_ClosePalette:
			if (HasInstance () && GetInstance ().IsVisible ())
				GetInstance ().Hide ();
			break;

		case APIPalMsg_HidePalette_Begin:
			if (HasInstance ()) {
				wasOpenBeforeHide = GetInstance ().IsVisible ();
				if (wasOpenBeforeHide)
					GetInstance ().Hide ();
			}
			break;

		case APIPalMsg_HidePalette_End:
			if (HasInstance () && wasOpenBeforeHide)
				GetInstance ().Show ();
			break;

		case APIPalMsg_OpenPalette:
			if (HasInstance ())
				GetInstance ().Show ();
			break;

		case APIPalMsg_IsPaletteVisible:
			*(reinterpret_cast<bool*> (param)) =
				HasInstance () && GetInstance ().IsVisible ();
			break;

		default:
			break;
	}

	return NoError;
}


// ---------------------------------------------------------------------------
// PanelObserver: close button clicked
// ---------------------------------------------------------------------------

void ClassSyncPalette::PanelCloseRequested (const DG::PanelCloseRequestEvent& /*ev*/,
											bool* accepted)
{
	ReleaseLockIfHeld ();
	Hide ();
	*accepted = true;

	// Update menu checkmark (defined in ClassSync.cpp, called via palette callback)
	API_MenuItemRef itemRef = {};
	GSFlags         itemFlags = 0;
	itemRef.menuResID = 32500;
	itemRef.itemIndex = 1;
	ACAPI_MenuItem_GetMenuItemFlags (&itemRef, &itemFlags);
	itemFlags &= ~API_MenuItemChecked;
	ACAPI_MenuItem_SetMenuItemFlags (&itemRef, &itemFlags);
}


// ---------------------------------------------------------------------------
// PanelObserver: palette resized - proportional 3-column layout
// ---------------------------------------------------------------------------

void ClassSyncPalette::PanelResized (const DG::PanelResizeEvent& /*ev*/)
{
	short w = GetClientWidth ();
	short h = GetClientHeight ();

	// Margins and spacing
	const short margin = 10;
	const short gap    = 15;
	const short bottomArea = 180;  // space for buttons/labels below trees (2 rows of action buttons)

	// 3 equal columns
	short colW = (w - 2 * margin - 2 * gap) / 3;
	if (colW < 100) colW = 100;
	short col1 = margin;
	short col2 = margin + colW + gap;
	short col3 = margin + 2 * (colW + gap);

	// Tree height fills available space
	short treeTop = 25;
	short treeH   = h - treeTop - bottomArea;
	if (treeH < 80) treeH = 80;

	short countY  = treeTop + treeH + 5;
	short xmlY    = countY + 22;
	short btnActY = xmlY + 28;
	short bottomY = h - 30;

	// Column labels (row 1)
	labelProject.SetPosition   (col1, 5);
	labelConflicts.SetPosition (col2, 5);
	labelServer.SetPosition    (col3, 5);

	// Trees
	treeProject.SetPosition   (col1, treeTop);
	treeProject.SetSize       (colW, treeH);
	treeConflicts.SetPosition (col2, treeTop);
	treeConflicts.SetSize     (colW, treeH);
	treeServer.SetPosition    (col3, treeTop);
	treeServer.SetSize        (colW, treeH);

	// Count labels
	countProject.SetPosition   (col1, countY);
	countProject.SetWidth      (colW);
	countConflicts.SetPosition (col2, countY);
	countConflicts.SetWidth    (colW);
	countServer.SetPosition    (col3, countY);
	countServer.SetWidth       (colW);

	// XML path label + Browse button
	labelXmlPath.SetPosition (col1, xmlY);
	labelXmlPath.SetWidth    (w - 2 * margin - 100);
	buttonBrowse.SetPosition (w - margin - 90, xmlY);

	// Action buttons row 1
	buttonImport.SetPosition     (col1,       btnActY);
	buttonExport.SetPosition     (col1 + 115, btnActY);
	buttonUseServerId.SetPosition (col1 + 230, btnActY);
	buttonUseServer.SetPosition  (col1 + 345, btnActY);
	buttonLock.SetPosition       (col1 + 520, btnActY);
	buttonLock.SetWidth          (130);
	labelWriteMode.SetPosition   (col1 + 660, btnActY + 4);

	// Action buttons row 2
	buttonReassignId.SetPosition (col1,       btnActY + 30);
	buttonFixCascade.SetPosition (col1 + 115, btnActY + 30);

	// Bottom row: version left, Refresh+Close right
	labelVersion.SetPosition  (col1,            bottomY);
	buttonRefresh.SetPosition (w - margin - 190, bottomY);
	buttonClose.SetPosition   (w - margin - 90,  bottomY);

	RedrawItems ();
}


// ---------------------------------------------------------------------------
// ButtonItemObserver: button clicked
// ---------------------------------------------------------------------------

void ClassSyncPalette::ButtonClicked (const DG::ButtonClickEvent& ev)
{
	if (ev.GetSource () == &buttonRefresh) {
		RefreshData ();
	} else if (ev.GetSource () == &buttonClose) {
		Hide ();
	} else if (ev.GetSource () == &buttonBrowse) {
		BrowseForXml ();
	} else if (ev.GetSource () == &buttonImport) {
		DoImportFromServer ();
	} else if (ev.GetSource () == &buttonExport) {
		DoExportToServer ();
	} else if (ev.GetSource () == &buttonUseServerId) {
		DoUseServerId ();
	} else if (ev.GetSource () == &buttonUseServer) {
		DoUseServer ();
	} else if (ev.GetSource () == &buttonReassignId) {
		DoReassignId ();
	} else if (ev.GetSource () == &buttonFixCascade) {
		DoFixCascade ();
	} else if (ev.GetSource () == &buttonLock) {
		DoToggleLock ();
	}
}


// ---------------------------------------------------------------------------
// TreeViewObserver: selection changed in conflicts tree
// ---------------------------------------------------------------------------

void ClassSyncPalette::TreeViewSelectionChanged (const DG::TreeViewSelectionEvent& ev)
{
	if (ev.GetSource () == &treeConflicts)
		UpdateActionButtons ();
}


// ---------------------------------------------------------------------------
// TreeViewObserver: item clicked (fires on every click, even re-click)
// ---------------------------------------------------------------------------

void ClassSyncPalette::TreeViewItemClicked (const DG::TreeViewItemClickEvent& ev,
											 bool* /*denySelectionChange*/)
{
	if (ev.GetSource () == &treeConflicts) {
		// Sync selection in side trees on every click
		Int32 selected = treeConflicts.GetSelectedItem ();
		UInt32 diffIdx;
		if (selected != 0 && conflictItemToDiffIndex.Get (selected, &diffIdx))
			SyncSideTreeSelection (diffEntries[diffIdx]);
	}
}


// ---------------------------------------------------------------------------
// Sync side tree selection: select and scroll to the matching item
// ---------------------------------------------------------------------------

void ClassSyncPalette::SyncSideTreeSelection (const DiffEntry& entry)
{
	Int32 projItem = 0;
	Int32 servItem = 0;

	projectIdToTreeItem.Get (entry.id, &projItem);

	// For S3/S6/IdCascade, server item has a different ID
	GS::UniString serverLookupId = entry.serverId.IsEmpty () ? entry.id : entry.serverId;
	serverIdToTreeItem.Get (serverLookupId, &servItem);

	if (projItem != 0)
		treeProject.SelectItem (projItem);

	if (servItem != 0)
		treeServer.SelectItem (servItem);
}


// ---------------------------------------------------------------------------
// Update action button enabled states based on conflicts tree selection
// ---------------------------------------------------------------------------

void ClassSyncPalette::UpdateActionButtons ()
{
	Int32 selected = treeConflicts.GetSelectedItem ();

	if (selected == 0 || selected == DG::TreeView::RootItem) {
		buttonImport.Disable ();
		buttonExport.Disable ();
		buttonUseServerId.Disable ();
		buttonUseServer.Disable ();
		buttonReassignId.Disable ();
		buttonFixCascade.Disable ();
		return;
	}

	UInt32 diffIdx;
	if (!conflictItemToDiffIndex.Get (selected, &diffIdx)) {
		// Section header or non-actionable item
		buttonImport.Disable ();
		buttonExport.Disable ();
		buttonUseServerId.Disable ();
		buttonUseServer.Disable ();
		buttonReassignId.Disable ();
		buttonFixCascade.Disable ();
		return;
	}

	const DiffEntry& entry = diffEntries[diffIdx];

	buttonImport.Disable ();
	buttonExport.Disable ();
	buttonUseServerId.Disable ();
	buttonUseServer.Disable ();
	buttonReassignId.Disable ();
	buttonFixCascade.Disable ();

	switch (entry.status) {
		case DiffStatus::OnlyInServer:
			buttonImport.Enable ();
			break;
		case DiffStatus::OnlyInProject:
			if (writeMode)
				buttonExport.Enable ();
			break;
		case DiffStatus::IdCollision:
			// Same ID, different name — can reassign ID or use server name
			buttonReassignId.Enable ();
			buttonUseServer.Enable ();
			break;
		case DiffStatus::IdMismatch:
		case DiffStatus::DoubleConflict:
			// Same name, different ID — can adopt server's ID
			buttonUseServerId.Enable ();
			break;
		case DiffStatus::IdCascade:
			// Descendants of S3 parent — fix cascade
			buttonFixCascade.Enable ();
			break;
		default:
			break;
	}
}


// ---------------------------------------------------------------------------
// Browse for XML file
// ---------------------------------------------------------------------------

void ClassSyncPalette::BrowseForXml ()
{
	DGTypePopupItem popup;
	popup.text = "XML Files (*.xml)";
	popup.extensions = "xml";

	IO::Location loc;
	bool success = DGGetOpenFile (&loc, 1, &popup, nullptr,
								  GS::UniString ("Select Classification XML"));

	if (success) {
		// Release old lock if switching XML files
		ReleaseLockIfHeld ();

		xmlFilePath = loc.ToDisplayText ();
		labelXmlPath.SetText (xmlFilePath);
		buttonLock.Enable ();
		SavePreferences ();
		RefreshData ();
	}
}


// ---------------------------------------------------------------------------
// Import from server: add all missing items to project using XML import
// ---------------------------------------------------------------------------

void ClassSyncPalette::DoImportFromServer ()
{
	if (xmlFilePath.IsEmpty ()) return;

	std::string pathUtf8 (xmlFilePath.ToCStr (0, MaxUSize, CC_UTF8).Get ());

	std::ifstream file (pathUtf8, std::ios::binary);
	if (!file.is_open ()) {
		ACAPI_WriteReport ("ClassSync: Cannot open XML for import: %s", false, pathUtf8.c_str ());
		return;
	}
	std::ostringstream ss;
	ss << file.rdbuf ();
	std::string content = ss.str ();
	file.close ();

	GS::UniString xmlContent (content.c_str (), CC_UTF8);

	GSErrCode err = ACAPI_CallUndoableCommand (
		GS::UniString ("ClassSync: Import from Server"),
		[&] () -> GSErrCode {
			return ACAPI_Classification_Import (
				xmlContent,
				API_MergeConflictingSystems,
				API_SkipConflicitingItems);
		});

	if (err == NoError) {
		ACAPI_WriteReport ("ClassSync: Import successful", false);
		LogImport (xmlFilePath);
	} else {
		ACAPI_WriteReport ("ClassSync: Import failed, error %d", false, (int)err);
	}

	RefreshData ();
}


// ---------------------------------------------------------------------------
// Export to server: add selected OnlyInProject item to XML file
// ---------------------------------------------------------------------------

void ClassSyncPalette::DoExportToServer ()
{
	if (xmlFilePath.IsEmpty ()) return;

	Int32 selected = treeConflicts.GetSelectedItem ();
	UInt32 diffIdx;
	if (!conflictItemToDiffIndex.Get (selected, &diffIdx)) return;

	const DiffEntry& entry = diffEntries[diffIdx];
	if (entry.status != DiffStatus::OnlyInProject) return;

	ClassificationNode node;
	node.id          = entry.id;
	node.name        = entry.projectName;
	node.description = entry.description;
	node.guid        = APINULLGuid;

	std::string pathUtf8 (xmlFilePath.ToCStr (0, MaxUSize, CC_UTF8).Get ());

	// Determine parent ID: strip last segment from the item ID
	// e.g. "DRZ.L.01.03" -> parent is "DRZ.L.01", "DRZ.L" -> parent is "DRZ"
	GS::UniString parentId;
	auto lastDot = entry.id.FindLast ('.');
	if (lastDot != MaxUIndex)
		parentId = entry.id.GetSubstring (0, lastDot);

	bool success = AddItemToXml (pathUtf8.c_str (), parentId, node);

	if (success) {
		ACAPI_WriteReport ("ClassSync: Exported '%s' to XML", false,
						   entry.id.ToCStr ().Get ());
		LogExport (xmlFilePath, entry.id, entry.projectName, parentId);
	} else {
		ACAPI_WriteReport ("ClassSync: Export failed for '%s'", false,
						   entry.id.ToCStr ().Get ());
	}

	RefreshData ();
}


// ---------------------------------------------------------------------------
// Use Server ID: change project item's ID to match server (S3, S6)
// ---------------------------------------------------------------------------

void ClassSyncPalette::DoUseServerId ()
{
	Int32 selected = treeConflicts.GetSelectedItem ();
	UInt32 diffIdx;
	if (!conflictItemToDiffIndex.Get (selected, &diffIdx)) return;

	const DiffEntry& entry = diffEntries[diffIdx];
	if (entry.status != DiffStatus::IdMismatch &&
		entry.status != DiffStatus::DoubleConflict) return;
	if (entry.projectItemGuid == APINULLGuid) return;
	if (entry.serverId.IsEmpty ()) return;

	API_ClassificationItem item;
	item.guid = entry.projectItemGuid;
	if (ACAPI_Classification_GetClassificationItem (item) != NoError) {
		ACAPI_WriteReport ("ClassSync: Cannot find project item '%s'", false,
						   entry.id.ToCStr ().Get ());
		return;
	}

	GS::UniString oldId = item.id;
	item.id = entry.serverId;

	GSErrCode err = ACAPI_CallUndoableCommand (
		GS::UniString ("ClassSync: Use Server ID"),
		[&] () -> GSErrCode {
			return ACAPI_Classification_ChangeClassificationItem (item);
		});

	if (err == NoError) {
		ACAPI_WriteReport ("ClassSync: Project item ID '%s' -> '%s' (name: '%s')", false,
						   oldId.ToCStr ().Get (),
						   entry.serverId.ToCStr ().Get (),
						   entry.projectName.ToCStr ().Get ());
		LogUseServerId (xmlFilePath, oldId, entry.serverId, entry.projectName);
	} else {
		ACAPI_WriteReport ("ClassSync: Failed to change project item ID '%s', error %d", false,
						   entry.id.ToCStr ().Get (), (int)err);
	}

	RefreshData ();
}


// ---------------------------------------------------------------------------
// Use Server: update project item name to match server
// ---------------------------------------------------------------------------

void ClassSyncPalette::DoUseServer ()
{
	Int32 selected = treeConflicts.GetSelectedItem ();
	UInt32 diffIdx;
	if (!conflictItemToDiffIndex.Get (selected, &diffIdx)) return;

	const DiffEntry& entry = diffEntries[diffIdx];
	if (entry.status != DiffStatus::IdCollision) return;
	if (entry.projectItemGuid == APINULLGuid) return;

	API_ClassificationItem item;
	item.guid = entry.projectItemGuid;
	if (ACAPI_Classification_GetClassificationItem (item) != NoError) {
		ACAPI_WriteReport ("ClassSync: Cannot find project item '%s'", false,
						   entry.id.ToCStr ().Get ());
		return;
	}

	item.name = entry.serverName;

	GSErrCode err = ACAPI_CallUndoableCommand (
		GS::UniString ("ClassSync: Use Server name"),
		[&] () -> GSErrCode {
			return ACAPI_Classification_ChangeClassificationItem (item);
		});

	if (err == NoError) {
		ACAPI_WriteReport ("ClassSync: Project item '%s' name -> '%s'", false,
						   entry.id.ToCStr ().Get (),
						   entry.serverName.ToCStr ().Get ());
		LogUseServer (xmlFilePath, entry.id, entry.projectName, entry.serverName);
	} else {
		ACAPI_WriteReport ("ClassSync: Failed to change project item '%s', error %d", false,
						   entry.id.ToCStr ().Get (), (int)err);
	}

	RefreshData ();
}


// ---------------------------------------------------------------------------
// FindNextFreeId: find the next available child ID under a parent
// Checks both project and server IDs to avoid collisions on either side.
// ---------------------------------------------------------------------------

GS::UniString ClassSyncPalette::FindNextFreeId (const GS::UniString& parentId) const
{
	// Collect all existing child numbers under this parent
	Int32 maxNum = 0;
	GS::UniString prefix = parentId + ".";

	auto checkId = [&] (const GS::UniString& id) {
		if (!id.BeginsWith (prefix))
			return;
		// Get the segment right after the prefix (before next dot)
		GS::UniString rest = id.GetSubstring (prefix.GetLength (),
			id.GetLength () - prefix.GetLength ());
		auto dot = rest.FindFirst ('.');
		GS::UniString segment = (dot != MaxUIndex)
			? rest.GetSubstring (0, dot) : rest;
		// Parse as number
		Int32 num = 0;
		for (UInt32 c = 0; c < segment.GetLength (); c++) {
			char ch = (char) segment.GetChar (c);
			if (ch >= '0' && ch <= '9')
				num = num * 10 + (ch - '0');
			else
				return;  // not a pure number
		}
		if (num > maxNum)
			maxNum = num;
	};

	for (auto it = allProjectIds.EnumerateBegin (); it != allProjectIds.EnumerateEnd (); ++it)
		checkId (*it);
	for (auto it = allServerIds.EnumerateBegin (); it != allServerIds.EnumerateEnd (); ++it)
		checkId (*it);

	Int32 nextNum = maxNum + 1;

	// Determine zero-padding width from siblings
	UInt32 padWidth = 2;  // default: 01, 02, ...
	for (auto it = allProjectIds.EnumerateBegin (); it != allProjectIds.EnumerateEnd (); ++it) {
		if ((*it).BeginsWith (prefix)) {
			GS::UniString rest = (*it).GetSubstring (prefix.GetLength (),
				(*it).GetLength () - prefix.GetLength ());
			auto dot = rest.FindFirst ('.');
			GS::UniString segment = (dot != MaxUIndex)
				? rest.GetSubstring (0, dot) : rest;
			if (segment.GetLength () > padWidth)
				padWidth = segment.GetLength ();
			break;  // use first sibling as reference
		}
	}

	// Format with zero-padding
	char buf[32];
	snprintf (buf, sizeof (buf), "%0*d", (int)padWidth, nextNum);
	return parentId + "." + GS::UniString (buf);
}


// ---------------------------------------------------------------------------
// Reassign ID: change a colliding project item to the next free ID (S2)
// ---------------------------------------------------------------------------

void ClassSyncPalette::DoReassignId ()
{
	Int32 selected = treeConflicts.GetSelectedItem ();
	UInt32 diffIdx;
	if (!conflictItemToDiffIndex.Get (selected, &diffIdx)) return;

	const DiffEntry& entry = diffEntries[diffIdx];
	if (entry.status != DiffStatus::IdCollision) return;
	if (entry.projectItemGuid == APINULLGuid) return;

	// Calculate parent ID and next free ID
	GS::UniString parentId;
	auto lastDot = entry.id.FindLast ('.');
	if (lastDot != MaxUIndex)
		parentId = entry.id.GetSubstring (0, lastDot);

	GS::UniString newId = FindNextFreeId (parentId);

	// Confirm with user
	GS::UniString msg = "Reassign project item ID:\n\n"
		"Current ID: " + entry.id + "\n"
		"Name: \"" + entry.projectName + "\"\n\n"
		"New ID: " + newId + "\n\n"
		"After reassigning, export the item to server\n"
		"and import the server item with the old ID.";

	short result = DGAlert (DG_INFORMATION, "ClassSync - Reassign ID", msg, "", "Reassign", "Cancel", "");
	if (result != DG_OK)
		return;

	// Change the item's ID in the project
	API_ClassificationItem item;
	item.guid = entry.projectItemGuid;
	if (ACAPI_Classification_GetClassificationItem (item) != NoError) {
		ACAPI_WriteReport ("ClassSync: Cannot find project item '%s'", false,
						   entry.id.ToCStr ().Get ());
		return;
	}

	GS::UniString oldId = item.id;
	item.id = newId;

	GSErrCode err = ACAPI_CallUndoableCommand (
		GS::UniString ("ClassSync: Reassign ID"),
		[&] () -> GSErrCode {
			return ACAPI_Classification_ChangeClassificationItem (item);
		});

	if (err == NoError) {
		ACAPI_WriteReport ("ClassSync: Reassigned ID '%s' -> '%s' (name: '%s')", false,
						   oldId.ToCStr ().Get (),
						   newId.ToCStr ().Get (),
						   entry.projectName.ToCStr ().Get ());
		LogReassignId (xmlFilePath, oldId, newId, entry.projectName);
	} else {
		ACAPI_WriteReport ("ClassSync: Failed to reassign ID '%s', error %d", false,
						   entry.id.ToCStr ().Get (), (int)err);
	}

	RefreshData ();
}


// ---------------------------------------------------------------------------
// Fix Cascade: batch-rename project items from wrong prefix to correct one
// ---------------------------------------------------------------------------

void ClassSyncPalette::DoFixCascade ()
{
	Int32 selected = treeConflicts.GetSelectedItem ();
	UInt32 diffIdx;
	if (!conflictItemToDiffIndex.Get (selected, &diffIdx)) return;

	const DiffEntry& entry = diffEntries[diffIdx];

	// Accept IdCascade items or IdMismatch items with cascadeChildCount > 0
	GS::UniString projPrefix, servPrefix;
	if (entry.status == DiffStatus::IdCascade) {
		projPrefix = entry.cascadeParentProjectId;
		servPrefix = entry.cascadeParentServerId;
	} else if (entry.status == DiffStatus::IdMismatch && entry.cascadeChildCount > 0) {
		projPrefix = entry.id;
		servPrefix = entry.serverId;
	} else {
		return;
	}

	if (projPrefix.IsEmpty () || servPrefix.IsEmpty ()) return;

	// Count items to fix (the parent + all cascade children)
	UInt32 fixCount = 0;
	for (UInt32 i = 0; i < diffEntries.GetSize (); i++) {
		if (diffEntries[i].status == DiffStatus::IdMismatch &&
			diffEntries[i].id == projPrefix)
			fixCount++;
		if (diffEntries[i].status == DiffStatus::IdCascade &&
			diffEntries[i].cascadeParentProjectId == projPrefix)
			fixCount++;
	}

	// Check for collisions: would any new ID collide with existing project items?
	bool hasCollision = false;
	for (UInt32 i = 0; i < diffEntries.GetSize (); i++) {
		GS::UniString newId;
		if (diffEntries[i].status == DiffStatus::IdMismatch && diffEntries[i].id == projPrefix) {
			newId = servPrefix;
		} else if (diffEntries[i].status == DiffStatus::IdCascade &&
				   diffEntries[i].cascadeParentProjectId == projPrefix) {
			GS::UniString suffix = diffEntries[i].id.GetSubstring (
				projPrefix.GetLength (),
				diffEntries[i].id.GetLength () - projPrefix.GetLength ());
			newId = servPrefix + suffix;
		} else {
			continue;
		}

		// Check if this new ID already exists in project (and is NOT one of our cascade items)
		if (allProjectIds.Contains (newId)) {
			bool isOurItem = false;
			for (UInt32 j = 0; j < diffEntries.GetSize (); j++) {
				if (diffEntries[j].id == newId &&
					(diffEntries[j].status == DiffStatus::IdMismatch ||
					 diffEntries[j].status == DiffStatus::IdCascade)) {
					isOurItem = true;
					break;
				}
			}
			if (!isOurItem) {
				hasCollision = true;
				ACAPI_WriteReport ("ClassSync: Fix Cascade collision: '%s' already exists", false,
								   newId.ToCStr ().Get ());
				break;
			}
		}
	}

	if (hasCollision) {
		DGAlert (DG_ERROR, "ClassSync - Fix Cascade",
			"Cannot fix cascade: new IDs would collide with existing items.",
			"Resolve individual collisions first.", "OK");
		return;
	}

	// Confirm
	GS::UniString msg = GS::UniString::Printf (
		"Fix ID cascade: %s -> %s (%d items)\n\n"
		"This will rename all items with prefix '%s'\n"
		"to use prefix '%s' instead.",
		projPrefix.ToCStr ().Get (),
		servPrefix.ToCStr ().Get (),
		fixCount,
		projPrefix.ToCStr ().Get (),
		servPrefix.ToCStr ().Get ());

	short result = DGAlert (DG_INFORMATION, "ClassSync - Fix Cascade", msg, "", "Fix", "Cancel", "");
	if (result != DG_OK)
		return;

	// Execute all changes in one undoable command
	GSErrCode err = ACAPI_CallUndoableCommand (
		GS::UniString ("ClassSync: Fix ID Cascade"),
		[&] () -> GSErrCode {
			for (UInt32 i = 0; i < diffEntries.GetSize (); i++) {
				GS::UniString newId;
				if (diffEntries[i].status == DiffStatus::IdMismatch &&
					diffEntries[i].id == projPrefix) {
					newId = servPrefix;
				} else if (diffEntries[i].status == DiffStatus::IdCascade &&
						   diffEntries[i].cascadeParentProjectId == projPrefix) {
					GS::UniString suffix = diffEntries[i].id.GetSubstring (
						projPrefix.GetLength (),
						diffEntries[i].id.GetLength () - projPrefix.GetLength ());
					newId = servPrefix + suffix;
				} else {
					continue;
				}

				if (diffEntries[i].projectItemGuid == APINULLGuid)
					continue;

				API_ClassificationItem item;
				item.guid = diffEntries[i].projectItemGuid;
				if (ACAPI_Classification_GetClassificationItem (item) != NoError)
					continue;

				item.id = newId;
				GSErrCode itemErr = ACAPI_Classification_ChangeClassificationItem (item);
				if (itemErr != NoError) {
					ACAPI_WriteReport ("ClassSync: Failed to change '%s' -> '%s', error %d",
						false, diffEntries[i].id.ToCStr ().Get (),
						newId.ToCStr ().Get (), (int)itemErr);
					return itemErr;
				}
			}
			return NoError;
		});

	if (err == NoError) {
		ACAPI_WriteReport ("ClassSync: Fixed cascade %s -> %s (%d items)", false,
						   projPrefix.ToCStr ().Get (),
						   servPrefix.ToCStr ().Get (),
						   fixCount);
		LogFixCascade (xmlFilePath, projPrefix, servPrefix, fixCount);
	} else {
		ACAPI_WriteReport ("ClassSync: Fix cascade failed, error %d", false, (int)err);
	}

	RefreshData ();
}


// ---------------------------------------------------------------------------
// Preferences: load
// ---------------------------------------------------------------------------

void ClassSyncPalette::LoadPreferences ()
{
	Int32 version = 0;
	GSSize nByte = 0;
	ACAPI_GetPreferences (&version, &nByte, nullptr);

	if (version == kPrefsVersion && nByte == sizeof (ClassSyncPrefs)) {
		ClassSyncPrefs prefs = {};
		ACAPI_GetPreferences (&version, &nByte, &prefs);
		if (prefs.xmlPath[0] != '\0')
			xmlFilePath = GS::UniString (prefs.xmlPath, CC_UTF8);
	}

	// Fallback to default path (master XML in repo)
	if (xmlFilePath.IsEmpty ())
		xmlFilePath = "C:\\Users\\Green\\claude\\PlantSyncAddon\\Green Accent PLANTS.xml";

	ACAPI_WriteReport ("ClassSync: Loaded prefs, XML path = %s", false,
					   xmlFilePath.ToCStr (0, MaxUSize, CC_UTF8).Get ());
}


// ---------------------------------------------------------------------------
// Preferences: save
// ---------------------------------------------------------------------------

void ClassSyncPalette::SavePreferences ()
{
	ClassSyncPrefs prefs = {};
	prefs.platform = GS::Win_Platform_Sign;

	std::string pathUtf8 (xmlFilePath.ToCStr (0, MaxUSize, CC_UTF8).Get ());
	strncpy (prefs.xmlPath, pathUtf8.c_str (), sizeof (prefs.xmlPath) - 1);

	ACAPI_SetPreferences (kPrefsVersion, sizeof (ClassSyncPrefs), &prefs);
}


// ---------------------------------------------------------------------------
// Helper: find diff status for a given classification ID
// ---------------------------------------------------------------------------

DiffStatus ClassSyncPalette::FindDiffStatus (const GS::Array<DiffEntry>& diffs,
											  const GS::UniString& id)
{
	for (UInt32 i = 0; i < diffs.GetSize (); i++) {
		if (diffs[i].id == id)
			return diffs[i].status;
	}
	return DiffStatus::Match;
}


// ---------------------------------------------------------------------------
// Helper: recursively fill tree with nodes + context-sensitive colors
// ---------------------------------------------------------------------------

void ClassSyncPalette::FillTreeWithNodes (DG::SingleSelTreeView& tree,
										   const GS::Array<ClassificationNode>& nodes,
										   Int32 parentItem,
										   UInt32& count,
										   const GS::Array<DiffEntry>& diffs,
										   TreeSide side,
										   GS::HashTable<GS::UniString, Int32>* idMap)
{
	for (UInt32 i = 0; i < nodes.GetSize (); i++) {
		const ClassificationNode& node = nodes[i];

		GS::UniString label = node.id + "  -  " + node.name;

		Int32 treeItem = tree.AppendItem (parentItem);
		tree.SetItemText (treeItem, label);
		count++;

		// Store mapping for selection sync
		if (idMap != nullptr)
			idMap->Add (node.id, treeItem);

		// Apply color based on diff status and which tree we're in
		if (diffs.GetSize () > 0) {
			DiffStatus st = FindDiffStatus (diffs, node.id);
			switch (st) {
				case DiffStatus::OnlyInProject:
					if (side == SideProject)
						tree.SetItemTextColor (treeItem, kColorNew);       // green: unique to project
					break;
				case DiffStatus::OnlyInServer:
					if (side == SideServer)
						tree.SetItemTextColor (treeItem, kColorNew);       // green: unique to server
					break;
				case DiffStatus::IdCollision:
					tree.SetItemTextColor (treeItem, kColorConflict);      // brick red: ID collision
					break;
				case DiffStatus::IdMismatch:
				case DiffStatus::DoubleConflict:
				case DiffStatus::IdCascade:
					tree.SetItemTextColor (treeItem, kColorIdMismatch);    // amber: ID mismatch
					break;
				default:
					break;
			}
		}

		// Recurse into children
		FillTreeWithNodes (tree, node.children, treeItem, count, diffs, side, idMap);
	}
}


// ---------------------------------------------------------------------------
// Clear a tree by deleting tracked root items
// ---------------------------------------------------------------------------

static void ClearTree (DG::SingleSelTreeView& tree, GS::Array<Int32>& rootItems)
{
	tree.DisableDraw ();
	for (UInt32 i = 0; i < rootItems.GetSize (); i++)
		tree.DeleteItem (rootItems[i]);
	rootItems.Clear ();
	tree.EnableDraw ();
}


// ---------------------------------------------------------------------------
// Populate Project tree (left panel)
// ---------------------------------------------------------------------------

void ClassSyncPalette::PopulateProjectTree ()
{
	ClearTree (treeProject, projectRootItems);
	projectIdToTreeItem.Clear ();
	treeProject.DisableDraw ();

	UInt32 itemCount = 0;
	UInt32 sysCount  = 0;

	for (UInt32 s = 0; s < projectData.GetSize (); s++) {
		const ClassificationTree& tree = projectData[s];
		sysCount++;

		GS::UniString sysLabel = tree.systemName + "  (v" + tree.version + ")";
		Int32 sysNode = treeProject.AppendItem (DG_TVI_ROOT);
		treeProject.SetItemText (sysNode, sysLabel);
		projectRootItems.Push (sysNode);

		FillTreeWithNodes (treeProject, tree.rootItems, sysNode, itemCount, diffEntries, SideProject, &projectIdToTreeItem);
		treeProject.ExpandItem (sysNode);
	}

	treeProject.EnableDraw ();
	treeProject.Redraw ();

	GS::UniString status = GS::UniString::Printf ("%d systems, %d items",
		sysCount, itemCount);
	countProject.SetText (status);
}


// ---------------------------------------------------------------------------
// Populate Server tree (right panel)
// ---------------------------------------------------------------------------

void ClassSyncPalette::PopulateServerTree ()
{
	ClearTree (treeServer, serverRootItems);
	serverIdToTreeItem.Clear ();
	treeServer.DisableDraw ();

	UInt32 itemCount = 0;
	UInt32 sysCount  = 0;

	for (UInt32 s = 0; s < serverData.GetSize (); s++) {
		const ClassificationTree& tree = serverData[s];
		sysCount++;

		GS::UniString sysLabel = tree.systemName + "  (v" + tree.version + ")";
		Int32 sysNode = treeServer.AppendItem (DG_TVI_ROOT);
		treeServer.SetItemText (sysNode, sysLabel);
		serverRootItems.Push (sysNode);

		FillTreeWithNodes (treeServer, tree.rootItems, sysNode, itemCount, diffEntries, SideServer, &serverIdToTreeItem);
		treeServer.ExpandItem (sysNode);
	}

	treeServer.EnableDraw ();
	treeServer.Redraw ();

	GS::UniString status = GS::UniString::Printf ("%d systems, %d items",
		sysCount, itemCount);
	countServer.SetText (status);
}


// ---------------------------------------------------------------------------
// Populate Conflicts tree (center panel)
// ---------------------------------------------------------------------------

void ClassSyncPalette::PopulateConflictsTree ()
{
	ClearTree (treeConflicts, conflictRootItems);
	conflictItemToDiffIndex.Clear ();
	treeConflicts.DisableDraw ();

	// Count by status
	UInt32 idCollisionCount  = 0;
	UInt32 idMismatchCount   = 0;
	UInt32 cascadeCount      = 0;
	UInt32 onlyProjCount     = 0;
	UInt32 onlyServCount     = 0;
	UInt32 doubleConflCount  = 0;

	for (UInt32 i = 0; i < diffEntries.GetSize (); i++) {
		switch (diffEntries[i].status) {
			case DiffStatus::IdCollision:    idCollisionCount++; break;
			case DiffStatus::IdMismatch:     idMismatchCount++;  break;
			case DiffStatus::IdCascade:      cascadeCount++;     break;
			case DiffStatus::OnlyInProject:  onlyProjCount++;    break;
			case DiffStatus::OnlyInServer:   onlyServCount++;    break;
			case DiffStatus::DoubleConflict: doubleConflCount++; break;
			default: break;
		}
	}

	UInt32 totalDiffs = idCollisionCount + idMismatchCount + cascadeCount
		+ onlyProjCount + onlyServCount + doubleConflCount;

	if (totalDiffs == 0) {
		Int32 item = treeConflicts.AppendItem (DG_TVI_ROOT);
		treeConflicts.SetItemText (item, "All items match");
		conflictRootItems.Push (item);
		treeConflicts.EnableDraw ();
		treeConflicts.Redraw ();
		countConflicts.SetText ("0 differences");
		return;
	}

	// --- ID Collisions section (S2: same ID, different Name) ---
	if (idCollisionCount > 0) {
		Int32 secItem = treeConflicts.AppendItem (DG_TVI_ROOT);
		treeConflicts.SetItemText (secItem,
			GS::UniString::Printf ("ID Collisions (%d)", idCollisionCount));
		treeConflicts.SetItemTextColor (secItem, kColorConflict);
		conflictRootItems.Push (secItem);

		for (UInt32 i = 0; i < diffEntries.GetSize (); i++) {
			if (diffEntries[i].status == DiffStatus::IdCollision) {
				Int32 child = treeConflicts.AppendItem (secItem);
				GS::UniString label = diffEntries[i].id
					+ "  P:\"" + diffEntries[i].projectName
					+ "\"  S:\"" + diffEntries[i].serverName + "\"";
				treeConflicts.SetItemText (child, label);
				treeConflicts.SetItemTextColor (child, kColorConflict);
				conflictItemToDiffIndex.Add (child, i);
			}
		}
		treeConflicts.ExpandItem (secItem);
	}

	// --- ID Mismatches section (S3: same Name, different ID) ---
	if (idMismatchCount > 0) {
		Int32 secItem = treeConflicts.AppendItem (DG_TVI_ROOT);
		treeConflicts.SetItemText (secItem,
			GS::UniString::Printf ("ID Mismatches (%d)", idMismatchCount));
		treeConflicts.SetItemTextColor (secItem, kColorIdMismatch);
		conflictRootItems.Push (secItem);

		for (UInt32 i = 0; i < diffEntries.GetSize (); i++) {
			if (diffEntries[i].status == DiffStatus::IdMismatch) {
				Int32 child = treeConflicts.AppendItem (secItem);
				GS::UniString label = "\"" + diffEntries[i].projectName
					+ "\"  P:" + diffEntries[i].id
					+ "  S:" + diffEntries[i].serverId;
				if (diffEntries[i].cascadeChildCount > 0)
					label += GS::UniString::Printf ("  (+%d)", diffEntries[i].cascadeChildCount);
				treeConflicts.SetItemText (child, label);
				treeConflicts.SetItemTextColor (child, kColorIdMismatch);
				conflictItemToDiffIndex.Add (child, i);
			}
		}
		treeConflicts.ExpandItem (secItem);
	}

	// --- ID Cascades section (H1-H4: descendants of S3) ---
	if (cascadeCount > 0) {
		Int32 secItem = treeConflicts.AppendItem (DG_TVI_ROOT);
		treeConflicts.SetItemText (secItem,
			GS::UniString::Printf ("ID Cascades (%d)", cascadeCount));
		treeConflicts.SetItemTextColor (secItem, kColorIdMismatch);
		conflictRootItems.Push (secItem);

		for (UInt32 i = 0; i < diffEntries.GetSize (); i++) {
			if (diffEntries[i].status == DiffStatus::IdCascade) {
				Int32 child = treeConflicts.AppendItem (secItem);
				GS::UniString label = diffEntries[i].id + " -> " + diffEntries[i].serverId
					+ "  \"" + diffEntries[i].projectName + "\"";
				treeConflicts.SetItemText (child, label);
				treeConflicts.SetItemTextColor (child, kColorIdMismatch);
				conflictItemToDiffIndex.Add (child, i);
			}
		}
		treeConflicts.ExpandItem (secItem);
	}

	// --- Double Conflicts section (S6: ID→A, Name→B) ---
	if (doubleConflCount > 0) {
		Int32 secItem = treeConflicts.AppendItem (DG_TVI_ROOT);
		treeConflicts.SetItemText (secItem,
			GS::UniString::Printf ("Double Conflicts (%d)", doubleConflCount));
		treeConflicts.SetItemTextColor (secItem, kColorConflict);
		conflictRootItems.Push (secItem);

		for (UInt32 i = 0; i < diffEntries.GetSize (); i++) {
			if (diffEntries[i].status == DiffStatus::DoubleConflict) {
				Int32 child = treeConflicts.AppendItem (secItem);
				GS::UniString label = diffEntries[i].id
					+ "  P:\"" + diffEntries[i].projectName
					+ "\"  S(id):\"" + diffEntries[i].serverName
					+ "\"  S(name):" + diffEntries[i].serverId;
				treeConflicts.SetItemText (child, label);
				treeConflicts.SetItemTextColor (child, kColorConflict);
				conflictItemToDiffIndex.Add (child, i);
			}
		}
		treeConflicts.ExpandItem (secItem);
	}

	// --- Only in Project section (S4) ---
	if (onlyProjCount > 0) {
		Int32 secItem = treeConflicts.AppendItem (DG_TVI_ROOT);
		treeConflicts.SetItemText (secItem,
			GS::UniString::Printf ("Only in Project (%d)", onlyProjCount));
		treeConflicts.SetItemTextColor (secItem, kColorNew);
		conflictRootItems.Push (secItem);

		for (UInt32 i = 0; i < diffEntries.GetSize (); i++) {
			if (diffEntries[i].status == DiffStatus::OnlyInProject) {
				Int32 child = treeConflicts.AppendItem (secItem);
				GS::UniString label = diffEntries[i].id + "  -  " + diffEntries[i].projectName;
				treeConflicts.SetItemText (child, label);
				treeConflicts.SetItemTextColor (child, kColorNew);
				conflictItemToDiffIndex.Add (child, i);
			}
		}
		treeConflicts.ExpandItem (secItem);
	}

	// --- Only on Server section (S5) ---
	if (onlyServCount > 0) {
		Int32 secItem = treeConflicts.AppendItem (DG_TVI_ROOT);
		treeConflicts.SetItemText (secItem,
			GS::UniString::Printf ("Only on Server (%d)", onlyServCount));
		treeConflicts.SetItemTextColor (secItem, kColorMissing);
		conflictRootItems.Push (secItem);

		for (UInt32 i = 0; i < diffEntries.GetSize (); i++) {
			if (diffEntries[i].status == DiffStatus::OnlyInServer) {
				Int32 child = treeConflicts.AppendItem (secItem);
				GS::UniString label = diffEntries[i].id + "  -  " + diffEntries[i].serverName;
				treeConflicts.SetItemText (child, label);
				treeConflicts.SetItemTextColor (child, kColorMissing);
				conflictItemToDiffIndex.Add (child, i);
			}
		}
		treeConflicts.ExpandItem (secItem);
	}

	treeConflicts.EnableDraw ();
	treeConflicts.Redraw ();

	GS::UniString status = GS::UniString::Printf ("%d differences", totalDiffs);
	countConflicts.SetText (status);
}


// ---------------------------------------------------------------------------
// Refresh: re-read project + server data, run diff, repopulate all trees
// ---------------------------------------------------------------------------

void ClassSyncPalette::SetStatus (const GS::UniString& text)
{
	countConflicts.SetText (text);
	countConflicts.Redraw ();
	DG::Panel::RedrawItems ();
}


void ClassSyncPalette::RefreshData ()
{
	if (xmlFilePath.IsEmpty ()) {
		ACAPI_WriteReport ("ClassSync: No XML path set", false);
		countServer.SetText ("No XML file");
		return;
	}

	std::string pathUtf8 (xmlFilePath.ToCStr (0, MaxUSize, CC_UTF8).Get ());

	ACAPI_WriteReport ("ClassSync v%s: RefreshData starting...", false, kClassSyncVersion);
	ACAPI_WriteReport ("ClassSync: XML path = %s", false, pathUtf8.c_str ());

	// Read project data
	SetStatus ("Reading project...");
	projectData = ReadProjectClassifications ();
	ACAPI_WriteReport ("ClassSync: Project: %d systems", false, (int)projectData.GetSize ());

	// Read server data
	SetStatus ("Reading XML...");
	serverData  = ReadXmlClassifications (pathUtf8.c_str ());
	ACAPI_WriteReport ("ClassSync: Server: %d systems", false, (int)serverData.GetSize ());

	// Build ID sets for FindNextFreeId
	allProjectIds.Clear ();
	allServerIds.Clear ();
	{
		GS::Array<ClassificationNode> stack;
		for (UInt32 s = 0; s < projectData.GetSize (); s++)
			for (UInt32 r = 0; r < projectData[s].rootItems.GetSize (); r++)
				stack.Push (projectData[s].rootItems[r]);
		while (stack.GetSize () > 0) {
			ClassificationNode node = stack.GetLast ();
			stack.DeleteLast ();
			allProjectIds.Add (node.id);
			for (UInt32 c = 0; c < node.children.GetSize (); c++)
				stack.Push (node.children[c]);
		}
		for (UInt32 s = 0; s < serverData.GetSize (); s++)
			for (UInt32 r = 0; r < serverData[s].rootItems.GetSize (); r++)
				stack.Push (serverData[s].rootItems[r]);
		while (stack.GetSize () > 0) {
			ClassificationNode node = stack.GetLast ();
			stack.DeleteLast ();
			allServerIds.Add (node.id);
			for (UInt32 c = 0; c < node.children.GetSize (); c++)
				stack.Push (node.children[c]);
		}
	}

	// Run diff
	SetStatus ("Comparing...");
	diffEntries = CompareClassifications (projectData, serverData);

	UInt32 matches = 0, idCollisions = 0, idMismatches = 0;
	UInt32 onlyProj = 0, onlyServ = 0, doubleConflicts = 0, cascades = 0;
	for (UInt32 i = 0; i < diffEntries.GetSize (); i++) {
		switch (diffEntries[i].status) {
			case DiffStatus::Match:          matches++;         break;
			case DiffStatus::IdCollision:    idCollisions++;    break;
			case DiffStatus::IdMismatch:     idMismatches++;    break;
			case DiffStatus::OnlyInProject:  onlyProj++;        break;
			case DiffStatus::OnlyInServer:   onlyServ++;        break;
			case DiffStatus::DoubleConflict: doubleConflicts++; break;
			case DiffStatus::IdCascade:      cascades++;        break;
		}
	}
	ACAPI_WriteReport ("ClassSync: Diff: %d match, %d id-collision, %d id-mismatch, %d only-project, %d only-server, %d double-conflict, %d cascade",
		false, matches, idCollisions, idMismatches, onlyProj, onlyServ, doubleConflicts, cascades);

	// Populate trees
	SetStatus ("Updating trees...");
	PopulateProjectTree ();
	PopulateServerTree ();
	PopulateConflictsTree ();

	// Check lock status (may have changed since last refresh)
	CheckLockStatus ();
	UpdateActionButtons ();

	ACAPI_WriteReport ("ClassSync: RefreshData done.", false);
}


// ---------------------------------------------------------------------------
// Toggle write lock on the XML database
// ---------------------------------------------------------------------------

void ClassSyncPalette::DoToggleLock ()
{
	if (xmlFilePath.IsEmpty ()) return;

	if (writeMode) {
		// Release our lock
		ReleaseLock (xmlFilePath);
		writeMode = false;
		buttonLock.SetText ("Open for write");
		labelWriteMode.Hide ();
		ACAPI_WriteReport ("ClassSync: Write lock released", false);
		UpdateActionButtons ();
		return;
	}

	// Try to acquire lock
	LockInfo info = GetLockInfo (xmlFilePath);
	if (info.locked) {
		// Someone else holds the lock - show alert
		GS::UniString msg = "Database is locked by " + info.user
			+ " since " + info.time
			+ "\nClick Refresh to check if it becomes available.";
		DGAlert (DG_INFORMATION, "ClassSync", "Database is locked", msg, "OK");
		return;
	}

	if (AcquireLock (xmlFilePath)) {
		writeMode = true;
		buttonLock.SetText ("Close write");
		labelWriteMode.Show ();
		ACAPI_WriteReport ("ClassSync: Write lock acquired", false);
		UpdateActionButtons ();
	} else {
		ACAPI_WriteReport ("ClassSync: Failed to acquire write lock", false);
	}
}


// ---------------------------------------------------------------------------
// Check lock file status and update UI accordingly
// ---------------------------------------------------------------------------

void ClassSyncPalette::CheckLockStatus ()
{
	if (xmlFilePath.IsEmpty ()) {
		writeMode = false;
		buttonLock.Disable ();
		return;
	}

	buttonLock.Enable ();

	if (IsLockedByUs (xmlFilePath)) {
		writeMode = true;
		buttonLock.SetText ("Close write");
		labelWriteMode.Show ();
	} else {
		writeMode = false;
		buttonLock.SetText ("Open for write");
		labelWriteMode.Hide ();
	}
}


// ---------------------------------------------------------------------------
// Release lock if we hold it (static, safe to call from FreeData)
// ---------------------------------------------------------------------------

void ClassSyncPalette::ReleaseLockIfHeld ()
{
	if (instance != nullptr && instance->writeMode) {
		ReleaseLock (xmlFilePath);
		instance->writeMode = false;
		ACAPI_WriteReport ("ClassSync: Write lock auto-released", false);
	}
}
