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
	GS::Array<ClassificationNode>  children;
};

struct ClassificationTree {
	GS::UniString  systemName;
	GS::UniString  version;
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
	DiffStatus     status;
};


// ---------------------------------------------------------------------------
// Functions
// ---------------------------------------------------------------------------

GS::Array<ClassificationTree>  ReadProjectClassifications ();

GS::Array<DiffEntry>  CompareClassifications (
	const GS::Array<ClassificationTree>& project,
	const GS::Array<ClassificationTree>& server);


#endif // CLASSIFICATIONDATA_HPP
