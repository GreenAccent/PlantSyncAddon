#include "XmlWriter.hpp"

#include <fstream>
#include <string>
#include <sstream>


// ---------------------------------------------------------------------------
// Helper: convert GS::UniString to UTF-8 std::string
// ---------------------------------------------------------------------------

static std::string ToUtf8 (const GS::UniString& us)
{
	if (us.IsEmpty ())
		return "";
	return std::string (us.ToCStr (0, MaxUSize, CC_UTF8).Get ());
}


// ---------------------------------------------------------------------------
// Helper: escape XML special characters
// ---------------------------------------------------------------------------

static std::string EscapeXml (const std::string& s)
{
	std::string result;
	result.reserve (s.size ());
	for (char c : s) {
		switch (c) {
			case '&':  result += "&amp;";  break;
			case '<':  result += "&lt;";   break;
			case '>':  result += "&gt;";   break;
			case '"':  result += "&quot;"; break;
			case '\'': result += "&apos;"; break;
			default:   result += c;        break;
		}
	}
	return result;
}


// ---------------------------------------------------------------------------
// Helper: read entire file into string
// ---------------------------------------------------------------------------

static bool ReadFile (const char* filePath, std::string& content)
{
	std::ifstream file (filePath, std::ios::binary);
	if (!file.is_open ())
		return false;
	std::ostringstream ss;
	ss << file.rdbuf ();
	content = ss.str ();
	file.close ();
	return true;
}


// ---------------------------------------------------------------------------
// Helper: write string to file
// ---------------------------------------------------------------------------

static bool WriteFile (const char* filePath, const std::string& content)
{
	std::ofstream file (filePath, std::ios::binary | std::ios::trunc);
	if (!file.is_open ())
		return false;
	file << content;
	file.close ();
	return true;
}


// ---------------------------------------------------------------------------
// Helper: detect indentation at a given position (count tabs before tag)
// ---------------------------------------------------------------------------

static std::string DetectIndent (const std::string& xml, size_t tagPos)
{
	std::string indent;
	size_t pos = tagPos;
	while (pos > 0) {
		pos--;
		if (xml[pos] == '\t')
			indent = "\t" + indent;
		else if (xml[pos] == ' ')
			indent = " " + indent;
		else
			break;
	}
	return indent;
}


// ---------------------------------------------------------------------------
// Build an <Item> XML block
// ---------------------------------------------------------------------------

static std::string BuildItemXml (const ClassificationNode& node,
								 const std::string& indent)
{
	std::string id   = EscapeXml (ToUtf8 (node.id));
	std::string name = EscapeXml (ToUtf8 (node.name));
	std::string desc = EscapeXml (ToUtf8 (node.description));

	std::string xml;
	xml += indent + "<Item>\n";
	xml += indent + "\t<ID>" + id + "</ID>\n";
	xml += indent + "\t<Name>" + name + "</Name>\n";
	if (desc.empty ())
		xml += indent + "\t<Description/>\n";
	else
		xml += indent + "\t<Description>" + desc + "</Description>\n";
	xml += indent + "\t<Children/>\n";
	xml += indent + "</Item>\n";
	return xml;
}


// ---------------------------------------------------------------------------
// Change an item's <Name> in the XML file
// ---------------------------------------------------------------------------

bool ChangeItemNameInXml (const char* filePath,
						  const GS::UniString& itemId,
						  const GS::UniString& newName)
{
	std::string content;
	if (!ReadFile (filePath, content))
		return false;

	std::string idStr = ToUtf8 (itemId);
	std::string nameStr = EscapeXml (ToUtf8 (newName));
	std::string idTag = "<ID>" + idStr + "</ID>";

	auto idPos = content.find (idTag);
	if (idPos == std::string::npos)
		return false;

	// Find <Name>...</Name> after the ID tag
	auto nameOpen = content.find ("<Name>", idPos);
	auto nameClose = content.find ("</Name>", idPos);
	if (nameOpen == std::string::npos || nameClose == std::string::npos)
		return false;

	size_t replaceStart = nameOpen + 6;  // strlen("<Name>")
	content.replace (replaceStart, nameClose - replaceStart, nameStr);

	return WriteFile (filePath, content);
}


// ---------------------------------------------------------------------------
// Add a new <Item> to the XML file
// ---------------------------------------------------------------------------

bool AddItemToXml (const char* filePath,
				   const GS::UniString& parentId,
				   const ClassificationNode& node)
{
	std::string content;
	if (!ReadFile (filePath, content))
		return false;

	if (parentId.IsEmpty ()) {
		// Add as root item under <Items>
		auto itemsClose = content.rfind ("</Items>");
		if (itemsClose == std::string::npos)
			return false;

		std::string indent = DetectIndent (content, itemsClose) + "\t";
		std::string itemXml = BuildItemXml (node, indent);
		content.insert (itemsClose, itemXml);

	} else {
		// Find the parent item by ID
		std::string parentIdStr = ToUtf8 (parentId);
		std::string parentIdTag = "<ID>" + parentIdStr + "</ID>";
		auto parentPos = content.find (parentIdTag);
		if (parentPos == std::string::npos)
			return false;

		// Find <Children.../> or <Children>...</Children> after the parent ID
		auto childrenSelfClose = content.find ("<Children/>", parentPos);
		auto childrenOpen = content.find ("<Children>", parentPos);

		// Use whichever comes first (and is before the parent's </Item>)
		auto parentClose = content.find ("</Item>", parentPos);

		if (childrenSelfClose != std::string::npos &&
			childrenSelfClose < parentClose &&
			(childrenOpen == std::string::npos || childrenSelfClose < childrenOpen))
		{
			// Self-closing <Children/> - replace with <Children>...<Item>...</Item>...</Children>
			std::string indent = DetectIndent (content, childrenSelfClose);
			std::string replacement = "<Children>\n";
			replacement += BuildItemXml (node, indent + "\t");
			replacement += indent + "</Children>";
			content.replace (childrenSelfClose, 11, replacement);  // 11 = strlen("<Children/>")

		} else if (childrenOpen != std::string::npos && childrenOpen < parentClose) {
			// Existing <Children>...</Children> - insert before </Children>
			// Find the matching </Children>
			auto childrenClose = content.find ("</Children>", childrenOpen);
			if (childrenClose == std::string::npos)
				return false;

			std::string indent = DetectIndent (content, childrenClose) + "\t";
			std::string itemXml = BuildItemXml (node, indent);
			content.insert (childrenClose, itemXml);

		} else {
			return false;
		}
	}

	return WriteFile (filePath, content);
}
