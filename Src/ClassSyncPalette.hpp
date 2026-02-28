#ifndef CLASSSYNCPALETTE_HPP
#define CLASSSYNCPALETTE_HPP

#include "APIEnvir.h"
#include "ACAPinc.h"
#include "DGModule.hpp"
#include "Color.hpp"
#include "HashTable.hpp"
#include "ClassificationData.hpp"


// ---------------------------------------------------------------------------
// Version
// ---------------------------------------------------------------------------

static const char* kClassSyncVersion = "0.5";


// ---------------------------------------------------------------------------
// Preferences structure
// ---------------------------------------------------------------------------

struct ClassSyncPrefs {
	unsigned short  platform;
	char            xmlPath[1024];
};

static const Int32 kPrefsVersion = 1;


// ---------------------------------------------------------------------------
// Resource IDs (must match ClassSync.grc)
// ---------------------------------------------------------------------------

enum {
	ClassSyncPaletteResId = 32600,

	ItemLabelProject     = 1,
	ItemLabelConflicts   = 2,
	ItemLabelServer      = 3,
	ItemTreeProject      = 4,
	ItemTreeConflicts    = 5,
	ItemTreeServer       = 6,
	ItemCountProject     = 7,
	ItemCountConflicts   = 8,
	ItemCountServer      = 9,
	ItemButtonRefresh    = 10,
	ItemButtonClose      = 11,
	ItemLabelXmlPath     = 12,
	ItemButtonBrowse     = 13,
	ItemButtonImport     = 14,
	ItemButtonExport     = 15,
	ItemButtonUseProject = 16,
	ItemButtonUseServer  = 17,
	ItemLabelVersion     = 18
};


// ---------------------------------------------------------------------------
// Which tree is being populated (for context-sensitive coloring)
// ---------------------------------------------------------------------------

enum TreeSide {
	SideProject,
	SideServer
};


// ---------------------------------------------------------------------------
// Palette class
// ---------------------------------------------------------------------------

class ClassSyncPalette : public DG::Palette,
						 public DG::PanelObserver,
						 public DG::ButtonItemObserver,
						 public DG::TreeViewObserver
{
public:
	ClassSyncPalette ();
	~ClassSyncPalette ();

	// Singleton
	static bool                HasInstance ();
	static ClassSyncPalette&   GetInstance ();
	static void                CreateInstance ();
	static void                DestroyInstance ();

	// ArchiCAD palette message callback
	static GSErrCode  PaletteControlCallback (Int32 referenceID,
											  API_PaletteMessageID messageID,
											  GS::IntPtr param);

	static Int32      GetRefId ()        { return 'CSYN'; }
	static GS::Guid   GetPaletteGuid ();

	// Refresh all data
	void  RefreshData ();

	// Preferences
	static void  LoadPreferences ();
	static void  SavePreferences ();

private:
	// DG::PanelObserver
	virtual void  PanelCloseRequested (const DG::PanelCloseRequestEvent& ev, bool* accepted) override;

	// DG::ButtonItemObserver
	virtual void  ButtonClicked (const DG::ButtonClickEvent& ev) override;

	// DG::TreeViewObserver
	virtual void  TreeViewSelectionChanged (const DG::TreeViewSelectionEvent& ev) override;

	// Tree population
	void  PopulateProjectTree ();
	void  PopulateServerTree ();
	void  PopulateConflictsTree ();

	static void  FillTreeWithNodes (DG::SingleSelTreeView& tree,
									const GS::Array<ClassificationNode>& nodes,
									Int32 parentItem,
									UInt32& count,
									const GS::Array<DiffEntry>& diffs,
									TreeSide side);

	static DiffStatus  FindDiffStatus (const GS::Array<DiffEntry>& diffs,
									   const GS::UniString& id);

	// Status
	void  SetStatus (const GS::UniString& text);

	// Actions
	void  BrowseForXml ();
	void  DoImportFromServer ();
	void  DoExportToServer ();
	void  DoUseProject ();
	void  DoUseServer ();
	void  UpdateActionButtons ();

	// Controls (items 1-11, existing)
	DG::LeftText            labelProject;
	DG::LeftText            labelConflicts;
	DG::LeftText            labelServer;
	DG::SingleSelTreeView   treeProject;
	DG::SingleSelTreeView   treeConflicts;
	DG::SingleSelTreeView   treeServer;
	DG::LeftText            countProject;
	DG::LeftText            countConflicts;
	DG::LeftText            countServer;
	DG::Button              buttonRefresh;
	DG::Button              buttonClose;

	// Controls (items 12-18, new)
	DG::LeftText            labelXmlPath;
	DG::Button              buttonBrowse;
	DG::Button              buttonImport;
	DG::Button              buttonExport;
	DG::Button              buttonUseProject;
	DG::Button              buttonUseServer;
	DG::LeftText            labelVersion;

	// Data
	GS::Array<ClassificationTree>   projectData;
	GS::Array<ClassificationTree>   serverData;
	GS::Array<DiffEntry>            diffEntries;

	// Root items for clearing trees
	GS::Array<Int32>  projectRootItems;
	GS::Array<Int32>  serverRootItems;
	GS::Array<Int32>  conflictRootItems;

	// Mapping: conflicts tree item ID -> index in diffEntries
	GS::HashTable<Int32, UInt32>  conflictItemToDiffIndex;

	// XML file path (loaded from preferences)
	static GS::UniString  xmlFilePath;

	// Singleton
	static ClassSyncPalette*  instance;
	static bool               wasOpenBeforeHide;
};


#endif // CLASSSYNCPALETTE_HPP
