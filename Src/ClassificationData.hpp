#ifndef CLASSIFICATIONDATA_HPP
#define CLASSIFICATIONDATA_HPP

#include "APIEnvir.h"
#include "ACAPinc.h"


// ---------------------------------------------------------------------------
// Data structures
// ---------------------------------------------------------------------------

struct ClassificationNode {
	GS::UniString  id;
	GS::UniString  name;
	GS::UniString  description;
	API_Guid       guid;		// APINULLGuid for XML-sourced items
	GS::Array<ClassificationNode>  children;
};

struct ClassificationTree {
	GS::UniString  systemName;
	GS::UniString  version;
	API_Guid       systemGuid;	// APINULLGuid for XML-sourced trees
	GS::Array<ClassificationNode>  rootItems;
};

enum class DiffStatus {
	Match,           // S1 — full match (same ID and Name)
	IdCollision,     // S2 — same ID, different plants (different Name)
	IdMismatch,      // S3 — same plant (same Name), different ID
	OnlyInProject,   // S4 — exists only in project
	OnlyInServer,    // S5 — exists only on server
	DoubleConflict,  // S6 — ID matches plant A, Name matches plant B
	IdCascade        // H1-H4 — descendant of an IdMismatch parent
};

struct DiffEntry {
	GS::UniString  id;              // project-side ID (or server ID for S5)
	GS::UniString  projectName;
	GS::UniString  serverName;
	GS::UniString  description;
	DiffStatus     status;
	API_Guid       projectItemGuid;		// GUID of item in project
	API_Guid       projectSystemGuid;	// GUID of system in project

	// S3/S6/IdCascade: the matching server-side ID (differs from project id)
	GS::UniString  serverId;

	// IdCascade: prefix pair that caused the cascade
	GS::UniString  cascadeParentProjectId;
	GS::UniString  cascadeParentServerId;

	// IdCascade parent entry: number of affected descendants
	UInt32          cascadeChildCount;
};


// ---------------------------------------------------------------------------
// Functions
// ---------------------------------------------------------------------------

GS::Array<ClassificationTree>  ReadProjectClassifications ();

GS::Array<DiffEntry>  CompareClassifications (
	const GS::Array<ClassificationTree>& project,
	const GS::Array<ClassificationTree>& server);


#endif // CLASSIFICATIONDATA_HPP
