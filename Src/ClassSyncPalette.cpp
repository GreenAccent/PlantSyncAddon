#include "ClassSyncPalette.hpp"
#include "XmlReader.hpp"


// ---------------------------------------------------------------------------
// Palette GUID (unique, for Work Environment persistence)
// ---------------------------------------------------------------------------

static const GS::Guid kPaletteGuid ("C1A55710-DA7A-4B2E-9F3C-8E2D1A6B5C4E");


// ---------------------------------------------------------------------------
// Static members
// ---------------------------------------------------------------------------

ClassSyncPalette*  ClassSyncPalette::instance          = nullptr;
bool               ClassSyncPalette::wasOpenBeforeHide = false;


// ---------------------------------------------------------------------------
// Color constants
// ---------------------------------------------------------------------------

static const Gfx::Color kColorConflict     (200,   0,   0);    // red
static const Gfx::Color kColorOnlyProject  (200, 120,   0);    // orange
static const Gfx::Color kColorOnlyServer   (  0, 100, 200);    // blue


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
	buttonClose        (GetReference (), ItemButtonClose)
{
	Attach (*this);
	buttonRefresh.Attach (*this);
	buttonClose.Attach (*this);
	BeginEventProcessing ();

	RefreshData ();
}


// ---------------------------------------------------------------------------
// Destructor
// ---------------------------------------------------------------------------

ClassSyncPalette::~ClassSyncPalette ()
{
	EndEventProcessing ();
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
	}
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
// Helper: recursively fill tree with nodes + apply diff colors
// ---------------------------------------------------------------------------

void ClassSyncPalette::FillTreeWithNodes (DG::SingleSelTreeView& tree,
										   const GS::Array<ClassificationNode>& nodes,
										   Int32 parentItem,
										   UInt32& count,
										   const GS::Array<DiffEntry>& diffs)
{
	for (UInt32 i = 0; i < nodes.GetSize (); i++) {
		const ClassificationNode& node = nodes[i];

		GS::UniString label = node.id + "  -  " + node.name;

		Int32 treeItem = tree.AppendItem (parentItem);
		tree.SetItemText (treeItem, label);
		count++;

		// Apply color based on diff status
		if (diffs.GetSize () > 0) {
			DiffStatus st = FindDiffStatus (diffs, node.id);
			switch (st) {
				case DiffStatus::Conflict:
					tree.SetItemTextColor (treeItem, kColorConflict);
					break;
				case DiffStatus::OnlyInProject:
					tree.SetItemTextColor (treeItem, kColorOnlyProject);
					break;
				case DiffStatus::OnlyInServer:
					tree.SetItemTextColor (treeItem, kColorOnlyServer);
					break;
				default:
					break;
			}
		}

		// Recurse into children
		FillTreeWithNodes (tree, node.children, treeItem, count, diffs);
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

		FillTreeWithNodes (treeProject, tree.rootItems, sysNode, itemCount, diffEntries);
		treeProject.ExpandItem (sysNode);
	}

	treeProject.EnableDraw ();

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

		FillTreeWithNodes (treeServer, tree.rootItems, sysNode, itemCount, diffEntries);
		treeServer.ExpandItem (sysNode);
	}

	treeServer.EnableDraw ();

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
		countConflicts.SetText ("0 differences");
		return;
	}

	// Conflicts section (same ID, different name)
	if (conflictCount > 0) {
		Int32 secItem = treeConflicts.AppendItem (DG_TVI_ROOT);
		treeConflicts.SetItemText (secItem,
			GS::UniString::Printf ("Name Conflicts (%d)", conflictCount));
		treeConflicts.SetItemTextColor (secItem, kColorConflict);
		conflictRootItems.Push (secItem);

		for (UInt32 i = 0; i < diffEntries.GetSize (); i++) {
			if (diffEntries[i].status == DiffStatus::Conflict) {
				Int32 child = treeConflicts.AppendItem (secItem);
				GS::UniString label = diffEntries[i].id
					+ "  P: " + diffEntries[i].projectName
					+ "  S: " + diffEntries[i].serverName;
				treeConflicts.SetItemText (child, label);
				treeConflicts.SetItemTextColor (child, kColorConflict);
			}
		}
		treeConflicts.ExpandItem (secItem);
	}

	// Only in Project section
	if (onlyProjCount > 0) {
		Int32 secItem = treeConflicts.AppendItem (DG_TVI_ROOT);
		treeConflicts.SetItemText (secItem,
			GS::UniString::Printf ("Only in Project (%d)", onlyProjCount));
		treeConflicts.SetItemTextColor (secItem, kColorOnlyProject);
		conflictRootItems.Push (secItem);

		for (UInt32 i = 0; i < diffEntries.GetSize (); i++) {
			if (diffEntries[i].status == DiffStatus::OnlyInProject) {
				Int32 child = treeConflicts.AppendItem (secItem);
				GS::UniString label = diffEntries[i].id + "  -  " + diffEntries[i].projectName;
				treeConflicts.SetItemText (child, label);
				treeConflicts.SetItemTextColor (child, kColorOnlyProject);
			}
		}
		treeConflicts.ExpandItem (secItem);
	}

	// Only on Server section
	if (onlyServCount > 0) {
		Int32 secItem = treeConflicts.AppendItem (DG_TVI_ROOT);
		treeConflicts.SetItemText (secItem,
			GS::UniString::Printf ("Only on Server (%d)", onlyServCount));
		treeConflicts.SetItemTextColor (secItem, kColorOnlyServer);
		conflictRootItems.Push (secItem);

		for (UInt32 i = 0; i < diffEntries.GetSize (); i++) {
			if (diffEntries[i].status == DiffStatus::OnlyInServer) {
				Int32 child = treeConflicts.AppendItem (secItem);
				GS::UniString label = diffEntries[i].id + "  -  " + diffEntries[i].serverName;
				treeConflicts.SetItemText (child, label);
				treeConflicts.SetItemTextColor (child, kColorOnlyServer);
			}
		}
		treeConflicts.ExpandItem (secItem);
	}

	treeConflicts.EnableDraw ();

	GS::UniString status = GS::UniString::Printf ("%d differences", totalDiffs);
	countConflicts.SetText (status);
}


// ---------------------------------------------------------------------------
// Refresh: re-read project + server data, run diff, repopulate all trees
// ---------------------------------------------------------------------------

void ClassSyncPalette::RefreshData ()
{
	// Read data
	projectData = ReadProjectClassifications ();
	serverData  = ReadXmlClassifications (kDefaultXmlPath);

	// Run diff
	diffEntries = CompareClassifications (projectData, serverData);

	// Populate trees
	PopulateProjectTree ();
	PopulateServerTree ();
	PopulateConflictsTree ();
}
