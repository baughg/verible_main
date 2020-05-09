// verible_main.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <iostream>
#include "absl/flags/flag.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"  // for MakeArraySlice
#include "common/util/file_util.h"
#include "common/util/init_command_line.h"
#include "common/util/logging.h"  // for operator<<, LOG, LogMessage, etc
#include "common/util/iterator_range.h"


#include <algorithm>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>  // IWYU pragma: keep  // for ostringstream
#include <string>   // for string, allocator, etc
#include <vector>

#include "absl/flags/flag.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"  // for MakeArraySlice
#include "common/text/concrete_syntax_tree.h"
#include "common/text/parser_verifier.h"
#include "common/text/text_structure.h"
#include "common/text/token_info.h"
#include "common/util/file_util.h"
#include "common/util/init_command_line.h"
#include "common/util/logging.h"  // for operator<<, LOG, LogMessage, etc
#include "verilog/CST/verilog_tree_print.h"
#include "verilog/analysis/verilog_analyzer.h"
#include "verilog/parser/verilog_parser.h"

ABSL_FLAG(bool, printtree, false, "Whether or not to print the tree");
ABSL_FLAG(bool, printtokens, false, "Prints all lexed and filtered tokens");
ABSL_FLAG(bool, printrawtokens, false,
	"Prints all lexed tokens, including filtered ones.");
ABSL_FLAG(
	bool, verifytree, false,
	"Verifies that all tokens are parsed into tree, prints unmatched tokens");

using verible::ConcreteSyntaxTree;
using verible::ParserVerifier;
using verible::TextStructureView;

// Prints all tokens in view that are not matched in root.
static void VerifyParseTree(const TextStructureView& text_structure) {
	const ConcreteSyntaxTree& root = text_structure.SyntaxTree();
	if (root == nullptr) return;
	// TODO(fangism): this seems like a good method for TextStructureView.
	ParserVerifier verifier(*root, text_structure.GetTokenStreamView());
	auto unmatched = verifier.Verify();

	if (unmatched.empty()) {
		std::cout << std::endl << "All tokens matched." << std::endl;
	}
	else {
		std::cout << std::endl << "Unmatched Tokens:" << std::endl;
		for (const auto& token : unmatched) {
			std::cout << token << std::endl;
		}
	}
}

static int AnalyzeOneFile(absl::string_view content,
	absl::string_view filename) {
	int exit_status = 0;
	const auto analyzer =
		verilog::VerilogAnalyzer::AnalyzeAutomaticMode(content, filename);
	const auto lex_status = ABSL_DIE_IF_NULL(analyzer)->LexStatus();
	const auto parse_status = analyzer->ParseStatus();
	if (!lex_status.ok() || !parse_status.ok()) {
		const std::vector<std::string> syntax_error_messages(
			analyzer->LinterTokenErrorMessages());
		for (const auto& message : syntax_error_messages) {
			std::cout << message << std::endl;
		}
		exit_status = 1;
	}
	const bool parse_ok = parse_status.ok();

	const verible::TokenInfo::Context context(
		analyzer->Data().Contents(), [](std::ostream& stream, int e) {
		stream << verilog::verilog_symbol_name(e);
	});
	// Check for printtokens flag, print all filtered tokens if on.
	if (absl::GetFlag(FLAGS_printtokens)) {
		std::cout << std::endl << "Lexed and filtered tokens: " << std::endl;
		for (const auto& t : analyzer->Data().GetTokenStreamView()) {
			t->ToStream(std::cout, context) << std::endl;
		}
	}

	// Check for printrawtokens flag, print all tokens if on.
	if (absl::GetFlag(FLAGS_printrawtokens)) {
		std::cout << std::endl << "All lexed tokens: " << std::endl;
		for (const auto& t : analyzer->Data().TokenStream()) {
			t.ToStream(std::cout, context) << std::endl;
		}
	}

	const auto& text_structure = analyzer->Data();
	const auto& syntax_tree = text_structure.SyntaxTree();

	// check for printtree flag, and print tree if on
	if (absl::GetFlag(FLAGS_printtree) && syntax_tree != nullptr) {
		std::cout << std::endl
			<< "Parse Tree"
			<< (!parse_ok ? " (incomplete due to syntax errors): " : ": ")
			<< std::endl;
		verilog::PrettyPrintVerilogTree(*syntax_tree, analyzer->Data().Contents(),
			&std::cout);
	}

	// Check for verifytree, verify tree and print unmatched if on.
	if (absl::GetFlag(FLAGS_verifytree)) {
		if (!parse_ok) {
			std::cout << std::endl
				<< "Note: verifytree will fail because syntax errors caused "
				"sections of text to be dropped during error-recovery."
				<< std::endl;
		}
		VerifyParseTree(text_structure);
	}

	return exit_status;
}

int main(int argc, char** argv) {
	const auto usage =
		absl::StrCat("usage: ", argv[0], " [options] <file> [<file>...]");
	const auto args = verible::InitCommandLine(usage, &argc, &argv);

	int exit_status = 0;
	// All positional arguments are file names.  Exclude program name.
	for (const auto filename :
		verible::make_range(args.begin() + 1, args.end())) {
		std::string content;
		if (!verible::file::GetContents(filename, &content)) {
			exit_status = 1;
			continue;
		}

		int file_status = AnalyzeOneFile(content, filename);
		exit_status = std::max(exit_status, file_status);
	}
}

	