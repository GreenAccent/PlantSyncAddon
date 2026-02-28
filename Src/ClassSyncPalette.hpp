#ifndef CLASSSYNCPALETTE_HPP
#define CLASSSYNCPALETTE_HPP

#include "APIEnvir.h"
#include "ACAPinc.h"
#include "DGModule.hpp"
#include "Color.hpp"
#include "ClassificationData.hpp"


// ---------------------------------------------------------------------------
// Resource IDs (must match ClassSync.grc)
// ---------------------------------------------------------------------------

enum {
	ClassSyncPaletteResId = 32600,

	ItemLabelProject    = 1,
	ItemLabelConflicts  = 2,
	ItemLabelServer     = 3,
	ItemTreeProject     = 4,
	ItemTreeConflicts   = 5,
	ItemTreeServer      = 6,
	ItemCountProject    = 7,
	ItemCountConflicts  = 8,
	ItemCountServer     = 9,
	ItemButtonRefresh   = 10,
	ItemButtonClose     = 11
};


// ---------------------------------------------------------------------------
// Default XML path (hardcoded for now)
// ---------------------------------------------------------------------------

static const char* kDefaultXmlPath = "C:\\Users\\Green\\claude\\Green Accent PLANTS.xml";


// ---------------------------------------------------------------------------
// Palette class
// ---------------------------------------------------------------------------

class ClassSyncPalette : public DG::Palette,
						 public DG::PanelObserver,
						 public DG::ButtonItemObserver
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

private:
	// DG::PanelObserver
	virtual void  PanelCloseRequested (const DG::PanelCloseRequestEvent& ev, bool* accepted) override;

	// DG::ButtonItemObserver
	virtual void  ButtonClicked (const DG::ButtonClickEvent& ev) override;

	// Tree population
	void  PopulateProjectTree ();
	void  PopulateServerTree ();
	void  PopulateConflictsTree ();

	static void  FillTreeWithNodes (DG::SingleSelTreeView& tree,
									const GS::Array<ClassificationNode>& nodes,
									Int32 parentItem,
									UInt32& count,
									const GS::Array<DiffEntry>& diffs);

	static DiffStatus  FindDiffStatus (const GS::Array<DiffEntry>& diffs,
									   const GS::UniString& id);

	// Controls
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

	// Data
	GS::Array<ClassificationTree>   projectData;
	GS::Array<ClassificationTree>   serverData;
	GS::Array<DiffEntry>            diffEntries;

	// Root items for clearing trees
	GS::Array<Int32>  projectRootItems;
	GS::Array<Int32>  serverRootItems;
	GS::Array<Int32>  conflictRootItems;

	// Singleton
	static ClassSyncPalette*  instance;
	static bool               wasOpenBeforeHide;
};


#endif // CLASSSYNCPALETTE_HPP
