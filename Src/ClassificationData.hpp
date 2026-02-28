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
	Match,
	Conflict,
	OnlyInProject,
	OnlyInServer
};

struct DiffEntry {
	GS::UniString  id;
	GS::UniString  projectName;
	GS::UniString  serverName;
	GS::UniString  description;
	DiffStatus     status;
	API_Guid       projectItemGuid;		// GUID of item in project
	API_Guid       projectSystemGuid;	// GUID of system in project
};


// ---------------------------------------------------------------------------
// Functions
// ---------------------------------------------------------------------------

GS::Array<ClassificationTree>  ReadProjectClassifications ();

GS::Array<DiffEntry>  CompareClassifications (
	const GS::Array<ClassificationTree>& project,
	const GS::Array<ClassificationTree>& server);


#endif // CLASSIFICATIONDATA_HPP
