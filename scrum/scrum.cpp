#include "pch.h"
#include "iostream"

#include <pugixml.hpp>
#include <tidy.h>
#include <tidybuffio.h>
#include <stdexcept>
#include <cpr/cpr.h>
#include <CLI/CLI.hpp>

std::string htmlToXml(const char* html);

int main(int argc, char **argv)
{
	std::string example = "span";

	CLI::App scraper{ "pog" };
	
	std::string url;
	std::string html_tag;
	std::string html_class;
	
	scraper.add_option("--url, --u", url, "the URL of the website to scrape");
	scraper.add_option("--tag, --t", html_tag, "the html tag to search for");
	scraper.add_option("--class, --c", html_class, "the class to search for");

	CLI11_PARSE(scraper, argc, argv);

	auto r = cpr::Get(cpr::Url{ url });
	if (r.status_code != 200) throw std::runtime_error("CPR Download failed\n");

	std::string xml;

	xml = htmlToXml(r.text.data());

	std::cout << "printing converted -> xml...\n";

	std::cout << xml;

	pugi::xml_document pugiDoc;

	pugi::xml_parse_result pugiResult = pugiDoc.load_buffer(xml.data(), xml.length());

	std::cout << pugiResult.description() << "\n";

	// example of accessing a node through navigating the tree
	// pugi::xml_node titleNode = pugiDoc.child("html").child("head").child("title"); 

	// find the node using the HTML tag provided in the command line using a lembda expression, this one will specifically find a node with a PCDATA child node (by checking if its NOT blank)
	const auto nodeFound = pugiDoc.find_node([&](auto x) { return x.name() == html_tag && (std::string)x.child_value() != ""; }); 

	std::cout << nodeFound.child_value();

	if (nodeFound.empty()) std::cout << "EMPTY\n";

	// saves the xml doc to a file
	// std::cout << pugiDoc.save_file("PATH");

	std::cout << "\n";

	exit(0);
}


std::string htmlToXml(const char* html)
{
	TidyBuffer output = { 0 };
	TidyBuffer errbuf = { 0 };
	int rc = -1;
	Bool ok;

	TidyDoc tdoc = tidyCreate();
	printf("Tidying:\t%s\n", html);

	ok = tidyOptSetBool(tdoc, TidyXhtmlOut, yes);
	if (ok) rc = tidySetErrorBuffer(tdoc, &errbuf);
	if (rc >= 0) rc = tidyParseString(tdoc, html);
	if (rc >= 0) rc = tidyCleanAndRepair(tdoc);
	if (rc >= 0) rc = tidyRunDiagnostics(tdoc);
	if (rc > 1) rc = (tidyOptSetBool(tdoc, TidyForceOutput, yes) ? rc : -1);
	if (rc >= 0) rc = tidySaveBuffer(tdoc, &output);

	if (rc >= 0)
	{
		if (rc > 0) printf("\nDiagnostics:\n\n%s", errbuf.bp);
		printf("\nAnd here is the result:\n\n%s", output.bp);
	}
	else
	{
		printf("A severe error (%d) occurred.\n", rc);
	}

	auto res = std::string((char*)output.bp);

	tidyBufFree(&output);
	tidyBufFree(&errbuf);
	tidyRelease(tdoc);

	if (rc != 0) throw std::runtime_error((char*)errbuf.bp);

	return res;
}