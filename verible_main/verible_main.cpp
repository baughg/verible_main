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
	}
}

	