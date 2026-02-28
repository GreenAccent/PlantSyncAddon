#include "XmlReader.hpp"

#include <fstream>
#include <string>
#include <sstream>


// ---------------------------------------------------------------------------
// Helper: extract text between <tag> and </tag>
// ---------------------------------------------------------------------------

static std::string ExtractTag (const std::string& xml, const std::string& tag,
							   size_t from = 0)
{
	std::string openTag  = "<" + tag + ">";
	std::string closeTag = "</" + tag + ">";
	auto start = xml.find (openTag, from);
	if (start == std::string::npos)
		return "";
	start += openTag.size ();
	auto end = xml.find (closeTag, start);
	if (end == std::string::npos)
		return "";
	return xml.substr (start, end - start);
}


// ---------------------------------------------------------------------------
// Helper: find matching </tag> accounting for nesting
// ---------------------------------------------------------------------------

static size_t FindMatchingClose (const std::string& xml, const std::string& tag,
								 size_t afterOpen)
{
	std::string openTag  = "<" + tag + ">";
	std::string closeTag = "</" + tag + ">";
	int depth = 1;
	size_t pos = afterOpen;

	while (depth > 0 && pos < xml.size ()) {
		auto nextOpen  = xml.find (openTag, pos);
		auto nextClose = xml.find (closeTag, pos);
		if (nextClose == std::string::npos)
			return std::string::npos;
		if (nextOpen != std::string::npos && nextOpen < nextClose) {
			depth++;
			pos = nextOpen + openTag.size ();
		} else {
			depth--;
			if (depth == 0)
				return nextClose;
			pos = nextClose + closeTag.size ();
		}
	}
	return std::string::npos;
}


// ---------------------------------------------------------------------------
// Convert UTF-8 std::string to GS::UniString
// ---------------------------------------------------------------------------

static GS::UniString ToUniStr (const std::string& s)
{
	if (s.empty ())
		return GS::UniString ();
	return GS::UniString (s.c_str (), CC_UTF8);
}


// ---------------------------------------------------------------------------
// Recursively parse <Item> elements
// ---------------------------------------------------------------------------

static void ParseItems (const std::string& xml, GS::Array<ClassificationNode>& result)
{
	std::string openItem = "<Item>";
	size_t pos = 0;

	while (true) {
		auto start = xml.find (openItem, pos);
		if (start == std::string::npos)
			break;

		size_t contentStart = start + openItem.size ();
		auto end = FindMatchingClose (xml, "Item", contentStart);
		if (end == std::string::npos)
			break;

		std::string itemXml = xml.substr (contentStart, end - contentStart);

		// Extract fields from portion before <Children> to avoid matching nested items
		auto childrenPos = itemXml.find ("<Children");
		std::string headerPart = (childrenPos != std::string::npos)
			? itemXml.substr (0, childrenPos)
			: itemXml;

		ClassificationNode node;
		node.id          = ToUniStr (ExtractTag (headerPart, "ID"));
		node.name        = ToUniStr (ExtractTag (headerPart, "Name"));
		node.description = ToUniStr (ExtractTag (headerPart, "Description"));

		// Parse children recursively (must use nesting-aware search
		// because Children tags are nested: Item/Children/Item/Children/...)
		auto childrenOpenPos = itemXml.find ("<Children>");
		if (childrenOpenPos != std::string::npos) {
			size_t childrenContentStart = childrenOpenPos + 10;  // strlen("<Children>")
			auto childrenClosePos = FindMatchingClose (itemXml, "Children", childrenContentStart);
			if (childrenClosePos != std::string::npos) {
				std::string childrenXml = itemXml.substr (childrenContentStart,
					childrenClosePos - childrenContentStart);
				if (!childrenXml.empty ())
					ParseItems (childrenXml, node.children);
			}
		}

		result.Push (node);
		pos = end + 7;  // skip past "</Item>"
	}
}


// ---------------------------------------------------------------------------
// Read classifications from an ArchiCAD XML file
// ---------------------------------------------------------------------------

GS::Array<ClassificationTree> ReadXmlClassifications (const char* filePath)
{
	GS::Array<ClassificationTree> result;

	// Read file
	std::ifstream file (filePath, std::ios::binary);
	if (!file.is_open ()) {
		ACAPI_WriteReport ("ClassSync: Cannot open XML file: %s", false, filePath);
		return result;
	}

	std::ostringstream ss;
	ss << file.rdbuf ();
	std::string content = ss.str ();
	file.close ();

	ACAPI_WriteReport ("ClassSync: Read XML file, %d bytes", false, (int)content.size ());

	// Find all <System> blocks
	std::string openSystem  = "<System>";
	std::string closeSystem = "</System>";
	size_t pos = 0;

	while (true) {
		auto sysStart = content.find (openSystem, pos);
		if (sysStart == std::string::npos)
			break;

		size_t sysContentStart = sysStart + openSystem.size ();
		auto sysEnd = content.find (closeSystem, sysContentStart);
		if (sysEnd == std::string::npos)
			break;

		std::string sysXml = content.substr (sysContentStart, sysEnd - sysContentStart);

		ClassificationTree tree;

		// Extract system name and version from content before <Items>
		auto itemsPos = sysXml.find ("<Items>");
		std::string sysHeader = (itemsPos != std::string::npos)
			? sysXml.substr (0, itemsPos)
			: sysXml;

		tree.systemName = ToUniStr (ExtractTag (sysHeader, "Name"));
		tree.version    = ToUniStr (ExtractTag (sysHeader, "EditionVersion"));

		// Parse items
		std::string itemsXml = ExtractTag (sysXml, "Items");
		if (!itemsXml.empty ())
			ParseItems (itemsXml, tree.rootItems);

		ACAPI_WriteReport ("ClassSync: Parsed system '%s' v%s, %d root items", false,
			ExtractTag (sysHeader, "Name").c_str (),
			ExtractTag (sysHeader, "EditionVersion").c_str (),
			(int)tree.rootItems.GetSize ());

		result.Push (tree);
		pos = sysEnd + closeSystem.size ();
	}

	return result;
}
