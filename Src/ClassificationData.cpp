#include "ClassificationData.hpp"


// ---------------------------------------------------------------------------
// Helper: recursively read children from ArchiCAD classification API
// ---------------------------------------------------------------------------

static void ReadChildrenRecursive (const API_Guid& parentGuid,
								   GS::Array<ClassificationNode>& result)
{
	GS::Array<API_ClassificationItem> children;
	if (ACAPI_Classification_GetClassificationItemChildren (parentGuid, children) != NoError)
		return;

	for (const auto& child : children) {
		API_ClassificationItem fullItem = {};
		fullItem.guid = child.guid;
		if (ACAPI_Classification_GetClassificationItem (fullItem) != NoError)
			continue;

		ClassificationNode node;
		node.id          = fullItem.id;
		node.name        = fullItem.name;
		node.description = fullItem.description;

		ReadChildrenRecursive (fullItem.guid, node.children);
		result.Push (node);
	}
}


// ---------------------------------------------------------------------------
// Read classification trees from current ArchiCAD project
// ---------------------------------------------------------------------------

GS::Array<ClassificationTree> ReadProjectClassifications ()
{
	GS::Array<ClassificationTree> result;

	GS::Array<API_ClassificationSystem> systems;
	if (ACAPI_Classification_GetClassificationSystems (systems) != NoError)
		return result;

	for (const auto& system : systems) {
		ClassificationTree tree;
		tree.systemName = system.name;
		tree.version    = system.editionVersion;

		GS::Array<API_ClassificationItem> rootItems;
		if (ACAPI_Classification_GetClassificationSystemRootItems (system.guid, rootItems) != NoError)
			continue;

		for (const auto& rootItem : rootItems) {
			API_ClassificationItem fullItem = {};
			fullItem.guid = rootItem.guid;
			if (ACAPI_Classification_GetClassificationItem (fullItem) != NoError)
				continue;

			ClassificationNode node;
			node.id          = fullItem.id;
			node.name        = fullItem.name;
			node.description = fullItem.description;

			ReadChildrenRecursive (fullItem.guid, node.children);
			tree.rootItems.Push (node);
		}

		result.Push (tree);
	}

	return result;
}


// ---------------------------------------------------------------------------
// Helper: flatten tree into an array of (id, name) pairs
// ---------------------------------------------------------------------------

struct FlatItem {
	GS::UniString id;
	GS::UniString name;
};

static void FlattenHelper (const GS::Array<ClassificationNode>& nodes,
						   GS::Array<FlatItem>& result)
{
	for (UInt32 i = 0; i < nodes.GetSize (); i++) {
		FlatItem fi;
		fi.id   = nodes[i].id;
		fi.name = nodes[i].name;
		result.Push (fi);
		FlattenHelper (nodes[i].children, result);
	}
}


// ---------------------------------------------------------------------------
// Compare project and server classification trees
// ---------------------------------------------------------------------------

GS::Array<DiffEntry> CompareClassifications (
	const GS::Array<ClassificationTree>& project,
	const GS::Array<ClassificationTree>& server)
{
	GS::Array<DiffEntry> result;

	// Flatten both sides
	GS::Array<FlatItem> projectItems;
	for (UInt32 s = 0; s < project.GetSize (); s++)
		FlattenHelper (project[s].rootItems, projectItems);

	GS::Array<FlatItem> serverItems;
	for (UInt32 s = 0; s < server.GetSize (); s++)
		FlattenHelper (server[s].rootItems, serverItems);

	// For each project item, check if it exists in server
	for (UInt32 i = 0; i < projectItems.GetSize (); i++) {
		DiffEntry entry;
		entry.id          = projectItems[i].id;
		entry.projectName = projectItems[i].name;

		bool found = false;
		for (UInt32 j = 0; j < serverItems.GetSize (); j++) {
			if (serverItems[j].id == projectItems[i].id) {
				entry.serverName = serverItems[j].name;
				entry.status = (projectItems[i].name == serverItems[j].name)
					? DiffStatus::Match
					: DiffStatus::Conflict;
				found = true;
				break;
			}
		}
		if (!found)
			entry.status = DiffStatus::OnlyInProject;

		result.Push (entry);
	}

	// Find items only in server
	for (UInt32 j = 0; j < serverItems.GetSize (); j++) {
		bool found = false;
		for (UInt32 i = 0; i < projectItems.GetSize (); i++) {
			if (projectItems[i].id == serverItems[j].id) {
				found = true;
				break;
			}
		}
		if (!found) {
			DiffEntry entry;
			entry.id         = serverItems[j].id;
			entry.serverName = serverItems[j].name;
			entry.status     = DiffStatus::OnlyInServer;
			result.Push (entry);
		}
	}

	return result;
}
