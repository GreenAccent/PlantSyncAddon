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

void LogUseProject (const GS::UniString& xmlPath,
					const GS::UniString& itemId,
					const GS::UniString& serverName,
					const GS::UniString& projectName);

void LogUseServer  (const GS::UniString& xmlPath,
					const GS::UniString& itemId,
					const GS::UniString& projectName,
					const GS::UniString& serverName);

void LogImport     (const GS::UniString& xmlPath);


#endif // CHANGELOG_HPP
