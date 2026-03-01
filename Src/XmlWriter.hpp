#ifndef XMLWRITER_HPP
#define XMLWRITER_HPP

#include "ClassificationData.hpp"


// ---------------------------------------------------------------------------
// Change an item's <Name> in the XML file, found by <ID>
// ---------------------------------------------------------------------------

bool ChangeItemNameInXml (const char* filePath,
						  const GS::UniString& itemId,
						  const GS::UniString& newName);


// ---------------------------------------------------------------------------
// Add a new <Item> block inside a parent's <Children> section.
// If parentId is empty, adds under the system's <Items> section.
// ---------------------------------------------------------------------------

bool AddItemToXml (const char* filePath,
				   const GS::UniString& parentId,
				   const ClassificationNode& node);


// ---------------------------------------------------------------------------
// Change an item's <Name> and <Description> in the XML file, found by <ID>
// ---------------------------------------------------------------------------

bool ChangeItemInXml (const char* filePath,
					  const GS::UniString& itemId,
					  const GS::UniString& newName,
					  const GS::UniString& newDescription);


#endif // XMLWRITER_HPP
