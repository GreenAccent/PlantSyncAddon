#ifndef CHANGELOG_HPP
#define CHANGELOG_HPP

#include "UniString.hpp"


// ---------------------------------------------------------------------------
// Changelog API - appends human-readable entries to daily log files
// Files are stored in a changelog/ subdirectory next to the XML file.
// ---------------------------------------------------------------------------

void LogExport     (const GS::UniString& xmlPath,
					const GS::UniString& itemId,
					const GS::UniString& itemName,
					const GS::UniString& parentId);

void LogUseServer  (const GS::UniString& xmlPath,
					const GS::UniString& itemId,
					const GS::UniString& projectName,
					const GS::UniString& serverName);

void LogUseServerId(const GS::UniString& xmlPath,
					const GS::UniString& oldId,
					const GS::UniString& newId,
					const GS::UniString& itemName);

void LogReassignId (const GS::UniString& xmlPath,
					const GS::UniString& oldId,
					const GS::UniString& newId,
					const GS::UniString& itemName);

void LogFixCascade (const GS::UniString& xmlPath,
					const GS::UniString& oldPrefix,
					const GS::UniString& newPrefix,
					UInt32 itemCount);

void LogImport     (const GS::UniString& xmlPath);


#endif // CHANGELOG_HPP
