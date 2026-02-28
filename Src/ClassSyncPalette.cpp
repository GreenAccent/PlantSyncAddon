#include "ClassSyncPalette.hpp"
#include "XmlReader.hpp"
#include "XmlWriter.hpp"
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

static const Gfx::Color kColorNew      (  0, 130,  60);   // dark green  - unique to this side
static const Gfx::Color kColorMissing  (  0,  80, 170);   // dark blue   - missing (exists on other side)
static const Gfx::Color kColorConflict (180,  50,   0);   // brick red   - conflict (same ID, different name)


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
	buttonUseProject   (GetReference (), ItemButtonUseProject),
	buttonUseServer    (GetReference (), ItemButtonUseServer),
	labelVersion       (GetReference (), ItemLabelVersion)
{
	Attach (*this);
	buttonRefresh.Attach (*this);
	buttonClose.Attach (*this);
	buttonBrowse.Attach (*this);
	buttonImport.Attach (*this);
	buttonExport.Attach (*this);
	buttonUseProject.Attach (*this);
	buttonUseServer.Attach (*this);
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
	buttonUseProject.Disable ();
	buttonUseServer.Disable ();

	ACAPI_WriteReport ("ClassSync v%s started", false, kClassSyncVersion);

	if (!xmlFilePath.IsEmpty ())
		RefreshData ();
}


// ---------------------------------------------------------------------------
// Destructor
// ---------------------------------------------------------------------------

ClassSyncPalette::~ClassSyncPalette ()
{
	EndEventProcessing ();
	treeConflicts.Detach (static_cast<DG::TreeViewObserver&> (*this));
	buttonUseServer.Detach (*this);
	buttonUseProject.Detach (*this);
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
	} else if (ev.GetSource () == &buttonUseProject) {
		DoUseProject ();
	} else if (ev.GetSource () == &buttonUseServer) {
		DoUseServer ();
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
// Update action button enabled states based on conflicts tree selection
// ---------------------------------------------------------------------------

void ClassSyncPalette::UpdateActionButtons ()
{
	Int32 selected = treeConflicts.GetSelectedItem ();

	if (selected == 0 || selected == DG::TreeView::RootItem) {
		buttonImport.Disable ();
		buttonExport.Disable ();
		buttonUseProject.Disable ();
		buttonUseServer.Disable ();
		return;
	}

	UInt32 diffIdx;
	if (!conflictItemToDiffIndex.Get (selected, &diffIdx)) {
		// Section header or non-actionable item
		buttonImport.Disable ();
		buttonExport.Disable ();
		buttonUseProject.Disable ();
		buttonUseServer.Disable ();
		return;
	}

	const DiffEntry& entry = diffEntries[diffIdx];

	buttonImport.Disable ();
	buttonExport.Disable ();
	buttonUseProject.Disable ();
	buttonUseServer.Disable ();

	switch (entry.status) {
		case DiffStatus::OnlyInServer:
			buttonImport.Enable ();
			break;
		case DiffStatus::OnlyInProject:
			buttonExport.Enable ();
			break;
		case DiffStatus::Conflict:
			buttonUseProject.Enable ();
			buttonUseServer.Enable ();
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
		xmlFilePath = loc.ToDisplayText ();
		labelXmlPath.SetText (xmlFilePath);
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

	if (err == NoError)
		ACAPI_WriteReport ("ClassSync: Import successful", false);
	else
		ACAPI_WriteReport ("ClassSync: Import failed, error %d", false, (int)err);

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

	if (success)
		ACAPI_WriteReport ("ClassSync: Exported '%s' to XML", false,
						   entry.id.ToCStr ().Get ());
	else
		ACAPI_WriteReport ("ClassSync: Export failed for '%s'", false,
						   entry.id.ToCStr ().Get ());

	RefreshData ();
}


// ---------------------------------------------------------------------------
// Use Project: update XML item name to match project
// ---------------------------------------------------------------------------

void ClassSyncPalette::DoUseProject ()
{
	if (xmlFilePath.IsEmpty ()) return;

	Int32 selected = treeConflicts.GetSelectedItem ();
	UInt32 diffIdx;
	if (!conflictItemToDiffIndex.Get (selected, &diffIdx)) return;

	const DiffEntry& entry = diffEntries[diffIdx];
	if (entry.status != DiffStatus::Conflict) return;

	std::string pathUtf8 (xmlFilePath.ToCStr (0, MaxUSize, CC_UTF8).Get ());

	bool success = ChangeItemNameInXml (pathUtf8.c_str (), entry.id, entry.projectName);

	if (success)
		ACAPI_WriteReport ("ClassSync: XML updated - '%s' name -> '%s'", false,
						   entry.id.ToCStr ().Get (),
						   entry.projectName.ToCStr ().Get ());
	else
		ACAPI_WriteReport ("ClassSync: Failed to update XML for '%s'", false,
						   entry.id.ToCStr ().Get ());

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
	if (entry.status != DiffStatus::Conflict) return;
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

	if (err == NoError)
		ACAPI_WriteReport ("ClassSync: Project item '%s' name -> '%s'", false,
						   entry.id.ToCStr ().Get (),
						   entry.serverName.ToCStr ().Get ());
	else
		ACAPI_WriteReport ("ClassSync: Failed to change project item '%s', error %d", false,
						   entry.id.ToCStr ().Get (), (int)err);

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
										   TreeSide side)
{
	for (UInt32 i = 0; i < nodes.GetSize (); i++) {
		const ClassificationNode& node = nodes[i];

		GS::UniString label = node.id + "  -  " + node.name;

		Int32 treeItem = tree.AppendItem (parentItem);
		tree.SetItemText (treeItem, label);
		count++;

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
				case DiffStatus::Conflict:
					tree.SetItemTextColor (treeItem, kColorConflict);      // brick red: conflict
					break;
				default:
					break;
			}
		}

		// Recurse into children
		FillTreeWithNodes (tree, node.children, treeItem, count, diffs, side);
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

		FillTreeWithNodes (treeProject, tree.rootItems, sysNode, itemCount, diffEntries, SideProject);
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

		FillTreeWithNodes (treeServer, tree.rootItems, sysNode, itemCount, diffEntries, SideServer);
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
	UInt32 conflictCount  = 0;
	UInt32 onlyProjCount  = 0;
	UInt32 onlyServCount  = 0;

	for (UInt32 i = 0; i < diffEntries.GetSize (); i++) {
		switch (diffEntries[i].status) {
			case DiffStatus::Conflict:      conflictCount++;  break;
			case DiffStatus::OnlyInProject: onlyProjCount++;  break;
			case DiffStatus::OnlyInServer:  onlyServCount++;  break;
			default: break;
		}
	}

	UInt32 totalDiffs = conflictCount + onlyProjCount + onlyServCount;

	if (totalDiffs == 0) {
		Int32 item = treeConflicts.AppendItem (DG_TVI_ROOT);
		treeConflicts.SetItemText (item, "All items match");
		conflictRootItems.Push (item);
		treeConflicts.EnableDraw ();
		treeConflicts.Redraw ();
		countConflicts.SetText ("0 differences");
		return;
	}

	// Conflicts section (same ID, different name)
	if (conflictCount > 0) {
		Int32 secItem = treeConflicts.AppendItem (DG_TVI_ROOT);
		treeConflicts.SetItemText (secItem,
			GS::UniString::Printf ("Conflicts (%d)", conflictCount));
		treeConflicts.SetItemTextColor (secItem, kColorConflict);
		conflictRootItems.Push (secItem);

		for (UInt32 i = 0; i < diffEntries.GetSize (); i++) {
			if (diffEntries[i].status == DiffStatus::Conflict) {
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

	// Only in Project section
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

	// Only on Server section
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

	// Run diff
	SetStatus ("Comparing...");
	diffEntries = CompareClassifications (projectData, serverData);

	UInt32 matches = 0, conflicts = 0, onlyProj = 0, onlyServ = 0;
	for (UInt32 i = 0; i < diffEntries.GetSize (); i++) {
		switch (diffEntries[i].status) {
			case DiffStatus::Match:         matches++;   break;
			case DiffStatus::Conflict:      conflicts++; break;
			case DiffStatus::OnlyInProject: onlyProj++;  break;
			case DiffStatus::OnlyInServer:  onlyServ++;  break;
		}
	}
	ACAPI_WriteReport ("ClassSync: Diff: %d match, %d conflict, %d only-project, %d only-server",
		false, matches, conflicts, onlyProj, onlyServ);

	// Populate trees
	SetStatus ("Updating trees...");
	PopulateProjectTree ();
	PopulateServerTree ();
	PopulateConflictsTree ();
	UpdateActionButtons ();

	ACAPI_WriteReport ("ClassSync: RefreshData done.", false);
}
