#include "ClassificationData.hpp"
#include "HashTable.hpp"
#include "HashSet.hpp"


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
		node.guid        = fullItem.guid;

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
		tree.systemGuid = system.guid;

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
			node.guid        = fullItem.guid;

			ReadChildrenRecursive (fullItem.guid, node.children);
			tree.rootItems.Push (node);
		}

		result.Push (tree);
	}

	return result;
}


// ---------------------------------------------------------------------------
// Helper: flatten tree into an array for comparison
// ---------------------------------------------------------------------------

struct FlatItem {
	GS::UniString  id;
	GS::UniString  name;
	GS::UniString  description;
	GS::UniString  parentId;
	API_Guid       guid;
	API_Guid       systemGuid;
};

static void FlattenHelper (const GS::Array<ClassificationNode>& nodes,
						   GS::Array<FlatItem>& result,
						   const API_Guid& systemGuid,
						   const GS::UniString& parentId = GS::UniString ())
{
	for (UInt32 i = 0; i < nodes.GetSize (); i++) {
		FlatItem fi;
		fi.id          = nodes[i].id;
		fi.name        = nodes[i].name;
		fi.description = nodes[i].description;
		fi.parentId    = parentId;
		fi.guid        = nodes[i].guid;
		fi.systemGuid  = systemGuid;
		result.Push (fi);
		FlattenHelper (nodes[i].children, result, systemGuid, nodes[i].id);
	}
}


// ---------------------------------------------------------------------------
// Helper: check if an ID starts with a given prefix + "."
// ---------------------------------------------------------------------------

static bool HasPrefix (const GS::UniString& id, const GS::UniString& prefix)
{
	if (id.GetLength () <= prefix.GetLength ())
		return false;
	return id.BeginsWith (prefix) && id.GetChar (prefix.GetLength ()) == '.';
}


// ---------------------------------------------------------------------------
// Compare project and server classification trees (dual matching: ID + Name)
// ---------------------------------------------------------------------------

GS::Array<DiffEntry> CompareClassifications (
	const GS::Array<ClassificationTree>& project,
	const GS::Array<ClassificationTree>& server)
{
	GS::Array<DiffEntry> result;

	// Flatten both sides
	GS::Array<FlatItem> projectItems;
	for (UInt32 s = 0; s < project.GetSize (); s++)
		FlattenHelper (project[s].rootItems, projectItems, project[s].systemGuid);

	GS::Array<FlatItem> serverItems;
	for (UInt32 s = 0; s < server.GetSize (); s++)
		FlattenHelper (server[s].rootItems, serverItems, server[s].systemGuid);

	// Build server indexes: ID -> index, Name -> index
	GS::HashTable<GS::UniString, UInt32>  serverById;
	GS::HashTable<GS::UniString, UInt32>  serverByName;

	for (UInt32 j = 0; j < serverItems.GetSize (); j++) {
		if (!serverById.ContainsKey (serverItems[j].id))
			serverById.Add (serverItems[j].id, j);
		if (!serverByName.ContainsKey (serverItems[j].name))
			serverByName.Add (serverItems[j].name, j);
	}

	// Track which server items were consumed (matched) by project items
	GS::HashSet<UInt32>  consumedServerIndices;

	// Phase 1: classify each project item by dual matching
	for (UInt32 i = 0; i < projectItems.GetSize (); i++) {
		const FlatItem& pi = projectItems[i];

		DiffEntry entry;
		entry.id                = pi.id;
		entry.projectName       = pi.name;
		entry.description       = pi.description;
		entry.projectItemGuid   = pi.guid;
		entry.projectSystemGuid = pi.systemGuid;
		entry.cascadeChildCount = 0;

		UInt32 idxById   = (UInt32)-1;
		UInt32 idxByName = (UInt32)-1;
		bool matchedById   = serverById.Get (pi.id, &idxById);
		bool matchedByName = serverByName.Get (pi.name, &idxByName);

		if (matchedById && matchedByName) {
			if (idxById == idxByName) {
				// S1 — same server item found by both ID and Name
				entry.status     = DiffStatus::Match;
				entry.serverName = serverItems[idxById].name;
				entry.serverId   = serverItems[idxById].id;
				consumedServerIndices.Add (idxById);
			} else {
				// S6 — ID matches server item A, Name matches server item B
				entry.status     = DiffStatus::DoubleConflict;
				entry.serverName = serverItems[idxById].name;   // name from ID-matched item
				entry.serverId   = serverItems[idxByName].id;   // ID from Name-matched item
				consumedServerIndices.Add (idxById);
				consumedServerIndices.Add (idxByName);
			}
		} else if (matchedById && !matchedByName) {
			// S2 — same ID, different Name (ID collision)
			entry.status     = DiffStatus::IdCollision;
			entry.serverName = serverItems[idxById].name;
			entry.serverId   = serverItems[idxById].id;
			consumedServerIndices.Add (idxById);
		} else if (!matchedById && matchedByName) {
			// S3 — same Name, different ID (ID mismatch)
			entry.status     = DiffStatus::IdMismatch;
			entry.serverName = serverItems[idxByName].name;
			entry.serverId   = serverItems[idxByName].id;
			consumedServerIndices.Add (idxByName);
		} else {
			// S4 — no match at all, only in project
			entry.status = DiffStatus::OnlyInProject;
		}

		result.Push (entry);
	}

	// Phase 2: unconsumed server items -> S5 (OnlyInServer)
	for (UInt32 j = 0; j < serverItems.GetSize (); j++) {
		if (consumedServerIndices.Contains (j))
			continue;

		DiffEntry entry;
		entry.id                = serverItems[j].id;
		entry.serverId          = serverItems[j].id;
		entry.serverName        = serverItems[j].name;
		entry.description       = serverItems[j].description;
		entry.status            = DiffStatus::OnlyInServer;
		entry.projectItemGuid   = APINULLGuid;
		entry.projectSystemGuid = APINULLGuid;
		entry.cascadeChildCount = 0;
		result.Push (entry);
	}

	// Phase 3: detect ID cascades — if a parent is S3, its descendants
	// with matching prefix pattern are IdCascade instead of S4
	// Collect all S3 entries (IdMismatch) as cascade roots
	GS::Array<UInt32>  s3Indices;
	for (UInt32 i = 0; i < result.GetSize (); i++) {
		if (result[i].status == DiffStatus::IdMismatch)
			s3Indices.Push (i);
	}

	// For each S4 entry, check if its project ID starts with an S3 entry's project ID
	// and there's a corresponding server entry with the replaced prefix
	for (UInt32 i = 0; i < result.GetSize (); i++) {
		if (result[i].status != DiffStatus::OnlyInProject)
			continue;

		for (UInt32 k = 0; k < s3Indices.GetSize (); k++) {
			const DiffEntry& parent = result[s3Indices[k]];

			if (!HasPrefix (result[i].id, parent.id))
				continue;

			// This item's project ID starts with the S3 parent's project ID.
			// Build expected server ID by replacing the prefix.
			GS::UniString suffix = result[i].id.GetSubstring (parent.id.GetLength (),
				result[i].id.GetLength () - parent.id.GetLength ());
			GS::UniString expectedServerId = parent.serverId + suffix;

			// Check if there's a matching S5 (OnlyInServer) entry with that ID
			for (UInt32 j = 0; j < result.GetSize (); j++) {
				if (result[j].status != DiffStatus::OnlyInServer)
					continue;
				if (result[j].id != expectedServerId)
					continue;

				// Match found — convert S4 to IdCascade, remove S5
				result[i].status                   = DiffStatus::IdCascade;
				result[i].serverId                 = expectedServerId;
				result[i].serverName               = result[j].serverName;
				result[i].cascadeParentProjectId   = parent.id;
				result[i].cascadeParentServerId    = parent.serverId;

				// Mark S5 entry as consumed (set to Match so it won't show)
				result[j].status = DiffStatus::Match;
				result[j].projectName = result[j].serverName;

				// Increment cascade child count on the parent S3 entry
				result[s3Indices[k]].cascadeChildCount++;
				break;
			}
			break;  // found the cascade parent, no need to check others
		}
	}

	return result;
}
